#ifndef FASTLY_GEO_H
#define FASTLY_GEO_H

#include "error.h"
#include "sdk-sys.h"
#include "util.h"
#include <optional>
#include <string>
#include <string_view>

namespace fastly::geo {

// Straight up re-export these enums. They're shared types.
using fastly::sys::geo::ConnSpeed;
using fastly::sys::geo::ConnType;
using fastly::sys::geo::Continent;
using fastly::sys::geo::ProxyDescription;
using fastly::sys::geo::ProxyType;

class Geo;

/// Look up the geographic data associated with a particular IP address.
///
/// Returns `std::nullopt` if no geographic data is available, such as when the
/// IP address is reserved for private use.
///
/// # Examples
///
/// To get geographic information for the downstream client:
///
/// ```cpp
/// auto client_ip{fastly::Request::from_client().get_client_ip_addr().value()};
/// auto geo{fastly::geo::geo_lookup(client_ip).value()};
/// if (geo.conn_type() == fastly::geo::ConnType::Satellite) {
///     std::cout << "receiving a request from outer space 🛸";
/// }
/// ```
fastly::expected<std::optional<Geo>> geo_lookup(std::string_view ip);

/// An offset from UTC.
///
/// This class can store values up to ±25:59:59.
class UtcOffset {
  friend Geo;

public:
  /// Obtain the number of whole hours the offset is from UTC. A positive value
  /// indicates an offset to the east; a negative to the west.
  int8_t whole_hours();

  /// Obtain the number of whole minutes the offset is from UTC. A positive
  /// value indicates an offset to the east; a negative to the west.
  int16_t whole_minutes();

  /// Obtain the number of minutes past the hour the offset is from UTC. A
  /// positive value indicates an offset to the east; a negative to the west.
  int8_t minutes_past_hour();

  /// Obtain the number of whole seconds the offset is from UTC. A positive
  /// value indicates an offset to the east; a negative to the west.
  int32_t whole_seconds();

  /// Obtain the number of seconds past the minute the offset is from UTC. A
  /// positive value indicates an offset to the east; a negative to the west.
  int8_t seconds_past_minute();

  /// Check if the offset is exactly UTC.
  bool is_utc();

  /// Check if the offset is positive, or east of UTC.
  bool is_positive();

  /// Check if the offset is negative, or west of UTC.
  bool is_negative();

private:
  rust::Box<fastly::sys::geo::UtcOffset> offset;
  UtcOffset(rust::Box<fastly::sys::geo::UtcOffset> o) : offset(std::move(o)) {};
};

/// The geographic data associated with a particular IP address.
class Geo {
  friend fastly::expected<std::optional<Geo>> geo_lookup(std::string_view ip);

public:
  /// The name of the organization associated with `as_number`.
  ///
  /// For example, `fastly` is the value given for IP addresses under AS-54113.
  std::string as_name();

  /// [Autonomous
  /// system](https://en.wikipedia.org/wiki/Autonomous_system_(Internet)) (AS)
  /// number.
  uint32_t as_number();

  /// The telephone area code associated with an IP address.
  ///
  /// These are only available for IP addresses in the United States, its
  /// territories, and Canada.
  uint16_t area_code();

  /// City or town name.
  std::string city();

  /// Connection speed.
  ConnSpeed conn_speed();

  /// Connection type.
  ConnType conn_type();

  /// Continent.
  Continent continent();

  /// A two-character [ISO 3166-1][iso] country code for the country associated
  /// with an IP address.
  ///
  /// The US country code is returned for IP addresses associated with overseas
  /// United States military bases.
  ///
  /// These values include subdivisions that are assigned their own country
  /// codes in ISO 3166-1. For example, subdivisions NO-21 and NO-22 are
  /// presented with the country code SJ for Svalbard and the Jan Mayen Islands.
  ///
  /// [iso]: https://en.wikipedia.org/wiki/ISO_3166-1
  std::string country_code();

  /// A three-character [ISO 3166-1 alpha-3][iso] country code for the country
  /// associated with the IP address.
  ///
  /// The USA country code is returned for IP addresses associated with overseas
  /// United States military bases.
  ///
  /// [iso]: https://en.wikipedia.org/wiki/ISO_3166-1_alpha-3
  std::string country_code3();

