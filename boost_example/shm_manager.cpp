#include "shm_manager.hpp"

#include <boost/interprocess/managed_external_buffer.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <iostream>
#include <stdexcept>



SharedMemoryManager::SharedMemoryManager(const std::string& shm_region_name)
{
  shm_region_name_ = shm_region_name;
  create_ = false;
  shm_growth_bytes_ = 1024;

  shm_obj_ = std::make_unique<bi::shared_memory_object>(
      bi::open_only, shm_region_name.c_str(), bi::read_write);

  shm_map_ = std::make_shared<bi::mapped_region>(*shm_obj_, bi::read_write);
  old_shm_maps_.push_back(shm_map_);

  int64_t shm_size = 0;
  shm_obj_->get_size(shm_size);
  managed_buffer_ = std::make_unique<bi::managed_external_buffer>(
      bi::open_only, shm_map_->get_address(), shm_size);
  current_capacity_ = shm_size;

  // Construct a mutex in shared memory.
  shm_mutex_ =
      managed_buffer_->find_or_construct<bi::interprocess_mutex>("shm_mutex")();
  total_size_ = managed_buffer_->find_or_construct<uint64_t>("total size")();
  delete_region_ = false;
}


SharedMemoryManager::SharedMemoryManager(
    const std::string& shm_region_name, size_t shm_size,
    size_t shm_growth_bytes, bool create)
{
  shm_region_name_ = shm_region_name;
  create_ = create;
  shm_growth_bytes_ = shm_growth_bytes;

  try {
    if (create) {
      // Remove (if any) and create the region.
      bi::shared_memory_object::remove(shm_region_name.c_str());
      shm_obj_ = std::make_unique<bi::shared_memory_object>(
          bi::create_only, shm_region_name.c_str(), bi::read_write);
      shm_obj_->truncate(shm_size);
    } else {
      // Open the existing region.
      shm_obj_ = std::make_unique<bi::shared_memory_object>(
          bi::open_only, shm_region_name.c_str(), bi::read_write);
    }

    current_capacity_ = shm_size;
    shm_map_ = std::make_shared<bi::mapped_region>(*shm_obj_, bi::read_write);
    old_shm_maps_.push_back(shm_map_);

    // Only create the managed external buffer for the stub process.
    if (create) {
      managed_buffer_ = std::make_unique<bi::managed_external_buffer>(
          bi::create_only, shm_map_->get_address(), shm_size);
    } else {
      int64_t shm_size = 0;
      shm_obj_->get_size(shm_size);
      managed_buffer_ = std::make_unique<bi::managed_external_buffer>(
          bi::open_only, shm_map_->get_address(), shm_size);
      current_capacity_ = shm_size;
    }
  }
  catch (bi::interprocess_exception& ex) {
    std::string error_message =
        ("Unable to initialize shared memory key '" + shm_region_name +
         "' to requested size (" + std::to_string(shm_size) +
         " bytes). If you are running Triton inside docker, use '--shm-size' "
         "flag to control the shared memory region size. Each Python backend "
         "model instance requires at least 1 MB of shared memory. Error: " +
         ex.what());
    // Remove the shared memory region if there was an error.
    bi::shared_memory_object::remove(shm_region_name.c_str());
    throw std::runtime_error(std::move(error_message));
  }

  // Construct a mutex in shared memory.
  shm_mutex_ =
      managed_buffer_->find_or_construct<bi::interprocess_mutex>("shm_mutex")();
  total_size_ = managed_buffer_->find_or_construct<uint64_t>("total size")();
  delete_region_ = true;
  if (create) {
    *total_size_ = current_capacity_;
    new (shm_mutex_) bi::interprocess_mutex;
  }
}

void
SharedMemoryManager::GrowIfNeeded(uint64_t byte_size)
{
  if (*total_size_ != current_capacity_) {
    shm_map_ = std::make_shared<bi::mapped_region>(*shm_obj_, bi::read_write);
    managed_buffer_ = std::make_unique<bi::managed_external_buffer>(
        bi::open_only, shm_map_->get_address(), *total_size_);
    old_shm_maps_.push_back(shm_map_);
    current_capacity_ = *total_size_;
  }

  if (byte_size != 0) {
    uint64_t bytes_to_be_added =
        shm_growth_bytes_ * (byte_size / shm_growth_bytes_ + 1);
    uint64_t new_size = *total_size_ + bytes_to_be_added;
    try {
      shm_obj_->truncate(new_size);
    }
    catch (bi::interprocess_exception& ex) {
      std::string error_message =
          ("Failed to increase the shared memory pool size for key '" +
           shm_region_name_ + "' to " + std::to_string(*total_size_) +
           " bytes. If you are running Triton inside docker, use '--shm-size' "
           "flag to control the shared memory region size. Error: " +
           ex.what());
      throw std::runtime_error(error_message);
    }

    try {
      shm_obj_->truncate(new_size);
      shm_map_ = std::make_shared<bi::mapped_region>(*shm_obj_, bi::read_write);
      old_shm_maps_.push_back(shm_map_);
      managed_buffer_ = std::make_unique<bi::managed_external_buffer>(
          bi::open_only, shm_map_->get_address(), new_size);
      managed_buffer_->grow(new_size - current_capacity_);
      current_capacity_ = managed_buffer_->get_size();
      *total_size_ = new_size;
    }
    catch (bi::interprocess_exception& ex) {
      shm_obj_->truncate(*total_size_);
      std::string error_message =
          ("Failed to create new mapped region for the grown shared memory "
           "region '" +
           shm_region_name_ + "'. " + ex.what());
      throw std::runtime_error(error_message);
    }
  }
}

size_t
SharedMemoryManager::FreeMemory()
{
  GrowIfNeeded(0);
  return managed_buffer_->get_free_memory();
}


SharedMemoryManager::~SharedMemoryManager() noexcept(false)
{
  if (delete_region_) {
    bi::shared_memory_object::remove(shm_region_name_.c_str());
  }
}

void
SharedMemoryManager::SetDeleteRegion(bool delete_region)
{
  delete_region_ = delete_region;
}
