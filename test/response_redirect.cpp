#include <catch2/catch_test_macros.hpp>
#include <fastly/http/response.h>

using namespace fastly::http;

namespace {

// Returns the response's `Location` header, or std::nullopt if absent.
std::optional<std::string> location_of(Response &resp) {
  auto header{resp.get_header("Location")};
  if (!header.has_value() || !header->has_value()) {
    return std::nullopt;
  }
  auto value{header->value().string()};
  if (!value.has_value()) {
    return std::nullopt;
  }
  return std::string(*value);
}

} // namespace

TEST_CASE("Response redirect constructors set the Location header",
          "[response]") {
  SECTION("Response::see_other") {
    auto resp{Response::see_other("https://www.fastly.com")};
    REQUIRE(location_of(resp) == std::optional("https://www.fastly.com"));
  }

  SECTION("Response::redirect") {
    auto resp{Response::redirect("https://www.fastly.com")};
    REQUIRE(location_of(resp) == std::optional("https://www.fastly.com"));
  }

  SECTION("Response::temporary_redirect") {
    auto resp{Response::temporary_redirect("https://www.fastly.com")};
    REQUIRE(location_of(resp) == std::optional("https://www.fastly.com"));
  }

  SECTION("accepts a std::string_view over a non-null-terminated buffer") {
    std::string buf{"https://www.fastly.com/path-and-then-some"};
    auto resp{Response::redirect(std::string_view(buf).substr(0, 22))};
    REQUIRE(location_of(resp) == std::optional("https://www.fastly.com"));
  }
}

// Required due to https://github.com/WebAssembly/wasi-libc/issues/485
#include <catch2/catch_session.hpp>
int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }
