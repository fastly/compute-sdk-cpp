#ifndef FASTLY_HTTP_BODY_H
#define FASTLY_HTTP_BODY_H

#include <fastly/error.h>
#include <fastly/http/request.h>
#include <fastly/http/response.h>
#include <fastly/sdk-sys.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <streambuf>
#include <string>
#include <string_view>
#include <vector>

namespace fastly::http {

class Response;
class Request;
class StreamingBody;
namespace request {
class PendingRequest;
std::pair<fastly::expected<fastly::http::Response>, std::vector<PendingRequest>>
select(std::vector<PendingRequest> &reqs);
} // namespace request

/// An HTTP body that can be read from, written to, or appended to another body.
///
/// The most efficient ways to read from and write to the body are through the
/// [`iostreams`] implementations. If you want a non-panicking interface, you
/// can use `Body::read` and `Body::write` directly, instead.
///
/// Read and write operations to a [`Body`] are automatically buffered.
class Body : public std::iostream, public std::streambuf {

  friend StreamingBody;
  friend Response;
  friend Request;

protected:
  int underflow();
  int overflow(int_type val);
  int sync();

public:
  /// Get a new, empty HTTP body.
  Body()
      : std::iostream(this), bod(fastly::sys::http::m_static_http_body_new()) {
    this->setg(this->gbuf.data(), this->gbuf.data(), this->gbuf.data());
    this->setp(this->pbuf.data(), this->pbuf.data() + this->pbuf.max_size());
  };
  Body(Body &&old)
      : std::iostream(this), bod((old.sync(), std::move(old.bod))),
        pbuf(std::move(old.pbuf)) {
    auto gcurr{old.gptr() - old.eback()};
    auto gend{old.egptr() - old.eback()};
    this->gbuf = std::move(old.gbuf);
    this->setg(this->gbuf.data(), this->gbuf.data() + gcurr,
               this->gbuf.data() + gend);
    this->setp(this->pbuf.data(), this->pbuf.data() + this->pbuf.max_size());
  }
  Body(std::vector<uint8_t> body_vec) : Body() {
    if (!this->fill_from_vec(body_vec).map_error([](fastly::FastlyError err) {
          std::cerr << err.error_msg() << std::endl;
        })) {
      std::abort();
    }
  };
  Body(std::string body_str) : Body() { *this << body_str << std::flush; };
  Body(const char *body_str) : Body() { *this << body_str << std::flush; };
  Body(std::string_view body_str) : Body() { *this << body_str << std::flush; };

  /// Read bytes from the body, and return the number of bytes read. Bytes read
  /// will be `0` when the Body is or has become empty.
  ///
  /// Unlike `operator<<`, this operation will return a
  /// `fastly::unexpected<FastlyError>` that can then be handled if the read
  /// itself fails, instead of aborting the entire process.
  fastly::expected<size_t> read(uint8_t *buf, size_t bufsize);

  /// Write bytes to the end of this body, and return the number of bytes
  /// written. Bytes written will be `0` on EOF.
  ///
  /// Unlike `operator<<`, this operation will return a
  /// `fastly::unexpected<FastlyError>` that can then be handled if the write
  /// itself fails, instead of aborting the entire process.
  fastly::expected<size_t> write(uint8_t *buf, size_t bufsize);

  /// Append another body onto the end of this body.
  void append(Body other);

  /// Appends request or response trailers to the body.
  fastly::expected<void> append_trailer(std::string_view header_name,
                                        std::string_view header_value);

  // TODO(@zkat): this needs a HeaderMap wrapper.
  // HeaderMap get_trailers();

  // TODO(@zkat)
  // Prefix get_prefix(uint32_t prefix_len);
  // PrefixString get_prefix_string(uint32_t prefix_len);
  // class Prefix {
  //     private:
  //     rust::Box<fastly::sys::http::Prefix> pref;
  //     Prefix(rust::Box<fastly::sys::http::Prefix> prefix) :
  //     pref(std::move(prefix)) {};
  // };

  // class PrefixString {
  //     private:
  //     rust::Box<fastly::sys::http::PrefixString> pref;
  //     PrefixString(rust::Box<fastly::sys::http::PrefixString> prefix) :
  //     pref(std::move(prefix)) {};
  // };
private:
  rust::Box<fastly::sys::http::Body> bod;
  std::array<char, 512> pbuf;
  std::array<char, 512> gbuf;
  Body(rust::Box<fastly::sys::http::Body> body)
      : std::iostream(this), bod(std::move(body)) {
    this->setg(this->gbuf.data(), this->gbuf.data(), this->gbuf.data());
    this->setp(this->pbuf.data(), this->pbuf.data() + this->pbuf.max_size());
  };

  fastly::expected<void> fill_from_vec(std::vector<uint8_t> vec) {
    size_t pos{0};
    fastly::expected<size_t> written{0};
    while (*written) {
      written = this->write(vec.data() + pos, vec.size() - pos);
      if (written.has_value()) {
        pos += *written;
      } else {
        return fastly::unexpected(std::move(written.error()));
      }
    }
    return fastly::expected<void>();
  }
};

class StreamingBody : public std::ostream, public std::streambuf {
  friend Response;
  friend Request;
  friend std::pair<fastly::expected<Response>,
                   std::vector<request::PendingRequest>>
  request::select(std::vector<request::PendingRequest> &reqs);

protected:
  int overflow(int_type val);
  int sync();

public:
  StreamingBody(StreamingBody &&other)
      : std::ostream(this), bod((other.sync(), std::move(other.bod))),
        pbuf(std::move(other.pbuf)) {
    this->setp(this->pbuf.data(), this->pbuf.data() + this->pbuf.max_size());
  };
  fastly::expected<void> finish();
  void append(Body other);
  fastly::expected<size_t> write(uint8_t *buf, size_t bufsize);
  fastly::expected<void> append_trailer(std::string_view header_name,
                                        std::string_view header_value);
  // TODO(@zkat): needs the HeaderMap type.
  // fastly::expected<void> finish_with_trailers(&HeaderMap trailers);

private:
  StreamingBody(rust::Box<fastly::sys::http::StreamingBody> body)
      : std::ostream(this), bod(std::move(body)) {
    this->setp(this->pbuf.data(), this->pbuf.data() + this->pbuf.max_size());
  };
  rust::Box<fastly::sys::http::StreamingBody> bod;
  std::array<char, 512> pbuf;
};

} // namespace fastly::http

namespace fastly {
using fastly::http::Body;
}

#endif
