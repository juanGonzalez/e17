
MAINTAINERCLEANFILES = Makefile.in

lib_LTLIBRARIES = libevil.la libdl.la

install_evilheadersdir = $(includedir)/evil-@VMAJ@
dist_install_evilheaders_DATA = \
Evil.h \
evil_fcntl.h \
evil_inet.h \
evil_langinfo.h \
evil_libgen.h \
evil_macro.h \
evil_macro_pop.h \
evil_main.h \
evil_print.h \
evil_stdlib.h \
evil_stdio.h \
evil_string.h \
evil_time.h \
evil_unistd.h \
evil_util.h

stdheadersdir = $(includedir)/evil-@VMAJ@
nobase_dist_stdheaders_DATA = pwd.h sys/mman.h fnmatch.h dirent.h dlfcn.h

if HAVE_WINCE

nobase_dist_stdheaders_DATA += mingw32ce/errno.h

endif

# gdtoa
libevil_la_SOURCES = \
gdtoa/arithchk.c \
gdtoa/dmisc.c \
gdtoa/dtoa.c \
gdtoa/gd_arith.h \
gdtoa/g_dfmt.c \
gdtoa/gd_qnan.h \
gdtoa/gdtoa.c \
gdtoa/gdtoa_fltrnds.h \
gdtoa/gdtoa.h \
gdtoa/gdtoaimp.h \
gdtoa/gethex.c \
gdtoa/g_ffmt.c \
gdtoa/g__fmt.c \
gdtoa/gmisc.c \
gdtoa/g_xfmt.c \
gdtoa/hd_init.c \
gdtoa/hexnan.c \
gdtoa/misc.c \
gdtoa/qnan.c \
gdtoa/smisc.c \
gdtoa/strtodg.c \
gdtoa/strtof.c \
gdtoa/strtopx.c \
gdtoa/sum.c \
gdtoa/ulp.c

#evil
libevil_la_SOURCES += \
evil_dirent.c \
evil_fcntl.c \
evil_fnmatch.c \
evil_fnmatch_list_of_states.c \
evil_inet.c \
evil_langinfo.c \
evil_libgen.c \
evil_main.c \
evil_mman.c \
evil_pformata.c \
evil_pformatw.c \
evil_printa.c \
evil_printw.c \
evil_pwd.c \
evil_stdlib.c \
evil_stdio.c \
evil_string.c \
evil_time.c \
evil_unistd.c \
evil_util.c \
evil_uuid.c \
evil_pformat.h \
evil_private.h \
evil_fnmatch_private.h

if HAVE_WINCE

libevil_la_SOURCES += evil_errno.c evil_link_ce.c

else

libevil_la_SOURCES += evil_link_xp.cpp

endif

libevil_la_CPPFLAGS = @EVIL_CPPFLAGS@
libevil_la_CFLAGS = @EVIL_CFLAGS@
libevil_la_CXXFLAGS = @EVIL_CXXFLAGS@
libevil_la_LIBADD = @EVIL_LIBS@
libevil_la_LDFLAGS = -no-undefined -Wl,--enable-auto-import -version-info @version_info@

if HAVE_WINCE

libevil_la_LINK = $(LINK) $(libevil_la_LDFLAGS)

else

libevil_la_LINK = $(CXXLINK) $(libevil_la_LDFLAGS)

endif

libdl_la_SOURCES = dlfcn.c

libdl_la_CPPFLAGS = @EVIL_DLFCN_CPPFLAGS@
libdl_la_CFLAGS = @EVIL_CFLAGS@
libdl_la_LIBADD = $(top_builddir)/src/lib/evil/libevil.la @EVIL_DLFCN_LIBS@
libdl_la_LDFLAGS = -no-undefined -Wl,--enable-auto-import -version-info @version_info@

EXTRA_DIST = gdtoa/README gdtoa/README.mingw

install-data-hook:
	rm -f $(libdir)/libevil.la $(libdir)/libdl.la

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/libevil-@VMAJ@.dll
	rm -f $(DESTDIR)$(bindir)/libdl-@VMAJ@.dll
	rm -f $(DESTDIR)$(libdir)/libevil.dll.a
	rm -f $(DESTDIR)$(libdir)/libevil.a
	rm -f $(DESTDIR)$(libdir)/libdl.dll.a
	rm -f $(DESTDIR)$(libdir)/libdl.a
