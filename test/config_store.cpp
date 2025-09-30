#include <catch2/catch_test_macros.hpp>
#include <fastly/config_store.h>
#include <fastly/http/body.h>

using namespace fastly::http;
using namespace fastly::config_store;

TEST_CASE("ConfigStore::open returns error for unknown store",
          "[config_store]") {
  auto result = ConfigStore::open("nonexistent_store");
  REQUIRE(result.error().error_code() ==
          fastly::FastlyErrorCode::ConfigStoreOpenError);
}

TEST_CASE("Opening and reading from ConfigStore", "[config_store]") {
  auto store = ConfigStore::open("test-store");
  REQUIRE(store.has_value());

  SECTION("ConfigStore::contains") {
    REQUIRE(store->contains("hello").value());
    REQUIRE(!store->contains("does_not_exist").value());
  }

  SECTION("ConfigStore::get") {
    auto get_result = store->get("hello").value();
    REQUIRE(get_result.has_value());
    REQUIRE(get_result.value() == "world");

    auto no_result = store->get("does_not_exist").value();
    REQUIRE(!no_result.has_value());
  }
}

// Required due to https://github.com/WebAssembly/wasi-libc/issues/485
#include <catch2/catch_session.hpp>
int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }
