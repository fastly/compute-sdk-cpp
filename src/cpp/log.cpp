#include "log.h"
#include "sdk-sys.h"

namespace fastly::log {

LogLevelFilter max_level() { return fastly::sys::log::f_log_max_level(); }

void set_max_level(LogLevelFilter level) {
  fastly::sys::log::f_log_set_max_level(level);
}

std::string Endpoint::name() {
  std::string out;
  this->ep->name(out);
  return out;
}

fastly::expected<Endpoint> Endpoint::from_name(std::string_view name) {
  fastly::sys::log::Endpoint *out;
  fastly::sys::error::FastlyError *err;
  fastly::sys::log::m_static_log_endpoint_try_from_name(
      static_cast<std::string>(name), out, err);
  if (err != nullptr) {
    return FSLY_BOX(log, Endpoint, out);
  } else {
    return fastly::unexpected(err);
  }
}

void init_simple(Endpoint endpoint, LogLevelFilter level) {
  fastly::sys::log::f_log_init_simple(std::move(endpoint.ep), level);
}

void init_simple(std::string_view endpoint, LogLevelFilter level) {
  init_simple(Endpoint::from_name(std::move(endpoint)).value(), level);
}

void init_simple(Endpoint endpoint) {
  init_simple(std::move(endpoint), LogLevelFilter::Info);
}

void init_simple(std::string_view endpoint) {
  init_simple(std::move(endpoint), LogLevelFilter::Info);
}

LoggerBuilder LoggerBuilder::endpoint(Endpoint ep) && {
  this->lb->endpoint(std::move(ep.ep));
  return std::move(*this);
}

LoggerBuilder LoggerBuilder::endpoint(std::string_view ep) && {
  return std::move(*this).endpoint(Endpoint::from_name(std::move(ep)).value());
}

LoggerBuilder LoggerBuilder::endpoint_level(Endpoint ep,
                                            LogLevelFilter level) && {
  this->lb->endpoint_level(std::move(ep.ep), level);
  return std::move(*this);
}

LoggerBuilder LoggerBuilder::endpoint_level(std::string_view ep,
                                            LogLevelFilter level) && {
  return std::move(*this).endpoint_level(
      Endpoint::from_name(std::move(ep)).value(), level);
}

LoggerBuilder LoggerBuilder::default_endpoint(Endpoint ep) && {
  this->lb->default_endpoint(std::move(ep.ep));
  return std::move(*this);
}

LoggerBuilder LoggerBuilder::default_endpoint(std::string_view ep) && {
  return std::move(*this).default_endpoint(
      Endpoint::from_name(std::move(ep)).value());
}

LoggerBuilder LoggerBuilder::default_level_endpoint(Endpoint ep,
                                                    LogLevel level) && {
  this->lb->default_level_endpoint(std::move(ep.ep), level);
  return std::move(*this);
}

LoggerBuilder LoggerBuilder::default_level_endpoint(std::string_view ep,
                                                    LogLevel level) && {
  return std::move(*this).default_level_endpoint(
      Endpoint::from_name(std::move(ep)).value(), level);
}

LoggerBuilder LoggerBuilder::max_level(LogLevelFilter level) && {
  this->lb->max_level(level);
  return std::move(*this);
}

LoggerBuilder LoggerBuilder::echo_stdout(bool enabled) && {
  this->lb->echo_stdout(enabled);
  return std::move(*this);
}

LoggerBuilder LoggerBuilder::echo_stderr(bool enabled) && {
  this->lb->echo_stderr(enabled);
  return std::move(*this);
}

void LoggerBuilder::init() { this->lb->init(); }

} // namespace fastly::log
