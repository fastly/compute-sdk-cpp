#ifndef FASTLY_DETAIL_RUST_ITERATOR_RANGE_H
#define FASTLY_DETAIL_RUST_ITERATOR_RANGE_H

#include <fastly/sdk-sys.h>
#include <optional>

namespace fastly::detail {
// A CRTP class that adapts a Rust-style iterator to a C++ range.
// CppRng is the C++ class that implements the iterator interface,
// and RustIt is the Rust iterator type that we are wrapping.
// The C++ class should implement a `next()` method that returns an
// `std::optional<value_type>`, where `value_type` is the type of the values
// being iterated over.
template <class CppRng, class RustIt> class RustIteratorRange {
public:
  RustIteratorRange(rust::Box<RustIt> iter) : iter_(std::move(iter)) {}

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

  iterator begin() { return iterator(this); }
  iterator end() { return iterator(nullptr); }

protected:
  rust::Box<RustIt> iter_;
};
} // namespace fastly::detail

#endif