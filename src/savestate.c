/******************************
  functions to manage storage
  of application configuration
******************************/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* translations */
#include <libintl.h>
#include <locale.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>
#include <glib/gstdio.h> /* g_fopen, etc */
#include "support.h" /* glade requirement */
#include "savestate.h"
#include "secret.h"

/****************************

  FUNCTIONS

****************************/

/*
 * Internal helper: equivalent to mkdir -p on linux.
 * Will recursively create missing folders to make the required foldername, and mode.
 * Always returns true!
 */
gboolean mkFullDir (gchar *folderName, gint mode)
{
  gchar *partialFolderName, *pPartialFolderName;
  gchar **folderParts;
  gint i = 0;
  
  /* Completely split folderName into parts */
  folderParts = g_strsplit_set (folderName, G_DIR_SEPARATOR_S, -1);
  partialFolderName = g_strdup (folderName);
  pPartialFolderName = partialFolderName;

  while (folderParts[i] != NULL) {
    pPartialFolderName = g_stpcpy (pPartialFolderName, folderParts[i]);
    pPartialFolderName = g_stpcpy (pPartialFolderName, G_DIR_SEPARATOR_S);
    
    if (!g_file_test (partialFolderName, G_FILE_TEST_IS_DIR)) {
      g_mkdir (partialFolderName, mode);
    }
    i++;
  }
  
  return TRUE;
}

/**************************************************************
 *    Keyfile interface commands
 **************************************************************/

/*******************************************
 from Intel's conman, so many thanks !
  here : https://kernel.googlesource.com/pub/scm/network/connman/connman/+/2ea605a3fe31f03384fa3ad0f01a901da3a7c095/src/storage.c
*******************************************/
static GKeyFile *storage_load (const char *pathname)
{
	GKeyFile *keyfile = NULL;
	GError *error = NULL;
	keyfile = g_key_file_new ();
	if (!g_key_file_load_from_file (keyfile, pathname, 0, &error)) {
		printf ("*PlanLibre critical - Unable to load %s: %s *", pathname, error->message);
		g_clear_error (&error);
		g_key_file_free (keyfile);
		keyfile = NULL;
	}
	return keyfile;
}

/******************************

 store all program's settings

*******************************/
gint storage_save (gchar *pathname, APP_data *data_app)
{
  gchar *data = NULL, buffer[81];
  gsize length = 0;
  GError *error = NULL;
  gint ret = 0;
  gint width_height[2], pos_x, pos_y;
  GKeyFile *keyString;
  time_t rawtime; 

  keyString = g_object_get_data (G_OBJECT(data_app->appWindow), "config"); 

  gchar* tmpStr = g_strdup_printf (_(" %s settings auto-generated - Do not edit!"), "Planlibre 0.1");
  g_key_file_set_comment (keyString, NULL, NULL, tmpStr, NULL);
  g_key_file_set_string (keyString, "version-info", "version", CURRENT_VERSION);
  /* we store window geometry */
  gtk_window_get_position (GTK_WINDOW(data_app->appWindow), &pos_x, &pos_y); 
  gtk_window_get_size (GTK_WINDOW(data_app->appWindow), &width_height[0], &width_height[1]);
  g_key_file_set_integer (keyString, "application", "geometry.x", pos_x);
  g_key_file_set_integer (keyString, "application", "geometry.y", pos_y);
  g_key_file_set_integer (keyString, "application", "geometry.width", width_height[0]);
  g_key_file_set_integer (keyString, "application", "geometry.height",width_height[1]);

  /* we get the current date */
  time (&rawtime );

  data = g_key_file_to_data (keyString, &length, &error);

  if(!g_file_set_contents (data_app->gConfigFile, data, length, &error)) {
		printf ("*PlanLibre critical - Failed to store information: %s *", error->message);
		g_error_free (error);
  }
  g_free (data);
  g_free (tmpStr);

  return ret;
}

/*******************************
 Create a new config file
 ******************************/
void createNewKeyFile (APP_data *data_app, GtkWidget *win, GKeyFile *keyString)
{
    gint width_height[2], i;
    gchar buffer[81], *tmpStr2;
    time_t rawtime; 
    GDate current_date;
    GError *error;

    g_date_set_time_t (&current_date, time (NULL)); /* date of today */

    gchar* tmpStr = g_strdup_printf (_(" %s settings auto-generated - Do not edit!"), "Planlibre 0.1");
    g_key_file_set_comment (keyString, NULL, NULL, tmpStr, NULL);
    g_key_file_set_string (keyString, "version-info", "version", CURRENT_VERSION);
    g_key_file_set_integer (keyString, "application", "geometry.x", 0);
    g_key_file_set_integer (keyString, "application", "geometry.y", 0);
    /* we get the current  window geometry */
    gtk_window_get_size (GTK_WINDOW(win),
                       &width_height[0], &width_height[1]);
    g_key_file_set_integer (keyString, "application", "geometry.width",  width_height[0]);
    g_key_file_set_integer (keyString, "application", "geometry.height", width_height[1]);
    /* we get the current date */
    time ( &rawtime );
    strftime(buffer, 80, "%c", localtime(&rawtime));/* don't change parameter %x */
  
    g_key_file_set_string (keyString, "application", "current-file", get_path_to_datas_file (buffer));

    /* setup and store the basename */
    tmpStr2= g_key_file_get_string (keyString, "application", "current-file", NULL);
    g_key_file_set_string (keyString, "application", "current-file-basename", 
                     g_filename_display_basename (tmpStr2));

    g_free (tmpStr2);
    /* general settings */
    g_key_file_set_boolean (keyString, "application", "interval-save", FALSE);
    g_key_file_set_integer (keyString, "application", "interval-save-frequency", 5);
    g_key_file_set_boolean (keyString, "application", "prompt-before-quit", TRUE);
    g_key_file_set_boolean (keyString, "application", "prompt-before-overwrite", TRUE);
    g_key_file_set_boolean (keyString, "application", "trello-user-token-defined", FALSE);
    g_key_file_set_integer (keyString, "application", "timer-run-time", 25);
    g_key_file_set_integer (keyString, "application", "timer-pause-time", 5);
    /* recent files */
    for(i=0;i<MAX_RECENT_FILES;i++) {
       g_key_file_set_string (keyString, "history", g_strdup_printf ("recent-file-%d", i), 
                    "");
       g_key_file_set_string (keyString, "history", g_strdup_printf ("recent-content-%d", i), 
                      "");
    }
 
    /* editor */
    /* text color for reports - dark grey from ElementaryOS */
    g_key_file_set_double (keyString, "editor", "text.color.red", 0.23);
    g_key_file_set_double (keyString, "editor", "text.color.green", 0.25);
    g_key_file_set_double (keyString, "editor", "text.color.blue", 0.27); 
    /* default text font */
    g_key_file_set_string (keyString, "editor", "font", "Sans 12");

    /* server */
    g_key_file_set_string (keyString, "server", "name", "?");
    g_key_file_set_string (keyString, "server", "adress", "?");
    g_key_file_set_integer (keyString, "server", "port", 465);
    g_key_file_set_boolean (keyString, "server", "ssltls", TRUE);
    g_key_file_set_string (keyString, "server", "id", "?");

    /* automated tasks */
    g_key_file_set_boolean (keyString, "collaborate", "mail_tasks_start_tomorrow", FALSE);
    g_key_file_set_boolean (keyString, "collaborate", "mail_tasks_overdue_today", FALSE);
    g_key_file_set_boolean (keyString, "collaborate", "mail_tasks_completed_yesterday", FALSE);
    g_key_file_set_boolean (keyString, "collaborate", "mail_tasks_ending_today", FALSE);
    g_key_file_set_integer (keyString, "collaborate", "last_auto_mailing-day", -1);
    g_key_file_set_integer (keyString, "collaborate", "last_auto_mailing-month", -1);
    g_key_file_set_integer (keyString, "collaborate", "last_auto_mailing-year", -1);

    gchar *context = g_key_file_to_data (keyString, NULL, &error);
    g_file_set_contents (data_app->gConfigFile, context, -1, NULL);
    g_free (tmpStr);
    g_free (context);
    return;
}

