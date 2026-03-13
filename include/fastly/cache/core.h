#ifndef FASTLY_CACHE_CORE_H
#define FASTLY_CACHE_CORE_H

#include <chrono>
#include <cstdint>
#include <fastly/expected.h>
#include <fastly/http/body.h>
#include <fastly/http/header.h>
#include <fastly/http/request.h>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace fastly::cache::core {

/// Cache key is a byte array used to identify cached items.
using CacheKey = std::vector<std::uint8_t>;

/// Errors that can arise during cache operations.
class CacheError {
public:
  enum Code {
    /// Operation failed due to a limit.
    LimitExceeded,
    /// Operation was not valid to be performed given the state of the cached
    /// item.
    InvalidOperation,
    /// Cache operation is not supported.
    Unsupported,
    /// An unknown error occurred.
    Unknown,
  };
  CacheError(Code code) : code_(code) {}
  Code code() const { return code_; }

private:
  Code code_;
};

namespace detail {
class CacheHandle {
public:
  explicit CacheHandle(std::uint32_t handle) : handle_(handle) {}
  std::uint32_t handle() const { return handle_; }
  ~CacheHandle();

private:
  std::uint32_t handle_;
};

class CacheReplaceHandle {
public:
  explicit CacheReplaceHandle(std::uint32_t handle) : handle_(handle) {}
  std::uint32_t handle() const { return handle_; }
  ~CacheReplaceHandle();

private:
  std::uint32_t handle_;
};

class CacheBusyHandle {
public:
  explicit CacheBusyHandle(std::uint32_t handle) : handle_(handle) {}
  std::uint32_t handle() const { return handle_; }
  ~CacheBusyHandle();

private:
  std::uint32_t handle_;
};

// TODO: this should really live somewhere else, but for now it's the only
// place that we need it, so we'll take an ad-hoc approach.
class RequestHandle {
public:
  static RequestHandle make();
  explicit RequestHandle(std::uint32_t handle) : handle_(handle) {}
  std::uint32_t handle() const { return handle_; }
  ~RequestHandle();

private:
  std::uint32_t handle_;
};
} // namespace detail

/// Options for cache lookup operations.
struct LookupOptions {
  std::optional<detail::RequestHandle> request_headers;
  std::optional<std::string> service;
  bool always_use_requested_range = false;
};

/// Strategy to use when replacing an existing cached object.
enum class ReplaceStrategy {
  /// Immediately start the replace and do not wait for any other pending
  /// requests for the same object, including insert requests.
  ///
  /// With this strategy a replace will race all other pending requests to
  /// update the object.
  ///
  /// The existing object will be accessible until this replace finishes
  /// providing the replacement object.
  ///
  /// This is the default replace strategy.
  Immediate,
  /// Immediate, but remove the existing object immediately
  ///
  /// Requests for the same object that arrive after this replace starts will
  /// wait until this replace starts providing the replacement object.
  ImmediateForceMiss,
  /// Join the wait list behind other pending requests before starting this
  /// request.
  ///
  /// With this strategy this replace request will wait for an in-progress
  /// replace or insert request before starting.
  ///
  /// This strategy allows implementing a counter, but may cause timeouts if
  /// too many requests are waiting for in-progress and waiting updates to
  /// complete.
  Wait,
};

/// Options for cache replace operations.
struct ReplaceOptions {
  std::optional<detail::RequestHandle> request_headers;
  ReplaceStrategy replace_strategy = ReplaceStrategy::Immediate;
  std::optional<std::string> service;
  bool always_use_requested_range = false;
};

/// Options for cache write operations (insert/update).
struct WriteOptions {
  std::chrono::nanoseconds max_age;
  std::optional<detail::RequestHandle> request_headers;
  std::optional<std::string> vary_rule;
  std::optional<std::chrono::nanoseconds> initial_age;
  std::optional<std::chrono::nanoseconds> stale_while_revalidate;
  std::optional<std::string> surrogate_keys;
  std::optional<std::uint64_t> length;
  std::optional<std::vector<std::uint8_t>> user_metadata;
  bool sensitive_data = false;
  std::optional<std::chrono::nanoseconds> edge_max_age;
  std::optional<std::string> service;
  explicit WriteOptions(std::chrono::nanoseconds max_age) : max_age(max_age) {}
};

/// A builder-style API for configuring a transactional cache update.
///
/// This builder is used to update an existing cached item's metadata, such as
/// its age, without changing the object itself. All builder methods consume the
/// builder and return it for method chaining.
class TransactionUpdateBuilder {
public:
  /// Sets the list of headers that must match when looking up this cached item.
  ///
  /// The header values must be provided by calling
  /// `TransactionLookupBuilder::header()` or
  /// `TransactionLookupBuilder::header_values()`. The header values may be a
  /// subset or superset of the header names supplied here.
  ///
  /// Note: These headers are narrowly useful for implementing cache lookups
  /// incorporating the semantics of the HTTP Vary header, but the APIs in this
  /// module are not suitable for HTTP caching out-of-the-box. Future SDK
  /// releases will contain an HTTP Cache API.
  ///
  /// The headers act as additional factors in object selection, and the choice
  /// of which headers to factor in is determined during insertion, via e.g.
  /// `crate::cache::core::InsertBuilder::vary_by`. A lookup will succeed when
  /// there is at least one cached item that matches lookup’s cache key, and all
  /// of the lookup’s headers included in the cache items’ vary_by list match
  /// the corresponding headers in that cached item.
  ///
  /// A typical example is a cached HTTP response, where the request had an
  /// `Accept-Encoding` header. In that case, the origin server may or may not
  /// decide on a given encoding, and whether that same response is suitable for
  /// a request with a different (or missing) `Accept-Encoding` header is
  /// determined by whether `Accept-Encoding` is listed in `Vary` header in the
  /// origin’s response.
  TransactionUpdateBuilder vary_by(std::vector<std::string> headers) &&;

  /// Sets the updated age of the cached item, to be used in freshness
  /// calculations.
  ///
  /// The updated age is 0 by default.
  TransactionUpdateBuilder age(std::chrono::nanoseconds age) &&;

  /// Sets the maximum time the cached item may live on a deliver node in a POP.
  TransactionUpdateBuilder
  deliver_node_max_age(std::chrono::nanoseconds duration) &&;

