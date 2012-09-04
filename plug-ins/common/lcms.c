/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Color management plug-in based on littleCMS
 * Copyright (C) 2006, 2007  Sven Neumann <sven@gimp.org>
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

#include <glib.h>  /* lcms.h uses the "inline" keyword */

#include <lcms2.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_BINARY          "lcms"
#define PLUG_IN_ROLE            "gimp-lcms"

#define PLUG_IN_PROC_SET        "plug-in-icc-profile-set"
#define PLUG_IN_PROC_SET_RGB    "plug-in-icc-profile-set-rgb"

#define PLUG_IN_PROC_APPLY      "plug-in-icc-profile-apply"
#define PLUG_IN_PROC_APPLY_RGB  "plug-in-icc-profile-apply-rgb"

#define PLUG_IN_PROC_INFO       "plug-in-icc-profile-info"
#define PLUG_IN_PROC_FILE_INFO  "plug-in-icc-profile-file-info"


enum
{
  STATUS,
  PROFILE_NAME,
  PROFILE_DESC,
  PROFILE_INFO,
  NUM_RETURN_VALS
};

enum
{
  PROC_SET,
  PROC_SET_RGB,
  PROC_APPLY,
  PROC_APPLY_RGB,
  PROC_INFO,
  PROC_FILE_INFO,
  NONE
};

typedef struct
{
  const gchar *name;
  const gint   min_params;
} Procedure;

typedef struct
{
  GimpColorRenderingIntent intent;
  gboolean                 bpc;
} LcmsValues;


static void  query (void);
static void  run   (const gchar      *name,
                    gint              nparams,
                    const GimpParam  *param,
                    gint             *nreturn_vals,
                    GimpParam       **return_vals);

static GimpPDBStatusType  lcms_icc_set       (GimpColorConfig  *config,
                                              gint32            image,
                                              const gchar      *filename);
static GimpPDBStatusType  lcms_icc_apply     (GimpColorConfig  *config,
                                              GimpRunMode       run_mode,
                                              gint32            image,
                                              const gchar      *filename,
                                              GimpColorRenderingIntent intent,
                                              gboolean          bpc,
                                              gboolean         *dont_ask);
static GimpPDBStatusType  lcms_icc_info      (GimpColorConfig  *config,
                                              gint32            image,
                                              gchar           **name,
                                              gchar           **desc,
                                              gchar           **info);
static GimpPDBStatusType  lcms_icc_file_info (const gchar      *filename,
                                              gchar           **name,
                                              gchar           **desc,
                                              gchar           **info);

static cmsHPROFILE  lcms_image_get_profile       (GimpColorConfig *config,
                                                  gint32           image,
                                                  guchar          *checksum);
static gboolean     lcms_image_set_profile       (gint32           image,
                                                  cmsHPROFILE      profile,
                                                  const gchar     *filename,
                                                  gboolean         undo_group);
static gboolean     lcms_image_apply_profile     (gint32           image,
                                                  cmsHPROFILE      src_profile,
                                                  cmsHPROFILE      dest_profile,
                                                  const gchar     *filename,
                                                  GimpColorRenderingIntent intent,
                                                  gboolean          bpc);
static void         lcms_image_transform_rgb     (gint32           image,
                                                  cmsHPROFILE      src_profile,
                                                  cmsHPROFILE      dest_profile,
                                                  GimpColorRenderingIntent intent,
                                                  gboolean          bpc);
static void         lcms_image_transform_indexed (gint32           image,
                                                  cmsHPROFILE      src_profile,
                                                  cmsHPROFILE      dest_profile,
                                                  GimpColorRenderingIntent intent,
                                                  gboolean          bpc);

static void         lcms_drawable_transform      (GimpDrawable    *drawable,
                                                  cmsHTRANSFORM    transform,
                                                  gdouble          progress_start,
                                                  gdouble          progress_end);
static void         lcms_sRGB_checksum           (guchar          *digest);

static cmsHPROFILE  lcms_load_profile            (const gchar     *filename,
                                                  guchar          *checksum);

static gboolean     lcms_icc_apply_dialog        (gint32           image,
                                                  cmsHPROFILE      src_profile,
                                                  cmsHPROFILE      dest_profile,
                                                  gboolean        *dont_ask);

static GimpPDBStatusType  lcms_dialog            (GimpColorConfig *config,
                                                  gint32           image,
                                                  gboolean         apply,
                                                  LcmsValues      *values);


