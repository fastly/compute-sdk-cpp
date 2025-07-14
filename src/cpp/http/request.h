#ifndef FASTLY_HTTP_REQUEST_H
#define FASTLY_HTTP_REQUEST_H

#include "../backend.h"
#include "../error.h"
#include "../sdk-sys.h"
#include "../util.h"
#include "body.h"
#include "header.h"
#include "method.h"
#include "response.h"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace fastly::backend {
class Backend;
}

namespace fastly::http {

class Body;
class StreamingBody;
class Response;
class Request;

namespace request {

/// A handle to a pending asynchronous request returned by
/// `Request::send_async()` or
/// `Request::send_async_streaming()`.
///
/// A handle can be evaluated using `PendingRequest::poll()`,
/// `PendingRequest::wait()`, or
/// `http::select`. It can also be discarded if the request was sent for effects
/// it might have, and the response is unimportant.
class PendingRequest {

  friend Request;
  friend std::pair<fastly::expected<Response>, std::vector<PendingRequest>>
  select(std::vector<PendingRequest> &reqs);

  /// Try to get the result of a pending request without blocking.
  ///
  /// This method returns immediately with a `std::variant` containing either
  /// the original `PendingRequest` if the response was not ready, or a
  /// `Response` if the response was ready. If you want to block until a result
  /// is ready, use `PendingRequest::wait()`.
  std::variant<PendingRequest, fastly::expected<Response>> poll();

  /// Block until the result of a pending request is ready.
  ///
  /// If you want check whether the result is ready without blocking, use
  /// `PendingRequest::poll()`.
  fastly::expected<Response> wait();

  /// Cloned version of the original request that was sent, without the original
  /// body. This is only a copy and cannot be used to modify anything, since the
  /// request has already been sent.
  Request cloned_sent_req();

private:
  rust::Box<fastly::sys::http::request::PendingRequest> req;

  PendingRequest(rust::Box<fastly::sys::http::request::PendingRequest> r)
      : req(std::move(r)) {};
};

/// Given a collection of `PendingRequest`s, block until the result of one of
/// the requests is ready.
///
/// Returns an `std::pair` of `<result, remaining>`, where:
///
/// - `result` is the result of the request that became ready.
///
/// - `remaining` is a vector containing all of the requests that did not become
/// ready. The order of the requests in this vector is not guaranteed to match
/// the order of the requests in the argument collection.
///
/// ### Panics
///
/// Panics if the argument collection is empty, or contains too many requests.
std::pair<fastly::expected<Response>, std::vector<PendingRequest>>
select(std::vector<PendingRequest> &reqs);

} // namespace request

/// An HTTP request, including body, headers, method, and URL.
///
/// # Getting the client request
///
/// Call `Request::from_client()` to get the client request being handled by
/// this execution of the Compute program.
///
/// # Creation and conversion
///
/// New requests can be created programmatically with the `Request()`
/// constructor. In addition, there are convenience constructors like
/// `Request::get()` which automatically select the appropriate method.
///
/// # Sending backend requests
///
/// Requests can be sent to a backend in blocking or asynchronous fashion using
/// `Request::send()`, `Request::send_async()`, or
/// `Request::send_async_streaming()`.
///
/// # Builder-style methods
///
/// `Request` can be used as a builder allowing requests to be constructed and
/// used through method chaining. Methods with the `with_` name prefix, such as
/// `Request::with_header()`, return a moved `Request` to allow chaining. The
/// builder style is typically most useful when constructing and using a request
/// in a single expression.
///
/// For example:
///
/// ```cpp
/// Request::get("https://example.com")
///     .with_header("my-header", "hello!")
///     .with_header("my-other-header", "Здравствуйте!")
///     .send("example_backend");
/// ```
///
/// # Setter methods
///
/// Setter methods, such as `Request::set_header()`, are prefixed by `set_`, and
/// can be used interchangeably with the builder-style methods, allowing you to
/// mix and match styles based on what is most convenient for your program.
/// Setter methods tend to work better than builder-style methods when
/// constructing a request involves conditional branches or loops.
///
/// For example:
///
/// ```cpp
/// auto req{Request::get("https://example.com").with_header("my-header",
/// "hello!")};
/// if (needs_translation) {
///     req.set_header("my-other-header", "Здравствуйте!");
/// }
/// req.send("example_backend");
/// ```
class Request {
  friend request::PendingRequest;
  friend Response;

public:
  /// Create a new request with the given method and URL, no headers, and an
  /// empty body.
  Request(Method method, std::string_view url);

