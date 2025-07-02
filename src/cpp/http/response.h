#ifndef FASTLY_HTTP_RESPONSE_H
#define FASTLY_HTTP_RESPONSE_H

#include "../sdk-sys.h"
#include "backend.h"
#include "body.h"
#include "header.h"
#include "request.h"
#include "status_code.h"
#include <chrono>
#include <string>
#include <utility>
#include <vector>

namespace fastly::http {

class Body;
class StreamingBody;
class Response;
class Request;
namespace request {
class PendingRequest;
std::pair<Response, std::vector<PendingRequest>>
select(std::vector<PendingRequest> &reqs);
} // namespace request

/// An HTTP response, including body, headers, and status code.
///
/// # Sending to the client
///
/// Each execution of a Compute program may send a single response back to the
/// client:
///
/// - `Response::send_to_client()`
/// - `Response::stream_to_client()`
///
/// If no response is explicitly sent by the program, a default `200 OK`
/// response is sent.
///
/// # Creation and conversion
///
/// Responses can be created programmatically:
///
/// - `Response::new()`
/// - `Response::from_body()`
/// - `Response::from_status()`
///
/// Responses are also returned from backend requests:
///
/// - `Request::send()`
/// - `Request::send_async()`
/// - `Request::send_async_streaming()`
///
/// # Builder-style methods
///
/// `Response` can be used as a
/// [builder](https://doc.rust-lang.org/1.0.0/style/ownership/builders.html),
/// allowing responses to be constructed and used through method chaining.
/// Methods with the `with_` name prefix, such as `Response::with_header()`,
/// return `std::move(*this)` to allow chaining. The builder style is typically
/// most useful when constructing and using a response in a single expression.
/// For example:
///
/// ```cpp
/// Response()
///     .with_header("my-header"s, "hello!"s)
///     .with_header("my-other-header"s, "Здравствуйте!"s)
///     .send_to_client();
/// ```
///
/// # Setter methods
///
/// Setter methods, such as `Response::set_header()`, are prefixed by `set_`,
/// and can be used interchangeably with the builder-style methods, allowing you
/// to mix and match styles based on what is most convenient for your program.
/// Setter methods tend to work better than builder-style methods when
/// constructing a value involves conditional branches or loops. For example:
///
/// ```cpp
/// auto resp{Response().with_header("my-header"s, "hello!"s)};
/// if (needs_translation) {
///     resp.set_header("my-other-header"s, "Здравствуйте!"s);
/// }
/// resp.send_to_client();
/// ```
class Response {
  friend Request;
  friend request::PendingRequest;
  friend std::pair<Response, std::vector<request::PendingRequest>>
  request::select(std::vector<request::PendingRequest> &reqs);

public:
  /// Create a new `Response`.
  ///
  /// The new response is created with status code `200 OK`, no headers, and an
  /// empty body.
  Response();
  // TODO(@zkat): Make this a "friend"?
  /// Return whether the response is from a backend request.
  bool is_from_backend();

  /// Make a new response with the same headers, status, and version of this
  /// response, but no body.
  ///
  /// If you also need to clone the response body, use
  /// `Response::clone_with_body()`.
  Response clone_without_body();

  /// Clone this response by reading in its body, and then writing the same body
  /// to the original and the cloned response.
  ///
  /// This method requires mutable access to this response because reading from
  /// and writing to the body can involve an HTTP connection.
  Response clone_with_body();

  /// Create a new `Response` with the given value as the body.
  static Response from_body(Body body);

  /// Create a new response with the given status code.
  static Response from_status(StatusCode status);

  /// Create a 303 See Other response with the given value as the `Location`
  /// header.
  ///
  /// # Examples
  ///
  /// ```cpp
  /// auto resp{Response::see_other("https://www.fastly.com"s)};
  /// assert(resp.get_status() == StatusCode::SEE_OTHER);
  /// assert(resp.get_header("Location"s).value(), "https://www.fastly.com"s);
  /// ```
  static Response see_other(std::string destination);

  /// Create a 308 Permanent Redirect response with the given value as the
  /// `Location` header.
  ///
  /// # Examples
  ///
  /// ```cpp
  /// auto resp{Response::redirect("https://www.fastly.com"s)};
  /// assert(resp.get_status() == StatusCode::PERMANENT_REDIRECT);
  /// assert(resp.get_header("Location"s).value(), "https://www.fastly.com"s);
  /// ```
  static Response redirect(std::string destination);

