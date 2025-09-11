#ifndef FASTLY_ESI_H
#define FASTLY_ESI_H

#include <string>
#include <optional>
#include <functional>
#include <variant>
#include <utility>
#include <fastly/error.h>
#include <fastly/http/request.h>
#include <fastly/http/response.h>
#include <fastly/expected.h>
#include <fastly/sdk-sys.h>

namespace fastly::esi
{
    /// Used to configure optional behaviour within the ESI processor.
    struct Configuration
    {
    public:
        /// Create a new configuration object.
        /// \param namespc The namespace to use for ESI tags. Defaults to "esi".
        /// \param is_escaped_content Whether to escape content by default. Defaults to true.
        Configuration(std::string namespc = "esi", bool is_escaped_content = true)
            : namespace_(std::move(namespc)), is_escaped_content_(is_escaped_content) {}

        std::string_view get_namespace() const { return namespace_; }
        bool is_escaped_content() const { return is_escaped_content_; }

    private:
        std::string namespace_;
        bool is_escaped_content_;
    };

    class ExecutionError
    {
    };

    using PendingFragmentContent = std::variant<http::request::PendingRequest, http::Response, std::monostate>;

    class DispatchFragmentRequestFn
    {
    public:
        DispatchFragmentRequestFn(std::function<tl::expected<PendingFragmentContent, ExecutionError>(Request)> fn)
            : fn_(std::move(fn)) {}

    private:
        friend detail::AccessBridgeInternals;
        auto &inner() const { return fn_; }
        std::function<tl::expected<PendingFragmentContent, ExecutionError>(Request)> fn_;
    };

    class ProcessFragmentResponseFn
    {
    public:
        ProcessFragmentResponseFn(std::function<tl::expected<Response, ExecutionError>(Request &, Response)> fn)
            : fn_(std::move(fn)) {}

    private:
        friend detail::AccessBridgeInternals;
        auto &inner() const { return fn_; }
        std::function<tl::expected<Response, ExecutionError>(Request &, Response)> fn_;
    };

    class Processor
    {
    public:
        /// Create a new ESI processor with the given configuration.
        Processor(std::optional<Request> original_request_metadata = std::nullopt,
                  Configuration config = Configuration());

        tl::expected<void, ExecutionError> process_response(
            Response &src_document,
            std::optional<Response> client_response_metadata = std::nullopt,
            std::optional<DispatchFragmentRequestFn> dispatch_fragment_request = std::nullopt,
            std::optional<ProcessFragmentResponseFn> process_fragment_response = std::nullopt);

    private:
        rust::Box<fastly::sys::esi::Processor> processor_;
    };

}
#endif