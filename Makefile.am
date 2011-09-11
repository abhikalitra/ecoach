SUBDIRS = src gfx data po

src_CPPFLAGS = -DLOCALEDIR=\"$(localedir)\"
LDADD = @LIBINTL@
AM_CPPFLAGS = -I../po -I$(top_srcdir)/po

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = ABOUT-NLS config.rpath m4/ChangeLog
