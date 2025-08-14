#ifndef FASTLY_HTTP_STATUS_CODE_H
#define FASTLY_HTTP_STATUS_CODE_H

#include <fastly/error.h>
#include <fastly/expected.h>
#include <fastly/sdk-sys.h>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>

namespace fastly::http {

class StatusCode {
public:
  StatusCode() = default;

  /// Creates a new StatusCode. Panics if the code is out of range.
  constexpr StatusCode(uint16_t code) : value(code) {
    if (code < 100 || code >= 1000) {
      std::cerr << "Invalid StatusCode value.";
      std::abort();
    }
  }

  /// 100 Continue
  /// [[RFC9110,
  /// Section 15.2.1](https://datatracker.ietf.org/doc/html/rfc9110#section-15.2.1)]
  static const StatusCode CONTINUE;
  /// 101 Switching Protocols
  /// [[RFC9110,
  /// Section 15.2.2](https://datatracker.ietf.org/doc/html/rfc9110#section-15.2.2)]
  static const StatusCode SWITCHING_PROTOCOLS;
  /// 102 Processing
  /// [[RFC2518,
  /// Section 10.1](https://datatracker.ietf.org/doc/html/rfc2518#section-10.1)]
  static const StatusCode PROCESSING;
  /// 200 OK
  /// [[RFC9110,
  /// Section 15.3.1](https://datatracker.ietf.org/doc/html/rfc9110#section-15.3.1)]
  static const StatusCode OK;
  /// 201 Created
  /// [[RFC9110,
  /// Section 15.3.2](https://datatracker.ietf.org/doc/html/rfc9110#section-15.3.2)]
  static const StatusCode CREATED;
  /// 202 Accepted
  /// [[RFC9110,
  /// Section 15.3.3](https://datatracker.ietf.org/doc/html/rfc9110#section-15.3.3)]
  static const StatusCode ACCEPTED;
  /// 203 Non-Authoritative Information
  /// [[RFC9110,
  /// Section 15.3.4](https://datatracker.ietf.org/doc/html/rfc9110#section-15.3.4)]
  static const StatusCode NON_AUTHORITATIVE_INFORMATION;
  /// 204 No Content
  /// [[RFC9110,
  /// Section 15.3.5](https://datatracker.ietf.org/doc/html/rfc9110#section-15.3.5)]
  static const StatusCode NO_CONTENT;
  /// 205 Reset Content
  /// [[RFC9110,
  /// Section 15.3.6](https://datatracker.ietf.org/doc/html/rfc9110#section-15.3.6)]
  static const StatusCode RESET_CONTENT;
  /// 206 Partial Content
  /// [[RFC9110,
  /// Section 15.3.7](https://datatracker.ietf.org/doc/html/rfc9110#section-15.3.7)]
  static const StatusCode PARTIAL_CONTENT;
  /// 207 Multi-Status
  /// [[RFC4918,
  /// Section 11.1](https://datatracker.ietf.org/doc/html/rfc4918#section-11.1)]
  static const StatusCode MULTI_STATUS;
  /// 208 Already Reported
  /// [[RFC5842,
  /// Section 7.1](https://datatracker.ietf.org/doc/html/rfc5842#section-7.1)]
  static const StatusCode ALREADY_REPORTED;
  /// 226 IM Used
  /// [[RFC3229,
  /// Section 10.4.1](https://datatracker.ietf.org/doc/html/rfc3229#section-10.4.1)]
  static const StatusCode IM_USED;
  /// 300 Multiple Choices
  /// [[RFC9110,
  /// Section 15.4.1](https://datatracker.ietf.org/doc/html/rfc9110#section-15.4.1)]
  static const StatusCode MULTIPLE_CHOICES;
  /// 301 Moved Permanently
  /// [[RFC9110,
  /// Section 15.4.2](https://datatracker.ietf.org/doc/html/rfc9110#section-15.4.2)]
  static const StatusCode MOVED_PERMANENTLY;
  /// 302 Found
  /// [[RFC9110,
  /// Section 15.4.3](https://datatracker.ietf.org/doc/html/rfc9110#section-15.4.3)]
  static const StatusCode FOUND;
  /// 303 See Other
  /// [[RFC9110,
  /// Section 15.4.4](https://datatracker.ietf.org/doc/html/rfc9110#section-15.4.4)]
  static const StatusCode SEE_OTHER;
  /// 304 Not Modified
  /// [[RFC9110,
  /// Section 15.4.5](https://datatracker.ietf.org/doc/html/rfc9110#section-15.4.5)]
  static const StatusCode NOT_MODIFIED;
  /// 305 Use Proxy
  /// [[RFC9110,
  /// Section 15.4.6](https://datatracker.ietf.org/doc/html/rfc9110#section-15.4.6)]
  static const StatusCode USE_PROXY;
  /// 307 Temporary Redirect
  /// [[RFC9110,
  /// Section 15.4.7](https://datatracker.ietf.org/doc/html/rfc9110#section-15.4.7)]
  static const StatusCode TEMPORARY_REDIRECT;
  /// 308 Permanent Redirect
  /// [[RFC9110,
  /// Section 15.4.8](https://datatracker.ietf.org/doc/html/rfc9110#section-15.4.8)]
  static const StatusCode PERMANENT_REDIRECT;
  /// 400 Bad Request
  /// [[RFC9110,
  /// Section 15.5.1](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.1)]
  static const StatusCode BAD_REQUEST;
  /// 401 Unauthorized
  /// [[RFC9110,
  /// Section 15.5.2](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.2)]
  static const StatusCode UNAUTHORIZED;
  /// 402 Payment Required
  /// [[RFC9110,
  /// Section 15.5.3](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.3)]
  static const StatusCode PAYMENT_REQUIRED;
  /// 403 Forbidden
  /// [[RFC9110,
  /// Section 15.5.4](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.4)]
  static const StatusCode FORBIDDEN;
  /// 404 Not Found
  /// [[RFC9110,
  /// Section 15.5.5](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.5)]
  static const StatusCode NOT_FOUND;
  /// 405 Method Not Allowed
  /// [[RFC9110,
  /// Section 15.5.6](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.6)]
  static const StatusCode METHOD_NOT_ALLOWED;
  /// 406 Not Acceptable
  /// [[RFC9110,
  /// Section 15.5.7](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.7)]
  static const StatusCode NOT_ACCEPTABLE;
  /// 407 Proxy Authentication Required
  /// [[RFC9110,
  /// Section 15.5.8](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.8)]
  static const StatusCode PROXY_AUTHENTICATION_REQUIRED;
  /// 408 Request Timeout
  /// [[RFC9110,
  /// Section 15.5.9](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.9)]
  static const StatusCode REQUEST_TIMEOUT;
  /// 409 Conflict
  /// [[RFC9110,
  /// Section 15.5.10](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.10)]
  static const StatusCode CONFLICT;
  /// 410 Gone
  /// [[RFC9110,
  /// Section 15.5.11](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.11)]
  static const StatusCode GONE;
  /// 411 Length Required
  /// [[RFC9110,
  /// Section 15.5.12](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.12)]
  static const StatusCode LENGTH_REQUIRED;
  /// 412 Precondition Failed
  /// [[RFC9110,
  /// Section 15.5.13](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.13)]
  static const StatusCode PRECONDITION_FAILED;
  /// 413 Payload Too Large
  /// [[RFC9110,
  /// Section 15.5.14](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.14)]
  static const StatusCode PAYLOAD_TOO_LARGE;
  /// 414 URI Too Long
  /// [[RFC9110,
  /// Section 15.5.15](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.15)]
  static const StatusCode URI_TOO_LONG;
  /// 415 Unsupported Media Type
  /// [[RFC9110,
  /// Section 15.5.16](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.16)]
  static const StatusCode UNSUPPORTED_MEDIA_TYPE;
  /// 416 Range Not Satisfiable
  /// [[RFC9110,
  /// Section 15.5.17](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.17)]
  static const StatusCode RANGE_NOT_SATISFIABLE;
  /// 417 Expectation Failed
  /// [[RFC9110,
  /// Section 15.5.18](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.18)]
  static const StatusCode EXPECTATION_FAILED;
  /// 418 I'm a teapot
  /// [curiously not registered by IANA but [RFC2324,
  /// Section 2.3.2](https://datatracker.ietf.org/doc/html/rfc2324#section-2.3.2)]
  static const StatusCode IM_A_TEAPOT;
  /// 421 Misdirected Request
  /// [[RFC9110,
  /// Section 15.5.20](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.20)]
  static const StatusCode MISDIRECTED_REQUEST;
  /// 422 Unprocessable Entity
  /// [[RFC9110,
  /// Section 15.5.21](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.21)]
  static const StatusCode UNPROCESSABLE_ENTITY;
  /// 423 Locked
  /// [[RFC4918,
  /// Section 11.3](https://datatracker.ietf.org/doc/html/rfc4918#section-11.3)]
  static const StatusCode LOCKED;
  /// 424 Failed Dependency
  /// [[RFC4918,
  /// Section 11.4](https://tools.ietf.org/html/rfc4918#section-11.4)]
  static const StatusCode FAILED_DEPENDENCY;
  /// 425 Too early
  /// [[RFC8470, Section 5.2](https://httpwg.org/specs/rfc8470.html#status)]
  static const StatusCode TOO_EARLY;
  /// 426 Upgrade Required
  /// [[RFC9110,
  /// Section 15.5.22](https://datatracker.ietf.org/doc/html/rfc9110#section-15.5.22)]
  static const StatusCode UPGRADE_REQUIRED;
  /// 428 Precondition Required
  /// [[RFC6585, Section
  /// 3](https://datatracker.ietf.org/doc/html/rfc6585#section-3)]
  static const StatusCode PRECONDITION_REQUIRED;
  /// 429 Too Many Requests
  /// [[RFC6585, Section
  /// 4](https://datatracker.ietf.org/doc/html/rfc6585#section-4)]
  static const StatusCode TOO_MANY_REQUESTS;
  /// 431 Request Header Fields Too Large
  /// [[RFC6585, Section
  /// 5](https://datatracker.ietf.org/doc/html/rfc6585#section-5)]
  static const StatusCode REQUEST_HEADER_FIELDS_TOO_LARGE;
  /// 451 Unavailable For Legal Reasons
  /// [[RFC7725, Section 3](https://tools.ietf.org/html/rfc7725#section-3)]
  static const StatusCode UNAVAILABLE_FOR_LEGAL_REASONS;
  /// 500 Internal Server Error
  /// [[RFC9110,
  /// Section 15.6.1](https://datatracker.ietf.org/doc/html/rfc9110#section-15.6.1)]
  static const StatusCode INTERNAL_SERVER_ERROR;
  /// 501 Not Implemented
  /// [[RFC9110,
  /// Section 15.6.2](https://datatracker.ietf.org/doc/html/rfc9110#section-15.6.2)]
  static const StatusCode NOT_IMPLEMENTED;
  /// 502 Bad Gateway
  /// [[RFC9110,
  /// Section 15.6.3](https://datatracker.ietf.org/doc/html/rfc9110#section-15.6.3)]
  static const StatusCode BAD_GATEWAY;
  /// 503 Service Unavailable
  /// [[RFC9110,
  /// Section 15.6.4](https://datatracker.ietf.org/doc/html/rfc9110#section-15.6.4)]
  static const StatusCode SERVICE_UNAVAILABLE;
  /// 504 Gateway Timeout
  /// [[RFC9110,
  /// Section 15.6.5](https://datatracker.ietf.org/doc/html/rfc9110#section-15.6.5)]
  static const StatusCode GATEWAY_TIMEOUT;
  /// 505 HTTP Version Not Supported
  /// [[RFC9110,
  /// Section 15.6.6](https://datatracker.ietf.org/doc/html/rfc9110#section-15.6.6)]
  static const StatusCode HTTP_VERSION_NOT_SUPPORTED;
  /// 506 Variant Also Negotiates
  /// [[RFC2295,
  /// Section 8.1](https://datatracker.ietf.org/doc/html/rfc2295#section-8.1)]
  static const StatusCode VARIANT_ALSO_NEGOTIATES;
  /// 507 Insufficient Storage
  /// [[RFC4918,
  /// Section 11.5](https://datatracker.ietf.org/doc/html/rfc4918#section-11.5)]
  static const StatusCode INSUFFICIENT_STORAGE;
  /// 508 Loop Detected
  /// [[RFC5842,
  /// Section 7.2](https://datatracker.ietf.org/doc/html/rfc5842#section-7.2)]
  static const StatusCode LOOP_DETECTED;
  /// 510 Not Extended
  /// [[RFC2774, Section
  /// 7](https://datatracker.ietf.org/doc/html/rfc2774#section-7)]
  static const StatusCode NOT_EXTENDED;
  /// 511 Network Authentication Required
  /// [[RFC6585, Section
  /// 6](https://datatracker.ietf.org/doc/html/rfc6585#section-6)]
  static const StatusCode NETWORK_AUTHENTICATION_REQUIRED;

