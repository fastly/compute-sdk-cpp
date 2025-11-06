//! @example ngwaf_inspect.cpp
#include "fastly/sdk.h"

int main() {
  fastly::log::init_simple("logs");
  auto req{fastly::Request::from_client()};
  
  auto ires{fastly::security::inspect(
      req,
      fastly::security::InspectConfig().with_corp("my_corp").with_workspace(
          "my_workspace"))};
  auto verdict{ires->verdict()};
  
  fastly::Body body{"NGWAF Verdict: "};
  if (verdict == fastly::security::InspectVerdict::Allow) {
    body << "Allow";
  } else if (verdict == fastly::security::InspectVerdict::Block) {
    body << "Block";
  } else if (verdict == fastly::security::InspectVerdict::Unauthorized) {
    body << "Unauthorized";
  } else if (verdict == fastly::security::InspectVerdict::Other) {
    body << *ires->unrecognized_verdict_info() << " (Other)";
  }
  
  fastly::Response::from_body(std::move(body)).send_to_client();
}
