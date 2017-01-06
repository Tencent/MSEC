dnl $Id$
dnl config.m4 for extension log_php

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(log_php, for log_php support,
dnl Make sure that the comment is aligned:
dnl [  --with-log_php             Include log_php support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(log_php, whether to enable log_php support,
  Make sure that the comment is aligned:
  [  --enable-log_php           Enable log_php support])

if test "$PHP_LOG_PHP" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-log_php -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/log_php.h"  # you most likely want to change this
  dnl if test -r $PHP_LOG_PHP/$SEARCH_FOR; then # path given as parameter
  dnl   LOG_PHP_DIR=$PHP_LOG_PHP
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for log_php files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       LOG_PHP_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$LOG_PHP_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the log_php distribution])
  dnl fi

  dnl # --with-log_php -> add include path
  dnl PHP_ADD_INCLUDE($LOG_PHP_DIR/include)

  dnl # --with-log_php -> check for lib and symbol presence
  dnl LIBNAME=log_php # you may want to change this
  dnl LIBSYMBOL=log_php # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $LOG_PHP_DIR/$PHP_LIBDIR, LOG_PHP_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_LOG_PHPLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong log_php lib version or lib not found])
  dnl ],[
  dnl   -L$LOG_PHP_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  PHP_SUBST(LOG_PHP_SHARED_LIBADD)

  PHP_NEW_EXTENSION(log_php, log_php.c, $ext_shared)
fi
