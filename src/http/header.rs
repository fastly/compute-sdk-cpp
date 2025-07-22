use cxx::{CxxVector, UniquePtr};

pub struct HeaderValuesIter(pub Box<dyn Iterator<Item = UniquePtr<CxxVector<u8>>>>);

impl HeaderValuesIter {
    pub fn next(&mut self) -> UniquePtr<CxxVector<u8>> {
        self.0.next().unwrap_or_else(CxxVector::new)
    }
}
