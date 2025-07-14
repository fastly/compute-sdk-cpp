#include "response.h"
#include "error.h"
#include "sdk-sys.h"

namespace fastly::http {

Response::Response()
    : res(std::move(fastly::sys::http::m_static_http_response_new())) {}

bool Response::is_from_backend() { return this->res->is_from_backend(); }

Response Response::clone_without_body() {
  Response res{this->res->clone_without_body()};
  return res;
}

Response Response::clone_with_body() {
  Response res{this->res->clone_with_body()};
  return res;
}

Response Response::from_body(Body body) {
  Response res(std::move(fastly::sys::http::m_static_http_response_from_body(
      std::move(body.bod))));
  return res;
}

Response Response::from_status(StatusCode status) {
  Response res(std::move(
      fastly::sys::http::m_static_http_response_from_status(status.as_code())));
  return res;
}

Response Response::with_body(Body body) {
  this->set_body(std::move(body));
  return std::move(*this);
}

void Response::set_body(Body body) {
  // TODO(@zkat): this is broken rn because I messed up the move constructor for
  // Body/StreamingBody.
  body.flush();
  this->res->set_body(std::move(body.bod));
}

Body Response::take_body() {
  Body body{this->res->take_body()};
  return body;
}

void Response::append_body(Body other) {
  other.flush();
  this->res->append_body(std::move(other.bod));
}

std::vector<uint8_t> Response::into_body_bytes() {
  auto str{this->into_body_string()};
  return std::vector<uint8_t>(str.begin(), str.end());
}

std::string Response::into_body_string() {
  Body body{this->into_body()};
  return std::string(std::istreambuf_iterator<char>(body),
                     std::istreambuf_iterator<char>());
}

Body Response::into_body() {
  return Body(
      fastly::sys::http::m_http_response_into_body(std::move(this->res)));
}

fastly::expected<void> Response::set_body_text_plain(std::string_view body) {
  fastly::sys::error::FastlyError *err;
  this->res->set_body_text_plain(static_cast<std::string>(body), err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<void>();
  }
}

fastly::expected<Response>
Response::with_body_text_html(std::string_view body) {
  return this->set_body_text_html(body).map(
      [this]() { return std::move(*this); });
}

fastly::expected<void> Response::set_body_text_html(std::string_view body) {
  fastly::sys::error::FastlyError *err;
  this->res->set_body_text_html(static_cast<std::string>(body), err);
  if (err != nullptr) {
    return fastly::expected<void>();
  } else {
    return fastly::unexpected(err);
  }
}

fastly::expected<Response>
Response::with_body_text_plain(std::string_view body) {
  return this->set_body_text_plain(body).map(
      [this]() { return std::move(*this); });
}

std::string Response::take_body_string() {
  Body bod{this->take_body()};
  return std::string(std::istreambuf_iterator<char>(bod),
                     std::istreambuf_iterator<char>());
}

Response Response::with_body_octet_stream(std::vector<uint8_t> body) {
  this->set_body_octet_stream(body);
  return std::move(*this);
}

void Response::set_body_octet_stream(std::vector<uint8_t> body) {
  this->res->set_body_octet_stream(body);
}

std::vector<uint8_t> Response::take_body_bytes() {
  auto body_str{this->take_body_string()};
  return std::vector<uint8_t>(body_str.begin(), body_str.end());
}

// ChunksIter Response::read_body_chunks(size_t chunk_size);

std::optional<std::string> Response::get_content_type() {
  std::string out;
  if (this->res->get_content_type(out)) {
    return out;
  } else {
    return std::nullopt;
  }
}

Response Response::with_content_type(std::string_view mime) {
  this->set_content_type(mime);
  return std::move(*this);
}

void Response::set_content_type(std::string_view mime) {
  this->res->set_content_type(static_cast<std::string>(mime));
}

std::optional<size_t> Response::get_content_length() {
  size_t len;
  if (this->res->get_content_length(len)) {
    return len;
  } else {
    return std::nullopt;
  }
}

fastly::expected<bool> Response::contains_header(std::string_view name) {
  fastly::sys::error::FastlyError *err;
  bool has_header{
      this->res->contains_header(static_cast<std::string>(name), err)};
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<bool>(has_header);
  }
}

