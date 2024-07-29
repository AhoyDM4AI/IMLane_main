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
    /* version 1
    boost::interprocess::managed_shared_memory managed_shm(boost::interprocess::open_only, "IPC_CHENELL");
    typedef boost::interprocess::allocator<char, boost::interprocess::managed_shared_memory::segment_manager> CharAllocator;
    typedef boost::interprocess::basic_string<char, std::char_traits<char>, CharAllocator> string;

    boost::interprocess::interprocess_mutex *mtx = managed_shm.find_or_construct<boost::interprocess::interprocess_mutex>("mtx")();
    boost::interprocess::interprocess_condition *cnd = managed_shm.find_or_construct<boost::interprocess::interprocess_condition>("cnd")();
    boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mtx);

    int i = 10;
    while (i--)
    {
        std::pair<string *, size_t> p = managed_shm.find<string>("CODE");
        std::cout << "[Server] get  : " << *p.first << std::endl;

        string * s = p.first;
        std::string tmp = std::to_string(i);
        string tmp_shm_str(tmp.begin(), tmp.end(), s->get_allocator());
        s->insert(s->size(), tmp_shm_str);
        std::cout << "[Server] sned : " << *s << std::endl;
        std::cout<<"\n";
        cnd->notify_all();
        cnd->wait(lock);
    }
    */

    SharedMemoryManagerTMP manager("MySharedMemory");
    bi::interprocess_mutex *mtx = manager.segment.find<bi::interprocess_mutex>("mtx").first;
    bi::interprocess_condition *cnd = manager.segment.find<bi::interprocess_condition>("cnd").first;
    bi::scoped_lock<bi::interprocess_mutex> lock(*mtx);

    char *shm_ptr = manager.segment.find<char>("MyTable").first;
    std::size_t size = manager.segment.find<char>("MyTable").second;

    std::shared_ptr<arrow::Buffer> buffer = arrow::Buffer::Wrap(shm_ptr, size);

    std::shared_ptr<arrow::io::InputStream> input = std::make_shared<arrow::io::BufferReader>(buffer);
    arrow::Result<std::shared_ptr<arrow::ipc::RecordBatchReader>> result = arrow::ipc::RecordBatchStreamReader::Open(input);

    if (!result.ok())
    {
        std::cerr << "Failed to open RecordBatchStreamReader: " << result.status() << std::endl;
        return 1;
    }

    std::shared_ptr<arrow::ipc::RecordBatchReader> reader = *result;

    std::vector<std::shared_ptr<arrow::RecordBatch>> batches;
    while (true)
    {
        arrow::Result<arrow::RecordBatchWithMetadata> batch_result = reader->ReadNext();
        if (!batch_result.ok())
        {
            std::cerr << "Failed to read next record batch: " << batch_result.status() << std::endl;
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

    if (!table_result.ok())
    {
        std::cerr << "Failed to create table from record batches: " << table_result.status() << std::endl;
        return 1;
    }

    std::shared_ptr<arrow::Table> my_table = *table_result;
    std::cout << "I got the table from the shared memory \n";
    // parse the data from shared memory
    std::shared_ptr<arrow::ChunkedArray> column1 = my_table->column(0);
    std::shared_ptr<arrow::ChunkedArray> column2 = my_table->column(1);

    // compute
    arrow::Int64Builder sum_builder;
    for (int chunk_idx = 0; chunk_idx < column1->num_chunks(); chunk_idx++)
    {
        auto chunk1 = std::static_pointer_cast<arrow::Int64Array>(column1->chunk(chunk_idx));
        auto chunk2 = std::static_pointer_cast<arrow::Int64Array>(column2->chunk(chunk_idx));
        for (int i = 0; i < chunk1->length(); i++)
        {
            sum_builder.Append(chunk1->Value(i) + chunk2->Value(i));
        }
    }
    std::shared_ptr<arrow::Array> sum_array;
    sum_builder.Finish(&sum_array);
    std::shared_ptr<arrow::Field> sum_field = arrow::field("sum", arrow::int64());
    std::shared_ptr<arrow::ChunkedArray> sum_column = std::make_shared<arrow::ChunkedArray>(sum_array);
    auto result2 = my_table->AddColumn(my_table->num_columns(), sum_field, sum_column);
    my_table = *result2;
    std::cout << my_table->num_columns() << "    finished compute \n";

    // print compute result 
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

    // create new buffer
    std::shared_ptr<arrow::io::BufferOutputStream> stream = arrow::io::BufferOutputStream::Create(1024 * 1024).ValueOrDie();
    std::shared_ptr<arrow::ipc::RecordBatchWriter> writer;
    arrow::Result<std::shared_ptr<arrow::ipc::RecordBatchWriter>> result1 = arrow::ipc::MakeStreamWriter(stream, my_table->schema());
    if (result1.ok())
    {
        writer = result1.ValueOrDie();
    }
    writer->WriteTable(*my_table);
    writer->Close();
    arrow::Result<std::shared_ptr<arrow::Buffer>> result3 = stream->Finish();
    if (result3.ok())
    {
        buffer = result3.ValueOrDie();
    }

    shm_ptr = manager.segment.construct<char>("MyResult")[buffer->size()]();
    std::memcpy(shm_ptr, buffer->data(), buffer->size());
    std::cout << "finish write the table buffer in the shared memory\n";
    cnd->notify_all();
    cnd->wait(lock);
    return 0;
}