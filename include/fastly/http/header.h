#ifndef FASTLY_HTTP_HEADER_H
#define FASTLY_HTTP_HEADER_H

#include <memory>
#include <string>
#include <vector>

#include <fastly/sdk-sys.h>

namespace fastly::http {

/// Iterates over multiple values for an individual header.
class HeaderValuesIter {
public:
  HeaderValuesIter(rust::Box<fastly::sys::http::HeaderValuesIter> i)
      : iter(std::move(i)) {};
  /// Gets the next value.
  std::string next();

private:
  rust::Box<fastly::sys::http::HeaderValuesIter> iter;
};

} // namespace fastly::http

#endif