  /// Sets the stale-while-revalidate period for the cached item, which is the
  /// time for which the item can be safely used despite being considered stale.
  ///
  /// Having a stale-while-revalidate period provides a signal that the cache
  /// should be updated (or its contents otherwise revalidated for freshness)
  /// asynchronously, while the stale cached item continues to be used, rather
  /// than blocking on updating the cached item. The methods Found::is_usable
  /// and Found::is_stale can be used to determine the current state of a found
  /// item.
  ///
  /// The stale-while-revalidate period is 0 by default.
  TransactionUpdateBuilder
  stale_while_revalidate(std::chrono::nanoseconds duration) &&;

  /// Sets the surrogate keys that can be used for purging this cached item.
  ///
  /// Surrogate key purges are the only means to purge specific items from the
  /// cache. At least one surrogate key must be set in order to remove an item
  /// without performing a
  /// [purge-all](https://www.fastly.com/documentation/guides/concepts/cache/purging/#purge-all),
  /// waiting for the item’s TTL to elapse, or overwriting the item with
  /// `crate::cache::core::insert()`.
  ///
  /// Surrogate keys must contain only printable ASCII characters (those between
  /// 0x21 and 0x7E, inclusive). Any invalid keys will be ignored.
  ///
  /// See the [Fastly surrogate keys
  /// guide](https://www.fastly.com/documentation/guides/full-site-delivery/purging/working-with-surrogate-keys/)
  /// for details.
  TransactionUpdateBuilder surrogate_keys(std::vector<std::string> keys) &&;

  /// Sets the user-defined metadata to associate with the cached item.
  TransactionUpdateBuilder user_metadata(std::vector<std::uint8_t> metadata) &&;

  /// Perform this update on behalf of another service, using its data store.
  ///
  /// *Internal / Privileged*
  /// This operation is privileged, and attempts to use this functionality
  /// without proper privileges will cause errors. If you are interested in
  /// having two or more of your services share the same cache, please talk to
  /// your Fastly account representative. While we have no plans to offer this
  /// ability widely – this capability is only currently allowed for
  /// Fastly-internal services – we may revisit this decision given sufficient
  /// customer input.
  TransactionUpdateBuilder on_behalf_of(std::string service) &&;

  /// Perform the update of the cache item's metadata.
  ///
  /// This consumes the builder and executes the update operation.
  tl::expected<void, CacheError> execute() &&;

private:
  TransactionUpdateBuilder(std::shared_ptr<detail::CacheHandle> handle,
                           WriteOptions options)
      : handle_(std::move(handle)), options_(std::move(options)) {}

  std::shared_ptr<detail::CacheHandle> handle_;
  WriteOptions options_;
  friend class Transaction;
};

/// A builder-style API for configuring a transactional cache insertion.
///
/// This builder is used to insert a new item into the cache during a
/// transaction. All builder methods consume the builder and return it for
/// method chaining.
class TransactionInsertBuilder {
public:
  /// Sets the list of headers that must match when looking up this cached item.
  ///
  /// The header values must be provided by calling
  /// `TransactionLookupBuilder::header()` or
  /// `TransactionLookupBuilder::header_values()`. The header values may be a
  /// subset or superset of the header names supplied here.
  TransactionInsertBuilder vary_by(std::vector<std::string> headers) &&;

  /// Sets the initial age of the cached item, to be used in freshness
  /// calculations. The initial age is 0 by default.
  TransactionInsertBuilder initial_age(std::chrono::nanoseconds age) &&;

  /// Sets the stale-while-revalidate period for the cached item, which is the
  /// time for which the item can be safely used despite being considered stale.
  ///
  /// Having a stale-while-revalidate period provides a signal that the cache
  /// should be updated (or its contents otherwise revalidated for freshness)
  /// asynchronously, while the stale cached item continues to be used, rather
  /// than blocking on updating the cached item. The methods `Found::is_usable`
  /// and `Found::is_stale` can be used to determine the current state of a
  /// found item.
  ///
  /// The stale-while-revalidate period is 0 by default.
  TransactionInsertBuilder
  stale_while_revalidate(std::chrono::nanoseconds duration) &&;

  /// Sets the surrogate keys that can be used for purging this cached item.
  ///
  /// Surrogate key purges are the only means to purge specific items from the
  /// cache. At least one surrogate key must be set in order to remove an item
  /// without performing a
  /// [purge-all](https://www.fastly.com/documentation/guides/concepts/cache/purging/#purge-all),
  /// waiting for the item’s TTL to elapse, or overwriting the item with
  /// `crate::cache::core::insert()`.
  ///
  /// Surrogate keys must contain only printable ASCII characters (those between
  /// 0x21 and 0x7E, inclusive). Any invalid keys will be ignored.
  ///
  /// See the [Fastly surrogate keys
  /// guide](https://www.fastly.com/documentation/guides/full-site-delivery/purging/working-with-surrogate-keys/)
  /// for details.
  TransactionInsertBuilder surrogate_keys(std::vector<std::string> keys) &&;

  /// Sets the size of the cached item, in bytes, when known prior to actually
  /// providing the bytes.
  ///
  /// It is preferable to provide a length, if possible. Clients that begin
  /// streaming the item’s contents before it is completely provided will see
  /// the promised length which allows them to, for example, use
  /// `content-length` instead of `transfer-encoding: chunked` if the item is
  /// used as the body of a `Request` or `Response`.
  TransactionInsertBuilder known_length(std::uint64_t length) &&;

  /// Sets the user-defined metadata to associate with the cached item.
  TransactionInsertBuilder user_metadata(std::vector<std::uint8_t> metadata) &&;

  /// Enable or disable PCI/HIPAA-compliant non-volatile caching.
  ///
  /// By default, this is false.
  ///
  /// See the [Fastly PCI-Compliant Caching and Delivery
  /// documentation](https://docs.fastly.com/products/pci-compliant-caching-and-delivery)
  /// for details.
  TransactionInsertBuilder sensitive_data(bool is_sensitive) &&;

  /// Sets the maximum time the cached item may live on a deliver node in a POP.
  TransactionInsertBuilder
  deliver_node_max_age(std::chrono::nanoseconds duration) &&;