  constexpr bool operator==(StatusCode a) const { return value == a.value; }
  constexpr bool operator!=(StatusCode a) const { return value != a.value; }
  explicit operator bool() const = delete;

  /// Converts a uint16_t to a status code, returning it wrapped in an
  /// `std::optional`.
  ///
  /// The method validates the correctness of the supplied uint16_t. It must be
  /// greater or equal to 100 and less than 1000, or this method will return
  /// `std::nullopt`.
  std::optional<StatusCode> from_code(uint16_t code);

  /// Returns the `uint16_t` corresponding to this `StatusCode`.
  uint16_t as_code();

  /// Get the standardised `reason-phrase` for this status code.
  ///
  /// This is mostly here for servers writing responses, but could potentially
  /// have application at other times.
  ///
  /// The reason phrase is defined as being exclusively for human readers. You
  /// should avoid deriving any meaning from it at all costs.
  ///
  /// Bear in mind also that in HTTP/2.0 and HTTP/3.0 the reason phrase is
  /// abolished from transmission, and so this canonical reason phrase really is
  /// the only reason phrase you’ll find.
  ///
  /// # Example
  ///
  /// ```cpp
  /// auto status{fastly::http::StatusCode::OK};
  /// assert(status.canonical_reason() == std::optional("OK"s)));
  /// ```
  tl::expected<std::optional<std::string>, fastly::FastlyError>
  canonical_reason();

  /// Check if status is within 100-199.
  bool is_informational();

  /// Check if status is within 200-299.
  bool is_success();

  /// Check if status is within 300-399.
  bool is_redirection();

  /// Check if status is within 400-499.
  bool is_client_error();

  /// Check if status is within 500-599.
  bool is_server_error();

private:
  uint16_t value;
};

} // namespace fastly::http

#endif
