// Clippy has a few false positives in this crate for lifetimes and boxing
// due to the way cxx works.
#![allow(clippy::boxed_local, clippy::needless_lifetimes)]

use backend::*;
use config_store::*;
use device_detection::*;
use error::*;
use esi::*;
use geo::*;
use http::{
    body::*, header::*, purge::*, request::request::*, request::*, response::*, status_code::*,
};
use kv_store::*;
use log::*;
use secret_store::*;
use security::*;

mod backend;
mod config_store;
mod device_detection;
mod error;
mod esi;
mod geo;
mod http;
mod kv_store;
mod log;
mod secret_store;
mod security;

// Unfortunately, due to some limitations with cxx, the ENTIRE bridge basically
// has to be under a single ffi module, or cross-referencing ffi types is gonna
// break.
#[cxx::bridge]
mod ffi {
    #[namespace = "fastly::sys::error"]
    #[derive(Copy, Clone, Debug)]
    #[repr(usize)]
    pub enum FastlyErrorCode {
        Utf8Error,
        InvalidHeaderName,
        InvalidHeaderValue,
        InvalidStatusCode,
        IoError,
        FastlyError,
        FastlySendError,
        AddrParseError,
        BackendError,
        BackendCreationError,
        FastlyStatus,
        ConfigStoreOpenError,
        ConfigStoreLookupError,
        SecretStoreOpenError,
        SecretStoreLookupError,
        LogError,
        ESIError,
    }

    #[namespace = "fastly::sys::http"]
    #[derive(Copy, Clone, Debug)]
    pub enum Method {
        GET,
        POST,
        PUT,
        DELETE,
        HEAD,
        OPTIONS,
        CONNECT,
        PATCH,
        TRACE,
    }

    #[namespace = "fastly::sys::http"]
    #[derive(Copy, Clone, Debug)]
    pub enum Version {
        HTTP_09,
        HTTP_10,
        HTTP_11,
        HTTP_2,
        HTTP_3,
    }

    /// Connection speed.
    ///
    /// These connection speeds imply different latencies, as well as throughput.
    ///
    /// See [OC rates][oc] and [T-carrier][t] for background on OC- and T- connections.
    ///
    /// [oc]: https://en.wikipedia.org/wiki/Optical_Carrier_transmission_rates
    /// [t]: https://en.wikipedia.org/wiki/T-carrier
    #[namespace = "fastly::sys::geo"]
    #[derive(Copy, Clone, Debug)]
    pub enum ConnSpeed {
        Broadband,
        Cable,
        Dialup,
        Mobile,
        Oc12,
        Oc3,
        Satellite,
        T1,
        T3,
        UltraBroadband,
        Wireless,
        Xdsl,
        Other,
    }

    /// Connection type.
    ///
    /// Defaults to `Unknown` when the connection type is not known. `Other`
    /// means a connection known to the database but that this version of the
    /// library might not yet be aware of.
    #[namespace = "fastly::sys::geo"]
    #[derive(Copy, Clone, Debug)]
    pub enum ConnType {
        Wired,
        Wifi,
        Mobile,
        Dialup,
        Satellite,
        Unknown,
        Other,
    }

    /// Continent.
    #[namespace = "fastly::sys::geo"]
    #[derive(Copy, Clone, Debug)]
    pub enum Continent {
        Africa,
        Antarctica,
        Asia,
        Europe,
        NorthAmerica,
        Oceania,
        SouthAmerica,
        Other,
    }

    /// Client proxy description.
    ///
    /// Defaults to `Unknown` when an IP address is not known to be a proxy or VPN.
    #[namespace = "fastly::sys::geo"]
    #[derive(Copy, Clone, Debug)]
    pub enum ProxyDescription {
        /// Enables ubiquitous network access to a shared pool of configurable
        /// computing resources.
        Cloud,

        /// A host accessing the internet via a web security and data protection
        /// cloud provider.
        ///
        /// Example providers with this type of service are Zscaler, Scansafe,
        /// and Onavo.
        CloudSecurity,

        /// A proxy used by overriding the client's DNS value for an endpoint
        /// host to that of the proxy instead of the actual DNS value.
        Dns,

        /// The gateway nodes where encrypted or anonymous Tor traffic hits the
        /// internet.
        TorExit,

        /// Receives traffic on the Tor network and passes it along; also
        /// referred to as "routers".
        TorRelay,

        /// Virtual private network that encrypts and routes all traffic through
        /// the VPN server, including programs and applications.
        Vpn,

        /// Connectivity that is taking place through mobile device web browser software that proxies
        /// the user through a centralized location.
        ///
        /// Examples of such browsers are Opera mobile browsers and UCBrowser.
        WebBrowser,

        /// An IP address that is not known to be a proxy or VPN.
        Unknown,

        /// Description of a proxy or VPN that is known, but not in the above list of variants.
        ///
        /// This typically indicates that the geolocation database contains a proxy description that
        /// did not exist when this crate was published.
        Other,
    }

    /// Client proxy type.
    ///
    /// Defaults to `Unknown` when an IP address is not known to be a proxy or VPN.
    #[namespace = "fastly::sys::geo"]
    #[derive(Copy, Clone, Debug)]
    pub enum ProxyType {
        Anonymous,
        Aol,
        Blackberry,
        Corporate,
        Edu,
        Hosting,
        Public,
        Transparent,
        Unknown,
        Other,
    }

    #[namespace = "fastly::sys::log"]
    #[repr(usize)]
    pub enum LogLevel {
        Error = 1,
        Warn = 2,
        Info = 3,
        Debug = 4,
        Trace = 5,
    }

    #[namespace = "fastly::sys::log"]
    #[repr(usize)]
    pub enum LogLevelFilter {
        Off = 0,
        Error = 1,
        Warn = 2,
        Info = 3,
        Debug = 4,
        Trace = 5,
    }

    #[namespace = "fastly::sys::esi"]
    #[repr(usize)]
    pub enum DispatchFragmentRequestFnResult {
        Error = 0,
        PendingRequest = 1,
        CompletedRequest = 2,
        NoContent = 3,
    }

    #[namespace = "fastly::sys::error"]
    extern "Rust" {
        type FastlyError;
        fn error_code(&self) -> FastlyErrorCode;
        fn error_msg(&self, out: Pin<&mut CxxString>);
    }

    #[namespace = "fastly::sys::backend"]
    extern "Rust" {
        type Backend;
        fn m_static_backend_backend_from_name(
            name: &CxxString,
            mut out: Pin<&mut *mut Backend>,
            mut err: Pin<&mut *mut FastlyError>,
        );
        fn m_static_backend_backend_builder(
            name: &CxxString,
            target: &CxxString,
        ) -> Box<BackendBuilder>;
        fn m_backend_backend_into_string(backend: Box<Backend>) -> String;
        fn equals(&self, other: &Backend) -> bool;
        fn clone(&self) -> Box<Backend>;
        fn name(&self) -> &str;
        fn exists(&self) -> bool;
        fn is_dynamic(&self) -> bool;
        fn get_host(&self) -> String;
        // TODO
        // pub fn get_host_override(&self) {
        // }
        fn get_port(&self) -> u16;
        fn get_connect_timeout(&self) -> u32;
        fn get_first_byte_timeout(&self) -> u32;
        fn get_between_bytes_timeout(&self) -> u32;
        fn get_http_keepalive_time(&self) -> u32;
        fn get_tcp_keepalive_enable(&self) -> bool;
        fn get_tcp_keepalive_interval(&self) -> u32;
        fn get_tcp_keepalive_probes(&self) -> u32;
        fn get_tcp_keepalive_time(&self) -> u32;
        fn is_ssl(&self) -> bool;
    }

    #[namespace = "fastly::sys::backend"]
    extern "Rust" {
        type BackendBuilder;
        fn m_static_backend_backend_builder_new(
            name: &CxxString,
            target: &CxxString,
        ) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_override_host(
            mut builder: Box<BackendBuilder>,
            name: &CxxString,
        ) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_connect_timeout(
            mut builder: Box<BackendBuilder>,
            timeout: u32,
        ) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_first_byte_timeout(
            mut builder: Box<BackendBuilder>,
            timeout: u32,
        ) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_between_bytes_timeout(
            mut builder: Box<BackendBuilder>,
            timeout: u32,
        ) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_enable_ssl(
            builder: Box<BackendBuilder>,
        ) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_disable_ssl(
            builder: Box<BackendBuilder>,
        ) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_check_certificate(
            builder: Box<BackendBuilder>,
            cert: &CxxString,
        ) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_ca_certificate(
            builder: Box<BackendBuilder>,
            cert: &CxxString,
        ) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_tls_ciphers(
            builder: Box<BackendBuilder>,
            ciphers: &CxxString,
        ) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_sni_hostname(
            builder: Box<BackendBuilder>,
            host: &CxxString,
        ) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_enable_pooling(
            builder: Box<BackendBuilder>,
            value: bool,
        ) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_http_keepalive_time(
            mut builder: Box<BackendBuilder>,
            timeout: u32,
        ) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_tcp_keepalive_enable(
            builder: Box<BackendBuilder>,
            value: bool,
        ) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_tcp_keepalive_interval_secs(
            builder: Box<BackendBuilder>,
            value: u32,
        ) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_tcp_keepalive_probes(
            builder: Box<BackendBuilder>,
            value: u32,
        ) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_tcp_keepalive_time_secs(
            builder: Box<BackendBuilder>,
            value: u32,
        ) -> Box<BackendBuilder>;
        fn m_backend_backend_builder_finish(
            builder: Box<BackendBuilder>,
            mut out: Pin<&mut *mut Backend>,
            mut err: Pin<&mut *mut FastlyError>,
        );
    }
    #[namespace = "fastly::sys::http"]
    extern "Rust" {
        type HeaderValuesIter;
        fn next(
            &mut self,
            mut value_out: Pin<&mut CxxVector<u8>>,
            mut is_sensitive_out: Pin<&mut bool>,
        ) -> bool;
        // Needed to force generation of `drop`.
        fn f_header_values_iter_force_symbols(val: Box<HeaderValuesIter>) -> Box<HeaderValuesIter>;
    }

    #[namespace = "fastly::sys::http"]
    extern "Rust" {
        type HeaderNamesIter;
        fn next(&mut self, mut out: Pin<&mut CxxString>) -> bool;
        // Needed to force generation of `drop`.
        fn f_header_names_iter_force_symbols(val: Box<HeaderNamesIter>) -> Box<HeaderNamesIter>;
    }

    #[namespace = "fastly::sys::http"]
    extern "Rust" {
        type OriginalHeaderNamesIter;
        fn next(&mut self, mut out: Pin<&mut CxxString>) -> bool;
        // Needed to force generation of `drop`.
        fn f_original_header_names_iter_force_symbols(
            val: Box<OriginalHeaderNamesIter>,
        ) -> Box<OriginalHeaderNamesIter>;
    }

    #[namespace = "fastly::sys::http"]
    extern "Rust" {
        type HeadersIter;
        fn next(
            &mut self,
            mut name_out: Pin<&mut CxxString>,
            mut value_out: Pin<&mut CxxVector<u8>>,
            mut is_sensitive_out: Pin<&mut bool>,
        ) -> bool;
        // Needed to force generation of `drop`.
        fn f_headers_iter_force_symbols(val: Box<HeadersIter>) -> Box<HeadersIter>;
    }

    #[namespace = "fastly::sys::http"]
    extern "Rust" {
        type Request;

        // Static methods
        fn m_static_http_request_get(url: &CxxString) -> Box<Request>;
        fn m_static_http_request_head(url: &CxxString) -> Box<Request>;
        fn m_static_http_request_post(url: &CxxString) -> Box<Request>;
        fn m_static_http_request_put(url: &CxxString) -> Box<Request>;
        fn m_static_http_request_delete(url: &CxxString) -> Box<Request>;
        fn m_static_http_request_connect(url: &CxxString) -> Box<Request>;
        fn m_static_http_request_options(url: &CxxString) -> Box<Request>;
        fn m_static_http_request_trace(url: &CxxString) -> Box<Request>;
        fn m_static_http_request_patch(url: &CxxString) -> Box<Request>;
        fn m_static_http_request_new(method: Method, url: &CxxString) -> Box<Request>;
        fn m_static_http_request_from_client() -> Box<Request>;

        // Regular methods
        fn is_from_client(&self) -> bool;
        fn clone_without_body(&self) -> Box<Request>;
        fn clone_with_body(&mut self) -> Box<Request>;
        fn m_http_request_send(
            request: Box<Request>,
            backend: &Backend,
            out: Pin<&mut *mut Response>,
            err: Pin<&mut *mut FastlyError>,
        );
        fn m_http_request_send_async(
            request: Box<Request>,
            backend: &Backend,
            out: Pin<&mut *mut PendingRequest>,
            err: Pin<&mut *mut FastlyError>,
        );
        fn m_http_request_send_async_streaming(
            request: Box<Request>,
            backend: &Backend,
            out: Pin<&mut *mut AsyncStreamRes>,
            err: Pin<&mut *mut FastlyError>,
        );
        fn set_body(&mut self, body: Box<Body>);
        fn has_body(&self) -> bool;
        fn take_body(&mut self) -> Box<Body>;
        fn m_http_request_into_body(request: Box<Request>) -> Box<Body>;
        fn set_body_text_plain(&mut self, body: &CxxString, mut err: Pin<&mut *mut FastlyError>);
        fn set_body_text_html(&mut self, body: &CxxString, mut err: Pin<&mut *mut FastlyError>);
        fn set_body_octet_stream(&mut self, body: &CxxVector<u8>);
        fn get_content_type(&self, mut out: Pin<&mut CxxString>) -> bool;
        fn set_content_type(&mut self, mime: &CxxString);
        fn get_content_length(&self, mut out: Pin<&mut usize>) -> bool;
        fn contains_header(&self, name: &CxxString, mut err: Pin<&mut *mut FastlyError>) -> bool;
        fn get_header(
            &self,
            name: &CxxString,
            value_out: Pin<&mut CxxVector<u8>>,
            is_sensitive_out: Pin<&mut bool>,
            mut err: Pin<&mut *mut FastlyError>,
        ) -> bool;
        fn get_header_all(
            &self,
            name: &CxxString,
            out: Pin<&mut *mut HeaderValuesIter>,
            mut err: Pin<&mut *mut FastlyError>,
        );
        fn get_headers(&self, out: Pin<&mut *mut HeadersIter>);
        fn get_header_names(&self, out: Pin<&mut *mut HeaderNamesIter>);
        fn get_original_header_names(&self, out: Pin<&mut *mut OriginalHeaderNamesIter>) -> bool;
        fn get_original_header_count(&self, mut out: Pin<&mut u32>) -> bool;
        fn set_header(
            &mut self,
            name: &CxxString,
            value: &CxxString,
            mut err: Pin<&mut *mut FastlyError>,
        );
        fn append_header(
            &mut self,
            name: &CxxString,
            value: &CxxString,
            mut err: Pin<&mut *mut FastlyError>,
        );
        fn remove_header(
            &mut self,
            name: &CxxString,
            out: Pin<&mut CxxString>,
            mut err: Pin<&mut *mut FastlyError>,
        ) -> bool;
        fn get_method(&self) -> Method;
        fn set_method(&mut self, method: Method);
        fn get_url(&self, mut out: Pin<&mut CxxString>);
        fn set_url(&mut self, url: &CxxString, mut err: Pin<&mut *mut FastlyError>);
        fn get_path(&self, mut out: Pin<&mut CxxString>);
        fn set_path(&mut self, path: &CxxString, mut err: Pin<&mut *mut FastlyError>);
        fn get_query_string(&self, mut out: Pin<&mut CxxString>) -> bool;
        fn get_query_parameter(&self, param: &CxxString, mut out: Pin<&mut CxxString>) -> bool;
        fn set_query_string(&mut self, qs: &CxxString, err: Pin<&mut *mut FastlyError>);
        fn remove_query(&mut self);
        fn get_client_ddos_detected(&self, mut out: Pin<&mut bool>) -> bool;
        fn get_client_ip_addr(&self, buf: Pin<&mut CxxString>) -> bool;
        fn get_server_ip_addr(&self, buf: Pin<&mut CxxString>) -> bool;
        fn set_pass(&mut self, pass: bool);
        fn set_ttl(&mut self, ttl: u32);
        fn set_stale_while_revalidate(&mut self, swr: u32);
        fn set_pci(&mut self, pci: bool);
        fn set_surrogate_key(&mut self, sk: &CxxString, mut err: Pin<&mut *mut FastlyError>);
        fn set_auto_decompress_gzip(&mut self, gzip: bool);
        fn fastly_key_is_valid(&self) -> bool;
        fn set_cache_key(&mut self, key: &CxxVector<u8>);
        fn is_cacheable(&mut self) -> bool;
        fn get_version(&self) -> Version;
        fn set_version(&mut self, version: Version);
    }

    #[namespace = "fastly::sys::http::request"]
    extern "Rust" {
        type PollResult;
        fn is_response(&self) -> bool;
        fn is_pending(&self) -> bool;
        fn m_http_request_poll_result_into_pending(result: Box<PollResult>) -> Box<PendingRequest>;
        fn m_http_request_poll_result_into_response(result: Box<PollResult>) -> Box<Response>;
        fn m_http_request_poll_result_into_error(result: Box<PollResult>) -> Box<FastlyError>;
    }

    #[namespace = "fastly::sys::http::request"]
    extern "Rust" {
        type PendingRequest;
        fn m_http_request_pending_request_poll(req: Box<PendingRequest>) -> Box<PollResult>;
        fn m_http_request_pending_request_wait(
            req: Box<PendingRequest>,
            out: Pin<&mut *mut Response>,
            err: Pin<&mut *mut FastlyError>,
        );
        fn cloned_sent_req(&self) -> Box<Request>;
    }

    #[namespace = "fastly::sys::http::request"]
    extern "Rust" {
        type BoxPendingRequest;
        fn f_http_push_box_pending_request_into_vec(
            vec: &mut Vec<BoxPendingRequest>,
            bx: Box<PendingRequest>,
        );
        fn extract_req(&mut self) -> Box<PendingRequest>;
    }

    #[namespace = "fastly::sys::http::request"]
    extern "Rust" {
        type AsyncStreamRes;
        fn take_body(&mut self) -> Box<StreamingBody>;
        fn take_req(&mut self) -> Box<PendingRequest>;
    }

    #[namespace = "fastly::sys::http::request"]
    extern "Rust" {
        fn f_http_request_select(
            reqs: Vec<BoxPendingRequest>,
            out: Pin<&mut *mut Response>,
            others: &mut Vec<BoxPendingRequest>,
            err: Pin<&mut *mut FastlyError>,
        );
    }

    #[namespace = "fastly::sys::http"]
    extern "Rust" {
        fn f_http_status_code_canonical_reason(
            code: u16,
            string: Pin<&mut CxxString>,
            err: Pin<&mut *mut FastlyError>,
        ) -> bool;
    }

    #[namespace = "fastly::sys::http"]
    extern "Rust" {
        type Response;
        fn m_static_http_response_new() -> Box<Response>;
        fn is_from_backend(&self) -> bool;
        fn clone_without_body(&self) -> Box<Response>;
        fn clone_with_body(&mut self) -> Box<Response>;
        fn m_static_http_response_from_body(body: Box<Body>) -> Box<Response>;
        fn m_static_http_response_from_status(status: u16) -> Box<Response>;
        fn m_static_http_response_see_other(destination: &CxxString) -> Box<Response>;
        fn m_static_http_response_redirect(destination: &CxxString) -> Box<Response>;
        fn m_static_http_response_temporary_redirect(destination: &CxxString) -> Box<Response>;
        fn has_body(&self) -> bool;
        fn set_body(&mut self, body: Box<Body>);
        fn take_body(&mut self) -> Box<Body>;
        fn append_body(&mut self, other: Box<Body>);
        fn m_http_response_into_body(response: Box<Response>) -> Box<Body>;
        fn set_body_text_plain(&mut self, body: &CxxString, mut err: Pin<&mut *mut FastlyError>);
        fn set_body_text_html(&mut self, body: &CxxString, mut err: Pin<&mut *mut FastlyError>);
        fn set_body_octet_stream(&mut self, body: &CxxVector<u8>);
        fn get_content_type(&self, mut out: Pin<&mut CxxString>) -> bool;
        fn set_content_type(&mut self, mime: &CxxString);
        fn get_content_length(&self, mut out: Pin<&mut usize>) -> bool;
        fn contains_header(&self, name: &CxxString, mut err: Pin<&mut *mut FastlyError>) -> bool;
        fn get_header(
            &self,
            name: &CxxString,
            value_out: Pin<&mut CxxVector<u8>>,
            is_sensitive_out: Pin<&mut bool>,
            mut err: Pin<&mut *mut FastlyError>,
        ) -> bool;
        fn get_header_all(
            &self,
            name: &CxxString,
            out: Pin<&mut *mut HeaderValuesIter>,
            mut err: Pin<&mut *mut FastlyError>,
        );
        fn get_headers(&self, out: Pin<&mut *mut HeadersIter>);
        fn get_header_names(&self, out: Pin<&mut *mut HeaderNamesIter>);
        fn set_header(
            &mut self,
            name: &CxxString,
            value: &CxxString,
            mut err: Pin<&mut *mut FastlyError>,
        );
        fn append_header(
            &mut self,
            name: &CxxString,
            value: &CxxString,
            mut err: Pin<&mut *mut FastlyError>,
        );
        fn remove_header(
            &mut self,
            name: &CxxString,
            out: Pin<&mut CxxString>,
            mut err: Pin<&mut *mut FastlyError>,
        ) -> bool;
        fn set_status(&mut self, status: u16);
        fn get_backend_name(&self, out: Pin<&mut CxxString>) -> bool;
        fn get_backend(&self) -> *mut Backend;
        fn get_backend_addr(&self, out: Pin<&mut CxxString>) -> bool;
        fn take_backend_request(&mut self) -> *mut Request;
        fn m_http_response_send_to_client(response: Box<Response>);
        fn m_http_response_stream_to_client(response: Box<Response>) -> Box<StreamingBody>;
        fn get_ttl(&self, mut out: Pin<&mut u32>) -> bool;
        fn get_age(&self, mut out: Pin<&mut u32>) -> bool;
        fn get_stale_while_revalidate(&self, mut out: Pin<&mut u32>) -> bool;
        fn get_version(&self) -> Version;
        fn set_version(&mut self, version: Version);
    }

    #[namespace = "fastly::sys::http"]
    extern "Rust" {
        type Body;
        fn m_static_http_body_new() -> Box<Body>;
        fn append(&mut self, other: Box<Body>);
        fn append_trailer(
            &mut self,
            name: &CxxString,
            value: &CxxString,
            err: Pin<&mut *mut FastlyError>,
        );
        fn read(&mut self, buf: &mut [u8], err: Pin<&mut *mut FastlyError>) -> usize;
        fn write(&mut self, bytes: &[u8], err: Pin<&mut *mut FastlyError>) -> usize;
    }

    #[namespace = "fastly::sys::http"]
    extern "Rust" {
        type StreamingBody;
        fn m_http_streaming_body_finish(body: Box<StreamingBody>, err: Pin<&mut *mut FastlyError>);
        fn append(&mut self, other: Box<Body>);
        fn append_trailer(
            &mut self,
            name: &CxxString,
            value: &CxxString,
            err: Pin<&mut *mut FastlyError>,
        );
        fn write(&mut self, bytes: &[u8], err: Pin<&mut *mut FastlyError>) -> usize;
    }

    #[namespace = "fastly::sys::http::purge"]
    extern "Rust" {
        fn f_http_purge_purge_surrogate_key(
            surrogate_key: &CxxString,
            err: Pin<&mut *mut FastlyError>,
        );
        fn f_http_purge_soft_purge_surrogate_key(
            surrogate_key: &CxxString,
            err: Pin<&mut *mut FastlyError>,
        );
    }

    #[namespace = "fastly::sys::device_detection"]
    extern "Rust" {
        type Device;
        fn f_device_detection_lookup(
            user_agent: &CxxString,
            mut out: Pin<&mut *mut Device>,
            mut err: Pin<&mut *mut FastlyError>,
        );
        fn device_name(&self, out: Pin<&mut CxxString>) -> bool;
        fn brand(&self, out: Pin<&mut CxxString>) -> bool;
        fn model(&self, out: Pin<&mut CxxString>) -> bool;
        fn hwtype(&self, out: Pin<&mut CxxString>) -> bool;
        fn is_ereader(&self) -> *const bool;
        fn is_gameconsole(&self) -> *const bool;
        fn is_mediaplayer(&self) -> *const bool;
        fn is_mobile(&self) -> *const bool;
        fn is_smarttv(&self) -> *const bool;
        fn is_tablet(&self) -> *const bool;
        fn is_tvplayer(&self) -> *const bool;
        fn is_desktop(&self) -> *const bool;
        fn is_touchscreen(&self) -> *const bool;
        // Get it to generate the appropriate symbols like ::drop() etc.
        fn f_device_detection_force_symbols(dev: Box<Device>) -> Box<Device>;
    }

    #[namespace = "fastly::sys::config_store"]
    extern "Rust" {
        type ConfigStore;
        fn m_static_config_store_config_store_open(
            name: &CxxString,
            mut out: Pin<&mut *mut ConfigStore>,
            mut err: Pin<&mut *mut FastlyError>,
        );
        fn get(
            &self,
            key: &CxxString,
            mut out: Pin<&mut CxxString>,
            mut err: Pin<&mut *mut FastlyError>,
        ) -> bool;
        fn contains(&self, key: &CxxString, mut err: Pin<&mut *mut FastlyError>) -> bool;
        fn f_config_store_config_store_force_symbols(x: Box<ConfigStore>) -> Box<ConfigStore>;
    }

    #[namespace = "fastly::sys::secret_store"]
    extern "Rust" {
        type Secret;
        fn plaintext(&self, out: Pin<&mut CxxString>);
        fn m_static_secret_store_secret_from_bytes(
            bytes: &CxxVector<u8>,
            out: Pin<&mut *mut Secret>,
            err: Pin<&mut *mut FastlyError>,
        );
        fn f_secret_store_secret_force_symbols(x: Box<Secret>) -> Box<Secret>;
    }

    #[namespace = "fastly::sys::secret_store"]
    extern "Rust" {
        type SecretStore;
        fn m_static_secret_store_secret_store_open(
            name: &CxxString,
            mut out: Pin<&mut *mut SecretStore>,
            mut err: Pin<&mut *mut FastlyError>,
        );
        fn get(
            &self,
            key: &CxxString,
            mut out: Pin<&mut *mut Secret>,
            mut err: Pin<&mut *mut FastlyError>,
        );
        fn contains(&self, key: &CxxString, mut err: Pin<&mut *mut FastlyError>) -> bool;
        fn f_secret_store_secret_store_force_symbols(x: Box<SecretStore>) -> Box<SecretStore>;
    }

    #[namespace = "fastly::sys::geo"]
    extern "Rust" {
        type UtcOffset;
        fn whole_hours(&self) -> i8;
        fn whole_minutes(&self) -> i16;
        fn minutes_past_hour(&self) -> i8;
        fn whole_seconds(&self) -> i32;
        fn seconds_past_minute(&self) -> i8;
        fn is_utc(&self) -> bool;
        fn is_positive(&self) -> bool;
        fn is_negative(&self) -> bool;
        fn f_geo_utc_offset_force_symbols(x: Box<UtcOffset>) -> Box<UtcOffset>;
    }

    #[namespace = "fastly::sys::geo"]
    extern "Rust" {
        type Geo;
        fn f_geo_geo_lookup(
            ip: &CxxString,
            mut out: Pin<&mut *mut Geo>,
            mut err: Pin<&mut *mut FastlyError>,
        );
        fn as_name(&self, out: Pin<&mut CxxString>);
        fn as_number(&self) -> u32;
        fn area_code(&self) -> u16;
        fn city(&self, out: Pin<&mut CxxString>);
        fn conn_speed(&self) -> ConnSpeed;
        fn conn_type(&self) -> ConnType;
        fn continent(&self) -> Continent;
        fn country_code(&self, out: Pin<&mut CxxString>);
        fn country_code3(&self, out: Pin<&mut CxxString>);
        fn country_name(&self, out: Pin<&mut CxxString>);
        fn latitude(&self) -> f64;
        fn longitude(&self) -> f64;
        fn metro_code(&self) -> i64;
        fn postal_code(&self, out: Pin<&mut CxxString>);
        fn proxy_description(&self) -> ProxyDescription;
        fn proxy_type(&self) -> ProxyType;
        fn region(&self, out: Pin<&mut CxxString>) -> bool;
        fn utc_offset(&self) -> *mut UtcOffset;
        fn f_geo_geo_force_symbols(x: Box<Geo>) -> Box<Geo>;
    }

    #[namespace = "fastly::sys::log"]
    extern "Rust" {
        type Endpoint;
        fn name(&self, out: Pin<&mut CxxString>);
        fn m_static_log_endpoint_try_from_name(
            name: &CxxString,
            out: Pin<&mut *mut Endpoint>,
            err: Pin<&mut *mut FastlyError>,
        );
        fn f_log_log(level: LogLevel, msg: &CxxString);
        fn f_log_log_to(target: &CxxString, level: LogLevel, msg: &CxxString);
        fn f_log_max_level() -> LogLevelFilter;
        fn f_log_set_max_level(level: LogLevelFilter);
        fn f_log_init_simple(endpoint: Box<Endpoint>, level: LogLevelFilter);
    }
    #[namespace = "fastly::sys::log"]
    extern "Rust" {
        type LoggerBuilder;
        fn m_static_log_logger_builder_new() -> Box<LoggerBuilder>;
        fn endpoint(&mut self, endpoint: Box<Endpoint>);
        fn endpoint_level(&mut self, endpoint: Box<Endpoint>, level: LogLevelFilter);
        fn default_endpoint(&mut self, endpoint: Box<Endpoint>);
        fn default_level_endpoint(&mut self, endpoint: Box<Endpoint>, level: LogLevel);
        fn max_level(&mut self, level: LogLevelFilter);
        fn echo_stdout(&mut self, enabled: bool);
        fn echo_stderr(&mut self, enabled: bool);
        fn init(&mut self);
    }

    #[namespace = "fastly::sys::security"]
    #[derive(Copy, Clone, Debug)]
    #[repr(usize)]
    pub enum InspectErrorCode {
        DeserializeError,
        InvalidConfig,
        RequestError,
        BufferSizeError,
        Unexpected,
    }

    #[namespace = "fastly::sys::security"]
    #[derive(Copy, Clone, Debug)]
    pub enum InspectVerdict {
        /// Security indicated that this request is allowed.
        Allow,

        /// Security indicated that this request should be blocked.
        Block,

        /// Security indicated that this service is not authorized to inspect a request.
        Unauthorized,

        /// Security returned an unrecognized verdict.
        ///
        /// This variant exists to allow for the possibility of future
        /// additions, but should normally not be seen.
        Other,
    }

    #[namespace = "fastly::sys::security"]
    extern "Rust" {
        type InspectResponse;
        fn status(&self) -> i16;
        fn is_redirect(&self) -> bool;
        fn decision_ms(&self) -> u32;
        fn redirect_url(&self, out: Pin<&mut CxxString>) -> bool;
        fn tags(&self) -> Vec<String>;
        fn verdict(&self) -> InspectVerdict;
        fn unrecognized_verdict_info(&self, out: Pin<&mut CxxString>) -> bool;
    }

    #[namespace = "fastly::sys::security"]
    extern "Rust" {
        fn m_security_inspect_response_into_redirect(
            response: Box<InspectResponse>,
            mut out: Pin<&mut *mut Response>,
        ) -> bool;
        unsafe fn f_security_lookup(
            request: &Request,
            client_ip: *const CxxString,
            corp: *const CxxString,
            workspace: *const CxxString,
            buffer_size: *const usize,
            mut out: Pin<&mut *mut InspectResponse>,
            mut err: Pin<&mut *mut InspectError>,
        );
    }

    #[namespace = "fastly::sys::security"]
    extern "Rust" {
        type InspectError;
        fn error_msg(&self, mut out: Pin<&mut CxxString>);
        fn error_code(&self) -> InspectErrorCode;
        fn required_buffer_size(&self, out: Pin<&mut usize>) -> bool;
        fn f_security_inspect_error_force_symbols(x: Box<InspectError>) -> Box<InspectError>;
    }

    #[namespace = "fastly::sys::kv_store"]
    #[derive(Copy, Clone, Debug)]
    #[repr(usize)]
    pub enum KVStoreErrorCode {
        InvalidKey,
        InvalidStoreHandle,
        InvalidStoreOptions,
        ItemBadRequest,
        ItemNotFound,
        ItemPreconditionFailed,
        ItemPayloadTooLarge,
        StoreNotFound,
        TooManyRequests,
        Unexpected,
    }

    #[namespace = "fastly::sys::kv_store"]
    extern "Rust" {
        type KVStoreError;
        fn error_msg(&self, mut out: Pin<&mut CxxString>);
        fn error_code(&self) -> KVStoreErrorCode;
        fn f_kv_store_kv_store_error_force_symbols(x: Box<KVStoreError>) -> Box<KVStoreError>;
    }

    #[namespace = "fastly::sys::kv_store"]
    #[derive(Copy, Clone, Debug)]
    pub enum InsertMode {
        Overwrite,
        Add,
        Append,
        Prepend,
    }

    #[namespace = "fastly::sys::kv_store"]
    #[derive(Copy, Clone, Debug)]
    pub enum ListModeType {
        Strong,
        Eventual,
        Other,
    }

    #[namespace = "fastly::sys::kv_store"]
    extern "Rust" {
        type InsertBuilder<'a>;
        fn m_kv_store_insert_builder_mode(
            mut builder: Box<InsertBuilder>,
            mode: InsertMode,
        ) -> Box<InsertBuilder>;
        fn m_kv_store_insert_builder_background_fetch(
            mut builder: Box<InsertBuilder>,
        ) -> Box<InsertBuilder>;
        fn m_kv_store_insert_builder_if_generation_match(
            mut builder: Box<InsertBuilder>,
            generation: u64,
        ) -> Box<InsertBuilder>;
        unsafe fn m_kv_store_insert_builder_metadata<'a>(
            mut builder: Box<InsertBuilder<'a>>,
            data: &CxxString,
        ) -> Box<InsertBuilder<'a>>;
        fn m_kv_store_insert_builder_time_to_live(
            mut builder: Box<InsertBuilder>,
            ttl: u32,
        ) -> Box<InsertBuilder>;
        fn m_kv_store_insert_builder_execute(
            builder: Box<InsertBuilder>,
            key: &CxxString,
            body: Box<Body>,
            mut err: Pin<&mut *mut KVStoreError>,
        );
        fn m_kv_store_insert_builder_execute_async(
            builder: Box<InsertBuilder>,
            key: &CxxString,
            body: Box<Body>,
            mut out: Pin<&mut u32>,
            mut err: Pin<&mut *mut KVStoreError>,
        );
    }

    #[namespace = "fastly::sys::kv_store"]
    extern "Rust" {
        type LookupResponse;
        fn take_body(&mut self) -> Box<Body>;
        fn try_take_body(&mut self, mut out: Pin<&mut *mut Body>) -> bool;
        fn take_body_bytes(&mut self, mut out: Pin<&mut CxxVector<u8>>);
        fn metadata(&self, mut out: Pin<&mut CxxVector<u8>>) -> bool;
        fn current_generation(&self) -> u64;
        fn f_kv_store_lookup_response_force_symbols(x: Box<LookupResponse>) -> Box<LookupResponse>;
    }

    #[namespace = "fastly::sys::kv_store"]
    extern "Rust" {
        type LookupBuilder<'a>;
        fn execute(
            &self,
            key: &str,
            mut out: Pin<&mut *mut LookupResponse>,
            mut err: Pin<&mut *mut KVStoreError>,
        );
        fn execute_async(
            &self,
            key: &str,
            mut out: Pin<&mut u32>,
            mut err: Pin<&mut *mut KVStoreError>,
        );
    }

    #[namespace = "fastly::sys::kv_store"]
    extern "Rust" {
        type EraseBuilder<'a>;
        fn execute(&self, key: &str, mut err: Pin<&mut *mut KVStoreError>);
        fn execute_async(
            &self,
            key: &str,
            mut out: Pin<&mut u32>,
            mut err: Pin<&mut *mut KVStoreError>,
        );
    }

    #[namespace = "fastly::sys::kv_store"]
    extern "Rust" {
        type ListPage;
        fn keys(&self) -> &[String];
        fn next_cursor(&self, out: Pin<&mut CxxString>) -> bool;
        fn prefix(&self, out: Pin<&mut CxxString>) -> bool;
        fn limit(&self) -> u32;
        fn mode(&self) -> Box<ListMode>;
        fn m_kv_store_list_page_into_keys(page: Box<ListPage>) -> Vec<String>;
    }

    #[namespace = "fastly::sys::kv_store"]
    extern "Rust" {
        type ListBuilder<'a>;
        fn execute_async(&self, mut out: Pin<&mut u32>, mut err: Pin<&mut *mut KVStoreError>);
        fn m_kv_store_list_builder_execute(
            builder: Box<ListBuilder>,
            mut out: Pin<&mut *mut ListPage>,
            mut err: Pin<&mut *mut KVStoreError>,
        );
        fn m_kv_store_list_builder_eventual_consistency(
            mut builder: Box<ListBuilder>,
        ) -> Box<ListBuilder>;
        unsafe fn m_kv_store_list_builder_cursor<'a>(
            mut builder: Box<ListBuilder<'a>>,
            cursor: &str,
        ) -> Box<ListBuilder<'a>>;
        fn m_kv_store_list_builder_limit(
            mut builder: Box<ListBuilder>,
            limit: u32,
        ) -> Box<ListBuilder>;
        unsafe fn m_kv_store_list_builder_prefix<'a>(
            mut builder: Box<ListBuilder<'a>>,
            prefix: &str,
        ) -> Box<ListBuilder<'a>>;
        fn m_kv_store_list_builder_iter(builder: Box<ListBuilder<'_>>) -> Box<ListResponse<'_>>;
    }

    #[namespace = "fastly::sys::kv_store"]
    extern "Rust" {
        type KVStore;
        fn lookup(
            &self,
            key: &str,
            mut out: Pin<&mut *mut LookupResponse>,
            mut err: Pin<&mut *mut KVStoreError>,
        );
        unsafe fn build_lookup(&self) -> Box<LookupBuilder<'_>>;
        fn pending_lookup_wait(
            &self,
            pending_request_handle: u32,
            mut out: Pin<&mut *mut LookupResponse>,
            mut err: Pin<&mut *mut KVStoreError>,
        );
        fn insert(&self, key: &str, value: Box<Body>, mut err: Pin<&mut *mut KVStoreError>);
        unsafe fn build_insert(&self) -> Box<InsertBuilder<'_>>;
        fn pending_insert_wait(
            &self,
            pending_insert_handle: u32,
            mut err: Pin<&mut *mut KVStoreError>,
        );
        fn erase(&self, key: &str, mut err: Pin<&mut *mut KVStoreError>);
        fn build_erase(&self) -> Box<EraseBuilder<'_>>;
        fn pending_erase_wait(
            &self,
            pending_delete_handle: u32,
            mut err: Pin<&mut *mut KVStoreError>,
        );
        fn list(&self, mut out: Pin<&mut *mut ListPage>, mut err: Pin<&mut *mut KVStoreError>);
        fn build_list(&self) -> Box<ListBuilder<'_>>;
        fn pending_list_wait(
            &self,
            pending_request_handle: u32,
            mut out: Pin<&mut *mut ListPage>,
            mut err: Pin<&mut *mut KVStoreError>,
        );
        fn m_static_kv_store_kv_store_open(
            name: &str,
            mut out: Pin<&mut *mut KVStore>,
            mut err: Pin<&mut *mut KVStoreError>,
        ) -> bool;
        fn f_kv_store_kv_store_force_symbols(kv_store: Box<KVStore>) -> Box<KVStore>;
    }

    #[namespace = "fastly::sys::kv_store"]
    extern "Rust" {
        type ListMode;
        fn code(&self) -> ListModeType;
        fn other_string(&self, out: Pin<&mut CxxString>) -> bool;
    }

    #[namespace = "fastly::sys::kv_store"]
    extern "Rust" {
        type ListResponse<'a>;
        fn next(
            &mut self,
            mut out: Pin<&mut *mut ListPage>,
            mut err: Pin<&mut *mut KVStoreError>,
        ) -> bool;
    }

    // These tag types are empty types used to communicate callbacks from C++ to Rust.
    // They will be cast back to the real callback types on the C++ side.
    #[namespace = "fastly::detail::rust_bridge_tags::esi"]
    unsafe extern "C++" {
        include!("fastly/detail/rust_bridge_tags.h");
        type DispatchFragmentRequestFnTag;
        type ProcessFragmentResponseFnTag;
    }

    #[namespace = "fastly::sys::esi"]
    extern "Rust" {
        type Processor;
        pub unsafe fn m_esi_processor_process_response(
            processor: Box<Processor>,
            src_document: &mut Response,
            // SAFETY: this parameter models an Option<Box<Response>>, but CXX does not
            // support this type directly. Care must be taken to take ownership of the pointer
            // and free it if it is non-null.
            client_response_metadata: *mut Response,
            dispatch_fragment_request: *const DispatchFragmentRequestFnTag,
            process_fragment_response: *const ProcessFragmentResponseFnTag,
            mut err: Pin<&mut *mut FastlyError>,
        ) -> bool;
        pub unsafe fn m_esi_processor_process_document(
            processor: Box<Processor>,
            src_document: &CxxString,
            dispatch_fragment_request: *const DispatchFragmentRequestFnTag,
            process_fragment_response: *const ProcessFragmentResponseFnTag,
            out: Pin<&mut CxxString>,
            mut err: Pin<&mut *mut FastlyError>,
        ) -> bool;
        pub unsafe fn m_static_esi_processor_new(
            // SAFETY: this parameter models an Option<Box<Request>>, but CXX does not
            // support this type directly. Care must be taken to take ownership of the pointer
            // and free it if it is non-null.
            original_request_metadata: *mut Request,
            namespc: &CxxString,
            is_escaped_content: bool,
        ) -> Box<Processor>;
    }
}

// Some types (notably callback functions) are not supported by CXX at all, so we
// define manual FFI bindings for them here.
mod manual_ffi {
    use crate::ffi::{
        DispatchFragmentRequestFnResult, DispatchFragmentRequestFnTag, ProcessFragmentResponseFnTag,
    };

    // We never rely on the layout of Rust types passed to these functions,
    // so we can ignore the improper_ctypes warning.
    #[allow(improper_ctypes)]
    unsafe extern "C" {
        // The link names here must exactly match the names of the extern "C" functions defined on the C++ side
        #[link_name = "fastly$esi$manualbridge$DispatchFragmentRequestFn$call"]
        pub(crate) fn fastly_esi_manualbridge_DispatchFragmentRequestFn_call(
            func: *const DispatchFragmentRequestFnTag,
            req: *mut crate::Request,
            out_pending: &mut *mut crate::PendingRequest,
            out_complete: &mut *mut crate::Response,
        ) -> DispatchFragmentRequestFnResult;

        #[link_name = "fastly$esi$manualbridge$ProcessFragmentResponseFn$call"]
        pub(crate) fn fastly_esi_manualbridge_ProcessFragmentResponseFn_call(
            func: *const ProcessFragmentResponseFnTag,
            request: *mut crate::Request,
            response: *mut crate::Response,
            out_response: &mut *mut crate::Response,
        ) -> bool;
    }
}
