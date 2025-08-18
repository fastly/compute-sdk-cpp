#include "util.h"
#include <fastly/error.h>
#include <fastly/sdk-sys.h>
#include <fastly/secret_store.h>

namespace fastly::secret_store {

fastly::expected<Secret> Secret::from_bytes(std::vector<uint8_t> data) {
  fastly::sys::secret_store::Secret *out;
  fastly::sys::error::FastlyError *err;
  fastly::sys::secret_store::m_static_secret_store_secret_from_bytes(data, out,
                                                                     err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return FSLY_BOX(secret_store, Secret, out);
  }
}

std::string Secret::plaintext() {
  std::string out;
  this->s->plaintext(out);
  return out;
}

fastly::expected<SecretStore> SecretStore::open(std::string_view name) {
  fastly::sys::secret_store::SecretStore *out;
  fastly::sys::error::FastlyError *err;
  fastly::sys::secret_store::m_static_secret_store_secret_store_open(
      static_cast<std::string>(name), out, err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return FSLY_BOX(secret_store, SecretStore, out);
  }
}

fastly::expected<std::optional<Secret>> SecretStore::get(std::string_view key) {
  fastly::sys::secret_store::Secret *out;
  fastly::sys::error::FastlyError *err;
  this->ss->get(static_cast<std::string>(key), out, err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else if (out != nullptr) {
    return std::optional<Secret>(FSLY_BOX(secret_store, Secret, out));
  } else {
    return std::nullopt;
  }
}

fastly::expected<bool> SecretStore::contains(std::string_view key) {
  fastly::sys::error::FastlyError *err;
  bool out{this->ss->contains(static_cast<std::string>(key), err)};
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<bool>(out);
  }
}

} // namespace fastly::secret_store
