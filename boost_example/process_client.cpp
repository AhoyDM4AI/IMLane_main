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
  boost::interprocess::shared_memory_object::remove("IPC_CHENELL");
  boost::interprocess::managed_shared_memory managed_shm(boost::interprocess::create_only, "IPC_CHENELL", 1024);
  typedef boost::interprocess::allocator<char, boost::interprocess::managed_shared_memory::segment_manager> CharAllocator;
  typedef boost::interprocess::basic_string<char, std::char_traits<char>, CharAllocator> string;
  string *s = managed_shm.construct<string>("CODE")("Hello!", managed_shm.get_segment_manager());
  s->insert(5, ", world");
  boost::interprocess::interprocess_mutex *mtx = managed_shm.construct<boost::interprocess::interprocess_mutex>("mtx")();
  boost::interprocess::interprocess_condition *cnd = managed_shm.construct<boost::interprocess::interprocess_condition>("cnd")();
  boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*mtx);

  int i = 10;
  while (i--)
  {
    std::cout << "[Client] send : " << *s << std::endl;
    cnd->notify_all();
    cnd->wait(lock);

    std::pair<string *, size_t> p = managed_shm.find<string>("CODE");
    std::cout << "[Client] get  : " << *p.first << std::endl;
    std::cout<<"\n";
  }
  cnd->notify_all();
  boost::interprocess::shared_memory_object::remove("IPC_CHENELL");
  return 0;
}