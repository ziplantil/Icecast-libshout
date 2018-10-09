# Configure paths for libFLAC

dnl XIPH_PATH_FLAC([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libFLAC, and define FLAC_CFLAGS FLAC_LIBS
dnl FLAC_FLACENC_LIBS FLAC_FLACFILE_LIBS FLAC_LDFLAGS
dnl

AC_DEFUN([XIPH_PATH_FLAC],
[
AC_REQUIRE([XIPH_PATH_OGG])

dnl Get the cflags and libraries for flac
dnl
AC_ARG_VAR([FLAC],[path to flac installation])
AC_ARG_WITH(flac,
    AC_HELP_STRING([--with-flac=PREFIX],
        [Prefix where libFLAC is installed (optional)]),
    flac_prefix="$withval",
    flac_prefix="$FLAC_PREFIX"
    )
if test "x$with_flac" = "xno"
then
  AC_MSG_RESULT([FLAC support disabled by request])
else
  if test "x$flac_prefix" = "x" -o "x$flac_prefix" = "xyes"; then
      if test "x$prefix" = "xNONE"; then
          flac_prefix="/usr/local"
      else
          flac_prefix="$prefix"
      fi
  fi

  FLAC_CFLAGS="$OGG_CFLAGS"
  FLAC_LDFLAGS="$OGG_LDFLAGS"
  if test "x$flac_prefix" != "x$ogg_prefix"; then
      XIPH_GCC_WARNING(-I"$flac_prefix/include",,
              [FLAC_CFLAGS="$FLAC_CFLAGS -I$flac_prefix/include"
              FLAC_LDFLAGS="-L$flac_prefix/lib $FLAC_LDFLAGS"
              ])
  fi

  FLAC_LIBS="-lFLAC"

  xt_save_LIBS="$LIBS"
  xt_save_LDFLAGS="$LDFLAGS"
  LDFLAGS="$LDFLAGS $FLAC_LDFLAGS"
  LIBS="$LIBS $FLAC_LIBS"
  xt_have_flac="yes"
  AC_MSG_CHECKING([for libFLAC])
  AC_TRY_LINK_FUNC(ogg_stream_init, [AC_MSG_RESULT([ok])],
          [LIBS="$LIBS $OGG_LIBS"
          AC_TRY_LINK_FUNC(ogg_stream_init,
              [FLAC_LIBS="$FLAC_LIBS $OGG_LIBS"],
              [xt_have_flac="no"])
          ])
  if test "x$xt_have_flac" = "xyes"
  then
      AC_LINK_IFELSE([AC_LANG_PROGRAM(
                  [#include "FLAC/stream_decoder.h"],
                  [void *p = FLAC__stream_decoder_init_stream;])],
              [],
              [xt_have_flac="no"])
  fi

  LIBS="$xt_save_LIBS"
  LDFLAGS="$xt_save_LDFLAGS"
  
  if test "x$xt_have_flac" = "xyes"
  then
      AC_MSG_RESULT([ok])
      AC_DEFINE([HAVE_FLAC],[1],[Define if FLAC support is available])
      $1
  else
      ifelse([$2], , AC_MSG_ERROR([Unable to link to libFLAC]), [$2])
      FLAC_CFLAGS=""
      FLAC_LDFLAGS=""
      FLAC_LIBS=""
  fi
  AC_SUBST(FLAC_CFLAGS)
  AC_SUBST(FLAC_LDFLAGS)
  AC_SUBST(FLAC_LIBS)
fi
])
