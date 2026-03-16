#ifndef FASTLY_CACHE_SIMPLE_H
#define FASTLY_CACHE_SIMPLE_H

#include <chrono>
#include <fastly/expected.h>
#include <fastly/http/body.h>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace fastly::cache::simple {
/// Errors arising from cache operations.
class CacheError {
public:
  enum Code {
    /// Operation failed due to a limit.
    LimitExceeded,
    /// An underlying Core Cache API operation found an invalid state.
    /// This should not arise during use of this API. If encountered, please
    /// report it as a bug.
    InvalidOperation,
    /// Cache operation is not supported.
    Unsupported,
    /// An I/O error occurred.
    Io,
    /// An error occurred when purging a value.
    Purge,
    /// An error occurred while running the closure argument of get_or_set_with.
    GetOrSet,
    /// An unknown error occurred.
    Other,
  };

  CacheError(Code code) : code_(code) {}
  Code code() const { return code_; }

private:
  Code code_;
};

/// The return type of the closure provided to get_or_set_with().
class CacheEntry {
public:
  CacheEntry(http::Body val, std::chrono::nanoseconds ttl)
      : value_(std::move(val)), ttl_(ttl) {}

  http::Body &value() { return value_; }
  const http::Body &value() const { return value_; }
  std::chrono::nanoseconds ttl() const { return ttl_; }

private:
  /// The value to cache.
  http::Body value_;
  /// The time-to-live for the cache entry.
  std::chrono::nanoseconds ttl_;
};

/// Options for purge operations.
class PurgeOptions {
public:
  enum Scope {
    /// Purge the key from the current POP (default behavior).
    Pop,
    /// Purge the key globally (requires additional Fastly configuration).
    Global,
  };

  /// Purge the key from the current POP (default behavior).
  ///
  /// This is the default option, and allows a higher throughput of purging
  /// than purging globally.
  static PurgeOptions pop_scope() { return PurgeOptions(Scope::Pop); }

  /// Purge the key globally.
  ///
  /// This requires the Fastly global purge feature to be enabled for your
  /// service. See the [Fastly purge
  /// documentation](https://developer.fastly.com/learning/concepts/purging/)
  /// for details.
  static PurgeOptions global_scope() { return PurgeOptions(Scope::Global); }

  Scope scope() const { return scope_; }

private:
  explicit PurgeOptions(Scope s) : scope_(s) {}
  Scope scope_;
};

namespace detail {
/// Internal function to generate surrogate keys for cache entries.
/// This is used internally by the simple cache API to enable purging.
std::string surrogate_key_for_cache_key(std::span<const std::uint8_t> key,
                                        PurgeOptions::Scope scope);

fastly::cache::simple::CacheError
from_core_error(const fastly::cache::core::CacheError &err);
} // namespace detail

/// Get the entry associated with the given cache key, if it exists.
///
/// Returns `nullopt` if the key is not in the cache, or `Body` with the cached
/// value if found.
///
/// ```cpp
/// std::vector<std::uint8_t> key_bytes = {0x01, 0x02, 0x03};
/// auto result = get(key_bytes);
/// ```
tl::expected<std::optional<http::Body>, CacheError>
get(std::span<const std::uint8_t> key);

/// Get the entry associated with the given cache key, if it exists.
///
/// Returns `nullopt` if the key is not in the cache, or `Body` with the cached
/// value if found.
///
/// ```cpp
/// auto result = get("my_key");
/// ```
inline tl::expected<std::optional<http::Body>, CacheError>
get(std::string_view key) {
  return get(std::span(reinterpret_cast<const std::uint8_t *>(key.data()),
                       key.size()));
}

