use std::pin::Pin;

use cxx::{CxxString, CxxVector, UniquePtr};
use http::{HeaderName, HeaderValue};

use crate::backend::Backend;
use crate::error::ErrPtr;
use crate::ffi::{Method, Version};
use crate::http::body::{Body, StreamingBody};
use crate::http::header::HeaderValuesIter;
use crate::http::request::request::{AsyncStreamRes, PendingRequest};
use crate::http::response::Response;
use crate::try_fe;

pub struct Request(pub(crate) fastly::Request);

#[allow(clippy::module_inception)]
pub mod request {

    use crate::{
        error::{ErrPtr, FastlyError},
        http::body::StreamingBody,
        try_fe,
    };

    use super::*;
    pub struct PendingRequest(pub(crate) fastly::http::request::PendingRequest);
    // Sigh https://github.com/dtolnay/cxx/issues/671
    pub struct BoxPendingRequest(pub(crate) Option<Box<PendingRequest>>);

    pub fn f_http_push_box_pending_request_into_vec(
        vec: &mut Vec<BoxPendingRequest>,
        bx: Box<PendingRequest>,
    ) {
        vec.push(BoxPendingRequest(Some(bx)))
    }

    impl BoxPendingRequest {
        pub fn extract_req(&mut self) -> Box<PendingRequest> {
            self.0.take().expect("nothing in the box. oops.")
        }
    }

    pub enum PollResult {
        Pending(PendingRequest),
        Response(Response),
        Error(FastlyError),
    }

    pub fn m_http_request_poll_result_into_pending(result: Box<PollResult>) -> Box<PendingRequest> {
        match *result {
            PollResult::Pending(pending_request) => Box::new(pending_request),
            _ => panic!("not a pending request"),
        }
    }

    pub fn m_http_request_poll_result_into_response(result: Box<PollResult>) -> Box<Response> {
        match *result {
            PollResult::Response(response) => Box::new(response),
            _ => panic!("not a response"),
        }
    }

    pub fn m_http_request_poll_result_into_error(result: Box<PollResult>) -> Box<FastlyError> {
        match *result {
            PollResult::Error(err) => Box::new(err),
            _ => panic!("not a response"),
        }
    }

    impl PollResult {
        pub fn is_response(&self) -> bool {
            matches!(self, PollResult::Response(_))
        }

        pub fn is_pending(&self) -> bool {
            matches!(self, PollResult::Pending(_))
        }
    }

    pub fn m_http_request_pending_request_poll(req: Box<PendingRequest>) -> Box<PollResult> {
        use fastly::http::request::PollResult::{Done, Pending};
        Box::new(match req.0.poll() {
            Pending(pending_request) => PollResult::Pending(PendingRequest(pending_request)),
            Done(response) => response
                .map(|r| PollResult::Response(Response(r)))
                .unwrap_or_else(|e| PollResult::Error(e.into())),
        })
    }

    pub fn m_http_request_pending_request_wait(
        req: Box<PendingRequest>,
        mut out: Pin<&mut *mut Response>,
        mut err: ErrPtr,
    ) {
        out.set(Box::into_raw(Box::new(Response(try_fe!(
            err,
            req.0.wait()
        )))));
    }

    impl PendingRequest {
        pub fn cloned_sent_req(&self) -> Box<Request> {
            Box::new(Request(self.0.sent_req().clone_without_body()))
        }
    }

    pub fn f_http_request_select(
        reqs: Vec<BoxPendingRequest>,
        mut out: Pin<&mut *mut Response>,
        others: &mut Vec<BoxPendingRequest>,
        mut err: ErrPtr,
    ) {
        let (res, xs) =
            fastly::http::request::select(reqs.into_iter().map(|mut r| (*(r.extract_req())).0));
        for x in xs {
            others.push(BoxPendingRequest(Some(Box::new(PendingRequest(x)))));
        }
        out.set(Box::into_raw(Box::new(Response(try_fe!(err, res)))));
    }

    pub struct AsyncStreamRes(
        pub(crate) Option<Box<StreamingBody>>,
        pub(crate) Option<Box<PendingRequest>>,
    );

    impl AsyncStreamRes {
        pub fn take_body(&mut self) -> Box<StreamingBody> {
            self.0.take().expect("no body")
        }

        pub fn take_req(&mut self) -> Box<PendingRequest> {
            self.1.take().expect("no req")
        }
    }
}

pub fn m_static_http_request_new(method: Method, url: &CxxString) -> Box<Request> {
    let method: fastly::http::Method = method.into();
    Box::new(Request(fastly::Request::new(
        method,
        url.to_str().expect("Invalid UTF-8 in URL"),
    )))
}

pub fn m_static_http_request_get(url: &CxxString) -> Box<Request> {
    Box::new(Request(fastly::Request::get(
        url.to_str().expect("Invalid UTF-8 in URL"),
    )))
}

pub fn m_static_http_request_head(url: &CxxString) -> Box<Request> {
    Box::new(Request(fastly::Request::head(
        url.to_str().expect("Invalid UTF-8 in URL"),
    )))
}

pub fn m_static_http_request_post(url: &CxxString) -> Box<Request> {
    Box::new(Request(fastly::Request::post(
        url.to_str().expect("Invalid UTF-8 in URL"),
    )))
}

pub fn m_static_http_request_put(url: &CxxString) -> Box<Request> {
    Box::new(Request(fastly::Request::put(
        url.to_str().expect("Invalid UTF-8 in URL"),
    )))
}

