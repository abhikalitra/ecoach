bin_PROGRAMS = ecoach
ecoach_SOURCES =			\
	calculate_bmi.h			\
	calculate_bmi.c			\
	main.c				\
	activity.h			\
	activity.c			\
	activity_tree.h			\
	activity_tree.c			\
	analyzer.h			\
	analyzer.c			\
	beat_detect.h			\
	beat_detect.c			\
	dbus_helper.h			\
	dbus_helper.c			\
	ec_error.h			\
	ec_error.c			\
	ec-progress.h			\
	ec-progress-private.h		\
	ec-progress.c			\
	ec-button.h			\
	ec-button-private.h		\
	ec-button.c			\
	ecg_data.h			\
	ecg_data.c			\
	gconf_helper.h			\
	gconf_helper.c			\
	gpx.h				\
	gpx.c				\
	gpx_parser.h			\
	gpx_parser.c			\
	heart_rate_settings.h		\
	heart_rate_settings.c		\
	hrm_shared.h			\
	hrm_shared.c			\
	hrm_settings.h			\
	hrm_settings.c			\
	interface.h			\
	interface.c			\
	navigation_menu_priv.h		\
	navigation_menu.h		\
	navigation_menu.c		\
	navigation_menu_item.h		\
	navigation_menu_item.c		\
	map_view.h			\
	map_view.c			\
	marshal.h			\
	marshal.c			\
	settings.h			\
	settings.c			\
	target_heart_rate.h		\
	target_heart_rate.c		\
	track.h				\
	track.c				\
	util.h				\
	util.c				\
	xml_util.h			\
	xml_util.c			\
	osea/analbeat.h			\
	osea/analbeat.c			\
	osea/bdac.h			\
	osea/bdac.c			\
	osea/classify.c			\
	osea/ecgcodes.h			\
	osea/match.h			\
	osea/match.c			\
	osea/noisechk.c			\
	osea/postclas.h			\
	osea/postclas.c			\
	osea/qrsdet.h			\
	osea/qrsdet.c			\
	osea/qrsfilt.c			\
	osea/rythmchk.h			\
	osea/rythmchk.c			\
	map_widget/map_widget.h		\
	map_widget/map_widget_defs.h	\
	map_widget/map_widget.c

if WANT_ECG_VIEW
ecoach_SOURCES += ecg_view.h ecg_view.c
endif

BUILT_SOURCES =				\
	marshal.h			\
	marshal.c

marshal.h: marshal.list
	$(GLIB_GENMARSHAL) --prefix=marshal marshal.list --header > marshal.h

marshal.c: marshal.list
	$(GLIB_GENMARSHAL) --prefix=marshal marshal.list --body > marshal.c

AM_CPPFLAGS =				\
	@GTHREAD_CFLAGS@		\
	@HILDON_CFLAGS@			\
	@HILDONFM_CFLAGS@		\
	@GTK_CFLAGS@			\
	@BLUEZ_CFLAGS@			\
	@DBUS_CFLAGS@			\
	@GCONF_CFLAGS@			\
	@OSSO_CFLAGS@			\
	@LOCATION_CFLAGS@		\
	@LIBXML2_CFLAGS@		\
	-DDATADIR=\"/usr/share/\"	\
	-Wall

LIBS =					\
	@GTHREAD_LIBS@			\
	@HILDON_LIBS@			\
	@HILDONFM_LIBS@			\
	@GTK_LIBS@			\
	@BLUEZ_LIBS@			\
	@DBUS_LIBS@			\
	@GCONF_LIBS@			\
	@OSSO_LIBS@			\
	@LOCATION_LIBS@			\
	@LIBXML2_LIBS@
