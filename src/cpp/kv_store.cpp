#include <fastly/kv_store.h>
#include <iostream>

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
    return unexpected(err);
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
    return unexpected(err);
  }
  return PendingInsertHandle::from_u32(handle);
}

std::vector<std::string> ListPage::keys() const {
  auto keys = page_->keys();
  std::vector<std::string> result;
  result.reserve(keys.size());
  for (const auto &key : keys) {
    result.emplace_back(std::move(key));
  }
  return result;
}

std::vector<std::string> ListPage::into_keys() {
  auto keys = fastly::sys::kv_store::m_kv_store_list_page_into_keys(
      std::move(this->page_));
  std::vector<std::string> result;
  result.reserve(keys.size());
  for (const auto &key : keys) {
    result.emplace_back(std::move(key));
  }
  return result;
}

std::optional<std::string> ListPage::next_cursor() const {
  std::string cursor;
  if (page_->next_cursor(cursor)) {
    return cursor;
  }
  return std::nullopt;
}

std::optional<std::string> ListPage::prefix() const {
  std::string prefix;
  if (page_->prefix(prefix)) {
    return prefix;
  }
  return std::nullopt;
}

std::uint32_t ListPage::limit() const { return page_->limit(); }

ListMode ListPage::mode() const {
  fastly::sys::kv_store::ListMode *mode;
  page_->mode(mode);
  switch (mode->code()) {
  case fastly::sys::kv_store::ListModeType::Strong:
    return ListModeStrong{};
  case fastly::sys::kv_store::ListModeType::Eventual:
    return ListModeEventual{};
  case fastly::sys::kv_store::ListModeType::Other: {
    std::string other;
    mode->other_string(other);
    return ListModeOther{std::move(other)};
  }
  default:
    std::cerr << "Unknown ListMode code: " << static_cast<int>(mode->code())
              << '\n';
    std::terminate();
  }
}

Body LookupResponse::take_body() { return {response_->take_body()}; }

std::optional<Body> LookupResponse::try_take_body() {
  fastly::sys::http::Body *bod;
  if (response_->try_take_body(bod)) {
    return Body{rust::Box<fastly::sys::http::Body>::from_raw(bod)};
  }
  return std::nullopt;
}

std::vector<uint8_t> LookupResponse::take_body_bytes() {
  std::vector<uint8_t> body_bytes;
  response_->take_body_bytes(body_bytes);
  return body_bytes;
}

std::optional<std::vector<std::uint8_t>> LookupResponse::metadata() const {
  std::vector<std::uint8_t> metadata;
  if (response_->metadata(metadata)) {
    return metadata;
  }
  return std::nullopt;
}

std::uint64_t LookupResponse::current_generation() const {
  return response_->current_generation();
}

expected<LookupResponse> LookupBuilder::execute(std::string_view key) const {
  fastly::sys::kv_store::LookupResponse *response;
  fastly::sys::kv_store::KVStoreError *err;
  builder_->execute({key.data(), key.size()}, response, err);
  if (err != nullptr) {
    return unexpected(err);
  }
  return LookupResponse{
      rust::Box<fastly::sys::kv_store::LookupResponse>::from_raw(response)};
}

expected<PendingLookupHandle>
LookupBuilder::execute_async(std::string_view key) const {
  std::uint32_t handle;
  fastly::sys::kv_store::KVStoreError *err;
  builder_->execute_async({key.data(), key.size()}, handle, err);
  if (err != nullptr) {
    return unexpected(err);
  }
  return PendingLookupHandle::from_u32(handle);
}

expected<> EraseBuilder::execute(std::string_view key) const {
  fastly::sys::kv_store::KVStoreError *err;
  builder_->execute({key.data(), key.size()}, err);
  if (err != nullptr) {
    return unexpected(err);
  }
  return {};
}

expected<PendingEraseHandle>
EraseBuilder::execute_async(std::string_view key) const {
  std::uint32_t handle;
  fastly::sys::kv_store::KVStoreError *err;
  builder_->execute_async({key.data(), key.size()}, handle, err);
  if (err != nullptr) {
    return unexpected(err);
  }
  return PendingEraseHandle::from_u32(handle);
}

ListBuilder ListBuilder::eventual_consistency() && {
  fastly::sys::kv_store::m_kv_store_list_builder_eventual_consistency(
      std::move(this->builder_));
  return std::move(*this);
}

ListBuilder ListBuilder::cursor(std::string_view cursor) && {
  fastly::sys::kv_store::m_kv_store_list_builder_cursor(
      std::move(this->builder_), {cursor.data(), cursor.size()});
  return std::move(*this);
}

ListBuilder ListBuilder::limit(std::uint32_t limit) && {
  fastly::sys::kv_store::m_kv_store_list_builder_limit(
      std::move(this->builder_), limit);
  return std::move(*this);
}

ListBuilder ListBuilder::prefix(std::string_view prefix) && {
  fastly::sys::kv_store::m_kv_store_list_builder_prefix(
      std::move(this->builder_), {prefix.data(), prefix.size()});
  return std::move(*this);
}
expected<ListPage> ListBuilder::execute() && {
  fastly::sys::kv_store::ListPage *page;
  fastly::sys::kv_store::KVStoreError *err;
  fastly::sys::kv_store::m_kv_store_list_builder_execute(
      std::move(this->builder_), page, err);
  if (err != nullptr) {
    return unexpected(err);
  }
  return ListPage{rust::Box<fastly::sys::kv_store::ListPage>::from_raw(page)};
}

ListResponse ListBuilder::iter() && {
  fastly::sys::kv_store::ListResponse *response;
  fastly::sys::kv_store::m_kv_store_list_builder_iter(std::move(this->builder_),
                                                      response);
  return ListResponse{
      rust::Box<fastly::sys::kv_store::ListResponse>::from_raw(response)};
}

expected<PendingListHandle> ListBuilder::execute_async() const {
  std::uint32_t handle;
  fastly::sys::kv_store::KVStoreError *err;
  builder_->execute_async(handle, err);
  if (err != nullptr) {
    return unexpected(err);
  }
  return PendingListHandle::from_u32(handle);
}

} // namespace fastly::kv_store