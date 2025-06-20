#include "backend.h"
#include "sdk-sys.h"
#include <chrono>

namespace fastly::backend {

Backend Backend::from_name(std::string name) {
  return std::move(
      fastly::sys::backend::m_static_backend_backend_from_name(name));
}

BackendBuilder Backend::builder(std::string name, std::string target) {
    return std::move(fastly::sys::backend::m_static_backend_backend_builder(name, target));
}

std::string Backend::into_string() {
    auto name{fastly::sys::backend::m_backend_backend_into_string(std::move(this->backend))};
    std::string ret{name.begin(), name.end()};
    return ret;
}

std::string Backend::name() {
    auto name{this->backend->name()};
    std::string ret{name.begin(), name.end()};
    return ret;
}

bool Backend::exists() {
    return this->backend->exists();
}

bool Backend::is_dynamic() {
    return this->backend->is_dynamic();
}

std::string Backend::get_host() {
    auto host{this->backend->get_host()};
    std::string ret{host.begin(), host.end()};
    return ret;
}

uint16_t Backend::get_port() {
    return this->backend->get_port();
}

std::chrono::milliseconds Backend::get_connect_timeout() {
    return std::chrono::milliseconds(this->backend->get_connect_timeout());
}

std::chrono::milliseconds Backend::get_first_byte_timeout() {
    return std::chrono::milliseconds(this->backend->get_first_byte_timeout());
}

std::chrono::milliseconds Backend::get_between_byte_timeout() {
    return std::chrono::milliseconds(this->backend->get_first_byte_timeout());
}

std::chrono::milliseconds Backend::get_http_keepalive_time() {
    return std::chrono::milliseconds(this->backend->get_http_keepalive_time());
}

bool Backend::get_tcp_keepalive_enable() {
    return this->backend->get_tcp_keepalive_enable();
}

std::chrono::milliseconds Backend::get_tcp_keepalive_interval() {
    return std::chrono::milliseconds(this->backend->get_tcp_keepalive_interval());
}

uint32_t Backend::get_tcp_keepalive_probes() {
    return this->backend->get_tcp_keepalive_probes();
}

std::chrono::milliseconds Backend::get_tcp_keepalive_time() {
    return std::chrono::milliseconds(this->backend->get_tcp_keepalive_time());
}

bool Backend::is_ssl() {
    return this->backend->is_ssl();
}

// TODO(@zkat): optional stuff is weird.
// SslVersion get_ssl_min_version();
// SslVersion get_ssl_max_version();

BackendBuilder *BackendBuilder::override_host(std::string name) {
  this->builder = fastly::sys::backend::m_backend_backend_builder_override_host(
      std::move(this->builder), name);
  return this;
}

BackendBuilder *
BackendBuilder::connect_timeout(std::chrono::milliseconds timeout) {
  this->builder =
      fastly::sys::backend::m_backend_backend_builder_connect_timeout(
          std::move(this->builder), timeout.count());
  return this;
}

BackendBuilder *
BackendBuilder::first_byte_timeout(std::chrono::milliseconds timeout) {
  this->builder =
      fastly::sys::backend::m_backend_backend_builder_first_byte_timeout(
          std::move(this->builder), timeout.count());
  return this;
}

BackendBuilder *
BackendBuilder::between_bytes_timeout(std::chrono::milliseconds timeout) {
  this->builder =
      fastly::sys::backend::m_backend_backend_builder_between_bytes_timeout(
          std::move(this->builder), timeout.count());
  return this;
}

BackendBuilder *BackendBuilder::enable_ssl() {
  this->builder = fastly::sys::backend::m_backend_backend_builder_enable_ssl(
      std::move(this->builder));
  return this;
}

BackendBuilder *BackendBuilder::disable_ssl() {
  this->builder = fastly::sys::backend::m_backend_backend_builder_disable_ssl(
      std::move(this->builder));
  return this;
}

BackendBuilder *BackendBuilder::check_certificate(std::string cert) {
  this->builder =
      fastly::sys::backend::m_backend_backend_builder_check_certificate(
          std::move(this->builder), cert);
  return this;
}

BackendBuilder *BackendBuilder::ca_certificate(std::string cert) {
  this->builder =
      fastly::sys::backend::m_backend_backend_builder_ca_certificate(
          std::move(this->builder), cert);
  return this;
}

BackendBuilder *BackendBuilder::tls_ciphers(std::string ciphers) {
  this->builder = fastly::sys::backend::m_backend_backend_builder_tls_ciphers(
      std::move(this->builder), ciphers);
  return this;
}

BackendBuilder *BackendBuilder::sni_hostname(std::string host) {
  this->builder = fastly::sys::backend::m_backend_backend_builder_sni_hostname(
      std::move(this->builder), host);
  return this;
}

BackendBuilder *BackendBuilder::enable_pooling(bool enable) {
  this->builder =
      fastly::sys::backend::m_backend_backend_builder_enable_pooling(
          std::move(this->builder), enable);
  return this;
}

BackendBuilder *
BackendBuilder::http_keepalive_time(std::chrono::milliseconds time) {
  this->builder =
      fastly::sys::backend::m_backend_backend_builder_http_keepalive_time(
          std::move(this->builder), time.count());
  return this;
}

BackendBuilder *BackendBuilder::tcp_keepalive_enable(bool enable) {
  this->builder =
      fastly::sys::backend::m_backend_backend_builder_tcp_keepalive_enable(
          std::move(this->builder), enable);
  return this;
}

BackendBuilder *BackendBuilder::tcp_keepalive_interval_secs(uint32_t secs) {
  this->builder = fastly::sys::backend::
      m_backend_backend_builder_tcp_keepalive_interval_secs(
          std::move(this->builder), secs);
  return this;
}

BackendBuilder *BackendBuilder::tcp_keepalive_probes(uint32_t probes) {
  this->builder =
      fastly::sys::backend::m_backend_backend_builder_tcp_keepalive_probes(
          std::move(this->builder), probes);
  return this;
}

BackendBuilder *BackendBuilder::tcp_keepalive_time_secs(uint32_t secs) {
  this->builder =
      fastly::sys::backend::m_backend_backend_builder_tcp_keepalive_time_secs(
          std::move(this->builder), secs);
  return this;
}

Backend BackendBuilder::finish() {
  return fastly::sys::backend::m_backend_backend_builder_finish(
      std::move(this->builder));
}

} // namespace fastly::backend
