/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include <stdlib.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpboundary.h"
#include "core/gimpcontext.h"
#include "core/gimpchannel.h"
#include "core/gimpdrawable-transform.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimpitem-linked.h"
#include "core/gimplayer.h"
#include "core/gimpprogress.h"
#include "core/gimptoolinfo.h"
#include "core/gimp-transform-utils.h"
#include "core/gimp-utils.h"

#include "vectors/gimpvectors.h"
#include "vectors/gimpstroke.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvasgroup.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"
#include "display/gimptooldialog.h"

#include "gimptoolcontrol.h"
#include "gimpperspectivetool.h"
#include "gimpunifiedtransformtool.h"
#include "gimptransformoptions.h"
#include "gimptransformtool.h"
#include "gimptransformtoolundo.h"

#include "gimp-intl.h"


#define RESPONSE_RESET  1
#define RESPONSE_UNDO   2
#define RESPONSE_REDO   3
#define MIN_HANDLE_SIZE 6


static void      gimp_transform_tool_finalize               (GObject               *object);

static gboolean  gimp_transform_tool_initialize             (GimpTool              *tool,
                                                             GimpDisplay           *display,
                                                             GError               **error);
static void      gimp_transform_tool_control                (GimpTool              *tool,
                                                             GimpToolAction         action,
                                                             GimpDisplay           *display);
static void      gimp_transform_tool_button_press           (GimpTool              *tool,
                                                             const GimpCoords      *coords,
                                                             guint32                time,
                                                             GdkModifierType        state,
                                                             GimpButtonPressType    press_type,
                                                             GimpDisplay           *display);
static void      gimp_transform_tool_button_release         (GimpTool              *tool,
                                                             const GimpCoords      *coords,
                                                             guint32                time,
                                                             GdkModifierType        state,
                                                             GimpButtonReleaseType  release_type,
                                                             GimpDisplay           *display);
static void      gimp_transform_tool_motion                 (GimpTool              *tool,
                                                             const GimpCoords      *coords,
                                                             guint32                time,
                                                             GdkModifierType        state,
                                                             GimpDisplay           *display);
static gboolean  gimp_transform_tool_key_press              (GimpTool              *tool,
                                                             GdkEventKey           *kevent,
                                                             GimpDisplay           *display);
static void      gimp_transform_tool_modifier_key           (GimpTool              *tool,
                                                             GdkModifierType        key,
                                                             gboolean               press,
                                                             GdkModifierType        state,
                                                             GimpDisplay           *display);
static void      gimp_transform_tool_oper_update            (GimpTool              *tool,
                                                             const GimpCoords      *coords,
                                                             GdkModifierType        state,
                                                             gboolean               proximity,
                                                             GimpDisplay           *display);
static void      gimp_transform_tool_cursor_update          (GimpTool              *tool,
                                                             const GimpCoords      *coords,
                                                             GdkModifierType        state,
                                                             GimpDisplay           *display);
static void      gimp_transform_tool_options_notify         (GimpTool              *tool,
                                                             GimpToolOptions       *options,
                                                             const GParamSpec      *pspec);

static void      gimp_transform_tool_draw                   (GimpDrawTool          *draw_tool);

static void      gimp_transform_tool_dialog_update          (GimpTransformTool     *tr_tool);

static GeglBuffer *
                 gimp_transform_tool_real_transform         (GimpTransformTool     *tr_tool,
                                                             GimpItem              *item,
                                                             GeglBuffer            *orig_buffer,
                                                             gint                   orig_offset_x,
                                                             gint                   orig_offset_y,
                                                             gint                  *new_offset_x,
                                                             gint                  *new_offset_y);
static void      gimp_transform_tool_real_draw_gui          (GimpTransformTool     *tr_tool,
                                                             gint                   handle_w,
                                                             gint                   handle_h);
static TransformAction
                 gimp_transform_tool_real_pick_function     (GimpTransformTool     *tr_tool,
                                                             const GimpCoords      *coords,
                                                             GdkModifierType        state,
                                                             GimpDisplay           *display);

static void      gimp_transform_tool_set_function           (GimpTransformTool     *tr_tool,
                                                             TransformAction        function);
static void      gimp_transform_tool_bounds                 (GimpTransformTool     *tr_tool,
                                                             GimpDisplay           *display);
static void      gimp_transform_tool_dialog                 (GimpTransformTool     *tr_tool);
static void      gimp_transform_tool_prepare                (GimpTransformTool     *tr_tool,
                                                             GimpDisplay           *display);
static void      gimp_transform_tool_transform              (GimpTransformTool     *tr_tool,
                                                             GimpDisplay           *display);
static void      gimp_transform_tool_transform_bounding_box (GimpTransformTool     *tr_tool);

static void      gimp_transform_tool_handles_recalc         (GimpTransformTool     *tr_tool,
                                                             GimpDisplay           *display,
                                                             gint                  *handle_w,
                                                             gint                  *handle_h);
static void      gimp_transform_tool_response               (GtkWidget             *widget,
                                                             gint                   response_id,
                                                             GimpTransformTool     *tr_tool);

static void free_trans         (gpointer           data);
static void update_sensitivity (GimpTransformTool *tr_tool);


G_DEFINE_TYPE (GimpTransformTool, gimp_transform_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_transform_tool_parent_class


static void
gimp_transform_tool_class_init (GimpTransformToolClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class   = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_class   = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->finalize          = gimp_transform_tool_finalize;

  tool_class->initialize          = gimp_transform_tool_initialize;
  tool_class->control             = gimp_transform_tool_control;
  tool_class->button_press        = gimp_transform_tool_button_press;
  tool_class->button_release      = gimp_transform_tool_button_release;
  tool_class->motion              = gimp_transform_tool_motion;
  tool_class->key_press           = gimp_transform_tool_key_press;
  tool_class->modifier_key        = gimp_transform_tool_modifier_key;
  tool_class->active_modifier_key = gimp_transform_tool_modifier_key;
  tool_class->oper_update         = gimp_transform_tool_oper_update;
  tool_class->cursor_update       = gimp_transform_tool_cursor_update;
  tool_class->options_notify      = gimp_transform_tool_options_notify;

  draw_class->draw                = gimp_transform_tool_draw;

  klass->dialog                   = NULL;
  klass->dialog_update            = NULL;
  klass->prepare                  = NULL;
  klass->motion                   = NULL;
  klass->recalc_matrix            = NULL;
  klass->get_undo_desc            = NULL;
  klass->transform                = gimp_transform_tool_real_transform;
  klass->pick_function            = gimp_transform_tool_real_pick_function;
  klass->draw_gui                 = gimp_transform_tool_real_draw_gui;
}

static void
gimp_transform_tool_init (GimpTransformTool *tr_tool)
{
  GimpTool *tool = GIMP_TOOL (tr_tool);

  gimp_tool_control_set_action_value_1 (tool->control,
                                        "tools/tools-transform-preview-opacity-set");

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask  (tool->control,
                                     GIMP_DIRTY_IMAGE_SIZE |
                                     GIMP_DIRTY_DRAWABLE   |
                                     GIMP_DIRTY_SELECTION  |
                                     GIMP_DIRTY_ACTIVE_DRAWABLE);
  gimp_tool_control_set_precision   (tool->control,
                                     GIMP_CURSOR_PRECISION_SUBPIXEL);

  tr_tool->function      = TRANSFORM_CREATING;
  tr_tool->progress_text = _("Transforming");

  gimp_matrix3_identity (&tr_tool->transform);
}

static void
gimp_transform_tool_finalize (GObject *object)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (object);

  if (tr_tool->dialog)
    {
      gtk_widget_destroy (tr_tool->dialog);
      tr_tool->dialog = NULL;
    }

  g_list_free_full (tr_tool->redo_list, free_trans);
  g_list_free_full (tr_tool->undo_list, free_trans);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_transform_tool_initialize (GimpTool     *tool,
                                GimpDisplay  *display,
                                GError      **error)
{
  GimpTransformTool *tr_tool  = GIMP_TRANSFORM_TOOL (tool);
  GimpImage         *image    = gimp_display_get_image (display);
  GimpDrawable      *drawable = gimp_image_get_active_drawable (image);

  if (! GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  if (gimp_item_is_content_locked (GIMP_ITEM (drawable)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("The active layer's pixels are locked."));
      return FALSE;
    }

  if (display != tool->display)
    {
      gint i;

      /*  Set the pointer to the active display  */
      tool->display  = display;
      tool->drawable = drawable;

      /*  Initialize the transform tool dialog */
      if (! tr_tool->dialog)
        gimp_transform_tool_dialog (tr_tool);

      /*  Find the transform bounds for some tools (like scale,
       *  perspective) that actually need the bounds for initializing
       */
      gimp_transform_tool_bounds (tr_tool, display);

      /*  Inizialize the tool-specific trans_info, and adjust the
       *  tool dialog
       */
      gimp_transform_tool_prepare (tr_tool, display);

      /*  Recalculate the transform tool  */
      gimp_transform_tool_recalc_matrix (tr_tool);

      /*  start drawing the bounding box and handles...  */
      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);

      tr_tool->function = TRANSFORM_CREATING;

      /* Initialize undo and redo lists */
      tr_tool->undo_list = g_list_prepend (NULL, g_slice_new (TransInfo));
      tr_tool->redo_list = NULL;
      tr_tool->old_trans_info = g_list_last (tr_tool->undo_list)->data;
      tr_tool->prev_trans_info = g_list_first (tr_tool->undo_list)->data;
      update_sensitivity (tr_tool);

      /*  Save the current transformation info  */
      for (i = 0; i < TRANS_INFO_SIZE; i++)
        {
          (*tr_tool->prev_trans_info)[i] = tr_tool->trans_info[i];
        }
    }

  return TRUE;
}

static void
gimp_transform_tool_control (GimpTool       *tool,
                             GimpToolAction  action,
                             GimpDisplay    *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
      break;

    case GIMP_TOOL_ACTION_RESUME:
      gimp_transform_tool_bounds (tr_tool, display);
      gimp_transform_tool_recalc_matrix (tr_tool);
      break;

    case GIMP_TOOL_ACTION_HALT:
      tr_tool->function = TRANSFORM_CREATING;

      if (tr_tool->dialog)
        gimp_dialog_factory_hide_dialog (tr_tool->dialog);

      tool->drawable = NULL;
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_transform_tool_button_press (GimpTool            *tool,
                                  const GimpCoords    *coords,
                                  guint32              time,
                                  GdkModifierType      state,
                                  GimpButtonPressType  press_type,
                                  GimpDisplay         *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);

  if (tr_tool->function == TRANSFORM_CREATING)
    GIMP_TOOL_GET_CLASS (tool)->oper_update (tool, coords, state, TRUE, display);

  tr_tool->lastx = tr_tool->mousex = coords->x;
  tr_tool->lasty = tr_tool->mousey = coords->y;

  gimp_tool_control_activate (tool->control);
}

void gimp_transform_tool_push_internal_undo (GimpTransformTool *tr_tool)
{
  gint i;

  /* push current state on the undo list and set this state as the
   * current state, but avoid doing this if there were no changes
   */
  for (i = 0; i < TRANS_INFO_SIZE; i++)
    if ((*tr_tool->prev_trans_info)[i] != tr_tool->trans_info[i])
      break;

  if (i < TRANS_INFO_SIZE)
    {
      tr_tool->prev_trans_info = g_slice_new (TransInfo);
      for (i = 0; i < TRANS_INFO_SIZE; i++)
        (*tr_tool->prev_trans_info)[i] = tr_tool->trans_info[i];
      tr_tool->undo_list = g_list_prepend (tr_tool->undo_list,
                                           tr_tool->prev_trans_info);

      /* If we undid anything and started interacting, we have to
       * discard the redo history
       */
      g_list_free_full (tr_tool->redo_list, free_trans);
      tr_tool->redo_list = NULL;

      update_sensitivity (tr_tool);
    }
}

static void
gimp_transform_tool_button_release (GimpTool              *tool,
                                    const GimpCoords      *coords,
                                    guint32                time,
                                    GdkModifierType        state,
                                    GimpButtonReleaseType  release_type,
                                    GimpDisplay           *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);
  gint               i;

  gimp_tool_control_halt (tool->control);

  /*  if we are creating, there is nothing to be done...exit  */
  if (tr_tool->function == TRANSFORM_CREATING && tr_tool->use_grid)
    return;

  if (release_type != GIMP_BUTTON_RELEASE_CANCEL)
    {
      /* We're done with an interaction, save it on the undo list */
      gimp_transform_tool_push_internal_undo (tr_tool);
    }
  else
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      /*  Restore the last saved state  */
      for (i = 0; i < TRANS_INFO_SIZE; i++)
        tr_tool->trans_info[i] = (*tr_tool->prev_trans_info)[i];

      /*  reget the selection bounds  */
      gimp_transform_tool_bounds (tr_tool, display);

      /*  recalculate the tool's transformation matrix  */
      gimp_transform_tool_recalc_matrix (tr_tool);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
}

