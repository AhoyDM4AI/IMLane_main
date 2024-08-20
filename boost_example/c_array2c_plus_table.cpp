#include "shm_manager.hpp"

#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <iostream>
#include <string>
#include <thread>

#include <arrow/api.h>
#include <arrow/status.h>
#include <arrow/c/abi.h>
#include <arrow/c/bridge.h>
#include <arrow/c/helpers.h>

void NoOpSchemaRelease(struct ArrowSchema *schema) { ArrowSchemaMarkReleased(schema); }
void NoOpArrayRelease(struct ArrowArray *schema) { ArrowArrayMarkReleased(schema); }

int main()
{
    int64_t values1[] = {1, 2, 3, 4, 5};
    int64_t values2[] = {6, 7, 8, 9, 10};

    static const void *buffer1[2] = {nullptr, values1};
    static const void *buffer2[2] = {nullptr, values2};

    struct ArrowSchema schema1;
    struct ArrowSchema schema2;

    struct ArrowSchema parent_schema;

    // 设置模式
    memset(&schema1, 0, sizeof(schema1));
    schema1.release = NoOpSchemaRelease;
    schema1.format = "l";
    schema1.flags = ARROW_FLAG_NULLABLE;
    schema1.name = "f1";

    memset(&schema2, 0, sizeof(schema2));
    schema2.release = NoOpSchemaRelease;
    schema2.format = "l";
    schema2.flags = ARROW_FLAG_NULLABLE;
    schema2.name = "f2";

    memset(&parent_schema, 0, sizeof(parent_schema));
    parent_schema.release = NoOpSchemaRelease;
    parent_schema.format = "+s";
    parent_schema.n_children = 2;
    parent_schema.children = (ArrowSchema **)malloc(parent_schema.n_children * sizeof(ArrowSchema *));
    parent_schema.children[0] = &schema1;
    parent_schema.children[1] = &schema2;

    struct ArrowArray array1;
    struct ArrowArray array2;
    // 设置数组值
    memset(&array1, 0, sizeof(array1));
    array1.release = NoOpArrayRelease;
    array1.length = 5;
    array1.null_count = 0;
    array1.offset = 0;
    array1.n_buffers = 2;
    array1.buffers = buffer1;

    memset(&array2, 0, sizeof(array2));
    array2.release = NoOpArrayRelease;
    array2.length = 5;
    array2.null_count = 0;
    array2.offset = 0;
    array2.n_buffers = 2;
    array2.buffers = buffer2;

    // 创建表
    ArrowArray parent_array;

    memset(&parent_array, 0, sizeof(parent_array));
    parent_array.release = NoOpArrayRelease;
    parent_array.length = 5;
    parent_array.null_count = 0;
    parent_array.offset = 0;
    parent_array.n_buffers = 0;
    parent_array.n_children = 2;
    parent_array.children = (ArrowArray **)malloc(parent_array.n_children * sizeof(ArrowArray *));
    parent_array.children[0] = &array1;
    parent_array.children[1] = &array2;

    // 打印 C_API的ArrowArray
    for (int i = 0; i < parent_array.n_children; i++)
    {
        std::cout << "Child " << i << ":" << std::endl;
        for (int j = 0; j < parent_array.children[i]->n_buffers; j++)
        {
            if (parent_array.children[i]->buffers[j] != NULL)
            {
                int64_t *buffer_data = (int64_t *)parent_array.children[i]->buffers[j];
                for (int k = 0; k < parent_array.children[i]->length; k++)
                {
                    std::cout << "  Buffer " << j << " data " << k << ": " << buffer_data[k] << std::endl;
                }
            }
        }
    }

    // 转化
    std::shared_ptr<arrow::Schema> arrow_schema;
    std::vector<std::shared_ptr<arrow::Array>> arrow_array;
    
    arrow_schema = arrow::ImportSchema(&parent_schema).ValueOrDie();
    for (int i = 0; i < parent_array.n_children; i++)
    {
        arrow_array.emplace_back(arrow::ImportArray(parent_array.children[i], parent_schema.children[i]).ValueOrDie());
    }

    auto my_table = arrow::Table::Make(arrow_schema, arrow_array);

    // 打印 C++ 的 ArrowTable
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

    // 清理
    free(parent_array.children);
    free(parent_schema.children);

    return 0;
}