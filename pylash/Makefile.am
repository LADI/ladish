#
# Copyright (C) 2007 Nedko Arnaudov <nedko@arnaudov.name>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
#

include $(top_srcdir)/common.am

pkgpyexecdir = $(pythondir)

if BUILD_PYLASH

pkgpyexec_LTLIBRARIES = _lash.la

INCLUDES = $(PYTHON_INCLUDES)

_lash_la_LDFLAGS = -module -avoid-version ../liblash/liblash.la

_lash_la_SOURCES = lash.c lash.h
BUILT_SOURCES = lash_wrap.c

pkgpyexec_SCRIPTS = lash.py

CLEANFILES = lash_wrap.c lash.py lash.pyc zynjacku.defs
EXTRA_DIST = test.py lash.i

lash_wrap.c lash.py: lash.i lash.h
	$(SWIG) -o lash_wrap.c -I$(top_srcdir) -python $(top_srcdir)/$(subdir)/lash.i
endif
