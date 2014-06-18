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
#include "ff.h"
#include "ugfx_cmd.h"

#define INFO_TEXT_HEIGHT        25
#define WIN_MENU_TOPMARGIN      10

#define MANUALTEST_TXT_START    "Start manual tests"
#define MANUALTEST_TXT_ENDED    "Stop manual testing"

#define UNSELECTED				-1	/**< Value, describing the selected box value as invalid */

#define IF_MENU_VISIBLE         if (GWmenu != NULL && gwinGetVisible(GWmenu))

#define FCWALL_USBPRINT( ... )    if (pSDU1) { chprintf((BaseSequentialStream *) pSDU1,  __VA_ARGS__); }
#define FCWALL_UARTPRINT( ... )    chprintf((BaseSequentialStream *) &SD6, __VA_ARGS__);

/******************************************************************************
 * GLOBAL VARIABLES of this module
 ******************************************************************************/

/* The handles for our two Windows */
GHandle gGWdefault = NULL;

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
static GHandle   ghButtonManualTesting = NULL;

/* The handles for our two Windows */
static GHandle GWmenu = NULL;

static uint8_t stopUIUpdate = FALSE;

static uint8_t gManualStatus = 0;

static int gSelectedX = UNSELECTED;
static int gSelectedY = UNSELECTED;

/******************************************************************************
 * LOCAL FUNCTIONS
 ******************************************************************************/

static void createWidgets(void) {
        GWidgetInit     wi;

        // Apply some default values for GWIN
        wi.customDraw = 0;
        wi.customParam = 0;
        wi.customStyle = 0;
        wi.g.show = TRUE;

        // Apply the menu button parameters
        wi.g.width = MENU_BUTTON_WIDTH - 4;
        wi.g.height = INFO_TEXT_HEIGHT - 2;
        wi.g.y = gdispGetHeight() - (INFO_TEXT_HEIGHT + 4);
        wi.g.x = 2;
        wi.text = "Menu";

        gwinClear(gGWdefault);
        // Create the actual button
        ghButton1 = gwinButtonCreate(0, &wi);
}

static void createMenuWindow(void)
{
  GWindowInit     wi;
  GWidgetInit     widgi;

  /* Create the window for the menu */
  gwinSetDefaultStyle( &BlackWidgetStyle, FALSE);
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
  gwinClear(GWmenu);

  /* Apply the button parameters */
  widgi.customDraw = 0;
  widgi.customParam = 0;
  widgi.customStyle = 0;
  widgi.g.width = 125;
  widgi.g.height = 20;
  widgi.g.y = wi.y + 10;
  widgi.g.x = wi.x + 10;
  widgi.g.show = TRUE;
  widgi.text = "Calibrate screen";

  /* Create the first button */
  ghButtonCalibrate = gwinButtonCreate(0, &widgi);

  /* Second button */
  widgi.g.y = wi.y + 40;
  widgi.g.x = wi.x + 10;
  switch (gManualStatus)
  {
  case 0:
    widgi.text = MANUALTEST_TXT_START;
    break;
  case 1:
  default:
    widgi.text = MANUALTEST_TXT_ENDED;
  }
  ghButtonManualTesting = gwinButtonCreate(0, &widgi);

}

static void deleteMenuWindow(void)
{
  gwinSetVisible(GWmenu, FALSE);
  /* clear the window away */
  gwinSetColor(gGWdefault, Black);
  gwinDrawBox (gGWdefault, gwinGetScreenX(GWmenu), gwinGetScreenY(GWmenu),
      gwinGetWidth(GWmenu), gwinGetHeight(GWmenu));

  gwinDestroy(ghButtonCalibrate);
  ghButtonCalibrate = NULL;
  gwinDestroy(GWmenu);
  GWmenu = NULL;
}

static void selectBox(int mouseX, int mouseY)
{
	gSelectedX = (int) (mouseX / boxWidth);
	gSelectedY = wallHeight - ((int) (mouseY / boxHeight));
}

/******************************************************************************
 * EXTERN FUNCTIONS
 ******************************************************************************/

