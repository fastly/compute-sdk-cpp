use std::{io::Write as _, net::IpAddr, pin::Pin};

use cxx::CxxString;

use crate::{
    error::FastlyError,
    ffi::{InspectErrorCode, InspectVerdict},
    http::request::Request,
    try_fe,
};

#[derive(Debug, Default)]
pub struct InspectConfig {
    client_ip: Option<IpAddr>,
    workspace: Option<String>,
    corp: Option<String>,
    buffer_size: Option<usize>,
}

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
    config: Box<InspectConfig>,
    mut out: Pin<&mut *mut InspectResponse>,
    mut err: Pin<&mut *mut InspectError>,
) {
    let mut icfg = fastly::security::InspectConfig::from_request(&request.0);
    if let Some(ip_addr) = config.client_ip {
        icfg = icfg.client_ip(ip_addr);
    }
    if let Some(corp) = config.corp {
        icfg = icfg.corp(corp);
    }
    if let Some(ws) = config.workspace {
        icfg = icfg.workspace(ws);
    }
    if let Some(sz) = config.buffer_size {
        icfg = icfg.buffer_size(sz);
    }
    out.set(try_ie!(
        err,
        fastly::security::inspect(icfg)
            .map(InspectResponse)
            .map(Box::new)
            .map(Box::into_raw)
    ))
}

pub fn m_static_inspect_inspect_config_new() -> Box<InspectConfig> {
    Box::default()
}

impl InspectConfig {
    pub fn client_ip(&mut self, ip: &CxxString, mut err: Pin<&mut *mut FastlyError>) {
        let s = try_fe!(err, ip.to_str());
        self.client_ip = Some(try_fe!(err, s.parse()));
    }

    pub fn workspace(&mut self, workspace: &CxxString, mut err: Pin<&mut *mut FastlyError>) {
        let s = try_fe!(err, workspace.to_str());
        self.workspace = Some(s.into());
    }

    pub fn corp(&mut self, corp: &CxxString, mut err: Pin<&mut *mut FastlyError>) {
        let s = try_fe!(err, corp.to_str());
        self.corp = Some(s.into());
    }

    pub fn buffer_size(&mut self, buffer_size: usize) {
        self.buffer_size = Some(buffer_size);
    }
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
}

pub fn f_security_inspect_error_force_symbols(x: Box<InspectError>) -> Box<InspectError> {
    x
}
