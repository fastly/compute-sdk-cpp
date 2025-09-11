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
        explicit Processor(std::optional<Request> original_request_metadata = std::nullopt,
                           Configuration config = Configuration())
        {
        }
    };

    class DispatchFragmentRequestFn
    {
    public:
        DispatchFragmentRequestFn(std::function<expected<PendingFragmentContent>(Request &)> fn)
            : fn_(std::move(fn)) {}
        expected<PendingFragmentContent> call(Request &req) const noexcept
        {
            return fn_(req);
        }

    private:
        std::function<expected<PendingFragmentContent>(Request &)> fn_;
    };
}
#endif