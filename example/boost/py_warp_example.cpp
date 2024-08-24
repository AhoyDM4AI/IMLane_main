#include <Python.h>
#include <arrow/array.h>
#include <arrow/builder.h>
#include <arrow/ipc/api.h>
#include <arrow/ipc/writer.h>
#include <arrow/io/api.h>
#include <arrow/io/memory.h>
#include <arrow/table.h>
#include <arrow/status.h>
#include <arrow/python/pyarrow.h>

#include <iostream>

int main()
{
    std::cout << "Arrow version: " << ARROW_VERSION << std::endl;
    Py_Initialize();
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    if (arrow::py::import_pyarrow())
    {
        std::cout << "import pyarrow error \n";
        exit(0);
    }
    printf("gan ni niang \n");
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

    const char *python_code = R"(
import pyarrow as pa

def process_table(table):
    col1 = table.column(0).to_pylist()
    col2 = table.column(1).to_pylist()
    result = [a + b for a, b in zip(col1, col2)]
    result_field = pa.field('result', pa.int64())
    result_schema = pa.schema([result_field])
    result_array = pa.array(result, type=pa.int64())
    result_table = pa.Table.from_arrays([result_array], schema=result_schema)
    print(result_table)
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

    PyRun_SimpleString(python_code);

    PyObject *py_table_tmp = arrow::py::wrap_table(my_table);

    PyObject *main_module = PyImport_AddModule("__main__");
    PyObject *main_dict = PyModule_GetDict(main_module);
    PyObject *MyTable = PyDict_GetItemString(main_dict, "MyTable");

    PyObject *args = PyTuple_Pack(1, py_table_tmp);
    PyObject *my_table_instance = PyObject_CallObject(MyTable, args);
    // Py_DECREF(my_table_instance);
    // std::cout<<"step1______________________\n";
    PyObject *result = PyObject_CallMethod(my_table_instance, "process", "O", py_table_tmp);
    // std::cout<<"step2______________________\n";

    

    // PyObject *func = PyDict_GetItemString(main_dict, "process_table");
    // PyObject *args = PyTuple_Pack(1, py_table_tmp);
    // PyObject *result = PyObject_CallObject(func, args);
    // std::cout<<"step3______________________\n";
    std::cout<<"result no null: " <<(result != NULL)<<std::endl;
    // std::cout<<"step4______________________\n";

    // if(result == NULL){
    //     PyErr_Print();
    // }
    PyGILState_Release(gstate);
    Py_Finalize();
    return 0;
}