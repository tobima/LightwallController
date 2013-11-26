#include "conf.h"
#include "ini/ini.h"
#include <string.h>
#include <stdlib.h>

#include "lwip/netif.h"

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

static uint8_t macid[] =
  { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00 };

static int
net_handler(void* config, const char* section, const char* name,
    const char* value)
{
  char buffer[] =
    { 0, 0, 0 };
  int i;

  configuration_t* pconfig = (configuration_t*) config;

  if (MATCH("ms0", "macid"))
    {
      if (strlen(value) == 17)
        {
          for (i = 0; i < 6; i++)
            {
              if ((value[i * 3 + 2] == ':') || i == 5)
                {
                  strncpy(buffer, value + i * 3, 2);
                  macid[i] = strtol(buffer, NULL, 16);
                }
              else
                {
                  return 0; /* not correct fromated mac address, error */
                }
            }
        }
      else
        {
          return 0; /* wrong size for mac address, error */
        }
    }
  else if (MATCH("ms0", "address"))
    {
      pconfig->network.address = ipaddr_addr(value);
    }
  else if (MATCH("ms0", "netmask"))
    {
      pconfig->network.netmask = ipaddr_addr(value);
    }
  else if (MATCH("ms0", "gateway"))
    {
      pconfig->network.gateway = ipaddr_addr(value);
    }
  else
    {
      return 0; /* unknown section/name, error */
    }
  return 1;
}

int
conf_load(configuration_t* config)
{
  config->network.macaddress = macid;

  if (ini_parse("fc/conf/network", net_handler, config) < 0)
    {
      return 1;
    }

  return 0;
}
