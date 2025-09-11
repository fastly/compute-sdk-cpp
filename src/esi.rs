use std::{
    pin::{self, Pin},
    ptr,
    str::ParseBoolError,
};

use cxx::CxxString;
use esi::Configuration;

use crate::{
    error::FastlyError,
    http::{request::Request, response::Response},
    try_fe,
};

pub(crate) struct ExecutionError(esi::ExecutionError);

pub(crate) struct Processor(esi::Processor);

pub(crate) struct DispatchFragmentRequestFn;
pub(crate) struct ProcessFragmentResponseFn;

pub fn m_esi_processor_process_response(
    processor: Box<Processor>,
    src_document: &mut Response,
    client_response_metadata: *mut Box<Response>,
    dispatch_fragment_request: *const DispatchFragmentRequestFn,
    process_fragment_response: *const ProcessFragmentResponseFn,
    mut err: Pin<&mut *mut ExecutionError>,
) -> bool {
    let client_response_metadata = if client_response_metadata.is_null() {
        None
    } else {
        Some(unsafe { client_response_metadata.read().0 })
    };
    let dispatch_fragment_request = if dispatch_fragment_request.is_null() {
        None
    } else {
        let func = unsafe { dispatch_fragment_request.read() };
        let shim: Box<
            dyn Fn(fastly::Request) -> Result<esi::PendingFragmentContent, esi::ExecutionError>,
        > =
            Box::new(move |req| {
                let mut out_pending = ptr::null_mut();
                let mut out_completed = ptr::null_mut();
                let mut err = ptr::null_mut();
                let result = unsafe {
                    crate::manual_ffi::fastly_esi_manualbridge_DispatchFragmentRequestFn_call(
                        &func,
                        Box::into_raw(Box::new(Request(req))),
                        &mut out_pending,
                        &mut out_completed,
                        &mut err,
                    )
                };
                match result {
                    1 => Ok(unsafe {
                        esi::PendingFragmentContent::PendingRequest(out_pending.read().0)
                    }),
                    2 => Ok(unsafe {
                        esi::PendingFragmentContent::CompletedRequest(out_completed.read().0)
                    }),
                    3 => Ok(esi::PendingFragmentContent::NoContent),
                    0 => Err(unsafe { err.read().0 }),
                    _ => unreachable!(),
                }
            });
        Some(shim)
    };
    let process_fragment_response = if process_fragment_response.is_null() {
        None
    } else {
        let func = unsafe { process_fragment_response.read() };
        let shim: Box<
            dyn Fn(
                &mut fastly::Request,
                fastly::Response,
            ) -> Result<fastly::Response, esi::ExecutionError>,
        > = Box::new(move |req, resp| {
            let mut out_resp = ptr::null_mut();
            let mut err = ptr::null_mut();
            let result = unsafe {
                crate::manual_ffi::fastly_esi_manualbridge_ProcessFragmentResponseFn_call(
                    &func,
                    Box::into_raw(Box::new(Request(req.clone_with_body()))), //TODO this leaks currently
                    Box::into_raw(Box::new(Response(resp))),
                    &mut out_resp,
                    &mut err,
                )
            };
            match result {
                true => Ok(unsafe { out_resp.read().0 }),
                false => Err(unsafe { err.read().0 }),
            }
        });
        Some(shim)
    };
    match processor.0.process_response(
        &mut src_document.0,
        client_response_metadata,
        dispatch_fragment_request.as_deref(),
        process_fragment_response.as_deref(),
    ) {
        Ok(_) => {
            err.set(ptr::null_mut());
            true
        }
        Err(e) => {
            err.set(Box::into_raw(Box::new(ExecutionError(e))));
            false
        }
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