  /// Create a new `GET` `Request` with the given URL, no headers, and an
  /// empty body.
  static Request get(std::string_view url);

  /// Create a new `HEAD` `Request` with the given URL, no headers, and an
  /// empty body.
  static Request head(std::string_view url);

  /// Create a new `POST` `Request` with the given URL, no headers, and an
  /// empty body.
  static Request post(std::string_view url);

  /// Create a new `PUT` `Request` with the given URL, no headers, and an
  /// empty body.
  static Request put(std::string_view url);

  /// Create a new `DELETE` `Request` with the given URL, no headers, and an
  /// empty body.
  static Request delete_(std::string_view url);

  /// Create a new `CONNECT` `Request` with the given URL, no headers, and an
  /// empty body.
  static Request connect(std::string_view url);

  /// Create a new `OPTIONS` `Request` with the given URL, no headers, and an
  /// empty body.
  static Request options(std::string_view url);

  /// Create a new `TRACE` `Request` with the given URL, no headers, and an
  /// empty body.
  static Request trace(std::string_view url);

  /// Create a new `PATCH` `Request` with the given URL, no headers, and an
  /// empty body.
  static Request patch(std::string_view url);

  /// Get the client request being handled by this execution of the Compute
  /// program.
  ///
  /// # Panics
  ///
  /// This method panics if the client request has already been retrieved by
  /// this method or by the low-level handle API.
  static Request from_client();

  /// Return `true` if this request is from the client of this execution of the
  /// Compute program.
  bool is_from_client();

  /// Make a new request with the same method, url, headers, and version of this
  /// request, but no body.
  ///
  /// If you also need to clone the request body, use
  /// [`clone_with_body()`][`Self::clone_with_body()`]
  ///
  /// # Examples
  ///
  /// ```cpp
  /// auto original = Request::post("https://example.com")
  ///     .with_header("hello", "world!")
  ///     .with_body("hello");
  /// auto new_req = original.clone_without_body();
  /// assert(original.get_method() == new.get_method());
  /// assert(original.get_url() == new.get_url());
  /// assert(original.get_header("hello") == new.get_header("hello"));
  /// assert(original.has_body());
  /// assert(!new.has_body());
  /// ```
  Request clone_without_body();

  /// Clone this request by reading in its body, and then writing the same body
  /// to the original and the cloned request.
  ///
  /// This method requires mutable access to this request because reading from
  /// and writing to the body can involve an HTTP connection.
  Request clone_with_body();

  /// Retrieve a reponse for the request, either from cache or by sending it to
  /// the given backend server. Returns once the response headers have been
  /// received, or an error occurs.
  ///
  /// # Examples
  ///
  /// Sending the client request to a backend without modification:
  ///
  /// ```cpp
  /// auto backend_resp{Request::from_client().send("example_backend")};
  /// assert(backend_resp.get_status().is_success());
  /// ```
  ///
  /// Sending a synthetic request:
  ///
  /// ```cpp
  /// auto
  /// backend_resp{Request::get("https://example.com").send("example_backend")};
  /// assert(backend_resp.get_status().is_success());
  /// ```
  fastly::expected<Response> send(fastly::backend::Backend &backend);
  fastly::expected<Response> send(std::string_view backend_name);

