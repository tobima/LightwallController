#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "chprintf.h"
#include "ch.h"

void cmd_ifconfig(BaseSequentialStream *chp, int argc, char *argv[]) {
  struct netif* netif_et0 = netif_default;

  if (argc<1) { 
    chprintf(chp, 
           "%c%c%d\tHWaddr %02x:%02x:%02x:%02x:%02x:%02x\r\n",
           netif_et0->name[0],
           netif_et0->name[1],
           netif_et0->num,
           netif_et0->hwaddr[0],
           netif_et0->hwaddr[1],
           netif_et0->hwaddr[2],
           netif_et0->hwaddr[3],
           netif_et0->hwaddr[4],
           netif_et0->hwaddr[5]);
    chprintf(chp,
           "\tinet addr %d.%d.%d.%d\r\n",
           ip4_addr1(&(netif_et0->ip_addr.addr)),
           ip4_addr2(&(netif_et0->ip_addr.addr)),
           ip4_addr3(&(netif_et0->ip_addr.addr)),
           ip4_addr4(&(netif_et0->ip_addr.addr)));
  } else {
    ip_addr_t addr;
    addr.addr = ipaddr_addr("10.23.42.123");
    ip_addr_t netmask;
    netmask.addr = ipaddr_addr("255.255.255.0");
    ip_addr_t gw;
    gw.addr = ipaddr_addr("10.23.42.1");
    
    netifapi_netif_set_addr(
      netif_et0,
      &addr,
      &netmask,
      &gw);
  }
}; 
