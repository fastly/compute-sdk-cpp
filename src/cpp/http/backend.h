#ifndef FASTLY_HTTP_BACKEND_H
#define FASTLY_HTTP_BACKEND_H

#include "../sdk-sys.h"
#include <chrono>
#include <string>

namespace fastly::backend {

class BackendBuilder;
class Backend {
public:
  Backend(std::string name)
      : backend(std::move(
            fastly::sys::backend::m_static_backend_backend_from_name(name))) {};
  Backend(rust::Box<fastly::sys::backend::Backend> b)
      : backend(std::move(b)) {};
  static Backend from_name(std::string name);
  Backend clone();
  BackendBuilder builder(std::string name, std::string target);
  std::string name();
  std::string into_string();
  bool exists();
  bool is_dynamic();
  std::string get_host();
  // TODO(@zkat): Do something about the optional thing here.
  // std::string get_host_override();
  uint16_t get_port();
  std::chrono::milliseconds get_connect_timeout();
  std::chrono::milliseconds get_first_byte_timeout();
  std::chrono::milliseconds get_between_byte_timeout();
  std::chrono::milliseconds get_http_keepalive_time();
  bool get_tcp_keepalive_enable();
  std::chrono::milliseconds get_tcp_keepalive_interval();
  uint32_t get_tcp_keepalive_probes();
  std::chrono::milliseconds get_tcp_keepalive_time();
  bool is_ssl();
  bool operator==(Backend b) { return backend->equals(b.backend); }
  bool operator!=(Backend b) { return !backend->equals(b.backend); }
  // TODO(@zkat): optional stuff is weird.
  // SslVersion get_ssl_min_version();
  // SslVersion get_ssl_max_version();
  rust::Box<fastly::sys::backend::Backend> backend;
};

class BackendBuilder {
public:
  BackendBuilder(std::string name, std::string target)
      : builder(std::move(
            fastly::sys::backend::m_static_backend_backend_builder_new(
                name, target))) {};
  BackendBuilder(rust::Box<fastly::sys::backend::BackendBuilder> b)
      : builder(std::move(b)) {};

  BackendBuilder override_host(std::string name);
  BackendBuilder connect_timeout(std::chrono::milliseconds timeout);
  BackendBuilder first_byte_timeout(std::chrono::milliseconds timeout);
  BackendBuilder between_bytes_timeout(std::chrono::milliseconds timeout);
  BackendBuilder enable_ssl();
  BackendBuilder disable_ssl();
  BackendBuilder check_certificate(std::string cert);
  BackendBuilder ca_certificate(std::string cert);
  BackendBuilder tls_ciphers(std::string ciphers);
  BackendBuilder sni_hostname(std::string host);
  BackendBuilder enable_pooling(bool enable);
  BackendBuilder http_keepalive_time(std::chrono::milliseconds timeout);
  BackendBuilder tcp_keepalive_enable(bool enable);
  BackendBuilder tcp_keepalive_interval_secs(uint32_t secs);
  BackendBuilder tcp_keepalive_probes(uint32_t probes);
  BackendBuilder tcp_keepalive_time_secs(uint32_t secs);
  Backend finish();

private:
  rust::Box<fastly::sys::backend::BackendBuilder> builder;
};

} // namespace fastly::backend

#endif
