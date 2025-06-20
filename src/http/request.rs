use cxx::{CxxString, CxxVector, UniquePtr};

use crate::ffi::Method;
use crate::http::body::Body;
use crate::http::header::HeaderIter;
use crate::http::response::Response;

pub struct Request(fastly::Request);

pub fn m_static_http_request_new(method: Method, url: &CxxString) -> Box<Request> {
    let method: fastly::http::Method = method.into();
    // TODO(@zkat): not liking this .to_string_lossy()
    Box::new(Request(fastly::Request::new(
        method,
        url.to_string_lossy().as_ref(),
    )))
}

pub fn m_static_http_request_get(url: &CxxString) -> Box<Request> {
    // TODO(@zkat): not liking this .to_string_lossy()
    Box::new(Request(fastly::Request::get(
        url.to_string_lossy().as_ref(),
    )))
}

pub fn m_static_http_request_head(url: &CxxString) -> Box<Request> {
    // TODO(@zkat): not liking this .to_string_lossy()
    Box::new(Request(fastly::Request::head(
        url.to_string_lossy().as_ref(),
    )))
}

pub fn m_static_http_request_post(url: &CxxString) -> Box<Request> {
    // TODO(@zkat): not liking this .to_string_lossy()
    Box::new(Request(fastly::Request::post(
        url.to_string_lossy().as_ref(),
    )))
}

pub fn m_static_http_request_put(url: &CxxString) -> Box<Request> {
    // TODO(@zkat): not liking this .to_string_lossy()
    Box::new(Request(fastly::Request::put(
        url.to_string_lossy().as_ref(),
    )))
}

pub fn m_static_http_request_delete(url: &CxxString) -> Box<Request> {
    // TODO(@zkat): not liking this .to_string_lossy()
    Box::new(Request(fastly::Request::delete(
        url.to_string_lossy().as_ref(),
    )))
}

pub fn m_static_http_request_connect(url: &CxxString) -> Box<Request> {
    // TODO(@zkat): not liking this .to_string_lossy()
    Box::new(Request(fastly::Request::connect(
        url.to_string_lossy().as_ref(),
    )))
}

pub fn m_static_http_request_options(url: &CxxString) -> Box<Request> {
    // TODO(@zkat): not liking this .to_string_lossy()
    Box::new(Request(fastly::Request::options(
        url.to_string_lossy().as_ref(),
    )))
}

pub fn m_static_http_request_trace(url: &CxxString) -> Box<Request> {
    // TODO(@zkat): not liking this .to_string_lossy()
    Box::new(Request(fastly::Request::trace(
        url.to_string_lossy().as_ref(),
    )))
}

pub fn m_static_http_request_patch(url: &CxxString) -> Box<Request> {
    // TODO(@zkat): not liking this .to_string_lossy()
    Box::new(Request(fastly::Request::patch(
        url.to_string_lossy().as_ref(),
    )))
}

pub fn m_static_http_request_from_client() -> Box<Request> {
    Box::new(Request(fastly::Request::from_client()))
}

pub fn m_http_request_send(request: Box<Request>, backend: &CxxString) -> Box<Response> {
    // TODO(@zkat): need a better error situation.
    Box::new(Response(
        request
            .0
            .send(backend.to_string_lossy().as_ref())
            .expect("Request failed to send."),
    ))
}

pub fn m_http_request_into_body_bytes(request: Box<Request>) -> UniquePtr<CxxVector<u8>> {
    let mut vec = CxxVector::new();
    for b in request.0.into_body_bytes() {
        vec.pin_mut().push(b);
    }
    vec
}

impl Request {
    pub fn is_from_client(&self) -> bool {
        self.0.is_from_client()
    }
    pub fn clone_without_body(&self) -> Box<Request> {
        Box::new(Request(self.0.clone_without_body()))
    }
    pub fn clone_with_body(&mut self) -> Box<Request> {
        Box::new(Request(self.0.clone_with_body()))
    }
    pub fn get_header_all(&self, name: &CxxString) -> Box<HeaderIter> {
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
        Box::new(HeaderIter(Box::new(iter.into_iter())))
    }

    // TODO(@zkat): cxx doesn't support function pointers
    // pub fn set_after_send(&mut self, _f: unsafe extern "C" fn(*mut Request)) {
    //     unimplemented!()
    // }

    pub fn set_auto_decompress_gzip(&mut self, gzip: bool) {
        self.0.set_auto_decompress_gzip(gzip);
    }

    // TODO(@zkat): cxx doesn't support function pointers
    // pub fn set_before_send(&mut self, _f: unsafe extern "C" fn(*mut Request)) -> Box<Request> {
    //     unimplemented!()
    // }

    pub fn set_body(&mut self, body: Box<Body>) {
        self.0.set_body(body.0);
    }

    pub fn has_body(&self) -> bool {
        self.0.has_body()
    }

    pub fn take_body(&mut self) -> Box<Body> {
        Box::new(Body(self.0.take_body()))
    }
}
