#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* translations */
#include <libintl.h>
#include <locale.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <stdlib.h>
#include "string.h"
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <cairo-svg.h>
#include <cairo-pdf.h>
#include <goocanvas.h>
#include "support.h"
#include "interface.h"
#include "callbacks.h"
#include "misc.h"
#include "properties.h"
#include "ressources.h"
#include "tasks.h"
#include "timeline.h"
#include "assign.h"
#include "dashboard.h"
#include "files.h"
#include "cursor.h"
#include "search.h"
#include "report.h"
#include "export.h"
#include "settings.h"
#include "mail.h"
#include "calendars.h"
#include "import.h"
#include "tasksutils.h"
#include "tasksdialogs.h"
#include "undoredo.h"
#include "secret.h"
#include "trello.h"
#include "home.h"
#include "links.h"
#include "savestate.h"
#include "msproject.h"

/******************************
  PUBLIC : same CB
  for eventbox on "home" page.
  Requires (in Glade fill)
  adding og butteon_press event
*******************************/
gboolean on_home_event_box_enter (GtkWidget *widget,
               GdkEvent  *event,
               APP_data   *data)
{
 
  /* switch to dashboard notebook page */
  cursor_pointer_hand (data->appWindow); 
  return TRUE;
}

/******************************
  PUBLIC : same CB
  for eventbox on "home" page.
  Requires (in Glade fill)
  adding og butteon_press event
*******************************/
gboolean on_home_event_box_leave (GtkWidget *widget,
               GdkEvent  *event,
               APP_data   *data)
{
 
  /* switch to dashboard notebook page */
  cursor_normal (data->appWindow);
  return TRUE;
}

/******************************
  PUBLIC : same CB
  for eventbox on "home" page.
  Requires (in Glade fill)
  adding og butteon_press event
*******************************/
gboolean on_home_event_box_press (GtkWidget *widget,
               GdkEvent  *event,
               APP_data   *data)
{
 
  /* switch to dashboard notebook page */
  GtkWidget *notebook = GTK_WIDGET(gtk_builder_get_object (data->builder, "notebook1"));
  gtk_notebook_set_current_page (GTK_NOTEBOOK(notebook), 4);
  return TRUE;
}
/*******************************
  callbacks for settings
  switches widgets
*******************************/
gboolean on_switch_use_trello_account_toggled (GObject *object, GdkEvent *event, APP_data *data)
{
printf ("trello notifiy \n");// ,entryTrelloUserToken  switchAllowTrelloAcess

  GKeyFile *keyString;

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config"); 

  gboolean active = gtk_switch_get_active (GTK_SWITCH (object));
//  g_key_file_set_boolean (keyString, "collaborate", "mail_tasks_start_tomorrow", active);

  return TRUE;
}

/* cb for Trello's secret token dialog */
void on_btn_trello_token_clicked (GtkButton *button, APP_data *data)
{
  GtkWidget *dialog, *entry, *labelStatus, *switchAllowTrelloAcess, *labelTrelloBtn;
  gint ret;
  /* get datas from xml Glade file - CRITICAL : only in activate phase !!!! */
  GtkBuilder *builder = NULL;
  builder = gtk_builder_new ();
  GError *err = NULL; 

  if(!gtk_builder_add_from_file (builder, find_ui_file ("usertoken.ui"), &err)) {
     g_error_free (err);
     misc_halt_after_glade_failure (data);
  }
  /* Get the dialog from the glade file. */
  gtk_builder_connect_signals (builder, data);
  dialog = GTK_WIDGET(gtk_builder_get_object (builder, "dialogRequestToken"));
  entry = GTK_WIDGET(gtk_builder_get_object (builder, "entrySecretToken"));
  labelStatus = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelTokenStatus")); 
  switchAllowTrelloAcess = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchAllowTrelloAcess")); 
  labelTrelloBtn = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelTrelloBtn")); 
  ret = gtk_dialog_run (GTK_DIALOG (dialog));
  if(ret==1) {
     /* we change status on settings dialog */
     gtk_label_set_markup (GTK_LABEL (labelStatus), ("<i>Stored</i>"));
     gtk_widget_set_sensitive (GTK_WIDGET(switchAllowTrelloAcess), TRUE);
     gtk_label_set_text (GTK_LABEL (labelTrelloBtn), ("Change user token ..."));  
     /* we update keyring */
     store_my_trello_password ((gchar *)gtk_entry_get_text (GTK_ENTRY(entry)));
     /* update config file */

  }
  /* if response == OK we store in Gnome Keyring */

  gtk_widget_destroy (GTK_WIDGET(dialog));
  g_object_unref (builder);
}


gboolean on_switch_start_tomorrow_toggled (GObject *object,  GdkEvent  *event, APP_data *data)
{

  GKeyFile *keyString;

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config"); 

  gboolean active = gtk_switch_get_active (GTK_SWITCH (object));
  g_key_file_set_boolean (keyString, "collaborate", "mail_tasks_start_tomorrow", active);

  return TRUE;
}

gboolean on_switch_ending_today_toggled (GObject *object, GdkEvent  *event, APP_data *data)
{

  GKeyFile *keyString;

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config"); 

  gboolean active = gtk_switch_get_active ( GTK_SWITCH (object));
  g_key_file_set_boolean (keyString, "collaborate", "mail_tasks_ending_today", active);

  return TRUE;
}

gboolean on_switch_overdue_today_toggled (GObject *object, GdkEvent  *event, APP_data *data)
{

  GKeyFile *keyString;

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config"); 

  gboolean active = gtk_switch_get_active ( GTK_SWITCH (object));
  g_key_file_set_boolean (keyString, "collaborate", "mail_tasks_overdue_today", active);
  return TRUE;
}

gboolean on_switch_completed_yesterday_toggled (GObject *object, GdkEvent  *event, APP_data *data)
{

  GKeyFile *keyString;

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config"); 

  gboolean active = gtk_switch_get_active ( GTK_SWITCH (object));
  g_key_file_set_boolean (keyString, "collaborate", "mail_tasks_completed_yesterday", active);
  return TRUE;
}

/***********************************
 Export project to Trello servces
***********************************/
void on_trello_export_project_activate (GtkMenuItem *menuitem, APP_data *data)
{
   gint rc = 0;
   GtkWidget *statusProgress;

  statusProgress = GTK_WIDGET(gtk_builder_get_object (data->builder, "boxProgressMail"));
  gtk_widget_show_all (GTK_WIDGET(statusProgress));

   cursor_busy (data->appWindow);
   rc = trello_export_project (data);
   cursor_normal (data->appWindow);
   if(rc!=0) {
      misc_ErrorDialog (data->appWindow, _("An error occured during export to Trello.\nPlease, check your password and/or your\nInternet connection !"));
   }
   else {
      misc_InfoDialog (data->appWindow, _("Export to Trello completed.\nCongratulations !"));
   }
}
/**********************************************

callback : menus/buttons "timeline" clicked

**********************************************/
void
on_timeline_export_pdf_activate (GtkMenuItem *menuitem, APP_data *data)
{
  gint ret = timeline_export (TIMELINE_EXPORT_PDF, data);
}

void
on_timeline_export_image_activate (GtkMenuItem *menuitem, APP_data *data)
{
  gint ret = timeline_export (TIMELINE_EXPORT_PNG, data);
}

void
on_timeline_export_drawing_activate (GtkMenuItem *menuitem, APP_data *data)
{
  gint ret = timeline_export (TIMELINE_EXPORT_SVG, data);
}


void
on_button_export_pdf_clicked (GtkButton *button, APP_data *data)
{
  gint ret = timeline_export (TIMELINE_EXPORT_PDF, data);
}

void
on_button_export_image_clicked (GtkButton *button, APP_data *data)
{
  gint ret = timeline_export (TIMELINE_EXPORT_PNG, data);
}

void
on_button_export_drawing_clicked (GtkButton *button, APP_data *data)
{
  gint ret = timeline_export (TIMELINE_EXPORT_SVG, data);
  /* renderings : http://zetcode.com/gfx/cairo/cairobackends/ */
  /* printing https://developer.gnome.org/goocanvas2/stable/GooCanvas.html#goo-canvas-render */
// bcp mieux : https://github.com/GNOME/goocanvas/blob/master/demo/demo.c

}

