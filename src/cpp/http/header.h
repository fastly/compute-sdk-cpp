#ifndef FASTLY_HTTP_HEADER_H
#define FASTLY_HTTP_HEADER_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../sdk-sys.h"
#include "../util.h"

namespace fastly::detail {
// A CRTP class that adapts a Rust-style iterator to a C++ range.
// CppRng is the C++ class that implements the iterator interface,
// and RustIt is the Rust iterator type that we are wrapping.
// The C++ class should implement a `next()` method that returns an
// `std::optional<value_type>`, where `value_type` is the type of the values
// being iterated over.
template <class CppRng, class RustIt> class RustIteratorRange {
public:
  class iterator {
  public:
    using iterator_category = std::input_iterator_tag;
    // next() should return an optional<value_type>, so we extract the value
    // type from that
    using value_type =
        typename decltype(std::declval<CppRng>().next())::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;

    iterator() : range_(nullptr) {}
    explicit iterator(RustIteratorRange *range) : range_(range) {
      if (range) {
        ++(*this); // Initialize the first value
      }
    }

    iterator &operator++() {
      if (!range_) {
        return *this; // No range to iterate over
      }

      auto val = static_cast<CppRng *>(range_)->next();
      if (val) {
        cache_.emplace(std::move(*val));
      } else {
        range_ = nullptr; // Signal end of iteration
      }
      return *this;
    }

    iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    value_type &operator*() { return *cache_; }

    bool operator==(const iterator &other) const {
      return range_ == other.range_;
    }

    bool operator!=(const iterator &other) const { return !(*this == other); }

  private:
    RustIteratorRange *range_;
    std::optional<value_type> cache_;
  };

  RustIteratorRange(rust::Box<RustIt> iter) : iter_(std::move(iter)) {}

  iterator begin() { return iterator(this); }

  iterator end() { return iterator(nullptr); }

protected:
  rust::Box<RustIt> iter_;
};
} // namespace fastly::detail

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
  HeaderValuesRange(rust::Box<fastly::sys::http::HeaderValuesIter> iter)
      : fastly::detail::RustIteratorRange<HeaderValuesRange,
                                          fastly::sys::http::HeaderValuesIter>(
            std::move(iter)) {}

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
  HeadersRange(rust::Box<fastly::sys::http::HeadersIter> iter)
      : fastly::detail::RustIteratorRange<HeadersRange,
                                          fastly::sys::http::HeadersIter>(
            std::move(iter)) {}

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
  HeaderNamesRange(rust::Box<fastly::sys::http::HeaderNamesIter> iter)
      : fastly::detail::RustIteratorRange<HeaderNamesRange,
                                          fastly::sys::http::HeaderNamesIter>(
            std::move(iter)) {}

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