/// Get the entry associated with the given cache key if it exists, or insert
/// and return the specified entry.
///
/// If the value is costly to compute, consider using `get_or_set_with()`
/// instead to avoid computation in the case where the value is already present.
///
/// The returned `Body` is always valid; either the cached value was found and
/// returned, or the new value was inserted and returned.
///
/// Example:
/// ```cpp
/// std::vector<std::uint8_t> key_bytes = {0x01, 0x02, 0x03};
/// auto value = get_or_set(key_bytes, http::Body("hello!"),
///                         std::chrono::nanoseconds(6000)).value();
/// std::string cached_string = value.take_body_string();
/// ```
tl::expected<http::Body, CacheError>
get_or_set(std::span<const std::uint8_t> key, http::Body value,
           std::chrono::nanoseconds ttl);

/// Get the entry associated with the given cache key if it exists, or insert
/// and return the specified entry.
///
/// If the value is costly to compute, consider using `get_or_set_with()`
/// instead to avoid computation in the case where the value is already present.
///
/// The returned `Body` is always valid; either the cached value was found and
/// returned, or the new value was inserted and returned.
///
/// Example:
/// ```cpp
/// auto value = get_or_set("my_key", http::Body("hello!"),
///                         std::chrono::nanoseconds(6000)).value();
/// std::string cached_string = value.take_body_string();
/// ```
inline tl::expected<http::Body, CacheError>
get_or_set(std::string_view key, http::Body value,
           std::chrono::nanoseconds ttl) {
  return get_or_set(
      std::span(reinterpret_cast<const std::uint8_t *>(key.data()), key.size()),
      std::move(value), ttl);
}

/// Get the entry associated with the given cache key if it exists, or insert
/// and return an entry specified by running the given closure.
///
/// The closure is only run when no value is present for the key, and no other
/// client is in the process of setting it. It takes no arguments, and returns
/// either a `CacheEntry` describing the entry to set, or `std::nullopt` in the
/// case of error.
///
/// Example successful insertion:
/// ```cpp
/// std::vector<std::uint8_t> key_bytes = {0x01, 0x02, 0x03};
/// auto value = get_or_set_with(key_bytes, []() -> std::optional<CacheEntry> {
///     return CacheEntry{http::Body("hello!"), std::chrono::nanoseconds(6000)};
/// }).value();
/// ```
template <class F>
tl::expected<std::optional<http::Body>, CacheError>
get_or_set_with(std::span<const std::uint8_t> key, F make_entry)
  requires std::invocable<F> &&
           std::same_as<std::invoke_result_t<F>, std::optional<CacheEntry>>
{
  auto lookup_result = core::Transaction::lookup(key).execute();
  if (!lookup_result.has_value()) {
    return tl::unexpected(detail::from_core_error(lookup_result.error()));
  }

  auto &lookup_tx = lookup_result.value();

  if (!lookup_tx.must_insert_or_update()) {
    if (auto found = lookup_tx.found()) {
      auto stream_result = found->to_stream();
      if (!stream_result.has_value()) {
        return tl::unexpected(detail::from_core_error(stream_result.error()));
      }
      return std::optional<http::Body>(std::move(stream_result.value()));
    } else {
      return tl::unexpected(CacheError(CacheError::Code::InvalidOperation));
    }
  }

  // Run the user-provided closure to produce the entry
  auto entry = make_entry();
  if (!entry.has_value()) {
    return tl::unexpected(CacheError(CacheError::Code::GetOrSet));
  }

  // Create surrogate keys for both POP and global purging
  std::vector<std::string> surrogate_keys = {
      detail::surrogate_key_for_cache_key(key, PurgeOptions::Scope::Pop),
      detail::surrogate_key_for_cache_key(key, PurgeOptions::Scope::Global),
  };

  auto insert_result = std::move(lookup_tx)
                           .insert(entry->ttl())
                           .surrogate_keys(surrogate_keys)
                           .execute_and_stream_back();

  if (!insert_result.has_value()) {
    return tl::unexpected(detail::from_core_error(insert_result.error()));
  }

  auto [insert_body, found] = std::move(insert_result.value());

  insert_body.append(std::move(entry->value()));
  auto finish_result = insert_body.finish();
  if (!finish_result.has_value()) {
    return tl::unexpected(CacheError(CacheError::Code::Other));
  }

  auto stream_result = found.to_stream();
  if (!stream_result.has_value()) {
    return tl::unexpected(detail::from_core_error(stream_result.error()));
  }

  return std::optional<http::Body>(std::move(stream_result.value()));
}

