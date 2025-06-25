#include "request.h"
#include <iterator>

namespace fastly::http {

Request::Request(Method method, std::string url)
    : req(std::move(
          fastly::sys::http::m_static_http_request_new(method, url))) {}

Request Request::from_client() {
  Request req{fastly::sys::http::m_static_http_request_from_client()};
  return req;
}

Request Request::get(std::string url) {
  Request req{fastly::sys::http::m_static_http_request_get(url)};
  return req;
}

Request Request::head(std::string url) {
  Request req{fastly::sys::http::m_static_http_request_head(url)};
  return req;
}

Request Request::post(std::string url) {
  Request req{fastly::sys::http::m_static_http_request_post(url)};
  return req;
}

Request Request::put(std::string url) {
  Request req{fastly::sys::http::m_static_http_request_put(url)};
  return req;
}

Request Request::delete_(std::string url) {
  Request req{fastly::sys::http::m_static_http_request_delete(url)};
  return req;
}

Request Request::connect(std::string url) {
  Request req{fastly::sys::http::m_static_http_request_connect(url)};
  return req;
}

Request Request::options(std::string url) {
  Request req{fastly::sys::http::m_static_http_request_options(url)};
  return req;
}

Request Request::trace(std::string url) {
  Request req{fastly::sys::http::m_static_http_request_trace(url)};
  return req;
}

Request Request::patch(std::string url) {
  Request req{fastly::sys::http::m_static_http_request_patch(url)};
  return req;
}

bool Request::is_from_client() { return this->req->is_from_client(); }

Request Request::clone_without_body() {
  Request req{this->req->clone_without_body()};
  return req;
}

Request Request::clone_with_body() {
  Request req{this->req->clone_with_body()};
  return req;
}

Response Request::send(fastly::backend::Backend backend) {
  Response res{fastly::sys::http::m_http_request_send(std::move(this->req),
                                                      backend.backend)};
  return res;
}

// PendingRequest send_async(fastly::backend::Backend backend);
// std::pair<fastly::http::StreamingBody, PendingRequest>
// send_async(fastly::backend::Backend backend);

void Request::set_body(Body body) { this->req->set_body(std::move(body.bod)); }

Request *Request::with_body(Body body) {
  this->set_body(std::move(body));
  return this;
}

bool Request::has_body() { return this->req->has_body(); }

Body Request::take_body() {
  Body body{this->req->take_body()};
  return body;
}

std::vector<uint8_t> Request::into_body_bytes() {
  auto str{this->into_body_string()};
  return std::vector<uint8_t>(str.begin(), str.end());
}

std::string Request::into_body_string() {
  Body body{this->into_body()};
  return std::string(std::istreambuf_iterator<char>(body),
                     std::istreambuf_iterator<char>());
}

Body Request::into_body() {
  return Body(
      fastly::sys::http::m_http_request_into_body(std::move(this->req)));
}

void Request::set_body_text_plain(std::string body) {
  return this->req->set_body_text_plain(body);
}

Request *Request::with_body_text_html(std::string body) {
  this->set_body_text_html(body);
  return this;
}

void Request::set_body_text_html(std::string body) {
  return this->req->set_body_text_html(body);
}

Request *Request::with_body_text_plain(std::string body) {
  this->set_body_text_plain(body);
  return this;
}

std::string Request::take_body_string() {
  Body bod{this->take_body()};
  return std::string(std::istreambuf_iterator<char>(bod),
                     std::istreambuf_iterator<char>());
}

Request *Request::with_body_octet_stream(std::vector<uint8_t> body) {
  this->set_body_octet_stream(body);
  return this;
}

void Request::set_body_octet_stream(std::vector<uint8_t> body) {
  this->req->set_body_octet_stream(body);
}

std::vector<uint8_t> Request::take_body_bytes() {
  auto body_str{this->take_body_string()};
  return std::vector<uint8_t>(body_str.begin(), body_str.end());
}

// ChunksIter Request::read_body_chunks(size_t chunk_size);

std::optional<std::string> Request::get_content_type() {
  auto ptr{this->req->get_content_type()};
  if (ptr == nullptr) {
    return std::nullopt;
  } else {
    std::string str(ptr->begin(), ptr->end());
    return std::optional<std::string>{str};
  }
}

Request *Request::with_content_type(std::string mime) {
  this->set_content_type(mime);
  return this;
}

void Request::set_content_type(std::string mime) {
  this->req->set_content_type(mime);
}

std::optional<size_t> Request::get_content_length() {
  auto ptr{this->req->get_content_length()};
  if (ptr == nullptr) {
    return std::nullopt;
  } else {
    return std::optional<size_t>(*ptr);
  }
}

bool Request::contains_header(std::string name) {
  return this->req->contains_header(name);
}

Request *Request::with_header(std::string name, std::string value) {
  this->append_header(name, value);
  return this;
}

Request *Request::with_set_header(std::string name, std::string value) {
  this->set_header(name, value);
  return this;
}

// TODO(@zkat): do a proper HeaderValue situation here?
std::optional<std::string> Request::get_header(std::string name) {
  auto ptr{this->req->get_header(name)};
  if (ptr == nullptr) {
    return std::nullopt;
  } else {
    std::string str{ptr->begin(), ptr->end()};
    return {str};
  }
}

HeaderValuesIter Request::get_header_all(std::string name) {
  return this->req->get_header_all(name);
}

// TODO(@zkat): sigh. IDK
// ??? get_headers();
// HeaderNamesIter get_header_names();

void Request::set_header(std::string name, std::string value) {
  this->req->set_header(name, value);
}

void Request::append_header(std::string name, std::string value) {
  this->req->append_header(name, value);
}

std::optional<std::string> Request::remove_header(std::string name) {
  auto ptr{this->req->remove_header(name)};
  if (ptr == nullptr) {
    return std::nullopt;
  } else {
    std::string str{ptr->begin(), ptr->end()};
    return {str};
  }
}

Request *Request::with_method(Method method) {
  this->set_method(method);
  return this;
}

Method Request::get_method() { return this->req->get_method(); }

void Request::set_method(Method method) { this->req->set_method(method); }

// TODO(@zkat): Actual URL object?
Request *Request::with_url(std::string url) {
  this->set_url(url);
  return this;
}

std::string Request::get_url() {
  std::unique_ptr<std::vector<uint8_t>> data{this->req->get_url()};
  return {data->begin(), data->end()};
}

void Request::set_url(std::string url) { this->req->set_url(url); }

std::string Request::get_path() {
  std::unique_ptr<std::vector<uint8_t>> data{this->req->get_path()};
  return {data->begin(), data->end()};
}

Request *Request::with_path(std::string path) {
  this->set_path(path);
  return this;
}

void Request::set_path(std::string path) { this->req->set_path(path); }

std::optional<std::string> Request::get_query_string() {
  auto ptr{this->req->get_query_string()};
  if (ptr == nullptr) {
    return std::nullopt;
  } else {
    std::string str{ptr->begin(), ptr->end()};
    return {str};
  }
}

std::optional<std::string> Request::get_query_parameter(std::string param) {
    auto ptr{this->req->get_query_parameter(param)};
    if (ptr == nullptr) {
      return std::nullopt;
    } else {
      std::string str{ptr->begin(), ptr->end()};
      return {str};
    }
}

Request *Request::with_query_string(std::string query) {
    this->set_query_string(query);
    return this;
}

void Request::set_query_string(std::string query) {
   this->req->set_query_string(query); 
}

void Request::remove_query() {
    this->req->remove_query();
}

std::optional<bool> Request::get_client_ddos_detected() {
    auto ptr{this->req->get_client_ddos_detected()};
    if (ptr == nullptr) {
      return std::nullopt;
    } else {
      return {*ptr};
    }
}

// TODO(@zkat): Do these later, I think they're lower-pri.
// // TODO(@zkat): need Version enum
// // Request *with_version(Version version);
// // Version get_version();
// // void set_version(Version version);
// Request *with_pass(bool pass);
// void set_pass(bool pass);
// Request *with_ttl(uint32_t ttl);
// void set_ttl(uint32_t ttl);
// Request *with_stale_while_revalidate(uint32_t swr);
// void set_stale_while_revalidate(uint32_t swr);
// Request *with_pci(bool pci);
// void set_pci(bool pci);
// Request *with_surrogate_key(std::string sk);
// void set_surrogate_key(std::string sk);
// // TODO(@zkat): needs an IpAddr situation.
// // std::optional<IpAddr> get_client_ip_addr();
// // std::optional<IpAddr> get_server_ip_addr();
// // TODO(@zkat): needs iterator
// // std::optional<HeaderNameIter> get_original_header_names();
// std::optional<uint32_t> get_original_header_count();
// std::optional<std::vector<uint8_t>> get_tls_client_hello();
// std::optional<std::array<uint8_t, 16>> get_tls_ja3_md5();
// std::optional<std::string> get_tls_ja4();
// std::optional<std::string> get_tls_raw_client_certificate();
// std::optional<std::vector<uint8_t>> get_tls_raw_client_certificate_bytes();
// // TODO(@zkat): needs additional type
// // std::optional<ClientCertVerifyResult> get_tls_client_cert_verify_result();
// std::optional<std::string> get_tls_cipher_openssl_name();
// std::optional<std::vector<uint8_t>> get_tls_cipher_openssl_name_bytes();
// std::optional<std::vector<uint8_t>> get_tls_protocol_bytes();

void Request::set_auto_decompress_gzip(bool gzip) {
  this->req->set_auto_decompress_gzip(gzip);
}

Request *Request::with_auto_decompress_gzip(bool gzip) {
  this->set_auto_decompress_gzip(gzip);
  return this;
}

// TODO(@zkat): needs enum
// void set_framing_headers_mode(FramingHeadersMode mode);
// Request *set_framing_headers_mode(FramingHeadersMode mode);
bool Request::fastly_key_is_valid() {
    return this->req->fastly_key_is_valid();
}

// TODO(@zkat): Do these later. I think they're lower-pri.
// void handoff_websocket(fastly::backend::Backend backend);
// void handoff_fanout(fastly::backend::Backend backend);
// Request *on_behalf_of(std::string service);

void Request::set_cache_key(std::string key) {
    this->req->set_cache_key(std::vector<uint8_t>{key.begin(), key.end()});
}

void Request::set_cache_key(std::vector<uint8_t> key) {
    this->req->set_cache_key(key);
}

Request *Request::with_cache_key(std::string key) {
    this->set_cache_key(key);
    return this;
}

Request *Request::with_cache_key(std::vector<uint8_t> key) {
    this->set_cache_key(key);
    return this;
}

bool is_cacheable();

} // namespace fastly::http
