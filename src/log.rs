use std::pin::Pin;

use cxx::CxxString;

use crate::{
    error::FastlyError,
    ffi::{LogLevel, LogLevelFilter},
    try_fe,
};

pub struct Endpoint(pub(crate) fastly::log::Endpoint);

impl Endpoint {
    pub fn name(&self, out: Pin<&mut CxxString>) {
        out.push_str(self.0.name())
    }
}

impl From<LogLevel> for log::Level {
    fn from(value: LogLevel) -> Self {
        match value {
            LogLevel::Error => log::Level::Error,
            LogLevel::Warn => log::Level::Warn,
            LogLevel::Info => log::Level::Info,
            LogLevel::Debug => log::Level::Debug,
            LogLevel::Trace => log::Level::Trace,
            _ => log::Level::Trace,
        }
    }
}

impl From<LogLevelFilter> for log::LevelFilter {
    fn from(value: LogLevelFilter) -> Self {
        match value {
            LogLevelFilter::Off => log::LevelFilter::Off,
            LogLevelFilter::Error => log::LevelFilter::Error,
            LogLevelFilter::Warn => log::LevelFilter::Warn,
            LogLevelFilter::Info => log::LevelFilter::Info,
            LogLevelFilter::Debug => log::LevelFilter::Debug,
            LogLevelFilter::Trace => log::LevelFilter::Trace,
            _ => log::LevelFilter::Off,
        }
    }
}

impl From<log::LevelFilter> for LogLevelFilter {
    fn from(value: log::LevelFilter) -> Self {
        match value {
            log::LevelFilter::Off => LogLevelFilter::Off,
            log::LevelFilter::Error => LogLevelFilter::Error,
            log::LevelFilter::Warn => LogLevelFilter::Warn,
            log::LevelFilter::Info => LogLevelFilter::Info,
            log::LevelFilter::Debug => LogLevelFilter::Debug,
            log::LevelFilter::Trace => LogLevelFilter::Trace,
        }
    }
}

pub fn m_static_log_endpoint_try_from_name(
    name: &CxxString,
    mut out: Pin<&mut *mut Endpoint>,
    mut err: Pin<&mut *mut FastlyError>,
) {
    out.set(try_fe!(
        err,
        fastly::log::Endpoint::try_from_name(try_fe!(err, name.to_str()))
            .map(Endpoint)
            .map(Box::new)
            .map(Box::into_raw)
    ))
}

pub fn f_log_log(ty: LogLevel, msg: &CxxString) {
    log::log!(ty.into(), "{}", msg.to_string_lossy(),)
}

pub fn f_log_log_to(target: &CxxString, ty: LogLevel, msg: &CxxString) {
    log::log!(
        target: target.to_str().expect("invalid string for target logger endpoint name"),
        ty.into(),
        "{}",
        msg.to_string_lossy(),
    )
}

pub fn f_log_max_level() -> LogLevelFilter {
    log::max_level().into()
}

pub fn f_log_set_max_level(level: LogLevelFilter) {
    log::set_max_level(level.into());
}

pub struct LoggerBuilder(pub(crate) log_fastly::Builder);

pub fn f_log_init_simple(endpoint: Box<Endpoint>, level: LogLevelFilter) {
    log_fastly::init_simple(endpoint.0, level.into());
}

pub fn m_static_log_logger_builder_new() -> Box<LoggerBuilder> {
    Box::new(LoggerBuilder(log_fastly::Builder::new()))
}

impl LoggerBuilder {
    pub fn endpoint(&mut self, endpoint: Box<Endpoint>) {
        self.0.endpoint(endpoint.0);
    }
    pub fn endpoint_level(&mut self, endpoint: Box<Endpoint>, level: LogLevelFilter) {
        self.0.endpoint_level(endpoint.0, level.into());
    }
    pub fn default_endpoint(&mut self, endpoint: Box<Endpoint>) {
        self.0.default_endpoint(endpoint.0);
    }
    pub fn default_level_endpoint(&mut self, endpoint: Box<Endpoint>, level: LogLevel) {
        self.0.default_level_endpoint(endpoint.0, level.into());
    }
    pub fn max_level(&mut self, level: LogLevelFilter) {
        self.0.max_level(level.into());
    }
    pub fn echo_stdout(&mut self, enabled: bool) {
        self.0.echo_stdout(enabled);
    }
    pub fn echo_stderr(&mut self, enabled: bool) {
        self.0.echo_stderr(enabled);
    }
    pub fn init(&mut self) {
        self.0.init();
    }
}
