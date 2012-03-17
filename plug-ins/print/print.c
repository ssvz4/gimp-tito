/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "print.h"
#include "print-settings.h"
#include "print-page-layout.h"
#include "print-page-setup.h"
#include "print-draw-page.h"

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_BINARY  "print"
#define PLUG_IN_ROLE    "gimp-print"
#define PRINT_PROC_NAME "file-print-gtk"


static void        query (void);
static void        run   (const gchar       *name,
                          gint               nparams,
                          const GimpParam   *param,
                          gint              *nreturn_vals,
                          GimpParam        **return_vals);

static GimpPDBStatusType  print_image       (gint32             image_ID,
                                             gboolean           interactive,
                                             GError           **error);

static void        print_show_error         (const gchar       *message);
static void        print_operation_set_name (GtkPrintOperation *operation,
                                             gint               image_ID);

static void        begin_print              (GtkPrintOperation *operation,
                                             GtkPrintContext   *context,
                                             PrintData         *data);
static void        end_print                (GtkPrintOperation *operation,
                                             GtkPrintContext   *context,
                                             gint32            *layer_ID);
static void        draw_page                (GtkPrintOperation *print,
                                             GtkPrintContext   *context,
                                             gint               page_nr,
                                             PrintData         *data);

static GtkWidget * create_custom_widget     (GtkPrintOperation *operation,
                                             PrintData         *data);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef print_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0) }" },
    { GIMP_PDB_IMAGE,    "image",    "Image to print"                       }
  };

  gimp_install_procedure (PRINT_PROC_NAME,
                          N_("Print the image"),
                          "Print the image using the GTK+ Print API.",
                          "Bill Skaggs, Sven Neumann, Stefan Röllin",
                          "Bill Skaggs <weskaggs@primate.ucdavis.edu>",
                          "2006 - 2008",
                          N_("_Print..."),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (print_args), 0,
                          print_args, NULL);

  gimp_plugin_menu_register (PRINT_PROC_NAME, "<Image>/File/Send");
  gimp_plugin_icon_register (PRINT_PROC_NAME, GIMP_ICON_TYPE_STOCK_ID,
                             (const guint8 *) GTK_STOCK_PRINT);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status;
  gint32             image_ID;
  GError            *error = NULL;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  g_thread_init (NULL);

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  image_ID = param[1].data.d_int32;

  if (strcmp (name, PRINT_PROC_NAME) == 0)
    {
      status = print_image (image_ID, run_mode == GIMP_RUN_INTERACTIVE, &error);

      if (error && run_mode == GIMP_RUN_INTERACTIVE)
        {
          print_show_error (error->message);
        }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}

static GimpPDBStatusType
print_image (gint32     image_ID,
             gboolean   interactive,
             GError   **error)
{
  GtkPrintOperation       *operation;
  GtkPrintOperationResult  result;
  gint32                   layer;
  PrintData                data;

  /*  create a print layer from the projection  */
  layer = gimp_layer_new_from_visible (image_ID, image_ID, PRINT_PROC_NAME);

  operation = gtk_print_operation_new ();

  gtk_print_operation_set_n_pages (operation, 1);
  print_operation_set_name (operation, image_ID);

  print_page_setup_load (operation, image_ID);

  /* fill in the PrintData struct */
  data.image_id        = image_ID;
  data.drawable_id     = layer;
  data.unit            = gimp_get_default_unit ();
  data.image_unit      = gimp_image_get_unit (image_ID);
  data.offset_x        = 0;
  data.offset_y        = 0;
  data.center          = CENTER_BOTH;
  data.use_full_page   = FALSE;
  data.draw_crop_marks = FALSE;
  data.operation       = operation;

  gimp_image_get_resolution (image_ID, &data.xres, &data.yres);

  print_settings_load (&data);

  gtk_print_operation_set_unit (operation, GTK_UNIT_PIXEL);

  g_signal_connect (operation, "begin-print",
                    G_CALLBACK (begin_print),
                    &data);
  g_signal_connect (operation, "draw-page",
                    G_CALLBACK (draw_page),
                    &data);
  g_signal_connect (operation, "end-print",
                    G_CALLBACK (end_print),
                    &layer);

  if (interactive)
    {
      gimp_ui_init (PLUG_IN_BINARY, FALSE);

      g_signal_connect_swapped (operation, "end-print",
                                G_CALLBACK (print_settings_save),
                                &data);

      g_signal_connect (operation, "create-custom-widget",
                        G_CALLBACK (create_custom_widget),
                        &data);

      gtk_print_operation_set_custom_tab_label (operation, _("Image Settings"));

      gtk_print_operation_set_embed_page_setup (operation, TRUE);

      result = gtk_print_operation_run (operation,
                                        GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
                                        NULL, error);

      if (result == GTK_PRINT_OPERATION_RESULT_APPLY ||
          result == GTK_PRINT_OPERATION_RESULT_IN_PROGRESS)
        {
          print_page_setup_save (operation, image_ID);
        }
    }
  else
    {
      result = gtk_print_operation_run (operation,
                                        GTK_PRINT_OPERATION_ACTION_PRINT,
                                        NULL, error);
    }

  g_object_unref (operation);

  if (gimp_item_is_valid (layer))
    gimp_item_delete (layer);

  switch (result)
    {
    case GTK_PRINT_OPERATION_RESULT_APPLY:
    case GTK_PRINT_OPERATION_RESULT_IN_PROGRESS:
      return GIMP_PDB_SUCCESS;

    case GTK_PRINT_OPERATION_RESULT_CANCEL:
      return GIMP_PDB_CANCEL;

    case GTK_PRINT_OPERATION_RESULT_ERROR:
      return GIMP_PDB_EXECUTION_ERROR;
    }

  return GIMP_PDB_EXECUTION_ERROR;
}

static void
print_show_error (const gchar *message)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (NULL, 0,
                                   GTK_MESSAGE_ERROR,
                                   GTK_BUTTONS_OK,
				   "%s",
                                   _("An error occurred while trying to print:"));

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            "%s", message);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
print_operation_set_name (GtkPrintOperation *operation,
                          gint               image_ID)
{
  gchar *name = gimp_image_get_name (image_ID);

  gtk_print_operation_set_job_name (operation, name);

  g_free (name);
}

static void
begin_print (GtkPrintOperation *operation,
             GtkPrintContext   *context,
             PrintData         *data)
{
  gtk_print_operation_set_use_full_page (operation, data->use_full_page);

  gimp_progress_init (_("Printing"));
}

static void
end_print (GtkPrintOperation *operation,
           GtkPrintContext   *context,
           gint32            *layer_ID)
{
  /* we don't need the print layer any longer, delete it */
  if (gimp_item_is_valid (*layer_ID))
    {
      gimp_item_delete (*layer_ID);
      *layer_ID = -1;
    }

  gimp_progress_end ();

  /* generate events to solve the problems described in bug #466928 */
  g_timeout_add_seconds (1, (GSourceFunc) gtk_true, NULL);
}

static void
draw_page (GtkPrintOperation *operation,
           GtkPrintContext   *context,
           gint               page_nr,
           PrintData         *data)
{
  print_draw_page (context, data);

  gimp_progress_update (1.0);
}

/*
 * This callback creates a "custom" widget that gets inserted into the
 * print operation dialog.
 */
static GtkWidget *
create_custom_widget (GtkPrintOperation *operation,
                      PrintData         *data)
{
  return print_page_layout_gui (data, PRINT_PROC_NAME);
}
