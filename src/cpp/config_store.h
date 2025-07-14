#ifndef FASTLY_CONFIG_STORE_H
#define FASTLY_CONFIG_STORE_H

#include "error.h"
#include "sdk-sys.h"
#include "util.h"
#include <optional>
#include <string>
#include <string_view>

namespace fastly::config_store {

/// A Compute Config Store.
class ConfigStore {
public:
  /// Open a config store, given its name.
  ///
  /// # Examples
  ///
  /// ```no_run
  /// auto merriam{fastly::ConfigStore::open("merriam webster")};
  /// auto oed{fastly::ConfigStore::open("oxford english config store")};
  /// ```
  static fastly::expected<ConfigStore> open(std::string_view name);

  /// Lookup a value in this config store.
  ///
  /// If successful, this function returns `std::optional<std::string>>` if an
  /// entry was found, or `std::nullopt` if no entry with the given key was
  /// found.
  ///
  /// # Examples
  ///
  /// ```no_run
  /// auto store{fastly::ConfigStore::open("test config store")};
  /// assert(store.get("bread").value() ==
  ///        "a usually baked and leavened food");
  ///
  /// assert(
  ///     store.get("freedom").value() ==
  ///     "the absence of necessity, coercion, or constraint",
  /// );
  ///
  /// // Otherwise, `get` will return nullopt.
  /// assert(store.get("zzzzz") == std::nullopt);
  /// ```
  fastly::expected<std::optional<std::string>> get(std::string_view key);

  /// Return true if the config_store contains an entry with the given key.
  ///
  /// # Examples
  ///
  /// ```no_run
  /// auto store{fastly::ConfigStore::open("test config store")};
  /// assert(store.contains("key").value());
  /// ```
  fastly::expected<bool> contains(std::string_view key);

private:
  rust::Box<fastly::sys::config_store::ConfigStore> cs;
  ConfigStore(rust::Box<fastly::sys::config_store::ConfigStore> c)
      : cs(std::move(c)) {};
};

} // namespace fastly::config_store

namespace fastly {
using fastly::config_store::ConfigStore;
}

#endif
