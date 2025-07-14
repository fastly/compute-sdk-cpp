#include "geo.h"

namespace fastly::geo {

fastly::expected<std::optional<Geo>> geo_lookup(std::string_view ip) {
  fastly::sys::geo::Geo *out;
  fastly::sys::error::FastlyError *err;
  fastly::sys::geo::f_geo_geo_lookup(static_cast<std::string>(ip), out, err);
  if (err != nullptr) {
    return fastly::unexpected(err);
  } else if (out != nullptr) {
    return FSLY_BOX(geo, Geo, out);
  } else {
    return std::nullopt;
  }
}

int8_t UtcOffset::whole_hours() { return this->offset->whole_hours(); }
int16_t UtcOffset::whole_minutes() { return this->offset->whole_minutes(); }
int8_t UtcOffset::minutes_past_hour() {
  return this->offset->minutes_past_hour();
}
int32_t UtcOffset::whole_seconds() { return this->offset->whole_seconds(); }
int8_t UtcOffset::seconds_past_minute() {
  return this->offset->seconds_past_minute();
}
bool UtcOffset::is_utc() { return this->offset->is_utc(); }
bool UtcOffset::is_positive() { return this->offset->is_positive(); }
bool UtcOffset::is_negative() { return this->offset->is_negative(); }

std::string Geo::as_name() {
  std::string out;
  this->geo->as_name(out);
  return out;
}

uint32_t Geo::as_number() { return this->geo->as_number(); }
uint16_t Geo::area_code() { return this->geo->area_code(); }
std::string Geo::city() {
  std::string out;
  this->geo->city(out);
  return out;
}
ConnSpeed Geo::conn_speed() { return this->geo->conn_speed(); }
ConnType Geo::conn_type() { return this->geo->conn_type(); }
Continent Geo::continent() { return this->geo->continent(); }
std::string Geo::country_code() {
  std::string out;
  this->geo->country_code(out);
  return out;
}
std::string Geo::country_code3() {
  std::string out;
  this->geo->country_code3(out);
  return out;
}
std::string Geo::country_name() {
  std::string out;
  this->geo->country_name(out);
  return out;
}
double Geo::latitude() { return this->geo->latitude(); }
double Geo::longitude() { return this->geo->longitude(); }
int64_t Geo::metro_code() { return this->geo->metro_code(); }
std::string Geo::postal_code() {
  std::string out;
  this->geo->postal_code(out);
  return out;
}
ProxyDescription Geo::proxy_description() {
  return this->geo->proxy_description();
}
ProxyType Geo::proxy_type() { return this->geo->proxy_type(); }
std::optional<std::string> Geo::region() {
  std::string out;
  if (this->geo->region(out)) {
    return std::optional<std::string>(out);
  } else {
    return std::nullopt;
  }
}
std::optional<UtcOffset> Geo::utc_offset() {
  auto ptr{this->geo->utc_offset()};
  if (ptr != nullptr) {
    return std::optional<UtcOffset>(FSLY_BOX(geo, UtcOffset, ptr));
  } else {
    return std::nullopt;
  }
}

} // namespace fastly::geo
