dnl Add audacity / vorbis license?
dnl
dnl Please increment the serial number below whenever you alter this macro
dnl for the benefit of automatic macro update systems
# audacity_checklib_libvorbis.m4 serial 4

AC_DEFUN([AUDACITY_CHECKLIB_LIBVORBIS], [
   AC_ARG_WITH(libvorbis,
               [AS_HELP_STRING([--with-libvorbis],
                               [use libvorbis for Ogg Vorbis support])],
               LIBVORBIS_ARGUMENT=$withval,
               LIBVORBIS_ARGUMENT="unspecified")

   dnl See if Vorbis is installed in the system

   PKG_CHECK_MODULES(LIBVORBIS, vorbisenc vorbisfile vorbis ogg,
                     LIBVORBIS_SYSTEM_AVAILABLE="yes",
                     LIBVORBIS_SYSTEM_AVAILABLE="no")
   dnl Have to list the dependent libraries (vorbis and ogg) here because their
   dnl symbols are used from Audacity directly, and (in a minimally linked system,
   dnl e.g. with LDFLAGS including -Wl,--as-needed and with libtool patched to not
   dnl break this) pulling in libraries that use libvorbis does not automagically
   dnl cause the libvorbis symbols to be linkable.

   if test "$LIBVORBIS_SYSTEM_AVAILABLE" = "yes"; then
      AC_MSG_NOTICE([Vorbis libraries are available as system libraries])
   else
      AC_MSG_NOTICE([Vorbis libraries are NOT available as system libraries])
   fi

   dnl see if Vorbis is available in the source dir

   AC_CHECK_FILE(${srcdir}/lib-src/libvorbis/include/vorbis/vorbisenc.h,
                 vorbisenc_h_available="yes",
                 vorbisenc_h_available="no")

   AC_CHECK_FILE(${srcdir}/lib-src/libogg/include/ogg/ogg.h,
                 ogg_h_available="yes",
                 ogg_h_available="no")

   if test "$vorbisenc_h_available" = "yes" -a "$ogg_h_available" = "yes"; then
      LIBVORBIS_LOCAL_AVAILABLE="yes"

      dnl We need to override the pkg-config check for libogg by passing
      dnl OGG_CFLAGS and OGG_LIBS to the configure script of libvorbis.
      ogg_srcdir="$(pwd)/${srcdir}/lib-src/libogg"
      ogg_blddir="$(pwd)/lib-src/libogg"
      LIBVORBIS_LOCAL_CONFIGURE_ARGS="--disable-oggtest OGG_CFLAGS='-I${ogg_blddir}/include -I${ogg_srcdir}/include' OGG_LIBS=${ogg_blddir}/src/libogg.la"

      dnl libflac needs libogg too. So we need to pass these flags to the
      dnl configure script of libflac, because it does not use pkg-config.
      LIBVORBIS_LOCAL_CONFIGURE_ARGS="$LIBVORBIS_LOCAL_CONFIGURE_ARGS --with-ogg-includes='${ogg_blddir}/include -I${ogg_srcdir}/include' --with-ogg-libraries=${ogg_blddir}/src/.libs"

      AC_MSG_NOTICE([Vorbis libraries are available in this source tree])
   else
      AC_MSG_NOTICE([Vorbis libraries are NOT available in this source tree])
   fi
   LIBVORBIS_MIMETYPES="application/ogg;audio/x-vorbis+ogg;"
])

AC_DEFUN([AUDACITY_CONFIG_LIBVORBIS], [
   if test "$LIBVORBIS_USE_LOCAL" = yes; then
      LIBVORBIS_CFLAGS='-I$(top_builddir)/lib-src/libogg/include -I$(top_srcdir)/lib-src/libogg/include -I$(top_srcdir)/lib-src/libvorbis/include'
      LIBVORBIS_LIBS='$(top_builddir)/lib-src/libvorbis/lib/libvorbisenc.la $(top_builddir)/lib-src/libvorbis/lib/libvorbisfile.la'
      AC_CONFIG_SUBDIRS([lib-src/libogg lib-src/libvorbis])
   fi

   AC_SUBST([LIBVORBIS_CFLAGS])
   AC_SUBST([LIBVORBIS_LIBS])

   AM_CONDITIONAL([USE_LIBVORBIS], [test "$LIBVORBIS_USE_LOCAL" = yes -o "$LIBVORBIS_USE_SYSTEM" = yes])
   AM_CONDITIONAL([USE_LOCAL_LIBVORBIS], [test "$LIBVORBIS_USE_LOCAL" = yes])

   if test "$LIBVORBIS_USE_LOCAL" = yes -o "$LIBVORBIS_USE_SYSTEM" = yes; then
      AC_DEFINE(USE_LIBVORBIS, 1,
                [Define if the ogg vorbis decoding library is present])
   fi
])