void setBox(int x, int y, uint8_t red, uint8_t green, uint8_t blue)
{
	int hexCol;
	int xBox = x*(boxWidth+1);
	int yBox = y*(boxHeight+1);


	if (gGWdefault == NULL || stopUIUpdate)
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
	if (gSelectedX == x && gSelectedY == y)
	{
		gwinSetColor(gGWdefault, Red); /* draw a red box around the selected one */
	}
	else
	{
		gwinSetColor(gGWdefault, Yellow); /* normally the box has a yellow border */
	}
	gwinDrawBox (gGWdefault, xBox, yBox, boxWidth, boxHeight);
}

void fcwall_init(int w, int h)
{
	wallWidth = w;
	wallHeight = h;

	boxWidth = ((int) (gdispGetWidth() / w))-1;
	boxHeight = ((int) ((gdispGetHeight() - INFO_TEXT_HEIGHT) / h))-1;
}


void fcwall_initWindow(void)
{
  GWindowInit     wi;

  // Set the widget defaults
  gwinSetDefaultFont( gdispOpenFont("UI2"));
  gwinSetDefaultStyle( &BlackWidgetStyle, FALSE);
  gdispClear(Black);

  wi.show = TRUE;
  wi.x = 0;
  wi.y = 0;
  wi.width = gdispGetWidth();
  wi.height = gdispGetHeight();
  gGWdefault = gwinWindowCreate(0, &wi);

  /* automatically load a configuration at startup */
  ginputSetMouseCalibrationRoutines(0, ugfx_cmd_cfgsave, ugfx_cmd_cfgload, FALSE);

  // Attach the mouse input
  gwinAttachMouse(0);

  createWidgets();

  // We want to listen for widget events
  geventListenerInit(&gl);
  gwinAttachListener(&gl);
}



void fcwall_processEvents(SerialUSBDriver* pSDU1)
{
  GEvent* 		pe;
  GSourceHandle	mouse;
  GEventMouse	*pem;
  // Get an Event
  pe = geventEventWait(&gl, TIME_INFINITE);

  switch(pe->type)
  {
          case GEVENT_GWIN_BUTTON:
                  if  (((GEventGWinButton*)pe)->button == ghButton1)
                  {
                    /* toggle visibility */
                    IF_MENU_VISIBLE
                    {
                        deleteMenuWindow();
                    }
                    else
                    {
                        createMenuWindow();
                        gwinSetVisible(GWmenu, TRUE);
                    }
                  }
                  else if  (((GEventGWinButton*)pe)->button == ghButtonCalibrate)
                  {
                      deleteMenuWindow();
                      stopUIUpdate = TRUE;
                      ugfx_cmd_calibrate(pSDU1);
                      stopUIUpdate = FALSE;
                  }
                  else if  (((GEventGWinButton*)pe)->button == ghButtonManualTesting)
                  {
                      /* do only something if window is visible */
                      IF_MENU_VISIBLE
                      {
                          deleteMenuWindow();

                          /*** add mouse listener to get the coordinates / box ***/
                          /* Initialise the first mouse/touch and get its handle */
                          mouse = ginputGetMouse(0);
                          /* we want to listen for mouse/touch events */
                          geventAttachSource(&gl, mouse, GLISTEN_MOUSEDOWNMOVES | GLISTEN_MOUSEMETA);

                          gManualStatus = !gManualStatus;
                          if (gManualStatus)
                          {
                              ugfx_cmd_manualtesting(UGFX_CMD_MANUAL_START);
                          }
                          else
                          {
                              ugfx_cmd_manualtesting(UGFX_CMD_MANUAL_ENDED);
                          }
                      }
                  }
                  else
                  {
                      FCWALL_USBPRINT("Other button clicked, window %X\r\n", ((GEventGWinButton*)pe)->button);
                  }
                  break;
          case GEVENT_TOUCH:

        	  /* convert event into a MouseEvent */
			  pem = (GEventMouse *) pe;
			  if ((pem->current_buttons & GINPUT_MOUSE_BTN_LEFT))
			  {
				gdispDrawPixel(pem->x, pem->y, Green);
				selectBox(pem->x, pem->y);
			  }
        	  break;
          default:
            FCWALL_UARTPRINT("Input Event %X\r\n", pe->type);
            FCWALL_USBPRINT("Input Event %X\r\n", pe->type);
            break;
  }
}
