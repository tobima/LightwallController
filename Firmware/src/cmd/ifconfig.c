#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "chprintf.h"
#include "ch.h"

#include <string.h>

void cmd_ifconfig(BaseSequentialStream *chp, int argc, char *argv[]) {
  struct netif* netif_et0 = netif_default;

  if (argc < 1)
  { 
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
	  
	chprintf(chp,
			   "\tinet mask %d.%d.%d.%d\r\n",
			   ip4_addr1(&(netif_et0->netmask.addr)),
			   ip4_addr2(&(netif_et0->netmask.addr)),
			   ip4_addr3(&(netif_et0->netmask.addr)),
			   ip4_addr4(&(netif_et0->netmask.addr)));
	  
	chprintf(chp,
			   "\tinet gw %d.%d.%d.%d\r\n",
			   ip4_addr1(&(netif_et0->gw.addr)),
			   ip4_addr2(&(netif_et0->gw.addr)),
			   ip4_addr3(&(netif_et0->gw.addr)),
			   ip4_addr4(&(netif_et0->gw.addr)));
  }
  else
  {
	  if (strcmp(argv[0], "help") == 0 || strcmp(argv[0], "--help") == 0)
	  {
		  chprintf(chp, "ifconfig interface [address [network mask]] [defaultgw]\r\n");
	  }
	  else if (strcmp(argv[0], "ms0") == 0 && argc >= 4) /* this is the only network interface */
	  {
		  ip_addr_t addr;
		  addr.addr = ipaddr_addr(argv[1]);
		  ip_addr_t netmask;
		  netmask.addr = ipaddr_addr(argv[2]);
		  ip_addr_t gw;
		  gw.addr = ipaddr_addr(argv[3]);
		  
		  netifapi_netif_set_addr(netif_et0, &addr, &netmask, &gw);
	  }
  }
}; 
