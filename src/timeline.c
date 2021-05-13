/**********************************
  timeline.c module
  functions, constants for
  displaying project on
  a virtual timeline
**********************************/

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
#include <goocanvas.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <cairo-svg.h>
#include <cairo-pdf.h>
#include "support.h"
#include "misc.h"
#include "cursor.h"
#include "links.h"
#include "tasks.h"
#include "tasksutils.h"
#include "assign.h"
#include "files.h"
#include "timeline.h"

/* LOCAL global variables */

static gint day_offset = 0, cDay = -1, cMonth = -1, cYear = -1;
static GooCanvasItem *cursor = NULL, *selBox = NULL;
static gdouble ypos_ruler =0, canvas_min_x = 0, canvas_max_x = 0, canvas_max_y = 0, zoom_factor = 1, newWidth;
static gboolean fSelect = FALSE;
static gboolean fResize = FALSE;

/*****************************
  PUBLIC : store zoom  value
******************************/
void timeline_store_zoom (gdouble z)
{
  if(z>=0)
     zoom_factor = z;
}

/**************************************
  LOCAL : draw week-ends
**************************************/
void timeline_draw_week_ends (GooCanvasItem *root, APP_data *data)
{
   gint i, ret = 0;
   gdouble x_val;

   GooCanvasItem *WE = goo_canvas_group_new (root, NULL);
   data->week_endsTL = WE;
   /* draw text - weeks */
   i = 0;
   while(ret==0) {/* 521 = 10 years 52 = 1 year*/
      /* we draw week_ends - not in ruler */
      x_val = i*(zoom_factor*TIMELINE_CALENDAR_DAY_WIDTH*7 )+(5*zoom_factor*TIMELINE_CALENDAR_DAY_WIDTH);
      if(x_val>TIMELINE_VIEW_MAX_WIDTH)
         ret = -1;
      GooCanvasItem *week_ends = goo_canvas_rect_new (WE, x_val, TIMELINE_CALENDAR_HEADER_HEIGHT, 
                                (TIMELINE_CALENDAR_DAY_WIDTH*2*zoom_factor), 
                                TIMELINE_VIEW_MAX_HEIGHT-TIMELINE_CALENDAR_HEADER_HEIGHT,
                                "line-width", 0.0, "radius-x", 0.0, "radius-y", 0.0,
                                "stroke-color", "#E9EEEE", "fill-color", "#E9EEEE",
                                 NULL);
      i++;
     /* same to allow right-click for various operations */
    // g_signal_connect ( (gpointer)week_ends, "button_press_event", G_CALLBACK ( on_rect_back_button_press), data);
   }/* wend  */
}

/*************************************
 PROTECTED : callbacks for righ-click
*************************************/
static void timeline_show_cursor (gint offset, APP_data *data)
{
  cursor = goo_canvas_polyline_new (data->ruler, TRUE, 3, 
             offset*zoom_factor*TIMELINE_CALENDAR_DAY_WIDTH+((TIMELINE_CALENDAR_DAY_WIDTH/2)*zoom_factor), 
             ypos_ruler+TIMELINE_CALENDAR_HEADER_HEIGHT-14, /* close path, nb points, coordos */
             offset*zoom_factor*TIMELINE_CALENDAR_DAY_WIDTH+((TIMELINE_CALENDAR_DAY_WIDTH/2)*zoom_factor)+6,         
             ypos_ruler+TIMELINE_CALENDAR_HEADER_HEIGHT,
             offset*zoom_factor*TIMELINE_CALENDAR_DAY_WIDTH+((TIMELINE_CALENDAR_DAY_WIDTH/2)*zoom_factor)-6,  
             ypos_ruler+TIMELINE_CALENDAR_HEADER_HEIGHT,
             "start-arrow", FALSE,
             "line-width", 2.0, "stroke-color", "#9EB6CD",
             NULL);

  goo_canvas_item_raise (cursor, NULL);
}

/***********************************
  PROTECTED : cb when gtkadjustment
  has changed
***********************************/
static void on_vertical_adjustment_value_changed (GtkAdjustment *adjustment, APP_data *data)
{
  timeline_hide_cursor (data);
  timeline_remove_ruler (data);
  ypos_ruler = gtk_adjustment_get_value (GTK_ADJUSTMENT(adjustment));
  timeline_draw_calendar_ruler (ypos_ruler, data);
  timeline_show_cursor (day_offset, data);
}

/*****************************
  PUBLIC : reacts to zoom
  combobox
******************************/
void on_combo_TL_zoom_changed (GtkComboBox *widget, APP_data *data)
{
  gint active; 
  gdouble value;
  GtkAdjustment *vadj;
  vadj = gtk_scrolled_window_get_vadjustment (
                    GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowTimeline")));

  active = gtk_combo_box_get_active (GTK_COMBO_BOX(widget));
  value = ABS(2*(active==0)+1.5*(active==1)+1*(active==2)+0.75*(active==3)+0.5*(active==4));/* old school, no ? */
  if(value!=zoom_factor) {
      timeline_store_zoom (value);
      /* we redraw all timeline */
      timeline_remove_ruler (data);
      value = timeline_get_ypos (data);
      goo_canvas_item_remove (data->week_endsTL);
      data->week_endsTL = NULL;
      timeline_remove_all_tasks (data);
      timeline_draw_week_ends (goo_canvas_get_root_item (GOO_CANVAS (data->canvasTL)), data);
      timeline_draw_all_tasks (data);
      timeline_draw_calendar_ruler (value, data);
  }
}

/******************************
  PUBLIC : get current ypos
  for navigation 
*******************************/
gdouble timeline_get_ypos (APP_data *data)
{
  GtkAdjustment *vadj;

  vadj = gtk_scrolled_window_get_vadjustment (
                    GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowTimeline")));
//  printf("ajustement vaut : %.2f \n", gtk_adjustment_get_value(GTK_ADJUSTMENT(vadj)));
  ypos_ruler = gtk_adjustment_get_value (GTK_ADJUSTMENT(vadj));
  return ypos_ruler;
}

/*****************************************************
  PUBLIC : store current date for navigation 
  arguments : day, month, year
  datas are stored in cDay, cMonth, cYear static vars
*****************************************************/
void timeline_store_date (gint dd, gint mm, gint yy)
{
   cDay = dd;
   cMonth = mm;
   cYear = yy;
}

/******************************************************
  PUBLIC : get current date for navigation 
  arguments : day, month, year
  datas are stored in cDay, cMonth, cYear static vars
*****************************************************/
void timeline_get_date (gint *dd, gint *mm, gint *yy)
{
   *dd = cDay;
   *mm = cMonth;
   *yy = cYear;
}

/******************************
  PUBLIC : reset stored dates
  datas are set to -1
******************************/
void timeline_reset_store_date (void)
{
   cDay = -1;
   cMonth = -1;
   cYear = -1;
}

/********************************
  LOCAL : test if there is true
  stored dates
********************************/
static gboolean test_stored_dates (void)
{
   if((cDay<0) || (cMonth<0) || (cYear<0))
     return FALSE;
   else
     return TRUE;
}

/************************
  PUBLIC : get extremas
************************/
gdouble timeline_get_max_x (void)
{
  gdouble tmp = canvas_max_x+(7*TIMELINE_CALENDAR_DAY_WIDTH);
  if(tmp>TIMELINE_VIEW_MAX_WIDTH)
    tmp = TIMELINE_VIEW_MAX_WIDTH;
  return tmp;
}

gdouble timeline_get_max_y (void)
{
  gdouble tmp = canvas_max_y+TIMELINE_AVATAR_HEIGHT;
  if(tmp>TIMELINE_VIEW_MAX_HEIGHT)
     tmp = TIMELINE_VIEW_MAX_HEIGHT;
  return tmp;
}

/********************************************
  returns a pointer on a line dashing style
  ####  ####  ####
  from old Ella code
TODO free it ????
*******************************************/
GooCanvasLineDash *timeline_get_line_dashed (void)
{
    static const double dashed1[] = {4.0, 2.0};
    static int len1  = sizeof(dashed1) / sizeof(dashed1[0]);
    GooCanvasLineDash *style_dashed;
    style_dashed = goo_canvas_line_dash_newv (len1, (gdouble *)dashed1);
    return style_dashed;
}

/***************************************
  function to redraw the selection box ; 
  - coordinates as argument,
  - returns a goocanvasitem*
  from old Ella code
************************************/
GooCanvasItem *timeline_selection_box (GooCanvasItem *ancestor, gdouble x1t, gdouble y1t, gdouble x2t, gdouble y2t)
{
   GooCanvasItem* temp = NULL;
   GooCanvasLineDash* style_dashed;
   gdouble t;

   style_dashed = timeline_get_line_dashed ();

   /* check corners coordinates */
   if (x2t<x1t) {
       	t = x1t;
        x1t = x2t;
        x2t = t;
   }
   if (y2t<y1t) {
	t = y1t;
        y1t = y2t;
        y2t = t;
   } 	

   temp = goo_canvas_rect_new (ancestor, x1t-1, y1t-1, x2t-x1t+2, y2t-y1t+2,
                                "line-width", 2.0, "radius-x", 2.0, "radius-y", 2.0,
                                "stroke-color", "orange",
                                "line-dash", style_dashed,
                                NULL);
   return temp;
}

/******************************
  remove selection box
******************************/
void timeline_remove_selection_box (APP_data *data)
{
  if(selBox != NULL) 
       goo_canvas_item_remove (selBox);
  selBox = NULL;
}



void timeline_hide_cursor (APP_data *data)
{
  if(cursor != NULL)
     goo_canvas_item_remove (cursor);
  cursor = NULL;
}

/*************************************
  CB : add a new task 
**************************************/
static void on_menu1BackTimelineNew (GtkMenuItem *menuitem, APP_data *data)
{
  gint day, month, year;
  GDate *date, start_date, end_date;

  /* get date stored for display */
  timeline_get_date (&day, &month, &year);
  date = g_date_new_dmy (day, month, year);
  /* we clamp to monday of the current week */
  while(g_date_get_weekday (date)!= G_DATE_MONDAY) {
     g_date_subtract_days (date, 1);
  }
  /* now, we go ahead according to last mouse click */
  g_date_add_days (date, day_offset);
  /* if the cursor position is after project's ending date, we display an error dialog */
  g_date_set_dmy (&end_date, data->properties.end_nthDay, data->properties.end_nthMonth, data->properties.end_nthYear);
  g_date_set_dmy (&start_date, data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
  if(g_date_compare (date, &end_date)>0  || g_date_compare (date, &start_date)<0) {
        misc_ErrorDialog (data->appWindow, _("<b>Error!</b>\n\nYou can't place a new task <b>outside</b>project's starting or ending date.\nYou can change this starting and ending date in <i>project's properties</i>.\nPlease, have a look on <b>Project>Properties</b> menu."));
  }
  else {     
     tasks_new (date, data);
  }/*elseif */
}

/*************************************
  CB : add a new milestone 
**************************************/
static void on_menu1BackTimeNewMile (GtkMenuItem *menuitem, APP_data *data)
{
  gint day, month, year;
  GDate *date, start_date, end_date;

  /* get date stored for display */
  timeline_get_date (&day, &month, &year);
  date = g_date_new_dmy (day, month, year);
  /* we clamp to monday of the current week */
  while(g_date_get_weekday (date)!= G_DATE_MONDAY ) {
     g_date_subtract_days (date, 1);
  }
  /* now, we go ahead according to last mouse click */
  g_date_add_days (date, day_offset);
  /* if the cursor position is after project's ending date, we display an error dialog */
  g_date_set_dmy (&end_date, data->properties.end_nthDay, data->properties.end_nthMonth, data->properties.end_nthYear);
  g_date_set_dmy (&start_date, data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
  if(g_date_compare (date, &end_date)>0  || g_date_compare (date, &start_date)<0) {
        misc_ErrorDialog (data->appWindow, _("<b>Error!</b>\n\nYou can't place a new milestone <b>outside</b>project's starting or ending date.\nYou can change this starting and ending date in <i>project's properties</i>.\nPlease, have a look on <b>Project>Properties</b> menu."));
  }
  else {
     tasks_milestone_new (date, data);
  }/*elseif */
}

/***********************************
  CB : edit/modify task or group
************************************/
static void on_menu1TimelineEdit (GtkMenuItem *menuitem, APP_data *data)
{
  tasks_modify (data);
}

/*************************************
  CB : move visually up a task
**************************************/
static void on_menuTimelineMoveUp (GtkMenuItem *menuitem, APP_data *data)
{
  tasks_move (data->curTasks, data->curTasks-1, TRUE, TRUE, data);
}

/*************************************
  CB : move visually down a task
**************************************/
static void on_menuTimelineMoveDown (GtkMenuItem *menuitem, APP_data *data)
{
  tasks_move (data->curTasks, data->curTasks+1, TRUE, TRUE, data);
}

/*************************************
  CB : move one day backward a task
**************************************/
static void on_menu1TimelineBackward (GtkMenuItem *menuitem, APP_data *data)
{
  tasks_move_backward (1, data->curTasks, data);
}

/*************************************
  CB : move one day forward a task
**************************************/
static void on_menu1TimelineForward (GtkMenuItem *menuitem, APP_data *data)
{
  tasks_move_forward (1, data->curTasks, data);
}

/********************************************
  CB : increase duration of a task
*********************************************/
static void on_menu1TimelineIncrDur (GtkMenuItem *menuitem, APP_data *data)
{
  tasks_change_duration (1, data->curTasks, data);
}

/*************************************
  CB : reduce duration of a task
**************************************/
static void on_menu1TimelineDecrDur (GtkMenuItem *menuitem, APP_data *data)
{
  tasks_change_duration (-1, data->curTasks, data);
}

/*******************************
  PUBLIC : cb when a day is
  selected
******************************/
void timeshift_calendars_day_selected (GtkCalendar *calendar, APP_data *data)
{
  gint cmp_date; 
  guint day, month, year;
  GDate date_start, new_date;
  gchar *tmpStr, *newDate;

  GtkWidget *labelBtn = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelChangeDate"));
  GtkWidget *labelShift = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelShift"));

  gtk_calendar_get_date (calendar, &year, &month, &day);

  /* we get current dates limits from properties */

  g_date_set_parse (&date_start, data->properties.start_date );
  g_date_set_dmy (&new_date, day, month+1, year);
  newDate = misc_convert_date_to_str (day, month+1, year);
  /* how many days shift ? */
  cmp_date = g_date_days_between (&date_start, &new_date);
  /* we display new value */
  if(cmp_date == 0) {
     tmpStr = g_strdup_printf ("%s", _("No shift"));
  }
  else {
    if(cmp_date<0)
      tmpStr = g_strdup_printf (_("%d day(s) backward"), ABS (cmp_date));
    else
      tmpStr = g_strdup_printf (_("%d day(s) forward"), cmp_date);
  }
  gtk_label_set_text (GTK_LABEL(labelShift), tmpStr);
  gtk_label_set_text (GTK_LABEL(labelBtn), newDate);
  g_free (tmpStr);
  g_free (newDate);
}

/*************************************
  PUBLIC : display time shift dialog
  flag is TRUE when dialog is used
  for group shift, and position>=0
**************************************/
GtkWidget *timeline_create_timeshift_dialog (gboolean groupMode, gint position, APP_data *data)
{
  GtkWidget *dialog, *btnDate, *labelShift, *labelBtn, *labelCurDate, *calendar1, *labelL2, *hbar;
  /* get datas from xml Glade file - CRITICAL : only in activate phase !!!! */
  GtkBuilder *builder = NULL;
  builder = gtk_builder_new ();
  GError *err = NULL;
  GDate current_date;
  tasks_data temp;
  gchar *groupDate, *curDate; 

  if(!gtk_builder_add_from_file (builder, find_ui_file ("timeshift.ui"), &err)) {
     g_error_free (err);
     misc_halt_after_glade_failure (data);
  }
  /* Get the dialog from the glade file. */
  gtk_builder_connect_signals (builder, data);
  dialog = GTK_WIDGET(gtk_builder_get_object (builder, "dialogTimeShift"));
  data->tmpBuilder = builder;
  /* header bar */

  hbar = gtk_header_bar_new ();
  gtk_widget_show (hbar);
  gtk_header_bar_set_title (GTK_HEADER_BAR (hbar), _("Date shift"));
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (hbar), _("Define a timeshift for whole project or current group."));
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (hbar), TRUE);
  gtk_window_set_titlebar (GTK_WINDOW(dialog), hbar);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  
  gtk_header_bar_pack_start (GTK_HEADER_BAR (hbar), box);

  GtkWidget *icon =  gtk_image_new_from_icon_name  ("x-office-calendar", GTK_ICON_SIZE_DIALOG);
  gtk_container_add (GTK_CONTAINER (box), icon);
  gtk_widget_show (box);
  gtk_widget_show (icon);

  /* update widgets */
  labelCurDate = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelCurDate"));
  btnDate = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonChangeDate"));
  labelBtn = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelChangeDate"));
  labelShift = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelShift"));
  calendar1 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "calendar1"));
  labelL2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelL2"));
  g_date_set_time_t (&current_date, time (NULL)); /* date of today */
  gtk_calendar_select_month (GTK_CALENDAR(calendar1), g_date_get_month (&current_date)-1, g_date_get_year (&current_date));
  gtk_calendar_select_day (GTK_CALENDAR(calendar1), g_date_get_day (&current_date));

  if(groupMode) {
    gtk_label_set_markup (GTK_LABEL(labelL2), _("<i>Define the new <b>starting date</b> of current group.</i>"));
    /* get somee datas */
    tasks_get_ressource_datas (position, &temp, data);
    groupDate = misc_convert_date_to_str (temp.start_nthDay, temp.start_nthMonth, temp.start_nthYear);
    gtk_label_set_text (GTK_LABEL(labelCurDate), groupDate);
    g_free (groupDate);
  }
  else {
    gtk_label_set_text (GTK_LABEL(labelCurDate), data->properties.start_date);
  }
  curDate = misc_convert_date_to_str (g_date_get_day(&current_date), g_date_get_month(&current_date), 
                                      g_date_get_year(&current_date));
  gtk_label_set_text (GTK_LABEL(labelBtn), curDate);
  g_free (curDate);
  return dialog;
}

/****************************************** 
 LOCAL :  PopUp menu after a right-click. 
 ******************************************/
static GtkWidget *create_menu_timeline (GtkWidget *win, APP_data *data)
{
  GtkWidget *menu1Timeline, *menu1TimelineEdit, *menu1TimelineMoveUp, *menu1TimelineMoveDown; 
  GtkWidget *menu1TimelineBackward, *menu1TimelineForward, *menu1TimelineIncrDur, *menu1TimelineDecrDur;
  GtkWidget *menuCancelTimeline, *separator1, *separator2, *separator3, *separator4;
  gint type, durMode;

  menu1Timeline = gtk_menu_new (); 

  menu1TimelineEdit = gtk_menu_item_new_with_mnemonic (_("_Modify ... "));
  gtk_container_add (GTK_CONTAINER (menu1Timeline), menu1TimelineEdit);

  separator2 = gtk_separator_menu_item_new ();
  gtk_container_add (GTK_CONTAINER (menu1Timeline), separator2);
  gtk_widget_set_sensitive (separator2, FALSE);

  menu1TimelineMoveUp= gtk_menu_item_new_with_mnemonic (_("Move _Up "));
  gtk_container_add (GTK_CONTAINER (menu1Timeline), menu1TimelineMoveUp);

  menu1TimelineMoveDown= gtk_menu_item_new_with_mnemonic (_("Move _down "));
  gtk_container_add (GTK_CONTAINER (menu1Timeline), menu1TimelineMoveDown);

  separator3 = gtk_separator_menu_item_new ();
  gtk_container_add (GTK_CONTAINER (menu1Timeline), separator3);
  gtk_widget_set_sensitive (separator3, FALSE);

  menu1TimelineBackward= gtk_menu_item_new_with_mnemonic (_("Move one day _backward "));
  gtk_container_add (GTK_CONTAINER (menu1Timeline), menu1TimelineBackward);

  menu1TimelineForward= gtk_menu_item_new_with_mnemonic (_("Move one day _forward "));
  gtk_container_add (GTK_CONTAINER (menu1Timeline), menu1TimelineForward);

  separator4 = gtk_separator_menu_item_new ();
  gtk_container_add (GTK_CONTAINER (menu1Timeline), separator4);
  gtk_widget_set_sensitive (separator4, FALSE);

  menu1TimelineIncrDur = gtk_menu_item_new_with_mnemonic (_("Increase duration by one day "));
  gtk_container_add (GTK_CONTAINER (menu1Timeline), menu1TimelineIncrDur);

  menu1TimelineDecrDur = gtk_menu_item_new_with_mnemonic (_("Reduce duration by one day "));
  gtk_container_add (GTK_CONTAINER (menu1Timeline), menu1TimelineDecrDur);

  separator1 = gtk_separator_menu_item_new ();
  gtk_container_add (GTK_CONTAINER (menu1Timeline), separator1);
  gtk_widget_set_sensitive (separator1, FALSE);

  menuCancelTimeline = gtk_menu_item_new_with_mnemonic (_("C_ancel"));
  gtk_container_add (GTK_CONTAINER (menu1Timeline), menuCancelTimeline);

  gtk_widget_show_all (GTK_WIDGET(menu1Timeline));
  /* tune menus according to current position */
  if(data->curTasks<=0) {
     gtk_widget_set_sensitive (menu1TimelineMoveUp, FALSE);
  } 
  if(data->curTasks>=g_list_length (data->tasksList)-1 ) {
     gtk_widget_set_sensitive (menu1TimelineMoveDown, FALSE);
  } 
  /* tune menus according to task type */
  type = tasks_get_type (data->curTasks, data);
  durMode = tasks_get_duration (data->curTasks, data);
  if(type!=TASKS_TYPE_TASK) {
     gtk_widget_set_sensitive (menu1TimelineIncrDur, FALSE);
     gtk_widget_set_sensitive (menu1TimelineDecrDur, FALSE);
  }
  if(type==TASKS_TYPE_GROUP) {
     gtk_widget_set_sensitive (menu1TimelineBackward, FALSE);
     gtk_widget_set_sensitive (menu1TimelineForward, FALSE);
  }
  if(type==TASKS_TYPE_TASK) {
     if(durMode == TASKS_FIXED) {
        gtk_widget_set_sensitive (menu1TimelineBackward, FALSE);
        gtk_widget_set_sensitive (menu1TimelineForward, FALSE);
     }
     /* TASKS_DEADLINE */
     if(durMode == TASKS_DEADLINE) {
        if(!tasks_test_increase_duration (data->curTasks, 1, data)) {
            gtk_widget_set_sensitive (menu1TimelineForward, FALSE);
            gtk_widget_set_sensitive (menu1TimelineIncrDur, FALSE);
        }
     }
     /* TASKS AFTER */
     if(durMode == TASKS_AFTER) {
        if(!tasks_test_decrease_dates (data->curTasks, 1, data)) {
            gtk_widget_set_sensitive (menu1TimelineBackward, FALSE);
        }
     }
  }
  
  g_signal_connect ((gpointer) menu1TimelineEdit, "activate",
                    G_CALLBACK (on_menu1TimelineEdit),
                    data);
  g_signal_connect ((gpointer) menu1TimelineMoveUp, "activate",
                    G_CALLBACK (on_menuTimelineMoveUp),
                    data);
  g_signal_connect ((gpointer) menu1TimelineMoveDown, "activate",
                    G_CALLBACK (on_menuTimelineMoveDown),
                    data);

  g_signal_connect ((gpointer) menu1TimelineBackward, "activate",
                    G_CALLBACK (on_menu1TimelineBackward),
                    data);
  g_signal_connect ((gpointer) menu1TimelineForward, "activate",
                    G_CALLBACK (on_menu1TimelineForward),
                    data);
  g_signal_connect ((gpointer) menu1TimelineIncrDur, "activate",
                    G_CALLBACK (on_menu1TimelineIncrDur),
                    data);
  g_signal_connect ((gpointer) menu1TimelineDecrDur, "activate",
                    G_CALLBACK (on_menu1TimelineDecrDur),
                    data);

  return menu1Timeline;
}

/****************************************** 
 LOCAL :  PopUp menu after a right-click. 
 ******************************************/
static GtkWidget *create_menu_back_timeline (GtkWidget *win, APP_data *data)
{
  GtkWidget *menu1BackTimeline, *menu1BackTimelineEdit;
  GtkWidget *menu1BackTimelineMile, *menuCancelBackTimeline, *separator2;

  menu1BackTimeline = gtk_menu_new (); 

  menu1BackTimelineEdit = gtk_menu_item_new_with_mnemonic (_("_New task ... "));
  gtk_container_add (GTK_CONTAINER (menu1BackTimeline), menu1BackTimelineEdit);

  menu1BackTimelineMile = gtk_menu_item_new_with_mnemonic (_("_New milestone ... "));
  gtk_container_add (GTK_CONTAINER (menu1BackTimeline), menu1BackTimelineMile);

  separator2 = gtk_separator_menu_item_new ();
  gtk_container_add (GTK_CONTAINER (menu1BackTimeline), separator2);
  gtk_widget_set_sensitive (separator2, FALSE);

  menuCancelBackTimeline = gtk_menu_item_new_with_mnemonic (_("C_ancel"));
  gtk_container_add (GTK_CONTAINER (menu1BackTimeline), menuCancelBackTimeline);

  gtk_widget_show_all (GTK_WIDGET(menu1BackTimeline));
  /* signals */
  g_signal_connect ((gpointer) menu1BackTimelineEdit, "activate",
                    G_CALLBACK (on_menu1BackTimelineNew),
                    data);
  g_signal_connect ((gpointer) menu1BackTimelineMile, "activate",
                    G_CALLBACK (on_menu1BackTimeNewMile),
                    data);

  return menu1BackTimeline;
}

/*******************************************
 LOCAL : given title of an item get and id 
 ****************************************/
static gint timeline_get_item_id (GooCanvasItem *item)
{
  gint id = -1;
  gchar *tmpStr;

  g_object_get (item, "title", &tmpStr, NULL);/* we get a string representation of unique IDentifier */
  if(tmpStr) {
      id = atoi (tmpStr);
      g_free (tmpStr);     
  }
  return id;
}

/*****************************************
 LOCAL : now we compute the item's index 
 ****************************************/
static gint timeline_get_item_index (gint id, APP_data *data)
{
   gint i, ret = -1;
   GList *l;
   tasks_data *tmp_tsk_datas;

   i = 0;
   while((i < g_list_length (data->tasksList)) && (ret < 0)) {
      l = g_list_nth (data->tasksList, i);
      tmp_tsk_datas = (tasks_data *)l->data;  
      if(id == tmp_tsk_datas->id) {
         ret = i;		     
      }
      i++;
   }/* wend */
  return ret;
}

/**********************************
 * LOCAL : Mouse move on areas for
   tasks
 * *******************************/
static gboolean on_timeline_mouse_move (GooCanvasItem *item,
		      GooCanvasItem  *target, GdkEventButton *event,
		      APP_data *data)
{
  gdouble xmem;
  /* by chance, once mouse pointer leaves selected object, the events happens */
   if(fSelect) {
      day_offset = (gint) event->x/(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor);
      timeline_hide_cursor (data);
      if(!fResize) {/* move tasks */
         g_object_set (selBox, "x", event->x, NULL);
      }
      else {/* change task duration */
         g_object_get (selBox, "x", &xmem, NULL);
         newWidth = event->x-xmem;
         if(newWidth<TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor) {
             newWidth = TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor;
         }
         g_object_set (selBox, "width", newWidth, NULL);
      }
      timeline_show_cursor (day_offset, data);
   }/* if select */

   return TRUE;
}

/**************************************
 * Mouse pointer enters on areas of
   a task object 
 * ************************************/
static gboolean on_timeline_enter_task (GooCanvasItem *item,
               GooCanvasItem *target_item,
               GdkEvent *event, APP_data *data)
{
  gint id, type, ret;

  id = timeline_get_item_id (item);
  if(id>=0) {
     ret = timeline_get_item_index (id, data);
     type = tasks_get_type (ret, data);
     if(type!=TASKS_TYPE_GROUP && !fSelect) {
        cursor_pointer_hand (data->appWindow);
     }
  }
  return TRUE;
}

/**************************************
 * Mouse pointer leaves on areas of
   a task object 
 * ************************************/
static gboolean on_timeline_leave_task (GooCanvasItem *item,
               GooCanvasItem *target_item,
               GdkEvent *event, APP_data *data)
{
  if(!fSelect)
    cursor_normal (data->appWindow);
  return TRUE;
}

/************************************
  PROTECTED : various warnings and
  error messages
************************************/
static void msg_error_deadline (APP_data *data)
{
  gchar *msg;

  msg = g_strdup_printf ("%s", _("Sorry !\nYou <b>can't move a task with a deadline</b> at this position. Please, check task dates or modify its duration."));
  misc_ErrorDialog (data->appWindow, msg);
  g_free (msg);
}

static void msg_error_after (APP_data *data)
{
  gchar *msg;
  msg = g_strdup_printf ("%s", _("Sorry !\nYou <b>can't move a task that is in <u>not before</u> mode</b> at this position. Please, check task dates or modify its duration."));
  misc_ErrorDialog (data->appWindow, msg);
  g_free (msg);
}

static void msg_error_fixed (APP_data *data)
{
  gchar *msg;
  msg = g_strdup_printf ("%s", _("Sorry !\nYou <b>can't move a fixed task</b> along the timeline.\nYou can change fixed date or\nmodify task duration mode."));
  misc_ErrorDialog (data->appWindow, msg);
  g_free (msg);
}

