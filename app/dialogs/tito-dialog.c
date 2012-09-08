  /* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 2012 Srihari Sriraman, Suhas V, Vidyashree K, Zeeshan Ali Ansari
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
#include <ctype.h>
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

#define           MAX_HISTORY_ACTIONS 20

gboolean            tito_run_result_action             (void);
static GtkWidget*   tito_setup_results_list            (void);
static gboolean     tito_search_dialog                 (void);
static gboolean     tito_is_action_match               (GtkAction         *action,
                                                        const gchar*       keyword);
static void         tito_add_to_results_list           (const gchar       *label,
                                                        const gchar       *tooltip,
                                                        GtkAction*         action);
static void         tito_search_history_and_actions    (const gchar       *keyword);

static void         tito_update_history                (GtkAction         *action);
static void         tito_read_history                  (void);
static void         tito_fill_history                  (void);
static void         tito_clear_history                 (void);

static void         tito_preferences_dialog            (void);
static void         tito_set_default_preferences       (void);
static void         tito_update_preferences            (void);
static void         tito_write_preferences             (void);
static void         tito_read_preferences              (void);
static void         tito_set_prefereces_ui_values      (void);

gboolean            tito_initializer                   (void);
void                tito_finalizer                     (void);
static void         tito_context_menu                  (void);

static GtkWidget        *tito_dialog;
static GtkWidget        *list;
static GtkWidget        *list_view;
static GtkWidget        *keyword_entry;
static GtkWidget        *preferences_button;

GimpUIManager           *manager;
static gchar            *history_file_path;
static gchar            *preference_file_path;
static gint              cur_no_of_his_actions;
static gint              default_height = 1;
static gboolean          first_time = TRUE;
static gint              tmp_x, tmp_y;
static gint              par_x, par_y;
static gint              par_height, par_width;
static GtkCellRenderer  *cell_renderer;


enum RES_COL {
  RESULT_ICON,
  RESULT_DATA,
  RESULT_ACTION,
  IS_SENSITIVE,
  N_COL
};

static struct HISTORY {
  GtkAction        *history_action;
  int               count;
}history[MAX_HISTORY_ACTIONS];

static struct HISTORY_ACTION_NAME{
  char             *action_name;
  int               no;
}name[MAX_HISTORY_ACTIONS];

static struct PREFERENCES {
  int               POSITION;
  float             POSITION_X;
  float             POSITION_Y;
  int               NO_OF_RESULTS;
  float             WIDTH;
  gboolean          AUTO_HIDE;
  gboolean          SHOW_INSENSITIVE;
  gdouble           OPACITY;
}PREF;

static struct TITO_PREF_UI{
  GtkWidget        *specify_radio;
  GtkWidget        *pos_x_hbox;
  GtkWidget        *pos_y_hbox;
  GtkWidget        *right_top_radio;
  GtkWidget        *middle_radio;
  GtkWidget        *pos_x_spin_button;
  GtkWidget        *pos_y_spin_button;
  GtkWidget        *no_of_results_spin_button;
  GtkWidget        *width_spin_button;
  GtkWidget        *opacity_spin_button;
  GtkWidget        *autohide_check_button;
  GtkWidget        *show_insensitive_check_button;
}PREF_UI;

GtkWidget *
tito_dialog_create (void)
{
  if(!tito_initializer())
    g_message("Tito tito_initializer failed");
  tito_search_dialog();
  return tito_dialog;
}

static void
modify_position_spins (void)
{
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PREF_UI.specify_radio)))
    {
      gtk_widget_set_sensitive(PREF_UI.pos_x_hbox, TRUE);
      gtk_widget_set_sensitive(PREF_UI.pos_y_hbox, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive(PREF_UI.pos_x_hbox, FALSE);
      gtk_widget_set_sensitive(PREF_UI.pos_y_hbox, FALSE);
    }
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(PREF_UI.pos_x_spin_button),
                            (gdouble)(PREF.POSITION_X/par_width*100));
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(PREF_UI.pos_y_spin_button),
                            (gdouble)(PREF.POSITION_Y/par_height*100));
  gtk_spin_button_set_range (GTK_SPIN_BUTTON(PREF_UI.pos_x_spin_button),
                            (gdouble)0,
                            (gdouble)( 100-gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(PREF_UI.width_spin_button))));
}

static void
tito_clear_history_button_clicked ( GtkButton *button,
                                    gpointer   user_data)
{
  tito_clear_history();
}

static void
restore_defaults_button_clicked ( GtkButton *button,
                                  gpointer   user_data)
{
  tito_set_default_preferences();
  tito_set_prefereces_ui_values();
}

static void
tito_set_prefereces_ui_values (void)
{
  if( PREF.POSITION == 0)
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(PREF_UI.right_top_radio), TRUE);
  else if( PREF.POSITION == 1)
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(PREF_UI.middle_radio), TRUE);
  else
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(PREF_UI.specify_radio), TRUE);

  modify_position_spins();

  gtk_spin_button_set_value (GTK_SPIN_BUTTON(PREF_UI.no_of_results_spin_button), (gdouble)PREF.NO_OF_RESULTS);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(PREF_UI.width_spin_button), (gdouble)PREF.WIDTH);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(PREF_UI.opacity_spin_button), (gdouble)PREF.OPACITY*100);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PREF_UI.autohide_check_button), PREF.AUTO_HIDE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(PREF_UI.show_insensitive_check_button), PREF.SHOW_INSENSITIVE);

}

static gboolean
on_focus_out (GtkWidget *widget,
              GdkEventFocus *event,
              gpointer data)
{
  if(!gtk_widget_is_focus(preferences_button))
    tito_finalizer();
  return TRUE;
}


static void
key_released( GtkWidget *widget,
              GdkEventKey *event,
              gpointer func_data)
{
  const gchar *entry_text;
  entry_text = gtk_editable_get_chars(GTK_EDITABLE(widget),0,-1);

  switch (event->keyval)
  {
    case GDK_Escape:
    {
      tito_finalizer();
      return;
    }
    case GDK_Return:
    {
      tito_run_result_action();
      return;
    }
  }

  if(strcmp(entry_text,"")!=0)
    {
      gtk_window_resize(GTK_WINDOW(tito_dialog),(PREF.WIDTH * par_width)/100,
                       (PREF.NO_OF_RESULTS*40+100));
      gtk_list_store_clear (GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list))));
      gtk_widget_show_all(list_view);
      tito_search_history_and_actions(entry_text);
      gtk_tree_selection_select_path ( gtk_tree_view_get_selection(GTK_TREE_VIEW(list)),
                                       gtk_tree_path_new_from_string("0"));
    }
  else if(strcmp(entry_text,"")==0 && (event->keyval == GDK_Down) )
    {
      gtk_window_resize(GTK_WINDOW(tito_dialog),(PREF.WIDTH * par_width)/100,
                       (PREF.NO_OF_RESULTS*40+100));
      gtk_list_store_clear (GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list))));
      gtk_widget_show_all(list_view);
      tito_search_history_and_actions(" ");
      gtk_tree_selection_select_path ( gtk_tree_view_get_selection(GTK_TREE_VIEW(list)),
                                       gtk_tree_path_new_from_string("0"));

    }
  else
    {
      gtk_widget_hide(list_view);
      gtk_window_resize (GTK_WINDOW(tito_dialog),(PREF.WIDTH * par_width)/100,default_height);
    }
}

static gboolean
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
              tito_run_result_action();
              break;
            }
          case GDK_Escape:
            {
              tito_finalizer();
              return TRUE;
            }
          case GDK_Up:
            {
              GtkTreeSelection *selection;
              GtkTreeModel     *model;
              GtkTreeIter       iter;
              GtkTreePath       *path;
              selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
              gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

              if (gtk_tree_selection_get_selected(selection, &model, &iter))
                {
                  path = gtk_tree_model_get_path(model,&iter);
                  if(strcmp(gtk_tree_path_to_string(path),"0")==0)
                    {
                      gtk_widget_grab_focus((GTK_WIDGET(keyword_entry)));
                      return TRUE;
                    }
                }
            }
        }
    }
  return FALSE;
}


static void
row_activated ( GtkTreeView        *treeview,
                GtkTreePath        *path,
                GtkTreeViewColumn  *col,
                gpointer            userdata)
{
    tito_run_result_action();
}

static gboolean
tito_action_view_accel_find_func ( GtkAccelKey *key,
                                   GClosure    *closure,
                                   gpointer     data)
{
  return (GClosure *) data == closure;
}

static gchar*
find_accel_label (GtkAction *action)
{
  guint            accel_key     = 0;
  GdkModifierType  accel_mask    = 0;
  GClosure        *accel_closure = NULL;
  gchar           *accel_string;
  GtkAccelGroup   *accel_group;

  accel_group = gtk_ui_manager_get_accel_group (GTK_UI_MANAGER (manager));
  accel_closure = gtk_action_get_accel_closure (action);

  if(accel_closure)
   {
     GtkAccelKey *key;
     key = gtk_accel_group_find (accel_group,
                                 tito_action_view_accel_find_func,
                                 accel_closure);
     if (key            &&
         key->accel_key &&
         key->accel_flags & GTK_ACCEL_VISIBLE)
       {
         accel_key  = key->accel_key;
         accel_mask = key->accel_mods;
       }
   }
  accel_string = gtk_accelerator_get_label (accel_key, accel_mask);
  if(strcmp(accel_string,""))
    return accel_string;

  return NULL;
}


static void
tito_add_to_results_list( const gchar *label,
                          const gchar *tooltip,
                          GtkAction* action)
{
  GtkListStore *store;
  GtkTreeIter iter;
  gchar *markuptxt;
  gchar *accel_string = find_accel_label (action);
  const gchar *stock_id = gtk_action_get_stock_id (action);
  char *data = g_new(char,1024);
  char shortcut[1024] = "";

  if(data==NULL)
    return;
  if(strchr(label, '@')!=NULL||strchr(label, '&')!=NULL)
    return;

  if(GTK_IS_TOGGLE_ACTION(action))
    {
      if(gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)))
        stock_id = GTK_STOCK_OK;
      else
        stock_id = GTK_STOCK_NO;
    }

  if(accel_string==NULL)
      strcpy(shortcut,"");
  else if(strchr(accel_string,'<')!=NULL)
      strcpy(shortcut,"");
  else
    {
      strcpy(shortcut," | ");
      strcat(shortcut,accel_string);
    }

  if(tooltip==NULL)
      strcpy(data,"");
  else if(strchr(tooltip, '<')!=NULL)
       strcpy(data,"");
  else
    {
      strcpy(data,"\n");
      strcat(data,tooltip);
    }

  markuptxt = g_strdup_printf("%s<small>%s<span weight='light'>%s</span></small>",
                               label, shortcut, data);
  store= GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));
  gtk_list_store_append(store, &iter);
  gtk_list_store_set(store, &iter,
                     RESULT_ICON, stock_id,
                     RESULT_DATA, markuptxt,
                     RESULT_ACTION, action,
                     IS_SENSITIVE, gtk_action_get_sensitive(action),
                     -1);
  g_free(data);
}

gboolean
tito_run_result_action (void)
{
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
  gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
  if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
      GtkAction *action;
      gtk_tree_model_get (model, &iter, RESULT_ACTION, &action, -1);
      if(!gtk_action_get_sensitive(action))
        return FALSE;

      if(PREF.AUTO_HIDE)
        gtk_widget_hide(tito_dialog);
      gtk_action_activate(action);
      tito_finalizer();
      tito_update_history(action);
    }
  return TRUE;
}


void
tito_search_history_and_actions (const gchar *keyword)
{
  GList             *list;
  int i = 0;
  manager= gimp_ui_managers_from_name ("<Image>")->data;
  if(strcmp(keyword,"")==0)
    return;

  //search in history
  for(i=0;i<cur_no_of_his_actions;i++)
  {
    if(history[i].history_action!=NULL)
      {
        if(tito_is_action_match(history[i].history_action,keyword))
          tito_add_to_results_list( gimp_strip_uline (gtk_action_get_label (history[i].history_action)),
                                    gtk_action_get_tooltip (history[i].history_action),
                                    history[i].history_action );
      }
  }

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
          gboolean        is_redundant = FALSE;
          name         = gtk_action_get_name (action);


          //exclude menus, popups, context and undo
          if (strstr (name, "-menu")  ||
              strstr (name, "-popup") ||
              strstr (name,"context") ||
              strstr (name,"edit-undo") ||
              name[0] == '<')
            continue;

            if( !gtk_action_get_sensitive(action) && !(PREF.SHOW_INSENSITIVE) )
                continue;

          for(i=0;i<cur_no_of_his_actions;i++)
            {
              if(history[i].history_action!=NULL)
                {
                  if(strcmp(gtk_action_get_name(history[i].history_action),name)==0)
                    {
                      is_redundant = TRUE;
                      break;
                    }
                }
            }

          if(is_redundant)
            continue;

          if(tito_is_action_match(action,keyword))
            {
              tito_add_to_results_list( gimp_strip_uline (gtk_action_get_label (action)),
                                        gtk_action_get_tooltip (action),
                                        action);
            }
        }
      g_list_free (actions);
   }
}

static void tito_fill_history (void)
{
  GList             *list;
  int i = 0;
  manager= gimp_ui_managers_from_name ("<Image>")->data;

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
             const gchar     *action_name;
             action_name        = gtk_action_get_name (action);

             //exclude menus and popups
             if ( strstr (action_name, "-menu")  ||
                  strstr (action_name, "-popup") ||
                  strstr (action_name, "context") ||
                  action_name[0] == '<')
                    continue;

             for(i=0;i<cur_no_of_his_actions;i++)
               {
                 if(name[i].action_name!=NULL)
                   {
                     if(strcmp(name[i].action_name,action_name)==0)
                       {
                         history[i].history_action=action;
                         history[i].count=name[i].no;
                       }
                    }
                }
            }
        }
}

static gboolean
fuzzy_search ( gchar *string,
               gchar *key)
{
  gchar *remaining_string = string;
  if ( strlen(key) == 0 || strlen(remaining_string) == 0 )
      return TRUE;

  if ( (remaining_string = strchr(string, key[0])) != NULL )
      fuzzy_search( key+1, remaining_string+1 );
  return FALSE;
}

static gboolean
tito_is_action_match ( GtkAction *action,
                       const gchar* keyword)
{
  gchar *label, *tooltip, *key;
  int i;
  gchar* space_pos;
  label =   g_new(gchar,1024);
  key =     g_new(gchar,1024);
  tooltip = g_new(gchar,1024);

  strcpy(label, gimp_strip_uline (gtk_action_get_label (action)));
  strcpy(key,keyword);

  for(i=0;i<strlen(label);i++)
    label[i]=tolower(label[i]);
  for(i=0;i<strlen(key);i++)
    key[i]=tolower(key[i]);
  if(strlen(key)==2)
    {
      space_pos=strchr(label,' ');
      if(space_pos!=NULL)
        {
          space_pos++;
          if(key[0]==label[0] && key[1]==*space_pos)
              return TRUE;
        }
    }

  if(strstr(label,key))
        return TRUE;
  if(fuzzy_search(label,key))
        return TRUE;

  if(strlen(key)>2 || strcmp(key," ")==0)
    {
      if(gtk_action_get_tooltip(action)!=NULL)
        {
          strcpy(tooltip,gtk_action_get_tooltip (action));
          for(i=0;i<strlen(tooltip);i++)
            tooltip[i]=tolower(tooltip[i]);

          if(strstr(tooltip,key))
                return TRUE;
        }
    }

  g_free(label);
  g_free(key);
  g_free(tooltip);

  return FALSE;
}

static void
tito_read_history (void)
{
  int i;
  FILE *fp;
  cur_no_of_his_actions=0;

  fp= fopen(history_file_path,"r");
  if(fp==NULL)
        return;

  for(i=0;i<MAX_HISTORY_ACTIONS;i++)
    {
      if(fscanf(fp,"%s  %d",name[i].action_name,&name[i].no) == EOF)
        break;
      cur_no_of_his_actions++;
    }
  fclose(fp);
  tito_fill_history();
}

static int
compare ( const void * a,
              const void * b)
{
  struct HISTORY *p = (struct HISTORY *)a;
  struct HISTORY *q = (struct HISTORY *)b;
  return (q->count - p->count);
}

static void
tito_update_history (GtkAction *action)
{
  int i;
  FILE *fp;
  gboolean is_present=FALSE;

  fp= fopen(history_file_path,"w");
  if(fp==NULL)
    {
      g_message("Unable to open history file to write");
     return;
    }

  //check if already in history var
  for(i=0;i<cur_no_of_his_actions;i++)
    {
      if(strcmp(gtk_action_get_name(action),gtk_action_get_name(history[i].history_action))==0)
        {
          history[i].count++;
          is_present=TRUE;
          break;
        }
    }

  //assign action to history var
  if(!is_present)
    {
      if(cur_no_of_his_actions==MAX_HISTORY_ACTIONS)
        {
          history[MAX_HISTORY_ACTIONS-1].history_action = action;
          history[MAX_HISTORY_ACTIONS-1].count=1;
        }
      else
        {
          history[cur_no_of_his_actions].history_action = action;
          history[cur_no_of_his_actions++].count=1;
        }
    }

  //sort history according to count
  qsort (history, cur_no_of_his_actions, sizeof(struct HISTORY), compare);

  //write history vars to file
  for(i=0;i<cur_no_of_his_actions;i++)
    {
      if(history[i].history_action != NULL)
          fprintf(fp,"%s  %d \n",gtk_action_get_name(history[i].history_action),
                                   history[i].count);
    }

  fclose(fp);
}

static void
tito_update_position (void)
{
  if(PREF.POSITION == 0)
  {
    PREF.POSITION_X = (1-PREF.WIDTH/100)*par_width+par_x;
    PREF.POSITION_Y = 0.04*par_height+par_y;
  }
  else if(PREF.POSITION == 1)
  {
    PREF.POSITION_X = (par_width- PREF.WIDTH*par_width*.01)/2 + par_x;
    PREF.POSITION_Y = 0.2*par_height + par_y;
  }
  else
  {
    PREF.POSITION_X = tmp_x*par_width/100 + par_x;
    PREF.POSITION_Y = tmp_y*par_height/100 + par_y;
  }
  gtk_window_move (GTK_WINDOW(tito_dialog),PREF.POSITION_X,PREF.POSITION_Y);
}

void
tito_finalizer(void)
{
  if(!PREF.AUTO_HIDE)
    {
      gtk_widget_hide(list_view);
      gtk_window_resize (GTK_WINDOW(tito_dialog),PREF.WIDTH*par_width/100,default_height);
      gtk_entry_set_text(GTK_ENTRY(keyword_entry),"");
      gtk_widget_grab_focus(keyword_entry);
    }
  else
    gtk_widget_destroy(tito_dialog);
}

gboolean
tito_initializer(void)
{
  int i=0;
  GdkWindow *par_window = gdk_screen_get_active_window(gdk_screen_get_default());
  gdk_window_get_geometry (par_window, &par_x, &par_y, &par_width, &par_height, NULL);
  tito_update_position();

  if(first_time)
  {
    history_file_path= g_new(gchar, 1024);
    strcpy(history_file_path,(gchar*)gimp_sysconf_directory());

    preference_file_path= g_new(gchar,1024);
    strcpy(preference_file_path,(gchar*)gimp_sysconf_directory());

    strcat(history_file_path,"/history_tito");
    strcat(preference_file_path,"/preferences_tito");

    for(i=0;i<MAX_HISTORY_ACTIONS;i++)
      {
        name[i].action_name = g_new(char, 100);
        strcpy(name[i].action_name,"");
        name[i].no=0;
      }
    first_time=FALSE;
  }
  tito_read_preferences();
  tito_read_history();
  gtk_accel_map_change_entry ("<Actions>/dialogs/dialogs-tito",'?',GDK_SHIFT_MASK,FALSE);
  return TRUE;
}

static void
tito_clear_history (void)
{
  FILE *fp;
  fp = fopen(history_file_path,"w");
  if(fp == NULL)
    {
        g_message("file not created");
        return;
    }
  fclose(fp);
}

static void
tito_set_default_preferences (void)
{
 PREF.POSITION = 0;
 PREF.WIDTH = 40;
 PREF.POSITION_X = (1-0.4)*par_width+par_x;
 PREF.POSITION_Y = 0.04*par_height+par_y;
 PREF.NO_OF_RESULTS = 4;
 PREF.AUTO_HIDE = TRUE;
 PREF.SHOW_INSENSITIVE = TRUE;
 PREF.OPACITY = 1;
 tito_write_preferences();
}

static void
tito_update_preferences (void)
{
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PREF_UI.right_top_radio)))
    PREF.POSITION = 0;
  else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PREF_UI.middle_radio)))
    PREF.POSITION = 1;
  else
    {
      PREF.POSITION = 2;
      tmp_x = (gfloat) gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(PREF_UI.pos_x_spin_button));
      tmp_y = (gfloat) gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(PREF_UI.pos_y_spin_button));
    }

  PREF.NO_OF_RESULTS    = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(PREF_UI.no_of_results_spin_button));
  PREF.WIDTH            = (gfloat)gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(PREF_UI.width_spin_button));
  PREF.OPACITY          = (gdouble)gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(PREF_UI.opacity_spin_button))/100;
  PREF.AUTO_HIDE        = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PREF_UI.autohide_check_button));
  PREF.SHOW_INSENSITIVE = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(PREF_UI.show_insensitive_check_button));

  tito_update_position();
  tito_write_preferences();
  tito_finalizer();
}

static void
tito_write_preferences (void)
{
 FILE *fp;
 fp=fopen(preference_file_path,"w");
 if(fp == NULL)
    return;
 fprintf(fp,"%d %f %f %d %f %d %d %lf", PREF.POSITION,PREF.POSITION_X, PREF.POSITION_Y,
                    PREF.NO_OF_RESULTS, PREF.WIDTH, PREF.AUTO_HIDE,PREF.SHOW_INSENSITIVE,PREF.OPACITY);
 fclose(fp);
}

static void
tito_read_preferences (void)
{
 FILE *fp;
 fp=fopen(preference_file_path,"r");
 if(fp == NULL)
   {
     tito_set_default_preferences();
     return;
   }
 if(fscanf(fp,"%d %f %f %d %f %d %d %lf", &PREF.POSITION, &PREF.POSITION_X, &PREF.POSITION_Y,
                   &PREF.NO_OF_RESULTS, &PREF.WIDTH, &PREF.AUTO_HIDE, &PREF.SHOW_INSENSITIVE, &PREF.OPACITY) == 0)
       tito_set_default_preferences();
 fclose(fp);
}

static gboolean
context_menu_invoked  ( GtkWidget *widget,
                        GdkEvent  *event,
                        gpointer   user_data)
{
  tito_context_menu();
  return TRUE;
}

static void
context_menu_handler( GtkMenuItem* menuitem,
                      gpointer *data)
{
  if(strchr(gtk_menu_item_get_label(menuitem),'r')!=NULL)
    tito_preferences_dialog();
  else
    gtk_widget_destroy(tito_dialog);
}

static void
tito_context_menu (void)
{
  GtkWidget *preferences_menuitem;
  GtkWidget *close_menuitem;
  GtkWidget *context_menu;

  context_menu = gtk_menu_new();
  preferences_menuitem =  gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES,NULL);
  close_menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLOSE,NULL);

  gtk_menu_shell_append (GTK_MENU_SHELL(context_menu), preferences_menuitem);
  gtk_menu_shell_append (GTK_MENU_SHELL(context_menu), close_menuitem);

  gtk_widget_show(context_menu);
  gtk_widget_show(preferences_menuitem);
  gtk_widget_show(close_menuitem);

  g_signal_connect (preferences_menuitem, "activate", G_CALLBACK (context_menu_handler), NULL);
  g_signal_connect (close_menuitem, "activate", G_CALLBACK (context_menu_handler), NULL);

  gtk_menu_popup( GTK_MENU(context_menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}

static void
tito_preferences_dialog (void)
{
  GtkWidget   *pref_dialog;
  GtkWidget   *top_hbox;

  GtkWidget   *position_frame;
  GtkWidget   *position_vbox;
  GtkWidget   *pos_x_label;
  GtkWidget   *pos_y_label;
  GtkWidget   *specify_alignment_x;
  GtkWidget   *specify_alignment_y;

  GtkWidget   *display_frame;
  GtkWidget   *display_vbox;
  GtkWidget   *no_of_results_hbox;
  GtkWidget   *width_hbox;
  GtkWidget   *opacity_hbox;
  GtkWidget   *no_of_results_label;
  GtkWidget   *width_label;
  GtkWidget   *opacity_label;

  GtkWidget   *bottom_hbox;
  GtkWidget   *tito_clear_history_button;
  GtkWidget   *restore_defaults_button;

  pref_dialog = gtk_dialog_new_with_buttons ( "Tito preferences",
                                              NULL,
                                              GTK_DIALOG_MODAL,
                                              GTK_STOCK_OK,
                                              GTK_RESPONSE_ACCEPT,
                                              GTK_STOCK_CANCEL,
                                              GTK_RESPONSE_REJECT,
                                              NULL);

  gtk_window_set_position (GTK_WINDOW(pref_dialog), GTK_WIN_POS_CENTER_ALWAYS);
  top_hbox = gtk_hbox_new (FALSE,10);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(pref_dialog))), top_hbox, FALSE, FALSE, 2);

  //Position preferences
  position_frame = gtk_frame_new("Postion");
  position_vbox = gtk_vbox_new(TRUE,2);

  gtk_frame_set_shadow_type (GTK_FRAME(position_frame), GTK_SHADOW_ETCHED_IN);

  PREF_UI.right_top_radio = gtk_radio_button_new_with_label(NULL,"Right-Top");
  PREF_UI.middle_radio = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (PREF_UI.right_top_radio), "Middle");
  PREF_UI.specify_radio = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (PREF_UI.right_top_radio), "Specify");
  PREF_UI.pos_x_hbox = gtk_hbox_new(FALSE, 1);
  PREF_UI.pos_y_hbox = gtk_hbox_new(FALSE, 1);
  specify_alignment_x = gtk_alignment_new (1,0,0,0);
  specify_alignment_y = gtk_alignment_new (1,0,0,0);
  pos_x_label = gtk_label_new("x:");
  pos_y_label = gtk_label_new("y:");
  PREF_UI.pos_x_spin_button = gtk_spin_button_new_with_range (0, 100-PREF.WIDTH, 1);
  PREF_UI.pos_y_spin_button = gtk_spin_button_new_with_range (0, 50, 1);

  gtk_box_pack_start (GTK_BOX(top_hbox), position_frame, FALSE, FALSE, 2);
  gtk_container_add (GTK_CONTAINER(position_frame), position_vbox);
      gtk_box_pack_start (GTK_BOX (position_vbox), PREF_UI.right_top_radio, TRUE, TRUE, 2);
      gtk_box_pack_start (GTK_BOX (position_vbox), PREF_UI.middle_radio, TRUE, TRUE, 2);
      gtk_box_pack_start (GTK_BOX (position_vbox), PREF_UI.specify_radio, TRUE, TRUE, 2);

      gtk_box_pack_start (GTK_BOX (position_vbox), specify_alignment_x, TRUE, TRUE, 1);
          gtk_container_add (GTK_CONTAINER (specify_alignment_x), PREF_UI.pos_x_hbox);
              gtk_box_pack_start (GTK_BOX (PREF_UI.pos_x_hbox),pos_x_label, TRUE, TRUE, 1);
              gtk_box_pack_start (GTK_BOX (PREF_UI.pos_x_hbox),PREF_UI.pos_x_spin_button, TRUE, TRUE, 1);

      gtk_box_pack_start (GTK_BOX (position_vbox), specify_alignment_y, TRUE, TRUE, 1);
          gtk_container_add (GTK_CONTAINER (specify_alignment_y), PREF_UI.pos_y_hbox);
              gtk_box_pack_start (GTK_BOX (PREF_UI.pos_y_hbox),pos_y_label, TRUE, TRUE, 1);
              gtk_box_pack_start (GTK_BOX (PREF_UI.pos_y_hbox),PREF_UI.pos_y_spin_button, TRUE, TRUE, 1);

  //Display preferences
  display_frame = gtk_frame_new("Display");
  display_vbox = gtk_vbox_new(TRUE,2);

  gtk_frame_set_shadow_type (GTK_FRAME(display_frame), GTK_SHADOW_ETCHED_IN);

  no_of_results_hbox = gtk_hbox_new(FALSE,2);
  width_hbox = gtk_hbox_new(FALSE,2);
  opacity_hbox = gtk_hbox_new(FALSE,2);
  no_of_results_label = gtk_label_new("Results height:");
  PREF_UI.no_of_results_spin_button = gtk_spin_button_new_with_range(2,10,1);
  width_label = gtk_label_new("Tito Width:");
  PREF_UI.width_spin_button = gtk_spin_button_new_with_range(20,60,1);
  opacity_label = gtk_label_new("Tito Opacity:");
  PREF_UI.opacity_spin_button = gtk_spin_button_new_with_range(40,100,10);
  PREF_UI.autohide_check_button = gtk_check_button_new_with_label("Autohide");
  PREF_UI.show_insensitive_check_button = gtk_check_button_new_with_label("Show inert actions");
  tito_clear_history_button = gtk_button_new_with_label ("Clear history");
  restore_defaults_button = gtk_button_new_with_label ("Restore defaults");

  gtk_box_pack_start (GTK_BOX(top_hbox), display_frame, FALSE, FALSE, 2);
  gtk_container_add (GTK_CONTAINER(display_frame), display_vbox);
      gtk_box_pack_start (GTK_BOX (display_vbox), no_of_results_hbox, TRUE, TRUE, 2);
          gtk_box_pack_start (GTK_BOX (no_of_results_hbox), no_of_results_label, TRUE, TRUE, 2);
          gtk_box_pack_start (GTK_BOX (no_of_results_hbox), PREF_UI.no_of_results_spin_button, TRUE, TRUE, 2);
      gtk_box_pack_start (GTK_BOX (display_vbox), width_hbox, TRUE, TRUE, 2);
          gtk_box_pack_start (GTK_BOX (width_hbox), width_label, TRUE, TRUE, 2);
          gtk_box_pack_start (GTK_BOX (width_hbox), PREF_UI.width_spin_button, TRUE, TRUE, 2);
      gtk_box_pack_start (GTK_BOX (display_vbox), opacity_hbox, TRUE, TRUE, 2);
          gtk_box_pack_start (GTK_BOX (opacity_hbox), opacity_label, TRUE, TRUE, 2);
          gtk_box_pack_start (GTK_BOX (opacity_hbox), PREF_UI.opacity_spin_button, TRUE, TRUE, 2);
      gtk_box_pack_start (GTK_BOX (display_vbox), PREF_UI.autohide_check_button, TRUE, TRUE, 2);
      gtk_box_pack_start (GTK_BOX (display_vbox), PREF_UI.show_insensitive_check_button, TRUE, TRUE, 2);

  //Clear and restore
  bottom_hbox = gtk_hbox_new(TRUE,2);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(pref_dialog))), bottom_hbox, FALSE, FALSE, 2);
    gtk_box_pack_start (GTK_BOX (bottom_hbox), tito_clear_history_button, TRUE, TRUE, 2);
    gtk_box_pack_start (GTK_BOX (bottom_hbox), restore_defaults_button, TRUE, TRUE, 2);

  tito_set_prefereces_ui_values();
  gtk_widget_show_all (pref_dialog);

  g_signal_connect (PREF_UI.right_top_radio, "toggled", G_CALLBACK (modify_position_spins), NULL);
  g_signal_connect (PREF_UI.middle_radio, "toggled", G_CALLBACK (modify_position_spins), NULL);
  g_signal_connect (PREF_UI.specify_radio, "toggled", G_CALLBACK (modify_position_spins), NULL);
  g_signal_connect (tito_clear_history_button, "clicked", G_CALLBACK (tito_clear_history_button_clicked), NULL);
  g_signal_connect (restore_defaults_button, "clicked", G_CALLBACK (restore_defaults_button_clicked), NULL);

  if(gtk_dialog_run (GTK_DIALOG (pref_dialog)) == GTK_RESPONSE_ACCEPT)
    tito_update_preferences();

  gtk_widget_destroy (pref_dialog);
}

static GtkWidget*
tito_setup_results_list(void)
{
  gint wid1=100;
  GtkWidget *sc_win;
  GtkListStore *store;
  GtkCellRenderer *cell1;
  GtkTreeViewColumn *column1, *column2;

  sc_win= gtk_scrolled_window_new(NULL, NULL);
  store=gtk_list_store_new(N_COL, G_TYPE_STRING, G_TYPE_STRING, GTK_TYPE_ACTION,G_TYPE_BOOLEAN);
  list=gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(list),FALSE);

  cell1 = gtk_cell_renderer_pixbuf_new ();
  column1=gtk_tree_view_column_new_with_attributes(NULL,
                                                  cell1,
                                                  "stock_id", RESULT_ICON,
                                                  NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(list),column1);
  gtk_tree_view_column_add_attribute(column1, cell1, "sensitive", IS_SENSITIVE);
  gtk_tree_view_column_set_min_width(column1,22);

  cell_renderer = gtk_cell_renderer_text_new();
  column2=gtk_tree_view_column_new_with_attributes(NULL,
                                                  cell_renderer,
                                                  "markup", RESULT_DATA,
                                                  NULL);
  gtk_tree_view_column_add_attribute(column2, cell_renderer, "sensitive", IS_SENSITIVE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(list),column2);
  gtk_tree_view_column_set_max_width(column2,wid1);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sc_win),
                                 GTK_POLICY_NEVER,
                                 GTK_POLICY_AUTOMATIC);

  gtk_container_add(GTK_CONTAINER(sc_win),list);
  g_object_unref(G_OBJECT(store));
  return sc_win;
}

static gboolean
tito_search_dialog (void)
{
  GtkWidget *main_vbox, *main_hbox;
  GtkWidget *preferences_image;

  tito_dialog= gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_decorated (GTK_WINDOW(tito_dialog),FALSE);
  gtk_window_set_default_size (GTK_WINDOW(tito_dialog),(PREF.WIDTH/100)*par_width,default_height);
  tito_update_position();
  gtk_window_set_opacity (GTK_WINDOW(tito_dialog),PREF.OPACITY);
  if(!PREF.AUTO_HIDE)
    gtk_window_set_keep_above(GTK_WINDOW(tito_dialog),TRUE);

  main_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (tito_dialog), main_vbox);
  gtk_widget_show (main_vbox);

  main_hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start(GTK_BOX(main_vbox),main_hbox,FALSE,TRUE,0);
  gtk_widget_show (main_hbox);

  keyword_entry = gtk_entry_new();
  gtk_entry_set_has_frame(GTK_ENTRY(keyword_entry),FALSE);
  gtk_entry_set_icon_from_stock(GTK_ENTRY(keyword_entry),GTK_ENTRY_ICON_PRIMARY,GTK_STOCK_FIND);
  gtk_widget_show (keyword_entry);
  gtk_box_pack_start(GTK_BOX(main_hbox),keyword_entry,TRUE,TRUE,0);

  preferences_image = gtk_image_new_from_stock (GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_MENU);
  preferences_button = gtk_button_new();
  gtk_button_set_image (GTK_BUTTON(preferences_button),preferences_image);
  gtk_widget_show (preferences_image);
  gtk_widget_show (preferences_button);
  gtk_box_pack_end(GTK_BOX(main_hbox),preferences_button,FALSE,TRUE,0);

  list_view = tito_setup_results_list();
  gtk_box_pack_start(GTK_BOX(main_vbox),list_view,TRUE,TRUE,0);


  gtk_widget_set_events(tito_dialog, GDK_KEY_RELEASE_MASK);
  gtk_widget_set_events(tito_dialog, GDK_KEY_PRESS_MASK);
  gtk_widget_set_events(tito_dialog, GDK_BUTTON_PRESS_MASK);
  gtk_widget_set_events(preferences_button, GDK_BUTTON_PRESS_MASK);

  g_signal_connect(list, "row-activated", (GCallback) row_activated, NULL);
  g_signal_connect (keyword_entry, "key-release-event", G_CALLBACK (key_released), NULL);
  g_signal_connect (list, "key_press_event", G_CALLBACK (result_selected), NULL);
  g_signal_connect (preferences_button, "clicked", G_CALLBACK(context_menu_invoked),NULL);
  g_signal_connect (tito_dialog, "focus-out-event", G_CALLBACK (on_focus_out), NULL);

  gtk_widget_show (tito_dialog);
  return TRUE;
}
