use std::pin::Pin;

use cxx::CxxString;

use crate::{error::ErrPtr, try_fe};

pub struct Device(pub(crate) fastly::device_detection::Device);

// Just used to force generation of symbols, since we're otherwise doing raw pointers.
pub fn f_device_detection_noop(dev: Box<Device>) -> Box<Device> {
    dev
}

pub fn f_device_detection_lookup(
    user_agent: &CxxString,
    mut out: Pin<&mut *mut Device>,
    mut err: ErrPtr,
) {
    out.set(
        fastly::device_detection::lookup(try_fe!(err, user_agent.to_str()))
            .map(Device)
            .map(Box::new)
            .map(Box::into_raw)
            .unwrap_or_else(std::ptr::null_mut),
    )
}

macro_rules! maybe_props {
    ( $( $name:ident ),* ) => {
        impl Device {
            $(
                pub fn $name(&self, out: Pin<&mut CxxString>) -> bool {
                    self.0.$name().map(|val| out.push_str(val)).is_some()
                }
            )*
        }
    }
}

macro_rules! maybe_predicates {
    ( $( $name:ident ),* ) => {
        impl Device {
            $(
                pub fn $name(&self) -> *const bool {
                    if let Some(b) = self.0.$name() {
                        Box::into_raw(Box::new(b))
                    } else {
                        std::ptr::null()
                    }
                }
            )*
        }
    }
}

maybe_props![device_name, brand, model, hwtype];
maybe_predicates![
    is_ereader,
    is_gameconsole,
    is_mediaplayer,
    is_mobile,
    is_smarttv,
    is_tablet,
    is_tvplayer,
    is_desktop,
    is_touchscreen
];
