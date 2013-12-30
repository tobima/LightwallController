/** @file fcwall.c
 * @brief Dynamic fullcircle simulation of the wall
 * @author Ollo
 *
 * @date 30.12.2013
 * @defgroup LightwallController
 *
 */

#include "fcwall.h"
#include "gfx.h"

/******************************************************************************
 * GLOBAL VARIABLES of this module
 ******************************************************************************/


/******************************************************************************
 * LOCAL VARIABLES for this module
 ******************************************************************************/
static int boxWidth;
static int boxHeight;

static int wallWidth;
static int wallHeight;

/******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

static void setBox(int x, int y, color_t c) {
        gdispFillArea(x*(boxWidth+1), y*(boxHeight+1), boxWidth, boxHeight, c);
        gdispDrawBox (x*(boxWidth+1), y*(boxHeight+1), boxWidth, boxHeight, Yellow);
}

/******************************************************************************
 * EXTERN FUNCTIONS
 ******************************************************************************/

void fcwall_init(int width, int height)
{
	wallWidth = width;
	wallHeight = height;
}

void fcwall_update(void)
{
	int width, height;
	int boxWidth, boxHeight;
	width = gdispGetWidth();
	height = gdispGetHeight();

	static int col = 0xc8c8c8;
	static int x = 0;
	static int y = 0;

	boxWidth = ((int) (width / wallWidth))-1;
	boxHeight = ((int) ((height-25) / wallHeight))-1;

	for(x=0; x < wallWidth; x++)
	{
		for(y=0; y < wallHeight; y++)
		{
			setBox(x,y, Blue);
		}
	}

	x = 0;
	y = 0;
	setBox(x,y, HTML2COLOR(col));
	x++;
	col += 0x000010;
	if (x >= wallWidth)
	{
		x = 0;
		y++;
		col += 0x001000;
	}
	if (y >= wallHeight)
	{
		col += 0x100000;
		x =0;
		y=0;
	}
	gfxSleepMilliseconds(200);

}