  /// Country name.
  ///
  /// This field is the [ISO 3166-1][iso] English short name for a country.
  ///
  /// [iso]: https://en.wikipedia.org/wiki/ISO_3166-1
  std::string country_name();

  /// Latitude, in units of degrees from the equator.
  ///
  /// Values range from -90.0 to +90.0 inclusive, and are based on the [WGS
  /// 84][wgs84] coordinate reference system.
  ///
  /// [wgs84]: https://en.wikipedia.org/wiki/World_Geodetic_System
  double latitude();

  /// Longitude, in units of degrees from the [IERS Reference Meridian][iers].
  ///
  /// Values range from -180.0 to +180.0 inclusive, and are based on the [WGS
  /// 84][wgs84] coordinate reference system.
  ///
  /// [iers]: https://en.wikipedia.org/wiki/IERS_Reference_Meridian
  /// [wgs84]: https://en.wikipedia.org/wiki/World_Geodetic_System
  double longitude();

  /// Metro code, representing designated market areas (DMAs) in the United
  /// States.
  int64_t metro_code();

  /// The postal code associated with the IP address.
  ///
  /// These are available for some IP addresses in Australia, Canada, France,
  /// Germany, Italy, Spain, Switzerland, the United Kingdom, and the United
  /// States.
  ///
  /// For Canadian postal codes, this is the first 3 characters. For the United
  /// Kingdom, this is the first 2-4 characters (outward code). For countries
  /// with alphanumeric postal codes, this field is a lowercase transliteration.
  std::string postal_code();

  /// Client proxy description.
  ProxyDescription proxy_description();

  /// Client proxy type.
  ProxyType proxy_type();

  /// [ISO 3166-2][iso] country subdivision code.
  ///
  /// For countries with multiple levels of subdivision (for example, nations
  /// within the United Kingdom), this variable gives the more specific
  /// subdivision.
  ///
  /// This field can be `std::nullopt` for countries that do not have ISO
  /// country subdivision codes. For example, `std::nullopt` is given for IP
  /// addresses assigned to the Åland Islands (country code AX, illustrated
  /// below).
  ///
  /// # Examples
  ///
  /// Region values are the subdivision part only. For typical use, a
  /// subdivision is normally formatted with its associated country code. The
  /// following example illustrates constructing an [ISO 3166-2][iso] two-part
  /// country and subdivision code from the respective fields:
  ///
  /// ```cpp
  /// auto
  /// client_ip{fastly::Request::from_client().get_client_ip_addr().value()};
  /// auto geo{fastly::geo::geo_lookup(client_ip).value()};
  /// if (auto region = geo.region()) {
  ///   return std::format("{}-{}", geo.country_code(), region.value());
  /// } else {
  ///   return geo.country_code();
  /// }
  /// ```
  ///
  /// | `code`| Region Name | Country | ISO 3166-2 subdivision |
  /// | --- | --- | --- | --- |
  /// | `AX`| Ödkarby | Åland Islands | (none) |
  /// | `DE-BE` | Berlin | Germany | Land (State) |
  /// | `GB-BNH` | Brighton and Hove | United Kingdom | Unitary authority |
  /// | `JP-13` | 東京都 (Tōkyō-to) | Japan | Prefecture |
  /// | `RU-MOW` | Москва́ (Moscow) | Russian Federation | Federal city |
  /// | `SE-AB` | Stockholms län | Sweden | Län (County) |
  /// | `US-CA` | California | United States | State |
  ///
  /// [iso]: https://en.wikipedia.org/wiki/ISO_3166-2
  std::optional<std::string> region();

  /// Time zone offset from coordinated universal time (UTC) for `city`.
  ///
  /// Returns `std::nullopt` if the geolocation database does not have a time
  /// zone offset for this IP address.
  std::optional<UtcOffset> utc_offset();

private:
  rust::Box<fastly::sys::geo::Geo> geo;
  Geo(rust::Box<fastly::sys::geo::Geo> g) : geo(std::move(g)) {};
};

} // namespace fastly::geo

#endif