pub fn m_static_http_request_delete(url: &CxxString) -> Box<Request> {
    Box::new(Request(fastly::Request::delete(
        url.to_str().expect("Invalid UTF-8 in URL"),
    )))
}

pub fn m_static_http_request_connect(url: &CxxString) -> Box<Request> {
    Box::new(Request(fastly::Request::connect(
        url.to_str().expect("Invalid UTF-8 in URL"),
    )))
}

pub fn m_static_http_request_options(url: &CxxString) -> Box<Request> {
    Box::new(Request(fastly::Request::options(
        url.to_str().expect("Invalid UTF-8 in URL"),
    )))
}

pub fn m_static_http_request_trace(url: &CxxString) -> Box<Request> {
    Box::new(Request(fastly::Request::trace(
        url.to_str().expect("Invalid UTF-8 in URL"),
    )))
}

pub fn m_static_http_request_patch(url: &CxxString) -> Box<Request> {
    Box::new(Request(fastly::Request::patch(
        url.to_str().expect("Invalid UTF-8 in URL"),
    )))
}

pub fn m_static_http_request_from_client() -> Box<Request> {
    Box::new(Request(fastly::Request::from_client()))
}

pub fn m_http_request_send(
    request: Box<Request>,
    backend: &Backend,
    mut out: Pin<&mut *mut Response>,
    mut err: ErrPtr,
) {
    let ret = Box::into_raw(Box::new(Response(try_fe!(err, request.0.send(&backend.0)))));
    out.set(ret);
}

pub fn m_http_request_send_async(
    request: Box<Request>,
    backend: &Backend,
    mut out: Pin<&mut *mut request::PendingRequest>,
    mut err: ErrPtr,
) {
    out.set(Box::into_raw(Box::new(request::PendingRequest(try_fe!(
        err,
        request.0.send_async(&backend.0)
    )))));
}

pub fn m_http_request_send_async_streaming(
    request: Box<Request>,
    backend: &Backend,
    mut out: Pin<&mut *mut AsyncStreamRes>,
    mut err: ErrPtr,
) {
    let (body, req) = try_fe!(err, request.0.send_async_streaming(&backend.0));
    out.set(Box::into_raw(Box::new(request::AsyncStreamRes(
        Some(Box::new(StreamingBody(body))),
        Some(Box::new(PendingRequest(req))),
    ))));
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

    pub fn get_method(&self) -> Method {
        self.0.get_method().clone().into()
    }

    pub fn set_method(&mut self, method: Method) {
        let method: fastly::http::Method = method.into();
        self.0.set_method(method);
    }

    pub fn get_url(&self, out: Pin<&mut CxxString>) {
        out.push_str(self.0.get_url().as_str());
    }

    pub fn set_path(&mut self, path: &CxxString, mut err: ErrPtr) {
        self.0.set_path(try_fe!(err, path.to_str()));
    }

    pub fn get_path(&self, mut out: Pin<&mut CxxString>) {
        out.as_mut().push_str(self.0.get_path());
    }

    pub fn set_url(&mut self, url: &CxxString, mut err: ErrPtr) {
        self.0.set_url(try_fe!(err, url.to_str()));
    }

    pub fn get_query_string(&self, mut out: Pin<&mut CxxString>) -> bool {
        self.0
            .get_query_str()
            .map(|qs| out.as_mut().push_str(qs))
            .is_some()
    }

    pub fn get_query_parameter(&self, param: &CxxString, mut out: Pin<&mut CxxString>) -> bool {
        self.0
            .get_query_parameter(param.to_str().expect("invalid UTF-8"))
            .map(|qp| out.as_mut().push_str(qp))
            .is_some()
    }

    pub fn set_query_string(&mut self, qs: &CxxString, mut err: ErrPtr) {
        self.0.set_query_str(try_fe!(err, qs.to_str()));
    }

    pub fn remove_query(&mut self) {
        self.0.remove_query();
    }

    pub fn get_client_ddos_detected(&self, mut out: Pin<&mut bool>) -> bool {
        self.0
            .get_client_ddos_detected()
            .map(|detected| out.set(detected))
            .is_some()
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

    pub fn set_pass(&mut self, pass: bool) {
        self.0.set_pass(pass);
    }

    pub fn set_ttl(&mut self, ttl: u32) {
        self.0.set_ttl(ttl);
    }

    pub fn set_stale_while_revalidate(&mut self, swr: u32) {
        self.0.set_stale_while_revalidate(swr);
    }

    pub fn set_pci(&mut self, pci: bool) {
        self.0.set_pci(pci);
    }

    pub fn set_surrogate_key(&mut self, sk: &CxxString, mut err: ErrPtr) {
        self.0
            .set_surrogate_key(try_fe!(err, HeaderValue::try_from(sk.as_bytes())));
    }

    pub fn get_client_ip_addr(&self, buf: Pin<&mut CxxString>) -> bool {
        self.0
            .get_client_ip_addr()
            .map(|ip| buf.push_str(ip.to_string().as_ref()))
            .is_some()
    }

    pub fn get_server_ip_addr(&self, buf: Pin<&mut CxxString>) -> bool {
        self.0
            .get_server_ip_addr()
            .map(|ip| buf.push_str(ip.to_string().as_ref()))
            .is_some()
    }

    pub fn get_version(&self) -> Version {
        self.0.get_version().into()
    }

    pub fn set_version(&mut self, version: Version) {
        self.0.set_version(version.into());
    }
}
