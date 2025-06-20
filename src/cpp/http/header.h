#ifndef FASTLY_HTTP_HEADER_H
#define FASTLY_HTTP_HEADER_H

#include <memory>
#include <string>
#include <vector>

#include "../sdk-sys.h"

namespace fastly::http {

class HeaderIter {
public:
  HeaderIter(rust::Box<fastly::sys::http::HeaderIter> i)
      : iter(std::move(i)) {};
  std::string next();

private:
  rust::Box<fastly::sys::http::HeaderIter> iter;
};

} // namespace fastly::http

#endif
