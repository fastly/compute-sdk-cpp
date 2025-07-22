#![allow(clippy::boxed_local)]

use backend::*;
use config_store::*;
use device_detection::*;
use error::*;
use geo::*;
use http::{
    body::*, header::*, purge::*, request::request::*, request::*, response::*, status_code::*,
};
use secret_store::*;

mod backend;
mod config_store;
mod device_detection;
mod error;
mod geo;
mod http;
mod secret_store;

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
        fn next(&mut self) -> UniquePtr<CxxVector<u8>>;
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
            out: Pin<&mut CxxString>,
            mut err: Pin<&mut *mut FastlyError>,
        ) -> bool;
        fn get_header_all(
            &self,
            name: &CxxString,
            out: Pin<&mut *mut HeaderValuesIter>,
            mut err: Pin<&mut *mut FastlyError>,
        );
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
            out: Pin<&mut CxxString>,
            mut err: Pin<&mut *mut FastlyError>,
        ) -> bool;
        fn get_header_all(
            &self,
            name: &CxxString,
            out: Pin<&mut *mut HeaderValuesIter>,
            mut err: Pin<&mut *mut FastlyError>,
        );
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
        fn f_device_detection_noop(dev: Box<Device>) -> Box<Device>;
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
    }
}