  /// Perform this insert on behalf of another service, using its data store.
  ///
  /// *Internal / Privileged*
  /// This operation is privileged, and attempts to use this functionality
  /// without proper privileges will cause errors. If you are interested in
  /// having two or more of your services share the same cache, please talk to
  /// your Fastly account representative. While we have no plans to offer this
  /// ability widely – this capability is only currently allowed for
  /// Fastly-internal services – we may revisit this decision given sufficient
  /// customer input.
  TransactionInsertBuilder on_behalf_of(std::string service) &&;

  /// Begin the insertion, returning a `StreamingBody` for providing the cached
  /// object itself.
  ///
  /// For the insertion to complete successfully, the object must be written
  /// into the `StreamingBody`, and then `StreamingBody::finish` must be called.
  /// If the `StreamingBody` is dropped before calling `StreamingBody::finish`,
  /// the insertion is considered incomplete, and any concurrent lookups that
  /// may be reading from the object as it is streamed into the cache may
  /// encounter a streaming error.
  tl::expected<http::StreamingBody, CacheError> execute() &&;

  /// Begin the insertion, and provide a `Found` object that can be used to
  /// stream out of the newly-inserted object.
  ///
  /// For the insertion to complete successfully, the object must be written
  /// into the `StreamingBody`, and then `StreamingBody::finish` must be called.
  /// If the `StreamingBody` is dropped before calling `StreamingBody::finish`,
  /// the insertion is considered incomplete, and any concurrent lookups that
  /// may be reading from the object as it is streamed into the cache may
  /// encounter a streaming error.
  ///
  /// The returned `Found` object allows the client inserting a cache item to
  /// efficiently read back the contents of that item, avoiding the need to
  /// buffer contents for copying to multiple destinations. This pattern is
  /// commonly required when caching an item that also must be provided to,
  /// e.g., the client response.
  tl::expected<std::pair<http::StreamingBody, Found>, CacheError>
  execute_and_stream_back() &&;

private:
  TransactionInsertBuilder(std::shared_ptr<detail::CacheHandle> handle,
                           WriteOptions options)
      : handle_(std::move(handle)), options_(std::move(options)) {}

  std::shared_ptr<detail::CacheHandle> handle_;
  WriteOptions options_;
  friend class Transaction;
};

class Transaction;
class PendingTransaction;

/// A cache transaction suspended between lookup and response.
///
/// This handle is produced by `TransactionLookupBuilder::execute_async()`.
///
/// Callers can check whether or not the request was instructed to wait behind
/// another caller by calling `PendingTransaction::pending()`, and can complete
/// the request (returning a `Transaction`) by calling
/// `PendingTransaction::wait()`.
class PendingTransaction {
public:
  /// Returns `true` if this request was instructed to wait for another request
  /// to insert the looked-up item, and `false` otherwise.
  ///
  /// Note that, even if `pending()` returns `true`, the `Transaction` returned
  /// by `wait()` may still be required to insert (or update) the object. This
  /// can happen if the caller providing the item called
  /// `Transaction::cancel_insert_or_update()` or encountered an error.
  tl::expected<bool, CacheError> pending() const;

  /// Returns the `Transaction` resulting from the lookup that produced this
  /// `PendingTransaction`, waiting for another caller to provide the requested
  /// item if necessary.
  tl::expected<Transaction, CacheError> wait() &&;

private:
  explicit PendingTransaction(std::shared_ptr<detail::CacheBusyHandle> handle)
      : handle_(std::move(handle)) {}

  std::shared_ptr<detail::CacheBusyHandle> handle_;
  friend class TransactionLookupBuilder;
};

/// A builder-style API for configuring a transactional cache lookup.
class TransactionLookupBuilder {
public:
  /// Sets a multi-value header for this lookup, discarding any previous values
  /// associated with the header name.
  ///
  /// Note: These headers are narrowly useful for implementing cache lookups
  /// incorporating the semantics of the [HTTP
  /// Vary](https://www.rfc-editor.org/rfc/rfc9110#section-12.5.5) header, but
  /// the APIs in this module are not suitable for HTTP caching out-of-the-box.
  /// Future SDK releases will contain an HTTP Cache API.
  ///
  /// The headers act as additional factors in object selection, and the choice
  /// of which headers to factor in is determined during insertion, via e.g.
  /// `crate::cache::core::InsertBuilder::vary_by`. A lookup will succeed when
  /// there is at least one cached item that matches lookup’s cache key, and all
  /// of the lookup’s headers included in the cache items’ vary_by list match
  /// the corresponding headers in that cached item.
  ///
  /// A typical example is a cached HTTP response, where the request had an
  /// Accept-Encoding header. In that case, the origin server may or may not
  /// decide on a given encoding, and whether that same response is suitable for
  /// a request with a different (or missing) Accept-Encoding header is
  /// determined by whether Accept-Encoding is listed in Vary header in the
  /// origin’s response.
  TransactionLookupBuilder
  header_values(std::string_view name,
                std::span<const http::HeaderValue> values) &&;

  /// Sets a single-value header for this lookup, discarding any previous values
  /// associated with the header `name`.

  /// Note: These headers are narrowly useful for implementing cache lookups
  /// incorporating the semantics of the [HTTP
  /// Vary](https://www.rfc-editor.org/rfc/rfc9110#section-12.5.5)
  /// header, but the APIs in this module are not suitable for HTTP
  /// caching out-of-the-box. Future SDK releases will contain an HTTP
  /// Cache API.
  ///
  /// The headers act as additional factors in object selection, and
  /// the choice of which headers to factor in is determined during
  /// insertion, via e.g.
  /// `crate::cache::core::InsertBuilder::vary_by`. A lookup will
  /// succeed when there is at least one cached item that matches
  /// lookup’s cache key, and all of the lookup’s headers included in
  /// the cache items’ vary_by list match the corresponding headers in
  /// that cached item.
  ///
  /// A typical example is a cached HTTP response, where the request
  /// had an Accept-Encoding header. In that case, the origin server
  /// may or may not decide on a given encoding, and whether that same
  /// response is suitable for a request with a different (or missing)
  /// Accept-Encoding header is determined by whether Accept-Encoding
  /// is listed in Vary header in the origin’s response.
  TransactionLookupBuilder header(std::string_view name,
                                  const http::HeaderValue &value) &&;

