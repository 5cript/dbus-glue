#include <systemd/sd-bus.h>

sd_bus_error dbus_glue_sdbus_error_null(void)
{
  return SD_BUS_ERROR_NULL;
}
