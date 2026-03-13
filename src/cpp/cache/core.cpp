#include "../fastly.h"
#include <fastly/cache/core.h>
#include <fastly/detail/access_bridge_internals.h>
#include <fastly/sdk-sys.h>
#include <sstream>

namespace {
fastly::cache::core::CacheError from_status(const fastly::Status &status) {
  using Code = fastly::Status::Code;
  switch (status.code()) {
  case Code::Unsupported:
    return fastly::cache::core::CacheError(
        fastly::cache::core::CacheError::Code::Unsupported);
  case Code::LimitExceeded:
    return fastly::cache::core::CacheError(
        fastly::cache::core::CacheError::Code::LimitExceeded);
  case Code::BadHandle:
    return fastly::cache::core::CacheError(
        fastly::cache::core::CacheError::Code::InvalidOperation);
  default:
    return fastly::cache::core::CacheError(
        fastly::cache::core::CacheError::Code::Unknown);
  }
}
} // namespace

#define CACHE_TRY(expr)                                                        \
  do {                                                                         \
    auto status = (expr);                                                      \
    if (!status.is_ok()) {                                                     \
      return tl::unexpected(from_status(status));                              \
    }                                                                          \
  } while (0)

namespace {
std::string join_strings(const std::vector<std::string> &items,
                         char delimiter) {
  std::ostringstream oss;
  bool first = true;
  for (const auto &item : items) {
    if (!first) {
      oss << delimiter;
    }
    oss << item;
    first = false;
  }
  return oss.str();
}

// Helper to set request headers on cache options
void set_request_header_values(
    std::optional<fastly::cache::core::detail::RequestHandle> &request_headers,
    std::string_view name, std::span<const fastly::http::HeaderValue> values) {
  if (!request_headers.has_value()) {
    uint32_t req_handle = 0;
    auto status = fastly::http_req_new(&req_handle);
    if (!status.is_ok()) {
      std::cerr << "http_req_new failed\n";
      abort();
    }
    request_headers = fastly::cache::core::detail::RequestHandle(req_handle);
  }

  std::vector<std::uint8_t> bytes;
  for (auto &&value : values) {
    auto str = value.bytes();
    bytes.insert(bytes.end(), str.begin(), str.end());
    bytes.push_back('\0');
  }
  auto req_handle = request_headers->handle();
  fastly::http_req_header_values_set(
      req_handle, reinterpret_cast<const uint8_t *>(name.data()), name.size(),
      bytes.data(), bytes.size());
}

// Convert from public options struct to the ABI struct, returning both the ABI
// struct and a bitmask of which options were set.
std::pair<fastly::CacheLookupOptions, std::uint32_t>
as_abi(fastly::cache::core::LookupOptions &options) {
  fastly::CacheLookupOptions host_opt{};
  std::uint32_t options_mask = 0;

  if (options.request_headers.has_value()) {
    host_opt.request_headers = options.request_headers->handle();
    options_mask |= FASTLY_CACHE_LOOKUP_OPTIONS_MASK_REQUEST_HEADERS;
  }

  if (options.service.has_value()) {
    host_opt.service = options.service->c_str();
    host_opt.service_len = options.service->size();
    options_mask |= FASTLY_CACHE_LOOKUP_OPTIONS_MASK_SERVICE;
  }

  if (options.always_use_requested_range) {
    options_mask |= FASTLY_CACHE_LOOKUP_OPTIONS_MASK_ALWAYS_USE_REQUESTED_RANGE;
  }

  return {host_opt, options_mask};
}

// Similar to above, but for write options.
std::pair<fastly::CacheWriteOptions, std::uint32_t>
as_abi(fastly::cache::core::WriteOptions &options) {
  fastly::CacheWriteOptions host_opt{};
  std::uint32_t options_mask = 0;

  host_opt.max_age_ns = options.max_age.count();

  if (options.request_headers.has_value()) {
    host_opt.request_headers = options.request_headers->handle();
    options_mask |= FASTLY_CACHE_WRITE_OPTIONS_MASK_REQUEST_HEADERS;
  }

  if (options.vary_rule.has_value()) {
    host_opt.vary_rule_ptr =
        reinterpret_cast<const uint8_t *>(options.vary_rule->c_str());
    host_opt.vary_rule_len = options.vary_rule->size();
    options_mask |= FASTLY_CACHE_WRITE_OPTIONS_MASK_VARY_RULE;
  }

  if (options.initial_age.has_value()) {
    host_opt.initial_age_ns = options.initial_age->count();
    options_mask |= FASTLY_CACHE_WRITE_OPTIONS_MASK_INITIAL_AGE_NS;
  }

  if (options.stale_while_revalidate.has_value()) {
    host_opt.stale_while_revalidate_ns =
        options.stale_while_revalidate->count();
    options_mask |= FASTLY_CACHE_WRITE_OPTIONS_MASK_STALE_WHILE_REVALIDATE_NS;
  }

  if (options.surrogate_keys.has_value()) {
    host_opt.surrogate_keys_ptr =
        reinterpret_cast<const uint8_t *>(options.surrogate_keys->c_str());
    host_opt.surrogate_keys_len = options.surrogate_keys->size();
    options_mask |= FASTLY_CACHE_WRITE_OPTIONS_MASK_SURROGATE_KEYS;
  }

  if (options.length.has_value()) {
    host_opt.length = options.length.value();
    options_mask |= FASTLY_CACHE_WRITE_OPTIONS_MASK_LENGTH;
  }

  if (options.user_metadata.has_value()) {
    host_opt.user_metadata_ptr = options.user_metadata->data();
    host_opt.user_metadata_len = options.user_metadata->size();
    options_mask |= FASTLY_CACHE_WRITE_OPTIONS_MASK_USER_METADATA;
  }

  if (options.sensitive_data) {
    options_mask |= FASTLY_CACHE_WRITE_OPTIONS_MASK_SENSITIVE_DATA;
  }

  if (options.edge_max_age.has_value()) {
    host_opt.edge_max_age_ns = options.edge_max_age->count();
    options_mask |= FASTLY_CACHE_WRITE_OPTIONS_MASK_EDGE_MAX_AGE_NS;
  }

  if (options.service.has_value()) {
    host_opt.service = options.service->c_str();
    host_opt.service_len = options.service->size();
    options_mask |= FASTLY_CACHE_WRITE_OPTIONS_MASK_SERVICE;
  }

  return {host_opt, options_mask};
}

// Similar to above, but for replace options.
std::pair<fastly::CacheReplaceOptions, std::uint32_t>
as_abi(fastly::cache::core::ReplaceOptions &options) {
  fastly::CacheReplaceOptions host_opt{};
  std::uint32_t options_mask = 0;

  if (options.request_headers.has_value()) {
    host_opt.request_headers = options.request_headers->handle();
    options_mask |= FASTLY_CACHE_REPLACE_OPTIONS_MASK_REQUEST_HEADERS;
  }

  host_opt.replace_strategy =
      static_cast<std::uint32_t>(options.replace_strategy);
  options_mask |= FASTLY_CACHE_REPLACE_OPTIONS_MASK_REPLACE_STRATEGY;

  if (options.service.has_value()) {
    host_opt.service = options.service->c_str();
    host_opt.service_len = options.service->size();
    options_mask |= FASTLY_CACHE_REPLACE_OPTIONS_MASK_SERVICE;
  }

  if (options.always_use_requested_range) {
    options_mask |=
        FASTLY_CACHE_REPLACE_OPTIONS_MASK_ALWAYS_USE_REQUESTED_RANGE;
  }

  return {host_opt, options_mask};
}
} // namespace