static void
gimp_transform_tool_motion (GimpTool         *tool,
                            const GimpCoords *coords,
                            guint32           time,
                            GdkModifierType   state,
                            GimpDisplay      *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);

  /*  if we are creating, there is nothing to be done so exit.  */
  if (tr_tool->function == TRANSFORM_CREATING || ! tr_tool->use_grid)
    return;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  tr_tool->curx = coords->x;
  tr_tool->cury = coords->y;

  /*  recalculate the tool's transformation matrix  */
  if (GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->motion)
    {
      GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->motion (tr_tool);

      gimp_transform_tool_recalc_matrix (tr_tool);
    }

  tr_tool->lastx = tr_tool->curx;
  tr_tool->lasty = tr_tool->cury;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static gboolean
gimp_transform_tool_key_press (GimpTool    *tool,
                               GdkEventKey *kevent,
                               GimpDisplay *display)
{
  GimpTransformTool *trans_tool = GIMP_TRANSFORM_TOOL (tool);
  GimpDrawTool      *draw_tool  = GIMP_DRAW_TOOL (tool);

  if (display == draw_tool->display)
    {
      switch (kevent->keyval)
        {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_ISO_Enter:
          gimp_transform_tool_response (NULL, GTK_RESPONSE_OK, trans_tool);
          return TRUE;

        case GDK_KEY_BackSpace:
          gimp_transform_tool_response (NULL, RESPONSE_UNDO, trans_tool);
          return TRUE;

        case GDK_KEY_space:
          gimp_transform_tool_response (NULL, RESPONSE_REDO, trans_tool);
          return TRUE;

        case GDK_KEY_Escape:
          gimp_transform_tool_response (NULL, GTK_RESPONSE_CANCEL, trans_tool);
          return TRUE;
        }
    }

  return FALSE;
}

static void
gimp_transform_tool_modifier_key (GimpTool        *tool,
                                  GdkModifierType  key,
                                  gboolean         press,
                                  GdkModifierType  state,
                                  GimpDisplay     *display)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);

  if (key == gimp_get_constrain_behavior_mask ())
    {
      g_object_set (options,
                    "frompivot-scale", ! options->frompivot_scale,
                    NULL);
      g_object_set (options,
                    "frompivot-shear", ! options->frompivot_shear,
                    NULL);
      g_object_set (options,
                    "frompivot-perspective", ! options->frompivot_perspective,
                    NULL);
    }

  if (key == gimp_get_extend_selection_mask ())
    {
      g_object_set (options,
                    "cornersnap", ! options->cornersnap,
                    NULL);

      g_object_set (options,
                    "constrain-move", ! options->constrain_move,
                    NULL);
      g_object_set (options,
                    "constrain-scale", ! options->constrain_scale,
                    NULL);
      g_object_set (options,
                    "constrain-rotate", ! options->constrain_rotate,
                    NULL);
      g_object_set (options,
                    "constrain-shear", ! options->constrain_shear,
                    NULL);
      g_object_set (options,
                    "constrain-perspective", ! options->constrain_perspective,
                    NULL);
    }
}

static TransformAction
gimp_transform_tool_real_pick_function (GimpTransformTool *tr_tool,
                                        const GimpCoords  *coords,
                                        GdkModifierType    state,
                                        GimpDisplay       *display)
{
  GimpDrawTool   *draw_tool = GIMP_DRAW_TOOL (tr_tool);
  TransformAction function  = TRANSFORM_HANDLE_NONE;

  if (tr_tool->use_handles)
    {
        gdouble closest_dist;
        gdouble dist;

        dist = gimp_draw_tool_calc_distance_square (draw_tool, display,
                                                    coords->x, coords->y,
                                                    tr_tool->tx1, tr_tool->ty1);
        closest_dist = dist;
        function = TRANSFORM_HANDLE_NW;

        dist = gimp_draw_tool_calc_distance_square (draw_tool, display,
                                                    coords->x, coords->y,
                                                    tr_tool->tx2, tr_tool->ty2);
        if (dist < closest_dist)
          {
            closest_dist = dist;
            function = TRANSFORM_HANDLE_NE;
          }

        dist = gimp_draw_tool_calc_distance_square (draw_tool, display,
                                                    coords->x, coords->y,
                                                    tr_tool->tx3, tr_tool->ty3);
        if (dist < closest_dist)
          {
            closest_dist = dist;
            function = TRANSFORM_HANDLE_SW;
          }

        dist = gimp_draw_tool_calc_distance_square (draw_tool, display,
                                                    coords->x, coords->y,
                                                    tr_tool->tx4, tr_tool->ty4);
        if (dist < closest_dist)
          {
            closest_dist = dist;
            function = TRANSFORM_HANDLE_SE;
          }

        if (tr_tool->use_mid_handles)
          {
            if (gimp_canvas_item_hit (tr_tool->handles[TRANSFORM_HANDLE_N],
                                      coords->x, coords->y))
              {
                function = TRANSFORM_HANDLE_N;
              }
            else if (gimp_canvas_item_hit (tr_tool->handles[TRANSFORM_HANDLE_E],
                                           coords->x, coords->y))
              {
                function = TRANSFORM_HANDLE_E;
              }
            else if (gimp_canvas_item_hit (tr_tool->handles[TRANSFORM_HANDLE_S],
                                           coords->x, coords->y))
              {
                function = TRANSFORM_HANDLE_S;
              }
            else if (gimp_canvas_item_hit (tr_tool->handles[TRANSFORM_HANDLE_W],
                                           coords->x, coords->y))
              {
                function = TRANSFORM_HANDLE_W;
              }
          }
    }

  if (tr_tool->use_pivot)
    {
      if (gimp_canvas_item_hit (tr_tool->handles[TRANSFORM_HANDLE_PIVOT],
                                coords->x, coords->y))
        {
          function = TRANSFORM_HANDLE_PIVOT;
        }
    }

  if (tr_tool->use_center &&
      gimp_canvas_item_hit (tr_tool->handles[TRANSFORM_HANDLE_CENTER],
                            coords->x, coords->y))
    {
      function = TRANSFORM_HANDLE_CENTER;
    }

  return function;
}

static void
gimp_transform_tool_oper_update (GimpTool         *tool,
                                 const GimpCoords *coords,
                                 GdkModifierType   state,
                                 gboolean          proximity,
                                 GimpDisplay      *display)
{
  GimpTransformTool *tr_tool   = GIMP_TRANSFORM_TOOL (tool);
  GimpDrawTool      *draw_tool = GIMP_DRAW_TOOL (tool);
  TransformAction    function  = TRANSFORM_HANDLE_NONE;

  if (display != tool->display || draw_tool->item == NULL)
    {
      gimp_transform_tool_set_function (tr_tool, function);
      return;
    }

  function = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->pick_function (tr_tool, coords, state, display);

  gimp_transform_tool_set_function (tr_tool, function);
}

static void
gimp_transform_tool_cursor_update (GimpTool         *tool,
                                   const GimpCoords *coords,
                                   GdkModifierType   state,
                                   GimpDisplay      *display)
{
  GimpTransformTool    *tr_tool = GIMP_TRANSFORM_TOOL (tool);
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);
  GimpCursorType        cursor;
  GimpCursorModifier    modifier = GIMP_CURSOR_MODIFIER_NONE;
  GimpImage            *image    = gimp_display_get_image (display);

  cursor = gimp_tool_control_get_cursor (tool->control);

  if (tr_tool->use_handles)
    {
      switch (tr_tool->function)
        {
        case TRANSFORM_HANDLE_NW:
          cursor = GIMP_CURSOR_CORNER_TOP_LEFT;
          break;

        case TRANSFORM_HANDLE_NE:
          cursor = GIMP_CURSOR_CORNER_TOP_RIGHT;
          break;

        case TRANSFORM_HANDLE_SW:
          cursor = GIMP_CURSOR_CORNER_BOTTOM_LEFT;
          break;

        case TRANSFORM_HANDLE_SE:
          cursor = GIMP_CURSOR_CORNER_BOTTOM_RIGHT;
          break;

        case TRANSFORM_HANDLE_N:
          cursor = GIMP_CURSOR_SIDE_TOP;
          break;

        case TRANSFORM_HANDLE_E:
          cursor = GIMP_CURSOR_SIDE_RIGHT;
          break;

        case TRANSFORM_HANDLE_S:
          cursor = GIMP_CURSOR_SIDE_BOTTOM;
          break;

        case TRANSFORM_HANDLE_W:
          cursor = GIMP_CURSOR_SIDE_LEFT;
          break;

        default:
          cursor = GIMP_CURSOR_CROSSHAIR_SMALL;
          break;
        }
    }

  if (tr_tool->use_center && tr_tool->function == TRANSFORM_HANDLE_CENTER)
    {
      modifier = GIMP_CURSOR_MODIFIER_MOVE;
    }

  if (GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->cursor_update)
    {
      GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->cursor_update (tr_tool, &cursor, &modifier);
    }

  switch (options->type)
    {
      GimpDrawable *drawable;

    case GIMP_TRANSFORM_TYPE_LAYER:
      drawable = gimp_image_get_active_drawable (image);
      if (gimp_item_is_content_locked (GIMP_ITEM (drawable)))
        modifier = GIMP_CURSOR_MODIFIER_BAD;
      break;

    case GIMP_TRANSFORM_TYPE_SELECTION:
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      if (! gimp_image_get_active_vectors (image))
        modifier = GIMP_CURSOR_MODIFIER_BAD;
      break;
    }

  gimp_tool_control_set_cursor          (tool->control, cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_transform_tool_options_notify (GimpTool         *tool,
                                    GimpToolOptions  *options,
                                    const GParamSpec *pspec)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);

  GIMP_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (tr_tool->use_grid)
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tr_tool));

      if (! strcmp (pspec->name, "type") ||
          ! strcmp (pspec->name, "direction"))
        {
          if (tr_tool->function != TRANSFORM_CREATING)
            {
              if (tool->display)
                {
                  /*  reget the selection bounds  */
                  gimp_transform_tool_bounds (tr_tool, tool->display);

                  /*  recalculate the tool's transformation matrix  */
                  gimp_transform_tool_recalc_matrix (tr_tool);
                }
            }
        }

      if (tr_tool->function != TRANSFORM_CREATING)
        {
          gimp_transform_tool_transform_bounding_box (tr_tool);
        }

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tr_tool));
    }

  if (g_str_has_prefix (pspec->name, "constrain-") ||
      g_str_has_prefix (pspec->name, "frompivot-") ||
      ! strcmp (pspec->name, "fixedpivot") ||
      ! strcmp (pspec->name, "cornersnap"))
    {
      gimp_transform_tool_dialog_update (tr_tool);
    }
}

