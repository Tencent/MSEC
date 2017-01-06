dnl $Id$
dnl config.m4 for extension monitor_php

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(monitor_php, for monitor_php support,
dnl Make sure that the comment is aligned:
dnl [  --with-monitor_php             Include monitor_php support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(monitor_php, whether to enable monitor_php support,
  Make sure that the comment is aligned:
  [  --enable-monitor_php           Enable monitor_php support])

if test "$PHP_MONITOR_PHP" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-monitor_php -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/monitor_php.h"  # you most likely want to change this
  dnl if test -r $PHP_MONITOR_PHP/$SEARCH_FOR; then # path given as parameter
  dnl   MONITOR_PHP_DIR=$PHP_MONITOR_PHP
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for monitor_php files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       MONITOR_PHP_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$MONITOR_PHP_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the monitor_php distribution])
  dnl fi

  dnl # --with-monitor_php -> add include path
  dnl PHP_ADD_INCLUDE($MONITOR_PHP_DIR/include)

  dnl # --with-monitor_php -> check for lib and symbol presence
  dnl LIBNAME=monitor_php # you may want to change this
  dnl LIBSYMBOL=monitor_php # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $MONITOR_PHP_DIR/$PHP_LIBDIR, MONITOR_PHP_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_MONITOR_PHPLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong monitor_php lib version or lib not found])
  dnl ],[
  dnl   -L$MONITOR_PHP_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  PHP_SUBST(MONITOR_PHP_SHARED_LIBADD)

  PHP_NEW_EXTENSION(monitor_php, monitor_php.c, $ext_shared)
fi
