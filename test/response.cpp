#include <catch2/catch_test_macros.hpp>
#include <fastly/http/response.h>
#include <fastly/http/status_code.h>

using namespace fastly::http;

TEST_CASE("Response::get_status", "[response]") {
  SECTION("new responses default to 200 OK") {
    Response resp;
    REQUIRE(resp.get_status().as_code() == 200);
    REQUIRE(resp.get_status() == StatusCode::OK);
  }

  SECTION("round-trips from_status") {
    REQUIRE(Response::from_status(404).get_status().as_code() == 404);
    REQUIRE(Response::from_status(StatusCode::NOT_FOUND).get_status() ==
            StatusCode::NOT_FOUND);
  }

  SECTION("reflects set_status") {
    Response resp;
    resp.set_status(StatusCode::IM_A_TEAPOT);
    REQUIRE(resp.get_status().as_code() == 418);
  }

  SECTION("reflects with_status") {
    auto resp{Response().with_status(StatusCode::SERVICE_UNAVAILABLE)};
    REQUIRE(resp.get_status().as_code() == 503);
  }

  SECTION("interoperates with StatusCode helpers") {
    REQUIRE(Response::from_status(204).get_status().is_success());
    REQUIRE(Response::from_status(StatusCode::PERMANENT_REDIRECT)
                .get_status()
                .is_redirection());
    REQUIRE(Response::from_status(500).get_status().is_server_error());
  }
}

TEST_CASE("StatusCode::from_code", "[response]") {
  REQUIRE(StatusCode::from_code(404) == std::optional(StatusCode::NOT_FOUND));
  REQUIRE(StatusCode::from_code(99) == std::nullopt);
  REQUIRE(StatusCode::from_code(1000) == std::nullopt);
}

// Required due to https://github.com/WebAssembly/wasi-libc/issues/485
#include <catch2/catch_session.hpp>
int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }
