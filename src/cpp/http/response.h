#ifndef FASTLY_HTTP_RESPONSE_H
#define FASTLY_HTTP_RESPONSE_H

#include "../sdk-sys.h"
#include "body.h"
#include <string>
#include <vector>

namespace fastly::http {

class Response {
public:
  Response();
  Response(rust::Box<fastly::sys::http::Response> response)
      : res(std::move(response)) {};
  static Response from_body(Body body);
  void set_body(Body body);
  Response *with_body(Body body);
  Body take_body();
  void send_to_client();

private:
  rust::Box<fastly::sys::http::Response> res;
};

} // namespace fastly::http

namespace fastly {
using fastly::http::Response;
}

#endif
