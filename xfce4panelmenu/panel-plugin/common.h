/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#ifndef _COMMON_H
#define _COMMON_H

#include <xfce4-modules/mime.h>
#include <xfce4-modules/mime_icons.h>

char *ms_get_save_file (const char *name);
char *ms_get_read_file (const char *name);

xfmime_icon_functions *load_mime_icon_module ();
xfmime_functions *load_mime_module ();

#endif /* _COMMON_H */
