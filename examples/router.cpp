#include "fastly/sdk.h"
#include <iostream>
#include <regex>

using namespace std::string_literals;

// NOTE: Please evaluate the SECURITY of this for your own purposes if you base
// your code on this. `std::regex` is vulnerable to ReDOS and is not suitable
// for user-provided regexes. Other vulnerabilities may be present depending on
// your C++ version.

// Routes
const std::regex HOME("^/?$");
const std::regex BOOK_GET("^/books/(\\d+)$");
const std::regex BOOK_LIST("^/books/?$");

int main() {
  auto req{fastly::Request::from_client()};
  fastly::http::Body body;
  auto path{req.get_path()};
  std::smatch match;
  if (std::regex_match(path, match, HOME)) {
    body << "Welcome Home!";
  } else if (std::regex_match(path, match, BOOK_GET)) {
    body << "So you want to get book with ID " << match[1]
         << "? I'll look for it later idk.";
  } else if (std::regex_match(path, match, BOOK_LIST)) {
    body << "There's a bunch of books. I don't feel like listing them right "
            "now. That sounds like a pain.";
  } else {
    fastly::Response::from_status(404)
        .with_body("Route not found")
        .send_to_client();
    return 0;
  }
  body << std::endl;
  fastly::Response::from_body(std::move(body)).send_to_client();
}