static const GimpParamDef set_args[] =
{
  { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"     },
  { GIMP_PDB_IMAGE,  "image",        "Input image"                      },
  { GIMP_PDB_STRING, "profile",      "Filename of an ICC color profile" }
};
static const GimpParamDef set_rgb_args[] =
{
  { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"     },
  { GIMP_PDB_IMAGE,  "image",        "Input image"                      },
};
static const GimpParamDef apply_args[] =
{
  { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"     },
  { GIMP_PDB_IMAGE,  "image",        "Input image"                      },
  { GIMP_PDB_STRING, "profile",      "Filename of an ICC color profile" },
  { GIMP_PDB_INT32,  "intent",       "Rendering intent (enum GimpColorRenderingIntent)" },
  { GIMP_PDB_INT32,  "bpc",          "Black point compensation"         }
};
static const GimpParamDef apply_rgb_args[] =
{
  { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"     },
  { GIMP_PDB_IMAGE,  "image",        "Input image"                      },
  { GIMP_PDB_INT32,  "intent",       "Rendering intent (enum GimpColorRenderingIntent)" },
  { GIMP_PDB_INT32,  "bpc",          "Black point compensation"         }
};
static const GimpParamDef info_args[] =
{
  { GIMP_PDB_IMAGE,  "image",        "Input image"                      },
};
static const GimpParamDef file_info_args[] =
{
  { GIMP_PDB_STRING, "profile",      "Filename of an ICC color profile" }
};

static const Procedure procedures[] =
{
  { PLUG_IN_PROC_SET,       2 },
  { PLUG_IN_PROC_SET_RGB,   2 },
  { PLUG_IN_PROC_APPLY,     2 },
  { PLUG_IN_PROC_APPLY_RGB, 2 },
  { PLUG_IN_PROC_INFO,      1 },
  { PLUG_IN_PROC_FILE_INFO, 1 }
};

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
  static const GimpParamDef info_return_vals[] =
  {
    { GIMP_PDB_STRING, "profile-name", "Name"        },
    { GIMP_PDB_STRING, "profile-desc", "Description" },
    { GIMP_PDB_STRING, "profile-info", "Info"        }
  };

  gimp_install_procedure (PLUG_IN_PROC_SET,
                          N_("Set a color profile on the image"),
                          "This procedure sets an ICC color profile on an "
                          "image using the 'icc-profile' parasite. It does "
                          "not do any color conversion.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006, 2007",
                          N_("_Assign Color Profile..."),
                          "RGB*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (set_args), 0,
                          set_args, NULL);

  gimp_install_procedure (PLUG_IN_PROC_SET_RGB,
                          "Set the default RGB color profile on the image",
                          "This procedure sets the user-configured RGB "
                          "profile on an image using the 'icc-profile' "
                          "parasite. If no RGB profile is configured, sRGB "
                          "is assumed and the parasite is unset. This "
                          "procedure does not do any color conversion.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006, 2007",
                          N_("Assign default RGB Profile"),
                          "RGB*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (set_rgb_args), 0,
                          set_rgb_args, NULL);

  gimp_install_procedure (PLUG_IN_PROC_APPLY,
                          _("Apply a color profile on the image"),
                          "This procedure transform from the image's color "
                          "profile (or the default RGB profile if none is "
                          "set) to the given ICC color profile. Only RGB "
                          "color profiles are accepted. The profile "
                          "is then set on the image using the 'icc-profile' "
                          "parasite.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006, 2007",
                          N_("_Convert to Color Profile..."),
                          "RGB*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (apply_args), 0,
                          apply_args, NULL);

  gimp_install_procedure (PLUG_IN_PROC_APPLY_RGB,
                          "Apply default RGB color profile on the image",
                          "This procedure transform from the image's color "
                          "profile (or the default RGB profile if none is "
                          "set) to the configured default RGB color profile.  "
                          "The profile is then set on the image using the "
                          "'icc-profile' parasite.  If no RGB color profile "
                          "is configured, sRGB is assumed and the parasite "
                          "is unset.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006, 2007",
                          N_("Convert to default RGB Profile"),
                          "RGB*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (apply_rgb_args), 0,
                          apply_rgb_args, NULL);

  gimp_install_procedure (PLUG_IN_PROC_INFO,
                          "Retrieve information about an image's color profile",
                          "This procedure returns information about the RGB "
                          "color profile attached to an image. If no RGB "
                          "color profile is attached, sRGB is assumed.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006, 2007",
                          N_("Image Color Profile Information"),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (info_args),
                          G_N_ELEMENTS (info_return_vals),
                          info_args, info_return_vals);

  gimp_install_procedure (PLUG_IN_PROC_FILE_INFO,
                          "Retrieve information about a color profile",
                          "This procedure returns information about an ICC "
                          "color profile on disk.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006, 2007",
                          N_("Color Profile Information"),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (file_info_args),
                          G_N_ELEMENTS (info_return_vals),
                          file_info_args, info_return_vals);

  gimp_plugin_menu_register (PLUG_IN_PROC_SET,
                             "<Image>/Image/Mode/Color Profile");
  gimp_plugin_menu_register (PLUG_IN_PROC_APPLY,
                             "<Image>/Image/Mode/Color Profile");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpPDBStatusType         status   = GIMP_PDB_CALLING_ERROR;
  gint                      proc     = NONE;
  GimpRunMode               run_mode = GIMP_RUN_NONINTERACTIVE;
  gint32                    image    = -1;
  const gchar              *filename = NULL;
  GimpColorConfig          *config   = NULL;
  gboolean                  dont_ask = FALSE;
  GimpColorRenderingIntent  intent;
  gboolean                  bpc;
  static GimpParam          values[6];

  INIT_I18N ();

  values[0].type = GIMP_PDB_STATUS;

  *nreturn_vals = 1;
  *return_vals  = values;

  for (proc = 0; proc < G_N_ELEMENTS (procedures); proc++)
    {
      if (strcmp (name, procedures[proc].name) == 0)
        break;
    }

  if (proc == NONE)
    goto done;

  if (nparams < procedures[proc].min_params)
    goto done;

  if (proc != PROC_FILE_INFO)
    config = gimp_get_color_configuration ();

  if (config)
    intent = config->display_intent;
  else
    intent = GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL;

  bpc = (intent == GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC);

  switch (proc)
    {
    case PROC_SET:
      run_mode = param[0].data.d_int32;
      image    = param[1].data.d_image;
      if (nparams > 2)
        filename = param[2].data.d_string;
      break;

    case PROC_APPLY:
      run_mode = param[0].data.d_int32;
      image    = param[1].data.d_image;
      if (nparams > 2)
        filename = param[2].data.d_string;
      if (nparams > 3)
        intent = param[3].data.d_int32;
      if (nparams > 4)
        bpc    = param[4].data.d_int32 ? TRUE : FALSE;
      break;

    case PROC_SET_RGB:
      run_mode = param[0].data.d_int32;
      image    = param[1].data.d_image;
      break;

    case PROC_APPLY_RGB:
      run_mode = param[0].data.d_int32;
      image    = param[1].data.d_image;
      if (nparams > 2)
        intent = param[2].data.d_int32;
      if (nparams > 3)
        bpc    = param[3].data.d_int32 ? TRUE : FALSE;
      break;

    case PROC_INFO:
      image    = param[0].data.d_image;
      break;

    case PROC_FILE_INFO:
      filename = param[0].data.d_string;
      break;
    }

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      LcmsValues values = { intent, bpc };

      switch (proc)
        {
        case PROC_SET:
          status = lcms_dialog (config, image, FALSE, &values);
          goto done;

        case PROC_APPLY:
          gimp_get_data (name, &values);

          status = lcms_dialog (config, image, TRUE, &values);

          if (status == GIMP_PDB_SUCCESS)
            gimp_set_data (name, &values, sizeof (LcmsValues));
          goto done;

        default:
          break;
        }
    }

  switch (proc)
    {
    case PROC_SET:
    case PROC_SET_RGB:
      status = lcms_icc_set (config, image, filename);
      break;

    case PROC_APPLY:
    case PROC_APPLY_RGB:
      status = lcms_icc_apply (config, run_mode,
                               image, filename, intent, bpc,
                               &dont_ask);

      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          *nreturn_vals = 2;

          values[1].type         = GIMP_PDB_INT32;
          values[1].data.d_int32 = dont_ask;
        }
      break;

    case PROC_INFO:
    case PROC_FILE_INFO:
      {
        gchar *name = NULL;
        gchar *desc = NULL;
        gchar *info = NULL;

        if (proc == PROC_INFO)
          status = lcms_icc_info (config, image, &name, &desc, &info);
        else
          status = lcms_icc_file_info (filename, &name, &desc, &info);

        if (status == GIMP_PDB_SUCCESS)
          {
            *nreturn_vals = NUM_RETURN_VALS;

            values[PROFILE_NAME].type          = GIMP_PDB_STRING;
            values[PROFILE_NAME].data.d_string = name;

            values[PROFILE_DESC].type          = GIMP_PDB_STRING;
            values[PROFILE_DESC].data.d_string = desc;

            values[PROFILE_INFO].type          = GIMP_PDB_STRING;
            values[PROFILE_INFO].data.d_string = info;
          }
      }
      break;
    }

 done:
  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  if (config)
    g_object_unref (config);

  values[0].data.d_status = status;
}

static gchar *
lcms_icc_profile_get_name (cmsHPROFILE profile)
{
  cmsUInt32Number  descSize;
  gchar           *descData;
  gchar           *name = NULL;

  descSize = cmsGetProfileInfoASCII (profile, cmsInfoModel,
                                     "en", "US", NULL, 0);
  if (descSize > 0)
    {
      descData = g_new (gchar, descSize + 1);
      descSize = cmsGetProfileInfoASCII (profile, cmsInfoModel,
                                         "en", "US", descData, descSize);
      if (descSize > 0)
        name = gimp_any_to_utf8 (descData, -1, NULL);

      g_free (descData);
    }

  return name;
}

static gchar *
lcms_icc_profile_get_desc (cmsHPROFILE profile)
{
  cmsUInt32Number  descSize;
  gchar           *descData;
  gchar           *desc = NULL;

  descSize = cmsGetProfileInfoASCII (profile, cmsInfoDescription,
                                     "en", "US", NULL, 0);
  if (descSize > 0)
    {
      descData = g_new (gchar, descSize + 1);
      descSize = cmsGetProfileInfoASCII (profile, cmsInfoDescription,
                                         "en", "US", descData, descSize);
      if (descSize > 0)
        desc = gimp_any_to_utf8 (descData, -1, NULL);

      g_free (descData);
    }

  return desc;
}

static gchar *
lcms_icc_profile_get_info (cmsHPROFILE profile)
{
  cmsUInt32Number  descSize;
  gchar           *descData;
  gchar           *info = NULL;

  descSize = cmsGetProfileInfoASCII (profile, cmsInfoModel,
                                     "en", "US", NULL, 0);
  if (descSize > 0)
    {
      descData = g_new (gchar, descSize + 1);
      descSize = cmsGetProfileInfoASCII (profile, cmsInfoModel,
                                         "en", "US", descData, descSize);
      if (descSize > 0)
        info = gimp_any_to_utf8 (descData, -1, NULL);

      g_free (descData);
    }

  return info;
}

static gboolean
lcms_icc_profile_is_rgb (cmsHPROFILE profile)
{
  return (cmsGetColorSpace (profile) == cmsSigRgbData);
}

static GimpPDBStatusType
lcms_icc_set (GimpColorConfig *config,
              gint32           image,
              const gchar     *filename)
{
  gboolean success;

  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail (image != -1, GIMP_PDB_CALLING_ERROR);

  if (filename)
    {
      success = lcms_image_set_profile (image, NULL, filename, TRUE);
    }
  else
    {
      success = lcms_image_set_profile (image, NULL, config->rgb_profile, TRUE);
    }

  return success ? GIMP_PDB_SUCCESS : GIMP_PDB_EXECUTION_ERROR;
}

static GimpPDBStatusType
lcms_icc_apply (GimpColorConfig          *config,
                GimpRunMode               run_mode,
                gint32                    image,
                const gchar              *filename,
                GimpColorRenderingIntent  intent,
                gboolean                  bpc,
                gboolean                 *dont_ask)
{
  GimpPDBStatusType status       = GIMP_PDB_SUCCESS;
  cmsHPROFILE       src_profile  = NULL;
  cmsHPROFILE       dest_profile = NULL;
  guchar            src_md5[16];
  guchar            dest_md5[16];

  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail (image != -1, GIMP_PDB_CALLING_ERROR);

  if (! filename)
    filename = config->rgb_profile;

  if (filename)
    {
      dest_profile = lcms_load_profile (filename, dest_md5);

      if (! dest_profile)
        return GIMP_PDB_EXECUTION_ERROR;

      if (! lcms_icc_profile_is_rgb (dest_profile))
        {
          g_message (_("Color profile '%s' is not for RGB color space."),
                     gimp_filename_to_utf8 (filename));

          cmsCloseProfile (dest_profile);
          return GIMP_PDB_EXECUTION_ERROR;
        }
    }

  src_profile = lcms_image_get_profile (config, image, src_md5);

  if (src_profile && ! lcms_icc_profile_is_rgb (src_profile))
    {
      g_printerr ("lcms: attached color profile is not for RGB color space "
                  "(skipping)\n");

      cmsCloseProfile (src_profile);
      src_profile = NULL;
    }

  if (! src_profile && ! dest_profile)
    return GIMP_PDB_SUCCESS;

  if (! src_profile)
    {
      src_profile = cmsCreate_sRGBProfile ();
      lcms_sRGB_checksum (src_md5);
    }

  if (! dest_profile)
    {
      dest_profile = cmsCreate_sRGBProfile ();
      lcms_sRGB_checksum (dest_md5);
    }

  if (memcmp (src_md5, dest_md5, 16) == 0)
    {
      gchar *src_desc  = lcms_icc_profile_get_desc (src_profile);
      gchar *dest_desc = lcms_icc_profile_get_desc (dest_profile);

      cmsCloseProfile (src_profile);
      cmsCloseProfile (dest_profile);

      g_printerr ("lcms: skipping conversion because profiles seem to be equal:\n");
      g_printerr (" %s\n", src_desc);
      g_printerr (" %s\n", dest_desc);

      g_free (src_desc);
      g_free (dest_desc);

      return GIMP_PDB_SUCCESS;
    }

  if (run_mode == GIMP_RUN_INTERACTIVE &&
      ! lcms_icc_apply_dialog (image, src_profile, dest_profile, dont_ask))
    {
      status = GIMP_PDB_CANCEL;
    }

  if (status == GIMP_PDB_SUCCESS &&
      ! lcms_image_apply_profile (image,
                                  src_profile, dest_profile, filename,
                                  intent, bpc))
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  cmsCloseProfile (src_profile);
  cmsCloseProfile (dest_profile);

  return status;
}

static GimpPDBStatusType
lcms_icc_info (GimpColorConfig *config,
               gint32           image,
               gchar          **name,
               gchar          **desc,
               gchar          **info)
{
  cmsHPROFILE profile;

  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail (image != -1, GIMP_PDB_CALLING_ERROR);

  profile = lcms_image_get_profile (config, image, NULL);

  if (profile && ! lcms_icc_profile_is_rgb (profile))
    {
      g_printerr ("lcms: attached color profile is not for RGB color space "
                  "(skipping)\n");

      cmsCloseProfile (profile);
      profile = NULL;
    }

  if (profile)
    {
      if (name) *name = lcms_icc_profile_get_name (profile);
      if (desc) *desc = lcms_icc_profile_get_desc (profile);
      if (info) *info = lcms_icc_profile_get_info (profile);

      cmsCloseProfile (profile);
    }
  else
    {
      if (name) *name = g_strdup ("sRGB");
      if (desc) *desc = g_strdup ("sRGB built-in");
      if (info) *info = g_strdup (_("Default RGB working space"));
    }

  return GIMP_PDB_SUCCESS;
}

static GimpPDBStatusType
lcms_icc_file_info (const gchar  *filename,
                    gchar       **name,
                    gchar       **desc,
                    gchar       **info)
{
  cmsHPROFILE profile;

  if (! g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    return GIMP_PDB_EXECUTION_ERROR;

  profile = cmsOpenProfileFromFile (filename, "r");

  if (! profile)
    return GIMP_PDB_EXECUTION_ERROR;

  *name = lcms_icc_profile_get_name (profile);
  *desc = lcms_icc_profile_get_desc (profile);
  *info = lcms_icc_profile_get_info (profile);

  cmsCloseProfile (profile);

  return GIMP_PDB_SUCCESS;
}

static void
lcms_sRGB_checksum (guchar *digest)
{
  digest[0]  = 0xcb;
  digest[1]  = 0x63;
  digest[2]  = 0x14;
  digest[3]  = 0x56;
  digest[4]  = 0xd4;
  digest[5]  = 0x0a;
  digest[6]  = 0x01;
  digest[7]  = 0x62;
  digest[8]  = 0xa0;
  digest[9]  = 0xdb;
  digest[10] = 0xe6;
  digest[11] = 0x32;
  digest[12] = 0x8b;
  digest[13] = 0xea;
  digest[14] = 0x1a;
  digest[15] = 0x89;
}

static void
lcms_calculate_checksum (const gchar *data,
                         gsize        len,
                         guchar      *digest)
{
  if (digest)
    {
      GChecksum *md5 = g_checksum_new (G_CHECKSUM_MD5);

      g_checksum_update (md5,
                         (const guchar *) data + sizeof (cmsICCHeader),
                         len - sizeof (cmsICCHeader));

      len = 16;
      g_checksum_get_digest (md5, digest, &len);
      g_checksum_free (md5);
    }
}

static cmsHPROFILE
lcms_image_get_profile (GimpColorConfig *config,
                        gint32           image,
                        guchar          *checksum)
{
  GimpParasite *parasite;
  cmsHPROFILE   profile = NULL;

  g_return_val_if_fail (image != -1, NULL);

  parasite = gimp_image_get_parasite (image, "icc-profile");

  if (parasite)
    {
      profile = cmsOpenProfileFromMem ((gpointer) gimp_parasite_data (parasite),
                                       gimp_parasite_data_size (parasite));

      if (profile)
        {
          lcms_calculate_checksum (gimp_parasite_data (parasite),
                                   gimp_parasite_data_size (parasite),
                                   checksum);
        }
      else
        {
          g_message (_("Data attached as 'icc-profile' does not appear to "
                       "be an ICC color profile"));
        }

      gimp_parasite_free (parasite);
    }
  else if (config->rgb_profile)
    {
      profile = lcms_load_profile (config->rgb_profile, checksum);
    }

  return profile;
}

static gboolean
lcms_image_set_profile (gint32       image,
                        cmsHPROFILE  profile,
                        const gchar *filename,
                        gboolean     undo_group)
{
  g_return_val_if_fail (image != -1, FALSE);

  if (filename)
    {
      GimpParasite *parasite;
      GMappedFile  *file;
      GError       *error = NULL;

      file = g_mapped_file_new (filename, FALSE, &error);

      if (! file)
        {
          g_message ("%s", error->message);
          g_error_free (error);

          return FALSE;
        }

      /* check that this file is actually an ICC profile */
      if (! profile)
        {
          profile = cmsOpenProfileFromMem (g_mapped_file_get_contents (file),
                                           g_mapped_file_get_length (file));

          if (profile)
            {
              cmsCloseProfile (profile);
            }
          else
            {
              g_message (_("'%s' does not appear to be an ICC color profile"),
                         gimp_filename_to_utf8 (filename));
              return FALSE;
            }
        }

      if (undo_group)
        gimp_image_undo_group_start (image);

      parasite = gimp_parasite_new ("icc-profile",
                                    GIMP_PARASITE_PERSISTENT |
                                    GIMP_PARASITE_UNDOABLE,
                                    g_mapped_file_get_length (file),
                                    g_mapped_file_get_contents (file));

      g_mapped_file_unref (file);

      gimp_image_attach_parasite (image, parasite);
      gimp_parasite_free (parasite);
    }
  else
    {
      if (undo_group)
        gimp_image_undo_group_start (image);

      gimp_image_detach_parasite (image, "icc-profile");
    }

  gimp_image_detach_parasite (image, "icc-profile-name");

  if (undo_group)
    gimp_image_undo_group_end (image);

  return TRUE;
}

static gboolean
lcms_image_apply_profile (gint32                    image,
                          cmsHPROFILE               src_profile,
                          cmsHPROFILE               dest_profile,
                          const gchar              *filename,
                          GimpColorRenderingIntent  intent,
                          gboolean                  bpc)
{
  gint32 saved_selection = -1;

  gimp_image_undo_group_start (image);

  if (! lcms_image_set_profile (image, dest_profile, filename, FALSE))
    {
      gimp_image_undo_group_end (image);

      return FALSE;
    }

  {
    gchar  *src  = lcms_icc_profile_get_desc (src_profile);
    gchar  *dest = lcms_icc_profile_get_desc (dest_profile);

      /* ICC color profile conversion */
      gimp_progress_init_printf (_("Converting from '%s' to '%s'"), src, dest);

      g_printerr ("lcms: converting from '%s' to '%s'\n", src, dest);

      g_free (dest);
      g_free (src);
  }

  if (! gimp_selection_is_empty (image))
    {
      saved_selection = gimp_selection_save (image);
      gimp_selection_none (image);
    }

  switch (gimp_image_base_type (image))
    {
    case GIMP_RGB:
      lcms_image_transform_rgb (image,
                                src_profile, dest_profile, intent, bpc);
      break;

    case GIMP_GRAY:
      g_warning ("colorspace conversion not implemented for "
                 "grayscale images");
      break;

    case GIMP_INDEXED:
      lcms_image_transform_indexed (image,
                                    src_profile, dest_profile, intent, bpc);
      break;
    }

  if (saved_selection != -1)
    {
      gimp_image_select_item (image, GIMP_CHANNEL_OP_REPLACE, saved_selection);
      gimp_image_remove_channel (image, saved_selection);
    }

  gimp_progress_update (1.0);

  gimp_image_undo_group_end (image);

  return TRUE;
}

static void
lcms_image_transform_rgb (gint32                    image,
                          cmsHPROFILE               src_profile,
                          cmsHPROFILE               dest_profile,
                          GimpColorRenderingIntent  intent,
                          gboolean                  bpc)
{
  cmsHTRANSFORM    transform   = NULL;
  cmsUInt32Number  last_format = 0;
  gint            *layers;
  gint             num_layers;
  gint             i;

  layers = gimp_image_get_layers (image, &num_layers);

  for (i = 0; i < num_layers; i++)
    {
      GimpDrawable    *drawable = gimp_drawable_get (layers[i]);
      cmsUInt32Number  format;

      switch (drawable->bpp)
        {
        case 3:
          format = TYPE_RGB_8;
          break;
        case 4:
          format = TYPE_RGBA_8;
          break;

        default:
          g_warning ("%s: unexpected bpp", G_STRLOC);
          continue;
        }

      if (! transform || format != last_format)
        {
          if (transform)
            cmsDeleteTransform (transform);

          transform = cmsCreateTransform (src_profile,  format,
                                          dest_profile, format,
                                          intent,
                                          bpc ?
                                          cmsFLAGS_BLACKPOINTCOMPENSATION : 0);

          last_format = format;
        }

      if (transform)
        {
          lcms_drawable_transform (drawable, transform,
                                   (gdouble) i / num_layers,
                                   (gdouble) (i + 1) / num_layers);
        }
      else
        {
          g_warning ("cmsCreateTransform() failed!");
        }

      gimp_drawable_detach (drawable);
    }

  if (transform)
    cmsDeleteTransform(transform);

  g_free (layers);
}

static void
lcms_image_transform_indexed (gint32                    image,
                              cmsHPROFILE               src_profile,
                              cmsHPROFILE               dest_profile,
                              GimpColorRenderingIntent  intent,
                              gboolean                  bpc)
{
  cmsHTRANSFORM   transform;
  guchar         *cmap;
  gint            num_colors;

  cmap = gimp_image_get_colormap (image, &num_colors);

  transform = cmsCreateTransform (src_profile,  TYPE_RGB_8,
                                  dest_profile, TYPE_RGB_8,
                                  intent,
                                  bpc ? cmsFLAGS_BLACKPOINTCOMPENSATION : 0);

  if (transform)
    {
      cmsDoTransform (transform, cmap, cmap, num_colors);
      cmsDeleteTransform(transform);
    }
  else
    {
      g_warning ("cmsCreateTransform() failed!");
    }

  gimp_image_set_colormap (image, cmap, num_colors);
}

static void
lcms_drawable_transform (GimpDrawable  *drawable,
                         cmsHTRANSFORM  transform,
                         gdouble        progress_start,
                         gdouble        progress_end)
{
  GimpPixelRgn   src_rgn;
  GimpPixelRgn   dest_rgn;
  gpointer       pr;
  const gboolean alpha = gimp_drawable_has_alpha (drawable->drawable_id);
  gdouble        range = progress_end - progress_start;
  guint          count = 0;
  guint          done  = 0;

  gimp_pixel_rgn_init (&src_rgn, drawable,
                       0, 0, drawable->width, drawable->height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable,
                       0, 0, drawable->width, drawable->height, TRUE, TRUE);

  for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      guchar *src  = src_rgn.data;
      guchar *dest = dest_rgn.data;
      gint    y;

      for (y = 0; y < dest_rgn.h; y++)
        {
          cmsDoTransform (transform, src, dest, dest_rgn.w);

          /* copy the alpha values, cmsDoTransform() leaves them untouched */
          if (alpha)
            {
              const guchar *s = src;
              guchar       *d = dest;
              gint          w = dest_rgn.w;

              while (w--)
                {
                  d[3] = s[3];

                  s += 4;
                  d += 4;
                }
            }

          src  += src_rgn.rowstride;
          dest += dest_rgn.rowstride;
        }

      done += dest_rgn.h * dest_rgn.w;

      if (count++ % 32 == 0)
        gimp_progress_update (progress_start +
                              (gdouble) done /
                              (drawable->width * drawable->height) * range);
    }

  gimp_progress_update (progress_end);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id,
                        0, 0, drawable->width, drawable->height);
}

