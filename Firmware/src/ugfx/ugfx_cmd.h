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

#define UGFX_CMD_MANUAL_START   1
#define UGFX_CMD_MANUAL_ENDED   2

/** @fn void ugfx_cmd_calibrate(void)
 *
 */
void ugfx_cmd_calibrate(SerialUSBDriver* pSDU1);

/** @fn void ugfx_cmd_manualtesting(uint8_t status)
 * @brief Logic when the manual tests are started or stopped
 * @param[in] status    @see{UGFX_CMD_MANUAL_START} the testing is started;
 *                      @see{UGFX_CMD_MANUAL_ENDED} the scheduler should be running again
 */
void ugfx_cmd_manualtesting(uint8_t status);

void ugfx_cmd_cfgsave(uint16_t instance, const uint8_t *calbuf, size_t size);

const char *ugfx_cmd_cfgload(uint16_t instance);

/**
 * @fn void ugfx_cmd_shell(BaseSequentialStream *chp, int argc, char *argv[])
 * @brief commands can also be triggered via the shell
 * @param[in|out]	chp	IO-stream
 * @param[in]		argc	amount of given arguments
 * @param[in]		argv	arguments themselves
 */
void ugfx_cmd_shell(BaseSequentialStream *chp, int argc, char *argv[]);

#endif /* UGFX_CMD_H_ */
