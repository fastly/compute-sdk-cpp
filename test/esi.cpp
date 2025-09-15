#include <catch2/catch_test_macros.hpp>
#include <fastly/esi.h>
#include <fastly/http/request.h>
#include <fastly/http/response.h>
#include <optional>
#include <string>
#include <variant>

using namespace fastly::esi;
using fastly::http::Request;
using fastly::http::Response;

TEST_CASE("Configuration default and custom values") {
  Configuration def;
  REQUIRE(def.get_namespace() == "esi");
  REQUIRE(def.is_escaped_content());

  Configuration custom("foo", false);
  REQUIRE(custom.get_namespace() == "foo");
  REQUIRE_FALSE(custom.is_escaped_content());
}

std::string_view html = R"(<!DOCTYPE html>
<html>
  <head>
    <title>My Shopping Website</title>
  </head>
  <body>
    <header style="background: #f1f1f1; padding: 16px">
      <h1>My Shopping Website</h1>
    </header>
    <div class="layout" style="display: flex">
      <esi:include src="https://esi-cpp-demo.edgecompute.app/_fragments/sidebar.html" onerror="continue"/>
      <esi:include src="/_fragments/content.html" onerror="continue"/>
      <esi:include src="/_fragments/doesnotexist.html" alt="https://esi-cpp-demo.edgecompute.app/_fragments/content.html"/>
      <esi:include src="/_fragments/doesnotexist.html" onerror="continue"/>
    </div>
  </body>
</html>
)";

TEST_CASE("Bare processor works") {
  fastly::esi::Processor processor;
  auto result =
      processor.process_document(std::string(html), std::nullopt, std::nullopt);
  REQUIRE(result.has_value());
  std::cout << *result;
}