/******************************* 
  timeline navigation callbacks 
*******************************/
void on_button_TL_go_start_clicked (GtkButton *button, APP_data *data)
{

  GtkAdjustment *adjustment
		       = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowTimeline")));
  gtk_adjustment_set_value (GTK_ADJUSTMENT(adjustment), 0);
  timeline_store_date (data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
  timeline_remove_ruler (data);
  timeline_remove_all_tasks (data);
  /* update timeline */
  timeline_draw_calendar_ruler (timeline_get_ypos (data), data);
 // gtk_adjustment_set_value (GTK_ADJUSTMENT(vadj), timeline_get_ypos ());/* mandatory AFTER ruler's redraw */
  timeline_draw_all_tasks (data);
}



/******************************* 
  timeline navigation callbacks 
*******************************/
void on_button_TL_prev_year_clicked (GtkButton *button, APP_data *data)
{
  gint dd=0, mm=0, yy=0;
  GDate date;

  timeline_get_date (&dd, &mm, &yy);
  g_date_set_dmy (&date, dd, mm, yy);
  g_date_subtract_days (&date, 365);
  dd = g_date_get_day (&date);
  mm = g_date_get_month (&date);
  yy = g_date_get_year (&date);
  timeline_store_date (dd, mm, yy);
  /* update timeline */
  timeline_remove_ruler (data);
  timeline_remove_all_tasks (data);
  timeline_draw_calendar_ruler (timeline_get_ypos (data), data);
 // gtk_adjustment_set_value (GTK_ADJUSTMENT(vadj), timeline_get_ypos ());/* mandatory AFTER ruler's redraw */
  timeline_draw_all_tasks (data);
}


void on_button_TL_prev_page_clicked (GtkButton *button, APP_data *data)
{
  gint dd=0, mm=0, yy=0;
  GDate date;

  timeline_get_date (&dd, &mm, &yy);
  g_date_set_dmy (&date, dd, mm, yy);
  g_date_subtract_days (&date, 30);
  dd = g_date_get_day (&date);
  mm = g_date_get_month (&date);
  yy = g_date_get_year (&date);
  timeline_store_date (dd, mm, yy);
  /* update timeline */
  timeline_remove_ruler (data);
  timeline_remove_all_tasks (data);
  timeline_draw_calendar_ruler (timeline_get_ypos (data), data);
 // gtk_adjustment_set_value (GTK_ADJUSTMENT(vadj), timeline_get_ypos ());/* mandatory AFTER ruler's redraw */
  timeline_draw_all_tasks (data);
}

void on_button_TL_next_page_clicked (GtkButton *button, APP_data *data)
{
  gint dd, mm, yy;
  GDate date;

  timeline_get_date (&dd, &mm, &yy);
  g_date_set_dmy (&date, dd, mm, yy);
  g_date_add_days (&date, 30);
  dd = g_date_get_day (&date);
  mm = g_date_get_month (&date);
  yy = g_date_get_year (&date);
  timeline_store_date (dd, mm, yy);
  /* update timeline */
  timeline_remove_ruler (data);
  timeline_remove_all_tasks (data);
  timeline_draw_calendar_ruler (timeline_get_ypos (data), data);
 // gtk_adjustment_set_value (GTK_ADJUSTMENT(vadj), 0);/* mandatory AFTER ruler's redraw */
  timeline_draw_all_tasks (data);
}

void on_button_TL_next_year_clicked (GtkButton *button, APP_data *data)
{
  gint dd, mm, yy;
  GDate date;

  timeline_get_date (&dd, &mm, &yy);
  g_date_set_dmy (&date, dd, mm, yy);
  g_date_add_days (&date, 365);
  dd = g_date_get_day (&date);
  mm = g_date_get_month (&date);
  yy = g_date_get_year (&date);
  timeline_store_date (dd, mm, yy);
  /* update timeline */
  timeline_remove_ruler (data);
  timeline_remove_all_tasks (data);
  timeline_draw_calendar_ruler (timeline_get_ypos (data), data);
 // gtk_adjustment_set_value (GTK_ADJUSTMENT(vadj), 0);/* mandatory AFTER ruler's redraw */
  timeline_draw_all_tasks (data);
}

/*************************** 
export assignments diagram 
***************************/

void
on_button_export_assign_pdf_clicked (GtkButton *button, APP_data *data)
{
  gint ret = assign_export (ASSIGN_EXPORT_PDF, data);
}

void
on_menu_export_assign_image_clicked (GtkMenuItem *menuitem, APP_data *data)
{
  gint ret = assign_export (ASSIGN_EXPORT_PNG, data);
}

void
on_menu_export_assign_drawing_clicked (GtkMenuItem *menuitem, APP_data *data)
{
  gint ret = assign_export (ASSIGN_EXPORT_SVG, data);
  /* renderings : http://zetcode.com/gfx/cairo/cairobackends/ */
  /* printing https://developer.gnome.org/goocanvas2/stable/GooCanvas.html#goo-canvas-render */
// bcp mieux : https://github.com/GNOME/goocanvas/blob/master/demo/demo.c

}

/*************************************
  PUBLIC : global time shift
*************************************/
void on_timeline_time_shift_activate (GtkMenuItem *menuitem, APP_data *data)
{
  GtkWidget *dialog, *labelBtn;
  gchar *newDate = NULL;
  gint ret, cmp_date, cmp_duration, dd, mm, yy;
  GDate date_start, new_date;
  GtkAdjustment *vadj;

  dialog = timeline_create_timeshift_dialog (FALSE, -1, data);
  ret = gtk_dialog_run (GTK_DIALOG(dialog));
  /* update widgets */
  labelBtn = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelChangeDate"));
  newDate = g_strdup_printf ("%s", gtk_label_get_text (GTK_LABEL(labelBtn)));
  /* clean */
  gtk_widget_destroy (GTK_WIDGET(dialog));/* OK = 1 */
  g_object_unref (data->tmpBuilder); 

  if(ret==1) {
    /* we get current dates limits from properties */
    g_date_set_parse (&date_start, data->properties.start_date);
    g_date_set_parse (&new_date, newDate);
    /* how many days shift ? */
    cmp_date = g_date_days_between (&date_start, &new_date);

    if(cmp_date != 0) {
          /* undo engine */
          undo_datas *tmp_value;  
          tmp_value = g_malloc (sizeof(undo_datas));
          tmp_value->opCode = OP_TIMESHIFT_TASKS;
          tmp_value->datas = NULL;
          tmp_value->list = NULL;
          tmp_value->groups = NULL;
          tmp_value->t_shift = cmp_date;
          undo_push (CURRENT_STACK_TASKS, tmp_value, data);
          dashboard_remove_all (FALSE, data);

          properties_shift_dates (cmp_date, data);
          /* update 'due' status for tasks and dashboard */
          tasks_shift_dates (cmp_date, FALSE, -1, data);

          /* we store extrems dates to update timeline */
          data->earliest = misc_get_earliest_date (data);
          data->latest = misc_get_latest_date (data);
          dd = g_date_get_day (&new_date);
          mm = g_date_get_month (&new_date);
          yy = g_date_get_year (&new_date);
          timeline_store_date (dd, mm, yy);
          /* redraw timeline */
          timeline_remove_all_tasks (data);
          vadj = gtk_scrolled_window_get_vadjustment 
                          (GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowTimeline")));
          gtk_adjustment_set_value (GTK_ADJUSTMENT(vadj), 0);
          timeline_draw_calendar_ruler (0, data);
          timeline_draw_all_tasks (data);
          /* redraw assignments */
          assign_remove_all_tasks (data);
          assign_draw_all_visuals (data);
          /* redraw dashboard - useless, done during call to tasks_shift_dates () */
          /* clear report */
          report_clear_text (data->buffer, "left");
          report_default_text (data->buffer);
          report_set_buttons_menus (FALSE, FALSE, data);
          misc_display_app_status (TRUE, data);
       }
    }

  if(newDate)
     g_free (newDate);
 
}

/*************************************
  PUBLIC : menus "mails" clicked
**************************************/
void
on_collaborate_mail_over_activate (GtkMenuItem *menuitem, APP_data *data_app)
{

  if(collaborate_test_not_matching (MAIL_TYPE_OVERDUE, data_app)) {
     cursor_busy (data_app->appWindow);
     gint ret = collaborate_send_manual_email (MAIL_TYPE_OVERDUE, data_app);
     cursor_normal (data_app->appWindow);
  }
}

void
on_collaborate_mail_incoming_activate (GtkMenuItem *menuitem, APP_data *data_app)
{

  if(collaborate_test_not_matching (MAIL_TYPE_INCOMING, data_app)) {
     cursor_busy (data_app->appWindow);
     gint ret = collaborate_send_manual_email (MAIL_TYPE_INCOMING, data_app);
     cursor_normal (data_app->appWindow);
  }
}

void
on_collaborate_mail_completed_activate (GtkMenuItem *menuitem, APP_data *data_app)
{

  if(collaborate_test_not_matching (MAIL_TYPE_COMPLETED, data_app)) {
     cursor_busy (data_app->appWindow);
     gint ret = collaborate_send_manual_email (MAIL_TYPE_COMPLETED, data_app);
     cursor_normal (data_app->appWindow);
  }
}

void
on_collaborate_mail_report_activate (GtkMenuItem *menuitem, APP_data *data_app)
{

  cursor_busy (data_app->appWindow);
  gint ret = collaborate_send_manual_email (MAIL_TYPE_REPORT, data_app);
  cursor_normal (data_app->appWindow);

}

void
on_collaborate_mail_free_activate (GtkMenuItem *menuitem, APP_data *data_app)
{

  cursor_busy (data_app->appWindow);
  gint ret = collaborate_send_manual_email (MAIL_TYPE_FREE, data_app);
  cursor_normal (data_app->appWindow);

}

void
on_collaborate_mail_icalendar_activate (GtkMenuItem *menuitem, APP_data *data_app)
{

  cursor_busy (data_app->appWindow);
  gint ret = collaborate_send_manual_email (MAIL_TYPE_CALENDAR, data_app);
  cursor_normal (data_app->appWindow);

}

/*************************************
callback : menu "prefs" clicked

**************************************/
void
on_edition_prefs_activate (GtkMenuItem *menuitem, APP_data *data_app)
{
  settings (data_app);
}

/*********************************************
  LOCAL : long list of check procedures
  in order to choose a valid filename
  in case of user aborts, the function
  returns NULL
*********************************************/
static gchar *get_a_filename (APP_data *data, gchar *defName)
{
  gchar *path_to_file = NULL, *tmpFileName;
  GtkWidget *alertDlg;
  GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
  GKeyFile *keyString;
  GtkFileFilter *filter = gtk_file_filter_new ();
  gtk_file_filter_add_pattern (filter, "*.plibre");
  gtk_file_filter_set_name (filter, _("PlanLibre files"));
  gint ret;

  files_set_re_entrant_state (TRUE, data);

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");

  /* first, we display a file dialog */
  GtkWidget *dialog = create_saveFileDialog (_("Save current project as ..."), data);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir());
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  /* we must extract a short filename */
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), defName);
  /* run dialog for file selection */
  if(gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
         path_to_file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
         gtk_widget_destroy (GTK_WIDGET(dialog));
  }/* if [OK] */
  else {
     gtk_widget_destroy (GTK_WIDGET(dialog));
     return NULL;
  }

  /* ok, now we check file extension */
  if(!g_str_has_suffix (path_to_file, ".PLIBRE" ) && !g_str_has_suffix (path_to_file, ".plibre" )) {
         /* we correct the filename */
         tmpFileName = g_strdup_printf ("%s.plibre", path_to_file);
         g_free (path_to_file);
         path_to_file = tmpFileName;
  }/* endif extension XML*/

  /* we must check if a file with same name exists */

  if(g_file_test (path_to_file, G_FILE_TEST_EXISTS)
     && g_key_file_get_boolean (keyString, "application", "prompt-before-overwrite",NULL ) ) {

           /* dialog to avoid overwriting */
           alertDlg = gtk_message_dialog_new (GTK_WINDOW(data->appWindow),
                          flags,
                          GTK_MESSAGE_ERROR,
                          GTK_BUTTONS_OK_CANCEL,
                          _("The file :\n%s\n already exists !\nDo you really want to overwrite this PlanLibre file ?"),
                          path_to_file);
           ret = gtk_dialog_run (GTK_DIALOG(alertDlg));
           gtk_widget_destroy (GTK_WIDGET(alertDlg));
           if(ret==GTK_RESPONSE_CANCEL) {             
              g_free (path_to_file);
              return NULL;
           }
  }/* endif file exists */
  files_set_re_entrant_state (FALSE, data);
  return path_to_file;

}

/***************************************************
 PUBLIC Timer function - called evry ONE minute
 for "tomato timer"
***************************************************/
gboolean timeout_tomato_timer (APP_data *data)
{
  GtkWidget *progressbarTimer;
  GKeyFile *keyString;
  gint timer_run_value, timer_pause_value, ellapsed, paused;

  if(!tasks_get_tomato_timer_status (data))
       return TRUE;

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config"); 

  progressbarTimer = GTK_WIDGET(gtk_builder_get_object (data->builder, "progressbarTimer"));
  timer_run_value = g_key_file_get_integer (keyString, "application", "timer-run-time", NULL)*60;
  timer_pause_value = g_key_file_get_integer (keyString, "application", "timer-pause-time", NULL)*60;

  if(tasks_tomato_get_ellapsed_value (data) < (timer_run_value)) {  
     tasks_update_tomato_timer_value (TOMATO_BEAT, data);
     tasks_reset_tomato_paused_value (data);
     tasks_tomato_update_ellapsed_value (TOMATO_BEAT, data);
     ellapsed = tasks_tomato_get_ellapsed_value (data);
     gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progressbarTimer),(gdouble)ellapsed/timer_run_value);
     gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progressbarTimer), g_strdup_printf ("%.2d:%.2d",
                             ellapsed/60, ellapsed % 60 ));
  }
  else {/* manage auto-pauses and limits */
     if(tasks_tomato_get_display_status (data)) {
        tasks_update_tomato_timer_value_for_ID (tasks_get_tomato_timer_ID (data), data);
     }
     tasks_tomato_reset_display_status (data);
     tasks_tomato_update_paused_value (TOMATO_BEAT, data);
     paused = tasks_tomato_get_paused_value (data);
     gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progressbarTimer), g_strdup_printf (_("Rest time %.2d:%.2d"),
                             paused/60, paused % 60 ));
     gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progressbarTimer), (gdouble)paused/timer_pause_value);

     if(paused+TOMATO_BEAT>timer_pause_value) {
         tasks_reset_tomato_ellapsed_value (data);
     }
  }
  return TRUE;
}

/***************************************************
  intervalometer function - called every 5 minutes
  (e.g. 300 secs) to quick save
 must return TRUE to continue schedulling - of course, the 
 quick saving is controlled by a flag, but timout runs aniway
***************************************************************/
gboolean timeout_quick_save (APP_data *data)
{
  GKeyFile *keyString;
  gchar *path_to_file;
  static gint count = 0;

  /* re-entrant test */
  if(files_started_saving (data))
     return TRUE;

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config"); 
  count++;

  if(!g_key_file_get_boolean (keyString, "application", "interval-save", NULL)) {
      printf ("je quitte autosave car non actif avec count =%d \n", count);
      return TRUE;
  }


  cursor_busy (data->appWindow);
  if(data->fProjectHasName == TRUE) {
       path_to_file = g_key_file_get_string (keyString, "application", "current-file", NULL);
       save (path_to_file, data);
       printf ("* PlanLibre - sucessfully autosaved %s \n", path_to_file);
       g_free (path_to_file);
       cursor_normal (data->appWindow);
       return TRUE;
  }
  else {/* we must choose a new filename */
      path_to_file = (gchar *) get_a_filename (data, _("Noname.plibre"));
      if(path_to_file != NULL) {
        save (path_to_file, data);
        g_free (path_to_file);     
      }
  }

  cursor_normal (data->appWindow);
  return TRUE;
}


void
on_about1_activate (GtkMenuItem *menuitem, APP_data *data)
{
  GtkWidget *dialog = create_aboutPlanlibre (data);
  gtk_dialog_run (GTK_DIALOG(dialog));
  gtk_widget_destroy (GTK_WIDGET(dialog));
  return;
}

void on_wiki1_activate (GtkMenuItem  *menuitem, APP_data *data)
{
 gboolean ret;
 GError *error = NULL;
 GtkWidget *alertDlg;
 GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
 gint rep;

 // g_type_init();

  ret = g_app_info_launch_default_for_uri ("https://github.com/Luc-Ami/redac/wiki", NULL, &error);
  if(!ret) {
      alertDlg = gtk_message_dialog_new_with_markup (GTK_WINDOW(data->appWindow),
                          flags,
                          GTK_MESSAGE_ERROR,
                          GTK_BUTTONS_OK, NULL,
                          _("<big><b>Can't access to online wiki !</b></big>\nPlease, check your Internet connection\n and/or your desfault desktop file browser parameters."));
      rep = gtk_dialog_run (GTK_DIALOG(alertDlg));
      gtk_widget_destroy (GTK_WIDGET(alertDlg));
  }
}

void
on_keyHelp1_activate (GtkMenuItem *menuitem, APP_data *data)
{
  gint ret;
  GtkWidget *dialog;
  GtkBuilder *builder = NULL;
  builder = gtk_builder_new ();
  GError *err = NULL; 

  if(!gtk_builder_add_from_file (builder, find_ui_file ("shortkeys.ui"), &err)) {
     g_error_free (err);
     misc_halt_after_glade_failure (data);
  }
  /* Get the dialog from the glade file. */
  gtk_builder_connect_signals (builder, data);
  dialog = GTK_WIDGET(gtk_builder_get_object (builder, "dialogKeys"));
  data->tmpBuilder = builder;

  /* header bar */

  GtkWidget *hbar = gtk_header_bar_new ();
  gtk_widget_show (hbar);
  gtk_header_bar_set_title (GTK_HEADER_BAR (hbar), _("Keyboard shortcuts"));
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (hbar), _("List of keyboard shortcuts used by PLanLibre"));
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (hbar), FALSE);
  gtk_window_set_titlebar (GTK_WINDOW(dialog), hbar);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);  
  gtk_header_bar_pack_start (GTK_HEADER_BAR (hbar), box);
  GtkWidget *icon =  gtk_image_new_from_icon_name  ("accessories-character-map", GTK_ICON_SIZE_DIALOG);
  gtk_container_add (GTK_CONTAINER (box), icon);
  gtk_widget_show (box);
  gtk_widget_show (icon);

  gtk_window_set_transient_for (GTK_WINDOW(dialog),
                              GTK_WINDOW(data->appWindow));  

  ret = gtk_dialog_run (GTK_DIALOG(dialog));
  g_object_unref (data->tmpBuilder);
  gtk_widget_destroy (GTK_WIDGET(dialog)); 
  return;
}

/*****************************
  project's properties

*****************************/
void
on_file_properties_activate (GtkMenuItem *menuitem, APP_data *data)
{

  properties_modify (data);

}

/*****************************
  project's main calendar

*****************************/
void on_project_calendar_activate (GtkMenuItem *menuitem, APP_data *data)
{
  GtkWidget *dialog, *labelCalName, *labelProjStart, *labelProjEnd, *calendar1;
  GtkWidget *btnStart, *btnEnd;
  GtkTreeSelection *selection;
  GtkTreePath *path;
  gint ret, i, j, k;
  GDate current_date;
  GList *l, *l1 = NULL, *l2 = NULL;
  calendar *cal, *tmp;
  calendar_element *element, *elem2;
  undo_datas *tmp_value;  

  /* undo engine */
  tmp_value = g_malloc (sizeof(undo_datas));
  tmp_value->opCode = OP_MODIFY_CAL;
  tmp_value->datas = NULL;
  tmp_value->list = NULL;
  tmp_value->groups = NULL;

  for(i=0;i<g_list_length (data->calendars);i++) {
     l = g_list_nth (data->calendars, i);
     cal = (calendar *)l->data;
     tmp = g_malloc (sizeof(calendar));
     tmp->id = cal->id;
     tmp->list = NULL;
     tmp->name = g_strdup_printf ("%s", cal->name);   
     for(k=0;k<7;k++) {
	     for(j=0;j<4;j++) {
	        tmp->fDays[k][j] = cal->fDays[k][j];
	        tmp->schedules[k][j][0] = cal->schedules[k][j][0];
	        tmp->schedules[k][j][0] = cal->schedules[k][j][0];
	     }/* next j */
     }/* next k */

     for(k=0;k<g_list_length (cal->list);k++) {
        l2 = g_list_nth (cal->list, k);
        element = (calendar_element *) l2->data;
        elem2 = g_malloc (sizeof(calendar_element));
        elem2->day = element->day;
        elem2->month = element->month;
        elem2->year = element->year;
        elem2->type = element->type;
        tmp->list = g_list_append (tmp->list, elem2);
     }
    tmp_value->list = g_list_append (tmp_value->list, tmp);
  }/* next i */

  undo_push (CURRENT_STACK_MISC, tmp_value, data);

  g_date_set_time_t (&current_date, time (NULL)); /* date of today */
  /* non-modal window to manage calendars */
  dialog = calendars_dialog (data);

  /* select default calendar */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(data->treeViewCalendars));
  path = gtk_tree_path_new_from_indices (0, -1);
  gtk_tree_selection_select_path (selection, path);
  gtk_tree_path_free (path);

 // calendars_update_schedules (0, 0, data);/* mandatory AFTER dialog creation */
  labelCalName = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelCalName"));
  labelProjStart = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelProjStart"));
  labelProjEnd = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelProjEnd"));
  calendar1 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "calendar1"));
  btnStart = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonDateStart"));
  btnEnd = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonDateEnd"));
  
  gtk_button_set_label (GTK_BUTTON (btnStart), data->properties.start_date);
  gtk_button_set_label (GTK_BUTTON (btnEnd), data->properties.end_date);
  gtk_label_set_text (GTK_LABEL (labelCalName), _("Project main calendar"));
  gtk_label_set_text (GTK_LABEL (labelProjStart), data->properties.start_date);
  gtk_label_set_text (GTK_LABEL (labelProjEnd), data->properties.end_date);

  /* select current day */
  gtk_calendar_select_day (GTK_CALENDAR (calendar1), data->properties.start_nthDay);
  gtk_calendar_select_month (GTK_CALENDAR (calendar1), data->properties.start_nthMonth-1, 
                             data->properties.start_nthYear);/* YES, required nmonth -1 by Gtk API */

  ret = gtk_dialog_run (GTK_DIALOG (dialog));
  if(ret==1) {
     misc_display_app_status (TRUE, data);
  }
  else {
     undo_pop (data);/* remove useless undo datas */
  }
  gtk_widget_destroy (GTK_WIDGET (dialog));
  /* spell checker, if not ref-ed, is automatically destroyed with gtktextview */
  g_object_unref (data->tmpBuilder);
  /* we reset report display, since something has changed, so cost computation are wrong */
  report_clear_text (data->buffer, "left");
  report_default_text (data->buffer);
  report_set_buttons_menus (FALSE, FALSE, data);
}

/*****************************
  project's export agenda
*****************************/
void on_project_export_agenda (GtkMenuItem *menuitem, APP_data *data)
{
  GKeyFile *keyString;
  gchar *filename, *tmpFileName;
  gint ret;
  GtkFileFilter *filter = gtk_file_filter_new ();
  GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
  GtkWidget *alertDlg;

  gtk_file_filter_add_pattern (filter, "*.ics");
  gtk_file_filter_set_name (filter, _("Calendar/agenda files"));

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");

  GtkWidget *dialog = create_saveFileDialog (_("Export current project tasks agenda as a Icalendar compatible file"), data);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir());
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  /* run dialog for file selection */
  if(gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    if( !g_str_has_suffix (filename, ".ics" ) && !g_str_has_suffix (filename, ".ICS" )) {
         /* we correct the filename */
         tmpFileName = g_strdup_printf ("%s.ics", filename);
         g_free (filename);
         filename = tmpFileName;
    }/* endif extension XML*/
    /* we must check if a file with same name exists */
    if(g_file_test (filename, G_FILE_TEST_EXISTS) 
        && g_key_file_get_boolean (keyString, "application", "prompt-before-overwrite",NULL )) {
           /* dialog to avoid overwriting */
           alertDlg = gtk_message_dialog_new (GTK_WINDOW(dialog),
                          flags,
                          GTK_MESSAGE_ERROR,
                          GTK_BUTTONS_OK_CANCEL,
                          _("The file :\n%s\n already exists !\nDo you really want to overwrite this ICalendar (ICS) file ?"),
                          filename);
           if(gtk_dialog_run (GTK_DIALOG(alertDlg))==GTK_RESPONSE_CANCEL) {
              gtk_widget_destroy (GTK_WIDGET(alertDlg));
              gtk_widget_destroy (GTK_WIDGET(dialog));
              g_free (filename);
              return;
           }
    }/* endif file exists */
    /* now we export as an Icalendar (ICS)  file */
   // ret = save_project_to_icalendar (filename, data);
    ret = save_project_to_icalendar_for_rsc (filename, 1, data);
    g_free (filename);
  }
  gtk_widget_destroy (GTK_WIDGET(dialog)); 
}

/*****************************
  project's tasks

*****************************/
void
on_button_add_task_activate (GtkButton *button, APP_data *user_data)
{
  tasks_new (NULL, user_data);
}

void
on_button_add_group_activate (GtkButton *button, APP_data *user_data)
{
  tasks_group_new (user_data);
}

void on_button_add_milestone_activate (GtkButton *button, APP_data *data)
{
  tasks_milestone_new (NULL, data);
}

void on_button_modify_task_activate (GtkButton *button, APP_data *user_data)
{
  if(user_data->curTasks>=0)
     tasks_modify (user_data);
}

void on_button_repeat_task_activate (GtkButton *button, APP_data *data)
{
   tasks_repeat (data);
}

void on_button_shift_group_activate (GtkButton *button, APP_data *data)
{
  GtkWidget *dialog, *labelBtn;
  gchar *newDate = NULL;
  gint ret, cmp_date, cmpLim1, cmpLim2;
  GDate date_start, new_date, proj_start, proj_end, temp_date, temp_end_date;
  GtkAdjustment *vadj;
  tasks_data temp;

  tasks_get_ressource_datas (data->curTasks, &temp, data);
  dialog = timeline_create_timeshift_dialog (TRUE, data->curTasks, data);
  ret = gtk_dialog_run (GTK_DIALOG(dialog));
  /* update widgets */
  labelBtn = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelChangeDate"));
  newDate = g_strdup_printf ("%s", gtk_label_get_text (GTK_LABEL(labelBtn)));
  /* clean */
  gtk_widget_destroy (GTK_WIDGET(dialog));
  g_object_unref (data->tmpBuilder); 
  /* OK == 1 */
  if(ret==1) {
    /* project's bounding dates */
    g_date_set_dmy (&proj_start, data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
    g_date_set_dmy (&proj_end, data->properties.end_nthDay, data->properties.end_nthMonth, data->properties.end_nthYear);
    /* we get current dates limits from properties */
    g_date_set_dmy (&date_start, temp.start_nthDay, temp.start_nthMonth, temp.start_nthYear);
    g_date_set_parse (&new_date, newDate);
    /* how many days shift ? */
    cmp_date = g_date_days_between (&date_start, &new_date);
    if(cmp_date!=0) {
      /* test if new date+group duration is inside projet's bounding dates */ 
      temp_date = date_start;
      g_date_add_days (&temp_date, cmp_date);
      temp_end_date = temp_date;
      g_date_add_days (&temp_end_date, temp.days-1);
      printf ("demande shift Groupe de %d jours pour un projet d'une durée de %d j soit nouv date =%d %d %d nouv fin =%d %d %d\n", cmp_date, temp.days, g_date_get_day (&temp_date), g_date_get_month (&temp_date), g_date_get_year (&temp_date),
g_date_get_day (&temp_end_date), g_date_get_month (&temp_end_date), g_date_get_year (&temp_end_date)
 );
      if((g_date_compare (&temp_date, &proj_start)>=0)  && (g_date_compare (&proj_end, &temp_end_date)>=0)) {
          /* undo engine */
          undo_datas *tmp_value;  
          tmp_value = g_malloc (sizeof(undo_datas));
          tmp_value->id = temp.id; /* store group id */
          tmp_value->insertion_row = data->curTasks;
          tmp_value->opCode = OP_TIMESHIFT_GROUP;
          tmp_value->datas = NULL;
          tmp_value->list = NULL;
          tmp_value->groups = NULL;
          tmp_value->t_shift = cmp_date;
          undo_push (CURRENT_STACK_TASKS, tmp_value, data);

          dashboard_remove_all (FALSE, data);
          /* update 'due' status for tasks and dashboard */
          tasks_shift_dates (cmp_date, TRUE, temp.id, data); // TODO gérer les dépendances entre tâches
          /* redraw timeline */
          timeline_remove_all_tasks (data);
          vadj = gtk_scrolled_window_get_vadjustment 
                          (GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowTimeline")));
          gtk_adjustment_set_value (GTK_ADJUSTMENT(vadj), 0);
          timeline_draw_calendar_ruler (0, data);

          timeline_draw_all_tasks (data);
          /* redraw assignments */
          assign_remove_all_tasks (data);
          assign_draw_all_visuals (data);
          /* redraw dashboard - useless, done during call to tasks_shift_dates () */
          /* clear report */
          report_clear_text (data->buffer, "left");
          report_default_text (data->buffer);
          report_set_buttons_menus (FALSE, FALSE, data);
          misc_display_app_status (TRUE, data);
      }/* if date OK */
      else {
        gchar *msg = g_strdup_printf ("%s", _("Sorry !\nCan't do that.\nRequested date after shift (included tasks duration) is\noutside of project's bounding dates.\nYou can try again, after updating project's properties."));
         misc_ErrorDialog (data->appWindow, msg);
        g_free (msg);
      }
    }/* cmp date != 0*/
  }/* endif ret = 1 */
  if(newDate)
     g_free (newDate);
}

void on_button_append_task_activate (GtkButton *button, APP_data *data)
{
  import_tasks_from_csv (data);
}

void on_menu_append_task_activate (GtkMenuItem *menuitem, APP_data *data)
{
  /* switch to report notebook page */
  GtkWidget *notebook = GTK_WIDGET(gtk_builder_get_object (data->builder, "notebook1"));
  gtk_notebook_set_current_page (GTK_NOTEBOOK(notebook), 2);
  import_tasks_from_csv (data);
}

void on_menu_export_task_activate (GtkMenuItem *menuitem, APP_data *data)
{
  save_tasks_CSV (data);
}

/*********************************
  PUBLIC : response to timer
  button clicks start/stop
*********************************/
void
on_button_tomato_timer_task_activate (GtkButton *button, APP_data *data)
{
  GtkWidget *boxProgressTimer, *progressbarTimer;

  boxProgressTimer = GTK_WIDGET(gtk_builder_get_object (data->builder, "boxProgressTimer"));
  progressbarTimer = GTK_WIDGET(gtk_builder_get_object (data->builder, "progressbarTimer"));
  misc_display_app_status (TRUE, data);
  if(tasks_get_tomato_timer_status (data)) {
     /* stop interval timer */
     gtk_widget_hide (GTK_WIDGET(boxProgressTimer));
     tasks_set_tomato_timer_status (FALSE, data);
     /* update timer_val field */
     tasks_update_tomato_timer_value_for_ID (tasks_get_tomato_timer_ID (data), data);
     tasks_store_tomato_timer_ID (-1, data);/* reset */
     tasks_reset_tomato_ellapsed_value (data);
     tasks_reset_tomato_paused_value (data);

  }
  else {
     gtk_widget_show (GTK_WIDGET(boxProgressTimer));
     gint id = tasks_get_id (task_get_selected_row (data), data);
     tasks_reset_tomato_ellapsed_value (data);
     tasks_store_tomato_timer_ID (id, data);
     tasks_reset_tomato_timer_value (data);
     tasks_set_tomato_timer_status (TRUE, data);
     gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progressbarTimer), 0.0);
     gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progressbarTimer), "00:00");
     /*  start interval timer */
  }
}

/*****************************
  project's ressources CB

*****************************/
void on_button_add_ressources_activate (GtkButton  *button, APP_data *user_data)
{
   rsc_new (user_data);
}

void on_button_modify_ressources_activate (GtkButton  *button, APP_data *user_data)
{
  rsc_modify (user_data);
}

void on_button_clone_ressources_activate (GtkButton *button, APP_data *data)
{
  rsc_clone (2, data);
}

void on_button_multiply_ressources_activate (GtkButton *button, APP_data *data)
{
  GtkWidget *spinMpy;
  gint times, i;
  /* we read some values */
  spinMpy = GTK_WIDGET(gtk_builder_get_object (data->builder, "spinMpy"));
  times = (gint) gtk_spin_button_get_value(GTK_SPIN_BUTTON(spinMpy));
  for(i=0;i<times;i++) {
     rsc_clone (i+2, data);
  }
}


/*********************************
   ressources charge
*********************************/
void on_ressources_charge_activate (GtkMenuItem  *menuitem, APP_data *data)
{
  report_all_rsc_charge (data);
}

void on_ressources_charge_clicked (GtkButton  *button, APP_data *data)
{
  report_all_rsc_charge (data);
}
/**********************************
  ressources cost

**********************************/
void on_ressources_cost_activate (GtkMenuItem  *menuitem, APP_data *data)
{
 report_all_rsc_cost (data);
}

void on_ressources_cost_clicked (GtkButton  *button, APP_data *data)
{
 report_all_rsc_cost (data);
}

/**********************************
  export current report cost
**********************************/
void on_button_export_report_clicked (GtkButton *button, APP_data *data)
{
   report_export_report_to_rtf (data);
}

void on_menu_export_report_activate (GtkMenuItem  *menuitem, APP_data *data)
{
   report_export_report_to_rtf (data);
}

/********************************
  export current report as
  e-mail
********************************/
void on_button_mail_report_clicked (GtkButton *button, APP_data *data)
{
 gint ret = collaborate_send_report_or_calendar (MAIL_TYPE_REPORT, data);
}

