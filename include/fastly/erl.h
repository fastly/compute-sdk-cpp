#ifndef FASTLY_ERL_H
#define FASTLY_ERL_H

#include <chrono>
#include <fastly/expected.h>

namespace fastly::erl {
/// Errors that can arise during ERL operations.
class ERLError {
public:
  enum Code {
    Unexpected,
    Unknown,
    InvalidArgument,
  };
  explicit ERLError(Code code) : code_(code) {}
  Code code() const { return code_; }

private:
  Code code_;
};

/// A penalty box that can be used with the edge rate limiter or stand alone for
/// adding and checking if some entry is in the data set.
class PenaltyBox {
public:
  explicit PenaltyBox(std::string name) : name_(std::move(name)) {};
  /// Add entry to a the penaltybox for the duration of ttl. Valid ttl span is
  /// 1m to 1h.
  tl::expected<void, ERLError> add(std::string_view entry,
                                   std::chrono::minutes ttl);
  /// Check if entry is in the penaltybox.
  tl::expected<bool, ERLError> has(std::string_view entry) const;
  std::string_view name() const { return name_; }

private:
  std::string name_;
};

/// To be used for picking the duration in a rate counter `lookup_count` call
enum class CounterDuration {
  TenSec = 10,
  TwentySecs = 20,
  ThirtySecs = 30,
  FortySecs = 40,
  FiftySecs = 50,
  SixtySecs = 60
};

/// To be used for picking the window in a rate counter `lookup_rate` or a ERL
/// `check_rate` call.
enum class RateWindow {
  OneSec = 1,
  TenSecs = 10,
  SixtySecs = 60,
};

/// A rate counter that can be used with an edge rate limiter or stand alone for
/// counting and rate calculations
class RateCounter {
public:
  explicit RateCounter(std::string name) : name_(std::move(name)) {}
  /// Increment an entry in the ratecounter by delta.
  tl::expected<void, ERLError> increment(std::string_view entry,
                                         std::uint32_t delta);
  /// Lookup the current rate for entry in the rate counter for a window.
  tl::expected<std::uint32_t, ERLError> lookup_rate(std::string_view entry,
                                                    RateWindow window) const;
  /// Lookup the current count for entry in the rate counter for a duration.
  tl::expected<std::uint32_t, ERLError> lookup_count(std::string_view entry,
                                                     CounterDuration duration) const;
  std::string_view name() const { return name_; }

private:
  std::string name_;
};

class ERL {
public:
  ERL(RateCounter rate_counter, PenaltyBox penalty_box)
      : rate_counter_(std::move(rate_counter)),
        penalty_box_(std::move(penalty_box)) {}

  /// Increment an entry in a rate counter and check if the client has exceeded
  /// some average number
  /// of requests per second (RPS) over the window. If the client is over the rps
  /// limit for the window, add to the penaltybox for ttl. Valid ttl span is 1m
  /// to 1h.
  tl::expected<bool, ERLError>
  check_rate(std::string_view entry, std::uint32_t delta, RateWindow window,
             std::uint32_t limit, std::chrono::minutes ttl) const;

  const RateCounter &rate_counter() const { return rate_counter_; }
  const PenaltyBox &penalty_box() const { return penalty_box_; }

private:
  RateCounter rate_counter_;
  PenaltyBox penalty_box_;
};
} // namespace fastly::erl
#endif