static void
gimp_transform_tool_real_draw_gui (GimpTransformTool *tr_tool, gint handle_w, gint handle_h)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tr_tool);

  if (tr_tool->use_handles)
    {
      /*  draw the tool handles  */
      tr_tool->handles[TRANSFORM_HANDLE_NW] =
        gimp_draw_tool_add_handle (draw_tool,
                                   GIMP_HANDLE_SQUARE,
                                   tr_tool->tx1, tr_tool->ty1,
                                   handle_w, handle_h,
                                   GIMP_HANDLE_ANCHOR_CENTER);

      tr_tool->handles[TRANSFORM_HANDLE_NE] =
        gimp_draw_tool_add_handle (draw_tool,
                                   GIMP_HANDLE_SQUARE,
                                   tr_tool->tx2, tr_tool->ty2,
                                   handle_w, handle_h,
                                   GIMP_HANDLE_ANCHOR_CENTER);

      tr_tool->handles[TRANSFORM_HANDLE_SW] =
        gimp_draw_tool_add_handle (draw_tool,
                                   GIMP_HANDLE_SQUARE,
                                   tr_tool->tx3, tr_tool->ty3,
                                   handle_w, handle_h,
                                   GIMP_HANDLE_ANCHOR_CENTER);

      tr_tool->handles[TRANSFORM_HANDLE_SE] =
        gimp_draw_tool_add_handle (draw_tool,
                                   GIMP_HANDLE_SQUARE,
                                   tr_tool->tx4, tr_tool->ty4,
                                   handle_w, handle_h,
                                   GIMP_HANDLE_ANCHOR_CENTER);

      if (tr_tool->use_mid_handles)
        {
          gdouble x, y;

          x = (tr_tool->tx1 + tr_tool->tx2) / 2.0;
          y = (tr_tool->ty1 + tr_tool->ty2) / 2.0;

          tr_tool->handles[TRANSFORM_HANDLE_N] =
            gimp_draw_tool_add_handle (draw_tool,
                                       GIMP_HANDLE_SQUARE,
                                       x, y,
                                       handle_w, handle_h,
                                       GIMP_HANDLE_ANCHOR_CENTER);

          x = (tr_tool->tx2 + tr_tool->tx4) / 2.0;
          y = (tr_tool->ty2 + tr_tool->ty4) / 2.0;

          tr_tool->handles[TRANSFORM_HANDLE_E] =
            gimp_draw_tool_add_handle (draw_tool,
                                       GIMP_HANDLE_SQUARE,
                                       x, y,
                                       handle_w, handle_h,
                                       GIMP_HANDLE_ANCHOR_CENTER);

          x = (tr_tool->tx3 + tr_tool->tx4) / 2.0;
          y = (tr_tool->ty3 + tr_tool->ty4) / 2.0;

          tr_tool->handles[TRANSFORM_HANDLE_S] =
            gimp_draw_tool_add_handle (draw_tool,
                                       GIMP_HANDLE_SQUARE,
                                       x, y,
                                       handle_w, handle_h,
                                       GIMP_HANDLE_ANCHOR_CENTER);

          x = (tr_tool->tx3 + tr_tool->tx1) / 2.0;
          y = (tr_tool->ty3 + tr_tool->ty1) / 2.0;

          tr_tool->handles[TRANSFORM_HANDLE_W] =
            gimp_draw_tool_add_handle (draw_tool,
                                       GIMP_HANDLE_SQUARE,
                                       x, y,
                                       handle_w, handle_h,
                                       GIMP_HANDLE_ANCHOR_CENTER);
        }
    }

  if (tr_tool->use_pivot)
    {
      GimpCanvasGroup *stroke_group;
      gint d = MIN (handle_w, handle_h);
      if (tr_tool->use_center)
        d *= 2; /* so you can grab it from under the center handle */

      stroke_group = gimp_draw_tool_add_stroke_group (draw_tool);

      tr_tool->handles[TRANSFORM_HANDLE_PIVOT] = GIMP_CANVAS_ITEM (stroke_group);

      gimp_draw_tool_push_group (draw_tool, stroke_group);

      gimp_draw_tool_add_handle (draw_tool,
                                 GIMP_HANDLE_CIRCLE,
                                 tr_tool->tpx, tr_tool->tpy,
                                 d, d,
                                 GIMP_HANDLE_ANCHOR_CENTER);
      gimp_draw_tool_add_handle (draw_tool,
                                 GIMP_HANDLE_CROSS,
                                 tr_tool->tpx, tr_tool->tpy,
                                 d, d,
                                 GIMP_HANDLE_ANCHOR_CENTER);

      gimp_draw_tool_pop_group (draw_tool);
    }

  /*  draw the center  */
  if (tr_tool->use_center)
    {
      GimpCanvasGroup *stroke_group;
      gint             d = MIN (handle_w, handle_h);

      stroke_group = gimp_draw_tool_add_stroke_group (draw_tool);

      tr_tool->handles[TRANSFORM_HANDLE_CENTER] = GIMP_CANVAS_ITEM (stroke_group);

      gimp_draw_tool_push_group (draw_tool, stroke_group);

      gimp_draw_tool_add_handle (draw_tool,
                                 GIMP_HANDLE_SQUARE,
                                 tr_tool->tcx, tr_tool->tcy,
                                 d, d,
                                 GIMP_HANDLE_ANCHOR_CENTER);
      gimp_draw_tool_add_handle (draw_tool,
                                 GIMP_HANDLE_CROSS,
                                 tr_tool->tcx, tr_tool->tcy,
                                 d, d,
                                 GIMP_HANDLE_ANCHOR_CENTER);

      gimp_draw_tool_pop_group (draw_tool);
    }
}

static void
gimp_transform_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool             *tool    = GIMP_TOOL (draw_tool);
  GimpTransformTool    *tr_tool = GIMP_TRANSFORM_TOOL (draw_tool);
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);
  GimpImage            *image   = gimp_display_get_image (tool->display);
  gint                  handle_w;
  gint                  handle_h;
  gint                  i;

  for (i = 0; i < G_N_ELEMENTS (tr_tool->handles); i++)
    tr_tool->handles[i] = NULL;

  if (tr_tool->use_grid)
    {
      if (gimp_transform_options_show_preview (options))
        {
          GimpMatrix3 matrix = tr_tool->transform;

          if (options->direction == GIMP_TRANSFORM_BACKWARD)
            gimp_matrix3_invert (&matrix);

          gimp_draw_tool_add_transform_preview (draw_tool,
                                                tool->drawable,
                                                &matrix,
                                                tr_tool->x1,
                                                tr_tool->y1,
                                                tr_tool->x2,
                                                tr_tool->y2,
                                                GIMP_IS_PERSPECTIVE_TOOL (tr_tool) ||
                                                GIMP_IS_UNIFIED_TRANSFORM_TOOL (tr_tool),
                                                options->preview_opacity);
        }

      gimp_draw_tool_add_transform_guides (draw_tool,
                                           &tr_tool->transform,
                                           options->grid_type,
                                           options->grid_size,
                                           tr_tool->x1,
                                           tr_tool->y1,
                                           tr_tool->x2,
                                           tr_tool->y2);
    }

  gimp_transform_tool_handles_recalc (tr_tool, tool->display,
                                      &handle_w, &handle_h);

  GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->draw_gui (tr_tool, handle_w, handle_h);

  if (tr_tool->handles[tr_tool->function])
    {
      gimp_canvas_item_set_highlight (tr_tool->handles[tr_tool->function],
                                      TRUE);
    }

  if (options->type == GIMP_TRANSFORM_TYPE_SELECTION)
    {
      GimpMatrix3         matrix = tr_tool->transform;
      const GimpBoundSeg *orig_in;
      const GimpBoundSeg *orig_out;
      GimpBoundSeg       *segs_in;
      GimpBoundSeg       *segs_out;
      gint                num_segs_in;
      gint                num_segs_out;

      gimp_channel_boundary (gimp_image_get_mask (image),
                             &orig_in, &orig_out,
                             &num_segs_in, &num_segs_out,
                             0, 0, 0, 0);

      segs_in  = g_memdup (orig_in,  num_segs_in  * sizeof (GimpBoundSeg));
      segs_out = g_memdup (orig_out, num_segs_out * sizeof (GimpBoundSeg));

      if (segs_in)
        {
          for (i = 0; i < num_segs_in; i++)
            {
              gdouble tx, ty;

              gimp_matrix3_transform_point (&matrix,
                                            segs_in[i].x1, segs_in[i].y1,
                                            &tx, &ty);
              segs_in[i].x1 = RINT (tx);
              segs_in[i].y1 = RINT (ty);

              gimp_matrix3_transform_point (&matrix,
                                            segs_in[i].x2, segs_in[i].y2,
                                            &tx, &ty);
              segs_in[i].x2 = RINT (tx);
              segs_in[i].y2 = RINT (ty);
            }

          gimp_draw_tool_add_boundary (draw_tool,
                                       segs_in, num_segs_in,
                                       NULL,
                                       0, 0);
          g_free (segs_in);
        }

      if (segs_out)
        {
          for (i = 0; i < num_segs_out; i++)
            {
              gdouble tx, ty;

              gimp_matrix3_transform_point (&matrix,
                                            segs_out[i].x1, segs_out[i].y1,
                                            &tx, &ty);
              segs_out[i].x1 = RINT (tx);
              segs_out[i].y1 = RINT (ty);

              gimp_matrix3_transform_point (&matrix,
                                            segs_out[i].x2, segs_out[i].y2,
                                            &tx, &ty);
              segs_out[i].x2 = RINT (tx);
              segs_out[i].y2 = RINT (ty);
            }

          gimp_draw_tool_add_boundary (draw_tool,
                                       segs_out, num_segs_out,
                                       NULL,
                                       0, 0);
          g_free (segs_out);
        }
    }
  else if (options->type == GIMP_TRANSFORM_TYPE_PATH)
    {
      GimpVectors *vectors;
      GimpStroke  *stroke = NULL;
      GimpMatrix3  matrix = tr_tool->transform;

      vectors = gimp_image_get_active_vectors (image);

      if (vectors)
        {
          if (options->direction == GIMP_TRANSFORM_BACKWARD)
            gimp_matrix3_invert (&matrix);

          while ((stroke = gimp_vectors_stroke_get_next (vectors, stroke)))
            {
              GArray   *coords;
              gboolean  closed;

              coords = gimp_stroke_interpolate (stroke, 1.0, &closed);

              if (coords && coords->len)
                {
                  gint i;

                  for (i = 0; i < coords->len; i++)
                    {
                      GimpCoords *curr = &g_array_index (coords, GimpCoords, i);

                      gimp_matrix3_transform_point (&matrix,
                                                    curr->x, curr->y,
                                                    &curr->x, &curr->y);
                    }

                  gimp_draw_tool_add_strokes (draw_tool,
                                              &g_array_index (coords,
                                                              GimpCoords, 0),
                                              coords->len, FALSE);
                }

              if (coords)
                g_array_free (coords, TRUE);
            }
        }
    }
}