namespace fastly::cache::core {

namespace detail {
// Really these should live somewhere else, maybe in fastly.h, but this is good
// enough for now, and it'll all be ripped out during componentisation anyway.
CacheHandle::~CacheHandle() { fastly::cache_close(handle_); }
CacheReplaceHandle::~CacheReplaceHandle() { fastly::cache_close(handle_); }
CacheBusyHandle::~CacheBusyHandle() { fastly::cache_close(handle_); }
RequestHandle::~RequestHandle() { fastly::http_req_close(handle_); }
RequestHandle RequestHandle::make() {
  uint32_t req_handle = 0;
  auto status = fastly::http_req_new(&req_handle);
  if (!status.is_ok()) {
    std::cerr << "http_req_new failed\n";
    abort();
  }
  return RequestHandle(req_handle);
}
} // namespace detail

bool Transaction::must_insert() const {
  std::uint8_t state = 0;
  auto status = fastly::cache_get_state(handle_->handle(), &state);
  if (!status.is_ok()) {
    std::cerr << "cache_get_state failed\n";
    abort();
  }
  return !(state & FASTLY_HOST_CACHE_LOOKUP_STATE_FOUND) &&
         (state & FASTLY_HOST_CACHE_LOOKUP_STATE_MUST_INSERT_OR_UPDATE);
}

bool Transaction::must_insert_or_update() const {
  std::uint8_t state = 0;
  auto status = fastly::cache_get_state(handle_->handle(), &state);
  if (!status.is_ok()) {
    std::cerr << "cache_get_state failed\n";
    abort();
  }
  return (state & FASTLY_HOST_CACHE_LOOKUP_STATE_MUST_INSERT_OR_UPDATE) != 0;
}

tl::expected<void, CacheError> Transaction::cancel_insert_or_update() const {
  CACHE_TRY(fastly::cache_transaction_cancel(handle_->handle()));
  return {};
}

TransactionUpdateBuilder
Transaction::update(std::chrono::nanoseconds max_age) && {
  WriteOptions options(max_age);
  return TransactionUpdateBuilder(
      handle_, // intentional copy of the shared handle
      std::move(options));
}

TransactionInsertBuilder
Transaction::insert(std::chrono::nanoseconds max_age) && {
  WriteOptions options(max_age);
  return TransactionInsertBuilder(
      handle_, // intentional copy of the shared handle
      std::move(options));
}

TransactionLookupBuilder Transaction::lookup(CacheKey key) {
  return TransactionLookupBuilder(std::move(key));
}

std::optional<Found> Transaction::found() const {
  std::uint8_t state = 0;
  auto status = fastly::cache_get_state(handle_->handle(), &state);
  if (!status.is_ok()) {
    std::cerr << "cache_get_state failed\n";
    abort();
  }
  if (state & FASTLY_HOST_CACHE_LOOKUP_STATE_FOUND) {
    return Found(handle_); // intentional copy of the shared handle
  }
  return std::nullopt;
}

tl::expected<bool, CacheError> PendingTransaction::pending() const {
  std::uint32_t is_ready = 0;
  CACHE_TRY(fastly::async_is_ready(handle_->handle(), &is_ready));
  // pending() returns true if NOT ready
  return !static_cast<bool>(is_ready);
}

tl::expected<Transaction, CacheError> PendingTransaction::wait() && {
  // Wait for the busy handle to resolve into a cache handle
  std::uint32_t cache_handle = 0;
  CACHE_TRY(fastly::cache_busy_handle_wait(handle_->handle(), &cache_handle));

  // Wait for the cache handle async item to be complete
  CACHE_TRY(fastly::cache_wait(cache_handle));

  return Transaction(std::make_shared<detail::CacheHandle>(cache_handle));
}

TransactionLookupBuilder TransactionLookupBuilder::header_values(
    std::string_view name, std::span<const http::HeaderValue> values) && {
  set_request_header_values(options_.request_headers, name, values);
  return std::move(*this);
}

TransactionLookupBuilder
TransactionLookupBuilder::header(std::string_view name,
                                 const http::HeaderValue &value) && {
  return std::move(*this).header_values(name, {&value, 1});
}

TransactionLookupBuilder
TransactionLookupBuilder::on_behalf_of(std::string service) && {
  options_.service = std::move(service);
  return std::move(*this);
}

TransactionLookupBuilder
TransactionLookupBuilder::always_use_requested_range() && {
  options_.always_use_requested_range = true;
  return std::move(*this);
}

tl::expected<Transaction, CacheError> TransactionLookupBuilder::execute() && {
  auto [options, options_mask] = as_abi(options_);
  std::uint32_t cache_handle = 0;
  CACHE_TRY(fastly::cache_transaction_lookup(
      key_.data(), key_.size(), options_mask, &options, &cache_handle));

  // Wait for the lookup to complete (synchronous behavior)
  CACHE_TRY(fastly::cache_wait(cache_handle));

  return Transaction(std::make_shared<detail::CacheHandle>(cache_handle));
}

tl::expected<PendingTransaction, CacheError>
TransactionLookupBuilder::execute_async() && {
  auto [options, options_mask] = as_abi(options_);
  std::uint32_t busy_handle = 0;
  CACHE_TRY(fastly::cache_transaction_lookup_async(
      key_.data(), key_.size(), options_mask, &options, &busy_handle));

  return PendingTransaction(
      std::make_shared<detail::CacheBusyHandle>(busy_handle));
}

TransactionUpdateBuilder
TransactionUpdateBuilder::vary_by(std::vector<std::string> headers) && {
  if (!headers.empty()) {
    options_.vary_rule = join_strings(headers, ' ');
  }
  return std::move(*this);
}

TransactionUpdateBuilder
TransactionUpdateBuilder::age(std::chrono::nanoseconds age) && {
  options_.initial_age = age;
  return std::move(*this);
}

TransactionUpdateBuilder TransactionUpdateBuilder::deliver_node_max_age(
    std::chrono::nanoseconds duration) && {
  options_.edge_max_age = duration;
  return std::move(*this);
}

TransactionUpdateBuilder TransactionUpdateBuilder::stale_while_revalidate(
    std::chrono::nanoseconds duration) && {
  options_.stale_while_revalidate = duration;
  return std::move(*this);
}

TransactionUpdateBuilder
TransactionUpdateBuilder::surrogate_keys(std::vector<std::string> keys) && {
  if (!keys.empty()) {
    options_.surrogate_keys = join_strings(keys, ' ');
  }
  return std::move(*this);
}

TransactionUpdateBuilder
TransactionUpdateBuilder::user_metadata(std::vector<std::uint8_t> metadata) && {
  options_.user_metadata = std::move(metadata);
  return std::move(*this);
}

TransactionUpdateBuilder
TransactionUpdateBuilder::on_behalf_of(std::string service) && {
  options_.service = std::move(service);
  return std::move(*this);
}

tl::expected<void, CacheError> TransactionUpdateBuilder::execute() && {
  std::uint32_t cache_handle = handle_->handle();
  auto [options, options_mask] = as_abi(options_);
  CACHE_TRY(
      fastly::cache_transaction_update(cache_handle, options_mask, &options));
  return {};
}

TransactionInsertBuilder
TransactionInsertBuilder::vary_by(std::vector<std::string> headers) && {
  if (!headers.empty()) {
    options_.vary_rule = join_strings(headers, ' ');
  }
  return std::move(*this);
}

TransactionInsertBuilder
TransactionInsertBuilder::initial_age(std::chrono::nanoseconds age) && {
  options_.initial_age = age;
  return std::move(*this);
}

TransactionInsertBuilder TransactionInsertBuilder::stale_while_revalidate(
    std::chrono::nanoseconds duration) && {
  options_.stale_while_revalidate = duration;
  return std::move(*this);
}

TransactionInsertBuilder
TransactionInsertBuilder::surrogate_keys(std::vector<std::string> keys) && {
  if (!keys.empty()) {
    options_.surrogate_keys = join_strings(keys, ' ');
  }
  return std::move(*this);
}

TransactionInsertBuilder
TransactionInsertBuilder::known_length(std::uint64_t length) && {
  options_.length = length;
  return std::move(*this);
}

TransactionInsertBuilder
TransactionInsertBuilder::user_metadata(std::vector<std::uint8_t> metadata) && {
  options_.user_metadata = std::move(metadata);
  return std::move(*this);
}

TransactionInsertBuilder
TransactionInsertBuilder::sensitive_data(bool is_sensitive) && {
  options_.sensitive_data = is_sensitive;
  return std::move(*this);
}

TransactionInsertBuilder TransactionInsertBuilder::deliver_node_max_age(
    std::chrono::nanoseconds duration) && {
  options_.edge_max_age = duration;
  return std::move(*this);
}

TransactionInsertBuilder
TransactionInsertBuilder::on_behalf_of(std::string service) && {
  options_.service = std::move(service);
  return std::move(*this);
}

tl::expected<http::StreamingBody, CacheError>
TransactionInsertBuilder::execute() && {
  std::uint32_t cache_handle = handle_->handle();
  auto [options, options_mask] = as_abi(options_);
  std::uint32_t body_handle = 0;
  CACHE_TRY(fastly::cache_transaction_insert(cache_handle, options_mask,
                                             &options, &body_handle));
  return http::StreamingBody::from_body_handle(body_handle);
}

tl::expected<std::pair<http::StreamingBody, Found>, CacheError>
TransactionInsertBuilder::execute_and_stream_back() && {
  std::uint32_t cache_handle_val = handle_->handle();
  auto [options, options_mask] = as_abi(options_);
  std::uint32_t body_handle = 0;
  std::uint32_t cache_handle_out = 0;
  CACHE_TRY(fastly::cache_transaction_insert_and_stream_back(
      cache_handle_val, options_mask, &options, &body_handle,
      &cache_handle_out));

  return std::make_pair(
      http::StreamingBody::from_body_handle(body_handle),
      Found(std::make_shared<detail::CacheHandle>(cache_handle_out)));
}

std::chrono::nanoseconds Found::age() const {
  std::uint64_t age_ns = 0;
  fastly::Status status;
  if (handle()) {
    status = fastly::cache_get_age_ns(handle()->handle(), &age_ns);
  } else {
    status =
        fastly::cache_replace_get_age_ns(replace_handle()->handle(), &age_ns);
  }

  if (!status.is_ok()) {
    std::cerr << "cache_get_age_ns failed\n";
    abort();
  }
  return std::chrono::nanoseconds(age_ns);
}

std::chrono::nanoseconds Found::max_age() const {
  std::uint64_t max_age_ns = 0;
  fastly::Status status;
  if (handle()) {
    status = fastly::cache_get_max_age_ns(handle()->handle(), &max_age_ns);
  } else {
    status = fastly::cache_replace_get_max_age_ns(replace_handle()->handle(),
                                                  &max_age_ns);
  }

  if (!status.is_ok()) {
    std::cerr << "cache_get_max_age_ns failed\n";
    abort();
  }
  return std::chrono::nanoseconds(max_age_ns);
}

std::chrono::nanoseconds Found::remaining_ttl() const {
  auto max = max_age();
  auto current = age();
  if (current >= max) {
    return std::chrono::nanoseconds(0);
  }
  return max - current;
}

std::chrono::nanoseconds Found::stale_while_revalidate() const {
  std::uint64_t swr_ns = 0;
  fastly::Status status;
  if (handle()) {
    status = fastly::cache_get_stale_while_revalidate_ns(handle()->handle(),
                                                         &swr_ns);
  } else {
    status = fastly::cache_replace_get_stale_while_revalidate_ns(
        replace_handle()->handle(), &swr_ns);
  }

  if (!status.is_ok()) {
    std::cerr << "cache_get_stale_while_revalidate_ns failed\n";
    abort();
  }
  return std::chrono::nanoseconds(swr_ns);
}

bool Found::is_stale() const {
  std::uint8_t state = 0;
  fastly::Status status;
  if (handle()) {
    status = fastly::cache_get_state(handle()->handle(), &state);
  } else {
    status =
        fastly::cache_replace_get_state(replace_handle()->handle(), &state);
  }

  if (!status.is_ok()) {
    std::cerr << "cache_get_state failed\n";
    abort();
  }
  return (state & FASTLY_HOST_CACHE_LOOKUP_STATE_STALE) != 0;
}

bool Found::is_usable() const {
  std::uint8_t state = 0;
  fastly::Status status;
  if (handle()) {
    status = fastly::cache_get_state(handle()->handle(), &state);
  } else {
    status =
        fastly::cache_replace_get_state(replace_handle()->handle(), &state);
  }

  if (!status.is_ok()) {
    std::cerr << "cache_get_state failed\n";
    abort();
  }
  return (state & FASTLY_HOST_CACHE_LOOKUP_STATE_USABLE) != 0;
}

uint64_t Found::hits() const {
  std::uint64_t hits = 0;
  fastly::Status status;
  if (handle()) {
    status = fastly::cache_get_hits(handle()->handle(), &hits);
  } else {
    status = fastly::cache_replace_get_hits(replace_handle()->handle(), &hits);
  }

  if (!status.is_ok()) {
    std::cerr << "cache_get_hits failed\n";
    abort();
  }
  return hits;
}

std::optional<uint64_t> Found::known_length() const {
  std::uint64_t length = 0;
  fastly::Status status;
  if (handle()) {
    status = fastly::cache_get_length(handle()->handle(), &length);
  } else {
    status =
        fastly::cache_replace_get_length(replace_handle()->handle(), &length);
  }

  if (!status.is_ok()) {
    // Length may not be known, return nullopt
    return std::nullopt;
  }
  return length;
}

std::vector<uint8_t> Found::user_metadata() const {
  std::vector<uint8_t> buffer(16 * 1024); // reasonable initial size
  size_t nwritten = 0;

  // Choose the appropriate function based on handle type
  auto get_metadata = [this](uint8_t *buf, size_t buf_size,
                             size_t *nwritten) -> fastly::Status {
    if (handle()) {
      return fastly::cache_get_user_metadata(handle()->handle(), buf, buf_size,
                                             nwritten);
    } else {
      return fastly::cache_replace_get_user_metadata(replace_handle()->handle(),
                                                     buf, buf_size, nwritten);
    }
  };

  auto status = get_metadata(buffer.data(), buffer.size(), &nwritten);

  if (!status.is_ok()) {
    // If buffer was too small, try again with the right size
    if (status.code() == Status::Code::BufferLen) {
      buffer.resize(nwritten);
      status = get_metadata(buffer.data(), buffer.size(), &nwritten);
      if (!status.is_ok()) {
        std::cerr << "cache_get_user_metadata failed\n";
        abort();
      }
    } else {
      std::cerr << "cache_get_user_metadata failed\n";
      abort();
    }
  }

  buffer.resize(nwritten);
  return buffer;
}

tl::expected<Body, CacheError> Found::to_stream() const {
  return to_stream_from_range(std::nullopt, std::nullopt);
}

tl::expected<Body, CacheError>
Found::to_stream_from_range(std::optional<uint64_t> from,
                            std::optional<uint64_t> to) const {
  // Validate range: if both are provided, to must be >= from
  if (from.has_value() && to.has_value() && to.value() < from.value()) {
    return tl::unexpected(CacheError::InvalidOperation);
  }

  std::uint32_t options_mask = 0;
  fastly::CacheGetBodyOptions options{};

  if (from.has_value()) {
    options.from = from.value();
    options_mask |= FASTLY_CACHE_GET_BODY_OPTIONS_MASK_FROM;
  }
  if (to.has_value()) {
    options.to = to.value();
    options_mask |= FASTLY_CACHE_GET_BODY_OPTIONS_MASK_TO;
  }

  std::uint32_t body_handle = 0;
  fastly::Status status;
  if (handle()) {
    status = fastly::cache_get_body(handle()->handle(), options_mask, &options,
                                    &body_handle);
  } else {
    status = fastly::cache_replace_get_body(
        replace_handle()->handle(), options_mask, &options, &body_handle);
  }

  if (!status.is_ok()) {
    return tl::unexpected(from_status(status));
  }
  return Body::from_handle(body_handle);
}

LookupBuilder lookup(CacheKey key) { return LookupBuilder(std::move(key)); }

LookupBuilder
LookupBuilder::header_values(std::string_view name,
                             std::span<const http::HeaderValue> values) && {
  set_request_header_values(options_.request_headers, name, values);
  return std::move(*this);
}

LookupBuilder LookupBuilder::header(std::string_view name,
                                    const http::HeaderValue &value) && {
  return std::move(*this).header_values(name, {&value, 1});
}

LookupBuilder LookupBuilder::on_behalf_of(std::string service) && {
  options_.service = std::move(service);
  return std::move(*this);
}

LookupBuilder LookupBuilder::always_use_requested_range() && {
  options_.always_use_requested_range = true;
  return std::move(*this);
}

tl::expected<std::optional<Found>, CacheError> LookupBuilder::execute() && {
  auto [options, options_mask] = as_abi(options_);
  std::uint32_t cache_handle = 0;
  CACHE_TRY(fastly::cache_lookup(key_.data(), key_.size(), options_mask,
                                 &options, &cache_handle));

  // Wait for the lookup to complete (synchronous behavior)
  CACHE_TRY(fastly::cache_wait(cache_handle));

  // Check if we found something
  std::uint8_t state = 0;
  auto status = fastly::cache_get_state(cache_handle, &state);
  if (!status.is_ok()) {
    return tl::unexpected(from_status(status));
  }

  if (state & FASTLY_HOST_CACHE_LOOKUP_STATE_FOUND) {
    return Found(std::make_shared<detail::CacheHandle>(cache_handle));
  }
  return std::nullopt;
}

InsertBuilder insert(CacheKey key, std::chrono::nanoseconds max_age) {
  WriteOptions options(max_age);
  return InsertBuilder(std::move(key), std::move(options));
}

InsertBuilder
InsertBuilder::header_values(std::string_view name,
                             std::span<const http::HeaderValue> values) && {
  set_request_header_values(options_.request_headers, name, values);
  return std::move(*this);
}

InsertBuilder InsertBuilder::header(std::string_view name,
                                    const http::HeaderValue &value) && {
  return std::move(*this).header_values(name, {&value, 1});
}

InsertBuilder InsertBuilder::vary_by(std::vector<std::string> headers) && {
  if (!headers.empty()) {
    options_.vary_rule = join_strings(headers, ' ');
  }
  return std::move(*this);
}

InsertBuilder InsertBuilder::initial_age(std::chrono::nanoseconds age) && {
  options_.initial_age = age;
  return std::move(*this);
}

InsertBuilder
InsertBuilder::stale_while_revalidate(std::chrono::nanoseconds duration) && {
  options_.stale_while_revalidate = duration;
  return std::move(*this);
}

InsertBuilder InsertBuilder::surrogate_keys(std::vector<std::string> keys) && {
  if (!keys.empty()) {
    options_.surrogate_keys = join_strings(keys, ' ');
  }
  return std::move(*this);
}

InsertBuilder InsertBuilder::known_length(std::uint64_t length) && {
  options_.length = length;
  return std::move(*this);
}

InsertBuilder
InsertBuilder::user_metadata(std::vector<std::uint8_t> metadata) && {
  options_.user_metadata = std::move(metadata);
  return std::move(*this);
}

InsertBuilder InsertBuilder::sensitive_data(bool is_sensitive) && {
  options_.sensitive_data = is_sensitive;
  return std::move(*this);
}

InsertBuilder
InsertBuilder::deliver_node_max_age(std::chrono::nanoseconds duration) && {
  options_.edge_max_age = duration;
  return std::move(*this);
}

InsertBuilder InsertBuilder::on_behalf_of(std::string service) && {
  options_.service = std::move(service);
  return std::move(*this);
}

tl::expected<http::StreamingBody, CacheError> InsertBuilder::execute() && {
  auto [options, options_mask] = as_abi(options_);
  std::uint32_t body_handle = 0;
  CACHE_TRY(fastly::cache_insert(key_.data(), key_.size(), options_mask,
                                 &options, &body_handle));
  return http::StreamingBody::from_body_handle(body_handle);
}

ReplaceBuilder replace(CacheKey key) { return ReplaceBuilder(std::move(key)); }

tl::expected<Replace, CacheError> ReplaceBuilder::begin() && {
  auto [options, options_mask] = as_abi(options_);
  std::uint32_t replace_handle = 0;
  CACHE_TRY(fastly::cache_replace(key_.data(), key_.size(), options_mask,
                                  &options, &replace_handle));

  auto handle = std::make_shared<detail::CacheReplaceHandle>(replace_handle);

  // Check if an existing object was found
  std::uint8_t state = 0;
  auto status = fastly::cache_replace_get_state(replace_handle, &state);
  if (!status.is_ok()) {
    return tl::unexpected(from_status(status));
  }
  std::optional<Found> existing_object;
  if (state & FASTLY_HOST_CACHE_LOOKUP_STATE_FOUND) {
    existing_object = Found(handle);
  }

  return Replace(std::move(handle), std::move(existing_object));
}

ReplaceBuilder ReplaceBuilder::replace_strategy(ReplaceStrategy strategy) && {
  options_.replace_strategy = strategy;
  return std::move(*this);
}

ReplaceBuilder ReplaceBuilder::always_use_requested_range() && {
  options_.always_use_requested_range = true;
  return std::move(*this);
}

tl::expected<http::StreamingBody, CacheError>
Replace::execute(std::chrono::nanoseconds max_age) && {
  options_.max_age = max_age;

  // Drop the existing_object first
  existing_object_.reset();

  // Now execute the replace insert
  auto [options, options_mask] = as_abi(options_);
  std::uint32_t body_handle = 0;
  CACHE_TRY(fastly::cache_replace_insert(handle_->handle(), options_mask,
                                         &options, &body_handle));
  return http::StreamingBody::from_body_handle(body_handle);
}

Replace Replace::header_values(std::string_view name,
                               std::span<const http::HeaderValue> values) && {
  set_request_header_values(options_.request_headers, name, values);
  return std::move(*this);
}

Replace Replace::header(std::string_view name,
                        const http::HeaderValue &value) && {
  return std::move(*this).header_values(name, {&value, 1});
}

Replace Replace::vary_by(std::vector<std::string> headers) && {
  if (!headers.empty()) {
    options_.vary_rule = join_strings(headers, ' ');
  }
  return std::move(*this);
}

Replace Replace::initial_age(std::chrono::nanoseconds age) && {
  options_.initial_age = age;
  return std::move(*this);
}

Replace Replace::stale_while_revalidate(std::chrono::nanoseconds duration) && {
  options_.stale_while_revalidate = duration;
  return std::move(*this);
}

Replace Replace::surrogate_keys(std::vector<std::string> keys) && {
  if (!keys.empty()) {
    options_.surrogate_keys = join_strings(keys, ' ');
  }
  return std::move(*this);
}

Replace Replace::known_length(std::uint64_t length) && {
  options_.length = length;
  return std::move(*this);
}

Replace Replace::user_metadata(std::vector<std::uint8_t> metadata) && {
  options_.user_metadata = std::move(metadata);
  return std::move(*this);
}

Replace Replace::sensitive_data(bool is_sensitive) && {
  options_.sensitive_data = is_sensitive;
  return std::move(*this);
}

Replace Replace::deliver_node_max_age(std::chrono::nanoseconds duration) && {
  options_.edge_max_age = duration;
  return std::move(*this);
}

} // namespace fastly::cache::core
