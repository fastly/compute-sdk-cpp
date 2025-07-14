#include "purge.h"
#include "sdk-sys.h"

namespace fastly::http::purge {

fastly::expected<void> purge_surrogate_key(std::string_view surrogate_key) {
  fastly::sys::error::FastlyError *err;
  fastly::sys::http::purge::f_http_purge_purge_surrogate_key(
      static_cast<std::string>(surrogate_key), err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<void>();
  }
}

fastly::expected<void>
soft_purge_surrogate_key(std::string_view surrogate_key) {
  fastly::sys::error::FastlyError *err;
  fastly::sys::http::purge::f_http_purge_soft_purge_surrogate_key(
      static_cast<std::string>(surrogate_key), err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<void>();
  }
}

} // namespace fastly::http::purge
