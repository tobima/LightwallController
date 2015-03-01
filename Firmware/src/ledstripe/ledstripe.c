/**
 * @file ledstrip.c
 * @author Manu
 * @author Ollo
 * @author tobi
 *
 * creation date 2014-11-08
 *
 * Inspired by: https://github.com/omriiluz/WS2812B-LED-Driver-ChibiOS/blob/master/LEDDriver.c
 *
 */

#include "ledstripe/ledstripe.h"

ledstripe_color ledstripe_framebuffer[LEDSTRIPE_FRAMEBUFFER_SIZE];
uint16_t ledstripe_pwm_buffer[LEDSTRIPE_PWM_BUFFER_SIZE];
static uint16_t frame_pos = 0;

// writes the pwm values of one byte into the array which will be used by the dma
static inline void color2pwm(uint16_t ** dest, uint8_t color) {
	uint8_t mask = 0x80;
	do {
		if (color & mask) {
			**dest = 49;
		} else {
			**dest = 20;
		}
		*dest += 1;
		mask >>= 1;
	} while (mask != 0);
}

static void Update_Buffer(uint16_t* buffer) {
	static int incomplete_return = 0;
	ledstripe_color *framebufferp;
	uint32_t i, j;
	uint16_t * bufp;

	for (i = 0; i < (LEDSTRIPE_PWM_BUFFER_SIZE / 2) / 24; i++) {
		/* Make the reset (the complete stripe is set) */
		if (incomplete_return) {
			incomplete_return = 0;
			for (j = 0; j < 24; j++) {
				buffer[i * 24 + j] = 0;
			}
		} else {
			if (frame_pos == LEDSTRIPE_FRAMEBUFFER_SIZE) {
				/* Fill the reset by setting everything to zero */
				incomplete_return = 1;
				frame_pos = 0;
				for (j = 0; j < 24; j++) {
					buffer[i * 24 + j] = 0;
				}
			} else {
				framebufferp = &(ledstripe_framebuffer[frame_pos++]);
				bufp = buffer + (i * 24);
// edit here to change order of colors in "ws2812_framebuffer" (0x00RRGGBB, 0x00GGBBRR, etc)
// the chip needs G R B
				color2pwm(&bufp, framebufferp->blue); // blue
				color2pwm(&bufp, framebufferp->green); // green
				color2pwm(&bufp, framebufferp->red); // red
			}
		}
	}
}

static void ledstripe_irq_handler(void* data, uint32_t flags) {
	(void) data;

	// Half-Transfer completed
	if ( flags & (1<<4)) {
		Update_Buffer(ledstripe_pwm_buffer);
	}

	// Transfer completed
	if (flags & (1<<5)) {
		Update_Buffer(ledstripe_pwm_buffer + (LEDSTRIPE_PWM_BUFFER_SIZE / 2));
	}
}

void ledstripe_init(void) {
	int i;

	// Init buffers
	for (i = 0; i < LEDSTRIPE_PWM_BUFFER_SIZE; i++) {
		ledstripe_pwm_buffer[i] = 0;
	}
	for (i = 0; i < LEDSTRIPE_FRAMEBUFFER_SIZE; i++) {
		ledstripe_framebuffer[i].red = 0x0;
		ledstripe_framebuffer[i].green = 0x0;
		ledstripe_framebuffer[i].blue = 0x0;
	}

	// Fill the buffer with values:
	Update_Buffer(ledstripe_pwm_buffer);
	Update_Buffer(ledstripe_pwm_buffer + (LEDSTRIPE_PWM_BUFFER_SIZE / 2));

	/*  GPIO config done in board.h
	 *  AF TIM3; Push
	 */

	// Timer/PWM init
	// enable clock for timer
	rccEnableTIM3(FALSE);
	rccResetTIM3();

	STM32_TIM3->CR1 = 0;
	STM32_TIM3->PSC = 0; // timer prescaler 0
	STM32_TIM3->ARR = 104; // timer period 104
	STM32_TIM3->CR1 = STM32_TIM_CR1_CKD(0) | // set clock CKD 1
			// STM32_TIM_CR1_DIR | // up counting
			STM32_TIM_CR1_ARPE; // ARRPreloadConfig enable
	STM32_TIM3->CCMR2 = STM32_TIM_CCMR2_OC3M(6) | // set PWM mode 1 for channel 3
			STM32_TIM_CCMR2_OC3PE; // OCPreload_Enable
	STM32_TIM3->CCER = STM32_TIM_CCER_CC3E; // OC3 output enable /* Not set: STM32_TIM_CCER_CC3P */

	//DEBUG: Set an default value (as DMA is not working)
	STM32_TIM3->CCR[2] = 0;
	

	/******************** DMA *****************************/
	dmaInit();
	// DMA init
	uint32_t mode = STM32_DMA_CR_CHSEL(5) | /* Select Channel 5 */
			STM32_DMA_CR_PL(2) |  //DMA priority (0..3|lowest..highest)
			STM32_DMA_CR_DIR_M2P |
			STM32_DMA_CR_TCIE |
			STM32_DMA_CR_HTIE |
			STM32_DMA_CR_MINC |
			STM32_DMA_CR_MSIZE_HWORD |
			STM32_DMA_CR_CIRC |
			STM32_DMA_CR_PSIZE_HWORD ;

	/* DMA setup.*/
	dmaStreamAllocate(STM32_DMA1_STREAM7, 10, ledstripe_irq_handler, NULL);
	dmaStreamSetPeripheral(STM32_DMA1_STREAM7, &(STM32_TIM3->CCR[2]) );

	dmaStreamSetMemory0(STM32_DMA1_STREAM7, ledstripe_pwm_buffer);
	dmaStreamSetTransactionSize(STM32_DMA1_STREAM7, LEDSTRIPE_PWM_BUFFER_SIZE);
	dmaStreamSetMode(STM32_DMA1_STREAM7, mode);
	//dmaStreamSetFIFO(STM32_DMA1_STREAM7,  STM32_DMA_FCR_DMDIS | STM32_DMA_FCR_FTH_HALF );
	dmaStreamEnable(STM32_DMA1_STREAM7);

	// DMA init
	STM32_TIM3->DIER = STM32_TIM_DIER_CC3DE; // enable DMA for channel 3

	// Activate it:
	STM32_TIM3->CR1 |= STM32_TIM_CR1_CEN; // TIM3 enable; CR1 is now locked
	STM32_TIM3->EGR |= STM32_TIM_EGR_UG;
}
