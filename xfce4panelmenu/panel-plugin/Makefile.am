plugindir = @XFCE4_PANEL_PLUGINSDIR@

plugin_LTLIBRARIES =							\
	libmenustart.la

libmenustart_la_LDFLAGS =							\
	-avoid-version							\
	-export-dynamic							\
	-module

INCLUDES = \
	$(GTK_CFLAGS)\
	 -I/usr/include/libxml2 -I/usr/include/xfce4/

libmenustart_la_SOURCES =							\
	main.c									\
	menu.c  \
	fstab.c \
	menustart.c

libmenustart_la_CFLAGS =							\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"					\
	-DICONDIR=\"$(datadir)/xfce4/menustart\"				\
	@XFCE4_PANEL_CFLAGS@ 

libmenustart_la_LIBADD = 							\
	@XFCE4_PANEL_LIBS@ \
	$(GTK_LIBS)								\
	 -lxml2 -lxfce4mcs-client -lxfce4mcs-manager -lxfce4util -lxfcegui4

