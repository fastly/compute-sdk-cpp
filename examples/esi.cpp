#include "fastly/sdk.h"

int main() {
  fastly::log::init_simple("logs", fastly::log::LogLevelFilter::Debug);
  auto req{fastly::http::Request::from_client()};
  fastly::log::info("Making backend request...");
  auto bereq = fastly::http::Request(fastly::http::Method::GET,
                                     "https://esi-cpp-demo.edgecompute.app/")
                   .with_auto_decompress_gzip(true);
  auto beresp = bereq.clone_without_body().send("esi-cpp-demo").value();
  fastly::log::info("Got backend response");

  // Pass in the request made to the backend as a template for fragment
  // requests: this ensures that the fragment requests also ask for gzipped
  // content and automatically gunzip the response content.
  fastly::esi::Processor processor(std::move(bereq));
  auto dispatch_fragment_request = [](fastly::http::Request req)
      -> std::optional<fastly::esi::PendingFragmentContent> {
    auto pending = req.send_async("esi-cpp-demo");
    if (pending) {
      return fastly::esi::PendingFragmentContent{std::move(*pending)};
    } else {
      return std::nullopt;
    }
  };

  auto result = processor.process_response(
      beresp, std::nullopt, dispatch_fragment_request, std::nullopt);
  if (!result) {
    fastly::log::error("Failed to process response");
  }

  std::string_view html = R"(<!DOCTYPE html>
<html>
  <head>
    <title>My Shopping Website</title>
  </head>
  <body>
    <header style="background: #f1f1f1; padding: 16px">
      <h1>My Shopping Website</h1>
    </header>
    <div class="layout" style="display: flex">
      <esi:include src="https://esi-cpp-demo.edgecompute.app/_fragments/sidebar.html" onerror="continue"/>
      <esi:include src="/_fragments/content.html" onerror="continue"/>
      <esi:include src="/_fragments/doesnotexist.html" alt="https://esi-cpp-demo.edgecompute.app/_fragments/content.html"/>
      <esi:include src="/_fragments/doesnotexist.html" onerror="continue"/>
    </div>
  </body>
</html>
)";

  fastly::log::info("Processing bare document...");
  fastly::esi::Processor processor2;
  auto res = processor2.process_document(std::string(html), std::nullopt,
                                         std::nullopt);
  std::cout << *res;
}
