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
#define WIN_MENU_TOPMARGIN      10

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
static GHandle   ghButtonCalibrate = NULL;

/* The handles for our two Windows */
GHandle gGWdefault = NULL;
static GHandle GWmenu = NULL;

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

static void createMenuWindow(void)
{
  GWindowInit     wi;
  GWidgetInit     widgi;

  /* Create the window for the menu */
  gwinSetDefaultStyle( &WhiteWidgetStyle, FALSE);
  wi.show = TRUE;
  if (wallWidth > 0 && wallHeight > 0)
  { /* calculate dimension for overlay window, that fits between boxes*/
    wi.x = (((wallWidth / 4) + 1) * (boxWidth + 2));
    wi.width = ((wallWidth / 2) * boxWidth);
    wi.y = WIN_MENU_TOPMARGIN;
    wi.height = (wallHeight * boxHeight) - WIN_MENU_TOPMARGIN;
  } else { /* use default offsets */
    wi.width = 150;
    wi.height = 200;
    wi.x = (int) ((gdispGetWidth() - wi.width) / 2);
    wi.y = 10;
  }
  GWmenu = gwinWindowCreate(0, &wi);
  /*FIXME debug code */
  gwinSetColor(gGWdefault, Purple);
  gwinFillCircle(GWmenu, 10, 60, 15);

  // Apply the button parameters
  widgi.customDraw = 0;
  widgi.customParam = 0;
  widgi.customStyle = 0;
  widgi.g.show = TRUE;
  widgi.g.width = 60;
  widgi.g.height = 15;
  widgi.g.y = 2;
  widgi.g.x = 2;
  widgi.text = "Calibrate";

  // Create the actual button
  ghButtonCalibrate = gwinButtonCreate(0, &widgi);
}

/******************************************************************************
 * EXTERN FUNCTIONS
 ******************************************************************************/

void setBox(int x, int y, uint8_t red, uint8_t green, uint8_t blue)
{
	int hexCol;
	int xBox = x*(boxWidth+1);
	int yBox = y*(boxHeight+1);


	if (gGWdefault == NULL)
	{
	    return; /* The display is not initialized */
	}

	/* some magic calculation, that there are no boxes drawn, where the menu window is shown */
	if (gwinGetVisible(GWmenu))
	{
	      if ((gwinGetScreenX(GWmenu) < xBox || gwinGetScreenX(GWmenu) < (xBox + boxWidth) )
	          && ((gwinGetScreenX(GWmenu) + gwinGetWidth(GWmenu)) > xBox || (gwinGetScreenX(GWmenu) + gwinGetWidth(GWmenu)) > (xBox + boxWidth) ))
	      {
	        return; /* Stooop, there is a window to be shown */
	      }
	}

	/* same orienatation as the pyhsical wall: */
	y = (wallHeight - 1) - y;

	/* Calculate the wall */
	hexCol = red << 16 | green << 8 | blue;

	color_t col = HTML2COLOR(hexCol);
	gwinSetColor(gGWdefault, col);
        gwinFillArea(gGWdefault, xBox, yBox, boxWidth, boxHeight);
        gwinSetColor(gGWdefault, Yellow);
        gwinDrawBox (gGWdefault, xBox, yBox, boxWidth, boxHeight);
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
  GWindowInit     wi;

  // Set the widget defaults
  gwinSetDefaultFont( gdispOpenFont("UI2"));
  gwinSetDefaultStyle( &BlackWidgetStyle, FALSE);
  /*gwinSetDefaultBgColor(gGWdefault, Black);
  gwinSetDefaultColor(gGWdefault, Black);*/
  gdispClear(Black);

  wi.show = TRUE;
  wi.x = 0;
  wi.y = 0;
  wi.width = gdispGetWidth();
  wi.height = gdispGetHeight();
  gGWdefault = gwinWindowCreate(0, &wi);


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
                    if (!GWmenu)
                    {
                        createMenuWindow();
                    }

                    /* toggle visibility */
                    if (gwinGetVisible(GWmenu))
                    {
                        gwinSetVisible(GWmenu, FALSE);
                    }
                    else
                    {
                        gwinSetVisible(GWmenu, TRUE);
                    }

                    if (pSDU1)
                    {
                        chprintf((BaseSequentialStream *) pSDU1, "Button clicked, window %d\r\n", gwinGetVisible(GWmenu));
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
