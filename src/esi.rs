use std::{
    pin::{self, Pin},
    ptr,
    str::ParseBoolError,
};

use cxx::CxxString;
use esi::Configuration;

use crate::{
    error::FastlyError,
    ffi::{DispatchFragmentRequestFnTag, ProcessFragmentResponseFnTag},
    http::{request::Request, response::Response},
    try_fe,
};

pub(crate) struct Processor(esi::Processor);

fn shim_dispatch_fragment_request_fn(
    func: *const DispatchFragmentRequestFnTag,
) -> Option<Box<dyn Fn(fastly::Request) -> Result<esi::PendingFragmentContent, esi::ExecutionError>>>
{
    if func.is_null() {
        return None;
    }
    let shim = Box::new(move |req| {
        let mut out_pending = ptr::null_mut();
        let mut out_completed = ptr::null_mut();
        let result = unsafe {
            crate::manual_ffi::fastly_esi_manualbridge_DispatchFragmentRequestFn_call(
                func,
                Box::into_raw(Box::new(Request(req))),
                &mut out_pending,
                &mut out_completed,
            )
        };
        match result {
            1 => Ok(unsafe { esi::PendingFragmentContent::PendingRequest(out_pending.read().0) }),
            2 => {
                Ok(
                    unsafe {
                        esi::PendingFragmentContent::CompletedRequest(out_completed.read().0)
                    },
                )
            }
            3 => Ok(esi::PendingFragmentContent::NoContent),
            0 => Err(esi::ExecutionError::FunctionError(
                "dispatch_fragment_request".into(),
            )),
            _ => unreachable!(),
        }
    });
    Some(shim)
}

fn shim_process_fragment_response_fn(
    func: *const ProcessFragmentResponseFnTag,
) -> Option<
    Box<
        dyn Fn(
            &mut fastly::Request,
            fastly::Response,
        ) -> Result<fastly::Response, esi::ExecutionError>,
    >,
> {
    if func.is_null() {
        return None;
    }
    let shim = Box::new(move |req: &mut fastly::Request, resp| {
        let mut out_resp = ptr::null_mut();
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
    match processor.0.process_response(
        &mut src_document.0,
        client_response_metadata.map(|r| r.0),
        shim_dispatch_fragment_request_fn(dispatch_fragment_request).as_deref(),
        shim_process_fragment_response_fn(process_fragment_response).as_deref(),
    ) {
        Ok(_) => {
            err.set(ptr::null_mut());
            true
        }
        Err(e) => {
            err.set(Box::into_raw(Box::new(FastlyError::ESIError(e))));
            false
        }
    }
}

pub fn m_static_esi_processor_new(
    original_request_metadata: *mut Request,
    namespace: &CxxString,
    is_escaped_content: bool,
) -> Box<Processor> {
    println!(
        "Original request metadata ptr: {:p}",
        original_request_metadata
    );
    let original_request_metadata = if original_request_metadata.is_null() {
        None
    } else {
        // Make sure to take ownership, as this pointer is modelling an Optional<Box<_>>
        Some(unsafe { Box::from_raw(original_request_metadata) })
    };
    println!(
        "Original request metadata: {:?}",
        original_request_metadata
            .as_ref()
            .map_or("None".into(), |r| r.0.get_url().to_string())
    );
    Box::new(Processor(esi::Processor::new(
        original_request_metadata.map(|r| r.0),
        Configuration::default()
            .with_escaped(is_escaped_content)
            .with_namespace(namespace.to_string()),
    )))
}
