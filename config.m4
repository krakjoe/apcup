dnl $Id$
dnl config.m4 for extension apcup

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(apcup, for apcup support,
dnl Make sure that the comment is aligned:
dnl [  --with-apcup             Include apcup support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(apcup, whether to enable apcup support,
[  --enable-apcup           Enable apcup support])

if test "$PHP_APCUP" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-apcup -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/apcup.h"  # you most likely want to change this
  dnl if test -r $PHP_APCUP/$SEARCH_FOR; then # path given as parameter
  dnl   APCUP_DIR=$PHP_APCUP
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for apcup files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       APCUP_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$APCUP_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the apcup distribution])
  dnl fi

  dnl # --with-apcup -> add include path
  dnl PHP_ADD_INCLUDE($APCUP_DIR/include)

  dnl # --with-apcup -> check for lib and symbol presence
  dnl LIBNAME=apcup # you may want to change this
  dnl LIBSYMBOL=apcup # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $APCUP_DIR/lib, APCUP_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_APCUPLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong apcup lib version or lib not found])
  dnl ],[
  dnl   -L$APCUP_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(APCUP_SHARED_LIBADD)

  PHP_NEW_EXTENSION(apcup, apcup.c, $ext_shared)
fi