  /// Perform this lookup on behalf of another service, using its
  /// data store.
  ///
  /// *Internal / Privileged*
  /// This operation is privileged, and attempts to use this
  /// functionality without proper privileges will cause errors.
  /// If you are interested in having two or more of your services
  /// share the same cache, please talk to your Fastly account
  /// representative. While we have no plans to offer this ability
  /// widely – this capability is only currently allowed for
  /// Fastly-internal services – we may revisit this decision
  /// given sufficient customer input.
  TransactionLookupBuilder on_behalf_of(std::string service) &&;

  /// Respect the range in to_stream_with_range even when the body length is not
  /// yet known.
  ///
  /// When a cache item is Found, the length of the cached item may or may not
  /// be known:
  ///
  /// - the item may be fully cached;
  /// - the item may be streaming in, but have a known length; or
  /// - the item may be streaming in progressively, without a known length.
  ///
  /// By default (legacy behavior), if a range is specified but the length is
  /// not known, the contents of the entire item will be provided instead of the
  /// requested range.
  ///
  /// always_use_requested_range indicates any cached item returned by this
  /// lookup should provide only the requested range, regardless of whether the
  /// length is known. An invalid range will eventually return a read error,
  /// possibly after providing some data.
  ///
  /// NOTE: In the future,
  /// the always_use_requested_range behavior will be the default,
  /// and this method will be removed.
  TransactionLookupBuilder always_use_requested_range() &&;

  /// Perform the lookup, entering a `Transaction`.
  ///
  /// If the lookup hits an object being provided by another client, this call
  /// will block until that client provides the object's metadata or cancels.
  tl::expected<Transaction, CacheError> execute() &&;

  /// Perform the lookup, returning a `PendingTransaction`.
  ///
  /// If the lookup hits an object being provided by another client, this call
  /// will not block. Instead, the returned `PendingTransaction` can be used to
  /// wait for the request collapsed lookup to complete. See the documentation
  /// of `PendingTransaction` for more details.
  tl::expected<PendingTransaction, CacheError> execute_async() &&;

private:
  explicit TransactionLookupBuilder(CacheKey key) : key_(std::move(key)) {}
  CacheKey key_;
  LookupOptions options_;
  friend class Transaction;
};

/// Represents a found cached item. This allows access to metadata and the
/// cached body.
///
/// A `Found` instance is returned from `Transaction::found()` when the
/// transaction has found a cached item during the lookup operation.
class Found {
public:
  /// The current age of the cached item.
  std::chrono::nanoseconds age() const;

  /// How long the cached item is/was considered fresh, starting from the
  /// start of the cached item's lifetime.
  ///
  /// Note that this is the maximum possible age, not the freshness time
  /// remaining; see `remaining_ttl()` for that.
  std::chrono::nanoseconds max_age() const;

  /// Returns the remaining TTL: how long this cached item has left before it
  /// is considered stale. Zero if the cached item is stale.
  ///
  /// Note: this reports the _remaining_ freshness period; `max_age()` reports
  /// the maximum possible TTL.
  std::chrono::nanoseconds remaining_ttl() const;

  /// The time for which a cached item can safely be used despite being
  /// considered stale.
  std::chrono::nanoseconds stale_while_revalidate() const;

  /// Determines whether the cached item is stale.
  ///
  /// A cached item is stale if its age is greater than its max-age period.
  bool is_stale() const;

  /// Determines whether the cached item is usable.
  ///
  /// A cached item is usable if its age is less than the sum of the max-age
  /// and stale-while-revalidate periods.
  bool is_usable() const;

  /// Determines the number of cache hits to this cached item.
  ///
  /// **Note**: this hit count only reflects the view of the server that
  /// supplied the cached item. Due to clustering, this count may vary between
  /// potentially many servers within the data center where the item is cached.
  /// See the [clustering
  /// documentation](https://www.fastly.com/documentation/guides/full-site-delivery/fastly-vcl/clustering-in-vcl/)
  /// for details, though note that the exact caching architecture of Compute is
  /// different from VCL services.
  uint64_t hits() const;

  /// The size in bytes of the cached item, if known.
  ///
  /// The length of the cached item may be unknown if the item is currently
  /// being streamed into the cache without a fixed length.
  std::optional<uint64_t> known_length() const;

  /// The user-controlled metadata associated with the cached item.
  std::vector<uint8_t> user_metadata() const;

  /// Retrieves the entire cached item as a `Body` that can be read in a
  /// streaming fashion.
  ///
  /// Only one stream can be active at a time for a given `Found`. The stream
  /// must be fully consumed (read) before a new stream can be created from the
  /// same `Found`. `CacheError::InvalidOperation` will be returned if a
  /// stream is already active for this `Found`. This restriction may be lifted
  /// in future releases.
  tl::expected<Body, CacheError> to_stream() const;

  /// Retrieves a range of bytes from the cached item as a `Body` that can be
  /// read in a streaming fashion.
  ///
  /// `from` and `to` will determine which bytes are returned:
  /// - If `from` is provided and `to` is not provided, the stream will begin
  ///   at the `from` byte and continue to the end of the body.
  /// - If `from` and `to` are both provided, only the range `from..=to` will
  ///   be provided. Note, both ends are inclusive.
  /// - If `to` is provided but `from` is not provided, the last `to` bytes of
  ///   the body will be provided.
  ///
  /// ## Inherently invalid ranges
  ///
  /// If `to` is strictly before `from`, this call returns an error immediately.
  /// (`to == from` is acceptable - the bounds are inclusive, so this addresses
  /// a single byte.)
  ///
  /// ## Known size
  ///
  /// The size of a cached item is known if it has completed streaming or if
  /// the expected length was provided at insert time
  /// (`TransactionInsertBuilder::known_length` and similar).
  ///
  /// If the size of the cached item is known, the range is respected only if
  /// the range is valid (`to` is after `from`, and both ranges are less than
  /// or equal to the size of the item). **Otherwise, no error is returned, and
  /// the entire body is returned instead.**
  ///
  /// ## Unknown size
  ///
  /// The size of a cached item may not be known (if `known_length` was not
  /// provided and the body is still being streamed). If the size of the cached
  /// item is unknown, **by default, the range is ignored, and the entire
  /// cached item is returned.**
  ///
  /// This behavior can be overriden by calling
  /// `TransactionLookupBuilder::always_use_requested_range`. If provided, this
  /// call will block until the start of the interval is found, then begin
  /// streaming content. The `Body` will generate a read error if the end of
  /// the range exceeds the available data.
  tl::expected<Body, CacheError>
  to_stream_from_range(std::optional<uint64_t> from,
                       std::optional<uint64_t> to) const;

private:
  explicit Found(std::shared_ptr<detail::CacheHandle> handle)
      : handle_(std::move(handle)) {}
  explicit Found(std::shared_ptr<detail::CacheReplaceHandle> handle)
      : handle_(std::move(handle)) {}

