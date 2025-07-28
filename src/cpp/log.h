#ifndef FASTLY_SECRET_STORE_H
#define FASTLY_SECRET_STORE_H

#include "error.h"
#include "sdk-sys.h"
#include "util.h"
#include <format>
#include <string>
#include <string_view>

/// Logger implementation for Fastly Compute.
///
/// With this logger configured, the various log statements, like
/// `fastly::log::error()`, `fastly::log::debug()`, etc will send log messages
/// to your chosen [Real-Time Log Streaming][rtls] endpoints. You should
/// initialize the logger as soon as your program starts. Logging statements
/// will not do anything before initialization.
///
/// See the [Fastly
/// documentation](https://docs.fastly.com/en/guides/integrations#_logging-endpoints)
/// for more information about configuring logging endpoints for your service.
///
/// # Getting started
///
/// All you need to get started is your endpoint name and the level of log
/// messages you want to emit. For example, if you have an endpoint called
/// `my_endpoint`, and you only want to emit log messages at the
/// `LogLevel::Warn` or `LogLevel::Error` level, you can use
/// `fastly::log::init_simple()`:
///
/// ```cpp
/// fastly::log::init_simple("my_endpoint", fastly::log::LogLevelFilter::Warn);
/// fastly::log::warn("This will be written to my_endpoint...");
/// fastly::log::info("...but this won't");
/// ```
///
/// # Advanced configuration
///
/// For more precise control, including multiple endpoints and default endpoints
/// for different logging levels, use the `LoggerBuilder` interface. The first
/// example is equivalent to:
///
/// ```cpp
/// fastly::log::LoggerBuilder()
///     .max_level(fastly::log::LogLevelFilter::Warn)
///     .default_endpoint("my_endpoint")
///     .init();
/// ```
///
/// The `LoggerBuilder::max_level()` option sets the most verbose level of
/// logging that will be emitted. Logging statements above that level, like
/// `info()` in the first example, will do nothing.
///
/// **Note:** The default level is `LevelFilter::Off`, which emits no logging at
/// all. You'll want to change this for most configurations.
///
/// `LoggerBuilder::default_endpoint()` sets the endpoint used whenever a
/// plain logging statement is called (the ones not suffixed with `_to`) to
/// specify its endpoint. With the default endpoint set to `my_endpoint`, the
/// logging statements in the first example are equivalent to:
///
/// ```cpp
/// fastly::log::warn_to(
///   "my_endpoint",
///   "This will be written to my_endpoint..."
/// );
/// fastly::log::info_to("my_endpoint", "...but this won't");
/// ```
///
/// ## Use with Compute Log Tailing
///
/// [Compute Log Tailing][log-tailing] is helpful for getting debugging output
/// quickly from a Compute program under development by capturing output from
/// `stdout` or `stderr`. To configure logging to output to `stdout` or `stderr`
/// in addition to the specified log endpoint, enable echoing when building the
/// logger:
///
/// ```cpp
/// fastly::log::LoggerBuilder()
///     .max_level(fastly::log::LogLevelFilter::Warn)
///     .default_endpoint("my_endpoint")
///     .echo_stdout(true)
///     .init();
/// ```
///
/// [log-tailing]:
/// https://www.fastly.com/blog/introducing-compute-edge-log-tailing-for-better-observability-and-easier-debugging
///
/// ## Multiple endpoints
///
/// Setting an endpoint as the default will automatically register it for use
/// with the logger, but you can register additional endpoints with
/// `LoggerBuilder::endpoint()`:
///
/// ```cpp
/// fastly::log::LoggerBuilder()
///     .max_level(log::LogLevelFilter::Warn)
///     .default_endpoint("my_endpoint")
///     .endpoint("my_other_endpoint")
///     .init();
/// log::warn_to("my_endpoint", "This will be written to my_endpoint...");
/// log::warn_to(
///   "my_other_endpoint",
///   "...but this will be written to my_other_endpoint"
/// );
/// ```
///
/// ## Per-endpoint logging levels
///
/// You can also set a per-endpoint logging level, though levels higher than
/// `max_level` are always ignored:
///
/// ```cpp
/// fastly::log::LoggerBuilder()
///     .max_level(log::LogLevelFilter::Warn)
///     .default_endpoint("my_endpoint")
///     .endpoint_level("my_other_endpoint", fastly::log::LogLevelFilter::Trace)
///     .endpoint_level("error_only", fastly::log::LogLevelFilter::Error)
///     .init();
/// fastly::log::warn_to(
///   "my_other_endpoint",
///   "This will be written to my_other_endpoint..."
/// );
/// fastly::log::trace_to(
///   "my_other_endpoint",
///   "...but this won't, because max_level wins"
/// );
/// fastly::log::error_to(
///   "error_only",
///   "This will be written to error_only..."
/// );
/// fastly::log::warn_to(
///   "error_only",
///   "...but this won't, because the endpoint's level is lower"
/// );
/// ```
///
/// ## Per-level default endpoints
///
/// In the previous examples, the same endpoint is set as the default for all
/// logging levels. You can also specify default endpoints for individual levels
/// using `LoggerBuilder::default_level_endpoint()`. The defaults are combined
/// in order, so you can specify an overall default endpoint, and then as many
/// level-specific endpoints as you need:
///
/// ```cpp
/// fastly::log::LoggerBuilder()
///     .max_level(log::LogLevelFilter::Info)
///     .default_endpoint("my_endpoint")
///     .default_level_endpoint("error_only", fastly::log::Level::Error)
///     .init();
/// fastly::log::info("This will be written to my_endpoint...");
/// fastly::log::warn(".. and this will too.");
/// fastly::log::error("But this will be written to error_only");
/// ```
///
/// # Registering endpoints
///
/// All endpoints used by your logging statements must be registered when the
/// logger is created. The following functions automatically register an
/// endpoint if it is not already registered.
///
/// - `init_simple()`
/// - `LoggerBuilder::endpoint()`
/// - `LoggerBuilder::endpoint_level()`
/// - `LoggerBuilder::default_endpoint()`
/// - `LoggerBuilder::default_level_endpoint()`
///
/// You can pass the endpoint name as an `std::string_view`-able, or an explicit
/// `fastly::log::Endpoint` value. The following examples are equivalent:
///
/// ```cpp
/// fastly::log::init_simple("my_endpoint", fastly::log::LogLevelFilter::Info);
/// ```
///
/// ```cpp
/// fastly::log::init_simple(
///     fastly::log::Endpoint::from_name("my_endpoint").value(),
///     fastly::log::LogLevelFilter::Info,
/// );
/// ```
///
/// If a logging statement uses one of the `_to`-suffixed versions with
/// `"my_endpoint"` but `my_endpoint` is not registered, the message will be
/// logged to the default endpoint for that level, if one exists.
///
/// [log]: https://docs.rs/log
/// [log-use]: https://docs.rs/log#use
/// [rtls]:
/// https://docs.fastly.com/en/guides/about-fastlys-realtime-log-streaming-features
/// [regex]: https://docs.rs/regex#syntax
/// [log-issue]: https://github.com/rust-lang/log/issues/390
namespace fastly::log {

using LogLevel = fastly::sys::log::LogLevel;
using LogLevelFilter = fastly::sys::log::LogLevelFilter;

/// Send an Error-level message to the configured default endpoint.
/// Supports C++20 format strings.
///
/// Logging must be initialized in order for this to do anything. See
/// `init_simple()` and `LoggerBuilder` for more information.
///
/// # Example
///
/// ```cpp
/// fastly::log::init_simple("my_log_endpoint");
/// fastly::log::error("my name is {}", user->name());
/// ```
template <typename... Args>
void error(std::format_string<Args...> fmt, Args &&...args) {
  fastly::sys::log::f_log_log(fastly::sys::log::LogLevel::Error,
                              std::format(fmt, args...));
}

/// Send an Error-level message to a specified configured endpoint.
/// Supports C++20 format strings.
///
/// Logging must be initialized in order for this to do anything. See
/// `init_simple()` and `LoggerBuilder` for more information.
///
/// # Example
///
/// ```cpp
/// fastly::log::LoggerBuilder()
///     .default_endpoint("my_default_logs")
///     .endpoint("my_other_endpoint")
///     .init();
/// fastly::log::error("Whoops");
/// fastly::log::error_to("my_other_endpoint", "my name is {}", user->name());
/// ```
template <typename... Args>
void error_to(std::string_view dest, std::format_string<Args...> fmt,
              Args &&...args) {
  fastly::sys::log::f_log_log_to(static_cast<std::string>(dest),
                                 fastly::sys::log::LogLevel::Error,
                                 std::format(fmt, args...));
}

/// Send a Warn-level message to the configured default endpoint.
/// Supports C++20 format strings.
///
/// Logging must be initialized in order for this to do anything. See
/// `init_simple()` and `LoggerBuilder` for more information.
///
/// # Example
///
/// ```cpp
/// fastly::log::init_simple("my_log_endpoint");
/// fastly::log::warn("my name is {}", user->name());
/// ```
template <typename... Args>
void warn(std::format_string<Args...> fmt, Args &&...args) {
  fastly::sys::log::f_log_log(fastly::sys::log::LogLevel::Warn,
                              std::format(fmt, args...));
}

/// Send a Warn-level message to a specified configured endpoint.
/// Supports C++20 format strings.
///
/// Logging must be initialized in order for this to do anything. See
/// `init_simple()` and `LoggerBuilder` for more information.
///
/// # Example
///
/// ```cpp
/// fastly::log::LoggerBuilder()
///     .default_endpoint("my_default_logs")
///     .endpoint("my_other_endpoint")
///     .init();
/// fastly::log::warn("Whoops");
/// fastly::log::warn_to("my_other_endpoint", "my name is {}", user->name());
/// ```
template <typename... Args>
void warn_to(std::string_view dest, std::format_string<Args...> fmt,
             Args &&...args) {
  fastly::sys::log::f_log_log_to(static_cast<std::string>(dest),
                                 fastly::sys::log::LogLevel::Warn,
                                 std::format(fmt, args...));
}

/// Send an Info-level message to the configured default endpoint.
/// Supports C++20 format strings.
///
/// Logging must be initialized in order for this to do anything. See
/// `init_simple()` and `LoggerBuilder` for more information.
///
/// # Example
///
/// ```cpp
/// fastly::log::init_simple("my_log_endpoint");
/// fastly::log::info("my name is {}", user->name());
/// ```
template <typename... Args>
void info(std::format_string<Args...> fmt, Args &&...args) {
  fastly::sys::log::f_log_log(fastly::sys::log::LogLevel::Info,
                              std::format(fmt, std::forward<Args>(args)...));
}

/// Send an Info-level message to a specified configured endpoint.
/// Supports C++20 format strings.
///
/// Logging must be initialized in order for this to do anything. See
/// `init_simple()` and `LoggerBuilder` for more information.
///
/// # Example
///
/// ```cpp
/// fastly::log::LoggerBuilder()
///     .default_endpoint("my_default_logs")
///     .endpoint("my_other_endpoint")
///     .init();
/// fastly::log::info("Whoops");
/// fastly::log::info_to("my_other_endpoint", "my name is {}", user->name());
/// ```
template <typename... Args>
void info_to(std::string_view dest, std::format_string<Args...> fmt,
             Args &&...args) {
  fastly::sys::log::f_log_log_to(static_cast<std::string>(dest),
                                 fastly::sys::log::LogLevel::Info,
                                 std::format(fmt, args...));
}

/// Send a Debug-level message to the configured default endpoint.
/// Supports C++20 format strings.
///
/// Logging must be initialized in order for this to do anything. See
/// `init_simple()` and `LoggerBuilder` for more information.
///
/// # Example
///
/// ```cpp
/// fastly::log::init_simple("my_log_endpoint");
/// fastly::log::debug("my name is {}", user->name());
/// ```
template <typename... Args>
void debug(std::format_string<Args...> fmt, Args &&...args) {
  fastly::sys::log::f_log_log(fastly::sys::log::LogLevel::Debug,
                              std::format(fmt, args...));
}

/// Send a Debug-level message to a specified configured endpoint.
/// Supports C++20 format strings.
///
/// Logging must be initialized in order for this to do anything. See
/// `init_simple()` and `LoggerBuilder` for more information.
///
/// # Example
///
/// ```cpp
/// fastly::log::LoggerBuilder()
///     .default_endpoint("my_default_logs")
///     .endpoint("my_other_endpoint")
///     .init();
/// fastly::log::debug("Whoops");
/// fastly::log::debug_to("my_other_endpoint", "my name is {}", user->name());
/// ```
template <typename... Args>
void debug_to(std::string_view dest, std::format_string<Args...> fmt,
              Args &&...args) {
  fastly::sys::log::f_log_log_to(static_cast<std::string>(dest),
                                 fastly::sys::log::LogLevel::Debug,
                                 std::format(fmt, args...));
}

/// Send a Trace-level message to the configured default endpoint.
/// Supports C++20 format strings.
///
/// Logging must be initialized in order for this to do anything. See
/// `init_simple()` and `LoggerBuilder` for more information.
///
/// # Example
///
/// ```cpp
/// fastly::log::init_simple("my_log_endpoint");
/// fastly::log::trace("my name is {}", user->name());
/// ```
template <typename... Args>
void trace(std::format_string<Args...> fmt, Args &&...args) {
  fastly::sys::log::f_log_log(fastly::sys::log::LogLevel::Trace,
                              std::format(fmt, args...));
}

/// Send a Trace-level message to a specified configured endpoint.
/// Supports C++20 format strings.
///
/// Logging must be initialized in order for this to do anything. See
/// `init_simple()` and `LoggerBuilder` for more information.
///
/// # Example
///
/// ```cpp
/// fastly::log::LoggerBuilder()
///     .default_endpoint("my_default_logs")
///     .endpoint("my_other_endpoint")
///     .init();
/// fastly::log::trace("Whoops");
/// fastly::log::trace_to("my_other_endpoint", "my name is {}", user->name());
/// ```
template <typename... Args>
void trace_to(std::string_view dest, std::format_string<Args...> fmt,
              Args &&...args) {
  fastly::sys::log::f_log_log_to(static_cast<std::string>(dest),
                                 fastly::sys::log::LogLevel::Trace,
                                 std::format(fmt, args...));
}

/// Set the global maximum log level
void set_max_level(LogLevelFilter level);

/// Returns the current maximum log level.
///
/// The maximum log level is set by the `set_max_level()` function.
LogLevelFilter max_level();

class LoggerBuilder;

/// A Fastly logging endpoint.
///
/// To write to this endpoint, use the `<iostream>` interface:
///
/// ```cpp
/// auto endpoint{fastly::log::Endpoint::from_name("my_log_endpoint")};
/// endpoint << "stuff happened y'all";
/// ```
class Endpoint {
  friend LoggerBuilder;
  friend void init_simple(Endpoint endpoint, LogLevelFilter level);

public:
  /// Get the name of an `Endpoint`.
  std::string name();

