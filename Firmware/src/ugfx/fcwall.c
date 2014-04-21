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

static GListener gl;
static GHandle   ghButton1;
/******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

/******************************************************************************
 * EXTERN FUNCTIONS
 ******************************************************************************/

void setBox(int x, int y, uint8_t red, uint8_t green, uint8_t blue)
{
	int hexCol;

	/* same orienatation as the pyhsical wall: */
	y = (wallHeight - 1) - y;

	/* Calculate the wall */
	hexCol = red << 16 | green << 8 | blue;

	color_t col = HTML2COLOR(hexCol);

        gdispFillArea(x*(boxWidth+1), y*(boxHeight+1), boxWidth, boxHeight, col);
        gdispDrawBox (x*(boxWidth+1), y*(boxHeight+1), boxWidth, boxHeight, Yellow);
}

void fcwall_init(int w, int h)
{
	coord_t width, height;
	int x = 0;
	int y = 0;
	wallWidth = w;
	wallHeight = h;

	width = gdispGetWidth();
	height = gdispGetHeight();

	boxWidth = ((int) (width / w))-1;
	boxHeight = ((int) ((height-25) / h))-1;

	for(x=0; x < w; x++)
	{
		for(y=0; y < h; y++)
		{
			setBox(x,y, 0,0,0);
		}
	}
}

static void createWidgets(void) {
        GWidgetInit     wi;

        // Apply some default values for GWIN
        wi.customDraw = 0;
        wi.customParam = 0;
        wi.customStyle = 0;
        wi.g.show = TRUE;

        // Apply the button parameters
        wi.g.width = 100;
        wi.g.height = 30;
        wi.g.y = 10;
        wi.g.x = 10;
        wi.text = "Push Button";

        // Create the actual button
        ghButton1 = gwinButtonCreate(0, &wi);
}


void fcwall_initWindow(void)
{
  // Set the widget defaults
  gwinSetDefaultFont(gdispOpenFont("UI2"));
  gwinSetDefaultStyle(&WhiteWidgetStyle, FALSE);
  gdispClear(Blue);

  // Attach the mouse input
  gwinAttachMouse(0);

  // create the widget
  createWidgets();

  // We want to listen for widget events
  geventListenerInit(&gl);
  gwinAttachListener(&gl);
}

void fcwall_processEvents(void)
{
  GEvent* pe;
  // Get an Event
  pe = geventEventWait(&gl, TIME_INFINITE);

  switch(pe->type) {
          case GEVENT_GWIN_BUTTON:
                  if (((GEventGWinButton*)pe)->button == ghButton1) {
                          // Our button has been pressed
                	      chprintf((BaseSequentialStream *) &SD6, "Button clicked\r\n");
                  }
                  break;

          default:
                  break;
  }

}
