#include "shm_manager.hpp"

#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <iostream>
#include <string>
#include <thread>
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
  // create shared memory and semaphore
  SharedMemoryManagerTMP manager("MySharedMemory");
  bi::interprocess_semaphore *sem_a = manager.segment.construct<bi::interprocess_semaphore>("semA")(0);
  bi::interprocess_semaphore *sem_b = manager.segment.construct<bi::interprocess_semaphore>("semB")(0);
  std::thread t([]()
                { std::system("./server"); });
  t.detach();
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
  std::string python_code = R"(
import pyarrow as pa
import torch

def process_table(table):
    a1 = torch.rand([3,4,5])
    a2 = torch.rand([3,4,5])
    rrr = a1*a2
    print(rrr.shape)
    col1 = table.column(0).to_pylist()
    col2 = table.column(1).to_pylist()
    result = [a + b for a, b in zip(col1, col2)]
    result_field = pa.field('result', pa.int64())
    result_schema = pa.schema([result_field])
    result_array = pa.array(result, type=pa.int64())
    result_table = pa.Table.from_arrays([result_array], schema=result_schema)
    return result_table

class MyTable:
    def __init__(self, table):
        self.table = table
        self.name = "MyTable"
        self.age = 18


    def process(self, table):
        # print("2333")
        print(self.name)
        print(self.age)
        return process_table(table)
)";
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
    char *shm_str_ptr = manager.segment.construct<char>("MyCode")[python_code.size()]();
    std::memcpy(shm_str_ptr, python_code.c_str(), python_code.size());
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
  sem_b->post();
  sem_a->wait();
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
    std::cout << "[Client] The result table is:\n";
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

  // destroy the shared memory
  bi::shared_memory_object::remove("MySharedMemory");
  return 0;
}