static void
gimp_transform_tool_dialog_update (GimpTransformTool *tr_tool)
{
  if (tr_tool->dialog &&
      GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->dialog_update)
    {
      GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->dialog_update (tr_tool);
    }
}

static GeglBuffer *
gimp_transform_tool_real_transform (GimpTransformTool *tr_tool,
                                    GimpItem          *active_item,
                                    GeglBuffer        *orig_buffer,
                                    gint               orig_offset_x,
                                    gint               orig_offset_y,
                                    gint              *new_offset_x,
                                    gint              *new_offset_y)
{
  GimpTool             *tool    = GIMP_TOOL (tr_tool);
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);
  GimpContext          *context = GIMP_CONTEXT (options);
  GeglBuffer           *ret     = NULL;
  GimpTransformResize   clip    = options->clip;
  GimpProgress         *progress;

  progress = gimp_progress_start (GIMP_PROGRESS (tool),
                                  tr_tool->progress_text, FALSE);

  if (gimp_item_get_linked (active_item))
    gimp_item_linked_transform (active_item, context,
                                &tr_tool->transform,
                                options->direction,
                                options->interpolation,
                                options->recursion_level,
                                clip,
                                progress);

  if (orig_buffer)
    {
      /*  this happens when transforming a selection cut out of a
       *  normal drawable, or the selection
       */

      /*  always clip the selction and unfloated channels
       *  so they keep their size
       */
      if (GIMP_IS_CHANNEL (active_item) &&
          ! babl_format_has_alpha (gegl_buffer_get_format (orig_buffer)))
        clip = GIMP_TRANSFORM_RESIZE_CLIP;

      ret = gimp_drawable_transform_buffer_affine (GIMP_DRAWABLE (active_item),
                                                   context,
                                                   orig_buffer,
                                                   orig_offset_x,
                                                   orig_offset_y,
                                                   &tr_tool->transform,
                                                   options->direction,
                                                   options->interpolation,
                                                   options->recursion_level,
                                                   clip,
                                                   new_offset_x,
                                                   new_offset_y,
                                                   progress);
    }
  else
    {
      /*  this happens for entire drawables, paths and layer groups  */

      /*  always clip layer masks so they keep their size
       */
      if (GIMP_IS_CHANNEL (active_item))
        clip = GIMP_TRANSFORM_RESIZE_CLIP;

      gimp_item_transform (active_item, context,
                           &tr_tool->transform,
                           options->direction,
                           options->interpolation,
                           options->recursion_level,
                           clip,
                           progress);
    }

  if (progress)
    gimp_progress_end (progress);

  return ret;
}

