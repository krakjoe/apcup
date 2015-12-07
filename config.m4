dnl $Id$
dnl config.m4 for extension apcup

PHP_ARG_ENABLE(apcup, whether to enable apcup support,
[  --enable-apcup           Enable apcup support])

if test "$PHP_APCUP" != "no"; then
  PHP_NEW_EXTENSION(apcup, apcup.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
  PHP_ADD_EXTENSION_DEP(apcup, apcu)
fi
