#include <fastly/esi.h>

namespace fastly::esi
{

    Processor::Processor(std::optional<Request> original_request_metadata = std::nullopt,
                         Configuration config = Configuration())
    {
        processor_ = fastly::sys::esi::m_esi_processor_new(original_request_metadata.has_value() ? &original_request_metadata->req : nullptr,
                                                           static_cast<std::string>(config.get_namespace()),
                                                           config.is_escaped_content());
    }

    uint32_t DispatchFragmentRequestFn::call(Request req, http::PendingResponse *&out_pending, http::Response *&out_complete, ExecutionError *&out_error) const noexcept;
    {
        auto res = fn_(std::move(req));
        if (!res)
        {
            out_error = new ExecutionError(res.error());
            return 3; // Error
        }
        if (std::holds_alternative<PendingResponse>(*res))
        {
            out_pending = new PendingResponse(std::get<PendingResponse>(*res));
            return 0; // Pending response
        }
        else if (std::holds_alternative<Request>(*res))
        {
            out_complete = new Response(std::get<Request>(*res));
            return 1; // Complete Response
        }
        else
        {
            return 2; // No content
        }
    }
}