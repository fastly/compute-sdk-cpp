#ifndef FASTLY_KV_STORE_H
#define FASTLY_KV_STORE_H
#include <chrono>
#include <fastly/detail/rust_iterator_range.h>
#include <fastly/error.h>
#include <fastly/http/body.h>
#include <fastly/sdk-sys.h>
#include <functional>
#include <optional>

namespace fastly::kv_store {
using fastly::sys::kv_store::KVStoreErrorCode;
class KVStoreError {
public:
  KVStoreError(fastly::sys::kv_store::KVStoreError *e)
      : err_(rust::Box<fastly::sys::kv_store::KVStoreError>::from_raw(e)) {};
  KVStoreError(rust::Box<fastly::sys::kv_store::KVStoreError> e)
      : err_(std::move(e)) {};
  KVStoreErrorCode error_code();
  std::string error_msg();

private:
  rust::Box<fastly::sys::kv_store::KVStoreError> err_;
};
template <class T = void> using expected = tl::expected<T, KVStoreError>;
template <class T = void> using unexpected = tl::unexpected<T>;

namespace detail {
template <class HandleType> class KVPendingHandle {
public:
  /// Get the underlying representation of the handle.
  /// This should only be used when calling the raw ABI directly,
  /// and care should be taken not to reuse or alias handle values.
  std::uint32_t as_u32() const { return handle_; }

  /// Make a handle from its underlying representation.
  /// This should only be used when calling the raw ABI directly,
  /// and care should be taken not to reuse or alias handle values.
  static HandleType from_u32(std::uint32_t handle) {
    return HandleType(handle);
  }

protected:
  explicit KVPendingHandle(std::uint32_t handle) : handle_(handle) {}
  std::uint32_t handle_;
};
} // namespace detail

/// A handle to a pending asynchronous insert returned by
/// `InsertBuilder::execute_async()`.
///
/// A handle can be evaluated using `KVStore::pending_insert_wait()`. It can
/// also be discarded if the request was sent for effects it might have, and the
/// response is unimportant.
struct PendingInsertHandle
    : public detail::KVPendingHandle<PendingInsertHandle> {
  using detail::KVPendingHandle<PendingInsertHandle>::KVPendingHandle;
};

/// A handle to a pending asynchronous lookup returned by
/// `LookupBuilder::execute_async()`.
///
/// A handle can be evaluated using `KVStore::pending_lookup_wait()`.
struct PendingLookupHandle
    : public detail::KVPendingHandle<PendingLookupHandle> {
  using detail::KVPendingHandle<PendingLookupHandle>::KVPendingHandle;
};

/// A handle to a pending asynchronous erase returned by
/// `EraseBuilder::execute_async()`.
///
/// A handle can be evaluated using KVStore::pending_erase_wait(). It can also
/// be discarded if the request was sent for effects it might have, and the
/// response is unimportant.
struct PendingEraseHandle : public detail::KVPendingHandle<PendingEraseHandle> {
  using detail::KVPendingHandle<PendingEraseHandle>::KVPendingHandle;
};

/// A handle to a pending asynchronous list returned by
/// `ListBuilder::execute_async()`.
///
/// A handle can be evaluated using `KVStore::pending_list_wait()`.
struct PendingListHandle : public detail::KVPendingHandle<PendingListHandle> {
  using detail::KVPendingHandle<PendingListHandle>::KVPendingHandle;
};

class LookupBuilder;
class LookupResponse {
  friend LookupBuilder;

public:
  /// Returns the body, making the `LookupResponse` bodyless.
  Body take_body();

  /// Returns the body if it exists, making the `LookupResponse` bodyless.
  /// Otherwise returns `std::nullopt`.
  std::optional<Body> try_take_body();

  /// Converts the body into a byte vector, making the `LookupResponse`
  /// bodyless.
  std::vector<uint8_t> take_body_bytes();

  /// Reads the metadata of the `KVStore` item.
  std::optional<std::vector<std::uint8_t>> metadata() const;

