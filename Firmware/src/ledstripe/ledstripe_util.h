/*
 * ledstripe_util.h
 *
 *  Created on: Mar 1, 2015
 *      Author: c3ma
 */

#ifndef LEDSTRIPE_UTIL_H_
#define LEDSTRIPE_UTIL_H_

#include <stdint.h>
#include "ch.h"

#define FCSCHED_WALLCFG_FILE	"fc/conf/stripewall"

void ledstripe_util_Init(void);

void ledstripe_util_button_demo(BaseSequentialStream *chp);

void ledstripe_util_update(uint8_t* rgb24, int width  , int height);

#endif /* LEDSTRIPE_UTIL_H_ */
