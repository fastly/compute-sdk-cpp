#ifndef FASTLY_ESI_H
#define FASTLY_ESI_H

#include <concepts>
#include <fastly/detail/rust_bridge_tags.h>
#include <fastly/error.h>
#include <fastly/expected.h>
#include <fastly/http/request.h>
#include <fastly/http/response.h>
#include <fastly/sdk-sys.h>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace fastly::esi {
/// Used to configure optional behaviour within the ESI processor.
struct Configuration {
public:
  /// Create a new configuration object.
  /// \param namespc The namespace to use for ESI tags.
  /// \param is_escaped_content Whether to escape content by default.
  Configuration(std::string namespc = "esi", bool is_escaped_content = true)
      : namespace_(std::move(namespc)),
        is_escaped_content_(is_escaped_content) {}

  std::string_view get_namespace() const { return namespace_; }
  bool is_escaped_content() const { return is_escaped_content_; }

private:
  std::string namespace_;
  bool is_escaped_content_;
};

/// Content that can be returned from a fragment request dispatcher. This can
/// either be a pending request, a response, or an empty value to indicate that
/// no content is available.
using PendingFragmentContent =
    std::variant<http::request::PendingRequest, http::Response, std::monostate>;

/// A callback type used to dispatch requests for ESI fragments.
class DispatchFragmentRequestFn
    : public detail::rust_bridge_tags::esi::DispatchFragmentRequestFnTag {
public:
  /// The type of the dispatch function.
  using function_type =
      std::function<std::optional<PendingFragmentContent>(Request)>;

  template <std::convertible_to<function_type> F>
  DispatchFragmentRequestFn(F &&fn) : fn_(std::forward<F>(fn)) {}

private:
  friend detail::AccessBridgeInternals;
  auto &inner() const { return fn_; }
  function_type fn_;
};

/// A callback type used to process responses from ESI fragment requests.
class ProcessFragmentResponseFn
    : public detail::rust_bridge_tags::esi::ProcessFragmentResponseFnTag {
public:
  /// The type of the processing function.
  using function_type =
      std::function<std::optional<Response>(Request &, Response)>;

  template <std::convertible_to<function_type> F>
  ProcessFragmentResponseFn(F &&fn) : fn_(std::forward<F>(fn)) {}

private:
  friend detail::AccessBridgeInternals;
  auto &inner() const { return fn_; }
  function_type fn_;
};

/// An ESI processor that can process a response containing ESI tags, dispatch
/// requests for fragments, and process the fragment responses.
class Processor {
public:
  /// Create a new ESI processor with the given configuration.
  Processor(std::optional<Request> original_request_metadata = std::nullopt,
            Configuration config = Configuration());

  /// Process a response containing ESI tags, optionally using the given
  /// callbacks to dispatch requests for fragments and process the fragment
  /// responses.
  ///
  /// \param src_document The response containing ESI tags to process.
  /// \param client_response_metadata Optional original client request data used
  /// for fragment requests.
  /// \param dispatch_fragment_request Optional callback to dispatch requests
  /// for fragments.
  /// \param process_fragment_response Optional callback to process fragment
  /// responses.
  fastly::expected<void> process_response(
      Response &src_document,
      std::optional<Response> client_response_metadata = std::nullopt,
      std::optional<DispatchFragmentRequestFn> dispatch_fragment_request =
          std::nullopt,
      std::optional<ProcessFragmentResponseFn> process_fragment_response =
          std::nullopt);

  /// Process a string containing ESI tags, optionally using the given
  /// callbacks to dispatch requests for fragments and process the fragment
  /// responses.
  /// \param src_document The string containing ESI tags to process.
  /// \param dispatch_fragment_request Optional callback to dispatch requests
  /// for fragments.
  /// \param process_fragment_response Optional callback to process fragment
  /// responses.
  fastly::expected<std::string> process_document(
      const std::string &src_document,
      std::optional<DispatchFragmentRequestFn> dispatch_fragment_request =
          std::nullopt,
      std::optional<ProcessFragmentResponseFn> process_fragment_response =
          std::nullopt);

private:
  rust::Box<fastly::sys::esi::Processor> processor_;
};

} // namespace fastly::esi
#endif