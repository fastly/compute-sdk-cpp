#ifndef FASTLY_ERROR_H
#define FASTLY_ERROR_H

#include "expected.h"
#include "sdk-sys.h"
#include <string>

namespace fastly::error {

using fastly::sys::error::FastlyErrorCode;

class FastlyError {
public:
  FastlyError(fastly::sys::error::FastlyError *e)
      : err(rust::Box<fastly::sys::error::FastlyError>::from_raw(e)) {};
  FastlyError(rust::Box<fastly::sys::error::FastlyError> e)
      : err(std::move(e)) {};
  FastlyErrorCode error_code();
  std::string error_msg();

private:
  rust::Box<fastly::sys::error::FastlyError> err;
};

template <class T> using expected = tl::expected<T, FastlyError>;
template <class T> using unexpected = tl::unexpected<T>;

} // namespace fastly::error

namespace fastly {
using fastly::error::expected;
using fastly::error::FastlyError;
using fastly::error::FastlyErrorCode;
using fastly::error::unexpected;
} // namespace fastly

#endif
