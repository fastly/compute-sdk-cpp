#include "fastly/sdk.h"
#include <iostream>

using namespace std::string_literals;

int main() {
  auto backend_resp{
      fastly::Request::get("https://www.wikipedia.org/").send("wikipedia")};

  if (!backend_resp) {
    std::cerr << backend_resp.error().error_msg();
    fastly::Response::from_status(
        fastly::http::StatusCode::INTERNAL_SERVER_ERROR)
        .send_to_client();
    return 1;
  }

  // Take the body so we can iterate through its lines later
  auto backend_resp_body{backend_resp->take_body()};

  // Start sending the backend response to the client with a now-empty body
  auto client_body{backend_resp->stream_to_client()};

  size_t num_lines{0};
  bool got_data{false};
  std::string line;
  for (std::string line; std::getline(backend_resp_body, line);) {
    if (!got_data) {
      got_data = true;
    }
    if (backend_resp_body.rdstate() != std::ios_base::failbit) {
      // `failbit` gets set if `getline` receives a line that is too long.
      num_lines++;
    }
    client_body << line << std::endl;
    ;
  }

  if (got_data && num_lines == 0) {
    // We treat non-EOL single-line bodies as one line.
    num_lines++;
  }

  // Finish the streaming body to close the client connection.
  if (client_body.finish()) {
    std::cout << "backend response body contained " << num_lines << " lines"
              << std::endl;
  } else {
    std::cerr << "finishing client body failed." << std::endl;
  }
}
