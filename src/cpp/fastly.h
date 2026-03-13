#ifndef FASTLY_H
#define FASTLY_H

#include <cstddef>
#include <cstdint>
#include <type_traits>

#define WASM_IMPORT(module, name)                                              \
  __attribute__((import_module(module), import_name(name)))

namespace fastly {
class Status {
public:
  enum Code : std::uint32_t {
    Ok = 0,
    GenericError = 1,
    InvalidArgument = 2,
    BadHandle = 3,
    BufferLen = 4,
    Unsupported = 5,
    BadAlign = 6,
    HttpInvalid = 7,
    HttpUser = 8,
    HttpIncomplete = 9,
    OptionalNone = 10,
    HttpHeadTooLarge = 11,
    HttpInvalidStatus = 12,
    LimitExceeded = 13,
  };
  bool is_ok() const { return code_ == Ok; }
  explicit operator bool() const { return is_ok(); }
  Code code() const { return code_; }
  Status() = default;
  Status(Code code) : code_(code) {}
  Status(const Status &) = default;
  Status &operator=(const Status &) = default;

private:
  Code code_;
};
static_assert(std::is_trivial_v<Status>, "Status must be trivial");

WASM_IMPORT("fastly_async_io", "is_ready")
Status async_is_ready(uint32_t busy_handle, uint32_t *is_ready_out);

WASM_IMPORT("fastly_erl", "check_rate")
Status check_rate(const char *rc, size_t rc_len, const char *entry,
                  size_t entry_len, uint32_t delta, uint32_t window,
                  uint32_t limit, const char *pb, size_t pb_len, uint32_t ttl,
                  bool *blocked_out);

WASM_IMPORT("fastly_erl", "ratecounter_increment")
Status ratecounter_increment(const char *rc, size_t rc_len, const char *entry,
                             size_t entry_len, uint32_t delta);

WASM_IMPORT("fastly_erl", "ratecounter_lookup_rate")
Status ratecounter_lookup_rate(const char *rc, size_t rc_len, const char *entry,
                               size_t entry_len, uint32_t window,
                               uint32_t *rate_out);

WASM_IMPORT("fastly_erl", "ratecounter_lookup_count")
Status ratecounter_lookup_count(const char *rc, size_t rc_len,
                                const char *entry, size_t entry_len,
                                uint32_t duration, uint32_t *count_out);

WASM_IMPORT("fastly_erl", "penaltybox_add")
Status penaltybox_add(const char *pb, size_t pb_len, const char *entry,
                      size_t entry_len, uint32_t ttl);

WASM_IMPORT("fastly_erl", "penaltybox_has")
Status penaltybox_has(const char *pb, size_t pb_len, const char *entry,
                      size_t entry_len, bool *has_out);

typedef struct __attribute__((aligned(8))) {
  uint64_t max_age_ns;
  uint32_t request_headers;
  const uint8_t *vary_rule_ptr;
  size_t vary_rule_len;
  uint64_t initial_age_ns;
  uint64_t stale_while_revalidate_ns;
  const uint8_t *surrogate_keys_ptr;
  size_t surrogate_keys_len;
  uint64_t length;
  const uint8_t *user_metadata_ptr;
  size_t user_metadata_len;
  uint64_t edge_max_age_ns;
  const char *service;
  size_t service_len;
} CacheWriteOptions;

#define FASTLY_CACHE_WRITE_OPTIONS_MASK_RESERVED (1 << 0)
#define FASTLY_CACHE_WRITE_OPTIONS_MASK_REQUEST_HEADERS (1 << 1)
#define FASTLY_CACHE_WRITE_OPTIONS_MASK_VARY_RULE (1 << 2)
#define FASTLY_CACHE_WRITE_OPTIONS_MASK_INITIAL_AGE_NS (1 << 3)
#define FASTLY_CACHE_WRITE_OPTIONS_MASK_STALE_WHILE_REVALIDATE_NS (1 << 4)
#define FASTLY_CACHE_WRITE_OPTIONS_MASK_SURROGATE_KEYS (1 << 5)
#define FASTLY_CACHE_WRITE_OPTIONS_MASK_LENGTH (1 << 6)
#define FASTLY_CACHE_WRITE_OPTIONS_MASK_USER_METADATA (1 << 7)
#define FASTLY_CACHE_WRITE_OPTIONS_MASK_SENSITIVE_DATA (1 << 8)
#define FASTLY_CACHE_WRITE_OPTIONS_MASK_EDGE_MAX_AGE_NS (1 << 9)
#define FASTLY_CACHE_WRITE_OPTIONS_MASK_SERVICE (1 << 10)

typedef struct __attribute__((aligned(8))) {
  uint32_t request_headers;
  const char *service;
  size_t service_len;
} CacheLookupOptions;

#define FASTLY_CACHE_LOOKUP_OPTIONS_MASK_RESERVED (1 << 0)
#define FASTLY_CACHE_LOOKUP_OPTIONS_MASK_REQUEST_HEADERS (1 << 1)
#define FASTLY_CACHE_LOOKUP_OPTIONS_MASK_SERVICE (1 << 2)
#define FASTLY_CACHE_LOOKUP_OPTIONS_MASK_ALWAYS_USE_REQUESTED_RANGE (1 << 3)

typedef struct __attribute__((aligned(8))) {
  uint32_t request_headers;
  uint32_t replace_strategy;
  const char *service;
  size_t service_len;
} CacheReplaceOptions;

#define FASTLY_CACHE_REPLACE_OPTIONS_MASK_RESERVED (1 << 0)
#define FASTLY_CACHE_REPLACE_OPTIONS_MASK_REQUEST_HEADERS (1 << 1)
#define FASTLY_CACHE_REPLACE_OPTIONS_MASK_REPLACE_STRATEGY (1 << 2)
#define FASTLY_CACHE_REPLACE_OPTIONS_MASK_SERVICE (1 << 3)
#define FASTLY_CACHE_REPLACE_OPTIONS_MASK_ALWAYS_USE_REQUESTED_RANGE (1 << 4)

// a cached object was found
#define FASTLY_HOST_CACHE_LOOKUP_STATE_FOUND (1 << 0)
// the cached object is valid to use (implies found)
#define FASTLY_HOST_CACHE_LOOKUP_STATE_USABLE (1 << 1)
// the cached object is stale (but may or may not be valid to use)
#define FASTLY_HOST_CACHE_LOOKUP_STATE_STALE (1 << 2)
// this client is requested to insert or revalidate an object
#define FASTLY_HOST_CACHE_LOOKUP_STATE_MUST_INSERT_OR_UPDATE (1 << 3)

struct CacheGetBodyOptions {
  uint64_t from;
  uint64_t to;
};

#define FASTLY_CACHE_GET_BODY_OPTIONS_MASK_RESERVED (1 << 0)
#define FASTLY_CACHE_GET_BODY_OPTIONS_MASK_FROM (1 << 1)
#define FASTLY_CACHE_GET_BODY_OPTIONS_MASK_TO (1 << 2)

WASM_IMPORT("fastly_cache", "transaction_lookup")
Status cache_transaction_lookup(const uint8_t *key_ptr, size_t key_len,
                                uint32_t options_mask,
                                CacheLookupOptions *options,
                                uint32_t *handle_out);

WASM_IMPORT("fastly_cache", "transaction_lookup_async")
Status cache_transaction_lookup_async(const uint8_t *key_ptr, size_t key_len,
                                      uint32_t options_mask,
                                      CacheLookupOptions *options,
                                      uint32_t *busy_handle_out);

WASM_IMPORT("fastly_cache", "cache_busy_handle_wait")
Status cache_busy_handle_wait(uint32_t busy_handle, uint32_t *handle_out);

WASM_IMPORT("fastly_cache", "lookup")
Status cache_lookup(const uint8_t *key_ptr, size_t key_len,
                    uint32_t options_mask, CacheLookupOptions *options,
                    uint32_t *handle_out);

WASM_IMPORT("fastly_cache", "insert")
Status cache_insert(const uint8_t *key_ptr, size_t key_len,
                    uint32_t options_mask, const CacheWriteOptions *options,
                    uint32_t *body_handle_out);

WASM_IMPORT("fastly_cache", "replace")
Status cache_replace(const uint8_t *key_ptr, size_t key_len,
                     uint32_t options_mask, CacheReplaceOptions *options,
                     uint32_t *replace_handle_out);

WASM_IMPORT("fastly_cache", "replace_insert")
Status cache_replace_insert(uint32_t replace_handle, uint32_t options_mask,
                            const CacheWriteOptions *options,
                            uint32_t *body_handle_out);

WASM_IMPORT("fastly_cache", "wait")
Status cache_wait(uint32_t handle);

WASM_IMPORT("fastly_cache", "transaction_update")
Status cache_transaction_update(uint32_t handle, uint32_t options_mask,
                                const CacheWriteOptions *options);

WASM_IMPORT("fastly_cache", "transaction_insert")
Status cache_transaction_insert(uint32_t handle, uint32_t options_mask,
                                const CacheWriteOptions *options,
                                uint32_t *body_handle_out);

WASM_IMPORT("fastly_cache", "transaction_insert_and_stream_back")
Status cache_transaction_insert_and_stream_back(
    uint32_t handle, uint32_t options_mask, const CacheWriteOptions *options,
    uint32_t *body_handle_out, uint32_t *cache_handle_out);

WASM_IMPORT("fastly_cache", "get_state")
Status cache_get_state(uint32_t handle, uint8_t *state_out);

WASM_IMPORT("fastly_cache", "replace_get_state")
Status cache_replace_get_state(uint32_t handle, uint8_t *state_out);

WASM_IMPORT("fastly_cache", "transaction_cancel")
Status cache_transaction_cancel(uint32_t handle);

WASM_IMPORT("fastly_cache", "get_age_ns")
Status cache_get_age_ns(uint32_t handle, uint64_t *age_out);

WASM_IMPORT("fastly_cache", "get_max_age_ns")
Status cache_get_max_age_ns(uint32_t handle, uint64_t *max_age_out);

WASM_IMPORT("fastly_cache", "get_stale_while_revalidate_ns")
Status cache_get_stale_while_revalidate_ns(uint32_t handle, uint64_t *swr_out);

WASM_IMPORT("fastly_cache", "get_hits")
Status cache_get_hits(uint32_t handle, uint64_t *hits_out);

WASM_IMPORT("fastly_cache", "get_length")
Status cache_get_length(uint32_t handle, uint64_t *length_out);

WASM_IMPORT("fastly_cache", "get_user_metadata")
Status cache_get_user_metadata(uint32_t handle, uint8_t *user_metadata_out,
                               size_t user_metadata_max_len,
                               size_t *nwritten_out);

WASM_IMPORT("fastly_cache", "get_body")
Status cache_get_body(uint32_t handle, uint32_t options_mask,
                      CacheGetBodyOptions *options, uint32_t *body_handle_out);

WASM_IMPORT("fastly_cache", "replace_get_age_ns")
Status cache_replace_get_age_ns(uint32_t handle, uint64_t *age_out);

WASM_IMPORT("fastly_cache", "replace_get_max_age_ns")
Status cache_replace_get_max_age_ns(uint32_t handle, uint64_t *max_age_out);

WASM_IMPORT("fastly_cache", "replace_get_stale_while_revalidate_ns")
Status cache_replace_get_stale_while_revalidate_ns(uint32_t handle,
                                                   uint64_t *swr_out);

WASM_IMPORT("fastly_cache", "replace_get_hits")
Status cache_replace_get_hits(uint32_t handle, uint64_t *hits_out);

WASM_IMPORT("fastly_cache", "replace_get_length")
Status cache_replace_get_length(uint32_t handle, uint64_t *length_out);

WASM_IMPORT("fastly_cache", "replace_get_user_metadata")
Status cache_replace_get_user_metadata(uint32_t handle,
                                       uint8_t *user_metadata_out,
                                       size_t user_metadata_max_len,
                                       size_t *nwritten_out);

WASM_IMPORT("fastly_cache", "replace_get_body")
Status cache_replace_get_body(uint32_t handle, uint32_t options_mask,
                              CacheGetBodyOptions *options,
                              uint32_t *body_handle_out);

WASM_IMPORT("fastly_cache", "close")
Status cache_close(uint32_t handle);

WASM_IMPORT("fastly_http_req", "new")
Status http_req_new(uint32_t *req_handle_out);

WASM_IMPORT("fastly_http_req", "close")
Status http_req_close(uint32_t req_handle);

WASM_IMPORT("fastly_http_req", "header_values_set")
Status http_req_header_values_set(uint32_t req_handle, const uint8_t *name,
                                  size_t name_len, const uint8_t *value,
                                  size_t value_len);
} // namespace fastly
#endif // FASTLY_H