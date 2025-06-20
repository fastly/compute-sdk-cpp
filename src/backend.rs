use std::{num::NonZero, time::Duration};

use cxx::CxxString;

pub struct Backend(pub(crate) fastly::backend::Backend);

pub fn m_static_backend_backend_from_name(name: &CxxString) -> Box<Backend> {
    Box::new(Backend(
        fastly::backend::Backend::from_name(name.to_string_lossy().as_ref())
            .expect("Failed to create backend"),
    ))
}

pub fn m_static_backend_backend_builder(
    name: &CxxString,
    target: &CxxString,
) -> Box<BackendBuilder> {
    Box::new(BackendBuilder(fastly::backend::Backend::builder(
        name.to_string_lossy().as_ref(),
        target.to_string_lossy().as_ref(),
    )))
}

pub fn m_backend_backend_into_string(backend: Box<Backend>) -> String {
    backend.0.into_string()
}

impl Backend {
    pub fn name(&self) -> &str {
        self.0.name()
    }

    pub fn exists(&self) -> bool {
        self.0.exists()
    }

    pub fn is_dynamic(&self) -> bool {
        self.0.is_dynamic()
    }

    pub fn get_host(&self) -> String {
        self.0.get_host()
    }

    // TODO
    // pub fn get_host_override(&self) {
    // }

    pub fn get_port(&self) -> u16 {
        self.0.get_port()
    }

    pub fn get_connect_timeout(&self) -> u32 {
        ensure_u32(self.0.get_connect_timeout().as_millis())
    }

    pub fn get_first_byte_timeout(&self) -> u32 {
        ensure_u32(self.0.get_first_byte_timeout().as_millis())
    }

    pub fn get_between_bytes_timeout(&self) -> u32 {
        ensure_u32(self.0.get_between_bytes_timeout().as_millis())
    }

    pub fn get_http_keepalive_time(&self) -> u32 {
        ensure_u32(self.0.get_http_keepalive_time().as_millis())
    }

    pub fn get_tcp_keepalive_enable(&self) -> bool {
        self.0.get_tcp_keepalive_enable()
    }

    pub fn get_tcp_keepalive_interval(&self) -> u32 {
        ensure_u32(self.0.get_tcp_keepalive_interval().as_millis())
    }

    pub fn get_tcp_keepalive_probes(&self) -> u32 {
        self.0.get_tcp_keepalive_probes()
    }

    pub fn get_tcp_keepalive_time(&self) -> u32 {
        ensure_u32(self.0.get_tcp_keepalive_time().as_millis())
    }

    pub fn is_ssl(&self) -> bool {
        self.0.is_ssl()
    }
}

fn ensure_u32(num: u128) -> u32 {
    if num > std::u32::MAX as u128 {
        std::u32::MAX
    } else {
        num as u32
    }
}

pub struct BackendBuilder(pub(crate) fastly::backend::BackendBuilder);

pub fn m_static_backend_backend_builder_new(
    name: &CxxString,
    target: &CxxString,
) -> Box<BackendBuilder> {
    Box::new(BackendBuilder(fastly::backend::BackendBuilder::new(
        name.to_string_lossy().as_ref(),
        target.to_string_lossy().as_ref(),
    )))
}

pub fn m_backend_backend_builder_override_host(
    mut builder: Box<BackendBuilder>,
    name: &CxxString,
) -> Box<BackendBuilder> {
    builder.0 = builder.0.override_host(name.to_string_lossy().as_ref());
    builder
}

pub fn m_backend_backend_builder_connect_timeout(
    mut builder: Box<BackendBuilder>,
    timeout: u32,
) -> Box<BackendBuilder> {
    builder.0 = builder
        .0
        .connect_timeout(Duration::from_millis(timeout as u64));
    builder
}

pub fn m_backend_backend_builder_first_byte_timeout(
    mut builder: Box<BackendBuilder>,
    timeout: u32,
) -> Box<BackendBuilder> {
    builder.0 = builder
        .0
        .first_byte_timeout(Duration::from_millis(timeout as u64));
    builder
}

pub fn m_backend_backend_builder_between_bytes_timeout(
    mut builder: Box<BackendBuilder>,
    timeout: u32,
) -> Box<BackendBuilder> {
    builder.0 = builder
        .0
        .between_bytes_timeout(Duration::from_millis(timeout as u64));
    builder
}

pub fn m_backend_backend_builder_enable_ssl(
    mut builder: Box<BackendBuilder>,
) -> Box<BackendBuilder> {
    builder.0 = builder.0.enable_ssl();
    builder
}

pub fn m_backend_backend_builder_disable_ssl(
    mut builder: Box<BackendBuilder>,
) -> Box<BackendBuilder> {
    builder.0 = builder.0.disable_ssl();
    builder
}

// TODO
// pub fn m_backend_backend_builder_set_min_tls_version(mut builder: Box<BackendBuilder>) -> Box<BackendBuilder> {
//     builder.0 = builder.0.set_min_tls_version();
//     builder
// }

// pub fn m_backend_backend_builder_set_max_tls_version(mut builder: Box<BackendBuilder>) -> Box<BackendBuilder> {
//     builder.0 = builder.0.set_max_tls_version();
//     builder
// }

pub fn m_backend_backend_builder_check_certificate(
    mut builder: Box<BackendBuilder>,
    cert: &CxxString,
) -> Box<BackendBuilder> {
    builder.0 = builder.0.check_certificate(cert.to_string_lossy().as_ref());
    builder
}

pub fn m_backend_backend_builder_ca_certificate(
    mut builder: Box<BackendBuilder>,
    cert: &CxxString,
) -> Box<BackendBuilder> {
    builder.0 = builder.0.ca_certificate(cert.to_string_lossy().as_ref());
    builder
}

pub fn m_backend_backend_builder_tls_ciphers(
    mut builder: Box<BackendBuilder>,
    ciphers: &CxxString,
) -> Box<BackendBuilder> {
    builder.0 = builder.0.tls_ciphers(ciphers.to_string_lossy().as_ref());
    builder
}

pub fn m_backend_backend_builder_sni_hostname(
    mut builder: Box<BackendBuilder>,
    host: &CxxString,
) -> Box<BackendBuilder> {
    builder.0 = builder.0.sni_hostname(host.to_string_lossy().as_ref());
    builder
}

// TODO
// pub fn m_backend_backend_builder_provide_client_certificate(mut builder: Box<BackendBuilder>, pem_certificate: &CxxString, pem_key: Box<Secret>) -> Box<BackendBuilder> {
//     builder.0 = builder.0.provide_client_certificate(pem_certificate.to_string_lossy().as_ref(), (*pem_key).0);
//     builder
// }

pub fn m_backend_backend_builder_enable_pooling(
    mut builder: Box<BackendBuilder>,
    value: bool,
) -> Box<BackendBuilder> {
    builder.0 = builder.0.enable_pooling(value);
    builder
}

pub fn m_backend_backend_builder_http_keepalive_time(
    mut builder: Box<BackendBuilder>,
    timeout: u32,
) -> Box<BackendBuilder> {
    builder.0 = builder
        .0
        .http_keepalive_time(Duration::from_millis(timeout as u64));
    builder
}

pub fn m_backend_backend_builder_tcp_keepalive_enable(
    mut builder: Box<BackendBuilder>,
    value: bool,
) -> Box<BackendBuilder> {
    builder.0 = builder.0.tcp_keepalive_enable(value);
    builder
}

pub fn m_backend_backend_builder_tcp_keepalive_interval_secs(
    mut builder: Box<BackendBuilder>,
    value: u32,
) -> Box<BackendBuilder> {
    builder.0 = builder
        .0
        .tcp_keepalive_interval_secs(NonZero::new(value).unwrap());
    builder
}

pub fn m_backend_backend_builder_tcp_keepalive_probes(
    mut builder: Box<BackendBuilder>,
    value: u32,
) -> Box<BackendBuilder> {
    builder.0 = builder.0.tcp_keepalive_probes(NonZero::new(value).unwrap());
    builder
}

pub fn m_backend_backend_builder_tcp_keepalive_time_secs(
    mut builder: Box<BackendBuilder>,
    value: u32,
) -> Box<BackendBuilder> {
    builder.0 = builder
        .0
        .tcp_keepalive_time_secs(NonZero::new(value).unwrap());
    builder
}

pub fn m_backend_backend_builder_finish(builder: Box<BackendBuilder>) -> Box<Backend> {
    Box::new(Backend(
        builder.0.finish().expect("Failed to build Backend object"),
    ))
}
