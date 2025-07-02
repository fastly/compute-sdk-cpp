#include "fastly/sdk.h"
#include "request.h"
#include <iostream>
#include <iterator>

using namespace std::string_literals;

int main() {
  // We can't use a plain `select({req1, req2, ...})` because PendingRequests
  // are not copyable, so we have to push them when building up the
  // `std::vector`.
  std::vector<fastly::http::request::PendingRequest> pending;
  pending.push_back(
      fastly::Request::get("https://www.fastly.com/"s).send_async("fastly"s));
  pending.push_back(
      fastly::Request::get("https://en.wikipedia.org/wiki/Fastly"s)
          .send_async("wikipedia"s));
  auto [resp, _other_pending] =
      fastly::http::request::select(std::move(pending));
  fastly::Body tail;
  tail << "\n\nThis was the request from "s << resp.get_backend_name().value()
       << "\nLocation: "s << resp.take_backend_request().value().get_url();
  resp.append_body(std::move(tail));
  resp.send_to_client();
}
