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
      <esi:include src="https://esi-cpp-demo.edgecompute.app/_fragments/content.html" onerror="continue"/>
      <esi:include src="/_fragments/doesnotexist.html" alt="https://esi-cpp-demo.edgecompute.app/_fragments/content.html"/>
      <esi:include src="/_fragments/doesnotexist.html" onerror="continue"/>
    </div>
  </body>
</html>
)";

#include <fastly/log.h>
TEST_CASE("Dispatch fragment callback works") {
  fastly::esi::Processor processor;

  auto dispatch_fragment_request = [](fastly::http::Request req)
      -> std::optional<fastly::esi::PendingFragmentContent> {
    auto pending = req.send_async("esi-cpp-demo");
    if (pending) {
      return fastly::esi::PendingFragmentContent{std::move(*pending)};
    } else {
      return std::nullopt;
    }
  };

  auto result = processor.process_document(
      std::string(html), dispatch_fragment_request, std::nullopt);
  REQUIRE(result.has_value());

  std::string_view expected = R"(<!DOCTYPE html>
<html>
  <head>
    <title>My Shopping Website</title>
  </head>
  <body>
    <header style="background: #f1f1f1; padding: 16px">
      <h1>My Shopping Website</h1>
    </header>
    <div class="layout" style="display: flex">
      <nav style="background: #f7f7f7; padding: 16px">
  <ul>
    <li><a href="/">Home</a></li>
  </ul>
</nav>

      <main style="padding: 32px">
  <div style="display: flex; gap: 16px">
    <img src="https://picsum.photos/250" />
    <img src="https://picsum.photos/250?1" />
    <img src="https://picsum.photos/250?2" />
  </div>
  <p>This is the page content.</p>
</main>
      <main style="padding: 32px">
  <div style="display: flex; gap: 16px">
    <img src="https://picsum.photos/250" />
    <img src="https://picsum.photos/250?1" />
    <img src="https://picsum.photos/250?2" />
  </div>
  <p>This is the page content.</p>
</main>
      
    </div>
  </body>
</html>
)";
  REQUIRE(*result == expected);
}

TEST_CASE("Process response callback works") {
  fastly::esi::Processor processor;

  auto dispatch_fragment_request = [](fastly::http::Request req)
      -> std::optional<fastly::esi::PendingFragmentContent> {
    auto pending = req.send_async("esi-cpp-demo");
    if (pending) {
      return fastly::esi::PendingFragmentContent{std::move(*pending)};
    } else {
      return std::nullopt;
    }
  };
  auto process_response =
      [](fastly::http::Request &,
         fastly::http::Response) -> std::optional<fastly::http::Response> {
    return fastly::http::Response::from_body(
        fastly::http::Body("<!-- Processed -->"));
  };

  auto result = processor.process_document(
      std::string(html), dispatch_fragment_request, process_response);
  REQUIRE(result.has_value());

  std::string_view expected = R"(<!DOCTYPE html>
<html>
  <head>
    <title>My Shopping Website</title>
  </head>
  <body>
    <header style="background: #f1f1f1; padding: 16px">
      <h1>My Shopping Website</h1>
    </header>
    <div class="layout" style="display: flex">
      <!-- Processed -->
      <!-- Processed -->
      <!-- Processed -->
      <!-- Processed -->
    </div>
  </body>
</html>
)";
  REQUIRE(*result == expected);
}

TEST_CASE("Return error from dispatch fragment callback fails processing") {
  fastly::esi::Processor processor;

  auto dispatch_fragment_request = [](fastly::http::Request)
      -> std::optional<fastly::esi::PendingFragmentContent> {
    return std::nullopt;
  };

  auto result = processor.process_document(
      std::string(html), dispatch_fragment_request, std::nullopt);
  REQUIRE_FALSE(result.has_value());
}

TEST_CASE("Return error from process response callback fails processing") {
  fastly::esi::Processor processor;

  auto dispatch_fragment_request = [](fastly::http::Request req)
      -> std::optional<fastly::esi::PendingFragmentContent> {
    auto pending = req.send_async("esi-cpp-demo");
    if (pending) {
      return fastly::esi::PendingFragmentContent{std::move(*pending)};
    } else {
      return std::nullopt;
    }
  };
  auto process_response = [](fastly::http::Request &, fastly::http::Response)
      -> std::optional<fastly::http::Response> { return std::nullopt; };

  auto result = processor.process_document(
      std::string(html), dispatch_fragment_request, process_response);
  REQUIRE_FALSE(result.has_value());
}

// Required due to https://github.com/WebAssembly/wasi-libc/issues/485
#include <catch2/catch_session.hpp>
int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }