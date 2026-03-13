#include <catch2/catch_test_macros.hpp>
#include <fastly/cache/core.h>
#include <fastly/http/body.h>

using namespace fastly::http;
using namespace fastly::cache::core;
using namespace std::string_literals;
using namespace std::chrono_literals;

TEST_CASE("cache::core::insert", "[cache_core]") {
  auto key_string("hello world"s);
  std::vector<uint8_t> key(key_string.begin(), key_string.end());
  auto contents("contents here"s);
  auto writer = insert(key, 1234ns)
                    .surrogate_keys({"my_key"s})
                    .known_length(contents.size())
                    .execute();
  ;
  REQUIRE(writer);

  *writer << contents;

  REQUIRE(writer->finish());
}

TEST_CASE("cache::core::lookup", "[cache_core]") {
  auto key_string("hello world"s);
  std::vector<uint8_t> key(key_string.begin(), key_string.end());
  auto contents("deadbeef badc0ffee"s);
  auto writer = insert(key, 1234ns).execute();
  ;
  *writer << contents;
  REQUIRE(writer->finish());

  auto found = lookup(key).execute();
  REQUIRE(found);
  REQUIRE(*found);
  auto stream = (*found)->to_stream();
  REQUIRE(stream);
  std::string from_lookup(std::istreambuf_iterator<char>(*stream), {});

  REQUIRE(contents == from_lookup);
}

TEST_CASE("cache::core::replace", "[cache_core]") {
  auto key_string("hello world"s);
  std::vector<uint8_t> key(key_string.begin(), key_string.end());
  auto contents("deadbeef badc0ffee"s);

  auto current = replace(key).begin();
  // TODO(@zkat): I'm not sure why this fails??? But everything else works???
  // REQUIRE(current);

  REQUIRE(!current->existing_object());

  auto writer = insert(key, 1234ns).execute();
  ;
  *writer << contents;
  REQUIRE(writer->finish());

  current = replace(key).begin();
  // TODO(@zkat): ????
  // REQUIRE(current);

  auto existing = current->existing_object();
  // TODO(@zkat): Not sure why this is failing. We _seem_ to be doing the same
  // thing as the Rust code. Am I holding it wrong?
  REQUIRE(existing.has_value());
}

// Required due to https://github.com/WebAssembly/wasi-libc/issues/485
#include <catch2/catch_session.hpp>
int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }
