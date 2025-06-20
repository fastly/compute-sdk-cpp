#include "response.h"

namespace fastly::http {

Response::Response()
    : res(std::move(fastly::sys::http::m_static_http_response_new())) {}

Response Response::from_body(Body body) {
  Response res(std::move(fastly::sys::http::m_static_http_response_from_body(
      std::move(body.bod))));
  return res;
}

void Response::set_body(Body body) { this->res->set_body(std::move(body.bod)); }

Response *Response::with_body(Body body) {
  this->set_body(std::move(body));
  return this;
}

Body Response::take_body() {
  Body body{this->res->take_body()};
  return body;
}

void Response::send_to_client() {
  fastly::sys::http::m_http_response_send_to_client(std::move(this->res));
}

} // namespace fastly::http
