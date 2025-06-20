use http::{body::*, header::*, request::*, response::*};

mod http;

// Unfortunately, due to some limitations with cxx, the ENTIRE bridge basically
// has to be under a single ffi module, or cross-referencing ffi types is gonna
// break.
#[cxx::bridge]
mod ffi {
    #[namespace = "fastly::sys::http"]
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

    #[namespace = "fastly::sys::http"]
    extern "Rust" {
        type HeaderIter;
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
        fn m_http_request_send(request: Box<Request>, backend: &CxxString) -> Box<Response>;
        fn is_from_client(&self) -> bool;
        fn clone_without_body(&self) -> Box<Request>;
        fn clone_with_body(&mut self) -> Box<Request>;
        fn get_header_all(&self, name: &CxxString) -> Box<HeaderIter>;
        fn set_auto_decompress_gzip(&mut self, gzip: bool);
        fn set_body(&mut self, body: Box<Body>);
        fn has_body(&self) -> bool;
        fn take_body(&mut self) -> Box<Body>;
        fn m_http_request_into_body_bytes(request: Box<Request>) -> UniquePtr<CxxVector<u8>>;
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