/**************************************
 * Mouse release on areas for
   tasks never fired for tasks objects
 * ************************************/
static gboolean on_timeline_mouse_release (GooCanvasItem *item, GooCanvasItem *target, GdkEventButton *event,
		                           APP_data *data)
{
  GList *l;
  GDate current_date, proj_date, end_date;
  gint days =0, ret, id = -1, day, month, year, duration, newDuration, s_links, retour;
  gchar *msg;
  gboolean fErr = FALSE;
  tasks_data temp;

  /* now we convert x coordinates en day offset */
  timeline_hide_cursor (data);
  day_offset = (gint) event->x/(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor);
  timeline_show_cursor (day_offset, data);
  timeline_remove_selection_box (data); 
  timeline_get_date (&day, &month, &year);
  if(fSelect) {
     fSelect = FALSE;
     cursor_normal (data->appWindow);
     /* and check if new start date is allowed by project and links */
     l = g_list_nth (data->tasksList, data->curTasks);
     tasks_data *tmp_tasks_datas;
     tmp_tasks_datas = (tasks_data *)l->data;
     /* we get the current starting date for current task */
     g_date_set_dmy (&current_date,  
                     tmp_tasks_datas->start_nthDay, tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
     g_date_set_dmy (&end_date, data->properties.end_nthDay, data->properties.end_nthMonth, data->properties.end_nthYear);
     /* we compute the date for the left date on display */
     g_date_set_dmy (&proj_date, day, month, year);/* true current left side date for current "page" */
     /* we clamp to monday of the week */
     while(g_date_get_weekday (&proj_date)!=G_DATE_MONDAY) {
       g_date_subtract_days (&proj_date, 1);
     }
     /* how much days between first monday and current task ? */
     days = g_date_days_between (&proj_date, &current_date);
     duration = (gint) newWidth/(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor);/* TODO +1 ou pas ??? new potential duration in resize mode */

 //    printf ("au lâché, la tâche commence à %d j du début, l'offset demandé est %d j et New Width =%.2f duration =%d\n", 
     //   days, day_offset, newWidth, duration);
     /* check if mouse drag is allowed */
     if(!fResize) {
	 if(tmp_tasks_datas->durationMode !=TASKS_FIXED) {
	    if(day_offset>days) {
		/* test for tasks with a deadline - checked if DEADLINE duration mode inside function */
		if(!tasks_test_increase_duration (data->curTasks, day_offset-days, data)) {
		     msg_error_deadline (data);
		}
		else {
	           tasks_move_forward (day_offset-days, data->curTasks, data);
                   /* we get MODIFIED values */
                   tasks_get_ressource_datas (data->curTasks, &temp, data);
                   /* we must get new starting date ! */
                   g_date_set_dmy (&current_date, temp.start_nthDay, temp.start_nthMonth, temp.start_nthYear);
                   s_links = links_check_task_successors (temp.id, data);
		   if(s_links>=0) {
		      retour = tasks_compute_dates_for_all_linked_task (temp.id, temp.durationMode, 
		                 temp.days, data->curTasks, &current_date, 0, s_links, TRUE, data->appWindow, data);
		      timeline_remove_all_tasks (data);/* YES, for other tasks */
		      timeline_draw_all_tasks (data);
		   }/* endif s_links */
                }/* elseif */
	    }
	    else {
	       /* test for tasks with BEFORE limit - checked if AFTER/BEFORE duration mode inside function*/
	       if(!tasks_test_decrease_dates (data->curTasks, day_offset-days, data)) {
		   msg_error_after (data);
	       }
	       else {printf ("backward day offset =%d days=%d  \n", day_offset, days);
		  tasks_move_backward (days-day_offset, data->curTasks, data);
                  /* we get MODIFIED values */
                  tasks_get_ressource_datas (data->curTasks, &temp, data);
                  /* we must get new starting date ! */
                  g_date_set_dmy (&current_date, temp.start_nthDay, temp.start_nthMonth, temp.start_nthYear);
                  s_links = links_check_task_successors (temp.id, data);
		  if(s_links>=0) {
		      retour = tasks_compute_dates_for_all_linked_task (temp.id, temp.durationMode, 
		                 temp.days, data->curTasks, &current_date, 0, s_links, TRUE, data->appWindow, data);
		      timeline_remove_all_tasks (data);/* YES, for other tasks */
		      timeline_draw_all_tasks (data);
		  }/* endif s_links */
	       }
	    }/* elseif decrease */
	 }/* endif not fixed */
	 else {
	    msg_error_fixed (data);
	 }
     }/* endif NOT resize */
     else {/* resize mode */
         newDuration = duration-tmp_tasks_datas->days;
         if(newDuration == 0)
             newDuration = 1;
          printf ("durée actuelle =%d nouvelle = %d New =%d\n", tmp_tasks_datas->days, duration, newDuration);
         
        /* we have only to check if new duration is compatible with deadline */
        if((tmp_tasks_datas->durationMode ==TASKS_DEADLINE) && (newDuration>0)) {
           printf ("je dois vérifier deadeline \n");
           if(!tasks_test_increase_duration (data->curTasks, newDuration, data)) {
              fErr = TRUE;
              msg_error_deadline (data);
           }
        }  
        if(!fErr) {
          /* if oK, we can check/change project ending date */
          tasks_change_duration (newDuration, data->curTasks, data);/* uNdo engine inside */
          /* we get MODIFIED values */
          tasks_get_ressource_datas (data->curTasks, &temp, data);
          s_links = links_check_task_successors (temp.id, data);
          printf ("ds modify s_link =%d avec newduration %d relue =%d mode +%d\n", s_links, newDuration, temp.days, temp.durationMode);
          if(s_links>=0) {
            retour = tasks_compute_dates_for_all_linked_task (temp.id, temp.durationMode, 
                         temp.days, data->curTasks, &current_date, 0, s_links, TRUE, data->appWindow, data);
            timeline_remove_all_tasks (data);/* YES, for other tasks */
            timeline_draw_all_tasks (data);
            printf ("j'ai modifié %d tâches et/ou jalons \n", retour);
	  }/* endif links */
        }
     }
     /* modify assignments */
      misc_display_app_status (TRUE, data);

 }/* endif */
 else {/* no selection, click on background TODO ajouter un test pour voir si l'on a cliqué sur un objet */
    /* is it a task ? */
    ret = (gint) (event->y-TIMELINE_CALENDAR_HEADER_HEIGHT)/TIMELINE_AVATAR_HEIGHT;
    id = timeline_get_item_id (item);

    printf (" trelease sans select tâche potentielle = %d  pour cutask =%d avec objet %d\n", ret, data->curTasks, id );
    printf ("offset jour potentiel =%d \n", day_offset);
   
    if(event->button==3) {/* right-click */
      if(ret>=g_list_length (data->tasksList)) {
	  GtkMenu *menu;
	  menu = GTK_MENU(create_menu_back_timeline (data->appWindow, data));
	  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
      }
      else { /* we display an helpful message =) */
         msg = g_strdup_printf ("%s", _("Sorry !\nWhen you click on the timeline, you can ...\n<i>right-click</i> on an <b>existing</b> task to edit it.\n ... Or right-click on a line <b>without</b> any task to add a new task."));
         misc_ErrorDialog (data->appWindow, msg);
         g_free (msg);
      }
    }
 }/* else if no selection */
 return TRUE;
}

/*************************************************** 
 LOCAL - This handles button presses in item views. 
 **************************************************/
static gboolean on_task_button_press (GooCanvasItem *item, GooCanvasItem *target, GdkEventButton *event,
                                      APP_data *data)
{
   gint i, ret = -1, id = -1, type, nchild;
   gdouble x1t, y1t, width, height;
   GList *l;
   GtkListBox *box;
   GtkListBoxRow *row;
   GooCanvasBounds bounds;
   GooCanvasItem *child;
   tasks_data temp;
   gchar *tmpStr;

   timeline_remove_selection_box (data); 
   fSelect = FALSE;

   if(event->type==GDK_2BUTTON_PRESS) {/* double-click */
      cursor_normal (data->appWindow);
      return TRUE;
   }

   /* how many children ? */
   nchild = goo_canvas_item_get_n_children (item);
   for(i=0;i<nchild;i++) {
      child = goo_canvas_item_get_child (item, i);
      if(child) {
         g_object_get (child, "title", &tmpStr, NULL);/* we get a string representation of unique IDentifier */
         if(tmpStr) {/* only main rectangle or milestone losange  has a name */
            g_free (tmpStr);   
            goo_canvas_item_get_bounds (child, &bounds); 
          }
      }
   }
   /* we looking for main rectangle sub idem */
   
   id = timeline_get_item_id (item);
   /* now we compute the item's index */
   ret = timeline_get_item_index (id, data);
   /* we select according row */
   box = GTK_LIST_BOX(GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasks"))); 
   /* TODO : unselect */
   row = gtk_list_box_get_row_at_index (GTK_LIST_BOX(box), ret);
   gtk_list_box_select_row (GTK_LIST_BOX(box), row);
   timeline_hide_cursor (data);
   day_offset = (gint) event->x/(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor);
   timeline_show_cursor (day_offset, data);
   tasks_get_ressource_datas (ret, &temp, data);

   if(event->button==3 && ret>=0) {/* right-click */
	  GtkMenu *menu = GTK_MENU(create_menu_timeline (data->appWindow, data));
	  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
   }
   else { /* simple-click, we can move the task alon its own line - apart for groups */
       type = tasks_get_type (ret, data);
       if(type!=TASKS_TYPE_GROUP) {
          selBox = timeline_selection_box (goo_canvas_get_root_item (GOO_CANVAS (data->canvasTL)), 
                                           bounds.x1, bounds.y1, bounds.x2, bounds.y2);
          fSelect = TRUE;
          cursor_move_task (data->appWindow);
          /* check if we are on right side of a task in order to switch to resize tasks mode */
          fResize = FALSE;
          newWidth = 0;
          width = 10;
          if(zoom_factor<1)
              width = 5;
          if( (ABS(event->x-bounds.x2)<width)  && (temp.type==TASKS_TYPE_TASK)) {
               fResize = TRUE;
               cursor_resize_task (data->appWindow);
          }
          else
               cursor_move_task (data->appWindow);
       }
   }

  return TRUE;
}

/******************************************
 PROTECTED : draw the 'bar' representing
 a task
******************************************/
static void timeline_draw_bar (gint id, gint nbDays, gint duration, gint index, gchar *sRgba, 
                               GdkRGBA modColor, GooCanvasItem *group_task, gdouble x_start_shift, gdouble x_end_shift)
{
  gchar *tmpStr;

  GooCanvasItem *rect_item = 
               goo_canvas_rect_new (group_task, nbDays*(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor)+x_start_shift, 
		              TIMELINE_CALENDAR_HEADER_HEIGHT+(TIMELINE_AVATAR_HEIGHT*index)+7, 
		              (duration*(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor))-x_end_shift-x_start_shift,              
                              TIMELINE_AVATAR_HEIGHT-7,
		                           "line-width", 2.0, "radius-x", 2.0*zoom_factor, "radius-y", 2.0*zoom_factor,
		                           "stroke-color", "#BEBEBE", "fill-color", "white",
		                           NULL); 

  tmpStr = misc_convert_gdkcolor_to_hex (modColor);
  GooCanvasItem *small_rect = goo_canvas_rect_new (group_task, 
                                              nbDays*(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor)+2*zoom_factor+x_start_shift,
		                                TIMELINE_CALENDAR_HEADER_HEIGHT+(TIMELINE_AVATAR_HEIGHT*index)+12, 
		                                8*zoom_factor, TIMELINE_AVATAR_HEIGHT-18,
		                           "line-width", 1.0, "radius-x", 1.0*zoom_factor, "radius-y", 1.0,
		                           "stroke-color", tmpStr, "fill-color", sRgba,
		                           NULL);
  g_free (tmpStr);
  /* extremas */
  if(nbDays*(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor)+(duration*(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor)) > canvas_max_x)
      canvas_max_x = nbDays*(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor)+(duration*(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor));
  if(TIMELINE_CALENDAR_HEADER_HEIGHT+(TIMELINE_AVATAR_HEIGHT*index)+TIMELINE_AVATAR_HEIGHT > canvas_max_y)
      canvas_max_y = TIMELINE_CALENDAR_HEADER_HEIGHT+(TIMELINE_AVATAR_HEIGHT*index)+TIMELINE_AVATAR_HEIGHT;

  /* we set a reference name */
  tmpStr = g_strdup_printf ("R%d", id);
  g_object_set (rect_item, "title", tmpStr, NULL);
  g_free (tmpStr);

/*

goo_canvas_rect_new (group_task, nbDays*TIMELINE_CALENDAR_DAY_WIDTH, 
		                                TIMELINE_CALENDAR_HEADER_HEIGHT+(TIMELINE_AVATAR_HEIGHT*index)+7, 
		                                duration*TIMELINE_CALENDAR_DAY_WIDTH, TIMELINE_AVATAR_HEIGHT-7,
		                           "line-width", 3.0,
		                           "radius-x", 8.0,
		                           "radius-y", 8.0,
		                           "stroke-color", g_strdup_printf ("%s", misc_convert_gdkcolor_to_hex(modColor) ),
		                           "fill-color", sRgba,
		                           NULL); 
*/

}

/******************************************
 LOCAL : same for a 'group' i.e.
 a kind of 'separator' between tasks
******************************************/
static void timeline_draw_group (gint nbDays, gint duration, gint index, gchar *name, GooCanvasItem *group_task)
{
   gchar *tmpStr;
   gdouble xorg, yorg, xend, yend;
printf ("groupe =%s durée = %d \n", name, nbDays);

   xorg = nbDays*(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor);
   yorg = TIMELINE_CALENDAR_HEADER_HEIGHT+(TIMELINE_AVATAR_HEIGHT*index)+10;
   xend = nbDays*(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor)+(duration*TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor);
   yend = yorg+TIMELINE_AVATAR_HEIGHT/1.5;

   tmpStr = g_strdup_printf ("M %d %d L %d %d L %d %d L %d %d L %d %d L %d %d z", (gint)xorg, (gint)yorg, 
              (gint)xend-10, (gint)yorg, 
              (gint)xend, (gint)yorg+TIMELINE_AVATAR_HEIGHT/3,
              (gint)xend-10, (gint)yend, 
              (gint)xorg, (gint)yend, 
              (gint)xorg+10, (gint)yorg+TIMELINE_AVATAR_HEIGHT/3);
   GooCanvasItem *task_path =  goo_canvas_path_new (group_task, tmpStr, "stroke-color", "#BEBEBE", 
                                                    "fill-color", "#e4e4e4", NULL); 
   g_free (tmpStr);

   tmpStr = g_markup_printf_escaped ("%s", name);
   GooCanvasItem *task_text = goo_canvas_text_new (group_task, tmpStr, 
		                           nbDays*(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor)+16,
		                           TIMELINE_CALENDAR_HEADER_HEIGHT+(TIMELINE_AVATAR_HEIGHT*index)+16,
		                           -1, GOO_CANVAS_ANCHOR_NW, "use-markup", TRUE,
		                           "font", "Sans 14", "fill-color", "black", NULL);  
   g_free (tmpStr);
}

/******************************************
 LOCAL : same for a 'milestone' i.e.
 a kind of 'anchor' between tasks
******************************************/
static void timeline_draw_milestone (gint id, gint nbDays, gint index, gchar *name, GooCanvasItem *group_task)
{
   gchar *tmpStr;
   gdouble xorg, yorg, xend, yend;

   xorg = nbDays*(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor)+6*zoom_factor;
   yorg = TIMELINE_CALENDAR_HEADER_HEIGHT+(TIMELINE_AVATAR_HEIGHT*index)+12;
   xend = (nbDays+1)*(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor)-6*zoom_factor;
   yend = yorg+TIMELINE_AVATAR_HEIGHT-10;

 
   tmpStr = g_strdup_printf ("M %d %d L %d %d L %d %d L %d %d z", (gint)((xorg+xend)/2), (gint)yorg, 
              (gint)xend, (gint)(yorg+TIMELINE_AVATAR_HEIGHT/2-6), 
              (gint)((xorg+xend)/2), (gint)yend,
              (gint)xorg, (gint)(yorg+TIMELINE_AVATAR_HEIGHT/2-6));
   GooCanvasItem *task_path =  goo_canvas_path_new (group_task, tmpStr, "line-width", 3.0,  "stroke-color", "#746fc5" ,
                                       "fill-color", "#e5e5e5", NULL); 
   g_free (tmpStr);
   /* we set a reference name */
   tmpStr = g_strdup_printf ("M%d", id);
   g_object_set (task_path, "title", tmpStr, NULL);
   g_free (tmpStr);

   tmpStr = g_markup_printf_escaped ("%s", name);
   GooCanvasItem *task_text = goo_canvas_text_new (group_task, tmpStr, 
		                           xend,
		                           TIMELINE_CALENDAR_HEADER_HEIGHT+(TIMELINE_AVATAR_HEIGHT*index)+14,
		                           -1,
		                           GOO_CANVAS_ANCHOR_NW, "use-markup", TRUE,
		                           "font", "Sans 9", "fill-color", "black", 
		                           NULL);  
   g_free (tmpStr);
}

/******************************************
LOCAL : display avatars for current tasks
id = current task
******************************************/
void timeline_draw_avatars (gint id, GooCanvasItem *group, gdouble xorg, gdouble yorg, APP_data *data)
{
  gint i, link=0, ok=-1, total, count =0;
  GList *l;
  rsc_datas *tmp_rsc_datas;
  GError *err = NULL;

  /* we read all ressources, and when a match is found, we update display */
  total = g_list_length (data->rscList);
  for(i=0; i<total; i++) {
     l = g_list_nth (data->rscList, i);
     tmp_rsc_datas = (rsc_datas *)l->data;
     ok = links_check_element (id, tmp_rsc_datas->id, data);
     /* we test association receiver-sender for current line */
     if(ok>=0) {
        GdkPixbuf *pix2;
        /* icon */
        if(tmp_rsc_datas->pix) {   
           pix2 = gdk_pixbuf_scale_simple (tmp_rsc_datas->pix, 
                             TASKS_RSC_AVATAR_SIZE+4, TASKS_RSC_AVATAR_SIZE+4, GDK_INTERP_BILINEAR);
        }
        else {
            //pix2 = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, TASKS_RSC_AVATAR_SIZE+6, TASKS_RSC_AVATAR_SIZE+6);
            pix2 = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("def_avatar.png"), TASKS_RSC_AVATAR_SIZE+4, TASKS_RSC_AVATAR_SIZE+4, &err);
            // gdk_pixbuf_fill (pix2, 0x000000FF);/* black */
        }
        /* converts to rounded avatar */
        cairo_surface_t *surf = NULL;
        cairo_t *cr = NULL;
        surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, TASKS_RSC_AVATAR_SIZE+4, TASKS_RSC_AVATAR_SIZE+4);
        /* now we can create the context */
        cr = cairo_create (surf);
        gdk_cairo_set_source_pixbuf (cr, pix2, 0, 0);
        cairo_arc (cr, (TASKS_RSC_AVATAR_SIZE+4)/2, (TASKS_RSC_AVATAR_SIZE+4)/2, (TASKS_RSC_AVATAR_SIZE+4)/2-2, 0, 2*G_PI);
        cairo_clip (cr);
        cairo_paint (cr);
        cairo_destroy (cr);
        GdkPixbuf *pixbuf2 = gdk_pixbuf_get_from_surface (surf, 0, 0,
                                      TASKS_RSC_AVATAR_SIZE+4, TASKS_RSC_AVATAR_SIZE+4);
        cairo_surface_destroy (surf);

        goo_canvas_image_new (group, pixbuf2, xorg+(count*(TASKS_RSC_AVATAR_SIZE+4)), yorg+14, NULL);
        g_object_unref (pixbuf2);
        g_object_unref (pix2);
        count++;
     }/* endif OK */
  }/* next i */
}

