#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <stdlib.h>
/* translations */
#include <libintl.h>
#include <locale.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtkspell/gtkspell.h>
#include "support.h"
#include "misc.h"
#include "callbacks.h"
#include "calendars.h"

/********************************
  a simple function to get 
  a Pango string in order to
  display shortcuts
  modifier : NONE =0
  1=CTRL, 2=SHIFT
********************************/
gchar *misc_get_pango_string (const gchar *key, const gint modifier)
{
  switch(modifier) {
    case 0:{
     return g_strdup_printf (" <span font=\"11\" foreground=\"white\" background=\"#15142B\"><b> %s </b></span>", key);
     break;
    }
    case 1:{
     return g_strdup_printf (" <span font=\"11\" foreground=\"white\" background=\"#6F7DC8\"><b>CTRL</b></span> + <span font=\"11\" foreground=\"white\" background=\"#15142B\"><b> %s </b></span>", key);
     break;
    }
   case 2:{
     return g_strdup_printf (_(" <span font=\"11\" foreground=\"white\" background=\"#2C8A3C\"><b>SHIFT</b></span> + <span font=\"11\" foreground=\"white\" background=\"#15142B\"><b> %s </b></span>"), key);
     break;
    }
   default:;
  }/* end switch */
}


/*
 * Internal helper: destroys pop-up menu created with right-click on results table.
 */
void undo_popup_menu (GtkWidget *attach_widget, GtkMenu *menu)
{  
  gtk_widget_destroy (GTK_WIDGET(menu));
}

/********************************
  display status of application

********************************/
void misc_display_app_status (gboolean modified, APP_data *data)
{
  GtkWidget *labelModified;

  labelModified = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelAppStatus"));
  if(modified) {
     data->fProjectModified = TRUE;
     gtk_label_set_markup (GTK_LABEL(labelModified), _("<span foreground=\"red\">Modified</span>"));
  }
  else {
     data->fProjectModified = FALSE;
     gtk_label_set_markup (GTK_LABEL(labelModified), _("<span foreground=\"green\">Ready</span>"));
  }
}

/********************************
  display status of application

********************************/
void misc_display_tips_status (const gchar *str, APP_data *data)
{

  GtkWidget *labelTip = GTK_WIDGET (gtk_builder_get_object (data->builder, "labelTips"));
  gtk_label_set_markup (GTK_LABEL (labelTip), g_strdup_printf ("<i>%s</i>", str));
 
}

/**********************
 initialize various
  values
**********************/
void misc_init_vars (APP_data *data)
{
  GDate current_date;

  /* project */
  data->objectsCounter = 0;/* internal counter, never decremented, in order to give UNIQUE id. to objects */
  data->linksList = NULL; /* very important list ! Contains all links between objects */
  /* RSC */
  data->curRsc = -1; /* index on selected ressource */
  data->rscList = NULL;

  data->path_to_pixbuf = NULL;
  data->path_to_snapshot = NULL;

  data->tmpBuilder = NULL;

  /* tasks */
  data->tasksList = NULL;
  data->groups = NULL;
  data->curTasks=-1; /* idem for tasks */
  data->treeViewTasksRsc = NULL;
  data->treeViewTasksTsk = NULL;
  data->treeViewReportMail = NULL;
  data->treeViewCalendars = NULL;
  data->calendars = NULL;
   /* project properties */
  data->properties.manager_name = g_strdup_printf ("%s", g_get_user_name ());
  data->properties.location = NULL;
  data->properties.gps = NULL;
  data->properties.units=NULL;
  data->properties.title = NULL;
  data->properties.summary = NULL;
  data->properties.manager_phone = NULL;
  data->properties.manager_mail = NULL;
  data->properties.org_name = NULL;
  data->properties.proj_website = NULL;
  data->properties.logo = NULL;
  /* now, we set-up default 'date' strings */
  /* we get the current date */
  g_date_set_time_t (&current_date, time (NULL)); /* date of today */
  /* start date : today */
  data->properties.start_nthDay = g_date_get_day (&current_date);
  data->properties.start_nthMonth = g_date_get_month (&current_date);
  data->properties.start_nthYear = g_date_get_year (&current_date);
  data->properties.start_date = misc_convert_date_to_locale_str (&current_date);
  /* default end date : today + 2 weeks */
  g_date_add_days (&current_date, 14);
  data->properties.end_nthDay = g_date_get_day (&current_date);
  data->properties.end_nthMonth = g_date_get_month (&current_date);
  data->properties.end_nthYear = g_date_get_year (&current_date);
  data->properties.end_date =  misc_convert_date_to_locale_str (&current_date);
  /* default schedules */
  data->properties.scheduleStart = 540; /* 09h00m */
  data->properties.scheduleEnd = 1080; /* 18h00m */
  /* APP */
  data->fProjectModified = FALSE;
  data->fProjectHasName = FALSE;
  data->currentStack = CURRENT_STACK_HOME;
  data->button_pressed = FALSE;
  data->fProjectHasSnapshot = FALSE;
  /* we prepare Sketch area display */
  data->ruler = NULL;
  /* undo engine */
 
}

/******************************************
 initalize spell checker. There is a 
  global pointer (for unref)
 BUT PLEASE NOTICE : spell checker is 
 attached to the  CURRENT object
*******************************************/
void misc_init_spell_checker (GtkTextView *view, APP_data *data)
{
  GtkSpellChecker* spell = gtk_spell_checker_new ();
  gtk_spell_checker_set_language (spell, pango_language_to_string (pango_language_get_default()), NULL);
  data->spell = spell;
  gboolean fAttachSpell = gtk_spell_checker_attach (spell, GTK_TEXT_VIEW (view));
}

/**********************************
  prepare timeouts
**********************************/
void misc_prepare_timeouts (APP_data *data)
{
  GtkWidget *imgAutoSave = GTK_WIDGET(gtk_builder_get_object (data->builder, "image_task_due"));

  if(g_key_file_get_boolean (data->keystring, "application", "interval-save",  NULL)) {
      gtk_widget_show (imgAutoSave);
  }
  else
      gtk_widget_hide (imgAutoSave);

  g_timeout_add_seconds (AUTOSAVE_BEAT, timeout_quick_save, data);/* yes, it's an intelligent call, keep as it */
  g_timeout_add_seconds (TOMATO_BEAT, timeout_tomato_timer, data);/* yes, it's an intelligent call, keep as it */

}

/**********************************
  function to load fonts and
  color settings
**********************************/
void misc_set_font_color_settings (APP_data *data)
{
  GKeyFile *keyString;
  GdkRGBA color, b_color;

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");

  /* set-up default value only with CSS */
  const gchar *fntFamily = NULL;
  gint fntSize = 12;
 // PangoContext* context = gtk_widget_get_pango_context  (GTK_WIDGET(app_data.view));
  PangoFontDescription *desc;// = pango_context_get_font_description(context);   
  desc = pango_font_description_from_string (g_key_file_get_string (keyString, "editor", "font", NULL));
  if(desc != NULL) {
          fntFamily = pango_font_description_get_family (desc);
          fntSize = pango_font_description_get_size (desc)/1000;
  }
  
  /* change gtktextview colors compliant with Gtk 3.16+ pLease note : re-change seleted state is mandatory, even if defned in interface*/
  color.red = g_key_file_get_double (keyString, "editor", "text.color.red", NULL);
  color.green = g_key_file_get_double (keyString, "editor", "text.color.green", NULL);
  color.blue = g_key_file_get_double (keyString, "editor", "text.color.blue", NULL);
  color.alpha = 1;
  /* paper color */
  b_color.red = g_key_file_get_double (keyString, "editor", "paper.color.red", NULL);
  b_color.green = g_key_file_get_double (keyString, "editor", "paper.color.green", NULL);
  b_color.blue = g_key_file_get_double (keyString, "editor", "paper.color.blue", NULL);
  b_color.alpha = 1;

  GtkCssProvider* css_provider = gtk_css_provider_new();
  gchar *css;
  css = g_strdup_printf ("  #view  { font-family:%s; font-size:%dpx; color: #%.2x%.2x%.2x; }\n  #view:selected, #view:selected:focus { background-color: @selected_bg_color; color:@selected_fg_color; }\n",
                 fntFamily, fntSize,
                 (gint)(color.red*255), (gint)(color.green*255), (gint)(color.blue*255));

  if(desc)
      pango_font_description_free (desc);

  gtk_css_provider_load_from_data (css_provider, css, -1, NULL);
  GdkScreen* screen = gdk_screen_get_default ();
  gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_free (css);

}

/****************************************************
  display an error/warning dialog
****************************************************/
void misc_ErrorDialog (GtkWidget *widget, const gchar* msg)
{ 
   GtkWidget *dialog;

   dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW(widget),
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_ERROR,
                                           GTK_BUTTONS_CLOSE,
                                           msg, NULL);
   gtk_dialog_run (GTK_DIALOG (dialog));
   gtk_widget_destroy (dialog);
}

/****************************************************
  display an info:confirm dialog
****************************************************/
void misc_InfoDialog (GtkWidget *widget, const gchar* msg)
{ 
   GtkWidget *dialog;

   dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW(widget),
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_INFO,
                                           GTK_BUTTONS_CLOSE,
                                           msg, NULL);
   gtk_dialog_run (GTK_DIALOG (dialog));
   gtk_widget_destroy (dialog);
}


/****************************************************
  display an info:confirm/dismisss dialog
****************************************************/
gint misc_QuestionDialog (GtkWidget *widget, const gchar* msg)
{ 
   GtkWidget *dialog;
   gint ret;

   dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW(widget),
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_INFO,
                                           GTK_BUTTONS_YES_NO,
                                           msg, NULL);
   ret = gtk_dialog_run (GTK_DIALOG (dialog));
   gtk_widget_destroy (dialog);
   return ret;
}


/*******************************************
  converts a gdkcolor to a string in 
  hexadecimal, compatible with CSS
  notation
*******************************************/
gchar *misc_convert_gdkcolor_to_hex (GdkRGBA color )
{

  /* converts to hex color description */
  gchar *sRgba;
  sRgba = g_strdup_printf ("#%02X%02X%02X", (int)(color.red*255), (int)(color.green*255), (int)(color.blue*255));
  return sRgba;
}

/*******************************************
  function to 'force' usage of decimal 
  point in order to keep compatibilty
  with other counties
  input : a gdouble value
  output : a string with a decimal point
*******************************************//*
gchar *misc_force_decimal_point( gdouble val)
{
  
  gint val_int =(gint) val;
  gdouble val_frac = 100*(val-val_int);
  if(val_frac<10)
    return g_strdup_printf ("%d.0%.0f",  val_int, val_frac);
  else
    return g_strdup_printf ("%d.%.0f", val_int, val_frac);
}

/**********************
  create a calendar
 *********************/

GtkWidget *misc_create_calendarDialog (GtkWidget *win)
{
  GtkWidget *calendarDialog, *dialog_vbox11;
  GtkWidget *calendar1, *dialog_action_area11;
  GtkWidget *cancelbutton5, *okbutton6;
  GtkWidget *gridCalendar, *labelCurrentDay;
  GtkWidget *labelToday, *labelCurrentMonth;
  GtkWidget *labelCurrentYear, *labelSpace;


  calendarDialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (calendarDialog), _("Select Date..."));
  /* GTK_WINDOW (calendarDialog)->type = GTK_WINDOW_POPUP;*/
 
  gtk_window_set_position (GTK_WINDOW (calendarDialog), GTK_WIN_POS_MOUSE);
  gtk_window_set_modal (GTK_WINDOW (calendarDialog), TRUE);
  gtk_window_set_resizable (GTK_WINDOW (calendarDialog), FALSE);
  gtk_window_set_type_hint (GTK_WINDOW (calendarDialog), GDK_WINDOW_TYPE_HINT_UTILITY);
  gtk_window_set_transient_for (GTK_WINDOW (calendarDialog), GTK_WINDOW(win)); 

  dialog_vbox11 = gtk_dialog_get_content_area (GTK_DIALOG (calendarDialog));
  gtk_widget_show (dialog_vbox11);
  gridCalendar = gtk_grid_new ();
  gtk_widget_set_name (gridCalendar,"gridCalendar");
  gtk_widget_show (gridCalendar);
  gtk_box_pack_start (GTK_BOX (dialog_vbox11), gridCalendar, TRUE, TRUE, 0);
  /* labels */
  /* we must get current date */
  GDateTime *currentDateTime = g_date_time_new_now_local ();
  /* we display day of the month/year in a stylish way */
  labelSpace = gtk_label_new ("  ");
  gtk_widget_show (labelSpace);
  gtk_grid_attach (GTK_GRID (gridCalendar), labelSpace, 0, 0, 1, 2);
  labelToday = gtk_label_new (_("<b>Today :</b>"));
  gtk_widget_show (labelToday);
  gtk_label_set_use_markup (GTK_LABEL (labelToday), TRUE);
  gtk_grid_attach (GTK_GRID (gridCalendar), labelToday, 1, 0, 1, 2);
  labelCurrentDay = gtk_label_new (  
                    g_strdup_printf ("<span font=\"36\" color=\"#8DB5EE\"><b><i> %d </i></b></span>",   
                    g_date_time_get_day_of_month(currentDateTime )) 
                    );
  gtk_widget_show (labelCurrentDay);
  gtk_label_set_use_markup (GTK_LABEL (labelCurrentDay), TRUE);
  gtk_grid_attach (GTK_GRID (gridCalendar), labelCurrentDay, 2, 0, 1, 2);
  labelCurrentMonth = gtk_label_new (g_strdup_printf ("<b><u>%s</u></b>", 
                      g_date_time_format (currentDateTime, "%B") ) );
  gtk_widget_show (labelCurrentMonth);
  gtk_label_set_use_markup (GTK_LABEL (labelCurrentMonth), TRUE);
  gtk_grid_attach (GTK_GRID (gridCalendar), labelCurrentMonth, 3, 0, 1, 1);
  labelCurrentYear = gtk_label_new (g_strdup_printf ("<span  font=\"18\"><b>%d</b></span>", 
                                  g_date_time_get_year (currentDateTime)));
  gtk_widget_show (labelCurrentYear);
  gtk_label_set_use_markup (GTK_LABEL (labelCurrentYear), TRUE);
  gtk_grid_attach (GTK_GRID (gridCalendar), labelCurrentYear, 3, 1, 1, 1);
  g_date_time_unref (currentDateTime);
  calendar1 = gtk_calendar_new ();
  gtk_widget_show (calendar1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox11), calendar1, TRUE, TRUE, 0);
  gtk_calendar_set_display_options (GTK_CALENDAR (calendar1),
                                GTK_CALENDAR_SHOW_HEADING
                                | GTK_CALENDAR_SHOW_DAY_NAMES);

  dialog_action_area11 = gtk_dialog_get_action_area (GTK_DIALOG (calendarDialog));
  gtk_widget_show (dialog_action_area11);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area11), GTK_BUTTONBOX_END);

  cancelbutton5 = gtk_button_new_with_label (_("Cancel" ));
  GtkWidget *image43 = gtk_image_new_from_icon_name ("gtk-cancel",  GTK_ICON_SIZE_BUTTON);
  gtk_button_set_always_show_image (GTK_BUTTON (cancelbutton5), TRUE);
  gtk_button_set_image (GTK_BUTTON (cancelbutton5), image43);
  gtk_widget_show (cancelbutton5);
  gtk_dialog_add_action_widget (GTK_DIALOG (calendarDialog), cancelbutton5, GTK_RESPONSE_CANCEL);
  gtk_widget_set_can_default (cancelbutton5, TRUE);

  okbutton6 = gtk_button_new_with_label (_("Ok" ));
  GtkWidget *image44 = gtk_image_new_from_icon_name ("gtk-ok",  GTK_ICON_SIZE_BUTTON);
  gtk_button_set_always_show_image (GTK_BUTTON (okbutton6), TRUE);
  gtk_button_set_image (GTK_BUTTON (okbutton6), image44);
  gtk_widget_show (okbutton6);
  gtk_dialog_add_action_widget (GTK_DIALOG (calendarDialog), okbutton6, GTK_RESPONSE_OK);
  gtk_widget_set_can_default (okbutton6, TRUE);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (calendarDialog, calendarDialog, "calendarDialog");
  GLADE_HOOKUP_OBJECT_NO_REF (calendarDialog, dialog_vbox11, "dialog_vbox11");
  GLADE_HOOKUP_OBJECT (calendarDialog, calendar1, "calendar1");
  GLADE_HOOKUP_OBJECT_NO_REF (calendarDialog, dialog_action_area11, "dialog_action_area11");
  GLADE_HOOKUP_OBJECT (calendarDialog, cancelbutton5, "cancelbutton5");
  GLADE_HOOKUP_OBJECT (calendarDialog, okbutton6, "okbutton6");

  return calendarDialog;
}

/**********************************
 Callback helper: Displays a 
 calendar popup and returns 
 selected date 
 *********************************/
gchar *misc_getDate (const gchar *curDate, GtkWidget *win)
{
  GtkWidget * calendarDialog = misc_create_calendarDialog (win);
  GtkCalendar *calendar = GTK_CALENDAR(lookup_widget (calendarDialog, "calendar1"));
  gchar* result = g_strdup (curDate);
  guint year, month, day;
  GDate date;
  
  g_date_set_parse (&date, curDate);
    
  if (g_date_valid(&date)) {
    year = g_date_get_year (&date);
    month = g_date_get_month (&date) - 1; /*Glib has months from 1-12 while GtkCalendar uses 0 - 11 */
    day = g_date_get_day (&date);
    gtk_calendar_select_day (calendar, day);
    gtk_calendar_select_month (calendar, month, year);
  }
    
  if(gtk_dialog_run(GTK_DIALOG(calendarDialog)) == GTK_RESPONSE_OK) {

    gtk_calendar_get_date (calendar, &year, &month, &day);
    gtk_widget_destroy (calendarDialog);

    result = (gchar *)g_malloc(sizeof(gchar) * 24);/* improved from 12 to 24 - Luc A., 27 déc 2017 */
    g_date_strftime (result, 23, "%d %b %Y", g_date_new_dmy(day, month +1, year));/* modifiyed from 11 to 23 - year with 4 digits  Luc A., 27 déc 2017 */
  } else {
    gtk_widget_destroy (calendarDialog);
  }

  return result;
}

/***********************************
  converts a natural dd/mm/yy date
  to a newly allocated string
***********************************/
gchar *misc_convert_date_to_str (gint dd, gint mm, gint yy)
{
  GDate date;
  gchar buffer[80];

  if(dd<0 || dd>31)
      return NULL;
  if(mm<0 || mm>12)
      return NULL;
  if(yy<0)
      return NULL;
  g_date_set_dmy (&date, dd, mm, yy);
  g_date_strftime (buffer, 75, _("%x"), &date);
  return  g_strdup_printf ("%s", buffer);
}