  detail::CacheHandle *handle() const {
    auto ptr = std::get_if<std::shared_ptr<detail::CacheHandle>>(&handle_);
    return ptr ? ptr->get() : nullptr;
  }
  detail::CacheReplaceHandle *replace_handle() const {
    auto ptr =
        std::get_if<std::shared_ptr<detail::CacheReplaceHandle>>(&handle_);
    return ptr ? ptr->get() : nullptr;
  }

  std::variant<std::shared_ptr<detail::CacheHandle>,
               std::shared_ptr<detail::CacheReplaceHandle>>
      handle_;
  friend class Transaction;
  friend class LookupBuilder;
  friend class TransactionInsertBuilder;
  friend class ReplaceBuilder;
};

/// A cache transaction resulting from a transactional lookup.
///
/// This type is returned from `TransactionLookupBuilder::execute()` and
/// provides access to the result of the lookup as well as methods to insert
/// or update cached items when required.
class Transaction {
public:
  /// Returns a `TransactionLookupBuilder` that will perform a transactional
  /// cache lookup.
  static TransactionLookupBuilder lookup(CacheKey key);

  /// Returns a `Found` object for this cache item, if one is available.
  ///
  /// Even if an object is found, the cache item might be stale and require
  /// updating. Use `Transaction::must_insert_or_update()` to determine whether
  /// this transaction client is expected to update the cached item.
  std::optional<Found> found() const;

  /// Returns `true` if a usable cached item was not found, and this
  /// transaction client is expected to insert one.
  ///
  /// Use `insert()` to insert the cache item, or `cancel_insert_or_update()`
  /// to exit the transaction without providing an item.
  bool must_insert() const;

  /// Returns `true` if a fresh cache item was not found, and this transaction
  /// client is expected to insert a new item or update a stale item.
  ///
  /// Use:
  /// - `update()` to freshen a found item by updating its metadata
  /// - `insert()` to insert a new item (including object data)
  /// - `cancel_insert_or_update()` to exit the transaction without providing
  ///   an item
  bool must_insert_or_update() const;

  /// Cancels the obligation for this transaction client to insert or update a
  /// cache item.
  ///
  /// If there are concurrent transactional lookups that were blocked waiting
  /// on this client to provide the item, one of them will be chosen to be
  /// unblocked and given the `must_insert_or_update()` obligation.
  ///
  /// This method should only be called when `must_insert_or_update()` is true;
  /// otherwise, a `CacheError::InvalidOperation` will be returned.
  tl::expected<void, CacheError> cancel_insert_or_update() const;

  /// Returns a `TransactionUpdateBuilder` that will perform a transactional
  /// cache update.
  ///
  /// Updating an item freshens it by updating its metadata, e.g. its age,
  /// without changing the object itself.
  ///
  /// This method should only be called when `must_insert_or_update()` is true
  /// AND the item is found. Otherwise, a `CacheError::InvalidOperation` will
  /// be returned when attempting to execute the update.
  ///
  /// **Important note**: The `TransactionUpdateBuilder` will replace ALL of
  /// the configuration in the underlying cache item; if any configuration is
  /// not set on the builder, it will revert to the default value.
  TransactionUpdateBuilder update(std::chrono::nanoseconds max_age) &&;

  /// Returns a `TransactionInsertBuilder` that will perform a transactional
  /// cache insertion.
  ///
  /// This method should only be called when `must_insert_or_update()` is true;
  /// otherwise, a `CacheError::InvalidOperation` will be returned when
  /// attempting to execute the insertion.
  TransactionInsertBuilder insert(std::chrono::nanoseconds max_age) &&;

private:
  explicit Transaction(std::shared_ptr<detail::CacheHandle> handle)
      : handle_(std::move(handle)) {}

  std::shared_ptr<detail::CacheHandle> handle_;
  friend class TransactionLookupBuilder;
  friend class PendingTransaction;
};

/// A builder-style API for configuring a non-transactional cache lookup.
///
/// In contrast to `Transaction::lookup()`, a non-transactional `lookup` will
/// not attempt to coordinate with any concurrent cache lookups. If two
/// instances of the service perform a `lookup` at the same time for the same
/// cache key, and the item is not yet cached, they will both get `Ok(None)`
/// from the eventual lookup execution.
///
/// To resolve races between concurrent lookups, use `Transaction::lookup()`
/// instead.
class LookupBuilder {
public:
  /// Sets a multi-value header for this lookup, discarding any previous values
  /// associated with the header name.
  ///
  /// Note: These headers are narrowly useful for implementing cache lookups
  /// incorporating the semantics of the [HTTP
  /// Vary](https://www.rfc-editor.org/rfc/rfc9110#section-12.5.5) header, but
  /// the APIs in this module are not suitable for HTTP caching out-of-the-box.
  /// Future SDK releases will contain an HTTP Cache API.
  ///
  /// The headers act as additional factors in object selection, and the choice
  /// of which headers to factor in is determined during insertion, via e.g.
  /// `crate::cache::core::InsertBuilder::vary_by`. A lookup will succeed when
  /// there is at least one cached item that matches lookup’s cache key, and all
  /// of the lookup’s headers included in the cache items’ vary_by list match
  /// the corresponding headers in that cached item.
  ///
  /// A typical example is a cached HTTP response, where the request had an
  /// Accept-Encoding header. In that case, the origin server may or may not
  /// decide on a given encoding, and whether that same response is suitable for
  /// a request with a different (or missing) Accept-Encoding header is
  /// determined by whether Accept-Encoding is listed in Vary header in the
  /// origin’s response.
  LookupBuilder header_values(std::string_view name,
                              std::span<const http::HeaderValue> values) &&;

