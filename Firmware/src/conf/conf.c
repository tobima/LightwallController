#include "conf.h"
#include "ini/ini.h"
#include <string.h>

#include "lwip/netif.h"

static uint8_t macid[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00 };

static int handler(void* config, const char* section, const char* name, 
		   const char* value)
{
  configuration_t* pconfig = (configuration_t*) config;
  
  #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
  if (MATCH("ms0", "address")) {
    pconfig->network.address = ipaddr_addr(value);
  } else if (MATCH("ms0", "netmask")) {
    pconfig->network.netmask = ipaddr_addr(value);
  } else if (MATCH("ms0", "gateway")) {
    pconfig->network.gateway = ipaddr_addr(value);
  } else {
    return 0;  /* unknown section/name, error */
  }
  return 1;
}

int conf_load(configuration_t* config) {
  config->network.macaddress = macid;
  
  if (ini_parse("fc/conf/network", handler, config) < 0) {
        return 1;
    }
  
  return 0;
}