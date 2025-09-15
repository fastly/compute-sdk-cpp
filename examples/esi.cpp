#include "fastly/sdk.h"

int main() {
  fastly::log::init_simple("logs", fastly::log::LogLevelFilter::Debug);
  auto req{fastly::http::Request::from_client()};

  auto bereq = fastly::http::Request(fastly::http::Method::GET,
                                     "https://esi-cpp-demo.edgecompute.app/")
                   .with_auto_decompress_gzip(true);
  auto beresp = bereq.clone_without_body().send("esi-cpp-demo").value();

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
}
