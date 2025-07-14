use std::pin::Pin;

use cxx::{CxxString, CxxVector};

use crate::{
    error::{ErrPtr, FastlyStatusWrapper},
    try_fe,
};

pub struct Secret(pub(crate) fastly::secret_store::Secret);

pub fn m_static_secret_store_secret_from_bytes(
    bytes: &CxxVector<u8>,
    mut out: Pin<&mut *mut Secret>,
    mut err: ErrPtr,
) {
    out.set(Box::into_raw(Box::new(Secret(try_fe!(
        err,
        fastly::secret_store::Secret::from_bytes(bytes.as_slice().to_owned())
            .map_err(FastlyStatusWrapper)
    )))))
}

impl Secret {
    pub fn plaintext(&self, out: Pin<&mut CxxString>) {
        out.push_bytes(self.0.plaintext().as_ref());
    }
}

pub struct SecretStore(pub(crate) fastly::SecretStore);

pub fn m_static_secret_store_secret_store_open(
    name: &CxxString,
    mut out: Pin<&mut *mut SecretStore>,
    mut err: ErrPtr,
) {
    out.set(Box::into_raw(Box::new(SecretStore(try_fe!(
        err,
        fastly::SecretStore::open(try_fe!(err, name.to_str()))
    )))))
}

impl SecretStore {
    pub fn get(&self, key: &CxxString, mut out: Pin<&mut *mut Secret>, mut err: ErrPtr) {
        out.set(
            self.0
                .get(try_fe!(err, key.to_str()))
                .map(Secret)
                .map(Box::new)
                .map(Box::into_raw)
                .unwrap_or_else(std::ptr::null_mut),
        );
    }

    pub fn contains(&self, key: &CxxString, mut err: ErrPtr) -> bool {
        try_fe!(err, self.0.contains(try_fe!(err, key.to_str())))
    }
}