  /// Try to get an `Endpoint` by name.
  ///
  /// Currently, the conditions on an endpoint name are:
  ///
  /// - It must not be empty
  ///
  /// - It must not contain newlines (`\n`) or colons (`:`)
  ///
  /// - It must not be `stdout` or `stderr`, which are reserved for
  /// debugging.
  static fastly::expected<Endpoint> from_name(std::string_view name);

private:
  Endpoint(rust::Box<fastly::sys::log::Endpoint> e) : ep(std::move(e)) {};
  rust::Box<fastly::sys::log::Endpoint> ep;
};

/// Initialize logging with a single endpoint filtered by log level.
///
/// For advanced configuration, see the `LoggerBuilder` class, and the
/// [module-level documentation](index.html#advanced-configuration).
///
/// ```cpp
/// fastly::log::init_simple("my_endpoint", fastly::log::LogLevelFilter::Warn);
/// fastly::log::warn("This will be written to my_endpoint...");
/// fastly::log::info("...but this won't");
/// ```
void init_simple(Endpoint endpoint, LogLevelFilter level);
void init_simple(std::string_view endpoint, LogLevelFilter level);
void init_simple(Endpoint endpoint);
void init_simple(std::string_view endpoint);

/// A builder class for configuring endpoints in detail.
///
/// You can use this builder to register endpoints, set default endpoints,
/// control the levels of logging messages emitted, and filter messages based on
/// module name.
class LoggerBuilder {
public:
  /// Create a new `LoggerBuilder`.
  ///
  /// By default, no endpoints are registered, the maximum log level is set to
  /// `LogLevelFilter::Off`, and no module name filtering is done.
  LoggerBuilder() : lb(fastly::sys::log::m_static_log_logger_builder_new()) {}

