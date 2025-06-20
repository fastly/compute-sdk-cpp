#include "fastly/sdk.h"

using namespace std::string_literals;

int main() {
  auto req{fastly::Request::from_client()};
  auto body{req.take_body()};
  body.append(" How are you?"s);
  fastly::Response::from_body(std::move(body)).send_to_client();
}