/**************************
  quit application

**************************/
static void quit_application (APP_data *data)
{
  GtkWidget *dialog;
  gint ret;
  GKeyFile *keyString;
  gchar *path_to_file = NULL, *summary;
  gboolean flag, filenameStored = FALSE;
  GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;


  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");

  /* remove snapshot if exists */
  if(data->fProjectHasSnapshot) {
     printf ("* PlanLibre : there is a snapshot to remove *\n");
     if(data->path_to_snapshot) {
       ret = g_remove (data->path_to_snapshot);
       g_free (data->path_to_snapshot);
     }
  }
  /* alert dialog before saving */
  if(data->fProjectModified) {  
     flag = misc_alert_before_clearing (data);/* returns TRUE if user wants to save datas */
     if(flag) {
       if(data->fProjectHasName == TRUE) {
             path_to_file = g_key_file_get_string (keyString, "application", "current-file", NULL);
             save (path_to_file, data);
             g_free (path_to_file);
             path_to_file = NULL;
       }
       else {/* we must choose a new filename */
          path_to_file = get_a_filename (data, _("Noname.plibre"));
          /* path_to_file == NULL  then nothing to do ! */
	  if(path_to_file != NULL) {
		  /* now, we save file & update config.ini */
		  save (path_to_file, data);
		  g_key_file_set_string (keyString, "application", "current-file", path_to_file);
		  g_key_file_set_string (keyString, "application", "current-file-basename", 
		              g_filename_display_basename (path_to_file));
		  g_free (path_to_file);
		  /* rearrange list of recent files */
		  filenameStored = TRUE;
		  rearrange_recent_file_list (keyString);// pb
	  }/* path not null */
       }/* no name */
     }/* if flag */
  }/* modified */

  if(g_key_file_get_boolean (keyString, "application", "prompt-before-quit", NULL )) {
      dialog = gtk_message_dialog_new (GTK_WINDOW(data->appWindow), flags,
                    GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL,
                _("Do you really want to quit this program ?"));
      ret = gtk_dialog_run (GTK_DIALOG(dialog));
      gtk_widget_destroy (GTK_WIDGET(dialog));
      if(ret!=GTK_RESPONSE_OK)
         return;/* we finally want to stay in PlanLibre */
  }

 /* if file is empty, we don't have to store anything in config file */
 if((filenameStored)  || (data->fProjectHasName)) {
    path_to_file = g_key_file_get_string (keyString, "application", "current-file", NULL);
    /* we change the default values for gkeyfile */
    summary = files_get_project_summary (data);
    store_current_file_in_keyfile (keyString, path_to_file, summary);
    g_free (summary);
    g_free (path_to_file);
  }
  storage_save (data->gConfigFile, data);
  destroyGKeyFile (data, data->appWindow);

  /* clear dashboard */
  dashboard_remove_all (FALSE, data);
printf("quitter : phase dashboard OK \n");
  /* clear "links" list */
  links_free_all (data);
printf("quitter : phase links OK \n");
  /* clear all tasks datas */
  tasks_remove_all_rows (data);
  tasks_free_all (data);/* goocanvasitem are removed here */
printf("quitter : phase tasksOK \n");
  /* clear all ressource datas */
  rsc_remove_all_rows (data);
  rsc_free_all (data);
printf("quitter : phase ressources OK \n");
  /* clear project's properties */
  properties_free_all (data);
  properties_set_default_dates (data);
  /* clear assignments */
  assign_remove_all_tasks (data);
  cursor_free ();
  /* calendars */
  calendars_free_calendars (data);
printf("quitter phase calendars OK \n");
  /* undo engine */
  undo_free_all (data);
    // g_object_unref(data->spell); TODO segfault
  g_application_quit (G_APPLICATION(data->app));

}

/*****************************
  new project

*****************************/
void
on_file_new_activate (GtkMenuItem *menuitem, APP_data *data)
{
  gint day =0, month, year = 0;
  GDate *date;
  GtkAdjustment *vadj, *hadj;
  gchar *path_to_file = NULL, *title, *tmpFileName;
  GKeyFile *keyString;
  GtkWidget *menuItem;
  
  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");

  menuItem = GTK_WIDGET(gtk_builder_get_object (data->builder, "menuRestoreSnapshot"));
  gtk_widget_set_sensitive (GTK_WIDGET (menuItem), FALSE);

  /* remove snapshot if exists */
  if(data->fProjectHasSnapshot) {
     printf ("* PlanLibre : there is already a snapshot when you request a new project *\n");
     if(data->path_to_snapshot) {
         gtk_widget_set_sensitive (GTK_WIDGET (menuItem), TRUE);
     }
  }
  else
     data->fProjectHasSnapshot = FALSE;

  /* alert dialog before clearing datas  */
  if(data->fProjectModified) {  
     gboolean flag = misc_alert_before_clearing (data);/* returns TRUE if user wants to save datas */
     if(flag) {
       if(data->fProjectHasName == TRUE) {
             path_to_file = g_key_file_get_string (keyString, "application", "current-file", NULL);
             save (path_to_file, data);
             g_free (path_to_file);
       }
       else {/* we must choose a new filename */
          path_to_file = get_a_filename (data, _("Noname.plibre"));
	  /* path_to_file == NULL  the nothing to do ! */
	  if(path_to_file != NULL) {
		  /* now, we save file & update config.ini */
		  save (path_to_file , data);
		  g_key_file_set_string (keyString, "application", "current-file", path_to_file);
		  g_key_file_set_string (keyString, "application", "current-file-basename", 
		              g_filename_display_basename (path_to_file));
		  g_free (path_to_file);
		  /* rearrange list of recent files */
		  rearrange_recent_file_list (keyString);
	  }/* path not null */
       }/* no name */
     }/* if flag */
  }/* modified */


  /* clear and reset calendars */
  calendars_free_calendars (data);
  calendars_init_main_calendar (data);
  /* clear dashboard */
  dashboard_remove_all (TRUE, data);
  /* clear "links" list */
  links_free_all (data);
  /* clear all tasks datas */
  tasks_remove_all_rows (data);
  tasks_free_all (data); /* goocanvasitem are removed here */
  /* clear all ressource datas */
  rsc_remove_all_rows (data);
  rsc_free_all (data);
  /* clear project's properties */
  properties_free_all (data);
  properties_set_default_dates (data);
  properties_set_default_schedules (data);
  properties_set_default_dashboard (data);
  /* clear assignments */
  assign_remove_all_tasks (data);
  /* clear report area */
  report_clear_text (data->buffer, "left");
  report_default_text (data->buffer);
  report_set_buttons_menus (FALSE, FALSE, data);
  /* clear undo datas */
  undo_free_all (data);
  /* reset date for auto-mailing facility */
  g_key_file_set_integer (keyString, "collaborate", "last_auto_mailing-day", -1);
  g_key_file_set_integer (keyString, "collaborate", "last_auto_mailing-month", -1);
  g_key_file_set_integer (keyString, "collaborate", "last_auto_mailing-year", -1);
  /* home page */
  home_today_date (data);
  home_project_infos (data);
  home_project_tasks (data);
  home_project_milestones (data);
  /* init dashboard */
  dashboard_init (data);

  /* update assignments */

 // vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowAsst")));
  //hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowAsst")));
 // gtk_adjustment_set_value (GTK_ADJUSTMENT(vadj), 0);
//  gtk_adjustment_set_value (GTK_ADJUSTMENT(hadj), 0);
  date = g_date_new_dmy (data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
  month = g_date_get_month (date);
  year = g_date_get_year (date);
  assign_store_date (1, month, year);

//  assign_draw_calendar_ruler (0, -1, month, year, data);
//  assign_draw_rsc_ruler (0, data);
//  assign_draw_all_tasks (data);
  assign_draw_all_visuals (data);
  /* update timeline */
  timeline_reset_store_date ();/* to 'force' to use projet's dates */
  timeline_store_date (data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
  vadj = gtk_scrolled_window_get_vadjustment 
               (GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowTimeline")));

  gtk_adjustment_set_value (GTK_ADJUSTMENT(vadj), 0);
  timeline_hide_cursor (data);
  timeline_remove_ruler (data);
  timeline_draw_calendar_ruler (0, data);
  timeline_draw_all_tasks (data);
  /* application */
  title = g_strdup_printf ("PlanLibre - %s", _("Noname"));
  gtk_window_set_title (GTK_WINDOW(data->appWindow), title);
  g_free (title);
  data->fProjectHasName = FALSE;
  data->fProjectModified = FALSE;
  data->fProjectHasSnapshot = FALSE;
  misc_display_app_status (FALSE, data);
}

/*****************************
  open project
*****************************/
void on_file_open_activate (GtkMenuItem *menuitem, APP_data *data)
{
  GtkWidget *menuItem;
  GKeyFile *keyString;
  gchar *filename, *path_to_file, *title, *summary;
  gint rc = 0, rc2 = 0;
  GtkFileFilter *filter = gtk_file_filter_new ();
  gtk_file_filter_add_pattern (filter, "*.plibre");
  gtk_file_filter_set_name (filter, _("PlanLibre files"));

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");
  /* alert dialog : add saving */
  menuItem = GTK_WIDGET(gtk_builder_get_object (data->builder, "menuRestoreSnapshot"));
  gtk_widget_set_sensitive (GTK_WIDGET (menuItem), FALSE);

  if(data->fProjectModified) {  
     gboolean flag = misc_alert_before_clearing (data);/* returns TRUE if user wants to save datas */
     if(flag) {
       if(data->fProjectHasName == TRUE) {
             path_to_file = g_key_file_get_string (keyString, "application", "current-file", NULL);
             save (path_to_file, data);
             g_free (path_to_file);
       }
       else {/* we must choose a new filename */
          path_to_file = get_a_filename (data, _("Noname.plibre"));
          /* path_to_file == NULL  the nothing to do ! */
	  if(path_to_file != NULL) {
		  /* now, we save file & update config.ini */
		  save (path_to_file, data);
		  g_key_file_set_string (keyString, "application", "current-file", path_to_file);
		  g_key_file_set_string (keyString, "application", "current-file-basename", 
		              g_filename_display_basename (path_to_file));
		  g_free (path_to_file);
	  }/* path not null */
       }/* no name */
     }/* if flag */
  }/* modified */
  /* all datas are cleared by open function */

  GtkWidget *dialog = create_loadFileDialog (data->appWindow, data, _("Open PlanLibre project ..."));
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir());
  /* we must extract a short filename */
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  /* run dialog */
  if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    gtk_widget_destroy (GTK_WIDGET(dialog)); 
    timeline_reset_store_date ();/* to 'force' to use projet's dates */
    rc = open_file (filename, data);
    /* we take a snapshot */
    rc2 = files_copy_snapshot (filename, data);
    if(rc==0) {   
       timeline_store_date (data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
       /* rearrange list of recent files */
       rearrange_recent_file_list (keyString);
       /* we change the default values for gkeyfile + summary */
       summary = files_get_project_summary (data);
       store_current_file_in_keyfile (keyString, filename, summary);
       g_free (summary);
       title = g_strdup_printf ("PlanLibre - %s", filename);
       gtk_window_set_title (GTK_WINDOW(data->appWindow), title);
       g_free (title);
       /* undo engine */
       undo_free_all (data);
    }
    else {/* display an error message */
        gchar *msg = g_strdup_printf ("%s", _("Sorry !\nAn error happened during file loading.\nOperation aborted. You can try again, or\ncheck your system."));
        misc_ErrorDialog (data->appWindow, msg );
        g_free (msg);
    }
    g_free (filename);
  }
  else
     gtk_widget_destroy (GTK_WIDGET(dialog)); 
 
}

/*****************************
  open recent
*****************************/
void on_file_recent_activate (GtkMenuItem *menuitem, APP_data *data)
{
  GKeyFile *keyString;
  gchar *path_to_file = NULL, *title = NULL, *summary = NULL, *msg;
  GtkWidget *currentFileDialog, *menuItem;
  gint ret, rc, rc2, file_to_load;

  GtkWidget *dialog;
  gint res;
  gchar *fileChoosen;
  GError *err = NULL;


  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");

  menuItem = GTK_WIDGET(gtk_builder_get_object (data->builder, "menuRestoreSnapshot"));
  gtk_widget_set_sensitive (GTK_WIDGET (menuItem), FALSE);

  /* alert before clearing datas */
  if(data->fProjectModified) {  
     gboolean flag = misc_alert_before_clearing (data);/* returns TRUE if user wants to save datas */
     if(flag) {
       if(data->fProjectHasName == TRUE) {
             path_to_file = g_key_file_get_string (keyString, "application", "current-file", NULL);
             save (path_to_file, data);
             g_free (path_to_file);
       }
       else {/* we must choose a new filename */
          path_to_file = get_a_filename (data, _("Noname.plibre"));
          /* path_to_file == NULL  the nothing to do ! */
	  if(path_to_file != NULL) {
		  /* now, we save file & update config.ini */
		  save(path_to_file , data);
		  g_key_file_set_string (keyString, "application", "current-file", path_to_file);
		  g_key_file_set_string (keyString, "application", "current-file-basename", 
		              g_filename_display_basename (path_to_file));
		  g_free (path_to_file);
	  }/* path not null */
       }/* no name */
     }/* if flag */
  }/* modified */
  /* all datas are cleared by open function */

  /* paving dialog from redac :) */
  GtkRecentFilter *filter = gtk_recent_filter_new ();
  gtk_recent_filter_add_pattern (filter, "*.plibre");
  gtk_recent_filter_set_name (filter, _("PlanLibre files"));

  dialog = gtk_recent_chooser_dialog_new (NULL,
                                        GTK_WINDOW(data->appWindow),
                                        _("_Cancel"),
                                        GTK_RESPONSE_CANCEL,
                                        _("_Open"),
                                        GTK_RESPONSE_ACCEPT,
                                        NULL);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 505, 305);
  gtk_recent_chooser_add_filter (GTK_RECENT_CHOOSER (dialog), filter);
  gtk_recent_chooser_set_sort_type (GTK_RECENT_CHOOSER (dialog), GTK_RECENT_SORT_MRU);
  gtk_recent_chooser_set_show_tips (GTK_RECENT_CHOOSER (dialog), TRUE);

 /* header bar */
  GtkWidget *hbar = gtk_header_bar_new ();
  gtk_widget_show (hbar);
  gtk_header_bar_set_title (GTK_HEADER_BAR (hbar), _("Recent PlanLibre projects"));
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (hbar), _("Open a previous project."));
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (hbar), FALSE);
  gtk_window_set_titlebar (GTK_WINDOW(dialog), hbar);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  
  gtk_header_bar_pack_start (GTK_HEADER_BAR (hbar), box);

  GtkWidget *icon =  gtk_image_new_from_icon_name  ("document-open-recent", GTK_ICON_SIZE_DIALOG);
  gtk_container_add (GTK_CONTAINER (box), icon);
  gtk_widget_show (box);
  gtk_widget_show (icon);


  res = gtk_dialog_run (GTK_DIALOG (dialog));
  if(res == GTK_RESPONSE_ACCEPT) {
     GtkRecentInfo *info;
     GtkRecentChooser *chooser = GTK_RECENT_CHOOSER (dialog);

     info = gtk_recent_chooser_get_current_item (chooser);
     fileChoosen = g_filename_from_uri (gtk_recent_info_get_uri (info), NULL, &err);
     gtk_recent_info_unref (info);
     /* we read the file path within le config.ini file */
     path_to_file = g_strdup_printf ("%s", fileChoosen);
     g_free (fileChoosen);
     gtk_widget_destroy (dialog);
     /* we check if the file exists */
     if(g_file_test (path_to_file, G_FILE_TEST_EXISTS)) {
       /* now we open the file */
       timeline_reset_store_date ();/* to 'force' to use projet's dates */
       rc = open_file (path_to_file, data);
       /* we take a snapshot */
       rc2 = files_copy_snapshot (path_to_file, data);
       if(rc==0) {
         timeline_store_date (data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
         /* rearrange list of recent files */
         if(file_to_load>0)/* don't rearrange when file of rank #1 is loaded ! */
            rearrange_recent_file_list (keyString);
         /* we change the default values for gkeyfile + summary */
         summary = files_get_project_summary (data);
         store_current_file_in_keyfile (keyString, path_to_file, summary);
         g_free (summary);
         title = g_strdup_printf ("PlanLibre - %s", path_to_file);
         gtk_window_set_title (GTK_WINDOW(data->appWindow), title);
         g_free (title);
         /* undo engine */
         undo_free_all (data);
// TODO snapshot
      }
      else {/* display an error message */
        msg = g_strdup_printf ("%s", _("Sorry !\nAn error happened during file loading.\nOperation aborted. You can try again, or\ncheck your system."));
        misc_ErrorDialog (data->appWindow, msg);
        g_free (msg);
      }
    }/* endif exists */
    /* display a message when we can't access a file, for example if removed */
    else {
        msg = g_strdup_printf ("%s", _("Sorry !\nI can't retrieve the file !\nOperation aborted. You can try again, or\ncheck your system."));
        misc_ErrorDialog (data->appWindow, msg);
        g_free (msg);
    }
    g_free (path_to_file); 
  }/* endif accept */
  else
          gtk_widget_destroy (dialog);
}

