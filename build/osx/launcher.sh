#!/bin/bash

# Stuff the Mac dev of gedit wrote.
if test "x$GTK_DEBUG_LAUNCHER" != x; then
	set -x
fi

if test "x$GTK_DEBUG_GDB" != x; then
	EXEC="gdb --args"
elif test "x$GTK_DEBUG_DTRUSS" != x; then
	EXEC="dtruss"
else
	EXEC=exec
fi

# Where we get all of our paths from.
name=$(basename "$0")
echo $name

dirn=$(dirname "$0")
echo $dirn

bundle=$(cd "$dirn/../../" && pwd)
bundle_contents="$bundle"/Contents
bundle_res="$bundle_contents"/Resources
bundle_lib="$bundle_res"/lib
bundle_bin="$bundle_res"/bin
bundle_data="$bundle_res"/share
bundle_etc="$bundle_res"/etc

export PATH="$bundle_bin:$PATH"
echo $PATH

# Works for most stuff.
export DYLD_FALLBACK_LIBRARY_PATH="$bundle_lib:$DYLD_FALLBACK_LIBRARY_PATH"

# Some more bullcrap to get fontconfig to actually LOOK for the freaking files.
export FONTCONFIG_FILE="$bundle_etc/fonts/fonts.conf"

# How about some bullcrap to fix the gdk crap.
export GDK_PIXBUF_MODULE_FILE="$bundle_etc/gtk-2.0/gdk-pixbuf.loaders"

# Ok, so that gdk crap failed, let's try some new stuff. EDIT: WORKS NOW.
# export GDK_PIXBUF_MODULEDIR="$bundle_lib/gdk-pixbuf-2.0/2.10.0/loaders"
# echo $GDK_PIXBUF_MODULEDIR

# Fix the theme engine paths
export GTK_PATH="$bundle_lib/gtk-2.0/2.10.0"

# export GTK_IM_MODULE_FILE="$bundle_etc/gtk-2.0/gtk.immodules"
# export PANGO_RC_FILE="$bundle_etc/pango/pangorc"

# Strip out the argument added by the OS.
if [ x`echo "x$1" | sed -e "s/^x-psn_.*//"` == x ]; then
	shift 1
fi

if [ "x$GTK_DEBUG_SHELL" != "x" ]; then
	exec bash

else
	$EXEC "$bundle_contents/MacOS/$name-bin"
fi
