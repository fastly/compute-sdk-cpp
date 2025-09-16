#include <fastly/detail/access_bridge_internals.h>
#include <fastly/detail/rust_bridge_tags.h>
#include <fastly/error.h>
#include <fastly/esi.h>
#include <fastly/log.h>
#include <optional>

namespace fastly::esi {
// These functions are called by Rust to invoke the C++ callbacks.
extern "C" fastly::sys::esi::DispatchFragmentRequestFnResult
fastly$esi$manualbridge$DispatchFragmentRequestFn$call(
    const detail::rust_bridge_tags::esi::DispatchFragmentRequestFnTag &fn_tag,
    fastly::sys::http::Request *raw_req,
    fastly::sys::http::request::PendingRequest *&out_pending,
    fastly::sys::http::Response *&out_complete) {
  auto req = detail::AccessBridgeInternals::from_raw<http::Request>(raw_req);

  // The real callback type is cast to this tag type before being passed in to
  // Rust to break circular dependencies. It's safe to cast it back here, as if
  // some other type was passed in, something has gone horribly wrong.
  auto fn = static_cast<const DispatchFragmentRequestFn &>(fn_tag);

  auto res = detail::AccessBridgeInternals::get(fn)(std::move(req));
  if (!res) {
    return fastly::sys::esi::DispatchFragmentRequestFnResult::Error;
  }
  if (std::holds_alternative<http::request::PendingRequest>(*res)) {
    out_pending = detail::AccessBridgeInternals::get(
                      std::get<http::request::PendingRequest>(*res))
                      .into_raw();
    return fastly::sys::esi::DispatchFragmentRequestFnResult::PendingRequest;
  } else if (std::holds_alternative<http::Response>(*res)) {
    out_complete =
        detail::AccessBridgeInternals::get(std::get<http::Response>(*res))
            .into_raw();
    return fastly::sys::esi::DispatchFragmentRequestFnResult::CompletedRequest;
  } else {
    return fastly::sys::esi::DispatchFragmentRequestFnResult::NoContent;
  }
}

extern "C" bool fastly$esi$manualbridge$ProcessFragmentResponseFn$call(
    const detail::rust_bridge_tags::esi::ProcessFragmentResponseFnTag &fn_tag,
    fastly::sys::http::Request *raw_req, fastly::sys::http::Response *raw_resp,
    fastly::sys::http::Response *&out_resp) {
  // Ideally we wouldn't do this and would instead pass a reference, but
  // that would require a separate wrapper type for Request references.
  auto req = detail::AccessBridgeInternals::from_raw<http::Request>(raw_req);

  // The real callback type is cast to this tag type before being passed in to
  // Rust to break circular dependencies. It's safe to cast it back here, as if
  // some other type was passed in, something has gone horribly wrong.
  auto fn = static_cast<const ProcessFragmentResponseFn &>(fn_tag);

  auto resp = detail::AccessBridgeInternals::from_raw<http::Response>(raw_resp);
  auto res = detail::AccessBridgeInternals::get(fn)(req, std::move(resp));

  if (res) {
    out_resp = detail::AccessBridgeInternals::get(*res).into_raw();
    return true;
  } else {
    return false;
  }
}

Processor::Processor(std::optional<Request> original_request_metadata,
                     Configuration config)
    : processor_(fastly::sys::esi::m_static_esi_processor_new(
          // The Rust side will take ownership
          original_request_metadata.has_value()
              ? detail::AccessBridgeInternals::get(*original_request_metadata)
                    .into_raw()
              : nullptr,
          static_cast<std::string>(config.get_namespace()),
          config.is_escaped_content())) {}

tl::expected<void, FastlyError> Processor::process_response(
    Response &src_document, std::optional<Response> client_response_metadata,
    std::optional<DispatchFragmentRequestFn> dispatch_fragment_request,
    std::optional<ProcessFragmentResponseFn> process_fragment_response) {
  fastly::sys::error::FastlyError *err;
  // The Rust side will take ownership
  auto raw_metadata =
      client_response_metadata.has_value()
          ? detail::AccessBridgeInternals::get(*client_response_metadata)
                .into_raw()
          : nullptr;

  // We convert the callbacks to their tag types here, or pass null if not
  // present. They will be converted back to their real types when the C++
  // callback bindings are invoked from Rust.
  detail::rust_bridge_tags::esi::DispatchFragmentRequestFnTag
      *dispatch_fragment_tag =
          dispatch_fragment_request.has_value() ? &*dispatch_fragment_request
                                                : nullptr;
  detail::rust_bridge_tags::esi::ProcessFragmentResponseFnTag
      *process_fragment_tag =
          process_fragment_response.has_value() ? &*process_fragment_response
                                                : nullptr;

  bool success = fastly::sys::esi::m_esi_processor_process_response(
      std::move(processor_), *detail::AccessBridgeInternals::get(src_document),
      raw_metadata, dispatch_fragment_tag, process_fragment_tag, err);
  if (success) {
    return {};
  } else {
    return tl::unexpected(error::FastlyError(err));
  }
}

tl::expected<std::string, FastlyError> Processor::process_document(
    const std::string &src_document,
    std::optional<DispatchFragmentRequestFn> dispatch_fragment_request,
    std::optional<ProcessFragmentResponseFn> process_fragment_response) {
  fastly::sys::error::FastlyError *err;
  // We convert the callbacks to their tag types here, or pass null if not
  // present. They will be converted back to their real types when the C++
  // callback bindings are invoked from Rust.
  detail::rust_bridge_tags::esi::DispatchFragmentRequestFnTag
      *dispatch_fragment_tag =
          dispatch_fragment_request.has_value() ? &*dispatch_fragment_request
                                                : nullptr;
  detail::rust_bridge_tags::esi::ProcessFragmentResponseFnTag
      *process_fragment_tag =
          process_fragment_response.has_value() ? &*process_fragment_response
                                                : nullptr;

  std::string out;
  bool success = fastly::sys::esi::m_esi_processor_process_document(
      std::move(processor_), src_document, dispatch_fragment_tag,
      process_fragment_tag, out, err);
  if (success) {
    return out;
  } else {
    return tl::unexpected(error::FastlyError(err));
  }
}
} // namespace fastly::esi