fastly::expected<Response> Response::with_header(std::string_view name,
                                                 std::string_view value) {
  return this->append_header(name, value).map([this]() {
    return std::move(*this);
  });
}

fastly::expected<Response> Response::with_set_header(std::string_view name,
                                                     std::string_view value) {
  return this->set_header(name, value).map([this]() {
    return std::move(*this);
  });
}

// TODO(@zkat): do a proper HeaderValue situation here?
fastly::expected<std::optional<std::string>>
Response::get_header(std::string_view name) {
  fastly::sys::error::FastlyError *err;
  std::string out;
  bool has_header{
      this->res->get_header(static_cast<std::string>(name), out, err)};
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else if (has_header) {
    return std::optional<std::string>(std::move(out));
  } else {
    return std::nullopt;
  }
}

fastly::expected<HeaderValuesIter>
Response::get_header_all(std::string_view name) {
  fastly::sys::http::HeaderValuesIter *out;
  fastly::sys::error::FastlyError *err;
  this->res->get_header_all(static_cast<std::string>(name), out, err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return FSLY_BOX(http, HeaderValuesIter, out);
  }
}

// TODO(@zkat): sigh. IDK
// ??? get_headers();
// HeaderNamesIter get_header_names();

fastly::expected<void> Response::set_header(std::string_view name,
                                            std::string_view value) {
  fastly::sys::error::FastlyError *err;
  this->res->set_header(static_cast<std::string>(name),
                        static_cast<std::string>(value), err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<void>();
  }
}

fastly::expected<void> Response::append_header(std::string_view name,
                                               std::string_view value) {
  fastly::sys::error::FastlyError *err;
  this->res->append_header(static_cast<std::string>(name),
                           static_cast<std::string>(value), err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<void>();
  }
}

fastly::expected<std::optional<std::string>>
Response::remove_header(std::string_view name) {
  fastly::sys::error::FastlyError *err;
  std::string out;
  bool has_header{
      this->res->remove_header(static_cast<std::string>(name), out, err)};
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else if (has_header) {
    return std::optional<std::string>(std::move(out));
  } else {
    return std::nullopt;
  }
}

void Response::set_status(StatusCode status) {
  this->res->set_status(status.as_code());
}

Response Response::with_status(StatusCode status) {
  this->set_status(status);
  return std::move(*this);
}

std::optional<std::string> Response::get_backend_name() {
  std::string name;
  bool existed{this->res->get_backend_name(name)};
  if (!existed) {
    return std::nullopt;
  } else {
    return {name};
  }
}

std::optional<fastly::backend::Backend> Response::get_backend() {
  auto ptr{this->res->get_backend()};
  if (ptr == nullptr) {
    return std::nullopt;
  } else {
    return FSLY_BOX(backend, Backend, ptr);
  }
}

std::optional<std::string> Response::get_backend_addr() {
  std::string addr;
  bool existed{this->res->get_backend_name(addr)};
  if (!existed) {
    return std::nullopt;
  } else {
    return {addr};
  }
}

std::optional<Request> Response::take_backend_request() {
  auto ptr{this->res->take_backend_request()};
  if (ptr == nullptr) {
    return std::nullopt;
  } else {
    return FSLY_BOX(http, Request, ptr);
  }
}

void Response::send_to_client() {
  // TODO(@zkat): flush body before sending.
  fastly::sys::http::m_http_response_send_to_client(std::move(this->res));
}

StreamingBody Response::stream_to_client() {
  return {fastly::sys::http::m_http_response_stream_to_client(
      std::move(this->res))};
}

std::optional<std::chrono::milliseconds> Response::get_ttl() {
  uint32_t out;
  if (this->res->get_ttl(out)) {
    return std::chrono::milliseconds(out);
  } else {
    return std::nullopt;
  }
}

std::optional<std::chrono::milliseconds> Response::get_age() {
  uint32_t out;
  if (this->res->get_age(out)) {
    return std::chrono::milliseconds(out);
  } else {
    return std::nullopt;
  }
}

std::optional<std::chrono::milliseconds>
Response::get_stale_while_revalidate() {
  uint32_t out;
  if (this->res->get_stale_while_revalidate(out)) {
    return std::chrono::milliseconds(out);
  } else {
    return std::nullopt;
  }
}

} // namespace fastly::http
