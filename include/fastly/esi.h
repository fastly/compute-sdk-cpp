#ifndef FASTLY_ESI_H
#define FASTLY_ESI_H

#include <string>

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
    }

#endif