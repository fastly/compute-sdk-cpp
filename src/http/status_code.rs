use std::pin::Pin;

use cxx::CxxString;
use fastly::http::StatusCode;

use crate::{error::ErrPtr, try_fe};

pub fn f_http_status_code_canonical_reason(
    code: u16,
    string: Pin<&mut CxxString>,
    mut err: ErrPtr,
) -> bool {
    try_fe!(err, StatusCode::from_u16(code))
        .canonical_reason()
        .map(|r| string.push_str(r))
        .is_some()
}