  /// Sets a single-value header for this lookup, discarding any previous values
  /// associated with the header `name`.

  /// Note: These headers are narrowly useful for implementing cache lookups
  /// incorporating the semantics of the [HTTP
  /// Vary](https://www.rfc-editor.org/rfc/rfc9110#section-12.5.5)
  /// header, but the APIs in this module are not suitable for HTTP
  /// caching out-of-the-box. Future SDK releases will contain an HTTP
  /// Cache API.
  ///
  /// The headers act as additional factors in object selection, and
  /// the choice of which headers to factor in is determined during
  /// insertion, via e.g.
  /// `crate::cache::core::InsertBuilder::vary_by`. A lookup will
  /// succeed when there is at least one cached item that matches
  /// lookup’s cache key, and all of the lookup’s headers included in
  /// the cache items’ vary_by list match the corresponding headers in
  /// that cached item.
  ///
  /// A typical example is a cached HTTP response, where the request
  /// had an Accept-Encoding header. In that case, the origin server
  /// may or may not decide on a given encoding, and whether that same
  /// response is suitable for a request with a different (or missing)
  /// Accept-Encoding header is determined by whether Accept-Encoding
  /// is listed in Vary header in the origin’s response.
  LookupBuilder header(std::string_view name,
                       const http::HeaderValue &value) &&;

  /// Perform this lookup on behalf of another service, using its data store.
  ///
  /// *Internal / Privileged*
  /// This operation is privileged, and attempts to use this functionality
  /// without proper privileges will cause errors. If you are interested in
  /// having two or more of your services share the same cache, please talk to
  /// your Fastly account representative. While we have no plans to offer this
  /// ability widely – this capability is only currently allowed for
  /// Fastly-internal services – we may revisit this decision given sufficient
  /// customer input.
  LookupBuilder on_behalf_of(std::string service) &&;

  /// Respect the range in to_stream_with_range even when the body length is not
  /// yet known.
  ///
  /// When a cache item is Found, the length of the cached item may or may not
  /// be known:
  ///
  /// - the item may be fully cached;
  /// - the item may be streaming in, but have a known length; or
  /// - the item may be streaming in progressively, without a known length.
  ///
  /// By default (legacy behavior), if a range is specified but the length is
  /// not known, the contents of the entire item will be provided instead of the
  /// requested range.
  ///
  /// always_use_requested_range indicates any cached item returned by this
  /// lookup should provide only the requested range, regardless of whether the
  /// length is known. An invalid range will eventually return a read error,
  /// possibly after providing some data.
  ///
  /// NOTE: In the future,
  /// the always_use_requested_range behavior will be the default,
  /// and this method will be removed.
  LookupBuilder always_use_requested_range() &&;

  /// Perform the lookup, returning a `Found` object if a usable cached item
  /// was found.
  ///
  /// A cached item is _usable_ if its age is less than the sum of its max_age
  /// and its stale-while-revalidate period. Items beyond that age are unusably
  /// stale.
  tl::expected<std::optional<Found>, CacheError> execute() &&;

private:
  explicit LookupBuilder(CacheKey key) : key_(std::move(key)) {}
  CacheKey key_;
  LookupOptions options_;
  friend LookupBuilder lookup(CacheKey key);
};

/// Returns a `LookupBuilder` that will perform a non-transactional cache
/// lookup.
///
/// In contrast to `Transaction::lookup()`, a non-transactional lookup will not
/// attempt to coordinate with any concurrent cache lookups. If two instances of
/// the service perform a lookup at the same time for the same cache key, and
/// the item is not yet cached, they will both get nothing from the eventual
/// lookup execution. Without further coordination, they may both end up
/// performing the work needed to `insert()` the item (which usually involves
/// origin requests and/or computation) and racing with each other to insert.
///
/// To resolve such races between concurrent lookups, use
/// `Transaction::lookup()` instead.
LookupBuilder lookup(CacheKey key);

/// A builder-style API for configuring a non-transactional cache insertion.
///
/// Like `lookup()`, `insert()` may race with concurrent lookups or insertions,
/// and will unconditionally overwrite existing cached items rather than
/// allowing for revalidation of an existing object.
///
/// The transactional equivalent is `Transaction::insert()`, which may only be
/// called following a transactional lookup when
/// `Transaction::must_insert_or_update()` returns `true`.
class InsertBuilder {
public:
  /// Sets a multi-value header for this lookup, discarding any previous values
  /// associated with the header name.
  ///
  /// Note: These headers are narrowly useful for implementing cache lookups
  /// incorporating the semantics of the [HTTP
  /// Vary](https://www.rfc-editor.org/rfc/rfc9110#section-12.5.5) header, but
  /// the APIs in this module are not suitable for HTTP caching out-of-the-box.
  /// Future SDK releases will contain an HTTP Cache API.
  ///
  /// The headers act as additional factors in object selection, and the choice
  /// of which headers to factor in is determined during insertion, via e.g.
  /// `crate::cache::core::InsertBuilder::vary_by`. A lookup will succeed when
  /// there is at least one cached item that matches lookup’s cache key, and all
  /// of the lookup’s headers included in the cache items’ vary_by list match
  /// the corresponding headers in that cached item.
  ///
  /// A typical example is a cached HTTP response, where the request had an
  /// Accept-Encoding header. In that case, the origin server may or may not
  /// decide on a given encoding, and whether that same response is suitable for
  /// a request with a different (or missing) Accept-Encoding header is
  /// determined by whether Accept-Encoding is listed in Vary header in the
  /// origin’s response.
  InsertBuilder header_values(std::string_view name,
                              std::span<const http::HeaderValue> values) &&;

  /// Sets a single-value header for this lookup, discarding any previous values
  /// associated with the header `name`.

