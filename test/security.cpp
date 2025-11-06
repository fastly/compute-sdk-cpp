#include <catch2/catch_test_macros.hpp>
#include <fastly/security.h>

using namespace fastly::security;

TEST_CASE("Mock NGWAF returns allow", "[security]") {
  auto request = fastly::http::Request::get("https://example.com");
  auto config = InspectConfig()
                    .with_corp("test")
                    .with_workspace("test")
                    .with_buffer_size(256)
                    .with_client_ip("10.10.10.10");
  auto res = inspect(request, config);
  REQUIRE(res.has_value());
  REQUIRE(res->verdict() == InspectVerdict::Allow);
}

// This is currently the case in the implementation, but not reflected in the
// API because it is planned to change
TEST_CASE("NGWAF requires corp and workspace", "[security]") {
  auto request = fastly::http::Request::get("https://example.com");
  SECTION("Missing corp") {
    auto res = inspect(request, InspectConfig().with_workspace("test"));
    REQUIRE(!res.has_value());
    REQUIRE(res.error().error_code() == InspectErrorCode::InvalidConfig);
  }
  SECTION("Missing workspace") {
    auto res = inspect(request, InspectConfig().with_corp("test"));
    REQUIRE(!res.has_value());
    REQUIRE(res.error().error_code() == InspectErrorCode::InvalidConfig);
  }
  SECTION("Missing both") {
    auto res = inspect(request, InspectConfig());
    REQUIRE(!res.has_value());
    REQUIRE(res.error().error_code() == InspectErrorCode::InvalidConfig);
  }
}

// Required due to https://github.com/WebAssembly/wasi-libc/issues/485
#include <catch2/catch_session.hpp>
int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }