#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <fastly/cache/simple.h>
#include <fastly/http/body.h>
#include <span>
#include <unistd.h>

using namespace fastly::http;
using namespace fastly::cache::simple;
using namespace std::string_literals;
using namespace std::chrono_literals;

TEST_CASE("cache::simple::get string key", "[cache_simple]") {
  auto key = "cache::simple::get::string";

  // First get should return nullopt (not found)
  auto result = get(key);
  REQUIRE(result);
  REQUIRE(!result->has_value());
}

TEST_CASE("cache::simple::get byte span key", "[cache_simple]") {
  std::vector<std::uint8_t> key_bytes = {0xca, 0xfe, 0xba, 0xbe};
  std::span<const std::uint8_t> key_span(key_bytes);

  // First get should return nullopt (not found)
  auto result = get(key_span);
  REQUIRE(result);
  REQUIRE(!result->has_value());
}

TEST_CASE("cache::simple::get_or_set string key", "[cache_simple]") {
  auto key = "cache::simple::get_or_set::string";
  auto contents = "test content"s;

  // First call should insert
  auto result1 = get_or_set(key, Body(contents), 10s);
  REQUIRE(result1);
  std::string cached1 = result1->take_body_string();
  REQUIRE(cached1 == contents);

  // Second call should retrieve cached value
  auto result2 = get_or_set(key, Body("different content"), 10s);
  REQUIRE(result2);
  std::string cached2 = result2->take_body_string();
  REQUIRE(cached2 == contents); // Should be original, not "different content"
}

TEST_CASE("cache::simple::get_or_set byte span key", "[cache_simple]") {
  std::vector<std::uint8_t> key_bytes = {0x01, 0x02, 0x03, 0x04};
  std::span<const std::uint8_t> key_span(key_bytes);
  auto contents = "byte span test"s;

  // First call should insert
  auto result1 = get_or_set(key_span, Body(contents), 10s);
  REQUIRE(result1);
  std::string cached1 = result1->take_body_string();
  REQUIRE(cached1 == contents);

  // Verify with get
  auto result2 = get(key_span);
  REQUIRE(result2);
  REQUIRE(result2->has_value());
  std::string cached2 = (*result2)->take_body_string();
  REQUIRE(cached2 == contents);
}

TEST_CASE("cache::simple::get_or_set_with string key success",
          "[cache_simple]") {
  auto key = "cache::simple::get_or_set_with::success";
  auto contents = "closure content"s;

  int call_count = 0;
  auto make_entry = [&]() -> std::optional<CacheEntry> {
    call_count++;
    return CacheEntry{Body(contents), 10s};
  };

  // First call should run closure
  auto result1 = get_or_set_with(key, make_entry);
  REQUIRE(result1);
  REQUIRE(result1->has_value());
  REQUIRE(call_count == 1);
  std::string cached1 = (*result1)->take_body_string();
  REQUIRE(cached1 == contents);

  // Second call should NOT run closure (cache hit)
  auto result2 = get_or_set_with(key, make_entry);
  REQUIRE(result2);
  REQUIRE(result2->has_value());
  REQUIRE(call_count == 1); // Still 1, closure not called again
}

TEST_CASE("cache::simple::get_or_set_with failure returns nullopt",
          "[cache_simple]") {
  auto key = "cache::simple::get_or_set_with::failure";

  auto make_entry = []() -> std::optional<CacheEntry> {
    // Simulate failure by returning nullopt
    return std::nullopt;
  };

  auto result = get_or_set_with(key, make_entry);
  REQUIRE(!result);
  REQUIRE(result.error().code() == CacheError::Code::GetOrSet);
}

TEST_CASE("cache::simple::get_or_set_with byte span key", "[cache_simple]") {
  std::vector<std::uint8_t> key_bytes = {0xde, 0xad, 0xbe, 0xef};
  std::span<const std::uint8_t> key_span(key_bytes);
  auto contents = "byte span closure"s;

  auto make_entry = [&]() -> std::optional<CacheEntry> {
    return CacheEntry{Body(contents), 10s};
  };

  auto result = get_or_set_with(key_span, make_entry);
  REQUIRE(result);
  REQUIRE(result->has_value());
  std::string cached = (*result)->take_body_string();
  REQUIRE(cached == contents);
}

TEST_CASE("cache::simple::purge string key", "[cache_simple]") {
  auto key = "cache::simple::purge::string";
  auto contents = "to be purged"s;

  auto insert_result = get_or_set(key, Body(contents), 10s);
  REQUIRE(insert_result);
  auto get_result1 = get(key);
  REQUIRE(get_result1);
  REQUIRE(get_result1->has_value());
  auto purge_result = purge(key);
  REQUIRE(purge_result);

  // Verify it's gone (may still be present for a short time due to eventual
  // consistency)
  sleep(1); // Sleep for 1 second to allow purge to propagate
  auto get_result2 = get(key);
  REQUIRE(get_result2);
  REQUIRE(!get_result2->has_value());
}

