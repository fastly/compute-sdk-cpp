#include "fastly/sdk.h"
#include <iostream>
#include <optional>
#include <string>

using namespace std::string_literals;

int main() {
  auto req{fastly::Request::from_client()};
  fastly::Body body;
  auto ua{req.get_header("User-Agent")};
  if (!ua.has_value()) {
    body << "Failed to get User-Agent header: " << ua.error().error_msg()
         << std::endl;
  } else if (ua == std::nullopt) {
    body << "No user agent. Can't detect device." << std::endl;
  } else {
    body << "Trying to detect device using UA `" << **ua << "`..." << std::endl;
    auto maybe_dev{fastly::device_detection::lookup(**ua)};
    if (!maybe_dev.has_value()) {
      body << "Error while looking up device: " << maybe_dev.error().error_msg()
           << std::endl;
    } else if (*maybe_dev == std::nullopt) {
      body << "Failed to detect device based on User Agent string."
           << std::endl;
    } else {
      auto dev{std::move(**maybe_dev)};
      body << "Device name: " << dev.device_name().value_or("UNKNOWN")
           << std::endl
           << "Brand: " << dev.brand().value_or("UNKNOWN") << std::endl
           << "Model: " << dev.model().value_or("UNKNOWN") << std::endl
           << "Hardware Type: " << dev.hwtype().value_or("UNKNOWN") << std::endl
           << "eReader?: " << dev.is_ereader().value_or("UNKNOWN") << std::endl
           << "Game console?: " << dev.is_gameconsole().value_or("UNKNOWN")
           << std::endl
           << "Media player?: " << dev.is_mediaplayer().value_or("UNKNOWN")
           << std::endl
           << "Mobile?: " << dev.is_mobile().value_or("UNKNOWN") << std::endl
           << "martTV?: " << dev.is_smarttv().value_or("UNKNOWN") << std::endl
           << "Tablet?: " << dev.is_tablet().value_or("UNKNOWN") << std::endl
           << "TV Player?: " << dev.is_tvplayer().value_or("UNKNOWN")
           << std::endl
           << "Desktop?: " << dev.is_desktop().value_or("UNKNOWN") << std::endl
           << "Has Touchsreen?: " << dev.is_touchscreen().value_or("UNKNOWN")
           << std::endl;
    }
  }
  fastly::Response::from_body(std::move(body)).send_to_client();
}