  /// Begin sending the request to the given backend server, and return a
  /// `PendingRequest` that can yield the backend response or an error.
  ///
  /// This method returns as soon as the request begins sending to the backend,
  /// and transmission of the request body and headers will continue in the
  /// background.
  ///
  /// This method allows for sending more than one request at once and receiving
  /// their responses in arbitrary orders. See `PendingRequest` for more details
  /// on how to wait on, poll, or select between pending requests.
  ///
  /// This method is also useful for sending requests where the response is
  /// unimportant, but the request may take longer than the Compute program is
  /// able to run, as the request will continue sending even after the program
  /// that initiated it exits.
  ///
  /// # Examples
  ///
  /// Sending a request to two backends and returning whichever response
  /// finishes first:
  ///
  /// ```cpp
  /// auto backend_resp_1{Request::get("https://example.com/")
  ///     .send_async("example_backend_1")};
  /// auto backend_resp_2{Request::get("https://example.com/")
  ///     .send_async("example_backend_2")};
  /// auto [selected_resp, _others] =
  /// fastly::http::request::select({backend_resp_1, backend_resp_2});
  /// selected_resp.send_to_client();
  /// ```
  ///
  /// Sending a long-running request and ignoring its result so that the program
  /// can exit before
  /// it completes:
  ///
  /// ```cpp
  /// Request::post("https://example.com")
  ///     .with_body(some_large_file)
  ///     .send_async("example_backend");
  /// ```
  fastly::expected<request::PendingRequest>
  send_async(fastly::backend::Backend &backend);
  fastly::expected<request::PendingRequest>
  send_async(std::string_view backend_name);

  /// Begin sending the request to the given backend server, and return a
  /// `PendingRequest` that
  /// can yield the backend response or an error along with a `StreamingBody`
  /// that can accept
  /// further data to send.
  ///
  /// The backend connection is only closed once `StreamingBody::finish()` is
  /// called. The
  /// `PendingRequest` will not yield a `Response` until the
  /// `StreamingBody` is finished.
  ///
  /// This method is most useful for programs that do some sort of processing or
  /// inspection of a
  /// potentially-large client request body. Streaming allows the program to
  /// operate on small
  /// parts of the body rather than having to read it all into memory at once.
  ///
  /// This method returns as soon as the request begins sending to the backend,
  /// and transmission
  /// of the request body and headers will continue in the background.
  ///
  /// # Examples
  ///
  /// Count the number of lines in a UTF-8 client request body while sending it
  /// to the backend:
  ///
  /// ```cpp
  /// auto req{Request::from_client()};
  /// // Take the body so we can iterate through its lines later
  /// auto req_body{req.take_body()};
  /// // Start sending the client request to the client with a now-empty body
  /// auto [backend_body, pending_req] = req
  ///     .send_async_streaming("example_backend");
  ///
  /// size_t num_lines{0};
  /// std::string buf;
  /// while (std::getline(req_body, buf)) {
  ///   num_lines++;
  ///   // Write the line to the streaming backend body
  ///   backend_body << buf << "\n" << std::flush;
  /// }
  /// // Finish the streaming body to allow the backend connection to close
  /// backend_body.finish();
  ///
  /// std::cout
  ///   << "client request body contained "
  ///   << num_lines
  ///   << " lines"
  ///   << std::endl;
  /// ```
  fastly::expected<std::pair<StreamingBody, request::PendingRequest>>
  send_async_streaming(fastly::backend::Backend &backend);
  fastly::expected<std::pair<StreamingBody, request::PendingRequest>>
  send_async_streaming(std::string_view backend_name);

  /// Builder-style equivalent of `Request::set_body()`.
  Request with_body(Body body);

  /// Returns `true` if this request has a body.
  bool has_body();

  /// Take and return the body from this request.
  ///
  /// After calling this method, this request will no longer have a body.
  Body take_body();

  /// Set the given value as the request's body.
  void set_body(Body body);

  /// Append another [`Body`] to the body of this request without reading or
  /// writing any body contents.
  ///
  /// If this request does not have a body, the appended body is set as the
  /// request's body.
  ///
  /// This method should be used when combining bodies that have not
  /// necessarily been read yet, such as the body of the client. To append
  /// contents that are already in memory as strings or bytes, you should
  /// instead use
  /// [`get_body_mut()`][`Self::get_body_mut()`] to write the contents to the
  /// end of the body.
  ///
  /// # Examples
  ///
  /// ```cpp
  /// auto req{Request::post("https://example.com").with_body("hello! client
  /// says: ")}; req.append_body(Request::from_client().into_body());
  /// req.send("example_backend");
  /// ```
  void append_body(Body &body);

  /// Consume the request and return its body as a byte vector.
  std::vector<uint8_t> into_body_bytes();

