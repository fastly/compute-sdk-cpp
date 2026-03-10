#include <catch2/catch_test_macros.hpp>
#include <fastly/erl.h>

using namespace fastly::erl;

// The Viceroy implementation of ERL is stubbed, so this just ensures that
// the C++ wrapper functions are correctly calling into the host and handling
// the results.

TEST_CASE("PenaltyBox add and has") {
  PenaltyBox box("testbox");
  auto result = box.add("bad_entry", std::chrono::minutes(5));
  REQUIRE(result.has_value());

    auto has_result = box.has("good_entry");
    REQUIRE(has_result.has_value());
    REQUIRE(!*has_result);
}

TEST_CASE("RateCounter increment, lookup_rate, and lookup_count") {
  RateCounter counter("testcounter");
  auto result = counter.increment("entry1", 1);
  REQUIRE(result.has_value());

  auto rate_result = counter.lookup_rate("entry1", RateWindow::OneSec);
  REQUIRE(rate_result.has_value());

  auto count_result = counter.lookup_count("entry1", CounterDuration::TenSec);
  REQUIRE(count_result.has_value());
}

TEST_CASE("ERL check_rate") {
  ERL erl(RateCounter("testcounter"), PenaltyBox("testbox"));
  auto result = erl.check_rate("entry1", 1, RateWindow::OneSec, 5,
                              std::chrono::minutes(5));
  REQUIRE(result.has_value());

  auto has_result = erl.penalty_box().has("entry1");
  REQUIRE(has_result.has_value());
}

// Required due to https://github.com/WebAssembly/wasi-libc/issues/485
#include <catch2/catch_session.hpp>
int main(int argc, char *argv[]) { return Catch::Session().run(argc, argv); }