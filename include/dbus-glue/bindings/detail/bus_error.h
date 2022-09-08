#pragma once

#include <systemd/sd-bus.h>

extern "C" {
  sd_bus_error dbus_glue_sdbus_error_null();
}
