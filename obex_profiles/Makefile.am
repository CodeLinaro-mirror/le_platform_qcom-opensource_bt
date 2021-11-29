AM_CPPFLAGS = -O2 \
              -D_GNU_SOURCE \
              -DOI_DEBUG
AM_CFLAGS = -Wall \
        -Wundef \
        -Wstrict-prototypes \
        -Wno-trigraphs \
        -DLINUX \
        -DOI_CPU_TYPE=19 \
        -DOI_DEBUG \
        -DOI_USE_NATIVE_MALLOC \
        -DOI_USE_NATIVE_MEMCPY \
        -DOI_OBEX_OVER_L2CAP \
        -I${WORKSPACE}/system/core/include/ \
        -I$(top_srcdir)/include \
        -I$(top_srcdir)/include/profiles \
        -I$(top_srcdir)/sdk/include \
        -I$(top_srcdir)/sdk/include/profiles \
        -I$(top_srcdir)/sdk/oem \
        -I$(top_srcdir)/platform/linux/include

ACLOCAL_AMFLAGS = -I m4

c_sources = profiles/obex/obexauth.c \
            profiles/obex/obexcommon.c \
            profiles/obex/obexcli.c \
            profiles/obex/obexsrv.c \
            profiles/obex/obex_lower.c \
            profiles/obex/obex_lower_socket.c \
            profiles/obex/obextest.c \
            profiles/pbap/pbap_client.c \
            profiles/pbap/pbap_server.c \
            profiles/pbap/pbap_private.c \
            profiles/opp/opp_client.c \
            profiles/opp/opp_server.c \
            profiles/opp/opp_common.c \
            stack/support/md5_hash.c \
            stack/support/memprof.c \
            stack/support/oi_handle.c \
            stack/support/oi_mbuf.c \
            stack/support/oi_time.c \
            stack/support/oi_list.c \
            stack/support/oi_init_flags.c \
            stack/support/oi_config_table.c \
            stack/support/oi_obextext.c \
            stack/support/oi_statustext.c \
            stack/support/oi_unicode.c \
            stack/support/oi_memmgr.c \
            stack/support/oi_debug.c \
            stack/support/oi_dispatch.c \
            stack/support/oi_fcs.c \
            stack/support/oi_varstring.c \
            stack/support/oi_utils.c \
            stack/support/oi_simplemap.c \
            stack/support/bt_assigned_nos.c \
            stack/dataelem/dataelem.c \
            stack/dataelem/dataelem_text.c \
            platform/linux/src/oi_osinterface.c \
            platform/linux/src/oi_assert.c \
            platform/linux/src/oi_wrapper.c \
            platform/linux/src/oi_thread.c \
            platform/linux/src/sockets/oi_eventloop.c \
            sdk/oem/oi_bt_stack_init_default.c \
            sdk/oem/oi_bt_profile_config_default.c \
            sdk/oem/oi_bt_stack_config_default.c \
            unicode/ConvertUTF.c

AM_LDFLAGS := $(LDFLAFGS)

library_includedir = $(pkgincludedir)
lib_LTLIBRARIES = libbtobex.la
libbtobex_la_CC = @CC@
libbtobex_la_SOURCES = $(c_sources)
libbtobex_la_CPPFLAGS = $(AM_CPPFLAGS)
libbtobex_la_CFLAGS = $(AM_CFLAGS)
libbtobex_la_LDFLAGS = $(AM_LDFLAGS) -static -lpthread -lm -ldl -lrt

#export headers
obex_includedir = $(includedir)/
obex_include_HEADERS = $(top_srcdir)/sdk/include/profiles/oi_obex.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_bt_assigned_nos.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_stddefs.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_cpu_dep.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_l2cap.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_handle.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_endian.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_dump.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_debugcontrol.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_debug.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_dataelem.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_crc16_ccitt.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_connect_policy.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_config_table.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_common.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_bt_stack_init.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_bt_stack_config.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_bt_spec.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_bt_profile_config.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_bt_module_init.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_bt.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_assert.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_argcheck.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_sdpdb.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_sdp.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_rfcomm_prefs.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_rfcomm.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_osinterface.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_modules.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_mbuf.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_list.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_l2cap_qos.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_l2cap_prefs.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_time.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_thread.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_text.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_test.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_support_init.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_string.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_statustext.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_status.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_simplemap.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_utils.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_unicode.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/oi_xml.h
obex_include_HEADERS += $(top_srcdir)/include/oi_memmgr.h
obex_include_HEADERS += $(top_srcdir)/profiles/obex/obex_lower.c
obex_include_HEADERS += $(top_srcdir)/profiles/obex/oi_obex_lower.h
obex_include_HEADERS += $(top_srcdir)/profiles/obex/obextest.c
obex_include_HEADERS += $(top_srcdir)/profiles/obex/obexsrv.c
obex_include_HEADERS += $(top_srcdir)/profiles/obex/obexcommon.c
obex_include_HEADERS += $(top_srcdir)/profiles/obex/obexcli.c
obex_include_HEADERS += $(top_srcdir)/profiles/obex/obexauth.c
obex_include_HEADERS += $(top_srcdir)/profiles/obex/obex_lower_socket.c
obex_include_HEADERS += $(top_srcdir)/profiles/obex/oi_obexauth.h
obex_include_HEADERS += $(top_srcdir)/profiles/obex/oi_obexcommon.h
obex_include_HEADERS += $(top_srcdir)/sdk/oem/oi_wrapper.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_bip_cli.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_bip_spec.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_bip_srv.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_bpp.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_bpp_printer.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_bpp_sender.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_ftp_client.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_ftp_file.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_ftp_server.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_ftp_sys.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_obex_sd.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_obexcli.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_obexspec.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_obexsrv.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_opp_client.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_opp_ofs.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_opp_server.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_opp_sys.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_pbap_client.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_pbap_consts.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_pbap_server.h
obex_include_HEADERS += $(top_srcdir)/sdk/include/profiles/oi_pbap_sys.h

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = obex.pc
EXTRA_DIST     = $(pkgconfig_DATA)
