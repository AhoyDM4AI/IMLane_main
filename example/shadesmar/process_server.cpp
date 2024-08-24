#include "translation_data_type.hpp"

#ifdef SINGLE_HEADER
#include "shadesmar.h"
#else
#include "shadesmar/memory/copier.h"
#include "shadesmar/memory/dragons.h"
#include "shadesmar/rpc/server.h"
#include "shadesmar/stats.h"
#endif


const int VECTOR_SIZE = 64 * 1024;
uint8_t *resp_buffer = nullptr;

bool callback(const shm::memory::Memblock &req, shm::memory::Memblock *resp) {
  auto *msg = reinterpret_cast<Message *>(req.ptr);
  std::cout << "[Server] get message : count " << msg->count << std::endl;
  std::cout << "[Server] get mesage : code- " << msg->code << std::endl;
  std::cout << "]\n\n";
  Message *response_msg = reinterpret_cast<Message *>(resp_buffer);
  response_msg->count = msg->count + 1;
  resp->ptr = response_msg;
  resp->size = VECTOR_SIZE;
  std::cout << "[Server] sned message : count " << response_msg->count
            << std::endl;
  std::cout << "[Server] send mesage : code- " << response_msg->code
            << std::endl;
  std::cout << "]\n\n";
  return true;
}

void cleanup(shm::memory::Memblock *resp) {
  resp->ptr = nullptr;
  resp->size = 0;
}

int main() {
  std::string name = "process_client";
  std::cout << "[Server] start a server process!\n";
  resp_buffer = reinterpret_cast<uint8_t *>(malloc(VECTOR_SIZE));
  std::memset(resp_buffer, 0, VECTOR_SIZE);

  shm::rpc::Server server(name, callback, cleanup);
  while (1) {
    server.serve_once();
    constexpr std::chrono::milliseconds SLEEP_TIME{400U};
    std::this_thread::sleep_for(SLEEP_TIME);
  }

  return 0;
}   