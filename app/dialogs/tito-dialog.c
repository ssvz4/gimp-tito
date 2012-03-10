/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) Srihari Sriraman, Suhas Bharadwaj, Vidyashree K and Zeeshan Ali Ansari
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "config.h"           

#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"

#include "dialogs-types.h"

#include "widgets/gimpuimanager.h"
#include "widgets/gimpaction.h"

#include "tito-dialog.h"

#include "gimp-intl.h"


static GtkWidget* setup_list(void);                  //creates results ui
static gboolean search_dialog (void);                //builds the main ui
static void key_released (GtkWidget *widget, 
                      GdkEventKey *event,
                      gpointer func_data);           //callback for key release event in entry
static gboolean search( GtkAction *action, 
                        const gchar* keyword);       //searches through labels
void search_display_results (const gchar *keyword);
static void add_to_list( const gchar *label,  
                          const gchar *tooltip,
                          GtkAction* action);      //adds a new list items with given name
gboolean result_selected (GtkWidget * widget,
                      GdkEventKey* pKey,
                      gpointer func_data);           //callback for return key pressed on result
void run_result_action(void);                        //resposible for carrying out action selected



static GtkWidget *dialog;
static GtkWidget *list;                             //view shown to the user
static GtkWidget *list_view;
static gint def_height=1,r_height=200;
static gfloat tito_width_perc=0.4,tito_top_perc=0.13;

static enum {
  RESULT_DATA,			// just 0, 1 and 2 for convenience
  RESULT_ACTION,
  N_COL
};
  

GtkWidget *
tito_dialog_create (void)
{
  search_dialog();
   gtk_accel_map_change_entry ("<Actions>/dialogs/dialogs-tito",'?',GDK_SHIFT_MASK,FALSE);
  return dialog;
}

static GtkWidget* 
setup_list(void)
{
  gint wid1=200,wid2=100;
  GtkWidget *sc_win;
  GtkListStore *store; // holds all objects to be displayed in the list
  GtkCellRenderer *cell;
  GtkTreeViewColumn *column;

  sc_win= gtk_scrolled_window_new(NULL, NULL);
  store=gtk_list_store_new(N_COL, G_TYPE_STRING, GTK_TYPE_ACTION);
  list=gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(list),FALSE);
  cell =gtk_cell_renderer_text_new();
  column=gtk_tree_view_column_new_with_attributes(NULL,
                                                  cell,
                                                  "text", RESULT_DATA,
                                                  NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(list),column);
  gtk_tree_view_column_set_max_width(column,wid1);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sc_win),
                                 GTK_POLICY_NEVER,
                                 GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(sc_win),list);
  g_object_unref(G_OBJECT(store));
  return sc_win;
}


static gboolean
search_dialog (void)
{
  GtkWidget *main_vbox, * keyword_entry;
  gboolean run;
  gdouble opacity=0.7;

  dialog= gtk_dialog_new();

  //set no decoration, size, opacity, position of dialog
  gtk_window_set_decorated (GTK_WINDOW(dialog),FALSE);
  gtk_window_set_default_size (GTK_WINDOW(dialog),tito_width_perc*gdk_screen_width(),def_height);
  gtk_window_move (GTK_WINDOW(dialog),((1-tito_width_perc)/2)*gdk_screen_width(),tito_top_perc*gdk_screen_height());
  gtk_window_set_opacity (GTK_WINDOW(dialog),opacity);
  gtk_window_set_transient_for (GTK_WINDOW(dialog),NULL);
 //main_vbox has keyword_entry
  main_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (GTK_VBOX (gtk_dialog_get_content_area(GTK_DIALOG(dialog)))), main_vbox);
  gtk_widget_show (main_vbox);

  keyword_entry = gtk_entry_new();
  gtk_widget_show (keyword_entry);
  gtk_box_pack_start(GTK_BOX(main_vbox),keyword_entry,FALSE,TRUE,0);
/*  gtk_container_add (GTK_CONTAINER (main_vbox ),keyword_entry);   */

  //set up list view and add results stuff
  list_view = setup_list();
  gtk_box_pack_start(GTK_BOX(main_vbox),list_view,TRUE,TRUE,0);

  /*events*****************************************************************************************************/

  //for entry in keyword_entry
  gtk_widget_set_events(dialog, GDK_KEY_PRESS_MASK);
  gtk_widget_set_events(dialog, GDK_KEY_RELEASE_MASK);
  g_signal_connect (keyword_entry, "key-release-event", G_CALLBACK (key_released), NULL);

  //for enter pressed in list_view
  g_signal_connect(list, "key_press_event", G_CALLBACK(result_selected), NULL);
  
  /*end of events**********************************************************************************************/

  gtk_widget_show (dialog);
  run = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK);
  return run;
}


static void
key_released( GtkWidget *widget,
              GdkEventKey *event,
              gpointer func_data)
{
  const gchar *entry_text;
  entry_text = gtk_editable_get_chars(GTK_EDITABLE(widget),0,-1);

  //close if Escape pressed
  switch (event->keyval)
  {
    case GDK_Escape: 
    {
      gtk_widget_destroy(dialog);
      return;
    }
  }
  
  //shows the results if key pressed and hides the results if entry is empty
  if(strcmp(entry_text,"")!=0)
  {
    gtk_window_resize(GTK_WINDOW(dialog),tito_width_perc*gdk_screen_width(),r_height);
    gtk_widget_show_all(list_view);
  }
  else
  {
    gtk_widget_hide(list_view);
    gtk_window_resize(GTK_WINDOW(dialog),tito_width_perc*gdk_screen_width(),def_height);
  } 
  gtk_list_store_clear (GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list))));
  search_display_results(entry_text);
}

gboolean
result_selected ( GtkWidget * widget, 
                  GdkEventKey* pKey,
                  gpointer func_data)
{
  if (pKey->type == GDK_KEY_PRESS)
  {
    switch (pKey->keyval)
    {
      case GDK_Return:
      {
        //if enter pressed then get selection
			  run_result_action();
			  gtk_widget_destroy(dialog);
        break;
    	}
    	case GDK_Escape: 
      {
        gtk_widget_destroy(dialog);
        return TRUE;
      }
    }
  }
  return FALSE;
}


static void
add_to_list( const gchar *label,
             const gchar *tooltip,
             GtkAction* action)
{
  GtkListStore *store;
  GtkTreeIter iter;
  char *desc = (char *) tooltip;
  char *labe = (char *) label;
  char *data = (char *) malloc(1024);
  if(data!=NULL)
    {
      strcpy(data,labe);
      if(desc!=NULL)
        { 
          strcat(data,"\n ");
          strcat(data,desc);
        }
      store= GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));

      gtk_list_store_append(store, &iter);
      gtk_list_store_set(store, &iter, RESULT_DATA, data, RESULT_ACTION, action, -1);
      free(data);
    }
}


void
run_result_action(void)
{
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  /* This will only work in single or browse selection mode! */
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

  if (gtk_tree_selection_get_selected(selection, &model, &iter))
  {
    gchar *name;
    GtkAction *action;
    gtk_tree_model_get (model, &iter, RESULT_DATA, &name, -1);
    gtk_tree_model_get (model, &iter, RESULT_ACTION, &action, -1);
    gtk_action_activate(action);
    g_free(name);
  }
}


void
search_display_results (const gchar *keyword)
{
  //iterate through all actions
  GimpUIManager *manager;
  GList             *list;
  manager= gimp_ui_managers_from_name ("<Image>")->data;
  
  if(strcmp(keyword,"")==0)
    return;

  //for every action group
  for (list = gtk_ui_manager_get_action_groups (GTK_UI_MANAGER (manager));
       list;
       list = g_list_next (list))
    {
      GimpActionGroup *group = list->data;
      GList           *actions;
      GList           *list2;
      
      //get and sort actions      
      actions = gtk_action_group_list_actions (GTK_ACTION_GROUP (group));
      actions = g_list_sort (actions, (GCompareFunc) gimp_action_name_compare);
      
      //for every action
      for (list2 = actions; list2; list2 = g_list_next (list2))
        {
          GtkAction       *action        = list2->data;
          const gchar     *name;
          name         = gtk_action_get_name (action);

          //exclude menus and popups
          if (strstr (name, "-menu")  ||
              strstr (name, "-popup") ||
              name[0] == '<')
            continue;

          if(search(action,keyword))
       	  {
  
            add_to_list(gimp_strip_uline (gtk_action_get_label (action)),gtk_action_get_tooltip (action),action);
          }
        }
      g_list_free (actions);

    }
}

static gboolean
search( GtkAction *action,
        const gchar* keyword)
{
	const gchar *label, *tooltip;
  label        = gimp_strip_uline (gtk_action_get_label (action));
  tooltip      = gtk_action_get_tooltip (action);
	
	if(strcasestr(label,keyword))
		{
		  return TRUE; 
		}
	else if(tooltip!=NULL && strcasestr(tooltip,keyword))
	  {
	  	return TRUE; 
  	}
  return FALSE;
}
