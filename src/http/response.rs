use std::pin::Pin;

use cxx::{CxxString, CxxVector, UniquePtr};
use http::{HeaderName, HeaderValue};

use crate::{
    backend::Backend,
    error::ErrPtr,
    http::{
        body::{Body, StreamingBody},
        header::HeaderValuesIter,
        request::Request,
    },
    try_fe,
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

    pub fn set_body_text_plain(&mut self, body: &CxxString, mut err: ErrPtr) {
        self.0.set_body_text_plain(try_fe!(err, body.to_str()));
    }

    pub fn set_body_text_html(&mut self, body: &CxxString, mut err: ErrPtr) {
        self.0.set_body_text_html(try_fe!(err, body.to_str()));
    }

    pub fn set_body_octet_stream(&mut self, body: &CxxVector<u8>) {
        self.0.set_body_octet_stream(body.as_slice())
    }

    pub fn get_content_type(&self, out: Pin<&mut CxxString>) -> bool {
        self.0
            .get_content_type()
            .map(|mime| out.push_str(mime.as_ref()))
            .is_some()
    }

    pub fn set_content_type(&mut self, mime: &CxxString) {
        self.0.set_content_type(
            mime.to_str()
                .expect("Invalid UTF-8")
                .parse()
                .expect("Invalid MIME type"),
        )
    }

    pub fn get_content_length(&self, mut out: Pin<&mut usize>) -> bool {
        self.0
            .get_content_length()
            .map(|len| out.set(len))
            .is_some()
    }

    pub fn contains_header(&self, name: &CxxString, mut err: ErrPtr) -> bool {
        self.0
            .contains_header(try_fe!(err, HeaderName::try_from(name.as_bytes())))
    }

    pub fn get_header(&self, name: &CxxString, out: Pin<&mut CxxString>, mut err: ErrPtr) -> bool {
        self.0
            .get_header(try_fe!(err, HeaderName::try_from(name.as_bytes())))
            .map(|header| out.push_bytes(header.as_bytes()))
            .is_some()
    }

    pub fn get_header_all(
        &self,
        name: &CxxString,
        mut out: Pin<&mut *mut HeaderValuesIter>,
        mut err: ErrPtr,
    ) {
        // Yeah. Sorry. Lifetimes :/
        let iter = self
            .0
            .get_header_all(try_fe!(err, HeaderName::try_from(name.as_bytes())))
            .map(|v| {
                let mut vector = CxxVector::new();
                for byte in v.as_bytes() {
                    vector.pin_mut().push(*byte);
                }
                vector
            })
            .collect::<Vec<UniquePtr<CxxVector<u8>>>>();
        out.set(Box::into_raw(Box::new(HeaderValuesIter(Box::new(
            iter.into_iter(),
        )))))
    }

    pub fn set_header(&mut self, name: &CxxString, value: &CxxString, mut err: ErrPtr) {
        self.0.set_header(
            try_fe!(err, HeaderName::try_from(name.as_bytes())),
            try_fe!(err, HeaderValue::try_from(value.as_bytes())),
        );
    }

    pub fn append_header(&mut self, name: &CxxString, value: &CxxString, mut err: ErrPtr) {
        self.0.append_header(
            try_fe!(err, HeaderName::try_from(name.as_bytes())),
            try_fe!(err, HeaderValue::try_from(value.as_bytes())),
        );
    }

    pub fn remove_header(
        &mut self,
        name: &CxxString,
        out: Pin<&mut CxxString>,
        mut err: ErrPtr,
    ) -> bool {
        self.0
            .remove_header(try_fe!(err, HeaderName::try_from(name.as_bytes())))
            .map(|header| out.push_bytes(header.as_bytes()))
            .is_some()
    }

    pub fn set_status(&mut self, status: u16) {
        self.0.set_status(status);
    }

    pub fn get_backend_name(&self, out: Pin<&mut CxxString>) -> bool {
        self.0.get_backend_name().map(|b| out.push_str(b)).is_some()
    }

    pub fn get_backend(&self) -> *mut Backend {
        if let Some(b) = self.0.get_backend() {
            Box::into_raw(Box::new(Backend(b.clone())))
        } else {
            std::ptr::null_mut()
        }
    }

    pub fn get_backend_addr(&self, out: Pin<&mut CxxString>) -> bool {
        self.0
            .get_backend_addr()
            .map(|ip| out.push_str(ip.to_string().as_ref()))
            .is_some()
    }

    pub fn take_backend_request(&mut self) -> *mut Request {
        if let Some(req) = self.0.take_backend_request() {
            Box::into_raw(Box::new(Request(req)))
        } else {
            std::ptr::null_mut()
        }
    }

    pub fn get_ttl(&self, mut out: Pin<&mut u32>) -> bool {
        self.0
            .get_ttl()
            .map(|ttl| ttl.as_millis())
            .map(ensure_u32)
            .map(|ttl| out.set(ttl))
            .is_some()
    }

    pub fn get_age(&self, mut out: Pin<&mut u32>) -> bool {
        self.0
            .get_age()
            .map(|age| age.as_millis())
            .map(ensure_u32)
            .map(|age| out.set(age))
            .is_some()
    }

    pub fn get_stale_while_revalidate(&self, mut out: Pin<&mut u32>) -> bool {
        self.0
            .get_age()
            .map(|swr| swr.as_millis())
            .map(ensure_u32)
            .map(|swr| out.set(swr))
            .is_some()
    }
}

fn ensure_u32(num: u128) -> u32 {
    if num > std::u32::MAX as u128 {
        std::u32::MAX
    } else {
        num as u32
    }
}
