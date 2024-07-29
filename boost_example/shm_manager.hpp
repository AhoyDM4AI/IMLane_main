#ifndef SHM_MANAGER_HPP
#define SHM_MANAGER_HPP


#include <boost/interprocess/managed_shared_memory.hpp>

namespace bi = boost::interprocess;


class SharedMemoryManagerTMP {
public:
    SharedMemoryManagerTMP(const std::string& name)
        : segment(boost::interprocess::open_or_create, name.c_str(), 65536) {}

    template <typename T>
    T* allocate(const std::string& name, std::size_t size) {
        return segment.construct<T>(name.c_str())[size]();
    }

    template <typename T>
    T* find(const std::string& name) {
        return segment.find<T>(name.c_str()).first;
    }

    void deallocate(const std::string& name) {
        segment.destroy<int64_t>(name.c_str());
    }
    boost::interprocess::managed_shared_memory segment;
};

#endif // SHM_MANAGER_HPP
