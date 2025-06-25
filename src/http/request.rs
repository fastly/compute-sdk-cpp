use cxx::{CxxString, CxxVector, UniquePtr};

use crate::backend::Backend;
use crate::ffi::Method;
use crate::http::body::Body;
use crate::http::header::HeaderValuesIter;
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

pub fn m_http_request_send(request: Box<Request>, backend: &Box<Backend>) -> Box<Response> {
    // TODO(@zkat): need a better error situation.
    Box::new(Response(
        request
            .0
            .send(&backend.0)
            .expect("Request failed to send."),
    ))
}

pub fn m_http_request_into_body(request: Box<Request>) -> Box<Body> {
    Box::new(Body(request.0.into_body()))
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
        self.0.set_content_type(mime.to_string_lossy().as_ref().parse().expect("Invalid MIME type"))
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
    
    pub fn get_method(&self) -> Method {
        self.0.get_method().clone().into()
    }

    pub fn set_method(&mut self, method: Method) {
        let method: fastly::http::Method = method.into();
        self.0.set_method(method);
    }
    
    pub fn get_url(&self) -> UniquePtr<CxxVector<u8>> {
        let mut vec = CxxVector::new();
        for b in self.0.get_url().as_str().as_bytes() {
            vec.pin_mut().push(*b);
        }
        vec
    }
    
    pub fn set_path(&mut self, path: &CxxString) {
        self.0.set_path(path.to_string_lossy().as_ref());
    }
    
    pub fn get_path(&self) -> UniquePtr<CxxVector<u8>> {
        let mut vec = CxxVector::new();
        for b in self.0.get_path().as_bytes() {
            vec.pin_mut().push(*b);
        }
        vec
    }
    
    pub fn set_url(&mut self, url: &CxxString) {
        self.0.set_url(url.to_string_lossy().as_ref());
    }
    
    pub fn get_query_string(&self) -> *const CxxVector<u8> {
        if let Some(qs) = self.0.get_query_str() {
            let mut vec: UniquePtr<CxxVector<u8>> = CxxVector::new();
            for b in qs.as_bytes() {
                vec.pin_mut().push(*b);
            }
            // SAFETY: we're no longer responsible for this vec, so YOLO
            vec.into_raw()
        } else {
            std::ptr::null()
        }
    }
    
    pub fn get_query_parameter(&self, param: &CxxString) -> *const CxxVector<u8> {
        if let Some(qp) = self.0.get_query_parameter(param.to_string_lossy().as_ref()) {
            let mut vec: UniquePtr<CxxVector<u8>> = CxxVector::new();
            for b in qp.as_bytes() {
                vec.pin_mut().push(*b);
            }
            // SAFETY: we're no longer responsible for this vec, so YOLO
            vec.into_raw()
        } else {
            std::ptr::null()
        }
    }
    
    pub fn set_query_string(&mut self, qs: &CxxString) {
        self.0.set_query_str(qs.to_string_lossy());
    }
    
    pub fn remove_query(&mut self) {
        self.0.remove_query();
    }
    
    pub fn get_client_ddos_detected(&self) -> *const bool {
        if let Some(len) = self.0.get_client_ddos_detected() {
            Box::into_raw(Box::new(len))
        } else {
            std::ptr::null()
        }
    }
    
    pub fn fastly_key_is_valid(&self) -> bool {
        self.0.fastly_key_is_valid()
    }
    
    pub fn set_cache_key(&mut self, key: &CxxVector<u8>) {
        self.0.set_cache_key(key.as_slice());
    }
    
    pub fn is_cacheable(&mut self) -> bool {
        self.0.is_cacheable()
    }
}
