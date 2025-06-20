use std::io::{Read as _, Write as _};

use cxx::CxxString;
use fastly::experimental::{BodyExt, StreamingBodyExt};

pub struct Body(pub(crate) fastly::http::Body);

pub fn m_static_http_body_new() -> Box<Body> {
    Box::new(Body(fastly::http::Body::new()))
}

impl Body {
    pub fn append(&mut self, other: Box<Body>) {
        self.0.append(other.0);
    }

    pub fn append_trailer(&mut self, name: &CxxString, value: &CxxString) {
        self.0.append_trailer(
            name.to_string_lossy().as_ref(),
            value.to_string_lossy().as_ref(),
        );
    }

    pub fn read(&mut self, buf: &mut [u8]) -> usize {
        self.0.read(buf).expect("read failed")
    }

    pub fn write(&mut self, bytes: &[u8]) -> usize {
        self.0.write(bytes).expect("write failed")
    }
}

pub struct StreamingBody(pub(crate) fastly::http::body::StreamingBody);

pub fn m_http_streaming_body_finish(body: Box<StreamingBody>) {
    body.0.finish().expect("StreamingBody finish failed");
}

impl StreamingBody {
    pub fn append(&mut self, other: Box<Body>) {
        self.0.append(other.0);
    }

    pub fn append_trailer(&mut self, name: &CxxString, value: &CxxString) {
        self.0.append_trailer(
            name.to_string_lossy().as_ref(),
            value.to_string_lossy().as_ref(),
        );
    }

    pub fn write(&mut self, bytes: &[u8]) -> usize {
        self.0.write(bytes).expect("write failed")
    }
}
