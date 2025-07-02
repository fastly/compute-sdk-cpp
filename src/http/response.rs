use std::pin::Pin;

use cxx::{CxxString, CxxVector, UniquePtr};

use crate::{
    backend::Backend,
    http::{
        body::{Body, StreamingBody},
        header::HeaderValuesIter,
        request::Request,
    },
};

pub struct Response(pub(crate) fastly::Response);

pub fn m_static_http_response_new() -> Box<Response> {
    Box::new(Response(fastly::Response::new()))
}

pub fn m_http_response_send_to_client(response: Box<Response>) {
    response.0.send_to_client();
}

pub fn m_http_response_stream_to_client(response: Box<Response>) -> Box<StreamingBody> {
    Box::new(StreamingBody(response.0.stream_to_client()))
}

pub fn m_static_http_response_from_body(body: Box<Body>) -> Box<Response> {
    Box::new(Response(fastly::Response::from_body(body.0)))
}

pub fn m_static_http_response_from_status(status: u16) -> Box<Response> {
    Box::new(Response(fastly::Response::from_status(status)))
}

pub fn m_static_http_response_see_other(destination: &CxxString) -> Box<Response> {
    Box::new(Response(fastly::Response::see_other(
        destination.as_bytes(),
    )))
}

pub fn m_static_http_response_redirect(destination: &CxxString) -> Box<Response> {
    Box::new(Response(fastly::Response::redirect(destination.as_bytes())))
}

pub fn m_static_http_response_temporary_redirect(destination: &CxxString) -> Box<Response> {
    Box::new(Response(fastly::Response::temporary_redirect(
        destination.as_bytes(),
    )))
}

pub fn m_http_response_into_body(response: Box<Response>) -> Box<Body> {
    Box::new(Body(response.0.into_body()))
}

impl Response {
    pub fn is_from_backend(&self) -> bool {
        self.0.is_from_backend()
    }

    pub fn clone_without_body(&self) -> Box<Response> {
        Box::new(Response(self.0.clone_without_body()))
    }

    pub fn clone_with_body(&mut self) -> Box<Response> {
        Box::new(Response(self.0.clone_with_body()))
    }

    pub fn has_body(&self) -> bool {
        self.0.has_body()
    }

    pub fn set_body(&mut self, body: Box<Body>) {
        self.0.set_body(body.0);
    }

    pub fn take_body(&mut self) -> Box<Body> {
        Box::new(Body(self.0.take_body()))
    }

    pub fn append_body(&mut self, other: Box<Body>) {
        self.0.append_body(other.0);
    }

    pub fn set_body_text_plain(&mut self, body: &CxxString) {
        self.0.set_body_text_plain(body.to_string_lossy().as_ref())
    }

    pub fn set_body_text_html(&mut self, body: &CxxString) {
        self.0.set_body_text_html(body.to_string_lossy().as_ref())
    }

    pub fn set_body_octet_stream(&mut self, body: &CxxVector<u8>) {
        self.0.set_body_octet_stream(body.as_slice())
    }

    pub fn get_content_type(&self) -> *const CxxVector<u8> {
        if let Some(mime) = self.0.get_content_type() {
            let mut vec: UniquePtr<CxxVector<u8>> = CxxVector::new();
            for b in mime.as_ref().as_bytes() {
                vec.pin_mut().push(*b);
            }
            // SAFETY: we're no longer responsible for this vec, so YOLO
            vec.into_raw()
        } else {
            std::ptr::null()
        }
    }

    pub fn set_content_type(&mut self, mime: &CxxString) {
        self.0.set_content_type(
            mime.to_string_lossy()
                .as_ref()
                .parse()
                .expect("Invalid MIME type"),
        )
    }

    pub fn get_content_length(&self) -> *const usize {
        if let Some(len) = self.0.get_content_length() {
            Box::into_raw(Box::new(len))
        } else {
            std::ptr::null()
        }
    }

