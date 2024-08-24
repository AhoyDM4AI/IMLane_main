#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <iostream>
#include <thread>
#include <chrono>

namespace bi = boost::interprocess;

int main()
{
    bi::managed_shared_memory segment = bi::managed_shared_memory(bi::open_or_create, "MySharedMemory", 1024 * 1024);
    bi::interprocess_semaphore *semA = segment.construct<bi::interprocess_semaphore>("semA")(0);
    bi::interprocess_semaphore *semB = segment.construct<bi::interprocess_semaphore>("semB")(0);
    bool* alive = segment.construct<bool>("alive")(true);

    std::thread t([]()
                  { std::system("./test2 9527"); });
    t.detach();
    int n = 5;
    while (n-- && *alive)
    {
        std::cout << "[" << n << "] A create data\n";
        if(n == 2){
            *alive = false;
        }
        semB->post();
        semA->wait();
        std::cout << "[" << n << "] A recieve the result\n";
        // sleep 3秒
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    bi::shared_memory_object::remove("MySharedMemory");
    return 0;
}