/***********************************************************************
  PUBLIC : redraw ONE task object
  arguments : index = #of task index TODO and for "groups ??? 
  root = root of goocanvas
  date_proj_start = current project to left date, clamped to a monday
                (it isn't necessary TRUE project's start date)
  tmp_task_date : valid pointer on current task structure
************************************************************************/
void timeline_draw_task (gint index, GooCanvasItem *root, GDate date_proj_start, tasks_data *tmp_tasks_datas, APP_data *data)
{
   GdkRGBA color, modColor;
   gboolean isDark = FALSE;
   gint day, i, link, link_mode, dd, sHour, eHour;
   gdouble x_pos, x_start_shift, x_start_shift_pred, x_end_shift, x_end_shift_pred;
   GList *l;
   gchar *sRgba, *tmpStr;
   GDate date_start, date_end, date_start_link;
   GooCanvasItem *group_task, *rect_item, *small_rect;

   /* x = shift in days between start projets and current task start */
   g_date_set_dmy (&date_start, tmp_tasks_datas->start_nthDay, tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
   g_date_set_dmy (&date_end, tmp_tasks_datas->end_nthDay, tmp_tasks_datas->end_nthMonth, tmp_tasks_datas->end_nthYear);
   gint nbDays = g_date_days_between (&date_proj_start, &date_start);
   gint duration = tmp_tasks_datas->days;

   if(duration<=0 && tmp_tasks_datas->type!=TASKS_TYPE_MILESTONE && tmp_tasks_datas->type!=TASKS_TYPE_GROUP) {
      printf ("* PlanLibre critical-duration <1 day for %s *\n", tmp_tasks_datas->name);
      duration = 1;/* temporary until hours aren't managed */
   }
 //  printf ("task %d écart date depuis début =%d durée =%d \n", index, nbDays, duration);
   /* detect dark color */
   color = tmp_tasks_datas->color;
   if(color.red == 0 && color.green ==0 && color.blue==0)  {
         color.red = .05;
         color.green = .05;
         color.blue = .05;
   }
   if(tmp_tasks_datas->color.red+tmp_tasks_datas->color.green+tmp_tasks_datas->color.blue<0.4) {
         isDark = TRUE;
         modColor.red = color.red * 2;
         modColor.green = color.green * 2;
         modColor.blue = color.blue * 2;
   }
   else {
         isDark = FALSE;
         modColor.red = color.red * 0.9;
         modColor.green = color.green * 0.9;
         modColor.blue = color.blue * 0.9;
   }

   /* y = TIMELINE_AVATAR_HEIGHT* nth of current task */

   x_pos = nbDays*(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor)+(32*zoom_factor);
   if(x_pos>TIMELINE_VIEW_MAX_WIDTH)
      return;
   group_task = goo_canvas_group_new (root, NULL);

   if(tmp_tasks_datas->type == TASKS_TYPE_TASK) {
           sRgba = misc_convert_gdkcolor_to_hex (tmp_tasks_datas->color);
           /* we get day of the week for current task */
           dd = misc_get_calendar_day_from_glib (g_date_get_weekday (&date_start));
           sHour = calendars_get_starting_hour (0, dd, data);
           eHour = 1440-calendars_get_ending_hour (0, dd, data);

           /* for now, default calendar = 0 - sHour starting hour in MINUTES*/
           x_start_shift = (TIMELINE_CALENDAR_DAY_WIDTH*((gdouble)sHour/1440))*zoom_factor;/* 24h = 1440 minutes */
           x_end_shift = (TIMELINE_CALENDAR_DAY_WIDTH * ((gdouble)eHour/1440))*zoom_factor;/* 24h = 1440 minutes */
	   timeline_draw_bar (tmp_tasks_datas->id, nbDays, duration, index, sRgba, modColor, group_task, x_start_shift, x_end_shift);
	   g_free (sRgba);
	   /* we add tasks' name */
           if(tmp_tasks_datas->durationMode == TASKS_FIXED)
	       tmpStr = g_markup_printf_escaped ("<i>%s</i>", tmp_tasks_datas->name);
           else
	       tmpStr = g_markup_printf_escaped ("%s", tmp_tasks_datas->name);
	   /* if task's fill color is dim, we must invert text color */
	   if(isDark)
		sRgba = g_strdup_printf ("%s", "black");// "#A9B2EB"
	   else
		sRgba = g_strdup_printf ("%s", "black");
		
	   GooCanvasItem *task_text = goo_canvas_text_new (group_task, tmpStr, 
		                           x_pos+x_start_shift/2-4*(zoom_factor),
		                           TIMELINE_CALENDAR_HEADER_HEIGHT+(TIMELINE_AVATAR_HEIGHT*index)+7,
		                           -1,
		                           GOO_CANVAS_ANCHOR_NW, "font", "Sans 9", "fill-color", sRgba, 
                                           "use-markup", TRUE,
		                           NULL);  
	   g_free (sRgba);
	   g_free (tmpStr);
           /*  avatars */
           timeline_draw_avatars (tmp_tasks_datas->id, group_task, x_pos+x_start_shift/2, 
                                  TIMELINE_CALENDAR_HEADER_HEIGHT+(TIMELINE_AVATAR_HEIGHT*index)+4, data);

   }/* endif std task */
   if(tmp_tasks_datas->type == TASKS_TYPE_GROUP) {/* we display a "group" task */
      timeline_draw_group (nbDays, duration, index, tmp_tasks_datas->name, group_task);
   }

   if(tmp_tasks_datas->type == TASKS_TYPE_MILESTONE) {/* we display a "milestone" task */
      timeline_draw_milestone (tmp_tasks_datas->id, nbDays, index, tmp_tasks_datas->name, group_task);
   }

   /* Connect a signal handler for the rectangle item. */
   g_signal_connect ((gpointer)group_task, "button_press_event", G_CALLBACK (on_task_button_press), data);
   g_signal_connect ((gpointer)group_task, "button_release_event", G_CALLBACK (on_timeline_mouse_release), data);

   g_signal_connect ((gpointer)group_task, "enter-notify-event", G_CALLBACK (on_timeline_enter_task), data);
   g_signal_connect ((gpointer)group_task, "leave-notify-event", G_CALLBACK (on_timeline_leave_task), data);

   /* now, we are looking for predecessors tasks */
   for(i=0; i<g_list_length (data->tasksList); i++) {
     l = g_list_nth (data->tasksList, i);
     tasks_data *tmp_tsk_datas;
     tmp_tsk_datas = (tasks_data *)l->data;
     if((tmp_tasks_datas->id != tmp_tsk_datas->id) && (tmp_tsk_datas->type != TASKS_TYPE_GROUP)) {
          link = links_check_element (tmp_tasks_datas->id, tmp_tsk_datas->id, data);
          if(link>=0) {/* there is a predecessor */
               link_mode = links_get_element_link_mode (tmp_tasks_datas->id, tmp_tsk_datas->id, data);
               /* we draw two lines for link */
               gdouble x1, y1, x2, y2, xend;
               g_date_set_dmy (&date_start_link, 
                               tmp_tsk_datas->start_nthDay, tmp_tsk_datas->start_nthMonth, tmp_tsk_datas->start_nthYear);
               gint nbDays2 = g_date_days_between (&date_proj_start, &date_start_link);
	       if(tmp_tsk_datas->type==TASKS_TYPE_MILESTONE) {
		  printf ("date début jalon =%d %d %d \n", tmp_tsk_datas->start_nthDay, tmp_tsk_datas->start_nthMonth, tmp_tsk_datas->start_nthYear);
                  //nbDays2++;
	        } 
               dd = misc_get_calendar_day_from_glib (g_date_get_weekday (&date_start_link));
               sHour = calendars_get_starting_hour (0, dd, data);
               eHour = calendars_get_ending_hour (0, dd, data);
               x_start_shift_pred = TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor* ((gdouble)sHour/1440);/* 24h = 1440 minutes */
               x_end_shift_pred = (TIMELINE_CALENDAR_DAY_WIDTH * ((gdouble)eHour/1440))*zoom_factor;/* 24h = 1440 minutes */
               x1 = nbDays*(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor)+x_start_shift;/* x1 = near final arrow */
               y1 = TIMELINE_CALENDAR_HEADER_HEIGHT+(TIMELINE_AVATAR_HEIGHT*index)+7+TIMELINE_AVATAR_HEIGHT/2;
              
              // x2 = nbDays2*(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor)+(xend/2)+2;/* at center of sender task */
              /* TODO différencier selon type lien E>>S S>>S etc.
  durationMode LINK_END_START 0  LINK_START_START 1  LINK_END_END 2  LINK_START_END 3 */
               if(link_mode == LINK_END_START) {
                  xend = (tmp_tsk_datas->days*(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor))-x_end_shift_pred+(16*zoom_factor);
                  x2 = nbDays2*(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor)+xend;/* at end of sender task */
                  y2 = TIMELINE_CALENDAR_HEADER_HEIGHT+(TIMELINE_AVATAR_HEIGHT*i)+TIMELINE_AVATAR_HEIGHT;    
                  GooCanvasItem *task_link_path = goo_canvas_polyline_new (group_task, FALSE, 4, x1, y1,
                                  x2, y1, 
                                  x2, y2, 
                                  x2, y2,
                                  "start-arrow", TRUE, "line-width", 2.0,
                                  "stroke-color", "#9EB6CD", NULL);           
               }
               if(link_mode == LINK_START_START) {
                  x2 = nbDays2*(TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor)+(4*zoom_factor);/* at start of sender task */
                  y2 = TIMELINE_CALENDAR_HEADER_HEIGHT+(TIMELINE_AVATAR_HEIGHT*i)+TIMELINE_AVATAR_HEIGHT/2+7;
                  GooCanvasItem *task_link_path =  goo_canvas_polyline_new (group_task, FALSE, 4, x1, y1,
                                  x2-4*zoom_factor, y1, 
                                  x2-4*zoom_factor, y2, 
                                  x2+8*zoom_factor, y2,
                                  "start-arrow", TRUE,
                                  "line-width", 2.0,
                                  "stroke-color", "#9EB6CD",
                                  NULL);
               }

              if(x2>canvas_max_x)
                 canvas_max_x = x2;
              if(y2>canvas_max_y)
                 canvas_max_y = y2;
          }/* if link >=0 */
     }/* endif */
   }/* next i */
   /* goovanvas item management */
   tmpStr = g_strdup_printf ("%d", tmp_tasks_datas->id);
   g_object_set (group_task, "title", tmpStr, NULL);
   tmp_tasks_datas->groupItems = group_task;
   g_free (tmpStr);
}

/******************************************
  PUBLIC : redraw tasks objects
******************************************/
void timeline_draw_all_tasks (APP_data *data)
{
  gint i, day, month, year;
  GList *l;
  GDate date_proj_start;
  GooCanvasItem *root;
  tasks_data *tmp_tasks_datas;

  /* reset exportation values */
  canvas_min_x = 0;
  canvas_max_x = 0; 
  canvas_max_y = 0;
  /* get date stored for display */
  timeline_get_date (&day, &month, &year);
  g_date_set_dmy (&date_proj_start, day, month, year);
  /* we adjust to first day of week like in calendar ruler */
  day = g_date_get_day_of_year (&date_proj_start);
  while(g_date_get_weekday (&date_proj_start)!= G_DATE_MONDAY ) {
     g_date_subtract_days (&date_proj_start, 1);
  }
  /* TODO store minimum x position - top left position for drwing exportation */ 
// TODO revoir affihage calendrier et le reste ! 
  /* now, we read all startdates */
  root = goo_canvas_get_root_item (GOO_CANVAS (data->canvasTL));
  /* first loop : tasks  only */
  for(i=0; i<g_list_length (data->tasksList); i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     if(tmp_tasks_datas->type == TASKS_TYPE_TASK) {
        timeline_draw_task (i, root, date_proj_start, tmp_tasks_datas, data);
     }
     /* TODO store top right maximum x position */

  }/* next */
  /* nd loop : groups and milestones  */
  for(i=0; i<g_list_length (data->tasksList); i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     if(tmp_tasks_datas->type != TASKS_TYPE_TASK) {
        timeline_draw_task (i, root, date_proj_start, tmp_tasks_datas, data);
     }
     /* TODO store top right maximum x position */

  }/* next */
  /* we raise calendar above ALL*/
  goo_canvas_item_raise (data->ruler, NULL);
}

/******************************************
  PUBLIC : remove ALL tasks objects
******************************************/
void timeline_remove_all_tasks (APP_data *data)
{
  gint i;
  GList *l;
  GooCanvasItem *root;
  tasks_data *tmp_tasks_datas;

  root = goo_canvas_get_root_item (GOO_CANVAS(data->canvasTL));

  for(i=0; i<g_list_length (data->tasksList); i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     if(tmp_tasks_datas->groupItems!=NULL) {
         goo_canvas_item_remove (tmp_tasks_datas->groupItems);
         tmp_tasks_datas->groupItems = NULL;
     }  
  }/* next */
}

/*****************************************
  PUBLIC : remove calendar ruler
*****************************************/
void timeline_remove_ruler (APP_data *data)
{
  if(data->ruler!=NULL) {
     goo_canvas_item_remove (data->ruler); 
     data->ruler = NULL;
     cursor = NULL;
  }
}

/******************************************
  PUBLIC : redraw calender ruler
  when a scroll event fires
******************************************/
void on_timeline_vadj_changed (GtkAdjustment *adjustment, APP_data *data)
{
  timeline_hide_cursor (data);
  g_object_set (data->ruler, "y", gtk_adjustment_get_value (adjustment), NULL);
  goo_canvas_item_raise (data->ruler, NULL);
 // goo_canvas_item_remove(data->ruler);
 // timeline_draw_calendar_ruler ( gtk_adjustment_get_value (adjustment), data);
  timeline_show_cursor (day_offset, data);
}

/*****************************************************
  PUBLIC : draw calendar ruler
  arguments : ypos = vertical position (adjustments)
******************************************************/
void timeline_draw_calendar_ruler (gdouble ypos, APP_data *data)
{
  GooCanvasItem *root, *sep_line, *calendar_header, *sep_days, *cur_day, *text_day;
  gint i, j, day =0, week = 0, month, year = 0;
  gdouble demiWidth, sevenWidth, demiHeight, pos_x, fnt_size;
  gchar *tmpStr = NULL, *tmpMonth = NULL;
  const gchar *tmpFnt = NULL;
  const gchar *grey_day = "#3E3E3E";
  const gchar *blue_day = "#1DD6F2";
  const gchar *smallFnt = "Sans 8";
  const gchar *normalFnt = "Sans 10";
  GDate *date, current_date, start_ruler_date;

  ypos_ruler = ypos;
  root = goo_canvas_get_root_item (GOO_CANVAS (data->canvasTL));
  /* machine time gains ;-) */
  demiWidth = (TIMELINE_CALENDAR_DAY_WIDTH/2)*zoom_factor;
  sevenWidth = TIMELINE_CALENDAR_DAY_WIDTH*7;
  demiHeight = TIMELINE_AVATAR_HEIGHT/2;
  /* if any of stored dates == -1 we MUST use projec's dates */
  if(!test_stored_dates()) {
     timeline_store_date (data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
  }
  /* we read stored dates */
  timeline_get_date (&day, &month, &year);
  date = g_date_new_dmy (day, month, year);
  day = g_date_get_day_of_year (date);

  /* we clamp to monday of the week */
  while(g_date_get_weekday (date)!= G_DATE_MONDAY ) {
     g_date_subtract_days (date, 1);
  }
  day = g_date_get_day_of_year (date);
  week = g_date_get_day_of_year (date)/7+1;// be carefull : in january & december, there is some issues
  month = g_date_get_month (date);
  year = g_date_get_year (date);
  /* copy date */

  g_date_set_dmy (&start_ruler_date, g_date_get_day (date), month, year);
  /* ruler calendar header group */
  GooCanvasItem *CalendarRuler = goo_canvas_group_new (root, NULL);
  data->ruler = CalendarRuler;
  calendar_header = goo_canvas_rect_new (CalendarRuler, 0, ypos, TIMELINE_VIEW_MAX_WIDTH, TIMELINE_CALENDAR_HEADER_HEIGHT,
                                   "line-width", 0.0, "radius-x", 0.0, "radius-y", 0.0,
                                   "stroke-color", "white", "fill-color", "white",
                                   NULL);

  /* back ground of am/pm graduation */
  GooCanvasItem *sep_am = goo_canvas_polyline_new_line (CalendarRuler, 
                                                     0,ypos+TIMELINE_CALENDAR_HEADER_HEIGHT-2,
                                                     TIMELINE_VIEW_MAX_WIDTH, ypos+TIMELINE_CALENDAR_HEADER_HEIGHT-2,
                                                        "stroke-color", "#6495ED", "line-width", 3.0, NULL); 
  /* draw a separation line for calendar header */
  sep_line = goo_canvas_polyline_new_line (CalendarRuler, 0, ypos+demiHeight,
                                             TIMELINE_VIEW_MAX_WIDTH, ypos+demiHeight,
                                             "stroke-color", "#D7D7D7", "line-width", 1.5, NULL);  
  /* draw text - weeks */
  tmpFnt = normalFnt;
  if(zoom_factor<1)
     tmpFnt = smallFnt;
  pos_x = 0;
  i = 0;
  while(pos_x<TIMELINE_VIEW_MAX_WIDTH) {
    if(week>52) {
       week = 1;
       year++;
    }
    pos_x = i*(zoom_factor*sevenWidth);// TODO moduler taille caract
    GooCanvasItem *sep_vert = goo_canvas_polyline_new_line (CalendarRuler, pos_x, ypos,
                                                     pos_x, ypos+TIMELINE_AVATAR_HEIGHT,
                                                        "stroke-color", "#D7D7D7", "line-width", 1.5, NULL); 
    /* dates are in properties data at start, and/or in tasks when a file is loaded */
    month = g_date_get_month (date);
    tmpMonth = misc_get_month_of_year (month);
    if(zoom_factor<1)
        tmpStr = g_strdup_printf (_("week %d of %d"), week, year);
    else
        tmpStr = g_strdup_printf (_("week %d of %d-%s"), week, year, tmpMonth);

    GooCanvasItem *sep_weeks = goo_canvas_text_new (CalendarRuler, tmpStr, pos_x+(12*zoom_factor), ypos+4, -1,
                                   GOO_CANVAS_ANCHOR_NW, "font", tmpFnt, "fill-color", "#282828", 
                                   NULL);  
    week++;
    g_free (tmpMonth);
    g_free (tmpStr);
    /* draw days inside week */
    for(j=0; j<7; j++) {
       tmpStr = g_strdup_printf ("%d", g_date_get_day (date));
       /* draw am/pm line */
       GooCanvasItem *sep_pm = goo_canvas_polyline_new_line (CalendarRuler, 
                                                     pos_x+(j*zoom_factor*TIMELINE_CALENDAR_DAY_WIDTH)+demiWidth, 
                                                     ypos+TIMELINE_CALENDAR_HEADER_HEIGHT-2,
                                                     pos_x+(j*zoom_factor*TIMELINE_CALENDAR_DAY_WIDTH)+demiWidth+demiWidth, 
                                                     ypos+TIMELINE_CALENDAR_HEADER_HEIGHT-2,
                                                        "stroke-color", "#DCE4F1",
                                                        "line-width", 3.0, NULL);
       if(j<5)
            sep_days = goo_canvas_text_new (CalendarRuler, tmpStr, 
                                   pos_x+(j*zoom_factor*TIMELINE_CALENDAR_DAY_WIDTH)+demiWidth, 
                                   ypos+4+(TIMELINE_CALENDAR_HEADER_HEIGHT/2), -1,
                                   GOO_CANVAS_ANCHOR_NORTH,
                                   "font", tmpFnt, "fill-color", grey_day, 
                                   NULL);  
       else
            sep_days = goo_canvas_text_new (CalendarRuler, tmpStr, 
                                   pos_x+(j*zoom_factor*TIMELINE_CALENDAR_DAY_WIDTH)+demiWidth, 
                                   ypos+4+(TIMELINE_CALENDAR_HEADER_HEIGHT/2), -1,
                                   GOO_CANVAS_ANCHOR_NORTH,
                                   "font", tmpFnt, "fill-color", blue_day, 
                                   NULL);  
       g_free (tmpStr);
       g_date_add_days (date, 1);
    }/* next day */
    i++;
  }/* wend wekks */

  /* we emphasize the current date's day */
  g_date_set_time_t (&current_date, time (NULL)); /* date of today */
  gint nbDays = g_date_days_between (&start_ruler_date, &current_date);
  /* we display ellipse only if in physical limits */
  pos_x = demiWidth+(nbDays*TIMELINE_CALENDAR_DAY_WIDTH*zoom_factor);
  if(pos_x<TIMELINE_VIEW_MAX_WIDTH) {
     tmpStr = g_strdup_printf ("%d", g_date_get_day (&current_date));
     cur_day = goo_canvas_ellipse_new (CalendarRuler, pos_x, 
                                 ypos+5+demiWidth/2+(TIMELINE_CALENDAR_HEADER_HEIGHT/2)-2, demiWidth/2+4/zoom_factor, demiWidth/2+4/zoom_factor,
                                 "line-width", 0.0, "stroke-color", "green", "fill-color", "green", NULL);
     text_day = goo_canvas_text_new (CalendarRuler, tmpStr, 
                                   pos_x, 
                                   ypos+4+(TIMELINE_CALENDAR_HEADER_HEIGHT/2), -1,
                                   GOO_CANVAS_ANCHOR_NORTH,
                                   "font", tmpFnt, "fill-color", "white", 
                                   NULL);
     g_free (tmpStr);

  }/* endif current day mark */
}

/***************************************
  PUBLIC ! initialize timeline drawing
****************************************/
void timeline_canvas (GtkWidget *win, APP_data *data)
{
   GtkWidget  *canvas;
   GooCanvasItem *root, *rect_item, *calendar_header, *sep_line, *text_item;
   GtkAdjustment *vadj;

   canvas = goo_canvas_new ();
   data->canvasTL = canvas;
   g_object_set (canvas, "background-color" , "#F3F6F6", NULL);

   goo_canvas_set_bounds (GOO_CANVAS (canvas), 0, 0, TIMELINE_VIEW_MAX_WIDTH, TIMELINE_VIEW_MAX_HEIGHT);
   gtk_widget_show (canvas);
   gtk_container_add (GTK_CONTAINER (win), canvas);

   root = goo_canvas_get_root_item (GOO_CANVAS (canvas));

   /* we draw a rectangle as a background in order to catch some events */
   GooCanvasItem *rect_back =  goo_canvas_rect_new (root, 0, 
                                TIMELINE_CALENDAR_HEADER_HEIGHT, 
                                TIMELINE_VIEW_MAX_WIDTH, 
                                TIMELINE_VIEW_MAX_HEIGHT-TIMELINE_CALENDAR_HEADER_HEIGHT,
                                   "line-width", 0.0,
                                   "radius-x", 0.0, "radius-y", 0.0,
                                   "stroke-color", "#E9EEEE", "fill-color", "#F3F6F6",
                                   NULL);
   /* Connect a signal handler for the rectangle item. */
 //  g_signal_connect ((gpointer) root, "button_press_event", G_CALLBACK (on_rect_back_button_press), data);
   g_signal_connect ((gpointer) root, "motion_notify_event",
		             G_CALLBACK (on_timeline_mouse_move), data);
   g_signal_connect ((gpointer) root, "button_release_event",
		             G_CALLBACK (on_timeline_mouse_release), data);  
   /* signal for verical slider */
   vadj = gtk_scrolled_window_get_vadjustment (
                    GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowTimeline")));
   g_signal_connect ((gpointer) vadj, "value-changed",
		             G_CALLBACK (on_vertical_adjustment_value_changed), data);  


   /* ruler calendar header group */
   timeline_draw_calendar_ruler (0, data);
   timeline_show_cursor (0, data);
   /* draw text - weeks */
   timeline_draw_week_ends (root, data);
}

/*********************************************
  PUBLIC : general routine to export timeline
  argument : type of export
  returns 0 if success
****************************************/
gint timeline_export (gint type, APP_data *data)
{
  gint ret = 0;
  GKeyFile *keyString;
  gchar *filename, *tmpFileName;
  GtkFileFilter *filter = gtk_file_filter_new ();
  GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
  GtkWidget *alertDlg, *dialog;
  GooCanvasBounds bounds;
  cairo_surface_t *surface;
  cairo_t *cr;

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");

  switch(type) {
    case TIMELINE_EXPORT_PDF: {
      gtk_file_filter_add_pattern (filter, "*.pdf");
      gtk_file_filter_set_name (filter, _("Documents PDF files"));
      dialog = create_saveFileDialog (_("Export current timeline to a PDF file"), data);
      break;
    }
    case TIMELINE_EXPORT_SVG: {
      gtk_file_filter_add_pattern (filter, "*.svg");
      gtk_file_filter_set_name (filter, _("SVG drawings files"));
      dialog = create_saveFileDialog (_("Export current timeline to a SVG file"), data);
      break;
    }
    case TIMELINE_EXPORT_PNG: {
      gtk_file_filter_add_pattern (filter, "*.png");
      gtk_file_filter_set_name (filter, _("PNG image files"));
      dialog = create_saveFileDialog (_("Export current timeline to a PNG file"), data);
      break;
    }
    default: return -1;
  }/* end switch */

  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir());
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  /* run dialog for file selection */
  if(gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    /* test suffix */
     switch(type) {
	    case TIMELINE_EXPORT_PDF: {
              if(!g_str_has_suffix (filename, ".pdf" ) && !g_str_has_suffix (filename, ".PDF" )) {
                  /* we correct the filename */
                  tmpFileName = g_strdup_printf ("%s.pdf", filename);
                  g_free (filename);
                  filename = tmpFileName;
              }
	      break;
	    }
	    case TIMELINE_EXPORT_SVG: {
              if (!g_str_has_suffix (filename, ".svg" ) && !g_str_has_suffix (filename, ".SVG" )) {
                  /* we correct the filename */
                  tmpFileName = g_strdup_printf ("%s.svg", filename);
                  g_free (filename);
                  filename = tmpFileName;
              }
	      break;
	    }
	    case TIMELINE_EXPORT_PNG: {
              if (!g_str_has_suffix (filename, ".png" ) && !g_str_has_suffix (filename, ".PNG" )) {
                  /* we correct the filename */
                  tmpFileName = g_strdup_printf ("%s.png", filename);
                  g_free (filename);
                  filename = tmpFileName;
              }
	      break;
	    }
    }/* end switch extension */

    /* we must check if a file with same name exists */
    if(g_file_test (filename, G_FILE_TEST_EXISTS) 
        && g_key_file_get_boolean (keyString, "application", "prompt-before-overwrite",NULL )) {
           /* dialog to avoid overwriting */
           alertDlg = gtk_message_dialog_new (GTK_WINDOW(dialog),
                          flags,
                          GTK_MESSAGE_ERROR,
                          GTK_BUTTONS_OK_CANCEL,
                          _("The file :\n%s\n already exists !\nDo you really want to overwrite this file ?"),
                          filename);
           if(gtk_dialog_run (GTK_DIALOG(alertDlg))==GTK_RESPONSE_CANCEL) {
              gtk_widget_destroy (GTK_WIDGET(alertDlg));
              gtk_widget_destroy (GTK_WIDGET(dialog));
              g_free (filename);
              return;
           }
    }/* endif file exists */

    /* now, we really export */
     ret = 0;
     switch(type) {
	    case TIMELINE_EXPORT_PDF: {
              surface = cairo_pdf_surface_create (filename, timeline_get_max_x (), timeline_get_max_y ());
	      break;
	    }
	    case TIMELINE_EXPORT_SVG: {
              surface = cairo_svg_surface_create (filename, timeline_get_max_x (), timeline_get_max_y ());
	      break;
	    }
	    case TIMELINE_EXPORT_PNG: {
              surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, timeline_get_max_x (), timeline_get_max_y ());
	      break;
	    }
    }/* end switch surfaces */
    cr = cairo_create (surface);
    cairo_translate (cr, 0, 0);
    /* canvas */
    bounds.x1 = 0;
    bounds.y1 = 0;
    bounds.x2 = timeline_get_max_x ();
    bounds.y2 = timeline_get_max_y ();
    goo_canvas_render (GOO_CANVAS (data->canvasTL), cr, &bounds, 0.0);/* don't work without this include : #include <cairo-pdf.h> */ /* because goocanvs waits for this macro : CAIRO_HAS_PDF_SURFACE */
    switch(type) {
	    case TIMELINE_EXPORT_PDF: {
              cairo_show_page (cr);
              cairo_surface_destroy (surface);
              cairo_destroy (cr);
	      break;
	    }
	    case TIMELINE_EXPORT_SVG: {
              cairo_surface_destroy (surface);
              cairo_destroy (cr);
	      break;
	    }
	    case TIMELINE_EXPORT_PNG: {
              cairo_surface_write_to_png (surface, filename);
              cairo_surface_destroy (surface);
              cairo_destroy (cr);
	      break;
	    }
    }/* end switch surfaces */
    if(ret!=0) {
        gchar *msg = g_strdup_printf ("%s", _("Sorry !\nAn error happened during timeline exporting.\nOperation aborted. You can try again, or\ncheck your system."));
        misc_ErrorDialog (data->appWindow, msg);
        g_free (msg);
    }
    g_free (filename);
  }

  gtk_widget_destroy (GTK_WIDGET(dialog)); 

  return ret;
}