    pub fn contains_header(&self, name: &CxxString) -> bool {
        self.0.contains_header(name.as_bytes())
    }

    pub fn get_header(&self, name: &CxxString) -> *const CxxVector<u8> {
        if let Some(header) = self.0.get_header(name.as_bytes()) {
            let mut vec: UniquePtr<CxxVector<u8>> = CxxVector::new();
            for b in header.as_bytes() {
                vec.pin_mut().push(*b);
            }
            // SAFETY: we're no longer responsible for this vec, so YOLO
            vec.into_raw()
        } else {
            std::ptr::null()
        }
    }

    pub fn get_header_all(&self, name: &CxxString) -> Box<HeaderValuesIter> {
        // Yeah. Sorry. Lifetimes :/
        let iter = self
            .0
            .get_header_all(name.as_bytes())
            .map(|v| {
                let mut vector = CxxVector::new();
                for byte in v.as_bytes() {
                    vector.pin_mut().push(*byte);
                }
                vector
            })
            .collect::<Vec<UniquePtr<CxxVector<u8>>>>();
        Box::new(HeaderValuesIter(Box::new(iter.into_iter())))
    }

    pub fn set_header(&mut self, name: &CxxString, value: &CxxString) {
        self.0.set_header(name.as_bytes(), value.as_bytes());
    }

    pub fn append_header(&mut self, name: &CxxString, value: &CxxString) {
        self.0.append_header(name.as_bytes(), value.as_bytes());
    }

    pub fn remove_header(&mut self, name: &CxxString) -> *const CxxVector<u8> {
        if let Some(header) = self.0.remove_header(name.as_bytes()) {
            let mut vec: UniquePtr<CxxVector<u8>> = CxxVector::new();
            for b in header.as_bytes() {
                vec.pin_mut().push(*b);
            }
            // SAFETY: we're no longer responsible for this vec, so YOLO
            vec.into_raw()
        } else {
            std::ptr::null()
        }
    }

    pub fn set_status(&mut self, status: u16) {
        self.0.set_status(status);
    }

    pub fn get_backend_name(&self, out: Pin<&mut CxxString>) -> bool {
        if let Some(b) = self.0.get_backend_name() {
            out.push_str(b);
            true
        } else {
            false
        }
    }

    pub fn get_backend(&self) -> *mut Backend {
        if let Some(b) = self.0.get_backend() {
            Box::into_raw(Box::new(Backend(b.clone())))
        } else {
            std::ptr::null_mut()
        }
    }

    pub fn get_backend_addr(&self, out: Pin<&mut CxxString>) -> bool {
        if let Some(ip) = self.0.get_backend_addr() {
            out.push_str(ip.to_string().as_ref());
            true
        } else {
            false
        }
    }

    pub fn take_backend_request(&mut self) -> *mut Request {
        if let Some(req) = self.0.take_backend_request() {
            Box::into_raw(Box::new(Request(req)))
        } else {
            std::ptr::null_mut()
        }
    }

    pub fn get_ttl(&self) -> *const u32 {
        if let Some(ttl) = self.0.get_ttl() {
            Box::into_raw(Box::new(ensure_u32(ttl.as_millis())))
        } else {
            std::ptr::null()
        }
    }

    pub fn get_age(&self) -> *const u32 {
        if let Some(age) = self.0.get_age() {
            Box::into_raw(Box::new(ensure_u32(age.as_millis())))
        } else {
            std::ptr::null()
        }
    }

    pub fn get_stale_while_revalidate(&self) -> *const u32 {
        if let Some(swr) = self.0.get_stale_while_revalidate() {
            Box::into_raw(Box::new(ensure_u32(swr.as_millis())))
        } else {
            std::ptr::null()
        }
    }
}

fn ensure_u32(num: u128) -> u32 {
    if num > std::u32::MAX as u128 {
        std::u32::MAX
    } else {
        num as u32
    }
}
