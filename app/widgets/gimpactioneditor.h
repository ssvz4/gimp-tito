/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpactioneditor.h
 * Copyright (C) 2008  Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_ACTION_EDITOR_H__
#define __GIMP_ACTION_EDITOR_H__


#define GIMP_TYPE_ACTION_EDITOR            (gimp_action_editor_get_type ())
#define GIMP_ACTION_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ACTION_EDITOR, GimpActionEditor))
#define GIMP_ACTION_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ACTION_EDITOR, GimpActionEditorClass))
#define GIMP_IS_ACTION_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ACTION_EDITOR))
#define GIMP_IS_ACTION_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ACTION_EDITOR))
#define GIMP_ACTION_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ACTION_EDITOR, GimpActionEditorClass))


typedef struct _GimpActionEditorClass GimpActionEditorClass;

struct _GimpActionEditor
{
  GtkVBox    parent_instance;

  GtkWidget *view;
};

struct _GimpActionEditorClass
{
  GtkVBoxClass  parent_class;
};


GType       gimp_action_editor_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_action_editor_new      (GimpUIManager *manager,
                                         const gchar   *select_action,
                                         gboolean       show_shortcuts);


#endif  /*  __GIMP_ACTION_EDITOR_H__  */