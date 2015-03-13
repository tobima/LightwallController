# List of all the board related files.
APPSRC = src/web/web.c \
       src/dmx/dmx.c \
       src/dmx/rgb.c \
       src/dmx/dmx_cmd.c \
	   src/ledstripe/ledstripe.c \
	   src/ledstripe/ledstripe_util.c \
       src/usbcdc/usbcdc.c \
       src/main.c \
       src/fullcircle/fcstatic.c \
       src/netstream/netstream.c \
       src/netshell/netshell.c \
       src/cmd/cmd_threads.c \
       src/cmd/ifconfig.c \
       src/cmd/cmd_mem.c \
       src/cmd/cmd_cat.c \
       src/cmd/cmd_flash.c \
       src/ini/ini.c \
       src/conf/conf.c \
       src/fullcircle/fcserverImpl.c \
       src/fullcircle/fcscheduler.c 

# Required include directories
APPINC = ${APP} \
       src/fullcircle

# List all user C define here
APPDEFS =

# Fullcricle (fc_c) specific:
# Debugging for the underling library
#APPDEFS += -DPRINT_DEBUG

#APPDEFS += -DWITH_TELNET
APPDEFS += -DDISABLE_FILESYSTEM
APPDEFS += -DWS2811_WALL
#APPDEFS += -DDMX_WALL
