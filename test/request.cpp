#include <catch2/catch_test_macros.hpp>
#include <fastly/http/request.h>

using namespace fastly::http;

TEST_CASE("Request::get_method_str reflects a standard method", "[request]") {
  auto req = Request::get("https://example.com");
  REQUIRE(req.get_method_str() == "GET");
}

TEST_CASE("Request::create accepts a nonstandard method", "[request]") {
  auto req = Request::create("PURGE", "https://example.com");
  REQUIRE(req.has_value());
  REQUIRE(req->get_method_str() == "PURGE");
}

TEST_CASE("Request::create rejects an invalid method token", "[request]") {
  auto req = Request::create("BAD METHOD", "https://example.com");
  REQUIRE(!req.has_value());
  REQUIRE(req.error().error_code() == fastly::FastlyErrorCode::InvalidMethod);
}

TEST_CASE("Request::set_method(string) accepts a nonstandard method",
          "[request]") {
  auto req = Request::get("https://example.com");
  auto res = req.set_method("PURGE");
  REQUIRE(res.has_value());
  REQUIRE(req.get_method_str() == "PURGE");
}

TEST_CASE("Request::set_method(string) rejects an invalid method token",
          "[request]") {
  auto req = Request::get("https://example.com");
  auto res = req.set_method("BAD METHOD");
  REQUIRE(!res.has_value());
  REQUIRE(res.error().error_code() == fastly::FastlyErrorCode::InvalidMethod);
  // The request's method is unchanged after a failed set_method.
  REQUIRE(req.get_method_str() == "GET");
}

TEST_CASE("Request::with_method(string) accepts a nonstandard method",
          "[request]") {
  auto req = Request::get("https://example.com").with_method("PURGE");
  REQUIRE(req.has_value());
  REQUIRE(req->get_method_str() == "PURGE");
}

// Required due to https://github.com/WebAssembly/wasi-libc/issues/485
#include <catch2/catch_session.hpp>
int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }
