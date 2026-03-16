// The Compute Simple Cache API.
//
// This is a non-durable key-value API backed by the same cache platform as the
// Core Cache API.
//
// ## Cache scope and purging
//
// Cache entries are scoped to Fastly points of presence (POPs): the value set
// for a key in one POP will not be visible in any other POP.
//
// Purging is also scoped to a POP by default, but can be configured to purge
// globally with Fastly's purging feature.
//
// ## Interoperability
//
// The Simple Cache API is implemented in terms of the Core Cache API.
// Items inserted with the Core Cache API can be read by the Simple Cache API,
// and vice versa. However, some metadata and advanced features like
// revalidation may be not be available via the Simple Cache API.

#include <algorithm>
#include <fastly/cache/core.h>
#include <fastly/cache/simple.h>
#include <fastly/function_ref.h>
#include <fastly/http/purge.h>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace fastly::cache::simple {
namespace detail {
// Helper to convert core::CacheError to simple::CacheError
fastly::cache::simple::CacheError
from_core_error(const fastly::cache::core::CacheError &err) {
  using SimpleCode = fastly::cache::simple::CacheError::Code;
  using CoreCode = fastly::cache::core::CacheError::Code;

  switch (err.code()) {
  case CoreCode::LimitExceeded:
    return fastly::cache::simple::CacheError(SimpleCode::LimitExceeded);
  case CoreCode::InvalidOperation:
    return fastly::cache::simple::CacheError(SimpleCode::InvalidOperation);
  case CoreCode::Unsupported:
    return fastly::cache::simple::CacheError(SimpleCode::Unsupported);
  default:
    return fastly::cache::simple::CacheError(SimpleCode::Other);
  }
}
std::string surrogate_key_for_cache_key(std::span<const std::uint8_t> key,
                                        PurgeOptions::Scope scope) {
  return {fastly::sys::cache::f_cache_surrogate_key_for_cache_key(
      rust::Slice(key.data(), key.size()),
      static_cast<fastly::sys::cache::PurgeScope>(scope))};
}
} // namespace detail

tl::expected<std::optional<http::Body>, CacheError>
get(std::span<const std::uint8_t> key) {
  auto lookup_result = core::lookup(key).execute();
  if (!lookup_result.has_value()) {
    return tl::unexpected(detail::from_core_error(lookup_result.error()));
  }

  auto found_opt = lookup_result.value();
  if (!found_opt.has_value()) {
    return std::nullopt;
  }

  auto stream_result = found_opt->to_stream();
  if (!stream_result.has_value()) {
    return tl::unexpected(detail::from_core_error(stream_result.error()));
  }

  return std::optional<http::Body>(std::move(stream_result.value()));
}

tl::expected<http::Body, CacheError>
get_or_set(std::span<const std::uint8_t> key, http::Body value,
           std::chrono::nanoseconds ttl) {
  return get_or_set_with(key,
                         [val = std::move(value),
                          ttl]() mutable -> std::optional<CacheEntry> {
                           return CacheEntry{std::move(val), ttl};
                         })
      .and_then([](std::optional<http::Body> opt)
                    -> tl::expected<http::Body, CacheError> {
        // The provided closure is infallible, so we always have a value
        if (opt.has_value()) {
          return std::move(opt.value());
        }
        // Should never happen, but if it does, treat it as an error
        std::cerr << "get_or_set_with did not return a value\n";
        abort();
      });
}

tl::expected<void, CacheError> purge(std::span<const std::uint8_t> key) {
  return purge_with_opts(key, PurgeOptions::pop_scope());
}

tl::expected<void, CacheError>
purge_with_opts(std::span<const std::uint8_t> key, const PurgeOptions &opts) {
  std::string surrogate_key =
      detail::surrogate_key_for_cache_key(key, opts.scope());

  auto purge_result = http::purge::purge_surrogate_key(surrogate_key);
  if (!purge_result.has_value()) {
    return tl::unexpected(CacheError(CacheError::Code::Purge));
  }

  return {};
}

} // namespace fastly::cache::simple