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
} // namespace fastly
#endif // FASTLY_H