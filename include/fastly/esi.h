#ifndef FASTLY_ESI_H
#define FASTLY_ESI_H

#include <string>
#include <optional>
#include <functional>
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
        std::function<tl::expected<void, ExecutionError>(Request &, Response &)>
            process_fragment_response = nullptr)

        private : rust::Box<fastly::sys::esi::Processor> processor_;
    };

    using PendingFragmentContent = std::variant<PendingRequest, Request, std::monostate>;

    class DispatchFragmentRequestFn
    {
    public:
        DispatchFragmentRequestFn(std::function<expected<PendingFragmentContent>(Request)> fn)
            : fn_(std::move(fn)) {}
        uint32_t call(Request req, PendingResponse *&out_pending, Response *&out_complete, ExecutionError *&out_error) const noexcept;

    private:
        std::function<expected<PendingFragmentContent>(Request)> fn_;
    };
}
#endif