  /// Consume the request and return its body as a string.
  std::string into_body_string();

  /// Consume the request and return its body as a `Body` instance.
  Body into_body();

  /// Builder-style equivalent of
  /// `Request::set_body_text_plain()`.
  fastly::expected<Request> with_body_text_plain(std::string_view body);

  /// Set the given string as the request's body with content type
  /// `text/plain; charset=UTF-8`.
  fastly::expected<void> set_body_text_plain(std::string_view body);

  /// Builder-style equivalent of
  /// `Request::set_body_text_html()`.
  fastly::expected<Request> with_body_text_html(std::string_view body);

  /// Set the given string as the request's body with content type `text/html;
  /// charset=UTF-8`.
  fastly::expected<void> set_body_text_html(std::string_view body);

  /// Take and return the body from this request as a string.
  ///
  /// After calling this method, this request will no longer have a body.
  std::string take_body_string();

  /// Builder-style equivalent of
  /// `Request::set_body_octet_stream()`.
  Request with_body_octet_stream(std::vector<uint8_t> body);

  /// Set the given bytes as the request's body with content type
  /// `application/octet-stream`.
  void set_body_octet_stream(std::vector<uint8_t> body);

  /// Take and return the body from this request as a vector of bytes.
  ///
  /// After calling this method, this request will no longer have a body.
  std::vector<uint8_t> take_body_bytes();

  // ChunksIter read_body_chunks(size_t chunk_size);

  /// Get the MIME type described by the request's
  /// [`Content-Type`](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Type)
  /// header, or `std::nullopt` if that header is absent or contains an
  /// invalid MIME type.
  std::optional<std::string> get_content_type();

  /// Builder-style equivalent of
  /// `Request::set_content_type()`.
  Request with_content_type(std::string_view mime);

  /// Set the MIME type described by the request's
  /// [`Content-Type`](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Type)
  /// header.
  ///
  /// Any existing `Content-Type` header values will be overwritten.
  void set_content_type(std::string_view mime);

  /// Get the value of the request's
  /// [`Content-Length`](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Length)
  /// header, if it exists.
  std::optional<size_t> get_content_length();

  /// Returns whether the given header name is present in the request.
  fastly::expected<bool> contains_header(std::string_view name);

  /// Builder-style equivalent of `Request::append_header()`.
  fastly::expected<Request> with_header(std::string_view name,
                                        std::string_view value);

  /// Builder-style equivalent of `Request::set_header()`.
  fastly::expected<Request> with_set_header(std::string_view name,
                                            std::string_view value);

  /// Get the value of a header as a string, or `std::nullopt` if the header
  /// is not present.
  ///
  /// If there are multiple values for the header, only one is returned, which
  /// may be any of the values. See
  /// `Request::get_header_all()`
  /// all of the values.
  // TODO(@zkat): do a proper HeaderValue situation here?
  fastly::expected<std::optional<std::string>>
  get_header(std::string_view name);

  /// Get an iterator of all the values of a header.
  fastly::expected<HeaderValuesIter> get_header_all(std::string_view name);

  // TODO(@zkat): sigh. IDK
  // ??? get_headers();
  // HeaderNamesIter get_header_names();

  /// Set a request header to the given value, discarding any previous values
  /// for the given header name.
  fastly::expected<void> set_header(std::string_view name,
                                    std::string_view value);

  /// Add a request header with given value.
  ///
  /// Unlike `Request::set_header()`, this does not discard existing values
  /// for the same header name.
  fastly::expected<void> append_header(std::string_view name,
                                       std::string_view value);

  /// Remove all request headers of the given name, and return one of the
  /// removed header values if any were present.
  fastly::expected<std::optional<std::string>>
  remove_header(std::string_view name);

  /// Builder-style equivalent of `Request::set_method()`.
  Request with_method(Method method);

  /// Get the request method.
  Method get_method();

  /// Set the request method.
  void set_method(Method method);

  /// Builder-style equivalent of `Request::set_url()`.
  fastly::expected<Request> with_url(std::string_view url);

  /// Get the request URL as a string.
  std::string get_url();

  /// Set the request URL.
  fastly::expected<void> set_url(std::string_view url);

