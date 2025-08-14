#ifndef FASTLY_HTTP_PURGE_H
#define FASTLY_HTTP_PURGE_H

#include <fastly/error.h>
#include <fastly/sdk-sys.h>
#include <string>
#include <string_view>

namespace fastly::http::purge {

/// Purge a surrogate key for the current service.
///
/// See the [Fastly purge documentation][doc] for details.
///
/// [doc]: https://developer.fastly.com/learning/concepts/purging/
fastly::expected<void> purge_surrogate_key(std::string_view surrogate_key);

/// Soft-purge a surrogate key for the current service.
///
/// See the [Fastly purge documentation][doc] for details.
///
/// [doc]: https://developer.fastly.com/learning/concepts/purging/
fastly::expected<void> soft_purge_surrogate_key(std::string_view surrogate_key);

} // namespace fastly::http::purge

#endif
