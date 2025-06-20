#ifndef FASTLY_HTTP_BODY_H
#define FASTLY_HTTP_BODY_H

#include <memory>
// #include <streambuf>
#include <string>
#include <vector>

#include "../sdk-sys.h"

namespace fastly::http {

class Body {

protected:
  // TODO(@zkat): Implement all the stream stuff.
  // int underflow();
  // int overflow(traits_type val);
  // int sync();
public:
  Body() : bod(std::move(fastly::sys::http::m_static_http_body_new())) {};
  Body(rust::Box<fastly::sys::http::Body> body) : bod(std::move(body)) {};
  Body(std::vector<uint8_t> body_vec) : Body() {
    this->fill_from_vec(body_vec);
  };
  Body(std::string body_str) : Body() {
    std::vector<uint8_t> vec{body_str.begin(), body_str.end()};
    this->fill_from_vec(vec);
  }
  size_t read(uint8_t *buf, size_t bufsize);
  size_t write(uint8_t *buf, size_t bufsize);
  void append(Body other);
  void append_trailer(std::string &header_name, std::string &header_value);

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

  rust::Box<fastly::sys::http::Body> bod;

private:
  void fill_from_vec(std::vector<uint8_t> vec) {
    size_t pos{0};
    size_t written{0};
    while ((written = this->write(vec.data() + pos, vec.size() - pos))) {
      pos += written;
    }
  }
};

class StreamingBody {
public:
  StreamingBody(rust::Box<fastly::sys::http::StreamingBody> body)
      : bod(std::move(body)) {};
  void finish();
  void append(Body other);
  size_t write(uint8_t *buf, size_t bufsize);
  void append_trailer(std::string &header_name, std::string &header_value);
  // TODO(@zkat): needs the HeaderMap type.
  // void finish_with_trailers(&HeaderMap trailers);
  rust::Box<fastly::sys::http::StreamingBody> bod;
};

} // namespace fastly::http

namespace fastly {
using fastly::http::Body;
}

#endif
