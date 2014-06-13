/*
 * @file ugfx_cmd.h
 *
 *  Created on: Apr 22, 2014
 *  Author: Ollo
 */

#ifndef UGFX_CMD_H_
#define UGFX_CMD_H_

#include "gfx.h"

#include "ch.h"
#include "hal.h"

#include "chprintf.h"

#define UGFX_CMD_MANUAL_START   1
#define UGFX_CMD_MANUAL_ENDED   2


#define PRINT( ... )	chprintf((BaseSequentialStream *) &SD6, __VA_ARGS__);


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


/** @fn void ugfx_cmd_manualtesting_init(void)
 * @brief start the manual tests
 *
 * The DMX buffer is displayed on the LCD and can be modified with the touchscreen.
 */
void ugfx_cmd_manualtesting_init(void);

/** @fn void ugfx_cmd_manualtesting_stop(void)
 *
 */
void ugfx_cmd_manualtesting_stop(void);

#endif /* UGFX_CMD_H_ */
