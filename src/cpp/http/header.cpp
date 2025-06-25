#include <memory>
#include <vector>

#include "header.h"

namespace fastly::http {

std::string HeaderValuesIter::next() {
  std::unique_ptr<std::vector<uint8_t>> vec = this->iter->next();
  return std::string(vec->begin(), vec->end());
}

} // namespace fastly::http
