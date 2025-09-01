use std::{fmt::Write, pin::Pin, time::Duration};

use cxx::{CxxString, CxxVector};

use crate::{
    ffi::{InsertMode, KVStoreErrorCode, ListModeType},
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

pub struct ListMode(pub(crate) fastly::kv_store::ListMode);

impl ListMode {
    pub fn code(&self) -> ListModeType {
        match self.0 {
            fastly::kv_store::ListMode::Strong => ListModeType::Strong,
            fastly::kv_store::ListMode::Eventual => ListModeType::Eventual,
            fastly::kv_store::ListMode::Other(_) => ListModeType::Other,
        }
    }

    pub fn other_string(&self, out: Pin<&mut CxxString>) -> bool {
        if let fastly::kv_store::ListMode::Other(ref s) = self.0 {
            out.push_str(s);
            true
        } else {
            false
        }
    }
}

pub struct LookupResponse(pub(crate) fastly::kv_store::LookupResponse);

impl LookupResponse {
    pub fn take_body(&mut self) -> Box<Body> {
        Box::new(Body(self.0.take_body()))
    }

    pub fn try_take_body(&mut self, mut out: Pin<&mut *mut Body>) -> bool {
        self.0
            .try_take_body()
            .map(|body| {
                out.set(Box::into_raw(Box::new(Body(body))));
            })
            .is_some()
    }

    pub fn take_body_bytes(&mut self, mut out: Pin<&mut CxxVector<u8>>) {
        for byte in self.0.take_body_bytes() {
            out.as_mut().push(byte);
        }
    }

    pub fn metadata(&self, mut out: Pin<&mut CxxVector<u8>>) -> bool {
        self.0
            .metadata()
            .map(|metadata| {
                for byte in metadata {
                    out.as_mut().push(byte);
                }
            })
            .is_some()
    }

    pub fn current_generation(&self) -> u64 {
        self.0.current_generation()
    }
}
pub struct LookupBuilder<'a>(pub(crate) fastly::kv_store::LookupBuilder<'a>);

impl LookupBuilder<'_> {
    pub fn execute(
        &self,
        key: &str,
        mut out: Pin<&mut *mut LookupResponse>,
        mut err: Pin<&mut *mut KVStoreError>,
    ) {
        let response = try_kve!(err, self.0.execute(key));
        out.set(Box::into_raw(Box::new(LookupResponse(response))));
    }

    pub fn execute_async(
        &self,
        key: &str,
        mut out: Pin<&mut u32>,
        mut err: Pin<&mut *mut KVStoreError>,
    ) {
        let handle = try_kve!(err, self.0.execute_async(key));
        out.set(handle.as_u32());
    }
}
pub struct EraseBuilder<'a>(pub(crate) fastly::kv_store::DeleteBuilder<'a>);

impl EraseBuilder<'_> {
    pub fn execute(&self, key: &str, mut err: Pin<&mut *mut KVStoreError>) {
        try_kve!(err, self.0.execute(key));
    }

    pub fn execute_async(
        &self,
        key: &str,
        mut out: Pin<&mut u32>,
        mut err: Pin<&mut *mut KVStoreError>,
    ) {
        let handle = try_kve!(err, self.0.execute_async(key));
        out.set(handle.as_u32());
    }
}

pub struct ListBuilder<'a>(pub(crate) fastly::kv_store::ListBuilder<'a>);

impl ListBuilder<'_> {
    pub fn execute_async(&self, mut out: Pin<&mut u32>, mut err: Pin<&mut *mut KVStoreError>) {
        let handle = try_kve!(err, self.0.execute_async());
        out.set(handle.as_u32());
    }
}

pub fn m_kv_store_list_builder_execute(
    builder: Box<ListBuilder>,
    mut out: Pin<&mut *mut ListPage>,
    mut err: Pin<&mut *mut KVStoreError>,
) {
    let page = try_kve!(err, builder.0.execute());
    println!("Got page with {} keys", page.keys().len());
    println!("Prefix: {:?}", page.prefix());
    out.set(Box::into_raw(Box::new(ListPage(page))));
}

pub fn m_kv_store_list_builder_eventual_consistency(
    mut builder: Box<ListBuilder>,
) -> Box<ListBuilder> {
    builder.0 = builder.0.eventual_consistency();
    builder
}

pub fn m_kv_store_list_builder_cursor<'a>(
    mut builder: Box<ListBuilder<'a>>,
    cursor: &str,
) -> Box<ListBuilder<'a>> {
    builder.0 = builder.0.cursor(cursor);
    builder
}

pub fn m_kv_store_list_builder_limit(
    mut builder: Box<ListBuilder>,
    limit: u32,
) -> Box<ListBuilder> {
    builder.0 = builder.0.limit(limit);
    builder
}

pub fn m_kv_store_list_builder_prefix<'a>(
    mut builder: Box<ListBuilder<'a>>,
    prefix: &str,
) -> Box<ListBuilder<'a>> {
    builder.0 = builder.0.prefix(prefix);
    builder
}

pub struct ListResponse<'a>(pub(crate) fastly::kv_store::ListResponse<'a>);

impl ListResponse<'_> {
    pub fn next(
        &mut self,
        mut out: Pin<&mut *mut ListPage>,
        mut err: Pin<&mut *mut KVStoreError>,
    ) -> bool {
        self.0
            .next()
            .map(|page| match page {
                Ok(page) => {
                    out.set(Box::into_raw(Box::new(ListPage(page))));
                }
                Err(e) => {
                    err.set(Box::into_raw(Box::new(KVStoreError(e))));
                }
            })
            .is_some()
    }
}

