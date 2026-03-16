#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <fastly/cache/core.h>
#include <fastly/http/body.h>

using namespace fastly::http;
using namespace fastly::cache::core;
using namespace std::string_literals;
using namespace std::chrono_literals;

TEST_CASE("cache::core::insert", "[cache_core]") {
  auto key_string("cache::core::insert"s);
  std::vector<uint8_t> key(key_string.begin(), key_string.end());
  auto contents("contents here"s);
  auto writer = insert(key, 1234ns)
                    .surrogate_keys({"my_key"s})
                    .known_length(contents.size())
                    .execute();

  REQUIRE(writer);

  *writer << contents;

  REQUIRE(writer->finish());
}

TEST_CASE("failed cache::core::lookup", "[cache_core]") {
  auto key_string("cache::core::lookup"s);
  std::vector<uint8_t> key(key_string.begin(), key_string.end());
  auto found = lookup(key).execute();
  REQUIRE(found);
  REQUIRE(!*found);
}

TEST_CASE("cache::core::lookup", "[cache_core]") {
  auto key_string("cache::core::lookup"s);
  std::vector<uint8_t> key(key_string.begin(), key_string.end());
  auto contents("deadbeef badc0ffee"s);
  auto writer =
      insert(key, std::chrono::duration_cast<std::chrono::nanoseconds>(1s))
          .execute();

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

TEST_CASE("cache::core::Found::user_metadata", "[cache_core]") {
  auto key_string("cache::core::Found::user_metadata"s);
  std::vector<uint8_t> key(key_string.begin(), key_string.end());
  auto metadata = std::vector<uint8_t>{0xde, 0xad, 0xbe, 0xef};
  auto contents("deadbeef badc0ffee"s);
  auto writer =
      insert(key, std::chrono::duration_cast<std::chrono::nanoseconds>(1s))
          .user_metadata(metadata)
          .execute();

  *writer << contents;
  REQUIRE(writer->finish());

  auto found = lookup(key).execute();
  REQUIRE(found);
  REQUIRE(*found);

  REQUIRE((*found)->user_metadata() == metadata);
}

TEST_CASE("cache::core::Transaction", "[cache_core]") {
  auto key_string("cache::core::Transaction"s);
  std::vector<uint8_t> key(key_string.begin(), key_string.end());
  auto contents("contents here"s);

  auto transaction = Transaction::lookup(key).execute();
  if (!transaction) {
    FAIL("transaction lookup failed: " << transaction.error().code());
  }
  REQUIRE(transaction);

  if (transaction->must_insert()) {
    auto writer =
        std::move(*transaction)
            .insert(std::chrono::duration_cast<std::chrono::nanoseconds>(1s))
            .execute();
    REQUIRE(writer);
    *writer << contents;
    REQUIRE(writer->finish());
  } else {
    auto found = transaction->found();
    REQUIRE(found);
    auto stream = found->to_stream();
    REQUIRE(stream);
    std::string from_lookup(std::istreambuf_iterator<char>(*stream), {});
    REQUIRE(contents == from_lookup);
  }
}

// Required due to https://github.com/WebAssembly/wasi-libc/issues/485
#include <catch2/catch_session.hpp>
int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }
