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
  /// \param namespc The namespace to use for ESI tags. Defaults to "esi".
  /// \param is_escaped_content Whether to escape content by default. Defaults
  /// to true.
  Configuration(std::string namespc = "esi", bool is_escaped_content = true)
      : namespace_(std::move(namespc)),
        is_escaped_content_(is_escaped_content) {}

  std::string_view get_namespace() const { return namespace_; }
  bool is_escaped_content() const { return is_escaped_content_; }

private:
  std::string namespace_;
  bool is_escaped_content_;
};

using PendingFragmentContent =
    std::variant<http::request::PendingRequest, http::Response, std::monostate>;

class DispatchFragmentRequestFn
    : public detail::rust_bridge_tags::esi::DispatchFragmentRequestFnTag {
public:
  using function_type =
      std::function<std::optional<PendingFragmentContent>(Request)>;
  template <std::convertible_to<function_type> F>
  DispatchFragmentRequestFn(F &&fn) : fn_(std::forward<F>(fn)) {}

private:
  friend detail::AccessBridgeInternals;
  auto &inner() const { return fn_; }
  function_type fn_;
};

class ProcessFragmentResponseFn
    : public detail::rust_bridge_tags::esi::ProcessFragmentResponseFnTag {
public:
  using function_type =
      std::function<std::optional<Response>(Request &, Response)>;
  template <std::convertible_to<function_type> F>
  ProcessFragmentResponseFn(F &&fn) : fn_(std::forward<F>(fn)) {}

private:
  friend detail::AccessBridgeInternals;
  auto &inner() const { return fn_; }
  function_type fn_;
};

class Processor {
public:
  /// Create a new ESI processor with the given configuration.
  Processor(std::optional<Request> original_request_metadata = std::nullopt,
            Configuration config = Configuration());

  tl::expected<void, FastlyError> process_response(
      Response &src_document,
      std::optional<Response> client_response_metadata = std::nullopt,
      std::optional<DispatchFragmentRequestFn> dispatch_fragment_request =
          std::nullopt,
      std::optional<ProcessFragmentResponseFn> process_fragment_response =
          std::nullopt);

private:
  rust::Box<fastly::sys::esi::Processor> processor_;
};

} // namespace fastly::esi
#endif