static void
gimp_transform_tool_transform (GimpTransformTool *tr_tool,
                               GimpDisplay       *display)
{
  GimpTool             *tool           = GIMP_TOOL (tr_tool);
  GimpTransformOptions *options        = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);
  GimpContext          *context        = GIMP_CONTEXT (options);
  GimpImage            *image          = gimp_display_get_image (display);
  GimpItem             *active_item    = NULL;
  GeglBuffer           *orig_buffer    = NULL;
  gint                  orig_offset_x;
  gint                  orig_offset_y;
  GeglBuffer           *new_buffer;
  gint                  new_offset_x;
  gint                  new_offset_y;
  const gchar          *null_message   = NULL;
  const gchar          *locked_message = NULL;
  gchar                *undo_desc      = NULL;
  gboolean              new_layer;

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      active_item = GIMP_ITEM (gimp_image_get_active_drawable (image));
      null_message   = _("There is no layer to transform.");
      locked_message = _("The active layer's pixels are locked.");
      break;

    case GIMP_TRANSFORM_TYPE_SELECTION:
      active_item = GIMP_ITEM (gimp_image_get_mask (image));
      /* cannot happen, so don't translate these messages */
      null_message   = "There is no selection to transform.";
      locked_message = "The selection's pixels are locked.";
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      active_item = GIMP_ITEM (gimp_image_get_active_vectors (image));
      null_message   = _("There is no path to transform.");
      locked_message = _("The active path's strokes are locked.");
      break;
    }

  if (! active_item)
    {
      gimp_tool_message_literal (tool, display, null_message);
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
      return;
    }

  if (gimp_item_is_content_locked (active_item))
    {
      gimp_tool_message_literal (tool, display, locked_message);
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
      return;
    }

  if (tr_tool->dialog)
    gimp_dialog_factory_hide_dialog (tr_tool->dialog);

  gimp_set_busy (display->gimp);

  /* undraw the tool before we muck around with the transform matrix */
  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tr_tool));

  /*  We're going to dirty this image, but we want to keep the tool around  */
  gimp_tool_control_push_preserve (tool->control, TRUE);

  undo_desc = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->get_undo_desc (tr_tool);
  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TRANSFORM, undo_desc);
  g_free (undo_desc);

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      if (! gimp_viewable_get_children (GIMP_VIEWABLE (tool->drawable)) &&
          ! gimp_channel_is_empty (gimp_image_get_mask (image)))
        {
          orig_buffer = gimp_drawable_transform_cut (tool->drawable,
                                                     context,
                                                     &orig_offset_x,
                                                     &orig_offset_y,
                                                     &new_layer);
        }
      break;

    case GIMP_TRANSFORM_TYPE_SELECTION:
      orig_buffer = g_object_ref (gimp_drawable_get_buffer (GIMP_DRAWABLE (active_item)));
      orig_offset_x = 0;
      orig_offset_y = 0;
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      break;
    }

  /*  Send the request for the transformation to the tool...
   */
  new_buffer = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->transform (tr_tool,
                                                                   active_item,
                                                                   orig_buffer,
                                                                   orig_offset_x,
                                                                   orig_offset_y,
                                                                   &new_offset_x,
                                                                   &new_offset_y);

  if (orig_buffer)
    g_object_unref (orig_buffer);

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      if (new_buffer)
        {
          /*  paste the new transformed image to the image...also implement
           *  undo...
           */
          gimp_drawable_transform_paste (tool->drawable, new_buffer,
                                         new_offset_x, new_offset_y,
                                         new_layer);
          g_object_unref (new_buffer);
        }
      break;

     case GIMP_TRANSFORM_TYPE_SELECTION:
      if (new_buffer)
        {
          gimp_channel_push_undo (GIMP_CHANNEL (active_item), NULL);

          gimp_drawable_set_buffer (GIMP_DRAWABLE (active_item),
                                    FALSE, NULL, new_buffer);
          g_object_unref (new_buffer);
        }
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      /*  Nothing to be done  */
      break;
    }

  gimp_image_undo_push (image, GIMP_TYPE_TRANSFORM_TOOL_UNDO,
                        GIMP_UNDO_TRANSFORM, NULL,
                        0,
                        "transform-tool", tr_tool,
                        NULL);

  gimp_image_undo_group_end (image);

  /*  We're done dirtying the image, and would like to be restarted if
   *  the image gets dirty while the tool exists
   */
  gimp_tool_control_pop_preserve (tool->control);

  gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);

  gimp_unset_busy (display->gimp);

  gimp_image_flush (image);
}

static void
gimp_transform_tool_set_function (GimpTransformTool *tr_tool,
                                  TransformAction    function)
{
  if (function != tr_tool->function)
    {
      if (tr_tool->handles[tr_tool->function] &&
          gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tr_tool)))
        {
          gimp_canvas_item_set_highlight (tr_tool->handles[tr_tool->function],
                                          FALSE);
        }

      tr_tool->function = function;

      if (tr_tool->handles[tr_tool->function] &&
          gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tr_tool)))
        {
          gimp_canvas_item_set_highlight (tr_tool->handles[tr_tool->function],
                                          TRUE);
        }
    }
}

static void
gimp_transform_tool_transform_bounding_box (GimpTransformTool *tr_tool)
{
  g_return_if_fail (GIMP_IS_TRANSFORM_TOOL (tr_tool));

  gimp_matrix3_transform_point (&tr_tool->transform,
                                tr_tool->x1, tr_tool->y1,
                                &tr_tool->tx1, &tr_tool->ty1);
  gimp_matrix3_transform_point (&tr_tool->transform,
                                tr_tool->x2, tr_tool->y1,
                                &tr_tool->tx2, &tr_tool->ty2);
  gimp_matrix3_transform_point (&tr_tool->transform,
                                tr_tool->x1, tr_tool->y2,
                                &tr_tool->tx3, &tr_tool->ty3);
  gimp_matrix3_transform_point (&tr_tool->transform,
                                tr_tool->x2, tr_tool->y2,
                                &tr_tool->tx4, &tr_tool->ty4);
  gimp_matrix3_transform_point (&tr_tool->transform,
                                tr_tool->px, tr_tool->py,
                                &tr_tool->tpx, &tr_tool->tpy);

  /* don't transform these */
  tr_tool->tpx = tr_tool->px;
  tr_tool->tpy = tr_tool->py;

  tr_tool->tcx = (tr_tool->tx1 +
                  tr_tool->tx2 +
                  tr_tool->tx3 +
                  tr_tool->tx4) / 4.0;
  tr_tool->tcy = (tr_tool->ty1 +
                  tr_tool->ty2 +
                  tr_tool->ty3 +
                  tr_tool->ty4) / 4.0;
}

static void
gimp_transform_tool_bounds (GimpTransformTool *tr_tool,
                            GimpDisplay       *display)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  GimpImage            *image   = gimp_display_get_image (display);

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      {
        GimpDrawable *drawable;
        gint          offset_x;
        gint          offset_y;

        drawable = gimp_image_get_active_drawable (image);

        gimp_item_get_offset (GIMP_ITEM (drawable), &offset_x, &offset_y);

        gimp_item_mask_bounds (GIMP_ITEM (drawable),
                               &tr_tool->x1, &tr_tool->y1,
                               &tr_tool->x2, &tr_tool->y2);
        tr_tool->x1 += offset_x;
        tr_tool->y1 += offset_y;
        tr_tool->x2 += offset_x;
        tr_tool->y2 += offset_y;
      }
      break;

    case GIMP_TRANSFORM_TYPE_SELECTION:
    case GIMP_TRANSFORM_TYPE_PATH:
      gimp_channel_bounds (gimp_image_get_mask (image),
                           &tr_tool->x1, &tr_tool->y1,
                           &tr_tool->x2, &tr_tool->y2);
      break;
    }

  tr_tool->aspect = ((gdouble) (tr_tool->x2 - tr_tool->x1) /
                     (gdouble) (tr_tool->y2 - tr_tool->y1));
}

