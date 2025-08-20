#include <fastly/kv_store.h>

namespace fastly::kv_store {
InsertBuilder InsertBuilder::mode(InsertMode mode) && {
  fastly::sys::kv_store::m_kv_store_insert_builder_mode(
      std::move(this->builder_), mode);
  return std::move(*this);
}

InsertBuilder InsertBuilder::background_fetch() && {
  fastly::sys::kv_store::m_kv_store_insert_builder_background_fetch(
      std::move(this->builder_));
  return std::move(*this);
}

InsertBuilder InsertBuilder::if_generation_match(std::uint64_t gen) && {
  fastly::sys::kv_store::m_kv_store_insert_builder_if_generation_match(
      std::move(this->builder_), gen);
  return std::move(*this);
}

InsertBuilder InsertBuilder::metadata(const std::string &data) && {
  fastly::sys::kv_store::m_kv_store_insert_builder_metadata(
      std::move(this->builder_), data);
  return std::move(*this);
}

InsertBuilder InsertBuilder::time_to_live(std::chrono::milliseconds ttl) && {
  fastly::sys::kv_store::m_kv_store_insert_builder_time_to_live(
      std::move(this->builder_), ttl.count());
  return std::move(*this);
}

expected<> InsertBuilder::execute(const std::string &key, Body body) && {
  fastly::sys::kv_store::KVStoreError *err;
  fastly::sys::kv_store::m_kv_store_insert_builder_execute(
      std::move(this->builder_), key, std::move(body.bod), err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  }
  // If the insert was successful, we return an empty expected.
  return {};
}

expected<PendingInsertHandle>
InsertBuilder::execute_async(const std::string &key, Body body) && {
  std::uint32_t handle;
  fastly::sys::kv_store::KVStoreError *err;
  fastly::sys::kv_store::m_kv_store_insert_builder_execute_async(
      std::move(this->builder_), key, std::move(body.bod), handle, err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  }
  return PendingInsertHandle::from_u32(handle);
}
} // namespace fastly::kv_store