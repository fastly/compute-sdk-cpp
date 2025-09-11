use std::pin::Pin;

use cxx::CxxString;
use esi::Configuration;

use crate::{
    http::{request::Request, response::Response},
    try_fe,
};

pub(crate) struct Processor(esi::Processor);

impl Processor {
    pub fn process_response(
        self,
        src_document: &mut Response,
        client_response_metadata: *mut Box<Response>,
        dispatch_fragment_request: Option<&dyn Fn(Request) -> Result<PendingFragmentContent>>,
        process_fragment_response: Option<&dyn Fn(&mut Request, Response) -> Result<Response>>,
    ) -> Result<()> {
    }
}

pub fn m_static_esi_processor_new(
    original_request_metadata: *mut Box<Request>,
    namespace: &CxxString,
    is_escaped_content: bool,
) -> Box<Processor> {
    let original_request_metadata = if original_request_metadata.is_null() {
        None
    } else {
        Some(unsafe { original_request_metadata.read().0 })
    };
    Box::new(Processor(esi::Processor::new(
        original_request_metadata,
        Configuration::default()
            .with_escaped(is_escaped_content)
            .with_namespace(namespace.to_string()),
    )))
}