  /// Reads the generation of the `KVStore` item.
  std::uint64_t current_generation() const;

private:
  friend class KVStore;
  LookupResponse(rust::Box<fastly::sys::kv_store::LookupResponse> response)
      : response_(std::move(response)) {}
  rust::Box<fastly::sys::kv_store::LookupResponse> response_;
};

class LookupBuilder {
public:
  /// Execute the lookup and wait for the response.
  expected<LookupResponse> execute(std::string_view key) const;

  /// Execute the lookup asynchronously.
  expected<PendingLookupHandle> execute_async(std::string_view key) const;

private:
  friend class KVStore;
  LookupBuilder(rust::Box<fastly::sys::kv_store::LookupBuilder> builder)
      : builder_(std::move(builder)) {}
  rust::Box<fastly::sys::kv_store::LookupBuilder> builder_;
};

class EraseBuilder {
public:
  /// Execute the erasure and wait for the response.
  expected<> execute(std::string_view key) const;

  /// Execute the erasure asynchronously.
  expected<PendingEraseHandle> execute_async(std::string_view key) const;

private:
  friend class KVStore;
  EraseBuilder(rust::Box<fastly::sys::kv_store::EraseBuilder> builder)
      : builder_(std::move(builder)) {}
  rust::Box<fastly::sys::kv_store::EraseBuilder> builder_;
};

using InsertMode = fastly::sys::kv_store::InsertMode;

class KVStore;
class InsertBuilder {
public:
  /// Change the behavior in the case when the new key matches an existing key.
  InsertBuilder mode(InsertMode mode) &&;

  /// If set, allows fetching from the origin to occur in the background,
  /// enabling a faster response with stale content. The cache will be updated
  /// with fresh content after the request is completed.
  InsertBuilder background_fetch() &&;

  /// Requests for keys will return a ‘generation’ header specific to the
  /// version of a key. The generation header is a unique, non-serial 64-bit
  /// unsigned integer that can be used for testing against a specific KV store
  /// value.
  InsertBuilder if_generation_match(std::uint64_t gen) &&;

  /// Sets an arbitrary data field which can contain up to 2000B of data.
  InsertBuilder metadata(const std::string &data) &&;

  /// Sets a time for the key to expire. Deletion will take place up to 24 hours
  /// after the ttl reaches 0.
  InsertBuilder time_to_live(std::chrono::milliseconds ttl) &&;

  /// Initiate and wait on an insert of a value into the KV Store.
  /// The return object is engaged if the value was inserted, and KVStoreError
  /// if the insert failed.
  expected<> execute(const std::string &key, Body body) &&;

  /// Initiate async insert of a value into the KV Store.
  expected<PendingInsertHandle> execute_async(const std::string &key,
                                              Body body) &&;

private:
  friend class KVStore;
  InsertBuilder(rust::Box<fastly::sys::kv_store::InsertBuilder> builder)
      : builder_(std::move(builder)) {}
  rust::Box<fastly::sys::kv_store::InsertBuilder> builder_;
};

struct ListModeStrong {};
struct ListModeEventual {};
struct ListModeOther {
  std::string other;
};
using ListMode = std::variant<ListModeStrong, ListModeEventual, ListModeOther>;

class ListResponse;
class KVStore;
class ListPage {
  friend ListResponse;
  friend KVStore;

public:
  /// Returns a vector of the listed keys in the current page.
  std::vector<std::string> keys() const;
  /// Returns a vector of the listed keys in the current page, consuming the
  /// page.
  std::vector<std::string> into_keys();

  /// Returns the next cursor of the List operation.
  std::optional<std::string> next_cursor() const;

  /// Returns the prefix used in the List operation.
  std::optional<std::string> prefix() const;

  /// Returns the limit used in the List operation.
  std::uint32_t limit() const;

