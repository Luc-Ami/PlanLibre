/*
 * File: files.c header
 * Description: Contains config save/restore specific functions so that widgets can keep their
 *              settings betweeen saves.
 *
 *
 *
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef FILES_H
#define FILES_H

#include <gtk/gtk.h>
#include "misc.h"

/* Local macros */
#define CONFIG_DISABLE_SAVE_CONFIG_STRING "CONFIG_DISABLE_SAVE_CONFIG" /* Used by config->restart to check for reset */
#define CONFIG_DISABLE_SAVE_CONFIG 1 /* Used by config->restart to check for reset */
#define MASTER_OPTIONS_DATA "options1" /* Storage for the Phase One data inside the g_object */
#define MAX_RECENT_FILES 10

/* functions */

gboolean files_started_saving (APP_data *data);
void files_set_re_entrant_state (gboolean flag, APP_data *data);
gint open_file (gchar *path_to_file, APP_data *data);
void save (gchar *filename, APP_data *data);

GtkWidget *create_loadFileDialog (GtkWidget *win, APP_data *data, gchar *sFileType);
GtkWidget *create_saveFileDialog (gchar *sFileType, APP_data *data);
gint files_append_rsc_from_library (gchar *path_to_file, APP_data *data);
gchar *files_get_project_summary (APP_data *data);
gint files_copy_snapshot (gchar *path_to_source, APP_data *data);

void save_to_project (gchar *filename, APP_data *data);

#endif /* FILES_H */
