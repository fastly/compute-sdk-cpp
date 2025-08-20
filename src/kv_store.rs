use std::{fmt::Write, pin::Pin, time::Duration};

use cxx::CxxString;

use crate::{
    ffi::{InsertMode, KVStoreErrorCode},
    http::body::Body,
};

pub struct KVStoreError(pub(crate) fastly::kv_store::KVStoreError);

impl KVStoreError {
    pub fn error_msg(&self, mut out: Pin<&mut CxxString>) {
        write!(out, "{}", self.0).expect("This should never fail.");
    }

    pub fn error_code(&self) -> KVStoreErrorCode {
        match self.0 {
            fastly::kv_store::KVStoreError::InvalidKey => KVStoreErrorCode::InvalidKey,
            fastly::kv_store::KVStoreError::InvalidStoreHandle => {
                KVStoreErrorCode::InvalidStoreHandle
            }
            fastly::kv_store::KVStoreError::InvalidStoreOptions => {
                KVStoreErrorCode::InvalidStoreOptions
            }
            fastly::kv_store::KVStoreError::ItemBadRequest => KVStoreErrorCode::ItemBadRequest,
            fastly::kv_store::KVStoreError::ItemNotFound => KVStoreErrorCode::ItemNotFound,
            fastly::kv_store::KVStoreError::ItemPreconditionFailed => {
                KVStoreErrorCode::ItemPreconditionFailed
            }
            fastly::kv_store::KVStoreError::ItemPayloadTooLarge => {
                KVStoreErrorCode::ItemPayloadTooLarge
            }
            fastly::kv_store::KVStoreError::StoreNotFound(_) => KVStoreErrorCode::StoreNotFound,
            fastly::kv_store::KVStoreError::TooManyRequests => KVStoreErrorCode::TooManyRequests,
            fastly::kv_store::KVStoreError::Unexpected(_) => KVStoreErrorCode::Unexpected,
            _ => KVStoreErrorCode::Unexpected,
        }
    }
}

#[macro_export]
macro_rules! try_kve {
    ( $err:ident, $x:expr ) => {
        match $x {
            std::result::Result::Ok(val) => {
                $err.set(std::ptr::null_mut());
                val
            }
            std::result::Result::Err(e) => {
                $err.set(Box::into_raw(Box::new(KVStoreError(e))));
                return Default::default();
            }
        }
    };
}

pub struct KvStore(pub(crate) fastly::KVStore);

pub struct InsertBuilder<'a>(pub(crate) fastly::kv_store::InsertBuilder<'a>);

pub fn m_kv_store_insert_builder_mode(
    mut builder: Box<InsertBuilder>,
    mode: InsertMode,
) -> Box<InsertBuilder> {
    let mode = match mode {
        InsertMode::Add => fastly::kv_store::InsertMode::Add,
        InsertMode::Append => fastly::kv_store::InsertMode::Append,
        InsertMode::Overwrite => fastly::kv_store::InsertMode::Overwrite,
        InsertMode::Prepend => fastly::kv_store::InsertMode::Prepend,
        _ => panic!("Unknown insert mode."),
    };
    builder.0 = builder.0.mode(mode);
    builder
}

pub fn m_kv_store_insert_builder_background_fetch(
    mut builder: Box<InsertBuilder>,
) -> Box<InsertBuilder> {
    builder.0 = builder.0.background_fetch();
    builder
}

pub fn m_kv_store_insert_builder_if_generation_match(
    mut builder: Box<InsertBuilder>,
    generation: u64,
) -> Box<InsertBuilder> {
    builder.0 = builder.0.if_generation_match(generation);
    builder
}

pub fn m_kv_store_insert_builder_metadata<'a>(
    mut builder: Box<InsertBuilder<'a>>,
    data: &CxxString,
) -> Box<InsertBuilder<'a>> {
    builder.0 = builder.0.metadata(data.to_str().expect("Invalid UTF-8"));
    builder
}

pub fn m_kv_store_insert_builder_time_to_live(
    mut builder: Box<InsertBuilder>,
    ttl: u32,
) -> Box<InsertBuilder> {
    builder.0 = builder.0.time_to_live(Duration::from_millis(ttl as u64));
    builder
}

pub fn m_kv_store_insert_builder_execute(
    builder: Box<InsertBuilder>,
    key: &CxxString,
    body: Box<Body>,
    mut err: Pin<&mut *mut KVStoreError>,
) {
    try_kve!(
        err,
        builder
            .0
            .execute(key.to_str().expect("Invalid UTF-8"), body.0)
    );
}

pub fn m_kv_store_insert_builder_execute_async(
    builder: Box<InsertBuilder>,
    key: &CxxString,
    body: Box<Body>,
    mut out: Pin<&mut u32>,
    mut err: Pin<&mut *mut KVStoreError>,
) {
    let handle = try_kve!(
        err,
        builder
            .0
            .execute_async(key.to_str().expect("Invalid UTF-8"), body.0)
    );
    out.set(handle.as_u32());
}

pub struct LookupResponse(pub(crate) fastly::kv_store::LookupResponse);
pub struct LookupBuilder<'a>(pub(crate) fastly::kv_store::LookupBuilder<'a>);
pub struct EraseBuilder<'a>(pub(crate) fastly::kv_store::DeleteBuilder<'a>);
pub struct ListBuilder<'a>(pub(crate) fastly::kv_store::ListBuilder<'a>);
pub struct ListPage(pub(crate) fastly::kv_store::ListPage);
pub struct KVStore(pub(crate) fastly::kv_store::KVStore);
