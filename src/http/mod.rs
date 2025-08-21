pub mod body;
pub mod header;
pub mod purge;
pub mod request;
pub mod response;
pub mod status_code;

use crate::ffi::Method;
use crate::ffi::Version;

impl From<Method> for fastly::http::Method {
    fn from(val: Method) -> Self {
        match val {
            Method::GET => fastly::http::Method::GET,
            Method::POST => fastly::http::Method::POST,
            Method::PUT => fastly::http::Method::PUT,
            Method::DELETE => fastly::http::Method::DELETE,
            Method::HEAD => fastly::http::Method::HEAD,
            Method::OPTIONS => fastly::http::Method::OPTIONS,
            Method::CONNECT => fastly::http::Method::CONNECT,
            Method::PATCH => fastly::http::Method::PATCH,
            Method::TRACE => fastly::http::Method::TRACE,
            _ => panic!("Unsupported method."),
        }
    }
}

impl From<fastly::http::Method> for Method {
    fn from(val: fastly::http::Method) -> Self {
        match val {
            fastly::http::Method::GET => Method::GET,
            fastly::http::Method::POST => Method::POST,
            fastly::http::Method::PUT => Method::PUT,
            fastly::http::Method::DELETE => Method::DELETE,
            fastly::http::Method::HEAD => Method::HEAD,
            fastly::http::Method::OPTIONS => Method::OPTIONS,
            fastly::http::Method::CONNECT => Method::CONNECT,
            fastly::http::Method::PATCH => Method::PATCH,
            fastly::http::Method::TRACE => Method::TRACE,
            _ => panic!("Unsupported method."),
        }
    }
}

impl From<Version> for fastly::http::Version {
    fn from(val: Version) -> Self {
        match val {
            Version::HTTP_09 => fastly::http::Version::HTTP_09,
            Version::HTTP_10 => fastly::http::Version::HTTP_10,
            Version::HTTP_11 => fastly::http::Version::HTTP_11,
            Version::HTTP_2 => fastly::http::Version::HTTP_2,
            Version::HTTP_3 => fastly::http::Version::HTTP_3,
            _ => panic!("Unsupported HTTP version."),
        }
    }
}

impl From<fastly::http::Version> for Version {
    fn from(val: fastly::http::Version) -> Self {
        match val {
            fastly::http::Version::HTTP_09 => Version::HTTP_09,
            fastly::http::Version::HTTP_10 => Version::HTTP_10,
            fastly::http::Version::HTTP_11 => Version::HTTP_11,
            fastly::http::Version::HTTP_2 => Version::HTTP_2,
            fastly::http::Version::HTTP_3 => Version::HTTP_3,
            _ => panic!("Unsupported HTTP version."),
        }
    }
}
