#include <fastly/kv_store.h>
#include <iostream>

namespace fastly::kv_store {
KVStoreErrorCode KVStoreError::error_code() { return err_->error_code(); }
std::string KVStoreError::error_msg() {
  std::string msg;
  err_->error_msg(msg);
  return msg;
}

InsertBuilder InsertBuilder::mode(InsertMode mode) && {
  builder_ = fastly::sys::kv_store::m_kv_store_insert_builder_mode(
      std::move(builder_), mode);
  return std::move(*this);
}

InsertBuilder InsertBuilder::background_fetch() && {
  builder_ = fastly::sys::kv_store::m_kv_store_insert_builder_background_fetch(
      std::move(builder_));
  return std::move(*this);
}

InsertBuilder InsertBuilder::if_generation_match(std::uint64_t gen) && {
  builder_ =
      fastly::sys::kv_store::m_kv_store_insert_builder_if_generation_match(
          std::move(builder_), gen);
  return std::move(*this);
}

InsertBuilder InsertBuilder::metadata(const std::string &data) && {
  builder_ = fastly::sys::kv_store::m_kv_store_insert_builder_metadata(
      std::move(builder_), data);
  return std::move(*this);
}

InsertBuilder InsertBuilder::time_to_live(std::chrono::milliseconds ttl) && {
  builder_ = fastly::sys::kv_store::m_kv_store_insert_builder_time_to_live(
      std::move(builder_), ttl.count());
  return std::move(*this);
}

expected<> InsertBuilder::execute(const std::string &key, Body body) && {
  fastly::sys::kv_store::KVStoreError *err;
  fastly::sys::kv_store::m_kv_store_insert_builder_execute(
      std::move(builder_), key, std::move(body.bod), err);
  if (err != nullptr) {
    return unexpected(err);
  }
  return {};
}

expected<PendingInsertHandle>
InsertBuilder::execute_async(const std::string &key, Body body) && {
  std::uint32_t handle;
  fastly::sys::kv_store::KVStoreError *err;
  fastly::sys::kv_store::m_kv_store_insert_builder_execute_async(
      std::move(builder_), key, std::move(body.bod), handle, err);
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
  auto keys =
      fastly::sys::kv_store::m_kv_store_list_page_into_keys(std::move(page_));
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
  auto mode = page_->mode();
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
  builder_ =
      fastly::sys::kv_store::m_kv_store_list_builder_eventual_consistency(
          std::move(builder_));
  return std::move(*this);
}

ListBuilder ListBuilder::cursor(const std::string &cursor) && {
  builder_ = fastly::sys::kv_store::m_kv_store_list_builder_cursor(
      std::move(builder_), cursor);
  return std::move(*this);
}

ListBuilder ListBuilder::limit(std::uint32_t limit) && {
  builder_ = fastly::sys::kv_store::m_kv_store_list_builder_limit(
      std::move(builder_), limit);
  return std::move(*this);
}

ListBuilder ListBuilder::prefix(const std::string &prefix) && {
  builder_ = fastly::sys::kv_store::m_kv_store_list_builder_prefix(
      std::move(builder_), prefix);
  return std::move(*this);
}
expected<ListPage> ListBuilder::execute() && {
  fastly::sys::kv_store::ListPage *page;
  fastly::sys::kv_store::KVStoreError *err;
  fastly::sys::kv_store::m_kv_store_list_builder_execute(std::move(builder_),
                                                         page, err);
  if (err != nullptr) {
    return unexpected(err);
  }
  return ListPage{rust::Box<fastly::sys::kv_store::ListPage>::from_raw(page)};
}

ListResponse ListBuilder::iter() && {
  return {
      fastly::sys::kv_store::m_kv_store_list_builder_iter(std::move(builder_))};
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

expected<std::optional<KVStore>> KVStore::open(std::string_view name) {
  fastly::sys::kv_store::KVStore *store;
  fastly::sys::kv_store::KVStoreError *err;
  if (fastly::sys::kv_store::m_static_kv_store_kv_store_open(
          {name.data(), name.size()}, store, err)) {
    return KVStore{rust::Box<fastly::sys::kv_store::KVStore>::from_raw(store)};
  } else {
    if (err != nullptr) {
      return unexpected(KVStoreError(
          rust::Box<fastly::sys::kv_store::KVStoreError>::from_raw(err)));
    } else {
      return std::nullopt;
    }
  }
}

expected<LookupResponse> KVStore::lookup(std::string_view key) const {
  fastly::sys::kv_store::LookupResponse *response;
  fastly::sys::kv_store::KVStoreError *err;
  store_->lookup({key.data(), key.size()}, response, err);
  if (err != nullptr) {
    return unexpected(err);
  }
  return LookupResponse{
      rust::Box<fastly::sys::kv_store::LookupResponse>::from_raw(response)};
}

LookupBuilder KVStore::build_lookup() const { return {store_->build_lookup()}; }

expected<LookupResponse>
KVStore::pending_lookup_wait(PendingLookupHandle pending_request_handle) const {
  fastly::sys::kv_store::LookupResponse *response;
  fastly::sys::kv_store::KVStoreError *err;
  store_->pending_lookup_wait(pending_request_handle.as_u32(), response, err);
  if (err != nullptr) {
    return unexpected(err);
  }
  return LookupResponse{
      rust::Box<fastly::sys::kv_store::LookupResponse>::from_raw(response)};
}

expected<> KVStore::insert(std::string_view key, Body value) const {
  fastly::sys::kv_store::KVStoreError *err;
  store_->insert({key.data(), key.size()}, std::move(value.bod), err);
  if (err != nullptr) {
    return unexpected(err);
  }
  return {};
}

InsertBuilder KVStore::build_insert() const { return {store_->build_insert()}; }

expected<>
KVStore::pending_insert_wait(PendingInsertHandle pending_insert_handle) const {
  fastly::sys::kv_store::KVStoreError *err;
  store_->pending_insert_wait(pending_insert_handle.as_u32(), err);
  if (err != nullptr) {
    return unexpected(err);
  }
  return {};
}

expected<> KVStore::erase(std::string_view key) const {
  fastly::sys::kv_store::KVStoreError *err;
  store_->erase({key.data(), key.size()}, err);
  if (err != nullptr) {
    return unexpected(err);
  }
  return {};
}

EraseBuilder KVStore::build_erase() const { return {store_->build_erase()}; }

expected<>
KVStore::pending_erase_wait(PendingEraseHandle pending_erase_handle) const {
  fastly::sys::kv_store::KVStoreError *err;
  store_->pending_erase_wait(pending_erase_handle.as_u32(), err);
  if (err != nullptr) {
    return unexpected(err);
  }
  return {};
}

expected<ListPage> KVStore::list() const {
  fastly::sys::kv_store::ListPage *page;
  fastly::sys::kv_store::KVStoreError *err;
  store_->list(page, err);
  if (err != nullptr) {
    return unexpected(err);
  }
  return ListPage{rust::Box<fastly::sys::kv_store::ListPage>::from_raw(page)};
}

ListBuilder KVStore::build_list() const { return {store_->build_list()}; }

expected<ListPage>
KVStore::pending_list_wait(PendingListHandle pending_request_handle) const {
  fastly::sys::kv_store::ListPage *page;
  fastly::sys::kv_store::KVStoreError *err;
  store_->pending_list_wait(pending_request_handle.as_u32(), page, err);
  if (err != nullptr) {
    return unexpected(err);
  }
  return ListPage{rust::Box<fastly::sys::kv_store::ListPage>::from_raw(page)};
}

} // namespace fastly::kv_store