AM_CPPFLAGS = \
	-I$(top_srcdir)/src/libstrongswan \
	-I$(top_srcdir)/src/libtncif \
	-I$(top_srcdir)/src/libtpmtss \
	-I$(top_srcdir)/src/libimcv

AM_CFLAGS = \
	$(PLUGIN_CFLAGS) $(json_CFLAGS)

imcv_LTLIBRARIES = imv-swid.la

imv_swid_la_LIBADD = \
	$(top_builddir)/src/libimcv/libimcv.la \
	$(top_builddir)/src/libstrongswan/libstrongswan.la \
	$(json_LIBS)

imv_swid_la_SOURCES = \
	imv_swid.c imv_swid_state.h imv_swid_state.c \
	imv_swid_agent.h imv_swid_agent.c

imv_swid_la_LDFLAGS = -module -avoid-version -no-undefined