  /// Register an endpoint.
  ///
  /// ```cpp
  /// fastly::log::LoggerBuilder()
  ///     .max_level(log::LogLevelFilter::Trace)
  ///     .endpoint("my_endpoint")
  ///     .init();
  /// fastly::log::info_to("my_endpoint", "Hello");
  /// ```
  LoggerBuilder endpoint(Endpoint endpoint);
  LoggerBuilder endpoint(std::string_view endpoint);

  /// Register an endpoint and set the maximum logging level for its messages.
  ///
  /// ```cpp
  /// fastly::log::LoggerBuilder()
  ///     .max_level(fastly::log::LogLevelFilter::Trace)
  ///     .endpoint_level("debug_endpoint", fastly::log::LogLevelFilter::Debug)
  ///     .init();
  /// fastly::log::info_to(
  ///   "debug_endpoint",
  ///   "This will be written to debug_endpoint..."
  /// );
  /// fastly::log::trace_to(
  ///   "debug_endpoint",
  ///   "...but this won't be..."
  /// );
  /// ```
  LoggerBuilder endpoint_level(Endpoint endpoint, LogLevelFilter level);
  LoggerBuilder endpoint_level(std::string_view endpoint, LogLevelFilter level);

  /// Set the default endpoint for all messages.
  ///
  /// The default endpoint is used when the logging statement does not use the
  /// `_to`-suffixed logger statements.
  ///
  /// This overrides any previous default endpoints, set either by this method
  /// or by
  /// `LoggerBuilder::default_level_endpoint()`.
  ///
  /// ```cpp
  /// fastly::log::LoggerBuilder()
  ///     .max_level(fastly::log::LogLevelFilter::Info)
  ///     .default_level_endpoint("error_only", fastly::log::Level::Error)
  ///     .default_endpoint("my_endpoint")
  ///     .endpoint("other_endpoint")
  ///     .init();
  /// fastly::log::info("This will be written to my_endpoint...");
  /// fastly::log::error("...and this will too");
  /// fastly::log::warn_to(
  ///   "other_endpoint",
  ///   "This will go to other_endpoint, though"
  /// );
  /// ```
  LoggerBuilder default_endpoint(Endpoint endpoint);
  LoggerBuilder default_endpoint(std::string_view endpoint);

