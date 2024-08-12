#include "shm_manager.hpp"

#include <boost/interprocess/sync/interprocess_semaphore.hpp>
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
#include <arrow/python/pyarrow.h>

int main()
{
    std::cout<< "[Server] start the server program\n";
    // create shared memory and lock
    SharedMemoryManagerTMP manager("MySharedMemory");
    bi::interprocess_semaphore *sem_a= manager.segment.find<bi::interprocess_semaphore>("semA").first;
    bi::interprocess_semaphore *sem_b= manager.segment.find<bi::interprocess_semaphore>("semB").first;
    sem_b->wait();

    // get the client table from the shared memory
    char *shm_ptr = manager.segment.find<char>("MyTable").first;
    std::size_t size = manager.segment.find<char>("MyTable").second;

    // transform the buffer to table
    std::shared_ptr<arrow::Buffer> buffer = arrow::Buffer::Wrap(shm_ptr, size);

    // read the table from the buffer
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
    std::cout << "[Server] I got the table from the shared memory 1\n";

    // using python to process the table
    if (!Py_IsInitialized())
    {
        Py_Initialize();
    }
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    if (arrow::py::import_pyarrow())
    {
        std::cout << "import pyarrow error!\n";
        exit(0);
    }

    PyObject *py_table_tmp = arrow::py::wrap_table(std::move(my_table));

    PyObject *py_main = PyImport_AddModule("__main__");
    PyObject *py_dict = PyModule_GetDict(py_main);

    const char *python_code = R"(
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
)";

    PyRun_SimpleString(python_code);

    PyObject *main_module = PyImport_AddModule("__main__");
    PyObject *main_dict = PyModule_GetDict(main_module);
    PyObject *func = PyDict_GetItemString(main_dict, "process_table");

    PyObject *args = PyTuple_Pack(1, py_table_tmp);
    PyObject *py_result = PyObject_CallObject(func, args);


    if (py_result != NULL)

    {
        std::cout << "you can come here!\n";
        arrow::Result<std::shared_ptr<arrow::Table>> result_py_table = arrow::py::unwrap_table(py_result);
        if (result_py_table.ok())
        {
            my_table = *result_py_table;
        }
        else
        {
            std::cerr << "Failed to convert result to arrow::Table: " << result_py_table.status() << std::endl;
            return 1;
        }
    }
    else
    {
        PyErr_Print();
        std::cout << "you can unwarp it\n";
        my_table = arrow::py::unwrap_table(py_table_tmp).ValueOrDie();
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
    std::cout << "[Server] finish write the table buffer in the shared memory\n";
    sem_a->post();
    PyGILState_Release(gstate);
    return 0;
}