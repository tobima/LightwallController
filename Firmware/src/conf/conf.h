#include "lwipthread.h"

typedef struct
{
  struct lwipthread_opts network;
} configuration_t;

int
conf_load(configuration_t* config);