  /// Set the default endpoint for all messages of the given level.
  ///
  /// The default endpoint is used when the logging statement does not use one
  /// of the `_to`-suffixed functions.
  ///
  /// This overrides any previous default endpoints set for this level, either
  /// by this method or by `LoggerBuilder::default_endpoint()`.
  ///
  /// **Note:** Unlike most/all other functions in this module, the level is
  /// actually a `fastly::log::LogLevel`, not a `fastly::log::LogLevelFilter`,
  /// and refers to a _specific_ level, not a range.
  ///
  /// # Example
  ///
  /// ```cpp
  /// fastly::log::LoggerBuilder()
  ///     .max_level(fastly::log::LogLevelFilter::Info)
  ///     .default_endpoint("my_endpoint")
  ///     .default_level_endpoint("error_only", fastly::log::Level::Error)
  ///     .endpoint("other_endpoint")
  ///     .init();
  /// fastly::log::info("This will be written to my_endpoint...");
  /// fastly::log::error("...but this will be written to error_only");
  /// fastly::log::error_to(
  ///   "other_endpoint",
  ///   "This will go to other_endpoint, though"
  /// );
  /// ```
  LoggerBuilder default_level_endpoint(Endpoint endpoint, LogLevel level);
  LoggerBuilder default_level_endpoint(std::string_view endpoint,
                                       LogLevel level);