/*****************************
  save project
*****************************/
void
on_file_save_activate (GtkMenuItem *menuitem, APP_data *data)
{
  GKeyFile *keyString;
  gchar *path_to_file = NULL;

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");
  /* alert dialog : add saving */
  if(data->fProjectHasName == TRUE) {
             path_to_file = g_key_file_get_string (keyString, "application", "current-file", NULL);
             save (path_to_file, data);
             
             /* temporarire !!! */
             save_to_project (g_strdup_printf ("%s.xml", path_to_file), data);
             
             g_free (path_to_file);
             data->fProjectModified = FALSE;
             return;
  }
  else {/* we must choose a new filename */
          path_to_file = get_a_filename (data, _("Noname.plibre"));
  }/* no name */
  /* path_to_file == NULL  the nothing to do ! */
  data->fProjectModified = TRUE;
  if(path_to_file != NULL) {
          /* now, we save file & update config.ini */
          save (path_to_file, data);
          g_key_file_set_string (keyString, "application", "current-file", path_to_file);
          g_key_file_set_string (keyString, "application", "current-file-basename", 
                                           g_filename_display_basename (path_to_file));
          /* rearrange list of recent files */
          rearrange_recent_file_list (keyString);
          store_current_file_in_keyfile (keyString, path_to_file, files_get_project_summary (data));
          /* change window's title */
          gchar *title = g_strdup_printf ("PlanLibre - %s", path_to_file);
          gtk_window_set_title (GTK_WINDOW(data->appWindow), title);
          g_free (title);
          g_free (path_to_file);
  }/* path not null */

}

/*****************************
  save as project
*****************************/
void on_file_save_as_activate (GtkMenuItem *menuitem, APP_data *data)
{
  GKeyFile *keyString;
  GtkWidget *alertDlg;
  GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
  gchar *filename, *newFilename, *tmpFileName;
  gint ret;
  GtkFileFilter *filter = gtk_file_filter_new ();

  gtk_file_filter_add_pattern (filter, "*.plibre");
  gtk_file_filter_set_name (filter, _("PlanLibre xml files"));

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");
  filename = g_key_file_get_string (keyString, "application", "current-file-basename", NULL);
  GtkWidget *dialog = create_saveFileDialog (_("Save current project as ..."), data);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), 
                 g_path_get_dirname (g_key_file_get_string(keyString, "application", "current-file", NULL) ));
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  /* we must extract a short filename */
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), g_strdup_printf ("%s", filename));
  /* run dialog for file selection */

  if(gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    newFilename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    /* we check if filename has .plibre extension */
    if( !g_str_has_suffix (newFilename, ".PLIBRE" ) && !g_str_has_suffix (newFilename, ".plibre" )) {
              /* we correct the filename */
              tmpFileName = g_strdup_printf ("%s.plibre", newFilename);
              g_free (newFilename);
              newFilename = tmpFileName;
    }/* endif extension XML*/
    /* we must check if a file with same name exists */
    if(g_file_test (newFilename, G_FILE_TEST_EXISTS) 
       && g_key_file_get_boolean (keyString, "application", "prompt-before-overwrite",NULL )) {
           /* dialog to avoid overwriting */
           alertDlg = gtk_message_dialog_new (GTK_WINDOW(dialog),
                          flags, GTK_MESSAGE_ERROR,
                          GTK_BUTTONS_OK_CANCEL,
                          _("The file :\n%s already exists !\nDo you really want to Overwrite an existing PlanLibre file ?"),
                          newFilename);
           if(gtk_dialog_run (GTK_DIALOG(alertDlg))==GTK_RESPONSE_CANCEL) {
              gtk_widget_destroy (GTK_WIDGET(alertDlg));
              gtk_widget_destroy (GTK_WIDGET(dialog));
              g_free (filename);
              g_free (newFilename);
              return;
           }
    }/* endif file exists */
    /* now, we save file & update config.ini */
    save (newFilename , data);
    g_key_file_set_string (keyString, "application", "current-file", newFilename);
    g_key_file_set_string (keyString, "application", "current-file-basename", 
                      g_filename_display_basename (newFilename));
    /* rearrange list of recent files */
    rearrange_recent_file_list (keyString);
    /* take a snapshot AFTER saving */
    ret = files_copy_snapshot (newFilename, data);
    /* change window's title */
    gchar *title = g_strdup_printf ("PlanLibre - %s", newFilename);
    gtk_window_set_title (GTK_WINDOW(data->appWindow), title);
    g_free (title);
    g_free (newFilename);

  }/* endif OK*/

  g_free (filename);
  gtk_widget_destroy (GTK_WIDGET(dialog)); 

}

