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
#include "gui/toolbars.h"

#include "gui/gtk.h"
#include "dtgtk/togglebutton.h"
#include <glade/glade.h>

/* get the box for toolbar */
static GtkBox * _gui_toolbars_get_toolbox (dt_gui_toolbar_t toolbar);

void 
dt_gui_toolbars_init ()
{
  GtkBox *hbox,*hbox2;
  
  /* create the topbar */ 
  hbox = GTK_BOX (glade_xml_get_widget (darktable.gui->main_window, "topbar"));
  /* add left toolbox */
  gtk_box_pack_start (hbox,GTK_WIDGET ((hbox2=GTK_BOX (gtk_hbox_new(FALSE,0)))),TRUE,FALSE,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox2),2);
  /* add center toolbox */
  gtk_box_pack_start (hbox,GTK_WIDGET ((hbox2=GTK_BOX (gtk_hbox_new(FALSE,0)))),FALSE,FALSE,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox2),2);
  /* add right toolbox */
  gtk_box_pack_end (hbox,GTK_WIDGET ((hbox2=GTK_BOX (gtk_hbox_new(FALSE,0)))),TRUE,FALSE,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox2),2);  
  
  /* create the bottombar */ 
  hbox = GTK_BOX (glade_xml_get_widget (darktable.gui->main_window, "bottombar"));
  /* add left toolbox */
  gtk_box_pack_start (hbox,GTK_WIDGET ((hbox2=GTK_BOX (gtk_hbox_new(FALSE,0)))),TRUE,FALSE,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox2),2);
  /* add center toolbox */
  gtk_box_pack_start (hbox,GTK_WIDGET ((hbox2=GTK_BOX (gtk_hbox_new(FALSE,0)))),FALSE,FALSE,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox2),2);
  /* add right toolbox */
  gtk_box_pack_end (hbox,GTK_WIDGET ((hbox2=GTK_BOX (gtk_hbox_new(FALSE,0)))),TRUE,FALSE,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox2),2);  

}

void 
dt_gui_toolbars_add_tool (dt_gui_toolbar_t toolbar,GtkWidget *tool)
{
  GtkBox *toolbox = _gui_toolbars_get_toolbox (toolbar);
  /* add tool to end of toolbar */
  if (toolbar==TopRightToolbar || toolbar==BottomRightToolbar)
    gtk_box_pack_end (toolbox,tool,FALSE,FALSE,0);
  else
    gtk_box_pack_start (toolbox,tool,FALSE,FALSE,0);
}

void 
dt_gui_toolbars_set_tool (dt_gui_toolbar_t toolbar,GtkWidget *tool)
{
  GtkBox *toolbox = _gui_toolbars_get_toolbox (toolbar);
  
  /* remove all childs of toolbox and add tool */
  GtkWidget *child=NULL;
  do
  {
    child = GTK_WIDGET (g_list_nth_data (gtk_container_get_children(GTK_CONTAINER (toolbox)),0));
    if (child)
      gtk_container_remove (GTK_CONTAINER (toolbox), child);
  } while (child!=NULL);
  
  /* add tool to toolbar */
  if (toolbar==TopRightToolbar || toolbar==BottomRightToolbar)
    gtk_box_pack_end (toolbox,tool,FALSE,FALSE,0);
  else
    gtk_box_pack_start (toolbox,tool,FALSE,FALSE,0);
  
}

void 
dt_gui_toolbars_add_seperator (dt_gui_toolbar_t toolbar)
{
  GtkBox *toolbox = _gui_toolbars_get_toolbox (toolbar);
   /* add tool to end of toolbar */
  if (toolbar==TopRightToolbar || toolbar==BottomRightToolbar)
    gtk_box_pack_end (toolbox,gtk_label_new(" "),FALSE,FALSE,0);
  else
    gtk_box_pack_start (toolbox,gtk_label_new(" "),FALSE,FALSE,0);
}

GtkBox * 
_gui_toolbars_get_toolbox (dt_gui_toolbar_t toolbar)
{
  GtkBox *bar=NULL, *toolbox=NULL;
  
  if (toolbar==TopLeftToolbar || 
      toolbar==TopCenterToolbar ||
      toolbar==TopRightToolbar 
  ) 
    bar = GTK_BOX (glade_xml_get_widget (darktable.gui->main_window, "topbar"));
  else
    bar = GTK_BOX (glade_xml_get_widget (darktable.gui->main_window, "bottombar"));
  
  if (toolbar==TopLeftToolbar || toolbar==BottomLeftToolbar)
    toolbox = GTK_BOX (g_list_nth_data (gtk_container_get_children (GTK_CONTAINER (bar)),0));
  else if (toolbar==TopCenterToolbar || toolbar==BottomCenterToolbar)
    toolbox = GTK_BOX (g_list_nth_data (gtk_container_get_children (GTK_CONTAINER (bar)),1));
  else if (toolbar==TopRightToolbar || toolbar==BottomRightToolbar)
    toolbox = GTK_BOX (g_list_nth_data (gtk_container_get_children (GTK_CONTAINER (bar)),2));
  
  return toolbox;
}