  /// Set the maximum logging level for all messages.
  ///
  /// No messages that exceed this level will be emitted, even if a higher level
  /// is set for a specific endpoint or module name.
  ///
  /// **Note:** The default level is `LogLevelFilter::Off`, which emits no
  /// logging at all. You'll want to change this for most configurations.
  ///
  /// ```cpp
  /// fastly::log::LoggerBuilder()
  ///     .max_level(fastly::log::LogLevelFilter::Warn)
  ///     .endpoint_level("my_endpoint", fastly::log::LogLevelFilter::Info)
  ///     .init();
  /// fastly::log::warn_to(
  ///   "my_endpoint",
  ///   "This will be written to my_endpoint..."
  /// );
  /// fastly::log::info_to("my_endpoint", "...but this won't");
  /// ```
  LoggerBuilder max_level(LogLevelFilter level);

  /// Set whether all log messages should be echoed to `stdout` (`false` by
  /// default).
  ///
  /// If this is set to `true`, all logging statements will write the message to
  /// `stdout` in addition to the specified endpoint. This is particularly
  /// useful when debugging with [Compute Log Tailing][log-tailing].
  ///
  /// [log-tailing]:
  /// https://www.fastly.com/blog/introducing-compute-edge-log-tailing-for-better-observability-and-easier-debugging
  LoggerBuilder echo_stdout(bool enabled);

  /// Set whether all log messages should be echoed to `stderr` (`false` by
  /// default).
  ///
  /// If this is set to `true`, all logging statements will write the message to
  /// `stderr` in addition to the specified endpoint. This is particularly
  /// useful when debugging with [Compute Log Tailing][log-tailing].
  ///
  /// [log-tailing]:
  /// https://www.fastly.com/blog/introducing-compute-edge-log-tailing-for-better-observability-and-easier-debugging
  LoggerBuilder echo_stderr(bool enabled);

  /// Build the logger and initialize it as the global logger.
  ///
  /// ```cpp
  /// fastly::log::LoggerBuilder()
  ///     .default_endpoint("my_endpoint")
  ///     .init();
  /// fastly::log::info("Hello");
  /// ```
  ///
  /// # Panics
  ///
  /// This may panic for various reasons if the logger fails to initialize.
  void init();

private:
  LoggerBuilder(rust::Box<fastly::sys::log::LoggerBuilder> l)
      : lb(std::move(l)) {};
  rust::Box<fastly::sys::log::LoggerBuilder> lb;
};

} // namespace fastly::log

#endif
