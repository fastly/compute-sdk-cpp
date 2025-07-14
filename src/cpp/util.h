#define FSLY_BOX(NS, CLS, PTR)                                                 \
  ::fastly::NS::CLS(std::move(::rust::Box<fastly::sys::NS::CLS>::from_raw(PTR)))
