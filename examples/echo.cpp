#include "fastly/sdk.h"
#include <iostream>

using namespace std::string_literals;

int main() {
  auto body{fastly::Request::from_client().take_body()};
  body << " What's up?" << std::flush;
  fastly::Response::from_body(std::move(body)).send_to_client();
}
