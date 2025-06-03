ACLOCAL_AMFLAGS = -I m4

root_etcdir  = "/etc"

bluetooth_sysconfdir = $(root_etcdir)/bluetooth
bluetooth_sysconf_DATA  = $(top_srcdir)/conf/bt_app.conf
bluetooth_sysconf_DATA += $(top_srcdir)/conf/ext_to_mimetype.conf

install-data-hook:
	chmod 0660 $(DESTDIR)/$(bluetooth_sysconfdir)/bt_app.conf
	chmod 0660 $(DESTDIR)/$(bluetooth_sysconfdir)/ext_to_mimetype.conf

AM_CPPFLAGS = -DUSE_ANDROID_LOGGING -DUSE_BT_OBEX -DUSE_LIBHW_AOSP -DUSE_GEN_GATT
common_LDFLAGS = -llog

SUBDIRS = osi utils gatt gap main
