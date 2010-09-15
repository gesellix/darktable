/*
    This file is part of darktable,
    copyright (c) 2009--2010 Henrik Andersson.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DT_GUI_TOOLS_COLLECTION_H
#define DT_GUI_TOOLS_COLLECTION_H

#include <gtk/gtk.h>

/**	The collection tool is used for filtering and sorting the 
	current collection.
*/

/** get the widget for the collection tool, works like a singelton */
GtkWidget* dt_gui_tools_collection_get ();

void dt_gui_tools_collection_restore_settings ();
#endif