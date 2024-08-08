#include "shm_manager.hpp"

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <iostream>
#include <string>
#include <arrow/array.h>
#include <arrow/builder.h>
#include <arrow/ipc/api.h>
#include <arrow/ipc/writer.h>
#include <arrow/io/api.h>
#include <arrow/io/memory.h>
#include <arrow/table.h>
#include <arrow/status.h>

int main()
{
  // create shared memory and lock
  SharedMemoryManagerTMP manager("MySharedMemory");
  bi::interprocess_mutex *mtx = manager.segment.construct<bi::interprocess_mutex>("mtx")();
  bi::interprocess_condition *cnd = manager.segment.construct<bi::interprocess_condition>("cnd")();
  bi::scoped_lock<bi::interprocess_mutex> lock(*mtx);


  // create table data
  arrow::Int64Builder builder;
  builder.AppendValues({1, 2, 3, 4, 5});
  std::shared_ptr<arrow::Array> array1;
  builder.Finish(&array1);

  builder.Reset();
  builder.AppendValues({6, 7, 8, 9, 10});
  std::shared_ptr<arrow::Array> array2;
  builder.Finish(&array2);

  auto schema = arrow::schema({arrow::field("a", arrow::int64()), arrow::field("b", arrow::int64())});
  std::shared_ptr<arrow::Table> my_table = arrow::Table::Make(schema, {array1, array2});
  {
    // transform the table to buffer
    std::shared_ptr<arrow::Buffer> buffer;
    std::shared_ptr<arrow::io::BufferOutputStream> stream = arrow::io::BufferOutputStream::Create(1024 * 1024).ValueOrDie();
    std::shared_ptr<arrow::ipc::RecordBatchWriter> writer;
    arrow::Result<std::shared_ptr<arrow::ipc::RecordBatchWriter>> result1 = arrow::ipc::MakeStreamWriter(stream, my_table->schema());
    if (result1.ok())
    {
      writer = result1.ValueOrDie();
    }
    writer->WriteTable(*my_table);
    writer->Close();
    arrow::Result<std::shared_ptr<arrow::Buffer>> result2 = stream->Finish();
    if (result2.ok())
    {
      buffer = result2.ValueOrDie();
    }

    // write the table to share memory
    char *shm_ptr = manager.segment.construct<char>("MyTable")[buffer->size()]();
    std::memcpy(shm_ptr, buffer->data(), buffer->size());
    std::cout << "[Client] finish write the table buffer in the shared memory\n";
    for (int i = 0; i < my_table->num_columns(); i++)
    {
      std::shared_ptr<arrow::ChunkedArray> column = my_table->column(i);
      std::cout << my_table->field(i)->name() << ": ";
      for (int chunk_idx = 0; chunk_idx < column->num_chunks(); chunk_idx++)
      {
        auto chunk = std::static_pointer_cast<arrow::Int64Array>(column->chunk(chunk_idx));
        for (int j = 0; j < chunk->length(); j++)
        {
          std::cout << chunk->Value(j) << " ";
        }
      }
      std::cout << std::endl;
    }
  }
  // wait for the server to read the table
  cnd->notify_all();
  cnd->wait(lock);
  {
    std::cout << "[Client] OK I got it\n";
    // recieve the server result
    char *shm_ptr = manager.segment.find<char>("MyResult").first;
    std::size_t size = manager.segment.find<char>("MyResult").second;

    // transform the buffer to table
    std::shared_ptr<arrow::Buffer> buffer = arrow::Buffer::Wrap(shm_ptr, size);
    std::shared_ptr<arrow::io::InputStream> input = std::make_shared<arrow::io::BufferReader>(buffer);
    arrow::Result<std::shared_ptr<arrow::ipc::RecordBatchReader>> result = arrow::ipc::RecordBatchStreamReader::Open(input);
    std::shared_ptr<arrow::ipc::RecordBatchReader> reader = *result;
    std::vector<std::shared_ptr<arrow::RecordBatch>> batches;
    while (true)
    {
      arrow::Result<arrow::RecordBatchWithMetadata> batch_result = reader->ReadNext();
      if (!batch_result.ok())
      {
        std::cerr << "[Client] Failed to read next record batch: " << batch_result.status() << std::endl;
        return 1;
      }

      std::shared_ptr<arrow::RecordBatch> batch = batch_result.ValueOrDie().batch;
      if (batch == nullptr)
      {
        break;
      }
      batches.push_back(batch);
    }

    arrow::Result<std::shared_ptr<arrow::Table>> table_result = arrow::Table::FromRecordBatches(reader->schema(), batches);
    my_table = *table_result;

    // read the result table
    std::cout<<"[Client] The result table is:\n";
    for (int i = 0; i < my_table->num_columns(); i++)
    {
      std::shared_ptr<arrow::ChunkedArray> column = my_table->column(i);
      std::cout << my_table->field(i)->name() << ": ";
      for (int chunk_idx = 0; chunk_idx < column->num_chunks(); chunk_idx++)
      {
        auto chunk = std::static_pointer_cast<arrow::Int64Array>(column->chunk(chunk_idx));
        for (int j = 0; j < chunk->length(); j++)
        {
          std::cout << chunk->Value(j) << " ";
        }
      }
      std::cout << std::endl;
    }
  }
  //  close the server
  cnd->notify_all();
  // destroy the shared memory
  bi::shared_memory_object::remove("MySharedMemory");
  return 0;
}