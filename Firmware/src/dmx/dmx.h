#ifndef _DMX_H_
#define _DMX_H_ 

#ifndef DMX_THREAD_STACK_SIZE
#define DMX_THREAD_STACK_SIZE   THD_WA_SIZE(128)
#endif

#ifndef DMX_THREAD_PRIORITY
#define DMX_THREAD_PRIORITY     (LOWPRIO + 3)
#endif

#define DMX_BUFFER_MAX	513

typedef struct  {
  uint8_t            buffer [DMX_BUFFER_MAX];
  uint16_t           length;
} DMXBuffer;

extern DMXBuffer dmx_buffer;

extern WORKING_AREA(wa_dmx, DMX_THREAD_STACK_SIZE);

#ifdef __cplusplus
extern "C" {
#endif
  msg_t dmxthread(void *p);
  void DMXInit(void);
#ifdef __cplusplus
}
#endif

#endif /*_DMX_H_*/