static cmsHPROFILE
lcms_load_profile (const gchar *filename,
                   guchar      *checksum)
{
  cmsHPROFILE  profile;
  GMappedFile *file;
  gchar       *data;
  gsize        len;
  GError      *error = NULL;

  g_return_val_if_fail (filename != NULL, NULL);

  file = g_mapped_file_new (filename, FALSE, &error);

  if (! file)
    {
      g_message ("%s", error->message);
      g_error_free (error);

      return NULL;
    }

  data = g_mapped_file_get_contents (file);
  len = g_mapped_file_get_length (file);

  profile = cmsOpenProfileFromMem (data, len);

  if (profile)
    {
      lcms_calculate_checksum (data, len, checksum);
    }
  else
    {
      g_message (_("Could not load ICC profile from '%s'"),
                 gimp_filename_to_utf8 (filename));
    }

  g_mapped_file_unref (file);

  return profile;
}

static GtkWidget *
lcms_icc_profile_src_label_new (gint32       image,
                                cmsHPROFILE  profile)
{
  GtkWidget *vbox;
  GtkWidget *label;
  gchar     *name;
  gchar     *desc;
  gchar     *text;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);

  name = gimp_image_get_name (image);
  text = g_strdup_printf (_("The image '%s' has an embedded color profile:"),
                          name);
  g_free (name);

  label = g_object_new (GTK_TYPE_LABEL,
                        "label",   text,
                        "wrap",    TRUE,
                        "justify", GTK_JUSTIFY_LEFT,
                        "xalign",  0.0,
                        "yalign",  0.0,
                        NULL);
  g_free (text);

  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  desc = lcms_icc_profile_get_desc (profile);
  label = g_object_new (GTK_TYPE_LABEL,
                        "label",   desc,
                        "wrap",    TRUE,
                        "justify", GTK_JUSTIFY_LEFT,
                        "xalign",  0.0,
                        "yalign",  0.0,
                        "xpad",    24,
                        NULL);
  g_free (desc);

  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  return vbox;
}