/* see : http://distro.ibiblio.org/vectorlinux/stretchedthin/yad/0.12.2/src/yad-0.12.2/src/util.c
 * Attempts to load the searchmonkey config.ini file.
 * If invalid, or non-existant, creates new config file.
 */
void createGKeyFile (APP_data *data_app, GtkWidget *win)
{
  GKeyFile *keyString;
  gint i, width, height, pos_x, pos_y;
  gsize length;
  gchar *tmpStr, buffer[81];
  time_t rawtime; 
  GDate current_date;
  GdkRGBA color;   
  GtkWidget *menuitemExportTrello; 


  g_date_set_time_t (&current_date, time (NULL)); /* date of today */

  keyString = g_key_file_new ();

  if(!g_key_file_load_from_file (keyString, data_app->gConfigFile,
                                  G_KEY_FILE_KEEP_COMMENTS,
                                  NULL)) {printf("pb =%s\n",data_app->gConfigFile);

    createNewKeyFile(data_app, win, keyString);
  } 
  /* we read datas for config.ini file */

  if(g_key_file_has_key (keyString, "application", "geometry.width", NULL)) {
    width = g_key_file_get_integer (keyString, "application", "geometry.width",  NULL);  
  }
  else width =980;
  if (g_key_file_has_key (keyString, "application", "geometry.height", NULL)) {
    height = g_key_file_get_integer (keyString, "application", "geometry.height",  NULL);  
  }
  else height =700;
  pos_x = 64;
  pos_y = 64;
  if(g_key_file_has_key (keyString, "application", "geometry.x", NULL)) {
    pos_x = g_key_file_get_integer (keyString, "application", "geometry.x",  NULL);  
  }
  if (g_key_file_has_key (keyString, "application", "geometry.y", NULL)) {
    pos_y = g_key_file_get_integer (keyString, "application", "geometry.y",  NULL);  
  }

  gtk_window_move (GTK_WINDOW(win), pos_x, pos_y);
  gtk_window_resize (GTK_WINDOW(win), width, height);
  /* some stuff about default file */
  if(!g_key_file_has_key (keyString, "application", "current-file", NULL)) { 
    /* we get the current date */
    time ( &rawtime );
    strftime(buffer, 80, "%c", localtime(&rawtime));
    g_key_file_set_string (keyString, "application", "current-file", 
                      get_path_to_datas_file (buffer));
  }
  /* general settings */
  if(!g_key_file_has_key (keyString, "application", "interval-save", NULL)) {
    g_key_file_set_boolean(keyString, "application", "interval-save", FALSE);
  }


  if(!g_key_file_has_key (keyString, "application", "interval-save-frequency", NULL)) {
    g_key_file_set_integer (keyString, "application", "interval-save-frequency", 5);
  }
  if(!g_key_file_has_key (keyString, "application", "prompt-before-quit", NULL)) {
    g_key_file_set_boolean(keyString, "application", "prompt-before-quit", TRUE);
  }
  if(!g_key_file_has_key (keyString, "application", "prompt-before-overwrite", NULL)) {
    g_key_file_set_boolean(keyString, "application", "prompt-before-overwrite", TRUE);
  }
  /* trello */
  menuitemExportTrello = GTK_WIDGET(gtk_builder_get_object (data_app->builder, "menuitemExportTrello"));
  if(!g_key_file_has_key (keyString, "application", "trello-user-token-defined", NULL)) {
    g_key_file_set_boolean(keyString, "application", "trello-user-token-defined", FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET(menuitemExportTrello), FALSE);
  }
  else {
    gchar *password = find_my_trello_password();
    if(password && g_key_file_get_boolean (keyString, "application", "trello-user-token-defined", NULL)) {
        gtk_widget_set_sensitive (GTK_WIDGET(menuitemExportTrello), TRUE);
        g_free (password);
    }
    else {
        gtk_widget_set_sensitive (GTK_WIDGET(menuitemExportTrello), FALSE);
    }
  }/* elsif */

  if(!g_key_file_has_key (keyString, "application", "timer-run-time", NULL)) {
    g_key_file_set_integer (keyString, "application", "timer-run-time", 25);
  }
  if(!g_key_file_has_key (keyString, "application", "timer-pause-time", NULL)) {
    g_key_file_set_integer (keyString, "application", "timer-pause-time", 5);
  }
  /* get and store the basename */
  tmpStr= g_key_file_get_string (keyString, "application", "current-file", NULL);
  g_key_file_set_string (keyString, "application", "current-file-basename", 
                      g_filename_display_basename (tmpStr));  

  /* check for recent files and recent files summary */
  for(i=0;i<MAX_RECENT_FILES;i++) {
     if(!g_key_file_has_key (keyString, "history", g_strdup_printf ("recent-file-%d",i ), NULL)) {
         g_key_file_set_string (keyString, "history", g_strdup_printf ("recent-file-%d", i),"");
     }
     if(!g_key_file_has_key (keyString, "history", g_strdup_printf ("recent-content-%d",i ), NULL)) {
         g_key_file_set_string (keyString, "history", g_strdup_printf ("recent-content-%d", i),"");
     }
  }

  /* if the current file-0 is empty there is something wrong, so we store the current file name as first entry */
  if(!g_key_file_has_key (keyString, "history", "recent-file-0", NULL)) {
         g_key_file_set_string (keyString, "history", "recent-file-0", tmpStr );
  }
  g_free (tmpStr);
/* TODO : same thing for file content ? */

  /* editor */
  /* report text color */
  if(!g_key_file_has_key (keyString, "editor", "text.color.red", NULL)) { 
    g_key_file_set_double (keyString, "editor", "text.color.red", 0.23);
  }
  if(!g_key_file_has_key (keyString, "editor", "text.color.green", NULL)) { 
    g_key_file_set_double (keyString, "editor", "text.color.green", 0.25);
  }
  if(!g_key_file_has_key (keyString, "editor", "text.color.blue", NULL)) { 
    g_key_file_set_double (keyString, "editor", "text.color.blue", 0.27);
  } 
  /* default text font */
  if(!g_key_file_has_key (keyString, "editor", "font", NULL)) { 
     g_key_file_set_string (keyString, "editor", "font", "Sans 12");
  }

  /* server */
  if(!g_key_file_has_key (keyString, "server", "name", NULL)) { 
    g_key_file_set_string (keyString, "server", "name", "?");
  }
  if(!g_key_file_has_key (keyString, "server", "adress", NULL)) { 
    g_key_file_set_string (keyString, "server", "adress", "?");
  }
  if(!g_key_file_has_key (keyString, "server", "port", NULL)) { 
    g_key_file_set_integer (keyString, "server", "port", 465);
  }
  if(!g_key_file_has_key (keyString, "server", "ssltls", NULL)) { 
    g_key_file_set_boolean (keyString, "server", "ssltls", TRUE);
  }
  if(!g_key_file_has_key (keyString, "server", "id", NULL)) { 
    g_key_file_set_string (keyString, "server", "id", "?");
  }

  /* automated tasks */
  if(!g_key_file_has_key (keyString, "collaborate", "mail_tasks_start_tomorrow", NULL)) { 
    g_key_file_set_boolean (keyString, "collaborate", "mail_tasks_start_tomorrow", FALSE);
  }
  if(!g_key_file_has_key (keyString, "collaborate", "mail_tasks_overdue_today", NULL)) { 
    g_key_file_set_boolean (keyString, "collaborate", "mail_tasks_overdue_today", FALSE);
  }
  if(!g_key_file_has_key (keyString, "collaborate", "mail_tasks_completed_yesterday", NULL)) { 
    g_key_file_set_boolean (keyString, "collaborate", "mail_tasks_completed_yesterday", FALSE);
  }
  if(!g_key_file_has_key (keyString, "collaborate", "mail_tasks_ending_today", NULL)) { 
    g_key_file_set_boolean (keyString, "collaborate", "mail_tasks_ending_today", FALSE);
  }
  if(!g_key_file_has_key (keyString, "collaborate", "last_auto_mailing-day", NULL)) {
    g_key_file_set_integer (keyString, "collaborate", "last_auto_mailing-day", -1);
  }
  if(!g_key_file_has_key (keyString, "collaborate", "last_auto_mailing-month", NULL)) {
    g_key_file_set_integer (keyString, "collaborate", "last_auto_mailing-month", -1);
  }
  if(!g_key_file_has_key (keyString, "collaborate", "last_auto_mailing-year", NULL)) {
    g_key_file_set_integer (keyString, "collaborate", "last_auto_mailing-year", -1);
  }
  /* store in main window */
  g_object_set_data (G_OBJECT(win), "config", keyString);
//  g_key_file_free (keyString);
}

