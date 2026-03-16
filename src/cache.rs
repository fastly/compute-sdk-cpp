use sha2::{Digest, Sha256};
use std::fmt::Write;

use crate::ffi::PurgeScope;

// Copied from fastly/src/cache/simple.rs with some edits, as that function is not exposed in the public API.
pub fn f_cache_surrogate_key_for_cache_key(key: &[u8], scope: PurgeScope) -> String {
    let mut sha = Sha256::new();
    sha.update(key);
    if let PurgeScope::Pop = scope {
        // if the POP string is empty for some reason, this will amount to a global purge
        // for now which is the safer choice
        let pop = fastly::compute_runtime::pop();
        sha.update(pop);
    }
    let mut sk_str = String::new();
    for b in sha.finalize() {
        write!(&mut sk_str, "{b:02X}").expect("writing to a String is infallible");
    }
    sk_str
}