/// Get the entry associated with the given cache key if it exists, or insert
/// and return an entry specified by running the given closure.
///
/// The closure is only run when no value is present for the key, and no other
/// client is in the process of setting it. It takes no arguments, and returns
/// either a `CacheEntry` describing the entry to set, or `std::nullopt` in the
/// case of error.
///
/// Example successful insertion:
/// ```cpp
/// auto value = get_or_set_with("my_key", []() -> std::optional<CacheEntry> {
///     return CacheEntry{http::Body("hello!"), std::chrono::nanoseconds(6000)};
/// }).value();
/// ```
template <class F>
tl::expected<std::optional<http::Body>, CacheError>
get_or_set_with(std::string_view key, F make_entry) {
  return get_or_set_with(
      std::span(reinterpret_cast<const std::uint8_t *>(key.data()), key.size()),
      std::move(make_entry));
}

/// Purge the entry associated with the given cache key.
///
/// To configure the behavior of the purge, such as to purge globally
/// rather than within the POP, use `purge_with_opts()`.
///
/// Note: Purged values may persist in cache for a short time (~150ms or
/// less) after this function returns.
///
/// Example:
/// ```cpp
/// std::vector<std::uint8_t> key_bytes = {0x01, 0x02, 0x03};
/// purge(key_bytes);
/// ```
tl::expected<void, CacheError> purge(std::span<const std::uint8_t> key);

/// Purge the entry associated with the given cache key.
///
/// To configure the behavior of the purge, such as to purge globally
/// rather than within the POP, use `purge_with_opts()`.
///
/// Note: Purged values may persist in cache for a short time (~150ms or
/// less) after this function returns.
///
/// Example:
/// ```cpp
/// purge("my_key");
/// ```
inline tl::expected<void, CacheError> purge(std::string_view key) {
  return purge(std::span(reinterpret_cast<const std::uint8_t *>(key.data()),
                         key.size()));
}

/// Purge the entry associated with the given cache key with specific
/// options.
///
/// The `PurgeOptions` argument determines the scope of the purge
/// operation.
///
/// Note: Purged values may persist in cache for a short time (~150ms or
/// less) after this function returns.
///
/// Example POP-scoped purge (default):
/// ```cpp
/// std::vector<std::uint8_t> key_bytes = {0x01, 0x02, 0x03};
/// purge_with_opts(key_bytes, PurgeOptions::pop_scope());
/// // Equivalent to just: purge(key_bytes);
/// ```
///
/// Example global-scoped purge:
/// ```cpp
/// std::vector<std::uint8_t> key_bytes = {0x01, 0x02, 0x03};
/// purge_with_opts(key_bytes, PurgeOptions::global_scope());
/// ```
tl::expected<void, CacheError>
purge_with_opts(std::span<const std::uint8_t> key, const PurgeOptions &opts);

/// Purge the entry associated with the given cache key with specific
/// options.
///
/// The `PurgeOptions` argument determines the scope of the purge
/// operation.
///
/// Note: Purged values may persist in cache for a short time (~150ms or
/// less) after this function returns.
///
/// Example POP-scoped purge (default):
/// ```cpp
/// purge_with_opts("my_key", PurgeOptions::pop_scope());
/// // Equivalent to just: purge("my_key");
/// ```
///
/// Example global-scoped purge:
/// ```cpp
/// purge_with_opts("my_key", PurgeOptions::global_scope());
/// ```
inline tl::expected<void, CacheError>
purge_with_opts(std::string_view key, const PurgeOptions &opts) {
  return purge_with_opts(
      std::span(reinterpret_cast<const std::uint8_t *>(key.data()), key.size()),
      opts);
}

} // namespace fastly::cache::simple

#endif // FASTLY_CACHE_SIMPLE_H
