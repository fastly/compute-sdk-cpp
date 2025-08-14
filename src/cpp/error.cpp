#include <fastly/error.h>

namespace fastly::error {

FastlyErrorCode FastlyError::error_code() { return this->err->error_code(); }

std::string FastlyError::error_msg() {
  std::string msg;
  this->err->error_msg(msg);
  return msg;
}

} // namespace fastly::error
