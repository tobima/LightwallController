#ifndef _NET_SHELL_H_
#define _NET_SHELL_H_

#ifndef NET_SHELL_THREAD_STACK_SIZE
#define NET_SHELL_THREAD_STACK_SIZE   THD_WA_SIZE(512)
#endif

#ifndef NET_SHELL__PORT
#define NET_SHELL_PORT         22
#endif

#ifndef NET_SHELL_THREAD_PRIORITY
#define NET_SHELL_THREAD_PRIORITY     (LOWPRIO + 2)
#endif

extern WORKING_AREA(wa_net_shell_server, NET_SHELL_THREAD_STACK_SIZE);

#ifdef __cplusplus
extern "C"
{
#endif
  msg_t
  server_thread(void *p);
#ifdef __cplusplus
}
#endif
#endif
