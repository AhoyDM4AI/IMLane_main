#include "translation_data_type.hpp"

#ifdef SINGLE_HEADER
#include "shadesmar.h"
#else
#include "shadesmar/memory/copier.h"
#include "shadesmar/memory/dragons.h"
#include "shadesmar/rpc/client.h"
#include "shadesmar/stats.h"
#endif

const int VECTOR_SIZE = 64 * 1024;

struct Message {
  uint64_t count;
  const char* code;
};

int main() {
  std::string name = "process_client";

  uint8_t *resp_buffer = nullptr;
  resp_buffer = reinterpret_cast<uint8_t *>(malloc(VECTOR_SIZE));
  std::memset(resp_buffer, 0, VECTOR_SIZE);
  int resp_size = VECTOR_SIZE;

  auto *rawptr = malloc(VECTOR_SIZE);
  std::memset(rawptr, 0, VECTOR_SIZE);
  Message *msg = reinterpret_cast<Message *>(rawptr);
  msg->count = 0;
  std::string a = "hello";
  msg->code = a.c_str();

  shm::rpc::Client client(name);
  while (1) {
    shm::memory::Memblock req, resp;
    req.ptr = msg;
    req.size = VECTOR_SIZE;
    std::cout << "[Client] send mesage : count- " << msg->count << std::endl;
    std::cout << "[Client] send mesage : code- " << msg->code << std::endl;

    bool success = client.call(req, &resp);
    auto *response_msg = reinterpret_cast<Message *>(resp.ptr);

    std::cout << "[Client] get mesage : count- " << response_msg->count
              << std::endl;
    std::cout << "[Client] get mesage : code- " << response_msg->code
              << std::endl;
    std::cout << "]\n\n";
    msg->count = response_msg->count;
    client.free_resp(&resp);
    constexpr std::chrono::milliseconds SLEEP_TIME{300U};
    std::this_thread::sleep_for(SLEEP_TIME);
  }
  free(msg);
  return 0;
}