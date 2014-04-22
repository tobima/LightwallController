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

#define INFO_TEXT_HEIGHT        25

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
static GHandle   ghButton1 = NULL;

/******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

static void createWidgets(void) {
        GWidgetInit     wi;

        // Generate Black with Black on Black
        GColorSet blackSet = { Black, Black, Black, Black};
        GWidgetStyle black;
        black.background = Black;       // @< The window background color
        black.enabled = blackSet;       // @< The colors when enabled
        black.disabled = blackSet;      // @< The colors when disabled
        black.pressed = blackSet;       // @< The colors when pressed

        // Apply some default values for GWIN
        wi.customDraw = 0;
        wi.customParam = 0;
        wi.customStyle = &black;
        wi.g.show = TRUE;

        // Apply the button parameters
        wi.g.width = gdispGetWidth();
        wi.g.height = gdispGetHeight();
        wi.g.y = 0;
        wi.g.x = 0;
        wi.text = "";

        // Create the actual button
        ghButton1 = gwinButtonCreate(0, &wi);
}

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
	boxHeight = ((int) ((height-INFO_TEXT_HEIGHT) / h))-1;

	for(x=0; x < w; x++)
	{
		for(y=0; y < h; y++)
		{
			setBox(x,y, 0,0,0);
		}
	}
}


void fcwall_initWindow(void)
{

  // Set the widget defaults
  gwinSetDefaultFont(gdispOpenFont("UI2"));
  gwinSetDefaultStyle(&BlackWidgetStyle, FALSE);
  gwinSetDefaultBgColor(Black);
  gwinSetDefaultColor(Black);
  gdispClear(Black);

  // Attach the mouse input
  gwinAttachMouse(0);

  createWidgets();

  // We want to listen for widget events
  geventListenerInit(&gl);
  gwinAttachListener(&gl);
}

void fcwall_processEvents(SerialUSBDriver* pSDU1)
{
  GEvent* pe;
  // Get an Event
  pe = geventEventWait(&gl, TIME_INFINITE);

  switch(pe->type)
  {
          case GEVENT_GWIN_BUTTON:
                  if (ghButton1 != NULL && ((GEventGWinButton*)pe)->button == ghButton1)
                  {
                    // Our button has been pressed
                    chprintf((BaseSequentialStream *) &SD6, "Button clicked\r\n");
                    if (pSDU1)
                    {
                        chprintf((BaseSequentialStream *) pSDU1, "Button clicked\r\n");
                    }
                  }
                  break;

          default:
        	  chprintf((BaseSequentialStream *) &SD6, "Input Event %d\r\n", pe->type);
        	  if (pSDU1)
        	  {
        	      chprintf((BaseSequentialStream *) pSDU1, "Input Event %d\r\n", pe->type);
        	  }
                  break;
  }

}
