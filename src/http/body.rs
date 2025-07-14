use std::io::{Read as _, Write as _};

use cxx::CxxString;
use fastly::{
    experimental::{BodyExt, StreamingBodyExt},
    http::{HeaderName, HeaderValue},
};

use crate::{error::ErrPtr, try_fe};

pub struct Body(pub(crate) fastly::http::Body);

pub fn m_static_http_body_new() -> Box<Body> {
    Box::new(Body(fastly::http::Body::new()))
}

impl Body {
    pub fn append(&mut self, other: Box<Body>) {
        self.0.append(other.0);
    }

    pub fn append_trailer(&mut self, name: &CxxString, value: &CxxString, mut err: ErrPtr) {
        self.0.append_trailer(
            try_fe!(err, HeaderName::try_from(name.as_bytes())),
            try_fe!(err, HeaderValue::try_from(value.as_bytes())),
        );
    }

    pub fn read(&mut self, buf: &mut [u8], mut err: ErrPtr) -> usize {
        try_fe!(err, self.0.read(buf))
    }

    pub fn write(&mut self, bytes: &[u8], mut err: ErrPtr) -> usize {
        try_fe!(err, self.0.write(bytes))
    }
}

pub struct StreamingBody(pub(crate) fastly::http::body::StreamingBody);

pub fn m_http_streaming_body_finish(body: Box<StreamingBody>, mut err: ErrPtr) {
    try_fe!(err, body.0.finish())
}

impl StreamingBody {
    pub fn append(&mut self, other: Box<Body>) {
        self.0.append(other.0);
    }

    pub fn append_trailer(&mut self, name: &CxxString, value: &CxxString, mut err: ErrPtr) {
        self.0.append_trailer(
            try_fe!(err, HeaderName::try_from(name.as_bytes())),
            try_fe!(err, HeaderValue::try_from(value.as_bytes())),
        );
    }

    pub fn write(&mut self, bytes: &[u8], mut err: ErrPtr) -> usize {
        try_fe!(err, self.0.write(bytes))
    }
}
