dnl $Id$
dnl config.m4 for extension nlb_php

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(nlb_php, for nlb_php support,
dnl Make sure that the comment is aligned:
dnl [  --with-nlb_php             Include nlb_php support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(nlb_php, whether to enable nlb_php support,
  Make sure that the comment is aligned:
  [  --enable-nlb_php           Enable nlb_php support])

if test "$PHP_NLB_PHP" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-nlb_php -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/nlb_php.h"  # you most likely want to change this
  dnl if test -r $PHP_NLB_PHP/$SEARCH_FOR; then # path given as parameter
  dnl   NLB_PHP_DIR=$PHP_NLB_PHP
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for nlb_php files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       NLB_PHP_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$NLB_PHP_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the nlb_php distribution])
  dnl fi

  dnl # --with-nlb_php -> add include path
  dnl PHP_ADD_INCLUDE($NLB_PHP_DIR/include)

  dnl # --with-nlb_php -> check for lib and symbol presence
  dnl LIBNAME=nlb_php # you may want to change this
  dnl LIBSYMBOL=nlb_php # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $NLB_PHP_DIR/$PHP_LIBDIR, NLB_PHP_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_NLB_PHPLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong nlb_php lib version or lib not found])
  dnl ],[
  dnl   -L$NLB_PHP_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  PHP_SUBST(NLB_PHP_SHARED_LIBADD)

  PHP_NEW_EXTENSION(nlb_php, nlb_php.c, $ext_shared)
fi