TEST_CASE("cache::simple::purge byte span key", "[cache_simple]") {
  std::vector<std::uint8_t> key_bytes = {0x05, 0x06, 0x07, 0x08};
  std::span<const std::uint8_t> key_span(key_bytes);
  auto contents = "byte span purge"s;

  auto insert_result = get_or_set(key_span, Body(contents), 10s);
  REQUIRE(insert_result);
  auto get_result1 = get(key_span);
  REQUIRE(get_result1);
  REQUIRE(get_result1->has_value());
  auto purge_result = purge(key_span);
  REQUIRE(purge_result);

  // Verify it's gone (may still be present for a short time due to eventual
  // consistency)
  sleep(1); // Sleep for 1 second to allow purge to propagate
  auto get_result2 = get(key_span);
  REQUIRE(get_result2);
  REQUIRE(!get_result2->has_value());
}

TEST_CASE("cache::simple::purge_with_opts pop scope", "[cache_simple]") {
  auto key = "cache::simple::purge::pop_scope";
  auto contents = "pop scope test"s;

  auto insert_result = get_or_set(key, Body(contents), 10s);
  REQUIRE(insert_result);

  auto purge_result = purge_with_opts(key, PurgeOptions::pop_scope());
  REQUIRE(purge_result);
}

TEST_CASE("cache::simple::purge_with_opts global scope", "[cache_simple]") {
  auto key = "cache::simple::purge::global_scope";
  auto contents = "global scope test"s;

  auto insert_result = get_or_set(key, Body(contents), 10s);
  REQUIRE(insert_result);

  auto purge_result = purge_with_opts(key, PurgeOptions::global_scope());
  REQUIRE(purge_result);
}

TEST_CASE("cache::simple::CacheEntry construction", "[cache_simple]") {
  auto contents = "entry content"s;
  auto ttl = 30s;

  CacheEntry entry(Body(contents), ttl);

  REQUIRE(entry.ttl() == ttl);
  std::string body_str = entry.value().take_body_string();
  REQUIRE(body_str == contents);
}

TEST_CASE("cache::simple::PurgeOptions", "[cache_simple]") {
  auto pop_opts = PurgeOptions::pop_scope();
  REQUIRE(pop_opts.scope() == PurgeOptions::Scope::Pop);

  auto global_opts = PurgeOptions::global_scope();
  REQUIRE(global_opts.scope() == PurgeOptions::Scope::Global);
}

TEST_CASE("cache::simple::get_or_set with different TTLs", "[cache_simple]") {
  auto key = "cache::simple::ttl_test";
  auto contents = "ttl test"s;

  auto result = get_or_set(key, Body(contents), 1ns);
  REQUIRE(result);

  // The value might expire quickly, but the operation should succeed
  std::string cached = result->take_body_string();
  REQUIRE(cached == contents);
}

TEST_CASE("cache::simple::round trip with get_or_set and get",
          "[cache_simple]") {
  auto key = "cache::simple::round_trip";
  auto contents = "round trip content"s;

  auto set_result = get_or_set(key, Body(contents), 60s);
  REQUIRE(set_result);
  std::string set_value = set_result->take_body_string();
  REQUIRE(set_value == contents);

  auto get_result = get(key);
  REQUIRE(get_result);
  REQUIRE(get_result->has_value());
  std::string get_value = (*get_result)->take_body_string();
  REQUIRE(get_value == contents);
}

TEST_CASE("cache::simple::large value", "[cache_simple]") {
  auto key = "cache::simple::large_value";

  // Create a large string (100KB)
  std::string large_content(100 * 1024, 'x');

  auto result = get_or_set(key, Body(large_content), 60s);
  REQUIRE(result);
  std::string cached = result->take_body_string();
  REQUIRE(cached.size() == large_content.size());
  REQUIRE(cached == large_content);
}

TEST_CASE("cache::simple::empty value", "[cache_simple]") {
  auto key = "cache::simple::empty_value";
  std::string empty_content = "";

  auto result = get_or_set(key, Body(empty_content), 10s);
  REQUIRE(result);
  std::string cached = result->take_body_string();
  REQUIRE(cached.empty());
}

TEST_CASE("cache::simple::binary data", "[cache_simple]") {
  auto key = "cache::simple::binary_data";

  // Create binary data with null bytes
  std::vector<uint8_t> binary_data = {0x00, 0x01, 0x02, 0xff, 0xfe, 0x00, 0x7f};
  std::string binary_string(binary_data.begin(), binary_data.end());

  auto result = get_or_set(key, Body(binary_string), 60s);
  REQUIRE(result);
  std::string cached = result->take_body_string();
  REQUIRE(cached.size() == binary_data.size());
  REQUIRE(cached == binary_string);
}

// Required due to https://github.com/WebAssembly/wasi-libc/issues/485
#include <catch2/catch_session.hpp>
int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }
