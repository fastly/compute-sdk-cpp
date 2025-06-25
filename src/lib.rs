use backend::*;
use http::{body::*, header::*, request::*, response::*};

mod backend;
mod http;

// Unfortunately, due to some limitations with cxx, the ENTIRE bridge basically
// has to be under a single ffi module, or cross-referencing ffi types is gonna
// break.
#[cxx::bridge]
mod ffi {
    #[namespace = "fastly::sys::http"]
    #[derive(Copy, Clone, Debug)]
    pub enum Method {
        GET,
        POST,
        PUT,
        DELETE,
        HEAD,
        OPTIONS,
        CONNECT,
        PATCH,
        TRACE,
    }

    #[namespace = "fastly::sys::backend"]
    extern "Rust" {
        type Backend;
        fn m_static_backend_backend_from_name(name: &CxxString) -> Box<Backend>;
        fn m_static_backend_backend_builder(name: &CxxString, target: &CxxString) -> Box<BackendBuilder>;
        fn m_backend_backend_into_string(backend: Box<Backend>) -> String;
        fn name(&self) -> &str;
        fn exists(&self) -> bool;
        fn is_dynamic(&self) -> bool;
        fn get_host(&self) -> String;
        // TODO
        // pub fn get_host_override(&self) {
        // }
        fn get_port(&self) -> u16;
        fn get_connect_timeout(&self) -> u32;
        fn get_first_byte_timeout(&self) -> u32;
        fn get_between_bytes_timeout(&self) -> u32;
        fn get_http_keepalive_time(&self) -> u32;
        fn get_tcp_keepalive_enable(&self) -> bool;
        fn get_tcp_keepalive_interval(&self) -> u32;
        fn get_tcp_keepalive_probes(&self) -> u32;
        fn get_tcp_keepalive_time(&self) -> u32;
        fn is_ssl(&self) -> bool;
    }

    #[namespace = "fastly::sys::backend"]
    extern "Rust" {
        type BackendBuilder;
        fn m_static_backend_backend_builder_new(name: &CxxString, target: &CxxString) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_override_host(mut builder: Box<BackendBuilder>, name: &CxxString) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_connect_timeout(mut builder: Box<BackendBuilder>, timeout: u32) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_first_byte_timeout(mut builder: Box<BackendBuilder>, timeout: u32) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_between_bytes_timeout(mut builder: Box<BackendBuilder>, timeout: u32) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_enable_ssl(builder: Box<BackendBuilder>) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_disable_ssl(builder: Box<BackendBuilder>) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_check_certificate(builder: Box<BackendBuilder>, cert: &CxxString) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_ca_certificate(builder: Box<BackendBuilder>, cert: &CxxString) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_tls_ciphers(builder: Box<BackendBuilder>, ciphers: &CxxString) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_sni_hostname(builder: Box<BackendBuilder>, host: &CxxString) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_enable_pooling(builder: Box<BackendBuilder>, value: bool) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_http_keepalive_time(mut builder: Box<BackendBuilder>, timeout: u32) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_tcp_keepalive_enable(builder: Box<BackendBuilder>, value: bool) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_tcp_keepalive_interval_secs(builder: Box<BackendBuilder>, value: u32) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_tcp_keepalive_probes(builder: Box<BackendBuilder>, value: u32) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_tcp_keepalive_time_secs(builder: Box<BackendBuilder>, value: u32) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_finish(builder: Box<BackendBuilder>) -> Box<Backend>;
    }

    #[namespace = "fastly::sys::http"]
    extern "Rust" {
        type HeaderValuesIter;
        fn next(&mut self) -> UniquePtr<CxxVector<u8>>;
    }

    #[namespace = "fastly::sys::http"]
    extern "Rust" {
        type Request;

        // Static methods
        fn m_static_http_request_get(url: &CxxString) -> Box<Request>;
        fn m_static_http_request_head(url: &CxxString) -> Box<Request>;
        fn m_static_http_request_post(url: &CxxString) -> Box<Request>;
        fn m_static_http_request_put(url: &CxxString) -> Box<Request>;
        fn m_static_http_request_delete(url: &CxxString) -> Box<Request>;
        fn m_static_http_request_connect(url: &CxxString) -> Box<Request>;
        fn m_static_http_request_options(url: &CxxString) -> Box<Request>;
        fn m_static_http_request_trace(url: &CxxString) -> Box<Request>;
        fn m_static_http_request_patch(url: &CxxString) -> Box<Request>;
        fn m_static_http_request_new(method: Method, url: &CxxString) -> Box<Request>;
        fn m_static_http_request_from_client() -> Box<Request>;

        // Regular methods
        fn is_from_client(&self) -> bool;
        fn clone_without_body(&self) -> Box<Request>;
        fn clone_with_body(&mut self) -> Box<Request>;
        fn m_http_request_send(request: Box<Request>, backend: &Box<Backend>) -> Box<Response>;
        fn set_body(&mut self, body: Box<Body>);
        fn has_body(&self) -> bool;
        fn take_body(&mut self) -> Box<Body>;
        fn m_http_request_into_body(request: Box<Request>) -> Box<Body>;
        fn set_body_text_plain(&mut self, body: &CxxString);
        fn set_body_text_html(&mut self, body: &CxxString);
        fn set_body_octet_stream(&mut self, body: &CxxVector<u8>);
        fn get_content_type(&self) -> *const CxxVector<u8>;
        fn set_content_type(&mut self, mime: &CxxString);
        fn get_content_length(&self) -> *const usize;
        fn contains_header(&self, name: &CxxString) -> bool;
        fn get_header(&self, name: &CxxString) -> *const CxxVector<u8>;
        fn get_header_all(&self, name: &CxxString) -> Box<HeaderValuesIter>;
        fn set_header(&mut self, name: &CxxString, value: &CxxString);
        fn append_header(&mut self, name: &CxxString, value: &CxxString);
        fn remove_header(&mut self, name: &CxxString) -> *const CxxVector<u8>;
        fn get_method(&self) -> Method;
        fn set_method(&mut self, method: Method);
        fn get_url(&self) -> UniquePtr<CxxVector<u8>>;
        fn set_url(&mut self, url: &CxxString);
        fn get_path(&self) -> UniquePtr<CxxVector<u8>>;
        fn set_path(&mut self, path: &CxxString);
        fn get_query_string(&self) -> *const CxxVector<u8>;
        fn get_query_parameter(&self, param: &CxxString) -> *const CxxVector<u8>;
        fn set_query_string(&mut self, qs: &CxxString);
        fn remove_query(&mut self);
        fn get_client_ddos_detected(&self) -> *const bool;
        fn set_auto_decompress_gzip(&mut self, gzip: bool);
        fn fastly_key_is_valid(&self) -> bool;
        fn set_cache_key(&mut self, key: &CxxVector<u8>);
        fn is_cacheable(&mut self) -> bool;
    }

    #[namespace = "fastly::sys::http"]
    extern "Rust" {
        type Response;
        fn m_static_http_response_new() -> Box<Response>;
        fn m_static_http_response_from_body(body: Box<Body>) -> Box<Response>;
        fn m_http_response_send_to_client(response: Box<Response>);
        fn set_body(&mut self, body: Box<Body>);
        fn take_body(&mut self) -> Box<Body>;
    }

    #[namespace = "fastly::sys::http"]
    extern "Rust" {
        type Body;
        fn m_static_http_body_new() -> Box<Body>;
        fn append(&mut self, other: Box<Body>);
        fn append_trailer(&mut self, name: &CxxString, value: &CxxString);
        fn read(&mut self, buf: &mut [u8]) -> usize;
        fn write(&mut self, bytes: &[u8]) -> usize;
    }

    #[namespace = "fastly::sys::http"]
    extern "Rust" {
        type StreamingBody;
        fn m_http_streaming_body_finish(body: Box<StreamingBody>);
        fn append(&mut self, other: Box<Body>);
        fn append_trailer(&mut self, name: &CxxString, value: &CxxString);
        fn write(&mut self, bytes: &[u8]) -> usize;
    }
}
