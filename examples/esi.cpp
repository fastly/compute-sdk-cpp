#include "fastly/sdk.h"

const auto html = R"(<!DOCTYPE html>
<html>
  <head>
    <title>My Shopping Website</title>
  </head>
  <body>
    <header style="background: #f1f1f1; padding: 16px">
      <h1>My Shopping Website</h1>
    </header>
    <div class="layout" style="display: flex">
      <esi:include src="https://mock-s3.edgecompute.app/_fragments/sidebar.html" onerror="continue"/>
      <esi:include src="/_fragments/content.html" onerror="continue"/>
      <esi:include src="/_fragments/doesnotexist.html" alt="https://mock-s3.edgecompute.app/_fragments/content.html"/>
      <esi:include src="/_fragments/doesnotexist.html" onerror="continue"/>
    </div>
  </body>
</html>
)";

auto to_string(fastly::http::Method method)
{
    switch (method)
    {
    case fastly::http::Method::GET:
        return "GET";
    case fastly::http::Method::POST:
        return "POST";
    case fastly::http::Method::PUT:
        return "PUT";
    case fastly::http::Method::DELETE:
        return "DELETE";
    default:
        return "UNKNOWN";
    }
}

int main()
{
    fastly::log::init_simple("logs");
    fastly::log::info("Starting ESI example");
    auto req{fastly::http::Request::from_client()};
    auto beresp = fastly::http::Response::from_body(html).with_content_type("text/html");
    fastly::esi::Processor processor(std::move(req));
    fastly::esi::DispatchFragmentRequestFn dispatch_fragment_request([](fastly::http::Request req) -> tl::expected<fastly::esi::PendingFragmentContent, fastly::esi::ExecutionError>
                                                                     {
        fastly::log::info("Sending request {} {}", to_string(req.get_method()), req.get_path());
        auto pending = std::move(req).with_ttl(120).send_async("mock-s3");
        if (pending)
        {
            return fastly::esi::PendingFragmentContent{std::move(*pending)};
        }
        else
        {
            return tl::unexpected(fastly::esi::ExecutionError{});
        } });
    fastly::log::info("Processor created");
    fastly::esi::ProcessFragmentResponseFn process_fragment_response([](fastly::http::Request &req, fastly::http::Response resp) -> tl::expected<fastly::http::Response, fastly::esi::ExecutionError>
                                                                     {
        fastly::log::info("Received response for {} {}", to_string(req.get_method()), req.get_path());

        return resp; });
    (void)processor.process_response(beresp, std::nullopt, dispatch_fragment_request, process_fragment_response);
}
