#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <iostream>
#include <thread>
#include <chrono>

namespace bi = boost::interprocess;

int main()
{
    // 创建共享内存
    bi::shared_memory_object shm(bi::open_or_create, "MySharedMemory", bi::read_write);
    shm.truncate(2 * sizeof(bi::interprocess_semaphore));

    // 映射共享内存到当前进程
    bi::mapped_region region(shm, bi::read_write);

    // 在共享内存中创建两个信号量
    void *addr = region.get_address();
    bi::interprocess_semaphore *semA = new (addr) bi::interprocess_semaphore(0);
    bi::interprocess_semaphore *semB = new ((char *)addr + sizeof(bi::interprocess_semaphore)) bi::interprocess_semaphore(0);

    std::thread t([]()
                  { std::system("./test_b"); });
    t.detach();
    int n = 5;
    while (n--)
    {
        std::cout << "[" << n << "] A create data\n";
        semB->post();
        semA->wait();
        std::cout << "[" << n << "] A recieve the result\n";
    }
    bi::shared_memory_object::remove("MySharedMemory");
    return 0;
}