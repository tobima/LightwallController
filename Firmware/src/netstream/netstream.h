#ifndef NETSTREAM_H_
#define NETSTREAM_H_

#include "ch.h"

#define _net_stream_data \
       _base_sequential_stream_data \
       struct netconn * conn; \
       struct netbuf * inbuf; \
       size_t in_offset;

struct NetStreamVMT
{
  _base_sequential_stream_methods
};

/**
 * @extends BaseSequentialStream
 */
typedef struct
{
  const struct NetStreamVMT *vmt;_net_stream_data
} NetStream;

#ifdef __cplusplus
extern "C"
{
#endif
  void
  nsObjectInit(NetStream *sp);
  void
  nsStart(NetStream *sp, struct netconn * conn);
#ifdef __cplusplus
}
#endif

#endif /* NETSTREAM_H_ */

