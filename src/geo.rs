use std::pin::Pin;

use crate::ffi::{ConnSpeed, ConnType, Continent, ProxyDescription, ProxyType};
use cxx::CxxString;
use fastly::geo::{
    ConnSpeed as FConnSpeed, ConnType as FConnType, Continent as FContinent,
    ProxyDescription as FProxyDescription, ProxyType as FProxyType,
};

pub struct Geo(pub(crate) fastly::geo::Geo);

pub fn f_geo_geo_lookup(ip: &CxxString) -> *mut Geo {
    if let Some(geo) = fastly::geo::geo_lookup(
        ip.to_string_lossy()
            .as_ref()
            .parse()
            .expect("bad ip format"),
    ) {
        Box::into_raw(Box::new(Geo(geo)))
    } else {
        std::ptr::null_mut()
    }
}

impl Geo {
    pub fn as_name(&self, out: Pin<&mut CxxString>) {
        out.push_str(self.0.as_name());
    }

    pub fn as_number(&self) -> u32 {
        self.0.as_number()
    }

    pub fn city(&self, out: Pin<&mut CxxString>) {
        out.push_str(self.0.city())
    }

    pub fn area_code(&self) -> u16 {
        self.0.area_code()
    }

    pub fn conn_speed(&self) -> ConnSpeed {
        self.0.conn_speed().into()
    }

    pub fn conn_type(&self) -> ConnType {
        self.0.conn_type().into()
    }

    pub fn continent(&self) -> Continent {
        self.0.continent().into()
    }

    pub fn country_code(&self, out: Pin<&mut CxxString>) {
        out.push_str(self.0.country_code());
    }

    pub fn country_code3(&self, out: Pin<&mut CxxString>) {
        out.push_str(self.0.country_code3());
    }

    pub fn country_name(&self, out: Pin<&mut CxxString>) {
        out.push_str(self.0.country_name());
    }

    pub fn latitude(&self) -> f64 {
        self.0.latitude()
    }

    pub fn longitude(&self) -> f64 {
        self.0.longitude()
    }

    pub fn metro_code(&self) -> i64 {
        self.0.metro_code()
    }

    pub fn postal_code(&self, out: Pin<&mut CxxString>) {
        out.push_str(self.0.postal_code());
    }

    pub fn proxy_description(&self) -> ProxyDescription {
        self.0.proxy_description().into()
    }

    pub fn proxy_type(&self) -> ProxyType {
        self.0.proxy_type().into()
    }

    pub fn region(&self, out: Pin<&mut CxxString>) -> bool {
        self.0.region().map(|v| out.push_str(v)).is_some()
    }

    pub fn utc_offset(&self) -> *mut UtcOffset {
        self.0
            .utc_offset()
            .map(UtcOffset)
            .map(Box::new)
            .map(Box::into_raw)
            .unwrap_or(std::ptr::null_mut())
    }
}

impl From<FConnSpeed> for ConnSpeed {
    fn from(value: FConnSpeed) -> Self {
        match value {
            FConnSpeed::Broadband => ConnSpeed::Broadband,
            FConnSpeed::Cable => ConnSpeed::Cable,
            FConnSpeed::Dialup => ConnSpeed::Dialup,
            FConnSpeed::Mobile => ConnSpeed::Mobile,
            FConnSpeed::Oc12 => ConnSpeed::Oc12,
            FConnSpeed::Oc3 => ConnSpeed::Oc3,
            FConnSpeed::Satellite => ConnSpeed::Satellite,
            FConnSpeed::T1 => ConnSpeed::T1,
            FConnSpeed::T3 => ConnSpeed::T3,
            FConnSpeed::UltraBroadband => ConnSpeed::UltraBroadband,
            FConnSpeed::Wireless => ConnSpeed::Wireless,
            FConnSpeed::Xdsl => ConnSpeed::Xdsl,
            FConnSpeed::Other(_) => ConnSpeed::Other,
            _ => ConnSpeed::Other,
        }
    }
}

impl From<FConnType> for ConnType {
    fn from(value: FConnType) -> Self {
        match value {
            FConnType::Wired => ConnType::Wired,
            FConnType::Wifi => ConnType::Wifi,
            FConnType::Mobile => ConnType::Mobile,
            FConnType::Dialup => ConnType::Dialup,
            FConnType::Satellite => ConnType::Satellite,
            FConnType::Unknown => ConnType::Unknown,
            FConnType::Other(_) => ConnType::Other,
            _ => ConnType::Other,
        }
    }
}

impl From<FContinent> for Continent {
    fn from(value: FContinent) -> Self {
        match value {
            FContinent::Africa => Continent::Africa,
            FContinent::Antarctica => Continent::Antarctica,
            FContinent::Asia => Continent::Asia,
            FContinent::Europe => Continent::Europe,
            FContinent::NorthAmerica => Continent::NorthAmerica,
            FContinent::Oceania => Continent::Oceania,
            FContinent::SouthAmerica => Continent::SouthAmerica,
            FContinent::Other(_) => Continent::Other,
        }
    }
}

impl From<FProxyDescription> for ProxyDescription {
    fn from(value: FProxyDescription) -> Self {
        match value {
            FProxyDescription::Cloud => ProxyDescription::Cloud,
            FProxyDescription::CloudSecurity => ProxyDescription::CloudSecurity,
            FProxyDescription::Dns => ProxyDescription::Dns,
            FProxyDescription::TorExit => ProxyDescription::TorExit,
            FProxyDescription::TorRelay => ProxyDescription::TorRelay,
            FProxyDescription::Vpn => ProxyDescription::Vpn,
            FProxyDescription::WebBrowser => ProxyDescription::WebBrowser,
            FProxyDescription::Unknown => ProxyDescription::Unknown,
            FProxyDescription::Other(_) => ProxyDescription::Other,
            _ => ProxyDescription::Other,
        }
    }
}

impl From<FProxyType> for ProxyType {
    fn from(value: FProxyType) -> Self {
        match value {
            FProxyType::Anonymous => ProxyType::Anonymous,
            FProxyType::Aol => ProxyType::Aol,
            FProxyType::Blackberry => ProxyType::Blackberry,
            FProxyType::Corporate => ProxyType::Corporate,
            FProxyType::Edu => ProxyType::Edu,
            FProxyType::Hosting => ProxyType::Hosting,
            FProxyType::Public => ProxyType::Public,
            FProxyType::Transparent => ProxyType::Transparent,
            FProxyType::Unknown => ProxyType::Unknown,
            FProxyType::Other(_) => ProxyType::Other,
            _ => ProxyType::Other,
        }
    }
}

pub struct UtcOffset(pub(crate) fastly::geo::UtcOffset);

impl UtcOffset {
    pub fn whole_hours(&self) -> i8 {
        self.0.whole_hours()
    }

    pub fn whole_minutes(&self) -> i16 {
        self.0.whole_minutes()
    }

    pub fn minutes_past_hour(&self) -> i8 {
        self.0.minutes_past_hour()
    }

    pub fn whole_seconds(&self) -> i32 {
        self.0.whole_seconds()
    }

    pub fn seconds_past_minute(&self) -> i8 {
        self.0.seconds_past_minute()
    }

    pub fn is_utc(&self) -> bool {
        self.0.is_utc()
    }

    pub fn is_positive(&self) -> bool {
        self.0.is_positive()
    }

    pub fn is_negative(&self) -> bool {
        self.0.is_negative()
    }
}
