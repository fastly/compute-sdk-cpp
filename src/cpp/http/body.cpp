#include "body.h"
#include "expected.h"
#include "sdk-sys.h"

namespace fastly::http {

int Body::underflow() {
  if (!this->in_avail()) {
    size_t read{this->read(reinterpret_cast<uint8_t *>(this->gbuf.data()),
                           this->gbuf.max_size())
                    .or_else([](fastly::FastlyError err) {
                      std::cerr << err.error_msg() << std::endl;
                      std::abort();
                    })
                    .value()};
    if (!read) {
      return traits_type::eof();
    }
    this->setg(this->gbuf.data(), this->gbuf.data(), this->gbuf.data() + read);
  }
  return *this->gptr();
}

int Body::overflow(int_type val) {
  auto const eof{traits_type::eof()};
  auto len{this->pptr() - this->pbase()};
  if (len) {
    auto pos{0};
    while (pos < len) {
      size_t written{
          this->write(reinterpret_cast<uint8_t *>(this->pbuf.data() + pos),
                      len - pos)
              .or_else([](fastly::FastlyError err) {
                std::cerr << err.error_msg() << std::endl;
                std::abort();
              })
              .value()};
      if (!written) {
        return traits_type::eof();
      } else {
        pos += written;
      }
    }
  }
  this->setp(this->pbuf.data(), this->pbuf.data() + this->pbuf.max_size());
  if (!traits_type::eq_int_type(val, eof)) {
    this->sputc(val);
  }
  return traits_type::not_eof(val);
}

int Body::sync() {
  auto result{this->overflow(traits_type::eof())};
  return traits_type::eq_int_type(result, traits_type::eof()) ? -1 : 0;
}

void Body::append(Body other) {
  other.flush();
  return this->bod->append(std::move(other.bod));
}

fastly::expected<std::size_t> Body::read(uint8_t *buf, std::size_t bufsize) {
  rust::Slice<uint8_t> slice{buf, bufsize};
  fastly::sys::error::FastlyError *err;
  auto ret{this->bod->read(slice, err)};
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return ret;
  }
}

fastly::expected<std::size_t> Body::write(uint8_t *buf, std::size_t bufsize) {
  rust::Slice<const uint8_t> slice{buf, bufsize};
  fastly::sys::error::FastlyError *err;
  auto ret{this->bod->write(slice, err)};
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return ret;
  }
}

// TODO(@zkat): these need other types
// Prefix get_prefix(uint32_t prefix_len);
// PrefixString get_prefix_string(uint32_t prefix_len);

fastly::expected<void> Body::append_trailer(std::string_view header_name,
                                            std::string_view header_value) {
  fastly::sys::error::FastlyError *err;
  this->bod->append_trailer(static_cast<std::string>(header_name),
                            static_cast<std::string>(header_value), err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<void>();
  }
}

// TODO(@zkat): this needs a HeaderMap wrapper.
// HeaderMap get_trailers();

int StreamingBody::overflow(int_type val) {
  auto const eof{traits_type::eof()};
  auto len{this->pptr() - this->pbase()};
  if (len) {
    auto pos{0};
    while (pos < len) {
      size_t written{
          this->write(reinterpret_cast<uint8_t *>(this->pbuf.data() + pos),
                      len - pos)
              .or_else([](fastly::FastlyError err) {
                std::cerr << err.error_msg() << std::endl;
                std::abort();
              })
              .value()};
      if (!written) {
        return traits_type::eof();
      } else {
        pos += written;
      }
    }
  }
  this->setp(this->pbuf.data(), this->pbuf.data() + this->pbuf.max_size());
  if (!traits_type::eq_int_type(val, eof)) {
    this->sputc(val);
  }
  return traits_type::not_eof(val);
}

int StreamingBody::sync() {
  auto result{this->overflow(traits_type::eof())};
  return traits_type::eq_int_type(result, traits_type::eof()) ? -1 : 0;
}

fastly::expected<void> StreamingBody::finish() {
  this->flush();
  fastly::sys::error::FastlyError *err;
  fastly::sys::http::m_http_streaming_body_finish(std::move(this->bod), err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<void>();
  }
}

void StreamingBody::append(Body other) {
  return this->bod->append(std::move(other.bod));
}

fastly::expected<std::size_t> StreamingBody::write(uint8_t *buf,
                                                   std::size_t bufsize) {
  rust::Slice<const uint8_t> slice{buf, bufsize};
  fastly::sys::error::FastlyError *err;
  auto ret{this->bod->write(slice, err)};
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return ret;
  }
}

fastly::expected<void>
StreamingBody::append_trailer(std::string_view header_name,
                              std::string_view header_value) {
  fastly::sys::error::FastlyError *err;
  this->bod->append_trailer(static_cast<std::string>(header_name),
                            static_cast<std::string>(header_value), err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<void>();
  }
}

} // namespace fastly::http
