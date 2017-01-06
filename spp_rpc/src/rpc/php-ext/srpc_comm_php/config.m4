dnl $Id$
dnl config.m4 for extension srpc_comm_php

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(srpc_comm_php, for srpc_comm_php support,
dnl Make sure that the comment is aligned:
dnl [  --with-srpc_comm_php             Include srpc_comm_php support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(srpc_comm_php, whether to enable srpc_comm_php support,
  Make sure that the comment is aligned:
  [  --enable-srpc_comm_php           Enable srpc_comm_php support])

if test "$PHP_SRPC_COMM_PHP" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-srpc_comm_php -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/srpc_comm_php.h"  # you most likely want to change this
  dnl if test -r $PHP_SRPC_COMM_PHP/$SEARCH_FOR; then # path given as parameter
  dnl   SRPC_COMM_PHP_DIR=$PHP_SRPC_COMM_PHP
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for srpc_comm_php files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       SRPC_COMM_PHP_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$SRPC_COMM_PHP_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the srpc_comm_php distribution])
  dnl fi

  dnl # --with-srpc_comm_php -> add include path
  dnl PHP_ADD_INCLUDE($SRPC_COMM_PHP_DIR/include)

  dnl # --with-srpc_comm_php -> check for lib and symbol presence
  dnl LIBNAME=srpc_comm_php # you may want to change this
  dnl LIBSYMBOL=srpc_comm_php # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $SRPC_COMM_PHP_DIR/$PHP_LIBDIR, SRPC_COMM_PHP_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_SRPC_COMM_PHPLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong srpc_comm_php lib version or lib not found])
  dnl ],[
  dnl   -L$SRPC_COMM_PHP_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  PHP_SUBST(SRPC_COMM_PHP_SHARED_LIBADD)

  PHP_NEW_EXTENSION(srpc_comm_php, srpc_comm_php.c, $ext_shared)
fi
