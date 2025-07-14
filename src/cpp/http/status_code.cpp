#include "status_code.h"

namespace fastly::http {

const StatusCode StatusCode::CONTINUE{100};

const StatusCode StatusCode::SWITCHING_PROTOCOLS{101};

const StatusCode StatusCode::PROCESSING{102};

const StatusCode StatusCode::OK{200};

const StatusCode StatusCode::CREATED{201};

const StatusCode StatusCode::ACCEPTED{202};

const StatusCode StatusCode::NON_AUTHORITATIVE_INFORMATION{203};

const StatusCode StatusCode::NO_CONTENT{204};

const StatusCode StatusCode::RESET_CONTENT{205};

const StatusCode StatusCode::PARTIAL_CONTENT{206};

const StatusCode StatusCode::MULTI_STATUS{207};

const StatusCode StatusCode::ALREADY_REPORTED{208};

const StatusCode StatusCode::IM_USED{226};

const StatusCode StatusCode::MULTIPLE_CHOICES{300};

const StatusCode StatusCode::MOVED_PERMANENTLY{301};

const StatusCode StatusCode::FOUND{302};

const StatusCode StatusCode::SEE_OTHER{303};

const StatusCode StatusCode::NOT_MODIFIED{304};

const StatusCode StatusCode::USE_PROXY{305};

const StatusCode StatusCode::TEMPORARY_REDIRECT{307};

const StatusCode StatusCode::PERMANENT_REDIRECT{308};

const StatusCode StatusCode::BAD_REQUEST{400};

const StatusCode StatusCode::UNAUTHORIZED{401};

const StatusCode StatusCode::PAYMENT_REQUIRED{402};

const StatusCode StatusCode::FORBIDDEN{403};

const StatusCode StatusCode::NOT_FOUND{404};

const StatusCode StatusCode::METHOD_NOT_ALLOWED{405};

const StatusCode StatusCode::NOT_ACCEPTABLE{406};

const StatusCode StatusCode::PROXY_AUTHENTICATION_REQUIRED{407};

const StatusCode StatusCode::REQUEST_TIMEOUT{408};

const StatusCode StatusCode::CONFLICT{409};

const StatusCode StatusCode::GONE{410};

const StatusCode StatusCode::LENGTH_REQUIRED{411};

const StatusCode StatusCode::PRECONDITION_FAILED{412};

const StatusCode StatusCode::PAYLOAD_TOO_LARGE{413};

const StatusCode StatusCode::URI_TOO_LONG{414};

const StatusCode StatusCode::UNSUPPORTED_MEDIA_TYPE{415};

const StatusCode StatusCode::RANGE_NOT_SATISFIABLE{416};

const StatusCode StatusCode::EXPECTATION_FAILED{417};

const StatusCode StatusCode::IM_A_TEAPOT{418};

const StatusCode StatusCode::MISDIRECTED_REQUEST{421};

const StatusCode StatusCode::UNPROCESSABLE_ENTITY{422};

const StatusCode StatusCode::LOCKED{423};

const StatusCode StatusCode::FAILED_DEPENDENCY{424};

const StatusCode StatusCode::TOO_EARLY{425};

const StatusCode StatusCode::UPGRADE_REQUIRED{426};

const StatusCode StatusCode::PRECONDITION_REQUIRED{428};

const StatusCode StatusCode::TOO_MANY_REQUESTS{429};

const StatusCode StatusCode::REQUEST_HEADER_FIELDS_TOO_LARGE{431};

const StatusCode StatusCode::UNAVAILABLE_FOR_LEGAL_REASONS{451};

const StatusCode StatusCode::INTERNAL_SERVER_ERROR{500};

const StatusCode StatusCode::NOT_IMPLEMENTED{501};

const StatusCode StatusCode::BAD_GATEWAY{502};

const StatusCode StatusCode::SERVICE_UNAVAILABLE{503};

const StatusCode StatusCode::GATEWAY_TIMEOUT{504};

const StatusCode StatusCode::HTTP_VERSION_NOT_SUPPORTED{505};

const StatusCode StatusCode::VARIANT_ALSO_NEGOTIATES{506};

const StatusCode StatusCode::INSUFFICIENT_STORAGE{507};

const StatusCode StatusCode::LOOP_DETECTED{508};

const StatusCode StatusCode::NOT_EXTENDED{510};

const StatusCode StatusCode::NETWORK_AUTHENTICATION_REQUIRED{511};

std::optional<StatusCode> StatusCode::from_code(uint16_t code) {
  if (code >= 100 && code < 1000) {
    return {StatusCode(code)};
  } else {
    return std::nullopt;
  }
}

/// Returns the `uint16_t` corresponding to this `StatusCode`.
uint16_t StatusCode::as_code() { return this->value; }

tl::expected<std::optional<std::string>, fastly::FastlyError>
StatusCode::canonical_reason() {
  std::string reason;
  fastly::sys::error::FastlyError *err;
  bool has_reason{fastly::sys::http::f_http_status_code_canonical_reason(
      this->value, reason, err)};
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else if (has_reason) {
    return reason;
  } else {
    return std::nullopt;
  }
}

/// Check if status is within 100-199.
bool StatusCode::is_informational() {
  return this->value >= 100 && this->value < 200;
}

/// Check if status is within 200-299.
bool StatusCode::is_success() {
  return this->value >= 200 && this->value < 300;
}

/// Check if status is within 300-399.
bool StatusCode::is_redirection() {
  return this->value >= 300 && this->value < 400;
}

/// Check if status is within 400-499.
bool StatusCode::is_client_error() {
  return this->value >= 400 && this->value < 500;
}

/// Check if status is within 500-599.
bool StatusCode::is_server_error() {
  return this->value >= 500 && this->value < 600;
}

} // namespace fastly::http
