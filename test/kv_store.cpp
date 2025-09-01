#include <catch2/catch_test_macros.hpp>
#include <fastly/http/body.h>
#include <fastly/kv_store.h>

using namespace fastly::http;
using namespace fastly::kv_store;

TEST_CASE("KVStore::open returns error for unknown store", "[kv_store]") {
  auto result = KVStore::open("nonexistent_store");
  REQUIRE(result.error().error_code() == KVStoreErrorCode::StoreNotFound);
}

TEST_CASE("KVStore basic operations", "[kv_store]") {
  auto store_result = KVStore::open("test-store");
  REQUIRE(store_result.has_value());
  KVStore store = std::move(store_result->value());

  std::string key = "test_key";
  Body value_body("1234");

  SECTION("insert and lookup") {
    REQUIRE(store.insert(key, std::move(value_body)));
    auto lookup_result = store.lookup(key);
    REQUIRE(lookup_result.has_value());
    auto body_bytes = lookup_result->take_body();
    REQUIRE(body_bytes.take_body_string() == "1234");
  }

  SECTION("erase") {
    REQUIRE(store.insert(key, std::move(value_body)));
    auto erase_result = store.erase(key);
    REQUIRE(erase_result.has_value());
    auto lookup_result = store.lookup(key);
    REQUIRE(!lookup_result.has_value());
  }

  SECTION("build_insert") {
    auto result = store.build_insert()
                      .mode(InsertMode::Overwrite)
                      .execute(key, std::move(value_body));
    REQUIRE(result.has_value());
  }

  SECTION("pending_insert_wait") {
    auto async_result =
        store.build_insert().execute_async(key, std::move(value_body));
    REQUIRE(async_result.has_value());
    auto wait_result = store.pending_insert_wait(async_result.value());
    REQUIRE(wait_result.has_value());
    auto lookup_result = store.lookup(key);
    REQUIRE(lookup_result.has_value());
    auto body_bytes = lookup_result->take_body();
    REQUIRE(body_bytes.take_body_string() == "1234");
  }

  SECTION("build_lookup") {
    REQUIRE(store.insert(key, std::move(value_body)));
    auto result = store.build_lookup().execute(key);
    REQUIRE(result.has_value());
    auto async_result = store.build_lookup().execute_async(key);
    REQUIRE(async_result.has_value());
    auto wait_result = store.pending_lookup_wait(async_result.value());
    REQUIRE(wait_result.has_value());
  }

  SECTION("build_erase") {
    REQUIRE(store.insert(key, std::move(value_body)));
    auto result = store.build_erase().execute(key);
    REQUIRE(result.has_value());
    REQUIRE(store.insert(key, Body("1234")));
    auto async_result = store.build_erase().execute_async(key);
    REQUIRE(async_result.has_value());
    auto wait_result = store.pending_erase_wait(async_result.value());
    REQUIRE(wait_result.has_value());
  }

  SECTION("list and build_list") {
    REQUIRE(store.insert("a", std::move(value_body)));
    REQUIRE(store.insert("b", Body("hello")));
    auto list_result = store.list();
    REQUIRE(list_result.has_value());
    auto keys = list_result->keys();
    REQUIRE(keys.size() == 2);
    REQUIRE((keys[0] == "a" || keys[0] == "b"));
    REQUIRE((keys[1] == "a" || keys[1] == "b"));
    REQUIRE(keys[0] != keys[1]);

    auto page_result = store.build_list().execute();
    REQUIRE(page_result.has_value());
    auto iter = store.build_list().iter();
    for (auto &&page : iter) {
      REQUIRE(page.has_value());
      auto page_keys = page->keys();
      REQUIRE(page_keys.size() == 2);
      REQUIRE((page_keys[0] == "a" || page_keys[0] == "b"));
      REQUIRE((page_keys[1] == "a" || page_keys[1] == "b"));
      REQUIRE(page_keys[0] != page_keys[1]);
    }
  }

  SECTION("pending_list_wait") {
    auto builder = store.build_list();
    auto async_result = builder.execute_async();
    REQUIRE(async_result.has_value());
    auto wait_result = store.pending_list_wait(async_result.value());
    REQUIRE(wait_result.has_value());
  }
}

TEST_CASE("InsertBuilder advanced options", "[kv_store]") {
  auto store_result = KVStore::open("test-store");
  REQUIRE(store_result.has_value());
  KVStore store = std::move(store_result->value());

  std::string key = "advanced_key";
  Body value_body("hello");

  auto builder = store.build_insert()
                     .mode(InsertMode::Overwrite)
                     .background_fetch()
                     .if_generation_match(1)
                     .metadata("meta")
                     .time_to_live(std::chrono::milliseconds(1000));
  auto result = std::move(builder).execute(key, std::move(value_body));
  REQUIRE(result.has_value());
}

TEST_CASE("ListBuilder advanced options", "[kv_store]") {
  auto store_result = KVStore::open("test-store");
  REQUIRE(store_result.has_value());
  KVStore store = std::move(store_result->value());

  REQUIRE(store.insert("prefix_key1", Body("value1")));
  REQUIRE(store.insert("other_key", Body("value2")));
  REQUIRE(store.insert("prefix_key2", Body("value3")));
  REQUIRE(store.insert("prefix_key3", Body("value4")));

  auto page_result = store.build_list().limit(2).prefix("pre").execute();
  REQUIRE(page_result.has_value());

  auto keys = page_result->keys();
  REQUIRE(keys.size() == 2);
  REQUIRE((keys[0] == "prefix_key1" || keys[0] == "prefix_key2" ||
           keys[0] == "prefix_key3"));
  REQUIRE((keys[1] == "prefix_key1" || keys[1] == "prefix_key2" ||
           keys[1] == "prefix_key3"));
  REQUIRE(keys[0] != keys[1]);

  REQUIRE(page_result->limit() == 2);
  REQUIRE(page_result->prefix().has_value());
  REQUIRE(page_result->prefix().value() == "pre");
}

TEST_CASE("LookupResponse metadata and generation", "[kv_store]") {
  auto store_result = KVStore::open("test-store");
  REQUIRE(store_result.has_value());
  KVStore store = std::move(store_result->value());

  std::string key = "meta_key";
  Body value_body("hello");
  REQUIRE(store.build_insert()
              .mode(InsertMode::Overwrite)
              .metadata("my metadata")
              .execute(key, std::move(value_body)));

  auto lookup_result = store.lookup(key);
  REQUIRE(lookup_result.has_value());
  auto meta = lookup_result->metadata();
  REQUIRE(meta.has_value());
  REQUIRE(std::string(meta->begin(), meta->end()) == "my metadata");

  auto original_generation = lookup_result->current_generation();

  REQUIRE(store.build_insert()
              .mode(InsertMode::Overwrite)
              .if_generation_match(original_generation)
              .execute(key, Body("new value")));

  auto new_generation = store.lookup(key)->current_generation();
  REQUIRE(new_generation != original_generation);
}

// Required due to https://github.com/WebAssembly/wasi-libc/issues/485
#include <catch2/catch_session.hpp>
int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }