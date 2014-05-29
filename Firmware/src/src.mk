# List of all the board related files.
APPSRC = src/main.c \
       src/dmx/dmx.c \
       src/dmx/dmx_cmd.c \
       src/dmx/rgb.c \
       src/cmd/cmd_threads.c \
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
APPDEFS = -DSHELL_MAX_ARGUMENTS=6

# Fullcricle (fc_c) specific:
# Debugging for the underling library

#APPDEFS += -DPRINT_DEBUG


# Append the WALL
APPSRC += src/ugfx/fcwall.c \
		  src/ugfx/ugfx_util.c \
		  src/ugfx/ugfx_cmd.c
		  		  
APPINC += src/ugfx
APPDEFS += -DUGFX_WALL

APPDEFS += -DFILESYSTEM_ONLY