  /// Get the path component of the request URL.
  ///
  /// # Examples
  ///
  /// ```cpp
  /// auto req{Request::get("https://example.com/hello#world")};
  /// assert(req.get_path() == "/hello");
  /// ```
  std::string get_path();

  /// Builder-style equivalent of `Request::set_path()`.
  fastly::expected<Request> with_path(std::string_view path);

  /// Set the path component of the request URL.
  /// # Examples
  ///
  /// ```cpp
  /// auto req{Request::get("https://example.com/")};
  /// req.set_path("/hello");
  /// assert!(req.get_url(), "https://example.com/hello");
  /// ```
  fastly::expected<void> set_path(std::string_view path);

  /// Get the query component of the request URL, if it exists, as a
  /// percent-encoded ASCII string.
  std::optional<std::string> get_query_string();

  /// Get the value of a query parameter in the request's URL.
  ///
  /// This assumes that the query string is a `&` separated list of
  /// `parameter=value` pairs. The value of the first occurrence of
  /// `parameter` is returned. No URL decoding is performed.
  std::optional<std::string> get_query_parameter(std::string_view param);

  /// Builder-style equivalent of `Request::set_query()`.
  fastly::expected<Request> with_query_string(std::string_view query);

  /// Set the query string of the request URL query component to the given
  /// string, performing percent-encoding if necessary.
  ///
  /// # Examples
  ///
  /// ```no_run
  /// auto req{Request::get("https://example.com/foo")};
  /// req.set_query_string("hello=🌐!&bar=baz");
  /// assert(req.get_url(),
  /// "https://example.com/foo?hello=%F0%9F%8C%90!&bar=baz");
  /// ```
  fastly::expected<void> set_query_string(std::string_view query);

  /// Remove the query component from the request URL, if one exists.
  void remove_query();

  // TODO(@zkat): need Version enum
  // Request with_version(Version version);
  // Version get_version();
  // void set_version(Version version);

  /// Builder-style equivalent of `Request::set_pass()`.
  Request with_pass(bool pass);

  /// Set whether this request should be cached if sent to a backend.
  ///
  /// By default this is `false`, which means the backend will only be reached
  /// if a cached response is not available. Set this to `true` to send the
  /// request directly to the backend without caching.
  ///
  /// # Overrides
  ///
  /// Setting this to `true` overrides any other custom caching behaviors for
  /// this request, such as `Request::set_ttl()` or
  /// `Request::set_surrogate_key()`.
  void set_pass(bool pass);

  /// Builder-style equivalent of `Request::set_ttl()`.
  Request with_ttl(uint32_t ttl);

  /// Override the caching behavior of this request to use the given Time to
  /// Live (TTL), in seconds.
  ///
  /// # Overrides
  ///
  /// This overrides the behavior specified in the response headers, and sets
  /// the `Request::set_pass()` behavior to `false`.
  void set_ttl(uint32_t ttl);

  /// Builder-style equivalent of
  /// `Request::set_stale_while_revalidate()`.
  Request with_stale_while_revalidate(uint32_t swr);

  /// Override the caching behavior of this request to use the given
  /// `stale-while-revalidate` time, in seconds.
  ///
  /// # Overrides
  ///
  /// This overrides the behavior specified in the response headers, and sets
  /// the `Request::set_pass()` behavior to `false`.
  void set_stale_while_revalidate(uint32_t swr);

  /// Builder-style equivalent of `Request::set_pci()`.
  Request with_pci(bool pci);

  /// Override the caching behavior of this request to enable or disable
  /// PCI/HIPAA-compliant non-volatile caching.
  ///
  /// By default, this is `false`, which means the request may not be
  /// PCI/HIPAA-compliant. Set it to `true` to enable compliant caching.
  ///
  /// See the [Fastly PCI-Compliant Caching and Delivery
  /// documentation](https://docs.fastly.com/products/pci-compliant-caching-and-delivery)
  /// for details.
  ///
  /// # Overrides
  ///
  /// This sets the `Request::set_pass()` behavior to `false`.
  void set_pci(bool pci);

  /// Builder-style equivalent of
  /// `Request::set_surrogate_key()`.
  fastly::expected<Request> with_surrogate_key(std::string_view sk);