pub fn m_kv_store_list_builder_iter(builder: Box<ListBuilder<'_>>) -> Box<ListResponse<'_>> {
    Box::new(ListResponse(builder.0.iter()))
}

pub struct ListPage(pub(crate) fastly::kv_store::ListPage);

impl ListPage {
    pub fn keys(&self) -> &[String] {
        self.0.keys()
    }
    pub fn next_cursor(&self, out: Pin<&mut CxxString>) -> bool {
        self.0
            .next_cursor()
            .map(|cursor| {
                out.push_str(&cursor);
            })
            .is_some()
    }
    pub fn prefix(&self, out: Pin<&mut CxxString>) -> bool {
        self.0
            .prefix()
            .map(|prefix| {
                out.push_str(prefix);
            })
            .is_some()
    }
    pub fn limit(&self) -> u32 {
        self.0.limit()
    }
    pub fn mode(&self) -> Box<ListMode> {
        Box::new(ListMode(self.0.mode()))
    }
}

pub fn m_kv_store_list_page_into_keys(page: Box<ListPage>) -> Vec<String> {
    page.0.into_keys()
}

pub struct KVStore(pub(crate) fastly::kv_store::KVStore);

impl KVStore {
    pub fn lookup(
        &self,
        key: &str,
        mut out: Pin<&mut *mut LookupResponse>,
        mut err: Pin<&mut *mut KVStoreError>,
    ) {
        let response = try_kve!(err, self.0.lookup(key));
        out.set(Box::into_raw(Box::new(LookupResponse(response))));
    }

    pub fn build_lookup(&self) -> Box<LookupBuilder<'_>> {
        Box::new(LookupBuilder(self.0.build_lookup()))
    }

    pub fn pending_lookup_wait(
        &self,
        pending_request_handle: u32,
        mut out: Pin<&mut *mut LookupResponse>,
        mut err: Pin<&mut *mut KVStoreError>,
    ) {
        // Safe because C++ should only pass back handles that were created by us.
        let handle =
            unsafe { fastly::kv_store::PendingLookupHandle::from_u32(pending_request_handle) };
        let response = try_kve!(err, self.0.pending_lookup_wait(handle));
        out.set(Box::into_raw(Box::new(LookupResponse(response))));
    }

    pub fn insert(&self, key: &str, value: Box<Body>, mut err: Pin<&mut *mut KVStoreError>) {
        try_kve!(err, self.0.insert(key, value.0,));
    }

    pub fn build_insert(&self) -> Box<InsertBuilder<'_>> {
        Box::new(InsertBuilder(self.0.build_insert()))
    }
    pub fn pending_insert_wait(
        &self,
        pending_insert_handle: u32,
        mut err: Pin<&mut *mut KVStoreError>,
    ) {
        // Safe because C++ should only pass back handles that were created by us.
        let handle =
            unsafe { fastly::kv_store::PendingInsertHandle::from_u32(pending_insert_handle) };
        try_kve!(err, self.0.pending_insert_wait(handle));
    }

    pub fn erase(&self, key: &str, mut err: Pin<&mut *mut KVStoreError>) {
        try_kve!(err, self.0.delete(key));
    }

    pub fn build_erase(&self) -> Box<EraseBuilder<'_>> {
        Box::new(EraseBuilder(self.0.build_delete()))
    }

    pub fn pending_erase_wait(
        &self,
        pending_delete_handle: u32,
        mut err: Pin<&mut *mut KVStoreError>,
    ) {
        // Safe because C++ should only pass back handles that were created by us.
        let handle =
            unsafe { fastly::kv_store::PendingDeleteHandle::from_u32(pending_delete_handle) };
        try_kve!(err, self.0.pending_delete_wait(handle));
    }

    pub fn list(&self, mut out: Pin<&mut *mut ListPage>, mut err: Pin<&mut *mut KVStoreError>) {
        let page = try_kve!(err, self.0.list());
        out.set(Box::into_raw(Box::new(ListPage(page))));
    }

    pub fn build_list(&self) -> Box<ListBuilder<'_>> {
        Box::new(ListBuilder(self.0.build_list()))
    }

    pub fn pending_list_wait(
        &self,
        pending_request_handle: u32,
        mut out: Pin<&mut *mut ListPage>,
        mut err: Pin<&mut *mut KVStoreError>,
    ) {
        // Safe because C++ should only pass back handles that were created by us.
        let handle =
            unsafe { fastly::kv_store::PendingListHandle::from_u32(pending_request_handle) };
        let page = try_kve!(err, self.0.pending_list_wait(handle));
        out.set(Box::into_raw(Box::new(ListPage(page))));
    }
}

pub fn m_static_kv_store_kv_store_open(
    name: &str,
    mut out: Pin<&mut *mut KVStore>,
    mut err: Pin<&mut *mut KVStoreError>,
) -> bool {
    match fastly::kv_store::KVStore::open(name) {
        Ok(store) => store
            .map(|s| {
                out.set(Box::into_raw(Box::new(KVStore(s))));
            })
            .is_some(),
        Err(e) => {
            err.set(Box::into_raw(Box::new(KVStoreError(e))));
            false
        }
    }
}

pub fn f_kv_store_kv_store_force_symbols(x: Box<KVStore>) -> Box<KVStore> {
    x
}
pub fn f_kv_store_kv_store_error_force_symbols(x: Box<KVStoreError>) -> Box<KVStoreError> {
    x
}
pub fn f_kv_store_lookup_response_force_symbols(x: Box<LookupResponse>) -> Box<LookupResponse> {
    x
}