/*
 * Saves config file (config.ini) to disk, and frees associated memory.
 * Automatically called when save data is destroyed (i.e. user closed searchmonkey).
 */
void destroyGKeyFile (APP_data *data_app, GtkWidget *win)
{
  GKeyFile *keyString = g_object_get_data (G_OBJECT(win), "config");
  printf ("* PlanLibre : GKeyfile destroyed successfully *\n");

  //  storeGKeyFile(keyString);

  g_key_file_free (keyString);
  if(data_app->gConfigFile != NULL) {
    g_free (data_app->gConfigFile);
  }
}

void storeGKeyFile(APP_data *data_app, GKeyFile *keyString)
{
  gsize length;
  gchar **outText;
  GError *error = NULL;
  gchar *folderName;
  GtkWidget *okCancelDialog;
  
     
  /* Write the configuration file to disk */
  folderName = g_path_get_dirname (data_app->gConfigFile);
  outText = g_key_file_to_data (keyString, &length, &error);

  if (!g_file_get_contents (data_app->gConfigFile, outText, &length, &error)) {
    /* Unable to immediately write to file, so attempt to recreate folders */
    mkFullDir(folderName, S_IRWXU);

    if (!g_file_get_contents (data_app->gConfigFile, outText, &length, &error)) { 
      g_print(_("*PlanLibre critical : Error saving %s: %s*\n"), data_app->gConfigFile, error->message);
      g_error_free (error);
      error = NULL;
    }
  }
  
  g_free (outText);
  g_free (folderName);
}

