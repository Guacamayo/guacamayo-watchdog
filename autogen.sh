#!/bin/sh
# Run this to generate all the initial makefiles, etc.

ORIGDIR=`pwd`

if automake-1.11 --version < /dev/null > /dev/null 2>&1 ; then
    AUTOMAKE=automake-1.11
    ACLOCAL=aclocal-1.11
    export AUTOMAKE ACLOCAL
else
        echo
        echo "Automake 1.11.x not found"
fi

rm -rf autom4te.cache

autoreconf -vfi || exit $?
cd $ORIGDIR || exit $?

if test -z "$NOCONFIGURE"; then
        ./configure $AUTOGEN_CONFIGURE_ARGS "$@" || exit $?
        echo "Now type 'make' to compile $PROJECT."
fi
