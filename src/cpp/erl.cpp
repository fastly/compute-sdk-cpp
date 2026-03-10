#include "fastly.h"
#include <fastly/erl.h>
#include <iostream>

namespace {
fastly::erl::ERLError from_status(const fastly::Status &status) {
  using Code = fastly::Status::Code;
  switch (status.code()) {
  case Code::InvalidArgument:
    return fastly::erl::ERLError(fastly::erl::ERLError::Code::InvalidArgument);
  case Code::GenericError:
    return fastly::erl::ERLError(fastly::erl::ERLError::Code::Unexpected);
  default:
    return fastly::erl::ERLError(fastly::erl::ERLError::Code::Unknown);
  }
}
} // namespace

#define ERL_TRY(expr)                                                          \
  do {                                                                         \
    auto status = (expr);                                                      \
    if (!status.is_ok()) {                                                     \
      return tl::unexpected(from_status(status));                              \
    }                                                                          \
  } while (0)

namespace fastly::erl {
tl::expected<void, ERLError> PenaltyBox::add(std::string_view entry,
                                             std::chrono::minutes ttl) {
  // The host expects the TTL in seconds, even though it's truncated to minutes.
  std::chrono::seconds ttl_seconds =
      std::chrono::duration_cast<std::chrono::seconds>(ttl);
  ERL_TRY(fastly::penaltybox_add(name_.c_str(), name_.size(), entry.data(),
                                 entry.size(), ttl_seconds.count()));
  return {};
}
tl::expected<bool, ERLError> PenaltyBox::has(std::string_view entry) const {
    alignas(4) bool has_out;
  ERL_TRY(fastly::penaltybox_has(name_.c_str(), name_.size(), entry.data(),
                                 entry.size(), &has_out));
  return has_out;
}

tl::expected<void, ERLError> RateCounter::increment(std::string_view entry,
                                                    std::uint32_t delta) {
  ERL_TRY(fastly::ratecounter_increment(name_.c_str(), name_.size(),
                                        entry.data(), entry.size(), delta));
  return {};
}

tl::expected<std::uint32_t, ERLError>
RateCounter::lookup_rate(std::string_view entry, RateWindow window) const {
  std::uint32_t rate_out;
  ERL_TRY(fastly::ratecounter_lookup_rate(
      name_.c_str(), name_.size(), entry.data(), entry.size(),
      static_cast<std::uint32_t>(window), &rate_out));
  return rate_out;
}

tl::expected<std::uint32_t, ERLError>
RateCounter::lookup_count(std::string_view entry, CounterDuration duration) const {
  std::uint32_t count_out;
  ERL_TRY(fastly::ratecounter_lookup_count(
      name_.c_str(), name_.size(), entry.data(), entry.size(),
      static_cast<std::uint32_t>(duration), &count_out));
  return count_out;
}

tl::expected<bool, ERLError>
ERL::check_rate(std::string_view entry, std::uint32_t delta, RateWindow window,
                std::uint32_t limit, std::chrono::minutes ttl) const {
  // The host expects the TTL in seconds, even though it's truncated to minutes.
  std::chrono::seconds ttl_seconds =
      std::chrono::duration_cast<std::chrono::seconds>(ttl);
  alignas(4) bool blocked_out;
  ERL_TRY(fastly::check_rate(
      rate_counter_.name().data(), rate_counter_.name().size(), entry.data(),
      entry.size(), delta, static_cast<std::uint32_t>(window), limit,
      penalty_box_.name().data(), penalty_box_.name().size(),
      static_cast<std::uint32_t>(ttl_seconds.count()), &blocked_out));
  return blocked_out;
}
} // namespace fastly::erl