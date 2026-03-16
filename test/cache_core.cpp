#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <fastly/cache/core.h>
#include <fastly/http/body.h>

using namespace fastly::http;
using namespace fastly::cache::core;
using namespace std::string_literals;
using namespace std::chrono_literals;

TEST_CASE("strings and spans", "[cache_core]") {
  std::string key = "my_key";
  auto result = lookup(key).execute();
  REQUIRE(result);
  REQUIRE(!*result);

  std::vector<std::uint8_t> key_bytes = {0x01, 0x02, 0x03};
  std::span<const std::uint8_t> key_span(key_bytes);
  auto result2 = lookup(key_span).execute();
  REQUIRE(result2);
  REQUIRE(!*result2);
}

TEST_CASE("cache::core::insert", "[cache_core]") {
  auto contents("contents here"s);
  auto writer = insert("cache::core::insert", 1234ns)
                    .surrogate_keys({"my_key"s})
                    .known_length(contents.size())
                    .execute();

  REQUIRE(writer);

  *writer << contents;

  REQUIRE(writer->finish());
}

TEST_CASE("failed cache::core::lookup", "[cache_core]") {
  auto found = lookup("cache::core::lookup").execute();
  REQUIRE(found);
  REQUIRE(!*found);
}

TEST_CASE("cache::core::lookup", "[cache_core]") {
  auto contents("deadbeef badc0ffee"s);
  auto writer = insert("cache::core::lookup",
                       std::chrono::duration_cast<std::chrono::nanoseconds>(1s))
                    .execute();

  *writer << contents;
  REQUIRE(writer->finish());

  auto found = lookup("cache::core::lookup").execute();
  REQUIRE(found);
  REQUIRE(*found);
  auto stream = (*found)->to_stream();
  REQUIRE(stream);
  std::string from_lookup(std::istreambuf_iterator<char>(*stream), {});

  REQUIRE(contents == from_lookup);
}

TEST_CASE("cache::core::Found::user_metadata", "[cache_core]") {
  auto key = "cache::core::Found::user_metadata";
  auto contents("deadbeef badc0ffee"s);
  auto metadata = std::vector<uint8_t>{0xde, 0xad, 0xbe, 0xef};
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
  auto key = "cache::core::Transaction";
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

TEST_CASE("cache::core::Transaction with string_view", "[cache_core]") {
  std::string_view key = "cache::core::Transaction::string_view";
  auto contents("string view key test"s);

  auto transaction = Transaction::lookup(key).execute();
  REQUIRE(transaction);

  if (transaction->must_insert()) {
    auto writer =
        std::move(*transaction)
            .insert(std::chrono::duration_cast<std::chrono::nanoseconds>(1s))
            .execute();
    REQUIRE(writer);
    *writer << contents;
    REQUIRE(writer->finish());
  }
}

TEST_CASE("cache::core::Transaction with byte span", "[cache_core]") {
  std::vector<std::uint8_t> key_bytes = {0xca, 0xfe, 0xba, 0xbe};
  auto contents("byte span key test"s);

  auto transaction = Transaction::lookup(key_bytes).execute();
  REQUIRE(transaction);

  if (transaction->must_insert()) {
    auto writer =
        std::move(*transaction)
            .insert(std::chrono::duration_cast<std::chrono::nanoseconds>(1s))
            .execute();
    REQUIRE(writer);
    *writer << contents;
    REQUIRE(writer->finish());
  }
}

TEST_CASE("cache::core::Found metadata", "[cache_core]") {
  auto key = "cache::core::Found::metadata";
  auto contents("test content"s);
  auto ttl = std::chrono::duration_cast<std::chrono::nanoseconds>(10s);

  auto writer = insert(key, ttl).execute();
  REQUIRE(writer);
  *writer << contents;
  REQUIRE(writer->finish());

  auto found = lookup(key).execute();
  REQUIRE(found);
  REQUIRE(*found);

  // Check metadata methods
  auto age = (*found)->age();
  REQUIRE(age.count() >= 0);

  auto max_age = (*found)->max_age();
  REQUIRE(max_age == ttl);

  auto remaining = (*found)->remaining_ttl();
  REQUIRE(remaining.count() > 0);
  REQUIRE(remaining.count() <= ttl.count());

  REQUIRE((*found)->is_usable());
  REQUIRE_FALSE((*found)->is_stale());
}

TEST_CASE("cache::core::insert string overloads", "[cache_core]") {
  std::string key = "cache::core::insert::string_overload";
  auto contents("string overload test"s);

  // Test the inline string_view overload
  auto writer = insert(key, 1s).execute();
  REQUIRE(writer);
  *writer << contents;
  REQUIRE(writer->finish());

  // Verify with string lookup
  auto found = lookup(key).execute();
  REQUIRE(found);
  REQUIRE(*found);
}

TEST_CASE("cache::core::Transaction execute_and_stream_back", "[cache_core]") {
  auto key = "cache::core::Transaction::stream_back";
  auto contents("stream back test"s);

  auto transaction = Transaction::lookup(key).execute();
  REQUIRE(transaction);

  if (transaction->must_insert()) {
    auto result =
        std::move(*transaction)
            .insert(std::chrono::duration_cast<std::chrono::nanoseconds>(1s))
            .execute_and_stream_back();
    REQUIRE(result);

    auto &[writer, found] = *result;
    writer << contents;
    REQUIRE(writer.finish());

    // Read back immediately using the found object
    auto stream = found.to_stream();
    REQUIRE(stream);
    std::string from_stream(std::istreambuf_iterator<char>(*stream), {});
    REQUIRE(from_stream == contents);
  }
}

// Required due to https://github.com/WebAssembly/wasi-libc/issues/485
#include <catch2/catch_session.hpp>
int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }
