#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <iostream>

namespace bi = boost::interprocess;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "you shoule add a parameter\n";
        return 0;
    }
    std::string id = argv[1];

    bi::managed_shared_memory segment = bi::managed_shared_memory(bi::open_only, "MySharedMemory");
    bi::interprocess_semaphore *semA = segment.find<bi::interprocess_semaphore>("semA").first;
    bi::interprocess_semaphore *semB = segment.find<bi::interprocess_semaphore>("semB").first;
    int n = 5;
    while (n--)
    {
        semB->wait();
        bool *alive = segment.find<bool>("alive").first;
        if (!*alive)
        {
            std::cout << "[" << n << "] B is dead\n";
            break;
        }
        std::cout << "[" << n << "] B handle data       " << id << "\n";
        semA->post();
    }
    semA->post();
    return 0;
}