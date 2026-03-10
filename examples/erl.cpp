//! @example erl.cpp
#include "fastly/sdk.h"

// Note that this example will return "Welcome!" unconditionally on Viceroy; to
// see the expected behavior, you need to run it on a Compute service.

int main() {
  auto req{fastly::Request::from_client()};
  auto client = req.get_client_ip_addr().value();

  fastly::erl::RateCounter rate_counter("mycounter");
  fastly::erl::PenaltyBox penalty_box("mypenaltybox");

  auto erl = fastly::erl::ERL(rate_counter, penalty_box);
  auto check_result = erl.check_rate(
      client, // Use the client IP address as the entry to check in the rate
              // counter
      1,      // How many requests to count this as
      fastly::erl::RateWindow::SixtySecs, // The window to check the rate over
      5, // The maximum allowed rate for the client over the window
      std::chrono::minutes(
          1)); // The duration to penalize the client if they exceed the rate
  if (check_result.has_value() && *check_result) {
    fastly::Response::from_body("You are blocked!").send_to_client();
  } else {
    std::string message = "Welcome! Your current rate is ";
    message += std::to_string(
        rate_counter.lookup_rate(client, fastly::erl::RateWindow::SixtySecs)
            .value());
    fastly::Response::from_body(message).send_to_client();
  }
}