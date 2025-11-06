use std::{io::Write as _, net::IpAddr, pin::Pin, str::FromStr};

use cxx::CxxString;

use crate::{
    ffi::{InspectErrorCode, InspectVerdict},
    http::request::Request,
    http::response::Response,
};

pub struct InspectResponse(pub(crate) fastly::security::InspectResponse);

pub struct InspectError(pub(crate) fastly::security::InspectError);

impl InspectError {
    pub fn error_msg(&self, mut out: Pin<&mut CxxString>) {
        write!(out, "{}", self.0).expect("This should never fail.");
    }

    pub fn error_code(&self) -> InspectErrorCode {
        match self.0 {
            fastly::security::InspectError::DeserializeError(_) => {
                InspectErrorCode::DeserializeError
            }
            fastly::security::InspectError::InvalidConfig => InspectErrorCode::InvalidConfig,
            fastly::security::InspectError::RequestError(_) => InspectErrorCode::RequestError,
            fastly::security::InspectError::BufferSizeError(_) => InspectErrorCode::BufferSizeError,
            _ => InspectErrorCode::Unexpected,
        }
    }

    pub fn required_buffer_size(&self, mut out: Pin<&mut usize>) -> bool {
        match self.0 {
            fastly::security::InspectError::BufferSizeError(n) => {
                out.set(n);
                true
            }
            _ => false,
        }
    }
}

#[macro_export]
macro_rules! try_ie {
    ( $err:ident, $x:expr ) => {
        match $x {
            std::result::Result::Ok(val) => {
                $err.set(std::ptr::null_mut());
                val
            }
            std::result::Result::Err(e) => {
                $err.set(Box::into_raw(Box::new(InspectError(e))));
                return Default::default();
            }
        }
    };
}

pub fn f_security_lookup(
    request: &Request,
    client_ip: *const CxxString,
    corp: *const CxxString,
    workspace: *const CxxString,
    buffer_size: *const usize,
    mut out: Pin<&mut *mut InspectResponse>,
    mut err: Pin<&mut *mut InspectError>,
) {
    let mut icfg = fastly::security::InspectConfig::from_request(&request.0);
    unsafe {
        if let Some(client_ip) = client_ip.as_ref() {
            let ip = IpAddr::from_str(try_ie!(
                err,
                client_ip
                    .to_str()
                    .map_err(|_| fastly::security::InspectError::InvalidConfig)
            ));
            let ip = try_ie!(
                err,
                ip.map_err(|_| fastly::security::InspectError::InvalidConfig)
            );
            icfg = icfg.client_ip(ip);
        }
        if let Some(corp) = corp.as_ref() {
            icfg = icfg.corp(corp.to_string());
        }
        if let Some(workspace) = workspace.as_ref() {
            icfg = icfg.workspace(workspace.to_string());
        }
        if let Some(buffer_size) = buffer_size.as_ref() {
            icfg = icfg.buffer_size(*buffer_size);
        }
    }
    out.set(try_ie!(
        err,
        fastly::security::inspect(icfg)
            .map(InspectResponse)
            .map(Box::new)
            .map(Box::into_raw)
    ))
}

impl InspectResponse {
    pub fn status(&self) -> i16 {
        self.0.status()
    }

    pub fn is_redirect(&self) -> bool {
        self.0.is_redirect()
    }

    pub fn decision_ms(&self) -> u32 {
        self.0.decision_ms().as_millis().min(u32::MAX as u128) as u32
    }

    pub fn redirect_url(&self, out: Pin<&mut CxxString>) -> bool {
        if let Some(url) = self.0.redirect_url() {
            out.push_str(url);
            true
        } else {
            false
        }
    }

    pub fn tags(&self) -> Vec<String> {
        self.0.tags().into_iter().map(|x| x.into()).collect()
    }

    pub fn verdict(&self) -> InspectVerdict {
        match self.0.verdict() {
            fastly::security::InspectVerdict::Allow => InspectVerdict::Allow,
            fastly::security::InspectVerdict::Block => InspectVerdict::Block,
            fastly::security::InspectVerdict::Unauthorized => InspectVerdict::Unauthorized,
            fastly::security::InspectVerdict::Other(_) => InspectVerdict::Other,
        }
    }

    pub fn unrecognized_verdict_info(&self, out: Pin<&mut CxxString>) -> bool {
        if let fastly::security::InspectVerdict::Other(s) = self.0.verdict() {
            out.push_str(s);
            true
        } else {
            false
        }
    }
}

pub fn m_security_inspect_response_into_redirect(
    response: Box<InspectResponse>,
    mut out: Pin<&mut *mut Response>,
) -> bool {
    if let Some(resp) = response.0.into_redirect() {
        out.set(Box::into_raw(Box::new(Response(resp))));
        true
    } else {
        false
    }
}

pub fn f_security_inspect_error_force_symbols(x: Box<InspectError>) -> Box<InspectError> {
    x
}
