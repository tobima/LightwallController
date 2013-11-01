#include "lwip/netif.h"
#include "chprintf.h"
#include "ch.h"

void cmd_ifconfig(BaseSequentialStream *chp, int argc, char *argv[]) {
  struct netif* netif_et0 = netif_default;
  
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
}; 