static void
gimp_transform_tool_handles_recalc (GimpTransformTool *tr_tool,
                                    GimpDisplay       *display,
                                    gint              *handle_w,
                                    gint              *handle_h)
{
  gint dx1, dy1;
  gint dx2, dy2;
  gint dx3, dy3;
  gint dx4, dy4;
  gint x1, y1;
  gint x2, y2;

  gimp_display_shell_transform_xy (gimp_display_get_shell (display),
                                   tr_tool->tx1, tr_tool->ty1,
                                   &dx1, &dy1);
  gimp_display_shell_transform_xy (gimp_display_get_shell (display),
                                   tr_tool->tx2, tr_tool->ty2,
                                   &dx2, &dy2);
  gimp_display_shell_transform_xy (gimp_display_get_shell (display),
                                   tr_tool->tx3, tr_tool->ty3,
                                   &dx3, &dy3);
  gimp_display_shell_transform_xy (gimp_display_get_shell (display),
                                   tr_tool->tx4, tr_tool->ty4,
                                   &dx4, &dy4);

  x1 = MIN4 (dx1, dx2, dx3, dx4);
  y1 = MIN4 (dy1, dy2, dy3, dy4);
  x2 = MAX4 (dx1, dx2, dx3, dx4);
  y2 = MAX4 (dy1, dy2, dy3, dy4);

  *handle_w = CLAMP ((x2 - x1) / 3,
                     MIN_HANDLE_SIZE, GIMP_TOOL_HANDLE_SIZE_LARGE);
  *handle_h = CLAMP ((y2 - y1) / 3,
                     MIN_HANDLE_SIZE, GIMP_TOOL_HANDLE_SIZE_LARGE);
}

static void
gimp_transform_tool_dialog (GimpTransformTool *tr_tool)
{
  GimpTool     *tool      = GIMP_TOOL (tr_tool);
  GimpToolInfo *tool_info = tool->tool_info;
  const gchar  *stock_id;

  if (! GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->dialog)
    return;

  stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool_info));

  tr_tool->dialog = gimp_tool_dialog_new (tool_info,
                                          gimp_display_get_shell (tool->display),
                                          tool_info->blurb,
                                          GIMP_STOCK_RESET, RESPONSE_RESET,
                                          GTK_STOCK_UNDO, RESPONSE_UNDO,
                                          GTK_STOCK_REDO, RESPONSE_REDO,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          stock_id,         GTK_RESPONSE_OK,
                                          NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (tr_tool->dialog),
                                   GTK_RESPONSE_OK);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (tr_tool->dialog),
                                           RESPONSE_RESET,
                                           RESPONSE_UNDO,
                                           RESPONSE_REDO,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (tr_tool->dialog, "response",
                    G_CALLBACK (gimp_transform_tool_response),
                    tr_tool);

  GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->dialog (tr_tool);
}

static void
gimp_transform_tool_prepare (GimpTransformTool *tr_tool,
                             GimpDisplay       *display)
{
  if (tr_tool->dialog)
    {
      GimpTransformOptions *options  = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
      GimpImage            *image    = gimp_display_get_image (display);
      GimpDrawable         *drawable = gimp_image_get_active_drawable (image);

      gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (tr_tool->dialog),
                                         GIMP_VIEWABLE (drawable),
                                         GIMP_CONTEXT (options));
      gimp_tool_dialog_set_shell (GIMP_TOOL_DIALOG (tr_tool->dialog),
                                  gimp_display_get_shell (display));
    }

  if (GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->prepare)
    GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->prepare (tr_tool);
}

void
gimp_transform_tool_recalc_matrix (GimpTransformTool *tr_tool)
{
  g_return_if_fail (GIMP_IS_TRANSFORM_TOOL (tr_tool));

  if (GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix)
    GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix (tr_tool);

  gimp_transform_tool_transform_bounding_box (tr_tool);

  gimp_transform_tool_dialog_update (tr_tool);

  if (tr_tool->dialog)
    gtk_widget_show (tr_tool->dialog);
}

static void
gimp_transform_tool_response (GtkWidget         *widget,
                              gint               response_id,
                              GimpTransformTool *tr_tool)
{
  GimpTool *tool = GIMP_TOOL (tr_tool);
  GList    *it   = tr_tool->redo_list;
  gint      i;

  switch (response_id)
    {
    case RESPONSE_UNDO:
      {
        it = g_list_next (tr_tool->undo_list);

    case RESPONSE_REDO:
        if (it)
          {
    case RESPONSE_RESET:
            if (response_id == RESPONSE_UNDO)
              {
                /* Move prev_trans_info from undo_list to redo_list */
                tr_tool->redo_list = g_list_prepend (tr_tool->redo_list,
                                                     tr_tool->prev_trans_info);
                tr_tool->undo_list = g_list_remove (tr_tool->undo_list,
                                                    tr_tool->prev_trans_info);
                tr_tool->prev_trans_info = it->data;
              }
            else if (response_id == RESPONSE_REDO)
              {
                /* And the opposite */
                tr_tool->prev_trans_info = it->data;
                tr_tool->undo_list = g_list_prepend (tr_tool->undo_list,
                                                     tr_tool->prev_trans_info);
                tr_tool->redo_list = g_list_remove (tr_tool->redo_list,
                                                    tr_tool->prev_trans_info);
              }
            else if (response_id == RESPONSE_RESET)
              {
                /* Move all undo events to redo, and pop off the first
                 * one as that's the current one, which always sits on
                 * the undo_list
                 */
                tr_tool->redo_list =
                  g_list_remove (g_list_concat (g_list_reverse (tr_tool->undo_list),
                                                tr_tool->redo_list),
                                 tr_tool->old_trans_info);
                tr_tool->prev_trans_info = tr_tool->old_trans_info;
                tr_tool->undo_list = g_list_prepend (NULL,
                                                     tr_tool->prev_trans_info);
              }
            update_sensitivity (tr_tool);

            gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

            /*  Restore the previous transformation info  */
            for (i = 0; i < TRANS_INFO_SIZE; i++)
              {
                tr_tool->trans_info[i] = (*tr_tool->prev_trans_info)[i];
              }

            /*  reget the selection bounds  */
            gimp_transform_tool_bounds (tr_tool, tool->display);

            /*  recalculate the tool's transformation matrix  */
            gimp_transform_tool_recalc_matrix (tr_tool);

            gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
          }
      }
      break;

    case GTK_RESPONSE_OK:
      g_return_if_fail (tool->display != NULL);
      gimp_transform_tool_transform (tr_tool, tool->display);
      break;

    default:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
free_trans (gpointer data)
{
  g_slice_free (TransInfo, data);
}

static void
update_sensitivity (GimpTransformTool *tr_tool)
{
  if (!tr_tool->dialog)
    return;

  gtk_dialog_set_response_sensitive (GTK_DIALOG (tr_tool->dialog), RESPONSE_UNDO,
      g_list_next (tr_tool->undo_list) != NULL);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (tr_tool->dialog), RESPONSE_REDO,
      tr_tool->redo_list != NULL);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (tr_tool->dialog), RESPONSE_RESET,
      g_list_next (tr_tool->undo_list) != NULL);
}
