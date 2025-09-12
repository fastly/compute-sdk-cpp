#ifndef FASTLY_ACCESS_BRIDGE_INTERNALS_H
#define FASTLY_ACCESS_BRIDGE_INTERNALS_H

#include <fastly/sdk-sys.h>

namespace fastly::detail {
// This type can be used to access the inner `rust::Box` of
// various wrapper types in the C++ SDK.
// It can also be used to construct wrapper types from raw pointers.
// This is intended for internal use only.
struct AccessBridgeInternals {
  template <class T> static auto &get(T &obj) { return obj.inner(); }
  template <class T> static auto &get(const T &obj) { return obj.inner(); }
  template <class T, class U> static auto from_raw(U *ptr) {
    return T(rust::Box<U>::from_raw(ptr));
  }
};
} // namespace fastly::detail

#endif