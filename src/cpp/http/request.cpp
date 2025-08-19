#include "request.h"
#include "error.h"
#include "sdk-sys.h"

namespace fastly::http {

namespace request {

std::variant<PendingRequest, fastly::expected<Response>>
PendingRequest::poll() {
  auto poll_result{
      fastly::sys::http::request::m_http_request_pending_request_poll(
          std::move(this->req))};
  if (poll_result->is_response()) {
    return {fastly::expected<Response>(Response(
        fastly::sys::http::request::m_http_request_poll_result_into_response(
            std::move(poll_result))))};
  } else if (poll_result->is_pending()) {
    return {PendingRequest(
        fastly::sys::http::request::m_http_request_poll_result_into_pending(
            std::move(poll_result)))};
  } else {
    return {fastly::unexpected(
        fastly::sys::http::request::m_http_request_poll_result_into_error(
            std::move(poll_result)))};
  }
}

fastly::expected<Response> PendingRequest::wait() {
  fastly::sys::http::Response *ret;
  fastly::sys::error::FastlyError *err;
  fastly::sys::http::request::m_http_request_pending_request_wait(
      std::move(this->req), ret, err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return FSLY_BOX(http, Response, ret);
  }
}

Request PendingRequest::cloned_sent_req() {
  return {this->req->cloned_sent_req()};
}

std::pair<fastly::expected<Response>, std::vector<PendingRequest>>
select(std::vector<PendingRequest> &reqs) {
  rust::Vec<fastly::sys::http::request::BoxPendingRequest> vecreqs;
  rust::Vec<fastly::sys::http::request::BoxPendingRequest> others;
  for (auto &boxed : reqs) {
    fastly::sys::http::request::f_http_push_box_pending_request_into_vec(
        vecreqs, std::move(boxed.req));
  }
  fastly::sys::http::Response *resp;
  fastly::sys::error::FastlyError *err;
  fastly::sys::http::request::f_http_request_select(std::move(vecreqs), resp,
                                                    others, err);
  std::vector<PendingRequest> ret_others;
  for (auto &box : others) {
    ret_others.push_back(PendingRequest(box.extract_req()));
  }
  if (err != nullptr) {
    return std::make_pair(fastly::unexpected(err), std::move(ret_others));
  } else {
    return std::make_pair(
        fastly::expected<Response>(FSLY_BOX(http, Response, resp)),
        std::move(ret_others));
  }
}

} // namespace request

Request::Request(Method method, std::string_view url)
    : req(fastly::sys::http::m_static_http_request_new(
          method, static_cast<std::string>(url))) {}

Request Request::from_client() {
  Request req{fastly::sys::http::m_static_http_request_from_client()};
  return req;
}

Request Request::get(std::string_view url) {
  Request req{fastly::sys::http::m_static_http_request_get(
      static_cast<std::string>(url))};
  return req;
}

Request Request::head(std::string_view url) {
  Request req{fastly::sys::http::m_static_http_request_head(
      static_cast<std::string>(url))};
  return req;
}

Request Request::post(std::string_view url) {
  Request req{fastly::sys::http::m_static_http_request_post(
      static_cast<std::string>(url))};
  return req;
}

Request Request::put(std::string_view url) {
  Request req{fastly::sys::http::m_static_http_request_put(
      static_cast<std::string>(url))};
  return req;
}

Request Request::delete_(std::string_view url) {
  Request req{fastly::sys::http::m_static_http_request_delete(
      static_cast<std::string>(url))};
  return req;
}

Request Request::connect(std::string_view url) {
  Request req{fastly::sys::http::m_static_http_request_connect(
      static_cast<std::string>(url))};
  return req;
}

Request Request::options(std::string_view url) {
  Request req{fastly::sys::http::m_static_http_request_options(
      static_cast<std::string>(url))};
  return req;
}

Request Request::trace(std::string_view url) {
  Request req{fastly::sys::http::m_static_http_request_trace(
      static_cast<std::string>(url))};
  return req;
}

Request Request::patch(std::string_view url) {
  Request req{fastly::sys::http::m_static_http_request_patch(
      static_cast<std::string>(url))};
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

fastly::expected<Response> Request::send(std::string_view backend_name) {
  return fastly::backend::Backend::from_name(backend_name)
      .and_then([this](fastly::backend::Backend backend) {
        return this->send(backend);
      });
}

fastly::expected<Response> Request::send(fastly::backend::Backend &backend) {
  fastly::sys::http::Response *resp;
  fastly::sys::error::FastlyError *err;
  fastly::sys::http::m_http_request_send(std::move(this->req), *backend.backend,
                                         resp, err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return FSLY_BOX(http, Response, resp);
  }
}

fastly::expected<request::PendingRequest>
Request::send_async(std::string_view backend_name) {
  return fastly::backend::Backend::from_name(backend_name)
      .and_then([this](fastly::backend::Backend backend) {
        return this->send_async(backend);
      });
}

fastly::expected<request::PendingRequest>
Request::send_async(fastly::backend::Backend &backend) {
  fastly::sys::http::request::PendingRequest *req;
  fastly::sys::error::FastlyError *err;
  fastly::sys::http::m_http_request_send_async(std::move(this->req),
                                               *backend.backend, req, err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return FSLY_BOX(http, request::PendingRequest, req);
  }
}

fastly::expected<std::pair<StreamingBody, request::PendingRequest>>
Request::send_async_streaming(std::string_view backend_name) {
  return fastly::backend::Backend::from_name(backend_name)
      .and_then([this](fastly::backend::Backend backend) {
        return this->send_async_streaming(backend);
      });
}

fastly::expected<std::pair<StreamingBody, request::PendingRequest>>
Request::send_async_streaming(fastly::backend::Backend &backend) {
  fastly::sys::http::request::AsyncStreamRes *res;
  fastly::sys::error::FastlyError *err;
  fastly::sys::http::m_http_request_send_async_streaming(
      std::move(this->req), *backend.backend, res, err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return std::make_pair(StreamingBody(res->take_body()),
                          request::PendingRequest(res->take_req()));
  }
}

void Request::set_body(Body body) {
  body << std::flush;
  this->req->set_body(std::move(body.bod));
}

Request Request::with_body(Body body) && {
  this->set_body(std::move(body));
  return std::move(*this);
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

fastly::expected<void> Request::set_body_text_plain(std::string_view body) {
  fastly::sys::error::FastlyError *err;
  this->req->set_body_text_plain(static_cast<std::string>(body), err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<void>();
  }
}

fastly::expected<Request> Request::with_body_text_html(std::string_view body) && {
  return this->set_body_text_html(body).map(
      [this]() { return std::move(*this); });
}

fastly::expected<void> Request::set_body_text_html(std::string_view body) {
  fastly::sys::error::FastlyError *err;
  this->req->set_body_text_html(static_cast<std::string>(body), err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<void>();
  }
}

fastly::expected<Request> Request::with_body_text_plain(std::string_view body) && {
  return this->set_body_text_plain(body).map(
      [this]() { return std::move(*this); });
}

std::string Request::take_body_string() {
  Body bod{this->take_body()};
  return std::string(std::istreambuf_iterator<char>(bod),
                     std::istreambuf_iterator<char>());
}

Request Request::with_body_octet_stream(std::vector<uint8_t> body) && {
  this->set_body_octet_stream(body);
  return std::move(*this);
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
  std::string out;
  if (this->req->get_content_type(out)) {
    return {out};
  } else {
    return std::nullopt;
  }
}

Request Request::with_content_type(std::string_view mime) && {
  this->set_content_type(mime);
  return std::move(*this);
}

void Request::set_content_type(std::string_view mime) {
  this->req->set_content_type(static_cast<std::string>(mime));
}

std::optional<size_t> Request::get_content_length() {
  size_t out;
  if (this->req->get_content_length(out)) {
    return {out};
  } else {
    return std::nullopt;
  }
}

fastly::expected<bool> Request::contains_header(std::string_view name) {
  fastly::sys::error::FastlyError *err;
  bool has_header{
      this->req->contains_header(static_cast<std::string>(name), err)};
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<bool>(has_header);
  }
}

fastly::expected<Request> Request::with_header(std::string_view name,
                                               std::string_view value) && {
  return this->append_header(name, value).map([this]() {
    return std::move(*this);
  });
}

fastly::expected<Request> Request::with_set_header(std::string_view name,
                                                   std::string_view value) && {
  return this->set_header(name, value).map([this]() {
    return std::move(*this);
  });
}

// TODO(@zkat): do a proper HeaderValue situation here?
fastly::expected<std::optional<std::string>>
Request::get_header(std::string_view name) {
  fastly::sys::error::FastlyError *err;
  std::string out;
  bool has_header{
      this->req->get_header(static_cast<std::string>(name), out, err)};
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else if (has_header) {
    return std::optional<std::string>(out);
  } else {
    return std::nullopt;
  }
}

fastly::expected<HeaderValuesIter>
Request::get_header_all(std::string_view name) {
  fastly::sys::http::HeaderValuesIter *out;
  fastly::sys::error::FastlyError *err;
  this->req->get_header_all(static_cast<std::string>(name), out, err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return FSLY_BOX(http, HeaderValuesIter, out);
  }
}

// TODO(@zkat): sigh. IDK
// ??? get_headers();
// HeaderNamesIter get_header_names();

fastly::expected<void> Request::set_header(std::string_view name,
                                           std::string_view value) {
  fastly::sys::error::FastlyError *err;
  this->req->set_header(static_cast<std::string>(name),
                        static_cast<std::string>(value), err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<void>();
  }
}

fastly::expected<void> Request::append_header(std::string_view name,
                                              std::string_view value) {
  fastly::sys::error::FastlyError *err;
  this->req->append_header(static_cast<std::string>(name),
                           static_cast<std::string>(value), err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<void>();
  }
}

fastly::expected<std::optional<std::string>>
Request::remove_header(std::string_view name) {
  fastly::sys::error::FastlyError *err;
  std::string out;
  bool has_header{
      this->req->remove_header(static_cast<std::string>(name), out, err)};
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else if (has_header) {
    return std::optional<std::string>(std::move(out));
  } else {
    return std::nullopt;
  }
}

Request Request::with_method(Method method) && {
  this->set_method(method);
  return std::move(*this);
}

Method Request::get_method() { return this->req->get_method(); }

void Request::set_method(Method method) { this->req->set_method(method); }

fastly::expected<Request> Request::with_url(std::string_view url) && {
  return this->set_url(url).map([this]() { return std::move(*this); });
}

std::string Request::get_url() {
  std::string out;
  this->req->get_url(out);
  return out;
}

fastly::expected<void> Request::set_url(std::string_view url) {
  fastly::sys::error::FastlyError *err;
  this->req->set_url(static_cast<std::string>(url), err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<void>();
  }
}

std::string Request::get_path() {
  std::string out;
  this->req->get_path(out);
  return out;
}

fastly::expected<Request> Request::with_path(std::string_view path) && {
  return this->set_path(path).map([this]() { return std::move(*this); });
}

fastly::expected<void> Request::set_path(std::string_view path) {
  fastly::sys::error::FastlyError *err;
  this->req->set_path(static_cast<std::string>(path), err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<void>();
  }
}

std::optional<std::string> Request::get_query_string() {
  std::string out;
  if (this->req->get_query_string(out)) {
    return {out};
  } else {
    return std::nullopt;
  }
}

std::optional<std::string>
Request::get_query_parameter(std::string_view param) {
  std::string out;
  if (this->req->get_query_parameter(static_cast<std::string>(param), out)) {
    return {out};
  } else {
    return std::nullopt;
  }
}

fastly::expected<Request> Request::with_query_string(std::string_view query) && {
  return this->set_query_string(query).map(
      [this]() { return std::move(*this); });
}

fastly::expected<void> Request::set_query_string(std::string_view query) {
  fastly::sys::error::FastlyError *err;
  this->req->set_query_string(static_cast<std::string>(query), err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<void>();
  }
}

void Request::remove_query() { this->req->remove_query(); }

std::optional<bool> Request::get_client_ddos_detected() {
  bool detected;
  if (this->req->get_client_ddos_detected(detected)) {
    return {detected};
  } else {
    return std::nullopt;
  }
}

// TODO(@zkat): Do these later, I think they're lower-pri.
// // TODO(@zkat): need Version enum
// // Request *with_version(Version version);
// // Version get_version();
// // void set_version(Version version);

Request Request::with_pass(bool pass) && {
  this->set_pass(pass);
  return std::move(*this);
}

void Request::set_pass(bool pass) { this->req->set_pass(pass); }

Request Request::with_ttl(uint32_t ttl) && {
  this->set_pass(ttl);
  return std::move(*this);
}

void Request::set_ttl(uint32_t ttl) { this->req->set_ttl(ttl); }

Request Request::with_stale_while_revalidate(uint32_t swr) && {
  this->set_stale_while_revalidate(swr);
  return std::move(*this);
}

void Request::set_stale_while_revalidate(uint32_t swr) {
  this->req->set_stale_while_revalidate(swr);
}

Request Request::with_pci(bool pci) && {
  this->set_pci(pci);
  return std::move(*this);
}

void Request::set_pci(bool pci) { this->req->set_pci(pci); }

fastly::expected<Request> Request::with_surrogate_key(std::string_view sk) && {
  return this->set_surrogate_key(sk).map([this]() { return std::move(*this); });
}

fastly::expected<void> Request::set_surrogate_key(std::string_view sk) {
  fastly::sys::error::FastlyError *err;
  this->req->set_surrogate_key(static_cast<std::string>(sk), err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else {
    return fastly::expected<void>();
  }
}

std::optional<std::string> Request::get_client_ip_addr() {
  std::string ret;
  if (this->req->get_client_ip_addr(ret)) {
    return {ret};
  } else {
    return std::nullopt;
  }
}

std::optional<std::string> Request::get_server_ip_addr() {
  std::string ret;
  if (this->req->get_server_ip_addr(ret)) {
    return {ret};
  } else {
    return std::nullopt;
  }
}

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

Request Request::with_auto_decompress_gzip(bool gzip) && {
  this->set_auto_decompress_gzip(gzip);
  return std::move(*this);
}

// TODO(@zkat): needs enum
// void set_framing_headers_mode(FramingHeadersMode mode);
// Request *set_framing_headers_mode(FramingHeadersMode mode);
bool Request::fastly_key_is_valid() { return this->req->fastly_key_is_valid(); }

// TODO(@zkat): Do these later. I think they're lower-pri.
// void handoff_websocket(fastly::backend::Backend backend);
// void handoff_fanout(fastly::backend::Backend backend);
// Request *on_behalf_of(std::string service);

void Request::set_cache_key(std::string_view key) {
  this->req->set_cache_key(std::vector<uint8_t>{key.begin(), key.end()});
}

void Request::set_cache_key(std::vector<uint8_t> key) {
  this->req->set_cache_key(key);
}

Request Request::with_cache_key(std::string_view key) && {
  this->set_cache_key(key);
  return std::move(*this);
}

Request Request::with_cache_key(std::vector<uint8_t> key) && {
  this->set_cache_key(key);
  return std::move(*this);
}

Version Request::get_version() {
  return this->req->get_version();
}

void Request::set_version(Version version) {
  this->req->set_version(version);
}

Request Request::with_version(Version version) && {
  this->set_version(version);
  return std::move(*this);
}

bool is_cacheable();

} // namespace fastly::http
