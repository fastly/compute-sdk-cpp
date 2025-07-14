use cxx::CxxString;

use crate::{error::ErrPtr, try_fe};

pub fn f_http_purge_purge_surrogate_key(surrogate_key: &CxxString, mut err: ErrPtr) {
    try_fe!(
        err,
        fastly::http::purge::purge_surrogate_key(try_fe!(err, surrogate_key.to_str()))
    );
}

pub fn f_http_purge_soft_purge_surrogate_key(surrogate_key: &CxxString, mut err: ErrPtr) {
    try_fe!(
        err,
        fastly::http::purge::soft_purge_surrogate_key(try_fe!(err, surrogate_key.to_str()))
    );
}