static GtkWidget *
lcms_icc_profile_dest_label_new (cmsHPROFILE  profile)
{
  GtkWidget *label;
  gchar     *desc;
  gchar     *text;

  desc = lcms_icc_profile_get_desc (profile);
  text = g_strdup_printf (_("Convert the image to the RGB working space (%s)?"),
                          desc);
  g_free (desc);

  label = g_object_new (GTK_TYPE_LABEL,
                        "label",   text,
                        "wrap",    TRUE,
                        "justify", GTK_JUSTIFY_LEFT,
                        "xalign",  0.0,
                        "yalign",  0.0,
                        NULL);
  g_free (text);

  return label;
}

static gboolean
lcms_icc_apply_dialog (gint32       image,
                       cmsHPROFILE  src_profile,
                       cmsHPROFILE  dest_profile,
                       gboolean    *dont_ask)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *toggle = NULL;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Convert to RGB working space?"),
                            PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC_APPLY,

                            _("_Keep"),    GTK_RESPONSE_CANCEL,

                            NULL);

  button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                  _("_Convert"), GTK_RESPONSE_OK);
  gtk_button_set_image (GTK_BUTTON (button),
                        gtk_image_new_from_stock (GTK_STOCK_CONVERT,
                                                  GTK_ICON_SIZE_BUTTON));

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  label = lcms_icc_profile_src_label_new (image, src_profile);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  label = lcms_icc_profile_dest_label_new (dest_profile);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  if (dont_ask)
    {
      toggle = gtk_check_button_new_with_mnemonic (_("_Don't ask me again"));
      gtk_box_pack_end (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), FALSE);
      gtk_widget_show (toggle);
    }

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  *dont_ask = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle));

  gtk_widget_destroy (dialog);

  return run;
}

