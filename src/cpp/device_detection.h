#ifndef FASTLY_DEVICE_DETECTION_H
#define FASTLY_DEVICE_DETECTION_H

#include "error.h"
#include "sdk-sys.h"
#include "util.h"
#include <optional>
#include <string>
#include <string_view>

namespace fastly::device_detection {

/// The device data associated with a particular User-Agent string.
class Device {
  friend fastly::expected<std::optional<Device>>
  lookup(std::string_view user_agent);

public:
  /// The name of the client device.
  std::optional<std::string> device_name();

  /// The brand of the client device, possibly different from the
  /// manufacturer of that device.
  std::optional<std::string> brand();

  /// The model of the client device.
  std::optional<std::string> model();

  /// A string representation of the primary client platform hardware.
  /// The most commonly used device types are also identified via
  /// boolean variables. Because a device may have multiple device
  /// types and this variable only has the primary type, we recommend
  /// using the boolean variables for logic and using this string
  /// representation for logging.
  std::optional<std::string> hwtype();

  /// The client device is a reading device (like a Kindle).
  std::optional<bool> is_ereader();

  /// The client device is a video game console (like a PlayStation or Xbox).
  std::optional<bool> is_gameconsole();

  /// The client device is a media player (like Blu-ray players, iPod
  /// devices, and smart speakers such as Amazon Echo).
  std::optional<bool> is_mediaplayer();

  /// The client device is a mobile phone.
  std::optional<bool> is_mobile();

  /// The client device is a smart TV.
  std::optional<bool> is_smarttv();

  /// The client device is a tablet (like an iPad).
  std::optional<bool> is_tablet();

  /// The client device is a set-top box or other TV player (like a Roku or
  /// Apple TV).
  std::optional<bool> is_tvplayer();

  /// The client is a desktop web browser.
  std::optional<bool> is_desktop();

  /// The client device's screen is touch sensitive.
  std::optional<bool> is_touchscreen();

private:
  rust::Box<fastly::sys::device_detection::Device> dev;
  Device(rust::Box<fastly::sys::device_detection::Device> d)
      : dev(std::move(d)) {};
};

/// Look up the data associated with a particular User-Agent string.
fastly::expected<std::optional<Device>> lookup(std::string_view user_agent);

} // namespace fastly::device_detection

#endif
