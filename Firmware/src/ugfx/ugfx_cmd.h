/*
 * @file ugfx_cmd.h
 *
 *  Created on: Apr 22, 2014
 *  Author: Ollo
 */

#ifndef UGFX_CMD_H_
#define UGFX_CMD_H_

#include "gfx.h"
#include "chprintf.h"

/** @fn void ugfx_cmd_calibrate(void)
 *
 */
void ugfx_cmd_calibrate(SerialUSBDriver* pSDU1);

void ugfx_cmd_cfgsave(uint16_t instance, const uint8_t *calbuf, size_t size);

const char *ugfx_cmd_cfgload(uint16_t instance);

#endif /* UGFX_CMD_H_ */