  /// Override the caching behavior of this request to include the given
  /// surrogate key(s), provided as a header value.
  ///
  /// The header value can contain more than one surrogate key, separated by
  /// spaces.
  ///
  /// Surrogate keys must contain only printable ASCII characters (those
  /// between `0x21` and `0x7E`, inclusive). Any invalid keys will be ignored.
  ///
  /// See the [Fastly surrogate keys
  /// guide](https://docs.fastly.com/en/guides/purging-api-cache-with-surrogate-keys)
  /// for details.
  ///
  /// # Overrides
  ///
  /// This sets the `Request::set_pass()` behavior to `false`, and
  /// extends (but does not replace) any `Surrogate-Key` response headers from
  /// the backend.
  fastly::expected<void> set_surrogate_key(std::string_view sk);

  std::optional<std::string> get_client_ip_addr();
  std::optional<std::string> get_server_ip_addr();

  // TODO(@zkat): needs iterator
  // std::optional<HeaderNameIter> get_original_header_names();

  // std::optional<uint32_t> get_original_header_count();

  /// Returns whether the request was tagged as contributing to a DDoS attack
  ///
  /// Returns `std::nullopt` if this is not the client request.
  std::optional<bool> get_client_ddos_detected();

  // std::optional<std::vector<uint8_t>> get_tls_client_hello();
  // std::optional<std::array<uint8_t, 16>> get_tls_ja3_md5();
  // std::optional<std::string> get_tls_ja4();
  // std::optional<std::string> get_tls_raw_client_certificate();
  // std::optional<std::vector<uint8_t>>
  // get_tls_raw_client_certificate_bytes();
  // // TODO(@zkat): needs additional type
  // // std::optional<ClientCertVerifyResult>
  // get_tls_client_cert_verify_result(); std::optional<std::string>
  // get_tls_cipher_openssl_name(); std::optional<std::vector<uint8_t>>
  // get_tls_cipher_openssl_name_bytes(); std::optional<std::vector<uint8_t>>
  // get_tls_protocol_bytes();

  /// Set whether a `gzip`-encoded response to this request will be
  /// automatically decompressed.
  ///
  /// Enabling this will set the `Accept-Encoding` header before the request
  /// is sent, regardless of the original value in the request, to ensure that
  /// any values originally sent by a browser or other client get replaced
  /// with `gzip`, so that the backend will not try sending unsupported
  /// compression algorithms.
  ///
  /// If the response to this request is `gzip`-encoded, it will be presented
  /// in decompressed form, and the `Content-Encoding` and `Content-Length`
  /// headers will be removed.
  void set_auto_decompress_gzip(bool gzip);

  /// Builder-style equivalent of
  /// `Request::set_auto_decompress_gzip()`.
  Request with_auto_decompress_gzip(bool gzip);

  // TODO(@zkat): needs enum
  // void set_framing_headers_mode(FramingHeadersMode mode);
  // Request *set_framing_headers_mode(FramingHeadersMode mode);

  /// Returns whether or not the client request had a `Fastly-Key` header
  /// which is valid for purging content for the service.
  ///
  /// This function ignores the current value of any `Fastly-Key` header for
  /// this request.
  bool fastly_key_is_valid();

  // void handoff_websocket(fastly::backend::Backend backend);
  // void handoff_fanout(fastly::backend::Backend backend);
  // Request *on_behalf_of(std::string_view service);

  /// Set the cache key to be used when attempting to satisfy this request
  /// from a cached response.
  void set_cache_key(std::string_view key);

  /// Set the cache key to be used when attempting to satisfy this request
  /// from a cached response.
  void set_cache_key(std::vector<uint8_t> key);

  /// Builder-style equivalent of `Request::set_cache_key()`.
  Request with_cache_key(std::string_view key);

  /// Builder-style equivalent of `Request::set_cache_key()`.
  Request with_cache_key(std::vector<uint8_t> key);

  /// Gets whether the request is potentially cacheable.
  bool is_cacheable();

private:
  Request(rust::Box<fastly::sys::http::Request> r) : req(std::move(r)) {};
  rust::Box<fastly::sys::http::Request> req;
};

} // namespace fastly::http

namespace fastly {
using fastly::http::Request;
}

#endif