  /// Note: These headers are narrowly useful for implementing cache lookups
  /// incorporating the semantics of the [HTTP
  /// Vary](https://www.rfc-editor.org/rfc/rfc9110#section-12.5.5)
  /// header, but the APIs in this module are not suitable for HTTP
  /// caching out-of-the-box. Future SDK releases will contain an HTTP
  /// Cache API.
  ///
  /// The headers act as additional factors in object selection, and
  /// the choice of which headers to factor in is determined during
  /// insertion, via e.g.
  /// `crate::cache::core::InsertBuilder::vary_by`. A lookup will
  /// succeed when there is at least one cached item that matches
  /// lookup’s cache key, and all of the lookup’s headers included in
  /// the cache items’ vary_by list match the corresponding headers in
  /// that cached item.
  ///
  /// A typical example is a cached HTTP response, where the request
  /// had an Accept-Encoding header. In that case, the origin server
  /// may or may not decide on a given encoding, and whether that same
  /// response is suitable for a request with a different (or missing)
  /// Accept-Encoding header is determined by whether Accept-Encoding
  /// is listed in Vary header in the origin’s response.
  InsertBuilder header(std::string_view name,
                       const http::HeaderValue &value) &&;

  /// Sets the list of headers that must match when looking up this cached item.
  InsertBuilder vary_by(std::vector<std::string> headers) &&;

  /// Sets the initial age of the cached item, to be used in freshness
  /// calculations.
  ///
  /// The initial age is zero by default.
  InsertBuilder initial_age(std::chrono::nanoseconds age) &&;

  /// Sets the time for which a cached item can safely be used despite being
  /// considered stale.
  InsertBuilder stale_while_revalidate(std::chrono::nanoseconds duration) &&;

  /// Sets the surrogate keys that can be used for purging this cached item.
  ///
  /// Surrogate key purges are the only means to purge specific items from the
  /// cache. At least one surrogate key must be set in order to remove an item
  /// without performing a
  /// [purge-all](https://www.fastly.com/documentation/guides/concepts/cache/purging/#purge-all),
  /// waiting for the item’s TTL to elapse, or overwriting the item with
  /// `crate::cache::core::insert()`.
  ///
  /// Surrogate keys must contain only printable ASCII characters (those between
  /// 0x21 and 0x7E, inclusive). Any invalid keys will be ignored.
  ///
  /// See the [Fastly surrogate keys
  /// guide](https://www.fastly.com/documentation/guides/full-site-delivery/purging/working-with-surrogate-keys/)
  /// for details.
  InsertBuilder surrogate_keys(std::vector<std::string> keys) &&;

  /// Sets the known length of the cached item.
  InsertBuilder known_length(std::uint64_t length) &&;

  /// Sets the user-defined metadata to associate with the cached item.
  InsertBuilder user_metadata(std::vector<std::uint8_t> metadata) &&;

  /// Enable or disable PCI/HIPAA-compliant non-volatile caching.
  ///
  /// By default, this is false.
  ///
  /// See the [Fastly PCI-Compliant Caching and Delivery
  /// documentation](https://docs.fastly.com/products/pci-compliant-caching-and-delivery)
  /// for details.
  InsertBuilder sensitive_data(bool is_sensitive) &&;

  /// Sets the maximum time the cached item may live on a deliver node in a POP.
  InsertBuilder deliver_node_max_age(std::chrono::nanoseconds duration) &&;

  /// Perform this insertion on behalf of another service, using its data store.
  ///
  /// *Internal / Privileged*
  /// This operation is privileged, and attempts to use this functionality
  /// without proper privileges will cause errors. If you are interested in
  /// having two or more of your services share the same cache, please talk to
  /// your Fastly account representative. While we have no plans to offer this
  /// ability widely – this capability is only currently allowed for
  /// Fastly-internal services – we may revisit this decision given sufficient
  /// customer input.
  InsertBuilder on_behalf_of(std::string service) &&;

  /// Begin the insertion, returning a `StreamingBody` for providing the cached
  /// object itself.
  tl::expected<http::StreamingBody, CacheError> execute() &&;

private:
  explicit InsertBuilder(CacheKey key, WriteOptions options)
      : key_(std::move(key)), options_(std::move(options)) {}
  CacheKey key_;
  WriteOptions options_;
  friend InsertBuilder insert(CacheKey key, std::chrono::nanoseconds max_age);
};

/// Returns an `InsertBuilder` that will perform a non-transactional cache
/// insertion.
///
/// The required `max_age` argument is the maximal "time to live" for the cache
/// item: the time for which the item will be considered fresh, starting from
/// the start of its history.
InsertBuilder insert(CacheKey key, std::chrono::nanoseconds max_age);

class Replace;

/// A builder-style API for configuring a non-transactional cache replacement.
class ReplaceBuilder {
public:
  /// Begin the replace, returning a `Replace` for reading the object to be
  /// replaced (if it exists) which is also used to provide the new replacement
  /// object.
  ///
  /// A `Replace` gives access to the existing object, if one is stored, that
  /// may be used to construct the replacement object. Use `Replace::execute()`
  /// to get a `StreamingBody` to write the replacement object into. The
  /// existing object cannot be accessed after calling `Replace::execute()`.
  ///
  /// For the replace to complete successfully, the object must be written into
  /// the `StreamingBody`, and then `StreamingBody::finish` must be called. If
  /// the `StreamingBody` is dropped before calling `StreamingBody::finish`, the
  /// replacement is considered incomplete, and any concurrent lookups that may
  /// be reading from the object as it is streamed into the cache may encounter
  /// a streaming error.
  tl::expected<Replace, CacheError> begin() &&;

  // TODO: header and header_values

  /// Sets the strategy for performing the replace.
  ReplaceBuilder replace_strategy(ReplaceStrategy strategy) &&;

  /// Respect the range in to_stream_with_range even when the body length is not
  /// yet known.
  ///
  /// When a cache item is Found, the length of the cached item may or may not
  /// be known:
  ///
  /// - the item may be fully cached;
  /// - the item may be streaming in, but have a known length; or
  /// - the item may be streaming in progressively, without a known length.
  ///
  /// By default (legacy behavior), if a range is specified but the length is
  /// not known, the contents of the entire item will be provided instead of the
  /// requested range.
  ///
  /// always_use_requested_range indicates any cached item returned by this
  /// lookup should provide only the requested range, regardless of whether the
  /// length is known. An invalid range will eventually return a read error,
  /// possibly after providing some data.
  ///
  /// NOTE: In the future,
  /// the always_use_requested_range behavior will be the default,
  /// and this method will be removed.
  ReplaceBuilder always_use_requested_range() &&;

private:
  explicit ReplaceBuilder(CacheKey key) : key_(std::move(key)) {}
  CacheKey key_;
  ReplaceOptions options_;
  friend ReplaceBuilder replace(CacheKey key);
};

