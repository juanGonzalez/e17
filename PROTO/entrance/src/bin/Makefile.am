
MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I$(top_srcdir)/src/bin \
-I$(top_builddir)/src/bin \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
-DSYSTEM_CONFIG_DIR=\"$(sysconfdir)\" \
@ENTRANCE_CLIENT_CFLAGS@

internal_bindir=$(libdir)/entrance
internal_bin_PROGRAMS = entrance_client

entrance_client_SOURCES = \
../event/entrance_event.h \
../event/entrance_event.c \
entrance_client.h         \
entrance_client.c         \
entrance_connect.h        \
entrance_connect.c        \
entrance_fill.h           \
entrance_fill.c           \
entrance_gui.h            \
entrance_gui.c


entrance_client_LDFLAGS =
entrance_client_LDADD = @ENTRANCE_CLIENT_LIBS@

