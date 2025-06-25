#ifndef FASTLY_HTTP_REQUEST_H
#define FASTLY_HTTP_REQUEST_H

#include "../sdk-sys.h"
#include "backend.h"
#include "body.h"
#include "header.h"
#include "method.h"
#include "response.h"
#include <iterator>
#include <optional>
#include <string>
#include <vector>

namespace fastly::http {

class Request {
public:
  Request(Method method, std::string url);
  static Request get(std::string url);
  static Request head(std::string url);
  static Request post(std::string url);
  static Request put(std::string url);
  static Request delete_(std::string url);
  static Request connect(std::string url);
  static Request options(std::string url);
  static Request trace(std::string url);
  static Request patch(std::string url);
  static Request from_client();
  bool is_from_client();
  Request clone_without_body();
  Request clone_with_body();
  Response send(fastly::backend::Backend backend);
  // PendingRequest send_async(fastly::backend::Backend backend);
  // std::pair<fastly::http::StreamingBody, PendingRequest> send_async(fastly::backend::Backend backend);
  Request *with_body(Body body);
  bool has_body();
  Body take_body();
  void set_body(Body body);
  void append_body(Body body);
  std::vector<uint8_t> into_body_bytes();
  std::string into_body_string();
  Body into_body();
  Request *with_body_text_plain(std::string body);
  void set_body_text_plain(std::string body);
  Request *with_body_text_html(std::string body);
  void set_body_text_html(std::string body);
  std::string take_body_string();
  Request *with_body_octet_stream(std::vector<uint8_t> body);
  void set_body_octet_stream(std::vector<uint8_t> body);
  std::vector<uint8_t> take_body_bytes();
  // ChunksIter read_body_chunks(size_t chunk_size);
  std::optional<std::string> get_content_type();
  Request *with_content_type(std::string mime);
  void set_content_type(std::string mime);
  std::optional<size_t> get_content_length();
  bool contains_header(std::string name);
  Request* with_header(std::string name, std::string value);
  Request* with_set_header(std::string name, std::string value);
  // TODO(@zkat): do a proper HeaderValue situation here?
  std::optional<std::string> get_header(std::string name);
  HeaderValuesIter get_header_all(std::string name);
  // TODO(@zkat): sigh. IDK
  // ??? get_headers();
  // HeaderNamesIter get_header_names();
  void set_header(std::string name, std::string value);
  void append_header(std::string name, std::string value);
  std::optional<std::string> remove_header(std::string name);
  Request *with_method(Method method);
  Method get_method();
  void set_method(Method method);
  // TODO(@zkat): Actual URL object?
  Request *with_url(std::string url);
  std::string get_url();
  void set_url(std::string url);
  std::string get_path();
  Request *with_path(std::string path);
  void set_path(std::string path);
  std::optional<std::string> get_query_string();
  std::optional<std::string> get_query_parameter(std::string param);
  Request *with_query_string(std::string query);
  void set_query_string(std::string query);
  void remove_query();
  // TODO(@zkat): need Version enum
  // Request *with_version(Version version);
  // Version get_version();
  // void set_version(Version version);
  Request *with_pass(bool pass);
  void set_pass(bool pass);
  Request *with_ttl(uint32_t ttl);
  void set_ttl(uint32_t ttl);
  Request *with_stale_while_revalidate(uint32_t swr);
  void set_stale_while_revalidate(uint32_t swr);
  Request *with_pci(bool pci);
  void set_pci(bool pci);
  Request *with_surrogate_key(std::string sk);
  void set_surrogate_key(std::string sk);
  // TODO(@zkat): needs an IpAddr situation.
  // std::optional<IpAddr> get_client_ip_addr();
  // std::optional<IpAddr> get_server_ip_addr();
  // TODO(@zkat): needs iterator
  // std::optional<HeaderNameIter> get_original_header_names();
  std::optional<uint32_t> get_original_header_count();
  std::optional<bool> get_client_ddos_detected();
  std::optional<std::vector<uint8_t>> get_tls_client_hello();
  std::optional<std::array<uint8_t, 16>> get_tls_ja3_md5();
  std::optional<std::string> get_tls_ja4();
  std::optional<std::string> get_tls_raw_client_certificate();
  std::optional<std::vector<uint8_t>> get_tls_raw_client_certificate_bytes();
  // TODO(@zkat): needs additional type
  // std::optional<ClientCertVerifyResult> get_tls_client_cert_verify_result();
  std::optional<std::string> get_tls_cipher_openssl_name();
  std::optional<std::vector<uint8_t>> get_tls_cipher_openssl_name_bytes();
  std::optional<std::vector<uint8_t>> get_tls_protocol_bytes();
  void set_auto_decompress_gzip(bool gzip);
  Request *with_auto_decompress_gzip(bool gzip);
  // TODO(@zkat): needs enum
  // void set_framing_headers_mode(FramingHeadersMode mode);
  // Request *set_framing_headers_mode(FramingHeadersMode mode);
  bool fastly_key_is_valid();
  void handoff_websocket(fastly::backend::Backend backend);
  void handoff_fanout(fastly::backend::Backend backend);
  Request *on_behalf_of(std::string service);
  void set_cache_key(std::string key);
  void set_cache_key(std::vector<uint8_t> key);
  Request *with_cache_key(std::string key);
  Request *with_cache_key(std::vector<uint8_t> key);
  bool is_cacheable();
  
private:
  Request(rust::Box<fastly::sys::http::Request> r) : req(std::move(r)) {};
  rust::Box<fastly::sys::http::Request> req;
};

} // namespace fastly::http

namespace fastly {
using fastly::http::Request;
}

#endif