/****************************
 reload previous version of
 project
****************************/
void on_edit_reload_activate (GtkMenuItem *menuitem, APP_data *data)
{
  printf ("reload previous file \n");

  GKeyFile *keyString;
  gchar *path_to_file, *msg, *title = NULL, *summary = NULL;
  gint rc;
  GtkWidget *menuItem;

  if(data->fProjectModified) {  
     gboolean flag = misc_alert_before_clearing (data);/* returns TRUE if user wants to save datas */
     if(flag) {
       if(data->fProjectHasName == TRUE) {
             path_to_file = g_key_file_get_string (keyString, "application", "current-file", NULL);
             save (path_to_file, data);
             g_free (path_to_file);
       }
       else {/* we must choose a new filename */
          path_to_file = get_a_filename (data, _("Noname.plibre"));
          /* path_to_file == NULL  the nothing to do ! */
	  if(path_to_file != NULL) {
		  /* now, we save file & update config.ini */
		  save (path_to_file, data);
		  g_key_file_set_string (keyString, "application", "current-file", path_to_file);
		  g_key_file_set_string (keyString, "application", "current-file-basename", 
		              g_filename_display_basename (path_to_file));
		  g_free (path_to_file);
	  }/* path not null */
       }/* no name */
     }/* if flag */
  }/* modified */
  /* all datas are cleared by open function */

  /* now we continue as a standard reload - please not that the snapshot remains as it */
  timeline_reset_store_date ();/* to 'force' to use projet's dates */
  rc = open_file (data->path_to_snapshot, data);
       if(rc==0) {
         /* we don't take a snapshot */
         menuItem = GTK_WIDGET(gtk_builder_get_object (data->builder, "menuRestoreSnapshot"));
         gtk_widget_set_sensitive (GTK_WIDGET (menuItem), FALSE);
         timeline_store_date (data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
         /* NO rearrange list of recent files */
         /* we change the default values for gkeyfile + summary */
         summary = files_get_project_summary (data);
         // store_current_file_in_keyfile (keyString, data->path_to_snapshot, summary);// TODO remove ???
         g_free (summary);
         title = g_strdup_printf ("PlanLibre - %s", data->path_to_snapshot);
         gtk_window_set_title (GTK_WINDOW(data->appWindow), title);
         g_free (title);
         /* undo engine */
         undo_free_all (data);
         /* force to set a new name in case of saving */
         data->fProjectModified = FALSE;
         data->fProjectHasName = TRUE;
         misc_InfoDialog (data->appWindow, _("Previous version of your project reloaded"));
      }
      else {/* display an error message */
        msg = g_strdup_printf ("%s", _("Sorry !\nAn error happened during file restoration.\nOperation aborted."));
        misc_ErrorDialog (data->appWindow, msg);
        g_free (msg);
      }
}

/****************************
  notebook  page change
****************************/
void on_page_switched (GtkNotebook *notebook, GtkWidget *page,
                       guint page_num,  APP_data *data)
{
  GtkListBox *box;
  GtkWidget *btnModif, *btnRepeat, *lblLine, *btnRem;

  lblLine = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelAppLine"));
  gtk_label_set_text (GTK_LABEL(lblLine), _("L--"));
  /* the current page is in page_num, first page index is 0 */
  switch(page_num) {
    case 0: {
      data->currentStack = CURRENT_STACK_HOME;
      misc_display_tips_status (_("This page displays project information summary"), data);
      home_project_infos (data);
      home_project_tasks (data);
      break;
    }
    case 1: {
      data->currentStack = CURRENT_STACK_TIMELINE;
      misc_display_tips_status (_("Right-click on a task for various options, or left-click and drag and drop to move a task along time"), data);
      break;
    }
    case 2: {
      data->currentStack = CURRENT_STACK_TASKS;
      box = GTK_LIST_BOX(gtk_builder_get_object (data->builder, "listboxTasks"));  
      btnModif = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonTaskModify"));  
      btnRem = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonRemove"));
      btnRepeat = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonTaskRepeat"));
      if(gtk_list_box_get_selected_row (box)) {
          gtk_widget_set_sensitive (GTK_WIDGET(btnModif), TRUE);
          gtk_widget_set_sensitive (GTK_WIDGET(btnRem), TRUE);
          if(tasks_get_type (data->curTasks, data)==TASKS_TYPE_TASK)
              gtk_widget_set_sensitive (GTK_WIDGET(btnRepeat), TRUE);
          else
               gtk_widget_set_sensitive (GTK_WIDGET(btnRepeat), FALSE);
      }
      else {
          gtk_widget_set_sensitive (GTK_WIDGET(btnModif), FALSE);
          gtk_widget_set_sensitive (GTK_WIDGET(btnRem), FALSE);
          gtk_widget_set_sensitive (GTK_WIDGET(btnRepeat), FALSE);
      }
      misc_display_tips_status (_("click on a task to select it"), data);
      break;
    }
    case 3: {
      data->currentStack = CURRENT_STACK_RESSOURCES;
      box = GTK_LIST_BOX(gtk_builder_get_object (data->builder, "listboxRsc"));  
      btnModif = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonRscModify"));  
      if( gtk_list_box_get_selected_row (box)) {
          gtk_widget_set_sensitive (GTK_WIDGET(btnModif), TRUE);
      }
      else {
          gtk_widget_set_sensitive (GTK_WIDGET(btnModif), FALSE);
      }
      misc_display_tips_status (_("click on a resource to select it "), data);
      break;
    }
    case 4: {
      data->currentStack = CURRENT_STACK_DASHBOARD;
      misc_display_tips_status (_("Clic on a task, and drag'n'drop it to change its status"), data);
      break;
    }
    case 5: {
      data->currentStack = CURRENT_STACK_ASSIGN;
      misc_display_tips_status (_("Double-click on a resource to modify it, right-click for other options "), data);
      break;
    }
    case 6: {
      data->currentStack = CURRENT_STACK_REPORT;
      misc_display_tips_status (_("This page displays last report. You can export it, see top buttons."), data);
      break;
    }
  }/* end switch */
  /* we de-activate some widgets, since current page is changed, so previous selections are false */
}

/********************************
  row activated in Ressources
  listbox 
********************************/
void on_ressources_row_selected (GtkListBox *box, GtkListBoxRow *row, APP_data *data)
{
  GtkWidget *btnModif, *btnClone, *lblLine, *frameMpy;

  lblLine = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelAppLine"));
  btnModif = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonRscModify"));
  btnClone = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonRscClone"));
  frameMpy = GTK_WIDGET(gtk_builder_get_object (data->builder, "frameMpy"));
  /* beware : if this event is called after row deletion, row isn't valid ! */
  if(row) {
      data->curRsc = gtk_list_box_row_get_index (row);
      gchar *tmpStr = g_strdup_printf (_("L%d/%d"), data->curRsc+1, g_list_length(data->rscList));
      data->curRsc = gtk_list_box_row_get_index (row);
      gtk_label_set_text (GTK_LABEL(lblLine), tmpStr);
      gtk_widget_set_sensitive (GTK_WIDGET(btnModif), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET(btnClone), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET(frameMpy), TRUE);
      g_free (tmpStr);
  }
  else {
      data->curRsc = -1;
      gtk_label_set_text (GTK_LABEL(lblLine), _("L--"));
      gtk_widget_set_sensitive (GTK_WIDGET(btnModif), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(btnClone), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(frameMpy), FALSE);
  }
}


void on_btn_search_prev_clicked (GtkButton *button, APP_data *data)
{
 const gchar *tmpStr;
 GtkWidget *btnNext, *btnPrev, *entry, *labelHits;
 gint hits = 0;

 btnNext = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonSearchNext"));
 btnPrev = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonSearchPrev"));
 entry = GTK_WIDGET(gtk_builder_get_object (data->builder, "searchentryTasks")); 
 labelHits = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelHits"));
 tmpStr= gtk_entry_get_text (GTK_ENTRY(entry));
 if(tmpStr) {
        hits = search_prev_hit (tmpStr, data);
        if(hits>=0) {
           gtk_widget_set_sensitive (GTK_WIDGET (btnNext), TRUE);
           gtk_widget_set_sensitive (GTK_WIDGET (btnPrev), TRUE);
           /* we select the convenient line in tasks listbox */
           search_select_task_row (hits, data);
           gtk_label_set_text (GTK_LABEL(labelHits), _("yes"));
           tasks_autoscroll ((gdouble)task_get_selected_row (data)/g_list_length(data->tasksList),
                             data);
        }
        else {
           gtk_widget_set_sensitive (GTK_WIDGET (btnNext), TRUE);
           gtk_widget_set_sensitive (GTK_WIDGET (btnPrev), FALSE);
           gtk_label_set_text (GTK_LABEL(labelHits), _("none before"));
        }
 }/* if tmpStr */

}

void on_btn_search_next_clicked (GtkButton *button, APP_data *data)
{
 const gchar *tmpStr;
 GtkWidget *btnNext, *btnPrev, *entry, *labelHits;
 gint hits = 0;

 btnNext = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonSearchNext"));
 btnPrev = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonSearchPrev"));
 entry = GTK_WIDGET(gtk_builder_get_object (data->builder, "searchentryTasks")); 
 labelHits = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelHits"));
 tmpStr= gtk_entry_get_text (GTK_ENTRY(entry));
 if(tmpStr) {
        hits = search_next_hit (tmpStr, data);
        if(hits>=0) {
           gtk_widget_set_sensitive (GTK_WIDGET (btnNext), TRUE);
           gtk_widget_set_sensitive (GTK_WIDGET (btnPrev), TRUE);
           /* we select the convenient line in tasks listbox */
           search_select_task_row (hits, data);
           gtk_label_set_text (GTK_LABEL(labelHits), _("yes"));
           tasks_autoscroll ((gdouble)task_get_selected_row (data)/g_list_length(data->tasksList),
                             data);
        }
        else {
           gtk_widget_set_sensitive (GTK_WIDGET (btnNext), FALSE);
           gtk_widget_set_sensitive (GTK_WIDGET (btnPrev), TRUE);
           gtk_label_set_text (GTK_LABEL(labelHits), _("none after"));
        }
 }/* if tmpStr */
}

/************************************************
 callback find for ressources module
*************************************************/
void on_find_changed (GtkSearchEntry *entry, APP_data *data)
{
 const gchar *tmpStr;
 GtkWidget *btnNext, *btnPrev, *labelHits; 
 gint hits = 0;
 GtkListBox *box;
 GtkListBoxRow *row;

 box = GTK_LIST_BOX(gtk_builder_get_object (data->builder, "listboxTasks")); 
 btnNext = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonSearchNext"));
 btnPrev = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonSearchPrev"));
 labelHits = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelHits"));
 if(entry) {
    tmpStr= gtk_entry_get_text (GTK_ENTRY(entry));
    if(tmpStr) {
        hits = search_first_hit (tmpStr, data);
        if(hits>=0) {
           gtk_widget_set_sensitive (GTK_WIDGET (btnNext), TRUE);
           gtk_widget_set_sensitive (GTK_WIDGET (btnPrev), TRUE);
           /* we select the convenient line in tasks listbox */
           search_select_task_row (hits, data);
           tasks_autoscroll ((gdouble)task_get_selected_row (data)/g_list_length(data->tasksList),
                             data);

           gtk_label_set_text (GTK_LABEL(labelHits), _("yes"));
        }
        else {
           gtk_widget_set_sensitive (GTK_WIDGET (btnNext), FALSE);
           gtk_widget_set_sensitive (GTK_WIDGET (btnPrev), FALSE);
           gtk_label_set_text (GTK_LABEL(labelHits), _("no"));
        }
    }/* if tmpStr */
 }
}

static void append_from_library (APP_data *data)
{
  GKeyFile *keyString;
  gchar *path_to_file, *filename;
  gint ret;
  GtkFileFilter *filter = gtk_file_filter_new ();

  gtk_file_filter_add_pattern (filter, "*.plibre");
  gtk_file_filter_set_name (filter, _("PlanLibre xml files"));

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");

  GtkWidget *dialog = create_loadFileDialog (data->appWindow, data, _("Append PlanLibre resources from file ..."));
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir ());
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  /* run dialog for file selection */
  if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    ret = files_append_rsc_from_library (filename, data);
    if(ret!=0) {
        gchar *msg = g_strdup_printf ("%s", _("Sorry !\nAn error happened during file importation.\nOperation finished with errors. You can try again, or\ncheck your system."));
        misc_ErrorDialog (data->appWindow, msg);
        g_free (msg);
    }
    g_free (filename);
  }

  gtk_widget_destroy (GTK_WIDGET(dialog)); 
}

/*********************************
  append ressources from library
********************************/
void on_rsc_append_from_library (GtkMenuItem *menuitem, APP_data *data)
{
  append_from_library (data);
}

void on_button_rsc_append_from_library (GtkMenuItem *menuitem, APP_data *data)
{
  append_from_library (data);
}

/*********************************
  PUBLIC : append ressources from 
  spreadsheet CSV file
********************************/
void on_rsc_append_from_csv (GtkMenuItem *menuitem, APP_data *data)
{
  GKeyFile *keyString;
  gchar *path_to_file, *filename;
  gint ret;
  GtkFileFilter *filter = gtk_file_filter_new ();

  gtk_file_filter_add_pattern (filter, "*.csv");
  gtk_file_filter_set_name (filter, _("Spreadsheets CSV files"));

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");

  GtkWidget *dialog = create_loadFileDialog(data->appWindow, data, _("Append PlanLibre resources from file ..."));
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir());
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  /* run dialog for file selection */
  if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    gtk_widget_destroy (GTK_WIDGET(dialog)); 
    ret = import_csv_file_to_rsc (filename, data);
    if(ret!=0) {
        gchar *msg = g_strdup_printf ("%s", _("Sorry !\nAn error happened during file importation.\nOperation finished with errors. You can try again, or\ncheck your system."));
        misc_ErrorDialog (data->appWindow, msg);
        g_free (msg);
    }
    g_free (filename);
  }
  else
     gtk_widget_destroy (GTK_WIDGET(dialog)); 
   /* switch to report notebook page */
  GtkWidget *notebook = GTK_WIDGET(gtk_builder_get_object (data->builder, "notebook1"));
  gtk_notebook_set_current_page (GTK_NOTEBOOK(notebook), 3);
}


/*********************************
  PUBLIC : append ressources from 
  spreadsheet CSV file

modèles dans le csv :
Prénom,Nom de famille,Nom à afficher,Surnom,Adresse électronique principale,Adresse électronique secondaire,Nom de l’écran,Tél. professionnel,Tél. personnel,Fax,Pager,Portable,Adresse privée,Adresse privée 2,Ville,Pays/État,Code postal,Pays/Région (domicile),Adresse professionnelle,Adresse professionnelle 2,Ville,Pays/État,Code postal,Pays/Région (bureau),Profession,Service,Société,Site web 1,Site web 2,Année de naissance,Mois,Jour,Divers 1,Divers 2,Divers 3,Divers 4,Notes,




********************************/
void on_rsc_append_from_thunderbird (GtkMenuItem *menuitem, APP_data *data)
{
  GKeyFile *keyString;
  gchar *path_to_file, *filename;
  gint ret;
  GtkFileFilter *filter = gtk_file_filter_new ();

  gtk_file_filter_add_pattern (filter, "*.csv");
  gtk_file_filter_set_name (filter, _("Thunderbird CSV files"));

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");

  GtkWidget *dialog = create_loadFileDialog(data->appWindow, data, _("Append PlanLibre resources from Thunderbird contacts ..."));
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir());
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  /* run dialog for file selection */
  if(gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    gtk_widget_destroy (GTK_WIDGET(dialog)); 
    ret = import_adress_book_file_to_rsc (filename, data);
    if(ret!=0) {
        gchar *msg = g_strdup_printf ("%s", _("Sorry !\nAn error happened during file importation.\nOperation finished with errors. You can try again, or\ncheck your system."));
        misc_ErrorDialog (data->appWindow, msg);
        g_free (msg);
    }
    g_free (filename);
  }
  else
     gtk_widget_destroy (GTK_WIDGET(dialog)); 
   /* switch to report notebook page */
  GtkWidget *notebook = GTK_WIDGET(gtk_builder_get_object (data->builder, "notebook1"));
  gtk_notebook_set_current_page (GTK_NOTEBOOK(notebook), 3);
}

/************************************************
 callback button UP for tasks module
*************************************************/
void on_buttonUp_tasks_clicked (GtkButton *button, APP_data *data)
{
  GtkListBox *box;
  GtkListBoxRow *row;
  gint ret=-1, type;

  box = GTK_LIST_BOX(GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasks"))); 
  row =  gtk_list_box_get_selected_row (box);
  if(row) {
        ret = gtk_list_box_row_get_index (row);  
  }

// tester s'il s'agit d'un groupe 
  type = tasks_get_type (ret, data);
  if(type==TASKS_TYPE_GROUP) {
         printf("tu demandes de monter groupe position%d \n", ret);
     tasks_group_move (tasks_get_id (ret, data), ret, ret-1, TRUE, data);
  }
  else
     tasks_move (ret, ret-1, TRUE, TRUE, data);
}

/************************************************
 callback button DOWN for tasks module
*************************************************/
void on_buttonDown_tasks_clicked (GtkButton *button, APP_data *data)
{
  GtkListBox *box;
  GtkListBoxRow *row;
  gint ret=-1, type;

  box = GTK_LIST_BOX(GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasks"))); 
  row =  gtk_list_box_get_selected_row (box);
  if(row) {
        ret = gtk_list_box_row_get_index (row);  
  }
   
  type = tasks_get_type (ret, data);
  if(type==TASKS_TYPE_GROUP) {
         printf("tu demandes de descendre groupe position%d \n", ret);
     tasks_group_move (tasks_get_id (ret, data), ret, ret+1, TRUE, data);
  }
  else
     tasks_move (ret, ret+1, TRUE, TRUE, data);
}

/********************************
  button remove task
  clicked
********************************/
void on_buttonRemove_tasks_clicked (GtkButton *button, APP_data *data)
{
  on_tasks_remove (data);
}

/********************************
  row activated in Tasks
  listbox 
********************************/
void on_tasks_row_selected (GtkListBox *box, GtkListBoxRow *row, APP_data *data)
{
  GtkWidget *btnModif, *btnRepeat, *btnUp, *btnDown, *btnTimer, *lblLine, *comboxGrp, *boxGrpShift, *btnRem;
  gint i;
  GList *l;
  gchar *tmpStr, buffer[64];
  tsk_group_type *tmp;
  tasks_data temp;

  lblLine = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelAppLine"));
  btnModif = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonTaskModify"));
  btnRepeat = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonTaskRepeat"));
  btnUp = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonTaskUp"));
  btnDown = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonTaskDown"));
  btnTimer = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonTaskTimer"));
  comboxGrp = GTK_WIDGET(gtk_builder_get_object (data->builder, "comboTasksGroup"));
  boxGrpShift = GTK_WIDGET(gtk_builder_get_object (data->builder, "boxGrpShift"));
  btnRem = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonRemove"));

  /* we free list group for combo box */
  tasks_free_list_groups (data);
  gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT(comboxGrp));
  /* beware : if this event is called after row deletion, row isn't valid ! */
  if(row) {
      data->curTasks = gtk_list_box_row_get_index (row);
      gtk_label_set_text (GTK_LABEL(lblLine), g_strdup_printf (_("L%d"), data->curTasks+1));
      gtk_widget_set_sensitive (GTK_WIDGET(btnModif), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET(btnRem), TRUE);
      if(tasks_get_type (data->curTasks, data)==TASKS_TYPE_TASK) {
           gtk_widget_set_sensitive (GTK_WIDGET(btnRepeat), TRUE);
           gtk_widget_set_sensitive (GTK_WIDGET(btnTimer), TRUE);
           gtk_widget_set_sensitive (GTK_WIDGET(comboxGrp), TRUE);
           /* we build group list and update combobox */
           tasks_fill_list_groups (data);
           gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(comboxGrp), _("None"));
           tasks_get_ressource_datas (data->curTasks, &temp, data);
	   for(i=1;i<g_list_length (data->groups);i++) {
	      l = g_list_nth (data->groups, i);
	      tmp = (tsk_group_type *) l->data;
	      if(strlen(tmp->name)<13)
		  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(comboxGrp), tmp->name);    
	      else {
                 g_utf8_strncpy (buffer, tmp->name, 12);
		 gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(comboxGrp), buffer);
              }
	   }/* next i */
         
           gtk_combo_box_set_active (GTK_COMBO_BOX(comboxGrp), tasks_get_group_rank (temp.group, data));/* default */ 
           tasks_free_list_groups (data);  
 
      }
      else {
           gtk_widget_set_sensitive (GTK_WIDGET(btnRepeat), FALSE);
           gtk_widget_set_sensitive (GTK_WIDGET(btnTimer), FALSE);
           gtk_widget_set_sensitive (GTK_WIDGET(comboxGrp), FALSE);
      }
      /* it's a group ? */
      if(tasks_get_type (data->curTasks, data)==TASKS_TYPE_GROUP) {
          tasks_get_ressource_datas (data->curTasks, &temp, data);
          if(tasks_count_tasks_linked_to_group (temp.id, data)>0) {
               gtk_widget_set_sensitive (GTK_WIDGET(boxGrpShift), TRUE);
          }
          else {
              gtk_widget_set_sensitive (GTK_WIDGET(boxGrpShift), FALSE);
          }
      }/* endif group */
      if(data->curTasks > 0) 
          gtk_widget_set_sensitive (GTK_WIDGET(btnUp), TRUE);
      else
          gtk_widget_set_sensitive (GTK_WIDGET(btnUp), FALSE);
      if(data->curTasks < tasks_datas_len (data)-1)      
          gtk_widget_set_sensitive (GTK_WIDGET(btnDown), TRUE);
      else
          gtk_widget_set_sensitive (GTK_WIDGET(btnDown), FALSE);
  }
  else {
      data->curTasks = -1;
      gtk_label_set_text (GTK_LABEL(lblLine), _("L--"));
      gtk_widget_set_sensitive (GTK_WIDGET(btnModif), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(btnRem), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(btnRepeat), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(btnUp), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(btnDown), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(btnTimer), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(comboxGrp), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(boxGrpShift), FALSE);
  }/* endif row */
}

/*******************
  quit application
 
*******************/
gboolean on_window_main_destroy (GtkWidget *window1, GdkEvent *event, APP_data *data)
{
      quit_application (data);
      return TRUE;
}

gboolean  on_menu_quit_activate (GtkMenuItem *menuitem, APP_data *data)
{
     quit_application (data);
     return TRUE;
}

/**********************************
  PUBLIC : button undo clicked 
  MAX_UNDO_OPERATION
 define in undoredo.h
***********************************/
void on_button_undo_clicked (GtkButton *button, APP_data *data)
{ 
  undo_pop (data); // TODO : not any button for now
}

/*******************************
  UNDO last operation
Note : interesting thread on wxWidgets
 about warnings on unrealiszed widget events here : https://trac.wxwidgets.org/ticket/15221

*******************************/
void
on_edit_undo_activate (GtkMenuItem *menuitem, APP_data *data)
{
printf ("avant undo pop menu \n");
  undo_pop (data);
  cursor_normal (data->appWindow);
printf ("après undo pop menu \n");
}

/*******************************
  REDO last operation
*******************************/
void
on_edit_redo_activate (GtkMenuItem *menuitem, APP_data *data)
{
  printf ("Redo \n");
}