/***********************************
  set defaults values for 
  current filename and store 
  in config file in  memory  
**********************************/
void store_current_file_in_keyfile (GKeyFile *keyString, gchar *filename, gchar *summary)
{
   g_key_file_set_string (keyString, "application", "current-file", filename);
   /* setup and store the basename */
   g_key_file_set_string (keyString, "application", "current-file-basename", 
                      g_filename_display_basename (filename));
   g_key_file_set_string (keyString, "history", "recent-file-0", filename );
   g_key_file_set_string (keyString, "history", "recent-content-0", summary );/* start of file's content */
}

/***********************************
 rearrange files' order in recent
  file list ; do the same for
  extracts
***********************************/
void rearrange_recent_file_list (GKeyFile *keyString)
{
  gint i;
  gchar *recent_prev, *content_prev;

  for(i=MAX_RECENT_FILES-1; i>0; i--) {     
     if(g_key_file_has_key (keyString, "history", g_strdup_printf ("recent-file-%d",i-1 ), NULL)) {
                 recent_prev = g_strdup_printf ("%s", g_key_file_get_string (keyString, "history", 
                                          g_strdup_printf ("recent-file-%d",i-1), NULL));
                 g_key_file_set_string (keyString, "history", g_strdup_printf ("recent-file-%d",i), recent_prev);
                 g_free (recent_prev);
                 /* summaries */
                 content_prev = g_strdup_printf ("%s", g_key_file_get_string (keyString, "history", 
                                          g_strdup_printf ("recent-content-%d",i-1), NULL));
                 g_key_file_set_string (keyString, "history", g_strdup_printf ("recent-content-%d",i), content_prev);
                 g_free (content_prev);

     }
  }
}