static void
lcms_icc_combo_box_set_active (GimpColorProfileComboBox *combo,
                               const gchar              *filename)
{
  cmsHPROFILE  profile = NULL;
  gchar       *label   = NULL;

  if (filename)
    profile = lcms_load_profile (filename, NULL);

  if (profile)
    {
      label = lcms_icc_profile_get_desc (profile);
      if (! label)
        label = lcms_icc_profile_get_name (profile);

      cmsCloseProfile (profile);
    }

  gimp_color_profile_combo_box_set_active (combo, filename, label);
  g_free (label);
}

static void
lcms_icc_file_chooser_dialog_response (GtkFileChooser           *dialog,
                                       gint                      response,
                                       GimpColorProfileComboBox *combo)
{
  if (response == GTK_RESPONSE_ACCEPT)
    {
      gchar *filename = gtk_file_chooser_get_filename (dialog);

      if (filename)
        {
          lcms_icc_combo_box_set_active (combo, filename);

          g_free (filename);
        }
    }

  gtk_widget_hide (GTK_WIDGET (dialog));
}

static GtkWidget *
lcms_icc_file_chooser_dialog_new (void)
{
  GtkWidget     *dialog;
  GtkFileFilter *filter;

  dialog = gtk_file_chooser_dialog_new (_("Select destination profile"),
                                        NULL,
                                        GTK_FILE_CHOOSER_ACTION_OPEN,

                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN,   GTK_RESPONSE_ACCEPT,

                                        NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_ACCEPT,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

#ifndef G_OS_WIN32
  {
    const gchar folder[] = "/usr/share/color/icc";

    if (g_file_test (folder, G_FILE_TEST_IS_DIR))
      gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog),
                                            folder, NULL);
  }
