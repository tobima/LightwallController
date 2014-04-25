# List of all the board related files.
APPSRC = src/web/web.c \
       src/dmx/dmx.c \
       src/dmx/dmx_cmd.c \
       src/dmx/rgb.c \
       src/cmd/cmd_threads.c \
       src/cmd/cmd_mem.c \
       src/main.c 

# Required include directories
APPINC = ${APP} \
       src/fullcircle

# List all user C define here
APPDEFS = -DSHELL_MAX_ARGUMENTS=5

# Fullcricle (fc_c) specific:
# Debugging for the underling library
# APPDEFS += -DPRINT_DEBUG

#APPDEFS += -DWITH_TELNET
APPDEFS += -DFILESYSTEM_ONLY

