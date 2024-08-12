#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <iostream>

namespace bi = boost::interprocess;

int main()
{
    // 打开共享内存
    bi::shared_memory_object shm(bi::open_only, "MySharedMemory", bi::read_write);

    // 映射共享内存到当前进程
    bi::mapped_region region(shm, bi::read_write);

    // 在共享内存中获取两个信号量
    void* addr = region.get_address();
    bi::interprocess_semaphore *semA = static_cast<bi::interprocess_semaphore *>(addr);
    bi::interprocess_semaphore *semB = reinterpret_cast<bi::interprocess_semaphore *>((char*)addr + sizeof(bi::interprocess_semaphore));

    int n = 5;
    while (n--)
    {
        semB->wait();
        std::cout << "[" << n << "] B handle data\n";
        semA->post();
    }

    return 0;
}