#endif

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All files (*.*)"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("ICC color profile (*.icc, *.icm)"));
  gtk_file_filter_add_pattern (filter, "*.[Ii][Cc][Cc]");
  gtk_file_filter_add_pattern (filter, "*.[Ii][Cc][Mm]");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

  return dialog;
}

static GtkWidget *
lcms_icc_combo_box_new (GimpColorConfig *config,
                        const gchar     *filename)
{
  GtkWidget   *combo;
  GtkWidget   *dialog;
  gchar       *history;
  gchar       *label;
  gchar       *name;
  cmsHPROFILE  profile;

  dialog = lcms_icc_file_chooser_dialog_new ();
  history = gimp_personal_rc_file ("profilerc");

  combo = gimp_color_profile_combo_box_new (dialog, history);

  g_free (history);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (lcms_icc_file_chooser_dialog_response),
                    combo);

  if (config->rgb_profile)
    profile = lcms_load_profile (config->rgb_profile, NULL);
  else
    profile = cmsCreate_sRGBProfile ();

  name = lcms_icc_profile_get_desc (profile);
  if (! name)
    name = lcms_icc_profile_get_name (profile);

  cmsCloseProfile (profile);

  label = g_strdup_printf (_("RGB workspace (%s)"), name);
  g_free (name);

  gimp_color_profile_combo_box_add (GIMP_COLOR_PROFILE_COMBO_BOX (combo),
                                    config->rgb_profile, label);
  g_free (label);

  if (filename)
    lcms_icc_combo_box_set_active (GIMP_COLOR_PROFILE_COMBO_BOX (combo),
                                   filename);
  else
    gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);

  return combo;
}