/******************************
  get the earliest date
  between all tasks
******************************/
GDate misc_get_earliest_date (APP_data *data)
{
  GDate date, date_start;
  tasks_data *tmp_tasks_datas;
  gint i;
  GList *l;

  g_date_set_dmy (&date, 1, 1, 2100);
  if( g_list_length (data->tasksList)==0) {
      g_date_set_dmy (&date, data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
      return date;
  }
  /* now, we read all startdates */
  for(i=0; i<g_list_length (data->tasksList); i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     if(tmp_tasks_datas->type!=TASKS_TYPE_GROUP) {        
          g_date_set_dmy (&date_start, 
                         tmp_tasks_datas->start_nthDay, tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
          if(g_date_valid (&date_start)) {
             gint cmp_date = g_date_compare (&date, &date_start);
             if(cmp_date>0) {
                date = date_start;
             }
          }/* date valid */
     }
  }/* next */

  return date;
}

/******************************
  get the latest date
  between all tasks
******************************/
GDate misc_get_latest_date (APP_data *data)
{
  GDate date, date_end;
  tasks_data *tmp_tasks_datas;
  gint i;
  GList *l;

  g_date_set_dmy (&date, 1, 1, 1970);
  if(g_list_length (data->tasksList)==0) {
      g_date_set_dmy (&date, data->properties.end_nthDay, data->properties.end_nthMonth, data->properties.end_nthYear);
      return date;
  }
  /* now, we read all startdates */
  for(i=0; i<g_list_length (data->tasksList); i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     if(tmp_tasks_datas->type!=TASKS_TYPE_GROUP) {        
          g_date_set_dmy (&date_end, 
                         tmp_tasks_datas->end_nthDay, tmp_tasks_datas->end_nthMonth, tmp_tasks_datas->end_nthYear);
          if(g_date_valid (&date_end )) {
             gint cmp_date = g_date_compare (&date, &date_end);
             if(cmp_date<0) {
                date = date_end;
             }
          }/* date valid */
     }
  }/* next */

  return date;
}

/*************************************
  converts a GDate to localiszed date
  the new string must be freed
*************************************/
gchar *misc_convert_date_to_locale_str (GDate *date)
{
  gchar buffer[80];

  g_date_strftime (buffer, 75, _("%x"), date);
  return g_strdup_printf ("%s", buffer);
}

/*****************************************
  converts day and hours durations
 into human ftiendly string
  the new string must be freed
*****************************************/
gchar *misc_duration_to_friendly_str (gint days, gint minutes)
{
   gchar *str;

   if(days==0) {
       str = g_strdup_printf (_("%d hour(s)."), minutes /60);
   }
   else {
      if(minutes==0) {
         str = g_strdup_printf (_("%d day(s)."), days);
      }
      else {
         str = g_strdup_printf (_("%d day(s) and %d hour(s)."), days, minutes/60);
      }
   }
   return str;
}

/*****************************************
  converts a GDate to human ftiendly date
  the new string must be freed
*****************************************/
gchar *misc_convert_date_to_friendly_str (GDate *date)
{
  gchar buffer[80];
  time_t rawtime; 
  GDate today;
  gint cmp;

  /* we get the current date */
  time (&rawtime);
  strftime (buffer, 80, "%x", localtime (&rawtime));
  g_date_set_parse (&today, buffer);
  /* we compute current date */
  g_date_strftime (buffer, 75, _("%x"), date);
  cmp = g_date_compare (&today, date);
  if(cmp == 0) {
      return g_strdup_printf ("%s", _("Today"));
  }
  cmp = g_date_days_between (&today, date );
  if(cmp==1) {
      return g_strdup_printf ("%s", _("Tomorrow"));
  }
  return g_strdup_printf ("%s", buffer);
}

/****************************************
  general alert message before clearing
  datas
  returns TRUE if user accepts to SAVE
  datas before clearing
****************************************/
gboolean misc_alert_before_clearing (APP_data *data)
{
  GtkWidget *dialog;
  GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
  gboolean flag = TRUE;

  dialog = gtk_message_dialog_new (GTK_WINDOW(data->appWindow), flags,
                    GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                _("Your current project is modified.\nDo you want to save it before\nclearing all datas ?"));
  if(gtk_dialog_run (GTK_DIALOG(dialog))!=GTK_RESPONSE_YES)
         flag = FALSE;
  gtk_widget_destroy (GTK_WIDGET(dialog));
  return flag;
}

/***************************************
  PUBLIC : retuens the higheest
  Identifier used in current datas
***************************************/
gint misc_get_highest_id (APP_data *data)
{
   gint ret = 0, i, lenList;
   GList *l;
   rsc_datas *tmp_rsc_datas;
   tasks_data *tmp_tasks_datas;

   lenList = g_list_length (data->rscList);
   for(i=0; i<lenList; i++) {
     l = g_list_nth (data->rscList, i);
     tmp_rsc_datas = (rsc_datas *)l->data;
     if(tmp_rsc_datas->id > ret)
        ret = tmp_rsc_datas->id;
   }

   lenList = g_list_length (data->tasksList);
   for(i=0; i<lenList; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     if(tmp_tasks_datas->id > ret)
        ret = tmp_tasks_datas->id;
   }

   return ret;
}

/*************************************
  converts two dates in duration
 (in hours)
*************************************/
gdouble misc_date_to_duration (GDate *date1, GDate *date2)
{
  gdouble ret = TASKS_WORKING_DAY; /* default : 1 day == 8 hours */

  gboolean fErrDateInterval1 = g_date_valid (date1);
  gboolean fErrDateInterval2 = g_date_valid (date2);

  if( (!fErrDateInterval1) || (!fErrDateInterval2)) {printf("err date \n");
     return 1;
  }
  ret = g_date_days_between (date1, date2);
  if(ret<0)
    ret = -1*ret;
  
  ret = (ret+1) *1;
  return ret;
}

/**************************************
  tests if a task is in overdue
  situation ; i.e. progress <100%
  and due date depassed
*************************************/
gboolean misc_task_is_overdue (gint progress, GDate *end_date)
{
  GDate current_date;
  gboolean ret = FALSE;
  
  g_date_set_time_t (&current_date, time (NULL)); /* date of today */

  if((g_date_compare (&current_date, end_date)>0) && (progress<100)){
      ret = TRUE;
  }     
  return ret;

}

/**********************************
  counts how many tasks are overdue
  PLEASE NOTE : use only when
  status are updated
**********************************/
gint misc_count_tasks_overdue (APP_data *data)
{
  tasks_data *tmp_tsk_datas;
  GList *l;
  gint overdue = 0, i;
  gint total = g_list_length (data->tasksList);
  GDate date;

  for(i=0; i<total; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if(tmp_tsk_datas->type == TASKS_TYPE_TASK) {
        /* test if overdue */
        g_date_set_dmy (&date, tmp_tsk_datas->end_nthDay, tmp_tsk_datas->end_nthMonth, tmp_tsk_datas->end_nthYear);
        if( misc_task_is_overdue (tmp_tsk_datas->progress, &date) )
             overdue++; 
     }/* endif type task */
  }/* next */

  return overdue;
}

/**********************************
  counts how many tasks are overdue
  PLEASE NOTE : use only when
  status are updated
**********************************/
gint misc_count_tasks_run (APP_data *data)
{
  tasks_data *tmp_tsk_datas;
  GList *l;
  gint count = 0, i;
  gint total = g_list_length (data->tasksList);
  GDate date;

  for(i=0; i<total; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if( tmp_tsk_datas->type == TASKS_TYPE_TASK) {
        /* test if running */
        g_date_set_dmy (&date, tmp_tsk_datas->end_nthDay, tmp_tsk_datas->end_nthMonth, tmp_tsk_datas->end_nthYear);
        if( misc_task_is_overdue (tmp_tsk_datas->progress, &date) )
             count++;
        if( tmp_tsk_datas->status == TASKS_STATUS_RUN)
             count++;
     }/* endif type task */
  }/* next */

  return count;
}

/**********************************
  counts how many tasks starting
  today
  PLEASE NOTE : use only when
  status are updated
**********************************/
gint misc_count_tasks_today (APP_data *data)
{
  tasks_data *tmp_tsk_datas;
  GList *l;
  gint count = 0, i;
  gint total = g_list_length (data->tasksList);
  GDate date, current_date;

  g_date_set_time_t (&current_date, time (NULL)); /* date of today */

  for(i=0; i<total; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if( tmp_tsk_datas->type == TASKS_TYPE_TASK) {
        /* test if today */
        g_date_set_dmy (&date, tmp_tsk_datas->start_nthDay, tmp_tsk_datas->start_nthMonth, tmp_tsk_datas->start_nthYear);
        if( g_date_compare (&date, &current_date)==0)
           count++;
     }/* endif type task */
  }/* next */

  return count;
}

/**********************************
  counts how many milestones 
  happens today
  PLEASE NOTE : use only when
  status are updated
**********************************/
gint misc_count_milestones_today (APP_data *data)
{
  tasks_data *tmp_tsk_datas;
  GList *l;
  gint count = 0, i;
  gint total = g_list_length (data->tasksList);
  GDate date, current_date;

  g_date_set_time_t (&current_date, time (NULL)); /* date of today */

  for(i=0; i<total; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if( tmp_tsk_datas->type == TASKS_TYPE_MILESTONE) {
        /* test if today */
        g_date_set_dmy (&date, tmp_tsk_datas->start_nthDay, tmp_tsk_datas->start_nthMonth, tmp_tsk_datas->start_nthYear);
        if(g_date_compare (&date, &current_date)==0)
           count++;
     }/* endif type task */
  }/* next */

  return count;
}

/**********************************
  counts how many tasks starting
  tomorrow
  PLEASE NOTE : use only when
  status are updated
**********************************/
gint misc_count_tasks_tomorrow (APP_data *data)
{
  tasks_data *tmp_tsk_datas;
  GList *l;
  gint count = 0, i;
  gint total = g_list_length (data->tasksList);
  GDate date, current_date;

  g_date_set_time_t (&current_date, time (NULL)); /* date of today */
  g_date_add_days (&current_date, 1);
  for(i=0; i<total; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if( tmp_tsk_datas->type == TASKS_TYPE_TASK) {
        /* test if tomorrow */
        g_date_set_dmy (&date, tmp_tsk_datas->start_nthDay, tmp_tsk_datas->start_nthMonth, tmp_tsk_datas->start_nthYear);
        if( g_date_compare (&date, &current_date)==0)
           count++;
     }/* endif type task */
  }/* next */

  return count;
}

/**********************************
  counts how many tasks ending
  today
  PLEASE NOTE : use only when
  status are updated
**********************************/
gint misc_count_tasks_ending_today (APP_data *data)
{
  tasks_data *tmp_tsk_datas;
  GList *l;
  gint count = 0, i;
  gint total = g_list_length (data->tasksList);
  GDate date, current_date;

  g_date_set_time_t (&current_date, time (NULL)); /* date of today */
  for(i=0; i<total; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if( tmp_tsk_datas->type == TASKS_TYPE_TASK) {
        /* test if ending today */
        g_date_set_dmy (&date, tmp_tsk_datas->start_nthDay, tmp_tsk_datas->start_nthMonth, tmp_tsk_datas->start_nthYear);
        if( g_date_compare (&date, &current_date)==0)
           count++;
     }/* endif type task */
  }/* next */

  return count;
}

/**********************************
  counts how many tasks completed
  yesterday
  PLEASE NOTE : use only when
  status are updated
**********************************/
gint misc_count_tasks_completed_yesterday (APP_data *data)
{
  tasks_data *tmp_tsk_datas;
  GList *l;
  gint count = 0, i;
  gint total = g_list_length (data->tasksList);
  GDate date, current_date;

  g_date_set_time_t (&current_date, time (NULL)); /* date of today */
  g_date_subtract_days (&current_date, 1);
  for(i=0; i<total; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if( tmp_tsk_datas->type == TASKS_TYPE_TASK) {
        /* test if completed yesterday */
        g_date_set_dmy (&date, tmp_tsk_datas->start_nthDay, tmp_tsk_datas->start_nthMonth, tmp_tsk_datas->start_nthYear);
        if( g_date_compare (&date, &current_date)==0)
           count++;
     }/* endif type task */
  }/* next */

  return count;
}

/**********************************
  counts how many tasks are completed
  PLEASE NOTE : us eonly when
  status are updated
**********************************/
gint misc_count_tasks_completed (APP_data *data)
{
  tasks_data *tmp_tsk_datas;
  GList *l;
  gint count = 0, i;
  gint total = g_list_length ( data->tasksList);
  GDate date;

  for(i=0; i<total; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if( tmp_tsk_datas->type == TASKS_TYPE_TASK) {
        if( tmp_tsk_datas->status == TASKS_STATUS_DONE)
             count++;
     }/* endif type task */
  }/* next */

  return count;
}

/***********************************
  PUBLIC : count
  total number of tasks 
***********************************/
gint misc_count_tasks (APP_data *data)
{
  gint i, total;
  tasks_data *tmp_tsk_datas;
  GList *l;

  total = 0;

  for(i=0; i<g_list_length (data->tasksList); i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if( tmp_tsk_datas->type == TASKS_TYPE_TASK) {
        total++;
     }/* endif type task */
  }

  return total;
}

/*********************************************
  PUBLIC : remove non working days between 
  a start date and a known duration
  use calendar for each ressource 
  output : number of days
********************************************/
gdouble misc_correct_duration (gint day, gint month, gint year, gint dur, gboolean monday, gboolean tuesday, gboolean wednesday,
                              gboolean thursday, gboolean friday, gboolean saturday, gboolean sunday,
                              gint task_calendar, gint calendar, APP_data *data)
{
  gdouble ret = 0;
  gint duration = 0, wd, index, indexTsk;
  gint t_day, t_month, t_year, t_mark, t_mark_tsk;
  GDate date;

  g_date_set_dmy (&date, day, month, year);// TODO check date is valid 
  index = calendars_get_nth_for_id (calendar, data);/* calendar = internal ID, not rank in GList */
  indexTsk = calendars_get_nth_for_id (task_calendar, data);

  while(duration < dur) {
     wd = g_date_get_weekday (&date);
     t_day = g_date_get_day (&date);
     t_month = g_date_get_month (&date);
     t_year = g_date_get_year (&date);
     t_mark = calendars_date_is_marked (t_day, t_month, t_year, index, data);
     t_mark_tsk = calendars_date_is_marked (t_day, t_month, t_year, indexTsk, data);/* calendar for tasks */

     switch(wd) {
       case G_DATE_MONDAY: {
         if((monday) && (t_mark<0) && (t_mark_tsk<0))
            ret++;
         break;
       }
       case G_DATE_TUESDAY: {
         if((tuesday) && (t_mark<0) && (t_mark_tsk<0))
            ret++;
         break;
       }
       case G_DATE_WEDNESDAY: {
         if((wednesday) && (t_mark<0) && (t_mark_tsk<0))
            ret++;
         break;
       }
       case G_DATE_THURSDAY: {
         if((thursday) && (t_mark<0) && (t_mark_tsk<0))
            ret++;
         break;
       }
       case G_DATE_FRIDAY: {
         if((friday) && (t_mark<0) && (t_mark_tsk<0))
            ret++;
         break;
       }
       case G_DATE_SATURDAY: {
         if((saturday) && (t_mark<0) && (t_mark_tsk<0))
            ret++;
         break;
       }
       case G_DATE_SUNDAY: {
         if((sunday) && (t_mark<0) && (t_mark_tsk<0))
            ret++;
         break;
       }
       case G_DATE_BAD_WEEKDAY: {
          printf ("* Bad date ! *\n");
       }
     }/* end switch */
     duration++;
     g_date_add_days (&date, 1);
  }/* wend */

  return ret;
}

/*********************************************
  PUBLIC : remove non working days since 
  a start date and a known duration
  use calendar for each ressource ;
  peiods are in account
  returns effective minutes of work
********************************************/
gdouble misc_correct_duration_with_periods (gint max_day_minutes, gint day, gint month, gint year, gint dur, gint hours,
                                            gboolean monday, gboolean tuesday, gboolean wednesday,
                                            gboolean thursday, gboolean friday, gboolean saturday, gboolean sunday, 
                                            gint task_calendar, gint calendar, APP_data *data)
{
  gdouble ret = 0;
  gint duration = 0, wd, start_hour, end_hour, value, index, indexTsk, days;
  gint t_day, t_month, t_year, t_mark, t_mark_tsk;
  GDate date;

  g_date_set_dmy (&date, day, month, year);// TODO check date is valid 

  index = calendars_get_nth_for_id (calendar, data);
  indexTsk = calendars_get_nth_for_id (task_calendar, data);
  days = dur;
  if(days<=0)
     days = 1;
  while(duration < days) {
     wd = g_date_get_weekday (&date);
     /* prepare for calendars */
     t_day = g_date_get_day (&date);
     t_month = g_date_get_month (&date);
     t_year = g_date_get_year (&date);
     t_mark = calendars_date_is_marked (t_day, t_month, t_year, index, data);/* calendar = internal ID, not rank in GList */
     t_mark_tsk = calendars_date_is_marked (t_day, t_month, t_year, indexTsk, data);/* calendar for tasks */
     value = 0;
     switch(wd) {
       case G_DATE_MONDAY: {
         if((monday) && (t_mark<0) && (t_mark_tsk<0)) {
            /* now we check current daily schedule */
            start_hour = calendars_get_starting_hour (index, CAL_MONDAY, data);
            end_hour = calendars_get_ending_hour (index, CAL_MONDAY, data);
            if((start_hour<0) || (end_hour<0)) {
               start_hour = 0; /* simplyest */
               end_hour = max_day_minutes;
            }
            if(dur>0) {
               value = end_hour-start_hour;
            }
            else {
               value = hours*60;
            }
            if(value>max_day_minutes)
                value = max_day_minutes;

            ret = ret+value;
         }
         break;
       }
       case G_DATE_TUESDAY: {
         if((tuesday) && (t_mark<0) && (t_mark_tsk<0)) {
            start_hour = calendars_get_starting_hour (index, CAL_TUESDAY, data);
            end_hour = calendars_get_ending_hour (index, CAL_TUESDAY, data);
            if((start_hour<0) || (end_hour<0)) {
               start_hour = 0; /* simplyest */
               end_hour = max_day_minutes;
            }
            if(dur>0) {
               value = end_hour-start_hour;
            }
            else {
               value = hours*60;
            }
            if(value>max_day_minutes)
                value = max_day_minutes;
            ret = ret+value;
         }
         break;
       }
       case G_DATE_WEDNESDAY: {
         if((wednesday) && (t_mark<0) && (t_mark_tsk<0)) {
            start_hour = calendars_get_starting_hour (index, CAL_WEDNESDAY, data);
            end_hour = calendars_get_ending_hour (index, CAL_WEDNESDAY, data);
            if((start_hour<0) || (end_hour<0)) {
               start_hour = 0; /* simplyest */
               end_hour = max_day_minutes;
            }
            if(dur>0) {
               value = end_hour-start_hour;
            }
            else {
               value = hours*60;
            }
            if(value>max_day_minutes)
                value = max_day_minutes;
            ret = ret+value;
         }
         break;
       }
       case G_DATE_THURSDAY: {
         if((thursday) && (t_mark<0) && (t_mark_tsk<0)) {
            start_hour = calendars_get_starting_hour (index, CAL_THURSDAY, data);
            end_hour = calendars_get_ending_hour (index, CAL_THURSDAY, data);
            if((start_hour<0) || (end_hour<0)) {
               start_hour = 0; /* simplyest */
               end_hour = max_day_minutes;
            }
            if(dur>0) {
               value = end_hour-start_hour;
            }
            else {
               value = hours*60;
            }
            if(value>max_day_minutes)
                value = max_day_minutes;
            ret = ret+value;
         }
         break;
       }
       case G_DATE_FRIDAY: {
         if((friday) && (t_mark<0) && (t_mark_tsk<0)) {
            start_hour = calendars_get_starting_hour (index, CAL_FRIDAY, data);
            end_hour = calendars_get_ending_hour (index, CAL_FRIDAY, data);
            if((start_hour<0) || (end_hour<0)) {
               start_hour = 0; /* simplyest */
               end_hour = max_day_minutes;
            }
            if(dur>0) {
               value = end_hour-start_hour;
            }
            else {
               value = hours*60;
            }
            if(value>max_day_minutes)
                value = max_day_minutes;
            ret = ret+value;
         }
         break;
       }
       case G_DATE_SATURDAY: {
         if((saturday) && (t_mark<0) && (t_mark_tsk<0)) {
            start_hour = calendars_get_starting_hour (index, CAL_SATURDAY, data);
            end_hour = calendars_get_ending_hour (index, CAL_SATURDAY, data);
            if((start_hour<0) || (end_hour<0)) {
               start_hour = 0; /* simplyest */
               end_hour = max_day_minutes;
            }
            if(dur>0) {
               value = end_hour-start_hour;
            }
            else {
               value = hours*60;
            }
            if(value>max_day_minutes)
                value = max_day_minutes;
            ret = ret+value;
         }
         break;
       }
       case G_DATE_SUNDAY: {
         if((sunday) && (t_mark<0) && (t_mark_tsk<0)) {
            start_hour = calendars_get_starting_hour (index, CAL_SUNDAY, data);
            end_hour = calendars_get_ending_hour (index, CAL_SUNDAY, data);
            if((start_hour<0) || (end_hour<0)) {
               start_hour = 0; /* simplyest */
               end_hour = max_day_minutes;
            }
            if(dur>0) {
               value = end_hour-start_hour;
            }
            else {
               value = hours*60;
            }
            if(value>max_day_minutes)
                value = max_day_minutes;
            ret = ret+value;
         }
         break;
       }
       case G_DATE_BAD_WEEKDAY: {
          printf("* Bad date ! *\n");
       }
     }/* end switch */

     duration++;
     g_date_add_days (&date, 1);
  }/* wend */

  return ret;
}

/******************************************
  get month of year in human
  readable form
  the string must be freed
******************************************/
gchar *misc_get_month_of_year (gint month )
{
  gchar *tmpMonth;

    switch(month) {
      case 1:{
        tmpMonth = g_strdup_printf ("%s", _("January"));
        break;
      }
      case 2:{
        tmpMonth = g_strdup_printf ("%s", _("February"));
        break;
      }
      case 3:{
        tmpMonth = g_strdup_printf ("%s", _("March"));
        break;
      }
      case 4:{
        tmpMonth = g_strdup_printf ("%s", _("April"));
        break;
      }
      case 5:{
        tmpMonth = g_strdup_printf ("%s", _("May"));
        break;
      }
      case 6:{
        tmpMonth = g_strdup_printf ("%s", _("June"));
        break;
      }
      case 7:{
        tmpMonth = g_strdup_printf ("%s", _("July"));
        break;
      }
      case 8:{
        tmpMonth = g_strdup_printf ("%s", _("August"));
        break;
      }
      case 9:{
        tmpMonth = g_strdup_printf ("%s", _("September"));
        break;
      }
      case 10:{
        tmpMonth = g_strdup_printf ("%s", _("October"));
        break;
      }
      case 11:{
        tmpMonth = g_strdup_printf ("%s", _("November"));
        break;
      }
      case 12:{
        tmpMonth = g_strdup_printf ("%s", _("December"));
        break;
      }
    }/* end switch */

  return tmpMonth;
}

/******************************************
  get short day representation
  of week in human
  readable form - string must be freed
******************************************/
gchar *misc_get_short_day_of_week (gint dd)
{
   gchar *tmpDay;

       switch(dd) {
          case G_DATE_MONDAY: {
              tmpDay = g_strdup_printf (_("Mo") );
            break;
          }
          case G_DATE_TUESDAY: {
              tmpDay = g_strdup_printf (_("Tu") );
            break;
          }
          case G_DATE_WEDNESDAY: {
              tmpDay = g_strdup_printf (_("We") );
            break;
          }
          case G_DATE_THURSDAY: {
              tmpDay = g_strdup_printf (_("Th") );
            break;
          }
          case G_DATE_FRIDAY: {
              tmpDay = g_strdup_printf (_("Fr") );
            break;
          }
          case G_DATE_SATURDAY: {
              tmpDay = g_strdup_printf (_("Sa") );
            break;
          }
          case G_DATE_SUNDAY: {
              tmpDay = g_strdup_printf (_("Su") );
            break;
          }
          case G_DATE_BAD_WEEKDAY: {
              tmpDay = g_strdup_printf (_("??") );
            break;
          }
       }/* end switch */

   return tmpDay;
}

/*********************************************
  PUBLIC : remove non working days since 
  a start date and a known duration
  TODO use calendar for each ressource 
********************************************/
gint misc_get_calendar_day_from_glib (gint wd)
{
  gint ret = 0;

  switch(wd) {
       case G_DATE_MONDAY: {
            ret = 0;
         break;
       }
       case G_DATE_TUESDAY: {
            ret = 1;
         break;
       }
       case G_DATE_WEDNESDAY: {
            ret = 2;
         break;
       }
       case G_DATE_THURSDAY: {
            ret = 3;
         break;
       }
       case G_DATE_FRIDAY: {
            ret = 4;
         break;
       }
       case G_DATE_SATURDAY: {
            ret = 5;
         break;
       }
       case G_DATE_SUNDAY: {
            ret = 6;
       }
  }/* end switch */
 
  return ret;
}

/********************************
  remove non desired char
  arguments :  p, gchar string
        modified in place
      ch char
  from : https://stackoverflow.com/questions/5457608/how-to-remove-the-character-at-a-given-index-from-a-string-in-c 
  thanks to author : Orkun Tokdemir
********************************/
gchar *misc_erase_c (gchar *p, int ch)
{
    gchar *ptr;

    while (ptr = strchr(p, ch))
        strcpy(ptr, ptr + 1);

    return p;
}

/************************************
  General exit with failure after
  an issue with glade
***********************************/
void misc_halt_after_glade_failure (APP_data *data)
{
     printf ("* CRITICAL : can't load glade UI file, quit application ! *\n");
     g_application_quit (G_APPLICATION(data->app));
}
