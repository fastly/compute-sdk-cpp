#ifndef FASTLY_SECURITY_H
#define FASTLY_SECURITY_H

#include <fastly/detail/access_bridge_internals.h>
#include <fastly/expected.h>
#include <fastly/http/body.h>
#include <fastly/http/request.h>
#include <optional>

namespace fastly::security {
/// Configuration for inspecting a `Request` using Security.
class InspectConfig {
public:
  /// Create a new default `InspectConfig`
  InspectConfig() = default;

  /// Specify an explicity client IP address to inspect.
  /// By default, inspect will use the IP address that made the request to the
  /// running Compute service, but you may want to use a different IP when
  /// service chaining or if requests are proxied from outside of Fastly’s
  /// network.
  InspectConfig with_client_ip(std::string ip) && {
    this->client_ip_ = std::move(ip);
    return std::move(*this);
  }

  /// Set a corp name for the configuration.
  InspectConfig with_corp(std::string name) && {
    this->corp_ = std::move(name);
    return std::move(*this);
  }

  /// Set a workspace name for the configuration.
  InspectConfig with_workspace(std::string name) && {
    this->workspace_ = std::move(name);
    return std::move(*this);
  }

  /// Set a buffer size for the response.
  InspectConfig with_buffer_size(std::size_t size) && {
    this->buffer_size_ = size;
    return std::move(*this);
  }

  const std::optional<std::string> &client_ip() const { return client_ip_; }
  const std::optional<std::string> &corp() const { return corp_; }
  const std::optional<std::string> &workspace() const { return workspace_; }
  const std::optional<std::size_t> &buffer_size() const { return buffer_size_; }

private:
  std::optional<std::string> client_ip_;
  std::optional<std::string> corp_;
  std::optional<std::string> workspace_;
  std::optional<std::size_t> buffer_size_;
};
using fastly::sys::security::InspectErrorCode;
using fastly::sys::security::InspectVerdict;
class InspectError {
public:
  InspectError(fastly::sys::security::InspectError *e)
      : err_(rust::Box<fastly::sys::security::InspectError>::from_raw(e)) {};
  InspectError(rust::Box<fastly::sys::security::InspectError> e)
      : err_(std::move(e)) {};
  InspectErrorCode error_code();
  std::string error_msg();
  /// When getting `InspectErrorCode::BufferSizeError`, this can be used to get
  /// the required size of the buffer before re-attempting the call.
  std::optional<std::size_t> required_buffer_size();

private:
  rust::Box<fastly::sys::security::InspectError> err_;
};

/// Results of asking Security to inspect a `Request`
class InspectResponse {
  friend detail::AccessBridgeInternals;

public:
  /// Security status code.
  std::int16_t status() const;
  /// A redirect URL returned from Security
  std::optional<std::string> redirect_url() const;
  /// Tags returned by Security
  std::vector<std::string> tags() const;
  /// Get Security's verdict on how to handle this request.
  InspectVerdict verdict() const;
  /// Get additional information for verdicts where `this->verdict()` is
  /// `Other`.
  std::optional<std::string> unrecognized_verdict_info() const;
  /// How long Security spent determining its verdict.
  std::chrono::milliseconds decision_ms() const;
  /// A redirect URI returned by Security.
  bool is_redirect() const;
  /// Convert a redirect URI returned by Security into a `Response`.
  std::optional<Response> into_redirect();

private:
  rust::Box<fastly::sys::security::InspectResponse> ir_;
  InspectResponse(rust::Box<fastly::sys::security::InspectResponse> ir)
      : ir_(std::move(ir)) {};
};

/// Inspect a `Request` using the [Fastly Next-Gen
/// WAF](https://docs.fastly.com/en/ngwaf/).
tl::expected<InspectResponse, InspectError>
inspect(fastly::http::Request &request, InspectConfig config);

} // namespace fastly::security

#endif