/// An in-progress Replace operation.
///
/// This type is returned from `ReplaceBuilder::begin()`.
class Replace {
public:
  /// Finish using the existing object and start writing a replacement object to
  /// the `StreamingBody`.
  ///
  /// The required `max_age` argument is the "time to live" for the replacement
  /// cache item: the time for which the item will be considered fresh, starting
  /// from the start of its history (now, unless `initial_age` was provided).
  tl::expected<http::StreamingBody, CacheError>
  execute(std::chrono::nanoseconds max_age) &&;

  /// The existing object, if one exists. The existing object may be stale.
  const std::optional<Found> &existing_object() const {
    return existing_object_;
  }

  /// Sets a multi-value header for this lookup, discarding any previous values
  /// associated with the header name.
  ///
  /// Note: These headers are narrowly useful for implementing cache lookups
  /// incorporating the semantics of the [HTTP
  /// Vary](https://www.rfc-editor.org/rfc/rfc9110#section-12.5.5) header, but
  /// the APIs in this module are not suitable for HTTP caching out-of-the-box.
  /// Future SDK releases will contain an HTTP Cache API.
  ///
  /// The headers act as additional factors in object selection, and the choice
  /// of which headers to factor in is determined during insertion, via e.g.
  /// `crate::cache::core::InsertBuilder::vary_by`. A lookup will succeed when
  /// there is at least one cached item that matches lookup’s cache key, and all
  /// of the lookup’s headers included in the cache items’ vary_by list match
  /// the corresponding headers in that cached item.
  ///
  /// A typical example is a cached HTTP response, where the request had an
  /// Accept-Encoding header. In that case, the origin server may or may not
  /// decide on a given encoding, and whether that same response is suitable for
  /// a request with a different (or missing) Accept-Encoding header is
  /// determined by whether Accept-Encoding is listed in Vary header in the
  /// origin’s response.
  Replace header_values(std::string_view name,
                        std::span<const http::HeaderValue> values) &&;

  /// Sets a single-value header for this lookup, discarding any previous values
  /// associated with the header `name`.

  /// Note: These headers are narrowly useful for implementing cache lookups
  /// incorporating the semantics of the [HTTP
  /// Vary](https://www.rfc-editor.org/rfc/rfc9110#section-12.5.5)
  /// header, but the APIs in this module are not suitable for HTTP
  /// caching out-of-the-box. Future SDK releases will contain an HTTP
  /// Cache API.
  ///
  /// The headers act as additional factors in object selection, and the choice
  /// of which headers to factor in is determined during insertion, via e.g.
  /// `crate::cache::core::InsertBuilder::vary_by`. A lookup will
  /// succeed when there is at least one cached item that matches
  /// lookup’s cache key, and all of the lookup’s headers included in
  /// the cache items’ vary_by list match the corresponding headers in
  /// that cached item.
  ///
  /// A typical example is a cached HTTP response, where the request
  /// had an Accept-Encoding header. In that case, the origin server
  /// may or may not decide on a given encoding, and whether that same
  /// response is suitable for a request with a different (or missing)
  /// Accept-Encoding header is determined by whether Accept-Encoding
  /// is listed in Vary header in the origin’s response.
  Replace header(std::string_view name, const http::HeaderValue &value) &&;

  /// Sets the list of headers that must match when looking up this cached item.
  Replace vary_by(std::vector<std::string> headers) &&;

  /// Sets the initial age of the cached item, to be used in freshness
  /// calculations. The initial age is zero by default.
  Replace initial_age(std::chrono::nanoseconds age) &&;

  /// Sets the time for which a cached item can safely be used despite being
  /// considered stale.
  Replace stale_while_revalidate(std::chrono::nanoseconds duration) &&;

  /// Sets the surrogate keys that can be used for purging this cached item.
  ///
  /// Surrogate key purges are the only means to purge specific items from the
  /// cache. At least one surrogate key must be set in order to remove an item
  /// without performing a
  /// [purge-all](https://www.fastly.com/documentation/guides/concepts/cache/purging/#purge-all),
  /// waiting for the item’s TTL to elapse, or overwriting the item with
  /// `crate::cache::core::insert()`.
  ///
  /// Surrogate keys must contain only printable ASCII characters (those between
  /// 0x21 and 0x7E, inclusive). Any invalid keys will be ignored.
  ///
  /// See the [Fastly surrogate keys
  /// guide](https://www.fastly.com/documentation/guides/full-site-delivery/purging/working-with-surrogate-keys/)
  /// for details.
  Replace surrogate_keys(std::vector<std::string> keys) &&;

  /// Sets the known length of the cached item.
  Replace known_length(std::uint64_t length) &&;

  /// Sets the user-defined metadata to associate with the cached item.
  Replace user_metadata(std::vector<std::uint8_t> metadata) &&;

  /// Enable or disable PCI/HIPAA-compliant non-volatile caching.
  ///
  /// By default, this is false.
  ///
  /// See the [Fastly PCI-Compliant Caching and Delivery
  /// documentation](https://docs.fastly.com/products/pci-compliant-caching-and-delivery)
  /// for details.
  Replace sensitive_data(bool is_sensitive) &&;

  /// Sets the maximum time the cached item may live on a deliver node in a POP.
  Replace deliver_node_max_age(std::chrono::nanoseconds duration) &&;

private:
  Replace(std::shared_ptr<detail::CacheReplaceHandle> handle,
          std::optional<Found> existing_object)
      : handle_(std::move(handle)),
        existing_object_(std::move(existing_object)) {}

  std::shared_ptr<detail::CacheReplaceHandle> handle_;
  std::optional<Found> existing_object_;
  WriteOptions options_{std::chrono::nanoseconds(0)};
  friend class ReplaceBuilder;
};

/// Returns a `ReplaceBuilder` that will perform a non-transactional cache
/// replacement.
ReplaceBuilder replace(CacheKey key);

} // namespace fastly::cache::core

#endif // FASTLY_CACHE_H
