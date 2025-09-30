use std::pin::Pin;

use cxx::CxxString;

use crate::{error::ErrPtr, try_fe};

pub struct ConfigStore(pub(crate) fastly::ConfigStore);

pub fn m_static_config_store_config_store_open(
    name: &CxxString,
    mut out: Pin<&mut *mut ConfigStore>,
    mut err: ErrPtr,
) {
    out.set(Box::into_raw(Box::new(ConfigStore(try_fe!(
        err,
        fastly::ConfigStore::try_open(try_fe!(err, name.to_str()))
    )))))
}

impl ConfigStore {
    pub fn get(&self, key: &CxxString, out: Pin<&mut CxxString>, mut err: ErrPtr) -> bool {
        try_fe!(err, self.0.try_get(try_fe!(err, key.to_str())))
            .map(|val| out.push_str(val.as_ref()))
            .is_some()
    }

    pub fn contains(&self, key: &CxxString, mut err: ErrPtr) -> bool {
        self.0.contains(try_fe!(err, key.to_str()))
    }
}

pub fn f_config_store_config_store_force_symbols(x: Box<ConfigStore>) -> Box<ConfigStore> {
    x
}