  /// Create a 307 Temporary Redirect response with the given value as the
  /// `Location` header.
  ///
  /// # Examples
  ///
  /// ```cpp
  /// auto resp{Response::temporary_redirect("https://www.fastly.com"s)};
  /// assert(resp.get_status() == StatusCode::TEMPORARY_REDIRECT);
  /// assert(resp.get_header("Location"s).value(), "https://www.fastly.com");
  /// ```
  static Response temporary_redirect(std::string destination);

  /// Builder-style equivalent of `Response::set_body()`.
  Response with_body(Body body);

  /// Returns `true` if this response has a body.
  bool has_body();

  /// Set the given value as the response's body.
  void set_body(Body body);

  /// Take and return the body from this response.
  ///
  /// After calling this method, this response will no longer have a body.
  Body take_body();

  /// Append another `Body` to the body of this response without reading or
  /// writing any body contents.
  ///
  /// If this response does not have a body, the appended body is set as the
  /// response's body.
  ///
  /// This method should be used when combining bodies that have not necessarily
  /// been read yet, such as a body returned from a backend response.
  ///
  /// # Examples
  ///
  /// ```cpp
  /// auto resp{Response::from_body("hello! backend says: "s)};
  /// auto backend_resp{
  ///   Request::get("https://example.com/"s).send("example_backend"s)
  /// };
  /// resp.append_body(backend_resp.into_body());
  /// resp.send_to_client();
  /// ```
  void append_body(Body body);

  /// Consume the response and return its body as a byte vector.
  std::vector<uint8_t> into_body_bytes();

  /// Consume the response and return its body as a string.
  std::string into_body_string();

  /// Consume the response and return its body.
  Body into_body();

  /// Builder-style equivalent of
  /// `Response::set_body_text_plain()`.
  Response with_body_text_plain(std::string body);

  /// Set the given string as the response's body with content type `text/plain;
  /// charset=UTF-8`.
  void set_body_text_plain(std::string body);

  /// Builder-style equivalent of
  /// `Response::set_body_text_html()`.
  Response with_body_text_html(std::string body);

  /// Set the given string as the response's body with content type `text/html;
  /// charset=UTF-8`.
  void set_body_text_html(std::string body);

  /// Take and return the body from this response as a string.
  ///
  /// After calling this method, this response will no longer have a body.
  std::string take_body_string();

  /// Builder-style equivalent of
  /// `Response::set_body_octet_stream()`.
  Response with_body_octet_stream(std::vector<uint8_t> body);

  /// Set the given bytes as the response's body with content type
  /// `application/octet-stream`.
  void set_body_octet_stream(std::vector<uint8_t> body);

  /// Take and return the body from this response as a vector of bytes.
  ///
  /// After calling this method, this response will no longer have a body.
  std::vector<uint8_t> take_body_bytes();

  /// Get the MIME type described by the response's
  /// [`Content-Type`](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Type)
  /// header, or `std::nullopt` if that header is absent or contains an invalid
  /// MIME type.
  std::optional<std::string> get_content_type();

  /// Builder-style equivalent of
  /// `Response::set_content_type()`.
  Response with_content_type(std::string mime);

  /// Set the MIME type described by the response's
  /// [`Content-Type`](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Type)
  /// header.
  ///
  /// Any existing `Content-Type` header values will be overwritten.
  void set_content_type(std::string mime);

  /// Get the value of the response's
  /// [`Content-Length`](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Length)
  /// header, if it exists.
  std::optional<size_t> get_content_length();

  /// Returns whether the given header name is present in the response.
  bool contains_header(std::string name);

  /// Builder-style equivalent of `Response::append_header()`.
  Response with_header(std::string name, std::string value);

  /// Builder-style equivalent of `Response::set_header()`.
  Response with_set_header(std::string name, std::string value);

  /// Get the value of a header as a string, or `std::nullopt` if the header is
  /// not present.
  ///
  /// If there are multiple values for the header, only one is returned, which
  /// may be any of the values. See
  /// `Response::get_header_all()`
  /// all of the values.
  // TODO(@zkat): do a proper HeaderValue situation here?
  std::optional<std::string> get_header(std::string name);

