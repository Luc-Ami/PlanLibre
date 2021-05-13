### INITIALIZATION #############################################################
# In this section we do preparations for initializing our build system.

AC_INIT([planlibre], [0.1], [amiluc_bis@yahoo.fr])
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE([-W foreign])
AM_SILENT_RULES([no])
AC_CONFIG_HEADERS([config.h])  # rune ./autogeb.sh then ./configure to obtain a config.h file
### SHOPPING LIST ##############################################################
# In this section we notify autoconf of what tools we will be using.

AC_PROG_CC
PKG_PROG_PKG_CONFIG
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.18.1])
AM_PATH_XML2([], [], [AC_MSG_ERROR([*** cannot find the libxml2 library ])])
AC_SUBST(XML_CPPFLAGS) 
AC_SUBST(XML_LIBS)     
IT_PROG_INTLTOOL([0.40])

### LIBRARIES ##################################################################
# In this section we use pkg-config to configure the libraries we are using.

PKG_CHECK_MODULES([PLANLIBRE], [
        uuid
	glib-2.0
	json-glib-1.0
	gtk+-3.0
	gtkspell3-3.0
	libxml-2.0
	goocanvas-2.0
	libcurl
	gnome-keyring-1
])

### VARIABLES ##################################################################
# Here, we deal with optional packages and substitution variables.

# Needed by intltool
GETTEXT_PACKAGE=${PACKAGE_TARNAME}
AC_SUBST([GETTEXT_PACKAGE])
AC_SUBST(ALL_LINGUAS) # veriy !
AC_SUBST(LDFLAGS) 
### OUTPUT #####################################################################
# After checking all this stuff, we must output our results to the rest of the
# build system. That is what this section does.

AC_OUTPUT([
	Makefile
	src/Makefile
	pixmaps/Makefile
	ui/Makefile
	po/Makefile.in
])
