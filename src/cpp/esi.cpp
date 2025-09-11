#include <fastly/esi.h>
#include <fastly/detail/access_bridge_internals.h>
#include <optional>

namespace fastly::esi
{
    extern "C" uint32_t fastly$esi$manualbridge$DispatchFragmentRequestFn$call(const DispatchFragmentRequestFn &fn, fastly::sys::http::Request *raw_req, fastly::sys::http::request::PendingRequest *&out_pending, fastly::sys::http::Response *&out_complete, fastly::sys::esi::ExecutionError *&)
    {
        auto req = detail::AccessBridgeInternals::from_raw<http::Request>(raw_req);
        auto res = detail::AccessBridgeInternals::get(fn)(std::move(req));
        if (!res)
        {
            // TODO
            return 0; // Error
        }
        if (std::holds_alternative<http::request::PendingRequest>(*res))
        {
            out_pending = detail::AccessBridgeInternals::get(std::get<http::request::PendingRequest>(*res)).into_raw();
            return 1; // Pending response
        }
        else if (std::holds_alternative<http::Response>(*res))
        {
            out_complete = detail::AccessBridgeInternals::get(std::get<http::Response>(*res)).into_raw();
            return 2; // Complete Response
        }
        else
        {
            return 3; // No content
        }
    }

    extern "C" bool fastly$esi$manualbridge$ProcessFragmentResponseFn$call(const ProcessFragmentResponseFn &fn, fastly::sys::http::Request *raw_req, fastly::sys::http::Response *raw_resp, fastly::sys::http::Response *&out_resp, fastly::sys::esi::ExecutionError *&)
    {
        // DANGER: This call is very unsafe. We are *not* taking ownership of the
        // request as it's supposed to be passed as a borrow. However, the existing
        // Request type in C++ always takes ownership of the inner pointer, so we
        // store the the request in a box temporarily to avoid having to create
        // an entirely new wrapper type just for this. As such, we *must* release
        // the box without dropping it after the call to the user function, otherwise
        // we will double-free the inner pointer.
        auto req = detail::AccessBridgeInternals::from_raw<http::Request>(raw_req);

        auto resp = detail::AccessBridgeInternals::from_raw<http::Response>(raw_resp);
        auto res = detail::AccessBridgeInternals::get(fn)(req, std::move(resp));

        // DANGER: Here we release the box without dropping it, to avoid double-freeing
        detail::AccessBridgeInternals::get(req).into_raw();

        if (res)
        {
            out_resp = detail::AccessBridgeInternals::get(*res).into_raw();
            return true;
        }
        else
        {
            // TODO
            return false;
        }
    }

    Processor::Processor(std::optional<Request> original_request_metadata,
                         Configuration config)
        : processor_(fastly::sys::esi::m_static_esi_processor_new(original_request_metadata.has_value() ? &detail::AccessBridgeInternals::get(*original_request_metadata) : nullptr,
                                                                  static_cast<std::string>(config.get_namespace()),
                                                                  config.is_escaped_content()))
    {
    }

    tl::expected<void, ExecutionError> Processor::process_response(
        Response &src_document,
        std::optional<Response> client_response_metadata,
        std::optional<DispatchFragmentRequestFn> dispatch_fragment_request,
        std::optional<ProcessFragmentResponseFn> process_fragment_response)
    {
        fastly::sys::esi::ExecutionError *err = nullptr;
        bool success = fastly::sys::esi::m_esi_processor_process_response(
            std::move(processor_),
            *detail::AccessBridgeInternals::get(src_document),
            client_response_metadata.has_value() ? &detail::AccessBridgeInternals::get(*client_response_metadata) : nullptr,
            dispatch_fragment_request.has_value() ? reinterpret_cast<fastly::sys::esi::DispatchFragmentRequestFn *>(&*dispatch_fragment_request) : nullptr,
            process_fragment_response.has_value() ? reinterpret_cast<fastly::sys::esi::ProcessFragmentResponseFn *>(&*process_fragment_response) : nullptr,
            err);
        if (success)
        {
            return {};
        }
        else
        {
            return tl::unexpected(ExecutionError());
        }
    }
}