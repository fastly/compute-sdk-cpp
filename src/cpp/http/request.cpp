#include "request.h"

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

Response Request::send(std::string backend) {
  Response res{fastly::sys::http::m_http_request_send(std::move(this->req),
                                                      std::move(backend))};
  return res;
}

HeaderIter Request::get_header_all(std::string name) {
  return this->req->get_header_all(name);
}

void Request::set_auto_decompress_gzip(bool gzip) {
  this->req->set_auto_decompress_gzip(gzip);
}

Request *Request::with_auto_decompress_gzip(bool gzip) {
  this->set_auto_decompress_gzip(gzip);
  return this;
}

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

std::unique_ptr<std::vector<uint8_t>> Request::into_body_bytes() {
  return fastly::sys::http::m_http_request_into_body_bytes(
      std::move(this->req));
}

std::string Request::into_body_string() {
  auto bytes{this->into_body_bytes()};
  return std::string(bytes->begin(), bytes->end());
}

} // namespace fastly::http
