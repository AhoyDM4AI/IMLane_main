#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <iostream>
#include <string>

int main()
{
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
    return 0;
}