  /// Returns the mode used in the List operation.
  ListMode mode() const;

private:
  friend class ListBuilder;
  ListPage(rust::Box<fastly::sys::kv_store::ListPage> page)
      : page_(std::move(page)) {}
  rust::Box<fastly::sys::kv_store::ListPage> page_;
};

class ListResponse : public fastly::detail::RustIteratorRange<
                         ListResponse, fastly::sys::kv_store::ListResponse> {
public:
  using fastly::detail::RustIteratorRange<
      ListResponse, fastly::sys::kv_store::ListResponse>::RustIteratorRange;

  /// Gets the next page of results.
  std::optional<expected<ListPage>> next() {
    fastly::sys::kv_store::ListPage *page;
    fastly::sys::kv_store::KVStoreError *err;
    if (this->iter_->next(page, err)) {
      if (err == nullptr) {
        return ListPage(
            rust::Box<fastly::sys::kv_store::ListPage>::from_raw(page));
      } else {
        return unexpected(KVStoreError(
            rust::Box<fastly::sys::kv_store::KVStoreError>::from_raw(err)));
      }
    } else {
      return std::nullopt;
    }
  }
};

class ListBuilder {
public:
  /// Rather than read data from the primary data source, which is slower
  /// but strongly consistent (strong, the default), instead read a local copy
  /// if available, which offers higher speed and may be a few seconds out of
  /// date (eventual).
  ListBuilder eventual_consistency() &&;

  /// Change the cursor of the request.
  ListBuilder cursor(const std::string &cursor) &&;

  /// Set the maximum number of items included in the response.
  ListBuilder limit(std::uint32_t limit) &&;

  /// Set the prefix match for items to include in the resultset.
  ListBuilder prefix(const std::string &prefix) &&;

  /// Initiate and wait on a list of values in the KV Store.
  expected<ListPage> execute() &&;

  /// Produce a new `ListResponse`, which can be used as an iterator for
  /// chunked list operations.
  ListResponse iter() &&;

  /// Initiate async list of values in the KV Store.
  expected<PendingListHandle> execute_async() const;

private:
  friend class KVStore;
  ListBuilder(rust::Box<fastly::sys::kv_store::ListBuilder> builder)
      : builder_(std::move(builder)) {}
  rust::Box<fastly::sys::kv_store::ListBuilder> builder_;
};

class KVStore {
public:
  /// Opens a key-value store with the given name.
  ///
  /// Returns an error if the store cannot be opened.
  static expected<std::optional<KVStore>> open(std::string_view name);
  expected<LookupResponse> lookup(std::string_view key) const;
  LookupBuilder build_lookup() const;

  /// Look up a value in the KV Store.
  expected<LookupResponse>
  pending_lookup_wait(PendingLookupHandle pending_request_handle) const;

  /// Insert a value into the KV Store.
  ///
  /// If the store already contained a value for this key, it will be
  /// overwritten. Returns an error if the value cannot be inserted.
  expected<> insert(std::string_view key, Body value) const;

  /// Create a buildable insert request for the KV Store.
  /// When the desired configuration has been set, execute the insert and
  /// wait for the response.
  InsertBuilder build_insert() const;

  /// Insert a value in the KV Store.
  expected<>
  pending_insert_wait(PendingInsertHandle pending_insert_handle) const;

  /// Erase a value in the KV Store.
  expected<> erase(std::string_view key) const;

  /// Create a buildable erase request for the KV Store.
  /// When the desired configuration has been set, execute the erase and
  /// wait for the response.
  EraseBuilder build_erase() const;

  /// Erase a value in the KV Store.
  expected<> pending_erase_wait(PendingEraseHandle pending_erase_handle) const;

  /// List keys in the KV Store.
  expected<ListPage> list() const;

  /// Create a buildable list request for the KV Store.
  /// When the desired configuration has been set, execute the list and wait
  /// for the response.
  ListBuilder build_list() const;

  /// Wait on a pending list operation.
  expected<ListPage>
  pending_list_wait(PendingListHandle pending_request_handle) const;

private:
  /// Create a new KVStore from the underlying Rust type.
  explicit KVStore(rust::Box<fastly::sys::kv_store::KVStore> store)
      : store_(std::move(store)) {}

  rust::Box<fastly::sys::kv_store::KVStore> store_;
};
} // namespace fastly::kv_store
#endif