#include "device_detection.h"
#include "sdk-sys.h"

namespace fastly::device_detection {

fastly::expected<std::optional<Device>> lookup(std::string_view user_agent) {
  fastly::sys::device_detection::Device *out;
  fastly::sys::error::FastlyError *err;
  fastly::sys::device_detection::f_device_detection_lookup(
      static_cast<std::string>(user_agent), out, err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else if (out != nullptr) {
    return std::optional<Device>(FSLY_BOX(device_detection, Device, out));
  } else {
    return std::nullopt;
  }
}

std::optional<std::string> Device::device_name() {
  std::string out;
  auto some{this->dev->device_name(out)};
  if (!some) {
    return std::nullopt;
  } else {
    return std::optional<std::string>{out};
  }
}

std::optional<std::string> Device::brand() {
  std::string out;
  auto some{this->dev->brand(out)};
  if (!some) {
    return std::nullopt;
  } else {
    return std::optional<std::string>{out};
  }
}

std::optional<std::string> Device::model() {
  std::string out;
  auto some{this->dev->model(out)};
  if (!some) {
    return std::nullopt;
  } else {
    return std::optional<std::string>{out};
  }
}

std::optional<std::string> Device::hwtype() {
  std::string out;
  auto some{this->dev->hwtype(out)};
  if (!some) {
    return std::nullopt;
  } else {
    return std::optional<std::string>{out};
  }
}

std::optional<bool> Device::is_ereader() {
  auto ptr{this->dev->is_ereader()};
  if (ptr == nullptr) {
    return std::nullopt;
  } else {
    return std::optional<bool>(*ptr);
  }
}

std::optional<bool> Device::is_gameconsole() {
  auto ptr{this->dev->is_gameconsole()};
  if (ptr == nullptr) {
    return std::nullopt;
  } else {
    return std::optional<bool>(*ptr);
  }
}

std::optional<bool> Device::is_mediaplayer() {
  auto ptr{this->dev->is_mediaplayer()};
  if (ptr == nullptr) {
    return std::nullopt;
  } else {
    return std::optional<bool>(*ptr);
  }
}

std::optional<bool> Device::is_mobile() {
  auto ptr{this->dev->is_mobile()};
  if (ptr == nullptr) {
    return std::nullopt;
  } else {
    return std::optional<bool>(*ptr);
  }
}

std::optional<bool> Device::is_smarttv() {
  auto ptr{this->dev->is_smarttv()};
  if (ptr == nullptr) {
    return std::nullopt;
  } else {
    return std::optional<bool>(*ptr);
  }
}

std::optional<bool> Device::is_tablet() {
  auto ptr{this->dev->is_tablet()};
  if (ptr == nullptr) {
    return std::nullopt;
  } else {
    return std::optional<bool>(*ptr);
  }
}

std::optional<bool> Device::is_tvplayer() {
  auto ptr{this->dev->is_tvplayer()};
  if (ptr == nullptr) {
    return std::nullopt;
  } else {
    return std::optional<bool>(*ptr);
  }
}

std::optional<bool> Device::is_desktop() {
  auto ptr{this->dev->is_desktop()};
  if (ptr == nullptr) {
    return std::nullopt;
  } else {
    return std::optional<bool>(*ptr);
  }
}

std::optional<bool> Device::is_touchscreen() {
  auto ptr{this->dev->is_touchscreen()};
  if (ptr == nullptr) {
    return std::nullopt;
  } else {
    return std::optional<bool>(*ptr);
  }
}

} // namespace fastly::device_detection
