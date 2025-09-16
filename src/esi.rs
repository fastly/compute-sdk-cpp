use std::{
    pin::{Pin, pin},
    ptr,
};

use cxx::CxxString;
use esi::Configuration;

use crate::{
    error::FastlyError,
    ffi::{
        DispatchFragmentRequestFnResult, DispatchFragmentRequestFnTag, ProcessFragmentResponseFnTag,
    },
    http::{request::Request, response::Response},
    try_fe,
};

pub(crate) struct Processor(esi::Processor);

type DispatchFragmentRequestFnType =
    dyn Fn(fastly::Request) -> Result<esi::PendingFragmentContent, esi::ExecutionError>;
fn shim_dispatch_fragment_request_fn(
    func: *const DispatchFragmentRequestFnTag,
) -> Option<Box<DispatchFragmentRequestFnType>> {
    if func.is_null() {
        return None;
    }
    let shim = Box::new(move |req| {
        let mut out_pending = pin!(ptr::null_mut());
        let mut out_completed = pin!(ptr::null_mut());
        let result = unsafe {
            crate::manual_ffi::fastly_esi_manualbridge_DispatchFragmentRequestFn_call(
                func,
                Box::into_raw(Box::new(Request(req))),
                &mut out_pending,
                &mut out_completed,
            )
        };
        match result {
            DispatchFragmentRequestFnResult::PendingRequest => {
                Ok(unsafe { esi::PendingFragmentContent::PendingRequest(out_pending.read().0) })
            }
            DispatchFragmentRequestFnResult::CompletedRequest => {
                Ok(
                    unsafe {
                        esi::PendingFragmentContent::CompletedRequest(out_completed.read().0)
                    },
                )
            }
            DispatchFragmentRequestFnResult::NoContent => {
                Ok(esi::PendingFragmentContent::NoContent)
            }
            DispatchFragmentRequestFnResult::Error => Err(esi::ExecutionError::FunctionError(
                "dispatch_fragment_request".into(),
            )),
            _ => unreachable!(),
        }
    });
    Some(shim)
}

type ProcessFragmentResponseFnType =
    dyn Fn(&mut fastly::Request, fastly::Response) -> Result<fastly::Response, esi::ExecutionError>;
fn shim_process_fragment_response_fn(
    func: *const ProcessFragmentResponseFnTag,
) -> Option<Box<ProcessFragmentResponseFnType>> {
    if func.is_null() {
        return None;
    }
    let shim = Box::new(move |req: &mut fastly::Request, resp| {
        let mut out_resp = pin!(ptr::null_mut());
        let result = unsafe {
            crate::manual_ffi::fastly_esi_manualbridge_ProcessFragmentResponseFn_call(
                func,
                // Ideally we wouldn't do this and would instead pass a reference, but
                // that would require a separate wrapper type for Request references.
                Box::into_raw(Box::new(Request(req.clone_with_body()))),
                Box::into_raw(Box::new(Response(resp))),
                &mut out_resp,
            )
        };
        match result {
            true => Ok(unsafe { out_resp.read().0 }),
            false => Err(esi::ExecutionError::FunctionError(
                "process_fragment_response".into(),
            )),
        }
    });
    Some(shim)
}
pub fn m_esi_processor_process_response(
    processor: Box<Processor>,
    src_document: &mut Response,
    client_response_metadata: *mut Response,
    dispatch_fragment_request: *const DispatchFragmentRequestFnTag,
    process_fragment_response: *const ProcessFragmentResponseFnTag,
    mut err: Pin<&mut *mut FastlyError>,
) -> bool {
    let client_response_metadata = if client_response_metadata.is_null() {
        None
    } else {
        // Make sure to take ownership, as this pointer is modelling an Optional<Box<_>>
        Some(unsafe { Box::from_raw(client_response_metadata) })
    };
    try_fe!(
        err,
        processor
            .0
            .process_response(
                &mut src_document.0,
                client_response_metadata.map(|r| r.0),
                shim_dispatch_fragment_request_fn(dispatch_fragment_request).as_deref(),
                shim_process_fragment_response_fn(process_fragment_response).as_deref(),
            )
            .map_err(FastlyError::ESIError)
    );
    true
}

pub fn m_esi_processor_process_document(
    processor: Box<Processor>,
    src_document: &CxxString,
    dispatch_fragment_request: *const DispatchFragmentRequestFnTag,
    process_fragment_response: *const ProcessFragmentResponseFnTag,
    out: Pin<&mut CxxString>,
    mut err: Pin<&mut *mut FastlyError>,
) -> bool {
    let doc_str = try_fe!(err, src_document.to_str().map_err(FastlyError::Utf8Error));
    let reader = quick_xml::reader::Reader::from_str(doc_str);
    let mut writer = quick_xml::Writer::new(out);
    try_fe!(
        err,
        processor
            .0
            .process_document(
                reader,
                &mut writer,
                shim_dispatch_fragment_request_fn(dispatch_fragment_request).as_deref(),
                shim_process_fragment_response_fn(process_fragment_response).as_deref(),
            )
            .map_err(FastlyError::ESIError)
    );
    true
}

pub fn m_static_esi_processor_new(
    original_request_metadata: *mut Request,
    namespace: &CxxString,
    is_escaped_content: bool,
) -> Box<Processor> {
    let original_request_metadata = if original_request_metadata.is_null() {
        None
    } else {
        // Make sure to take ownership, as this pointer is modelling an Optional<Box<_>>
        Some(unsafe { Box::from_raw(original_request_metadata) })
    };
    Box::new(Processor(esi::Processor::new(
        original_request_metadata.map(|r| r.0),
        Configuration::default()
            .with_escaped(is_escaped_content)
            .with_namespace(namespace.to_string()),
    )))
}