static GimpPDBStatusType
lcms_dialog (GimpColorConfig *config,
             gint32           image,
             gboolean         apply,
             LcmsValues      *values)
{
  GimpColorProfileComboBox *box;
  GtkWidget                *dialog;
  GtkWidget                *main_vbox;
  GtkWidget                *frame;
  GtkWidget                *label;
  GtkWidget                *combo;
  cmsHPROFILE               src_profile;
  gchar                    *name;
  gboolean                  success = FALSE;
  gboolean                  run;

  src_profile = lcms_image_get_profile (config, image, NULL);

  if (src_profile && ! lcms_icc_profile_is_rgb (src_profile))
    {
      g_printerr ("lcms: attached color profile is not for RGB color space "
                  "(skipping)\n");

      cmsCloseProfile (src_profile);
      src_profile = NULL;
    }

  if (! src_profile)
    src_profile = cmsCreate_sRGBProfile ();

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (apply ?
                            _("Convert to ICC Color Profile") :
                            _("Assign ICC Color Profile"),
                            PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func,
                            apply ? PLUG_IN_PROC_APPLY : PLUG_IN_PROC_SET,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,

                            apply ? GTK_STOCK_CONVERT : _("_Assign"),
                            GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  frame = gimp_frame_new (_("Current Color Profile"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  name = lcms_icc_profile_get_desc (src_profile);
  if (! name)
    name = lcms_icc_profile_get_name (src_profile);

  label = gtk_label_new (name);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (frame), label);
  gtk_widget_show (label);

  g_free (name);

  frame = gimp_frame_new (apply ? _("Convert to") : _("Assign"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  combo = lcms_icc_combo_box_new (config, NULL);
  gtk_container_add (GTK_CONTAINER (frame), combo);
  gtk_widget_show (combo);

  box = GIMP_COLOR_PROFILE_COMBO_BOX (combo);

  if (apply)
    {
      GtkWidget *vbox;
      GtkWidget *hbox;
      GtkWidget *toggle;

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);
      gtk_widget_show (vbox);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new_with_mnemonic (_("_Rendering Intent:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      combo = gimp_enum_combo_box_new (GIMP_TYPE_COLOR_RENDERING_INTENT);
      gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
      gtk_widget_show (combo);

      gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                                  values->intent,
                                  G_CALLBACK (gimp_int_combo_box_get_active),
                                  &values->intent);

      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

      toggle =
        gtk_check_button_new_with_mnemonic (_("_Black Point Compensation"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), values->bpc);
      gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &values->bpc);
    }

  while ((run = gimp_dialog_run (GIMP_DIALOG (dialog))) == GTK_RESPONSE_OK)
    {
      gchar       *filename = gimp_color_profile_combo_box_get_active (box);
      cmsHPROFILE  dest_profile;

      gtk_widget_set_sensitive (dialog, FALSE);

      if (filename)
        {
          dest_profile = lcms_load_profile (filename, NULL);
        }
      else
        {
          dest_profile = cmsCreate_sRGBProfile ();
        }

      if (dest_profile)
        {
          if (lcms_icc_profile_is_rgb (dest_profile))
            {
              if (apply)
                success = lcms_image_apply_profile (image,
                                                    src_profile, dest_profile,
                                                    filename,
                                                    values->intent,
                                                    values->bpc);
              else
                success = lcms_image_set_profile (image,
                                                  dest_profile, filename, TRUE);
            }
          else
            {
              gimp_message (_("Destination profile is not for RGB color space."));
            }

          cmsCloseProfile (dest_profile);
        }

      if (success)
        break;
      else
        gtk_widget_set_sensitive (dialog, TRUE);
    }

  gtk_widget_destroy (dialog);

  cmsCloseProfile (src_profile);

  return (run ?
          (success ? GIMP_PDB_SUCCESS : GIMP_PDB_EXECUTION_ERROR) :
          GIMP_PDB_CANCEL);
}
