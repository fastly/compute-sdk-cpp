//! @example config_store.cpp
#include "fastly/sdk.h"

int main() {
  fastly::log::init_simple("logs");
  auto store = fastly::ConfigStore::open("example-store");
  fastly::Response::from_body(store->get("hello")->value()).send_to_client();
}
