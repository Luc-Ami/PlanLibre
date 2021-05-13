
#include <libintl.h>
#include <locale.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <glib/gstdio.h> /* g_fopen, etc */
#include <gtk/gtk.h>
#include "support.h"
#include "misc.h"
#include "files.h"
#include "interface.h"
#include "savestate.h"
#include "calendars.h"

/*********************************
  startup for a standard
  g_application

**********************************/

static void planlibre_startup (APP_data *data)
{

  /* we add directories ton find various ressources */
  add_pixmap_directory (PACKAGE_DATA_DIR "/pixmaps/" PACKAGE); /* New location for all pixmaps */
  add_pixmap_directory (PACKAGE_DATA_DIR "/pixmaps"); /* Gnome users /usr/share/pixmaps folder */  
  add_ui_directory (PACKAGE_DATA_DIR "/planlibre/ui" ); /* New location for all glade xml files */

}


/*********************************
  activation for a standard
  g_application

**********************************/
static void
planlibre_activate (GApplication *app, APP_data *data)
{
  GList *list;
  GKeyFile *keyString;
  GError *err = NULL; 
  gchar *filename, *summary;

  /* get datas from xml Glade file - CRITICAL : only in activate phase !!!! */
  data->builder =NULL;
  data->builder = gtk_builder_new ();

  /* load UI file */

  /* theming */
//g_object_set(gtk_settings_get_default(),
  //  "gtk-application-prefer-dark-theme", TRUE,
   // NULL);

  
  if(!gtk_builder_add_from_file (data->builder, find_ui_file ("main.ui"), &err)) {
     g_error_free (err);
     misc_halt_after_glade_failure (data);
  }

  list = gtk_application_get_windows (GTK_APPLICATION (app));
  /* test for uniqueness */
  if (list) {
      gtk_window_present (GTK_WINDOW (list->data));
  }
  else
    {
      planlibre_prepare_GUI (app, data);
      gtk_widget_show (data->appWindow);
    }
  /* now we get configuration stored datas */
  createGKeyFile (data, data->appWindow);/* reload */
  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");
  /* same for application */
  data->keystring = keyString;
  data->app = GTK_APPLICATION(app);

  /* default name for current project */

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");
//  filename = g_strdup_printf ("%s.xml", _("noname_project"));
//filename = NULL;
  //summary = g_strdup_printf ("%s", _("No summary"));
//  summary = NULL;
//  store_current_file_in_keyfile (keyString, filename, summary);
//  g_free (summary);
//  g_free (filename);

  misc_display_app_status (FALSE, data);
  /* timeouts */
  misc_prepare_timeouts (data);
  /* calendars */
  calendars_init_main_calendar (data);
  /* dashboard */
  dashboard_init (data);

  /* prepare home page */
  home_set_app_logo (data);
  home_today_date (data);
  home_project_infos (data);
  home_project_tasks (data);
  home_project_milestones (data);
}

/**********************

  MAIN

***********************/

int main (int argc, char *argv[]) {
  GtkApplication *app;
  APP_data app_data;
  gint status;

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  /* we parse datas from config file - we get the configuration file   */
  app_data.gConfigFile = g_build_filename (g_get_user_config_dir (), 
                       "/planlibre/", PLANLIBRE_CONFIG, NULL); /* Create hidden directory to store PlanLibre config datas */
  /* we check if the directory already exists */
  if(!g_file_test (app_data.gConfigFile, G_FILE_TEST_EXISTS)) {
     printf("* PlanLibre critical : config.ini file absent or corrupted ! *\n");
     /* we create the directory */
     gint i = g_mkdir (g_strdup_printf ("%s/planlibre/", g_get_user_config_dir ()), S_IRWXU);/* i's a macro from gstdio.h */
  }

  timeline_reset_store_date ();
  misc_init_vars (&app_data);
  /* g_application mechanism */
  app = gtk_application_new ("org.gtk.planlibre", 0);
  app_data.app = app;
  g_signal_connect (app, "startup", G_CALLBACK (planlibre_startup), &app_data);
  g_signal_connect (app, "activate", G_CALLBACK (planlibre_activate), &app_data);

  /* main loop */
  status = g_application_run (G_APPLICATION (app), argc, argv);

  g_object_unref (app_data.builder);
  g_object_unref (app);

  return status;
}
