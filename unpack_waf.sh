#!/bin/bash

#    unpack_waf - generate an unpacked instance of the waf all-in-one blob
#    Copyright (C) 2012 Alessio Treglia <alessio@debian.org>
#    Based on: http://wiki.debian.org/UnpackWaf
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc.,
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

set -e

echo -n "Unpacking waf... "
./waf --help &>/dev/null
WAFDIR=`ls .waf*/`
mv .waf*/${WAFDIR} ${WAFDIR}
sed -i '/^#==>$/,$d' waf
rmdir .waf*
echo "OK."

echo -n "Purging .pyc files... "
find ${WAFDIR} -name "*.pyc" -delete
echo "OK."
