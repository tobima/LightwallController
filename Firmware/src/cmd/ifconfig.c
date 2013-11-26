#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "chprintf.h"
#include "ch.h"

#include <string.h>

void
cmd_ifconfig(BaseSequentialStream *chp, int argc, char *argv[])
{
  struct netif* netif_et0 = netif_default;
  ip_addr_t addr;
  ip_addr_t netmask;
  ip_addr_t gw;

  /* fill the actual values into the fields, that will be modified */
  addr.addr = netif_et0->ip_addr.addr;
  netmask.addr = netif_et0->netmask.addr;
  gw.addr = netif_et0->gw.addr;

  /* Send some output to the user */
  if (argc < 1)
    {
      chprintf(chp, "%c%c%d\tHWaddr %02x:%02x:%02x:%02x:%02x:%02x\r\n",
          netif_et0->name[0], netif_et0->name[1], netif_et0->num,
          netif_et0->hwaddr[0], netif_et0->hwaddr[1], netif_et0->hwaddr[2],
          netif_et0->hwaddr[3], netif_et0->hwaddr[4], netif_et0->hwaddr[5]);
      chprintf(chp, "\tinet addr %d.%d.%d.%d\r\n",
          ip4_addr1(&(netif_et0->ip_addr.addr)),
          ip4_addr2(&(netif_et0->ip_addr.addr)),
          ip4_addr3(&(netif_et0->ip_addr.addr)),
          ip4_addr4(&(netif_et0->ip_addr.addr)));

      chprintf(chp, "\tinet mask %d.%d.%d.%d\r\n",
          ip4_addr1(&(netif_et0->netmask.addr)),
          ip4_addr2(&(netif_et0->netmask.addr)),
          ip4_addr3(&(netif_et0->netmask.addr)),
          ip4_addr4(&(netif_et0->netmask.addr)));

      chprintf(chp, "\tinet gw %d.%d.%d.%d\r\n",
          ip4_addr1(&(netif_et0->gw.addr)), ip4_addr2(&(netif_et0->gw.addr)),
          ip4_addr3(&(netif_et0->gw.addr)), ip4_addr4(&(netif_et0->gw.addr)));
    }
  else
    {
      if (strcmp(argv[0], "ms0") == 0) /* this is the only network interface */
        {
          if (argc >= 3)
            {
              addr.addr = ipaddr_addr(argv[1]);
              netmask.addr = ipaddr_addr(argv[2]);
            }
          else
            {
              chprintf(chp,
                  "try e.g. ifconfig ms0 192.168.0.2 255.255.255.0\r\n");
            }

          if (argc >= 4)
            {
              gw.addr = ipaddr_addr(argv[3]);
            }

          netifapi_netif_set_addr(netif_et0, &addr, &netmask, &gw);
        }
      else
        {
          chprintf(chp,
              "ifconfig interface [address [network mask]] [defaultgw]\r\n");
        }

    }
}
;
