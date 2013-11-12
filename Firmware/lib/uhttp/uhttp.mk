UHTTP_PWD = $(APP_LIBS)/uhttp

LIBINC +=  $(UHTTP_PWD)/include 

LIBSRC += $(UHTTP_PWD)/src/envinit.c \
	  $(UHTTP_PWD)/src/envreg.c \
	  $(UHTTP_PWD)/src/envvars.c \
	  $(UHTTP_PWD)/src/mediatypes.c \
	  $(UHTTP_PWD)/src/mtinit.c \
	  $(UHTTP_PWD)/src/mtreg.c \
	  $(UHTTP_PWD)/src/responses.c \
	  $(UHTTP_PWD)/src/uhttpd.c \
	  $(UHTTP_PWD)/src/utils.c \
	  $(UHTTP_PWD)/src/modules/mod_auth_basic.c \
	  $(UHTTP_PWD)/src/modules/mod_cgi_func.c \
	  $(UHTTP_PWD)/src/modules/mod_redir.c \
	  $(UHTTP_PWD)/src/modules/mod_ssi.c \
	  $(UHTTP_PWD)/src/os/chibios/streamio.c



