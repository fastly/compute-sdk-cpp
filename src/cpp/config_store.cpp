#include "util.h"
#include <fastly/config_store.h>
#include <fastly/sdk-sys.h>

namespace fastly::config_store {

fastly::expected<ConfigStore> ConfigStore::open(std::string_view name) {
  fastly::sys::config_store::ConfigStore *out;
  fastly::sys::error::FastlyError *err;
  fastly::sys::config_store::m_static_config_store_config_store_open(
      static_cast<std::string>(name), out, err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return FSLY_BOX(config_store, ConfigStore, out);
  }
}

fastly::expected<std::optional<std::string>>
ConfigStore::get(std::string_view key) {
  std::string out;
  fastly::sys::error::FastlyError *err;
  auto some{this->cs->get(static_cast<std::string>(key), out, err)};
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else if (!some) {
    return std::nullopt;
  } else {
    return std::optional<std::string>(std::move(out));
  }
}

fastly::expected<bool> ConfigStore::contains(std::string_view key) {
  fastly::sys::error::FastlyError *err;
  bool out{this->cs->contains(static_cast<std::string>(key), err)};
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<bool>(out);
  }
}

} // namespace fastly::config_store
