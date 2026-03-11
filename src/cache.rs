use std::pin::{Pin, pin};
use std::ptr;
use std::{fmt::Write as _, time::Duration};

use cxx::CxxString;

use crate::ffi::{GetOrSetWithFnResult, GetOrSetWithFnTag};
use crate::{ffi::SimpleCacheErrorCode, http::body::Body};

pub struct SimpleCacheError(fastly::cache::simple::CacheError);

impl SimpleCacheError {
    pub fn error_msg(&self, mut out: Pin<&mut CxxString>) {
        write!(out, "{}", self.0).expect("This should never fail.");
    }
    pub fn error_code(&self) -> SimpleCacheErrorCode {
        match self.0 {
            fastly::cache::simple::CacheError::LimitExceeded => SimpleCacheErrorCode::LimitExceeded,
            fastly::cache::simple::CacheError::InvalidOperation => {
                SimpleCacheErrorCode::InvalidOperation
            }
            fastly::cache::simple::CacheError::Unsupported => SimpleCacheErrorCode::Unsupported,
            fastly::cache::simple::CacheError::Io(..) => SimpleCacheErrorCode::Io,
            fastly::cache::simple::CacheError::Purge(..) => SimpleCacheErrorCode::Purge,
            fastly::cache::simple::CacheError::GetOrSet(..) => SimpleCacheErrorCode::GetOrSet,
            fastly::cache::simple::CacheError::Other(..) => SimpleCacheErrorCode::Other,
            _ => unimplemented!("Unknown cache error"),
        }
    }
}

#[macro_export]
macro_rules! try_sce {
    ( $err:ident, $x:expr ) => {
        match $x {
            std::result::Result::Ok(val) => {
                $err.set(std::ptr::null_mut());
                val
            }
            std::result::Result::Err(e) => {
                $err.set(Box::into_raw(Box::new(SimpleCacheError(e))));
                return Default::default();
            }
        }
    };
}

pub fn f_cache_simple_get(
    key: &[u8],
    mut out: Pin<&mut *mut Body>,
    mut err: Pin<&mut *mut SimpleCacheError>,
) -> bool {
    try_sce!(err, fastly::cache::simple::get(key.to_owned()))
        .map(|body| out.set(Box::into_raw(Box::new(Body(body)))))
        .is_some()
}

pub fn f_cache_simple_get_or_set(
    key: &[u8],
    value: Box<Body>,
    ttl: u32,
    mut out: Pin<&mut *mut Body>,
    mut err: Pin<&mut *mut SimpleCacheError>,
) -> bool {
    out.set(Box::into_raw(Box::new(Body(try_sce!(
        err,
        fastly::cache::simple::get_or_set(
            key.to_owned(),
            value.0,
            Duration::from_millis(ttl as u64)
        )
    )))));
    true
}

pub struct SimpleCacheEntry(fastly::cache::simple::CacheEntry);

type GetOrSetWithFnType = dyn Fn() -> Result<fastly::cache::simple::CacheEntry, fastly::Error>;
fn shim_get_or_set_with_fn(func: *const GetOrSetWithFnTag) -> Box<GetOrSetWithFnType> {
    Box::new(move || {
        let mut out = pin!(ptr::null_mut());
        let result = unsafe {
            crate::manual_ffi::fastly_esi_manualbridge_GetOrSetWithFn_call(func, &mut out)
        };
        match result {
            GetOrSetWithFnResult::Ok => Ok(unsafe { out.read().0 }),
            GetOrSetWithFnResult::Err => Err(fastly::Error::msg("error during get_or_set_with")),
            _ => unreachable!(),
        }
    })
}

pub fn f_cache_simple_get_or_set_with(
    key: &[u8],
    make_entry: *const GetOrSetWithFnTag,
    mut out: Pin<&mut *mut Body>,
    mut err: Pin<&mut *mut SimpleCacheError>,
) -> bool {
    try_sce!(
        err,
        fastly::cache::simple::get_or_set_with(key.to_owned(), shim_get_or_set_with_fn(make_entry))
    )
    .map(|bod| out.set(Box::into_raw(Box::new(Body(bod)))))
    .is_some()
}

pub fn f_cache_simple_purge(key: &[u8], mut err: Pin<&mut *mut SimpleCacheError>) -> bool {
    try_sce!(err, fastly::cache::simple::purge(key.to_owned()));
    true
}

pub struct PurgeOptions(fastly::cache::simple::PurgeOptions);

pub fn m_static_cache_simple_purge_options_pop_scope() -> Box<PurgeOptions> {
    Box::new(PurgeOptions(
        fastly::cache::simple::PurgeOptions::pop_scope(),
    ))
}

pub fn m_static_cache_simple_purge_options_global_scope() -> Box<PurgeOptions> {
    Box::new(PurgeOptions(
        fastly::cache::simple::PurgeOptions::global_scope(),
    ))
}

pub fn f_cache_simple_purge_with_opts(
    key: &[u8],
    opts: Box<PurgeOptions>,
    mut err: Pin<&mut *mut SimpleCacheError>,
) -> bool {
    try_sce!(
        err,
        fastly::cache::simple::purge_with_opts(key.to_owned(), opts.0)
    );
    true
}

pub fn f_cache_simple_error_force_symbols(x: Box<SimpleCacheError>) -> Box<SimpleCacheError> {
    x
}