  /// Get an iterator of all the values of a header.
  HeaderValuesIter get_header_all(std::string name);

  // TODO(@zkat): sigh. IDK
  // ??? get_headers();
  // HeaderNamesIter get_header_names();

  /// Set a response header to the given value, discarding any previous values
  /// for the given header name.
  void set_header(std::string name, std::string value);

  /// Add a request header with given value.
  ///
  /// Unlike `Response::set_header()`, this does not discard existing values for
  /// the same header name.
  void append_header(std::string name, std::string value);

  /// Remove all request headers of the given name, and return one of the
  /// removed header values if any were present.
  std::optional<std::string> remove_header(std::string name);

  /// Builder-style equivalent of `Response::set_status()`.
  void set_status(StatusCode status);

  /// Set the HTTP status code of the response.
  ///
  /// # Examples
  ///
  /// Using the constants from `StatusCode`:
  ///
  /// ```cpp
  /// auto resp{fastly::Response::from_body("not found!"s)};
  /// resp.set_status(fastly::http::StatusCode::NOT_FOUND);
  /// resp.send_to_client();
  /// ```
  ///
  /// Using a `uint16_t`:
  ///
  /// ```cpp
  /// auto resp{fastly::Response::from_body("not found!"s)};
  /// resp.set_status(404);
  /// resp.send_to_client();
  /// ```
  Response with_status(StatusCode status);

  // TODO(@zkat): need Version enum
  // Response with_version(Version version);
  // Version get_version();
  // void set_version(Version version);

  // TODO(@zkat): needs enum
  // void set_framing_headers_mode(FramingHeadersMode mode);
  // Response set_framing_headers_mode(FramingHeadersMode mode);

  /// Get the name of the `Backend` this response came from, or `std::nullopt`
  /// if the response is synthetic.
  ///
  /// # Examples
  ///
  /// From a backend response:
  ///
  /// ```cpp
  /// auto
  /// backend_resp{Request::get("https://example.com/"s).send("example_backend"s)};
  /// assert(backend_resp.get_backend_name(),
  /// std::optional("example_backend"s));
  /// ```
  ///
  /// From a synthetic response:
  ///
  /// ```cpp
  /// Response synthetic_resp;
  /// assert(synthetic_resp.get_backend_name() == std::nullopt);
  /// ```
  std::optional<std::string> get_backend_name();

  /// Get the backend this response came from, or `std::nullopt` if the response
  /// is synthetic.
  ///
  /// # Examples
  ///
  /// From a backend response:
  ///
  /// ```cpp
  /// auto backend_resp{
  ///   Request::get("https://example.com/"s).send("example_backend"s)
  /// };
  /// assert(
  ///   backend_resp.get_backend() ==
  ///   std::optional(Backend::from_name("example_backend"s))
  /// );
  /// ```
  ///
  /// From a synthetic response:
  ///
  /// ```cpp
  /// Response synthetic_resp;
  /// assert(synthetic_resp.get_backend() == std::nullopt);
  /// ```
  std::optional<fastly::backend::Backend> get_backend();

  /// Get the address of the backend this response came from, or `std::nullopt`
  /// when the response is synthetic or cached.
  ///
  /// # Examples
  ///
  /// From a backend response:
  ///
  /// ```cpp
  /// auto backend_resp{Request::get("https://example.com/"s)
  ///     .with_pass(true)
  ///     .send("example_backend"s)};
  /// assert(
  ///    backend_resp.get_backend_addr() ==
  ///    std::optional("127.0.0.1:443"s));
  /// ```
  ///
  /// From a synthetic response:
  ///
  /// ```cpp
  /// Response synthetic_resp;
  /// assert(synthetic_resp.get_backend_addr() == std::nullopt);
  /// ```
  std::optional<std::string> get_backend_addr();

  /// Take and return the request this response came from, or `std::nullopt` if
  /// the response is synthetic.
  ///
  /// Note that the returned request will only have the headers and metadata of
  /// the original request, as the body is consumed when sending the request.
  ///
  /// # Examples
  ///
  /// From a backend response:
  ///
  /// ```cpp
  /// auto backend_resp{Request::post("https://example.com/"s)
  ///     .with_body("hello"s)
  ///     .send("example_backend"s)};
  /// auto backend_req{backend_resp.take_backend_request().value()};
  /// assert(backend_req.get_url() == "https://example.com/"s);
  /// assert(!backend_req.has_body());
  /// backend_req.with_body("goodbye"s).send("example_backend"s);
  /// ```
  ///
  /// From a synthetic response:
  ///
  /// ```cpp
  /// Response synthetic_resp;
  /// assert(synthetic_resp.take_backend_request() == std::nullopt);
  /// ```
  std::optional<Request> take_backend_request();

