use std::pin::Pin;

use cxx::{CxxString, CxxVector};

pub struct HeaderValuesIter(pub Box<dyn Iterator<Item = http::HeaderValue>>);

impl HeaderValuesIter {
    pub fn next(
        &mut self,
        mut value_out: Pin<&mut CxxVector<u8>>,
        mut is_sensitive_out: Pin<&mut bool>,
    ) -> bool {
        self.0
            .next()
            .map(|val| {
                for byte in val.as_bytes() {
                    value_out.as_mut().push(*byte);
                }
                is_sensitive_out.set(val.is_sensitive());
            })
            .is_some()
    }
}

pub struct HeaderNamesIter(pub Box<dyn Iterator<Item = http::HeaderName>>);

impl HeaderNamesIter {
    pub fn next(&mut self, mut out: Pin<&mut CxxString>) -> bool {
        self.0
            .next()
            .map(|name| {
                out.as_mut().push_str(name.as_str());
                true
            })
            .is_some()
    }
}

pub struct OriginalHeaderNamesIter(pub Box<dyn Iterator<Item = String>>);

impl OriginalHeaderNamesIter {
    pub fn next(&mut self, mut out: Pin<&mut CxxString>) -> bool {
        self.0
            .next()
            .map(|name| {
                out.as_mut().push_str(&name);
                true
            })
            .is_some()
    }
}

pub struct HeadersIter(pub Box<dyn Iterator<Item = (http::HeaderName, http::HeaderValue)>>);

impl HeadersIter {
    pub fn next(
        &mut self,
        mut name_out: Pin<&mut CxxString>,
        mut value_out: Pin<&mut CxxVector<u8>>,
        mut is_sensitive_out: Pin<&mut bool>,
    ) -> bool {
        if let Some((name, value)) = self.0.next() {
            name_out.as_mut().push_str(name.as_str());
            for byte in value.as_bytes() {
                value_out.as_mut().push(*byte);
            }
            is_sensitive_out.set(value.is_sensitive());
            true
        } else {
            false
        }
    }
}

// Needed to force generation of `drop` functions for the iterators.
pub fn f_headers_iter_force_symbols(val: Box<HeadersIter>) -> Box<HeadersIter> {
    val
}
pub fn f_header_names_iter_force_symbols(val: Box<HeaderNamesIter>) -> Box<HeaderNamesIter> {
    val
}
pub fn f_header_values_iter_force_symbols(val: Box<HeaderValuesIter>) -> Box<HeaderValuesIter> {
    val
}
pub fn f_original_header_names_iter_force_symbols(
    val: Box<OriginalHeaderNamesIter>,
) -> Box<OriginalHeaderNamesIter> {
    val
}
