#include "body.h"

namespace fastly::http {

void Body::append(Body other) {
  return this->bod->append(std::move(other.bod));
}

std::size_t Body::read(uint8_t *buf, std::size_t bufsize) {
  rust::Slice<uint8_t> slice{buf, bufsize};
  return this->bod->read(slice);
}

std::size_t Body::write(uint8_t *buf, std::size_t bufsize) {
  rust::Slice<const uint8_t> slice{buf, bufsize};
  return this->bod->write(slice);
}

// TODO(@zkat): these need other types
// Prefix get_prefix(uint32_t prefix_len);
// PrefixString get_prefix_string(uint32_t prefix_len);

void Body::append_trailer(std::string &header_name, std::string &header_value) {
  this->bod->append_trailer(header_name, header_value);
}

// TODO(@zkat): this needs a HeaderMap wrapper.
// HeaderMap get_trailers();

void StreamingBody::finish() {
  return fastly::sys::http::m_http_streaming_body_finish(std::move(this->bod));
}

void StreamingBody::append(Body other) {
  return this->bod->append(std::move(other.bod));
}

std::size_t StreamingBody::write(uint8_t *buf, std::size_t bufsize) {
  rust::Slice<const uint8_t> slice{buf, bufsize};
  return this->bod->write(slice);
}

void StreamingBody::append_trailer(std::string &header_name,
                                   std::string &header_value) {
  this->bod->append_trailer(header_name, header_value);
}
} // namespace fastly::http