  /// Begin sending the response to the client.
  ///
  /// This method returns as soon as the response header begins sending to the
  /// client, and transmission of the response will continue in the background.
  ///
  /// Once this method is called, nothing else may be added to the response
  /// body. To stream additional data to a response body after it begins to
  /// send, use `Response::stream_to_client()`.
  ///
  /// # Panics
  ///
  /// This method panics if another response has already been sent to the client
  /// by this method, by `Response::stream_to_client()`.
  ///
  /// # Examples
  ///
  /// Sending a backend response without modification:
  ///
  /// ```cpp
  /// Request::get("https://example.com/"s).send("example_backend"s).send_to_client();
  /// ```
  ///
  /// Removing a header from a backend response before sending to the client:
  ///
  /// ```cpp
  /// auto
  /// backend_resp{Request::get("https://example.com/"s).send("example_backend"s)};
  /// backend_resp.remove_header("bad-header"s);
  /// backend_resp.send_to_client();
  /// ```
  ///
  /// Sending a synthetic response:
  ///
  /// ```cpp
  /// Response::from_body("hello, world!"s).send_to_client();
  /// ```
  void send_to_client();

  /// Begin sending the response to the client, and return a `StreamingBody`
  /// that can accept further data to send.
  ///
  /// The client connection must be closed when finished writing the response by
  /// calling `StreamingBody::finish()`.
  ///
  /// This method is most useful for programs that do some sort of processing or
  /// inspection of a potentially-large backend response body. Streaming allows
  /// the program to operate on small parts of the body rather than having to
  /// read it all into memory at once.
  ///
  /// This method returns as soon as the response header begins sending to the
  /// client, and transmission of the response will continue in the background.
  ///
  /// # Panics
  ///
  /// This method panics if another response has already been sent to the client
  /// by this method, by `Response::send_to_client()`.
  ///
  /// # Examples
  ///
  /// Count the number of lines in a UTF-8 backend response body while sending
  /// it to the client:
  ///
  /// ```cpp
  /// auto backend_resp{
  ///    Request::get("https://example.com/").send("example_backend")
  /// };
  ///
  /// // Take the body so we can iterate through its lines later
  /// auto backend_resp_body{backend_resp.take_body()};
  ///
  /// // Start sending the backend response to the client with a now-empty body
  /// auto client_body{backend_resp.stream_to_client()};
  ///
  /// size_t num_lines{0};
  /// std::string line;
  /// while (getline(backend_resp_body, line)) {
  ///   num_lines++;
  ///   client_body << line;
  /// }
  /// // Finish the streaming body to close the client connection.
  /// client_body.finish();
  ///
  /// std::cout
  ///   << "backend response body contained "s
  ///   << num_lines
  ///   << " lines"s
  ///   << std::endl;
  /// ```
  StreamingBody stream_to_client();

  /// Get the Time to Live (TTL) in the cache for this response, if it is
  /// cached.
  ///
  /// The TTL provides the duration of "freshness" for the cached response
  /// after it is inserted into the cache. If the response is stale,
  /// the TTL is 0 (i.e. this returns `std::optional(0)`.
  ///
  /// Returns `std::nullopt` if the response is not cached.
  std::optional<std::chrono::milliseconds> get_ttl();

  /// The current age of the response, if it is cached.
  ///
  /// Returns `std::nullopt` if the response is not cached.
  std::optional<std::chrono::milliseconds> get_age();

  /// The time for which the response can safely be used despite being
  /// considered stale, if it is cached.
  ///
  /// Returns `std::nullopt` if the response is not cached.
  std::optional<std::chrono::milliseconds> get_stale_while_revalidate();

private:
  Response(rust::Box<fastly::sys::http::Response> response)
      : res(std::move(response)) {};
  rust::Box<fastly::sys::http::Response> res;
};

} // namespace fastly::http

namespace fastly {
using fastly::http::Response;
}

#endif
