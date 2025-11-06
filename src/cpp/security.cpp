#include "util.h"
#include <fastly/sdk-sys.h>
#include <fastly/security.h>

namespace fastly::security {
InspectErrorCode InspectError::error_code() { return err_->error_code(); }
std::string InspectError::error_msg() {
  std::string msg;
  err_->error_msg(msg);
  return msg;
}
std::optional<std::size_t> InspectError::required_buffer_size() {
  std::size_t buf_size{0};
  if (err_->required_buffer_size(buf_size)) {
    return buf_size;
  } else {
    return std::nullopt;
  }
}

std::int16_t InspectResponse::status() const { return ir_->status(); }

std::optional<std::string> InspectResponse::redirect_url() const {
  std::string ret;
  if (ir_->redirect_url(ret)) {
    return ret;
  }
  return std::nullopt;
}

std::vector<std::string> InspectResponse::tags() const {
  auto tags = ir_->tags();
  std::vector<std::string> ret;
  ret.resize(tags.size());
  std::transform(tags.begin(), tags.end(), std::back_inserter(ret),
                 [](auto str) { return std::string(str); });
  return ret;
}

InspectVerdict InspectResponse::verdict() const { return ir_->verdict(); }
std::optional<std::string> InspectResponse::unrecognized_verdict_info() const {
  std::string ret;
  if (ir_->unrecognized_verdict_info(ret)) {
    return ret;
  }
  return std::nullopt;
}

std::chrono::milliseconds InspectResponse::decision_ms() const {
  return std::chrono::milliseconds(ir_->decision_ms());
}

bool InspectResponse::is_redirect() const { return ir_->is_redirect(); }
std::optional<Response> InspectResponse::into_redirect() {
  fastly::sys::http::Response *resp;
  if (fastly::sys::security::m_security_inspect_response_into_redirect(
          std::move(ir_), resp)) {
    return detail::AccessBridgeInternals::from_raw<Response>(resp);
  }
  return std::nullopt;
}

tl::expected<InspectResponse, InspectError>
inspect(fastly::http::Request &request, InspectConfig config) {
  fastly::sys::security::InspectResponse *out;
  fastly::sys::security::InspectError *err;
  auto client_ip = config.client_ip() ? &*config.client_ip() : nullptr;
  auto corp = config.corp() ? &*config.corp() : nullptr;
  auto workspace = config.workspace() ? &*config.workspace() : nullptr;
  auto buffer_size = config.buffer_size() ? &*config.buffer_size() : nullptr;
  fastly::sys::security::f_security_lookup(
      *detail::AccessBridgeInternals::get(request), client_ip, corp, workspace,
      buffer_size, out, err);
  if (err == nullptr) {
    return detail::AccessBridgeInternals::from_raw<InspectResponse>(out);
  } else {
    return tl::unexpected(err);
  }
}
} // namespace fastly::security
