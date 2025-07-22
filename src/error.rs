use std::{fmt::Write, pin::Pin};

use cxx::CxxString;

use crate::ffi::FastlyErrorCode;

#[macro_export]
macro_rules! try_fe {
    ( $err:ident, $x:expr ) => {
        match $x {
            std::result::Result::Ok(val) => {
                $err.set(std::ptr::null_mut());
                val
            }
            std::result::Result::Err(e) => {
                $err.set(Box::into_raw(Box::new(e.into())));
                return Default::default();
            }
        }
    };
}

#[derive(Debug, thiserror::Error)]
#[error("Fastly status error: {}", fastly_status_message(.0))]
pub(crate) struct FastlyStatusWrapper(pub(crate) fastly_shared::FastlyStatus);

#[derive(thiserror::Error, Debug)]
pub enum FastlyError {
    #[error(transparent)]
    Utf8Error(#[from] std::str::Utf8Error),
    #[error(transparent)]
    InvalidHeaderName(#[from] fastly::http::header::InvalidHeaderName),
    #[error(transparent)]
    InvalidHeaderValue(#[from] fastly::http::header::InvalidHeaderValue),
    #[error(transparent)]
    InvalidStatusCode(#[from] http::status::InvalidStatusCode),
    #[error(transparent)]
    IoError(#[from] std::io::Error),
    #[error(transparent)]
    #[allow(clippy::enum_variant_names)]
    FastlyError(#[from] fastly::Error),
    #[error(transparent)]
    FastlySendError(#[from] fastly::http::request::SendError),
    #[error(transparent)]
    AddrParseError(#[from] std::net::AddrParseError),
    #[error(transparent)]
    BackendError(#[from] fastly::error::BackendError),
    #[error(transparent)]
    BackendCreationError(#[from] fastly::backend::BackendCreationError),
    #[error(transparent)]
    FastlyStatus(#[from] FastlyStatusWrapper),
    #[error(transparent)]
    ConfigStoreOpenError(#[from] fastly::config_store::OpenError),
    #[error(transparent)]
    ConfigStoreLookupError(#[from] fastly::config_store::LookupError),
    #[error(transparent)]
    SecretStoreOpenError(#[from] fastly::secret_store::OpenError),
    #[error(transparent)]
    SecretStoreLookupError(#[from] fastly::secret_store::LookupError),
    // Make sure to add any new variants to the `FastlyErrorCode` enum in `lib.rs` _and_ to the match below!
}

fn fastly_status_message(status: &fastly_shared::FastlyStatus) -> &'static str {
    use fastly_shared::FastlyStatus;
    match *status {
        FastlyStatus::OK => "Ok",
        FastlyStatus::ERROR => "Unexpected error",
        FastlyStatus::INVAL => "Invalid argument",
        FastlyStatus::BADF => "Invalid handle",
        FastlyStatus::BUFLEN => "Buffer is too long",
        FastlyStatus::UNSUPPORTED => "Unsupported operation",
        FastlyStatus::BADALIGN => "Alignment error",
        FastlyStatus::HTTPINVALID => "Invalid HTTP error",
        FastlyStatus::HTTPUSER => "HTTP user error",
        FastlyStatus::HTTPINCOMPLETE => "HTTP incomplete message error",
        FastlyStatus::NONE => "Optional value did not exist",
        FastlyStatus::HTTPHEADTOOLARGE => {
            "HTTP head too large error. Aw yeah it's the big brain time"
        }
        FastlyStatus::HTTPINVALIDSTATUS => "HTTP invalid status error",
        FastlyStatus::LIMITEXCEEDED => "Limit exceeded",
        FastlyStatus::AGAIN => "Resource temporarily unavailable.",
        _ => "Unknown status code",
    }
}

pub(crate) type ErrPtr<'a> = Pin<&'a mut *mut FastlyError>;

impl FastlyError {
    pub fn error_msg(&self, mut out: Pin<&mut CxxString>) {
        write!(out, "{self}").expect("This should never fail.");
    }

    pub fn error_code(&self) -> FastlyErrorCode {
        match self {
            FastlyError::Utf8Error(_) => FastlyErrorCode::Utf8Error,
            FastlyError::InvalidHeaderName(_) => FastlyErrorCode::InvalidHeaderName,
            FastlyError::InvalidHeaderValue(_) => FastlyErrorCode::InvalidHeaderValue,
            FastlyError::InvalidStatusCode(_) => FastlyErrorCode::InvalidStatusCode,
            FastlyError::IoError(_) => FastlyErrorCode::IoError,
            FastlyError::FastlyError(_) => FastlyErrorCode::FastlyError,
            FastlyError::FastlySendError(_) => FastlyErrorCode::FastlySendError,
            FastlyError::AddrParseError(_) => FastlyErrorCode::AddrParseError,
            FastlyError::BackendError(_) => FastlyErrorCode::BackendError,
            FastlyError::BackendCreationError(_) => FastlyErrorCode::BackendCreationError,
            FastlyError::FastlyStatus(_) => FastlyErrorCode::FastlyStatus,
            FastlyError::ConfigStoreOpenError(_) => FastlyErrorCode::ConfigStoreOpenError,
            FastlyError::ConfigStoreLookupError(_) => FastlyErrorCode::ConfigStoreLookupError,
            FastlyError::SecretStoreOpenError(_) => FastlyErrorCode::SecretStoreOpenError,
            FastlyError::SecretStoreLookupError(_) => FastlyErrorCode::SecretStoreLookupError,
        }
    }
}
