// Some types (primarily callback types) must be defined in C++, but depend upon
// types defined in Rust. To break this circular dependency, we define empty
// "tag" structs here in C++ that can be referenced both from C++ and Rust,
// allowing Rust to pass pointers to these types back to C++ without needing to
// know their full definition.
//
// C++ types that implement the tags should inherit from the tag structs and the
// bindings should cast to/from the tag types as necessary.

#ifndef FASTLY_DETAIL_RUST_BRIDGE_TAGS_H
#define FASTLY_DETAIL_RUST_BRIDGE_TAGS_H

namespace fastly::detail::rust_bridge_tags {
namespace esi {
// esi.h:DispatchFragmentRequestFn
struct DispatchFragmentRequestFnTag {
  // Ensure that only classes that inherit from this tag can be used as
  // DispatchFragmentRequestFn.
protected:
  DispatchFragmentRequestFnTag() = default;
};
// esi.h:ProcessFragmentResponseFn
struct ProcessFragmentResponseFnTag {
protected:
  ProcessFragmentResponseFnTag() = default;
};
} // namespace esi
} // namespace fastly::detail::rust_bridge_tags

#endif