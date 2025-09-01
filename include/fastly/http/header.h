#ifndef FASTLY_HTTP_HEADER_H
#define FASTLY_HTTP_HEADER_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <fastly/detail/rust_iterator_range.h>
#include <fastly/sdk-sys.h>

namespace fastly::http {

/// Represents a single HTTP header value.
class HeaderValue {
public:
  HeaderValue(std::string value, bool is_sensitive = false)
      : value_(std::move(value)), is_sensitive_(is_sensitive) {}

  std::optional<std::string_view> string() const {
    if (has_non_visible_characters()) {
      return std::nullopt;
    }
    return value_;
  }

  std::span<const uint8_t> bytes() const {
    return std::span<const uint8_t>(
        reinterpret_cast<const uint8_t *>(value_.data()), value_.size());
  }

  bool is_sensitive() const { return is_sensitive_; }

  bool set_sensitive(bool sensitive) {
    is_sensitive_ = sensitive;
    return is_sensitive_;
  }

  bool has_non_visible_characters() const {
    return std::any_of(value_.begin(), value_.end(), [](char c) {
      return c < 32 || c > 126; // ASCII range for visible characters
    });
  }

private:
  std::string value_;
  bool is_sensitive_;
};

/// Iterates over multiple values for an individual header.
class HeaderValuesRange
    : public fastly::detail::RustIteratorRange<
          HeaderValuesRange, fastly::sys::http::HeaderValuesIter> {
public:
  using fastly::detail::RustIteratorRange<
      HeaderValuesRange,
      fastly::sys::http::HeaderValuesIter>::RustIteratorRange;

  /// Gets the next value.
  std::optional<HeaderValue> next() {
    std::vector<uint8_t> value;
    bool is_sensitive{false};
    if (this->iter_->next(value, is_sensitive)) {
      return HeaderValue(std::string(value.begin(), value.end()), is_sensitive);
    }
    return std::nullopt;
  }
};

/// Iterates over all headers in a request or response.
class HeadersRange
    : public fastly::detail::RustIteratorRange<HeadersRange,
                                               fastly::sys::http::HeadersIter> {
public:
  using fastly::detail::RustIteratorRange<
      HeadersRange, fastly::sys::http::HeadersIter>::RustIteratorRange;

  /// Gets the next header name and value.
  std::optional<std::pair<std::string, HeaderValue>> next() {
    std::string name;
    std::vector<uint8_t> value;
    bool is_sensitive{false};

    if (this->iter_->next(name, value, is_sensitive)) {
      return std::make_pair(
          name,
          HeaderValue(std::string(value.begin(), value.end()), is_sensitive));
    }
    return std::nullopt;
  }
};

/// Iterates over all header names in a request or response.
class HeaderNamesRange
    : public fastly::detail::RustIteratorRange<
          HeaderNamesRange, fastly::sys::http::HeaderNamesIter> {
public:
  using fastly::detail::RustIteratorRange<
      HeaderNamesRange, fastly::sys::http::HeaderNamesIter>::RustIteratorRange;

  /// Gets the next header name.
  std::optional<std::string> next() {
    std::string name;
    if (this->iter_->next(name)) {
      return name;
    }
    return std::nullopt;
  }
};

/// Iterates over all original header names in a request or response.
class OriginalHeaderNamesRange
    : public fastly::detail::RustIteratorRange<
          OriginalHeaderNamesRange,
          fastly::sys::http::OriginalHeaderNamesIter> {
public:
  OriginalHeaderNamesRange(
      rust::Box<fastly::sys::http::OriginalHeaderNamesIter> iter)
      : fastly::detail::RustIteratorRange<
            OriginalHeaderNamesRange,
            fastly::sys::http::OriginalHeaderNamesIter>(std::move(iter)) {}

  /// Gets the next header name.
  std::optional<std::string> next() {
    std::string name;
    if (this->iter_->next(name)) {
      return name;
    }
    return std::nullopt;
  }
};

} // namespace fastly::http

#endif
