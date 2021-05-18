#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* translations */
#include <libintl.h>
#include <locale.h>

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "support.h"
#include "misc.h"
#include "calendars.h"

enum {
  CAL_COLUMN = 0,
  CAL_NUM_COLS
};

static gint cal_current_type = CAL_TYPE_NON_WORKED;
static gint day_of_week = 0;
static gboolean fStopEmit = FALSE;

/*******************************
  variables glo-cales
*******************************/

/***************************************
  PUBLIC : change formating of minutes
  inspired by Gnome web site
***************************************/
gboolean on_min_spin_output (GtkSpinButton *spin, APP_data *data)
{
   GtkAdjustment *adjustment;
   gchar *text;
   gint value;

   adjustment = gtk_spin_button_get_adjustment (spin);
   value = (gint)gtk_adjustment_get_value (adjustment);
   text = g_strdup_printf ("%02d", value);
   gtk_entry_set_text (GTK_ENTRY (spin), text);
   g_free (text);

   return TRUE;
}

/************************************
  LOCAL : update display of number
  of non-worked days
  index : nth of current calendar
************************************/
static void calendars_update_number_nw_days (gint index, APP_data *data)
{
  GList *l;
  calendar *cal;
  gint nb = 0;
  GtkWidget *lblNwDays, *btnReset;

  lblNwDays = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelNwDays")); 
  btnReset = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonReset"));  
  if((index>=0) && (index<g_list_length (data->calendars))) {
     l = g_list_nth (data->calendars, index);
     cal = (calendar *) l->data;
     nb = g_list_length (cal->list);
     gtk_label_set_text (GTK_LABEL(lblNwDays), g_strdup_printf ("%d", nb));
  }/* endif index */
  if(nb==0)
      gtk_widget_set_sensitive (GTK_WIDGET(btnReset), FALSE);
  else
      gtk_widget_set_sensitive (GTK_WIDGET(btnReset), TRUE);

}

/*******************************
  PUBLIC : get index for 
 current calndar (index in GList
 of calendars, not calendar's ID
********************************/
gint calendars_get_current_calendar_index (APP_data *data)
{
  GtkTreePath *path_tree;
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  gint *indices, true_index = -1;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(data->treeViewCalendars));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  if(gtk_tree_selection_get_selected (GTK_TREE_SELECTION(selection), &model, &iter)) {
      gtk_tree_model_get (model, &iter, -1);
      /* read path  */
      path_tree = gtk_tree_model_get_path (model, &iter);
      indices = gtk_tree_path_get_indices (path_tree);
      true_index = 0;/* default, main calendar */
      if(gtk_tree_path_get_depth (path_tree)>1)
         true_index = indices[1]+1;
      /* now, we know rank of calendar */
      gtk_tree_path_free (path_tree);
  }/* endif selection */
  else 
     printf ("* No calendar selected ! *\n");

  return true_index;
}

/***********************************
  LOCAL : mark all days 
  for user defined period and
  current user choice or marking
***********************************/
static void calendar_mark_days_for_period (gint index, gint type, GDate *start_date, GDate *end_date, APP_data *data)
{
  GList *l, *l1;
  calendar *cal;
  calendar_element *element;
  gint i, j, ret, cmp_dates;
  guint day, month, year;

  if((index<0) || (index>g_list_length(data->calendars)-1))
     return;
  GtkWidget *wcalendar = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "calendar1"));
  gtk_widget_hide (GTK_WIDGET (wcalendar));
  /* get a pointer on calendar */
  l = g_list_nth (data->calendars, index);
  cal = (calendar *) l->data;
  cmp_dates = g_date_days_between (start_date, end_date)+1;
  /* do loop according to nth of days */
  for(i=0; i<cmp_dates; i++) {
     day = g_date_get_day (start_date);
     month = g_date_get_month (start_date);
     year = g_date_get_year (start_date);
     ret = -1;
     j = 0;
     /* if date is already marked, we only have to set the value */
     while((j<g_list_length (cal->list)) && (ret<0)) {
		l1 = g_list_nth (cal->list, j);
		element = (calendar_element *) l1;
                if((element->day == day) && (element->month == month) && (element->year==year)){
                   element->type = type;
                   ret = 1;
                }/* endif element */
		j++;
     }/* wend */
     /* in othe case, we set up a new date */
     if(ret<0) {
		    element = g_malloc (sizeof (calendar_element));
		    element->day = day;
		    element->month = month;/* yes, for gtk calendars, month are from 0 to 11 */
		    element->year = year;
		    element->type = type;  
		    cal->list = g_list_append (cal->list, element);
     }/* add a new element */
     g_date_add_days (start_date, 1);
  }/* next i */

  gtk_widget_show (GTK_WIDGET (wcalendar));
  calendars_update_number_nw_days (index, data);
}

/***********************************
  LOCAL : mark all days 
  for project's period and
  current user choice or marking
***********************************/
static void calendar_mark_days_of_week_for_project (gint index, gint day_of_week, gint type, APP_data *data)
{
  GDate start_date, end_date;
  gint i, cmp_dates, ret, j;
  guint weekday, day, month, year;
  GList *l, *l1;
  calendar *cal;
  calendar_element *element;

  if((index<0) || (index>g_list_length(data->calendars)-1))
     return;
  GtkWidget *wcalendar = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "calendar1"));
  gtk_widget_hide (GTK_WIDGET (wcalendar));
  /* get a pointer on calendar */
  l = g_list_nth (data->calendars, index);
  cal = (calendar *) l->data;

  /* we get start and end dates and convert them to GDates */
  g_date_set_dmy (&start_date, data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
  g_date_set_dmy (&end_date, data->properties.end_nthDay, data->properties.end_nthMonth, data->properties.end_nthYear);
  cmp_dates = g_date_days_between (&start_date, &end_date)+1;
  /* do loop according to nth of days */
  for(i=0; i<cmp_dates; i++) {
     day = g_date_get_day (&start_date);
     weekday = g_date_get_weekday (&start_date);
     month = g_date_get_month (&start_date);
     year = g_date_get_year (&start_date);
     ret = -1;
     j = 0;
     if(weekday == day_of_week) {
	     /* if date is already marked, we only have to set the value */
	     while((j<g_list_length (cal->list)) && (ret<0)) {
		l1 = g_list_nth (cal->list, j);
		element = (calendar_element *) l1;
                if((element->day == day) && (element->month == month) && (element->year==year)){
                   element->type = type;
                   ret = 1;
                }/* endif element */
		j++;
	     }/* wend */
	     /* in othe case, we set up a new date */
             if(ret<0) {
		    element = g_malloc (sizeof (calendar_element));
		    element->day = day;
		    element->month = month;/* yes, for gtk calendars, month are from 0 to 11 */
		    element->year = year;
		    element->type = type;  
		    cal->list = g_list_append (cal->list, element);
             }/* add a new element */
     }/* endif day_of_week */
     g_date_add_days (&start_date, 1);
  }/* next i */
  gtk_widget_show (GTK_WIDGET (wcalendar));
  calendars_update_number_nw_days (index, data);
}

/***********************************
  LOCAL : mark (bold) all current
  month's days in order to visually
  see days of current month
***********************************/
static void calendar_mark_days_of_month (GtkCalendar *calendar)
{
  gint day, month, t_month, year;
  GDate date;

  /* we set marks for current month */
  g_object_get (G_OBJECT(calendar), "month", &month, "year", &year, NULL);
  month = month+1;/* yes, for gtk calendars, month are from 0 to 11 */
  t_month = month;
  day = 1;

  g_date_set_dmy (&date, 1, month, year);
  while(t_month==month) {
    gtk_calendar_mark_day (GTK_CALENDAR(calendar), day);
    g_date_add_days (&date, 1);
    t_month = g_date_get_month (&date);
    day++;
  }

}

/************************************
  LOCAL : ADD a date "marked"
  index = nth of calendar
  fDisplay : flag for displaying
************************************/
void calendars_add_new_marked_date (gboolean fDisplay, gint index, guint day, guint month, guint year, gint type, APP_data *data)
{
  GList *l, *l1;
  calendar *cal;
  calendar_element *element;

  if((index>=0) && (index<g_list_length (data->calendars))) {
    l = g_list_nth (data->calendars, index);
    cal = (calendar *) l->data;
    element = g_malloc (sizeof (calendar_element));
    element->day = day;
    element->month = month;
    element->year = year;
    element->type = type;  
    cal->list = g_list_append (cal->list, element);
    if(fDisplay)
        calendars_update_number_nw_days (index, data);
  }/* endif index */
}

/************************************
  LOCAL : remove a date "marked"
  index = nth of calendar
  rank : nth of date inside calendar
************************************/
static void calendars_remove_marked_date (gint index, gint rank, APP_data *data)
{
  GList *l, *l1;
  calendar *cal;

  if((index>=0) && (index<g_list_length (data->calendars))) {
    l = g_list_nth (data->calendars, index);
    cal = (calendar *) l->data;
    if(cal->list!=NULL) {
      if(rank<g_list_length (cal->list)) {
          l1 = g_list_nth (cal->list, rank);
          cal->list = g_list_delete_link (cal->list, l1);
      }/* endif length */
      calendars_update_number_nw_days (index, data);
    }/* endif cal list */
  }/* endif index */

}

/************************************
  LOCAL : remove ALL date "marked"
  index = nth of calendar
************************************/
static void calendars_remove_all_marked_dates (gint index, APP_data *data)
{
  GList *l, *l1;
  calendar *cal;

  if((index>=0) && (index<g_list_length (data->calendars))) {
    l = g_list_nth (data->calendars, index);
    cal = (calendar *) l->data;
    if(cal->list!=NULL) {
       g_list_free (cal->list);/* easy, nothing inside is dynamically allocated */
       cal->list = NULL;
    }/* endif cal list */
  }/* endif index */

}

/************************************
  PUBLIC : test if current date is
  marked or not
  returns -1 if the day isn't marked.
  In other cases, returns rank in GList
  for fast access
****************************************/
gint calendars_date_is_marked (gint day, gint month, gint year, gint index, APP_data *data)
{
  GList *l, *l1;
  calendar *cal;
  calendar_element *element;
  gint ret = -1, i;

  if((index>=0) && (index<g_list_length (data->calendars))) {
    l = g_list_nth (data->calendars, index);
    cal = (calendar *) l->data;
    if(cal->list!=NULL) {
       for(i=0;i<g_list_length (cal->list);i++) {
          l1 = g_list_nth (cal->list, i);
          element = (calendar_element *) l1->data;
          if( (element->day == day) && (element->month == month) && (element->year == year))
                ret = element->type;
       }/* next */
    }/* endif cal list */
  }/* endif index */
  return ret;
}
/************************************
  LOCAL : returns a valid identifier
  for a new calendar 
************************************/
static gint calendars_new_id (APP_data *data)
{
  gint ret = 0, i;
  GList *l;
  calendar *cal;

  for(i=0; i<g_list_length (data->calendars); i++) {
    l = g_list_nth (data->calendars, i);
    cal = (calendar *) l->data;
    if(cal->id > ret)
        ret = cal->id;
  }
  return ret+1;
}

/************************************
  PUBLIC : returns a valid identifier
  for nth calendar 
************************************/
gint calendars_get_id (gint num, APP_data *data)
{
  gint ret = 0;
  GList *l;
  calendar *cal;

  if(num>=0 && num<g_list_length (data->calendars)) {
    l = g_list_nth (data->calendars, num);
    cal = (calendar *) l->data;
    ret = cal->id;
  }
  return ret;
}

/************************************
  PUBLIC : returns a valid nth
  for an Id
************************************/
gint calendars_get_nth_for_id (gint id, APP_data *data)
{
  gint ret = 0, i = 0;
  GList *l;
  calendar *cal;

  while(i<g_list_length (data->calendars)) {
    l = g_list_nth (data->calendars, i);
    cal = (calendar *) l->data;
    if(cal->id ==id)
       ret = i;
    i++;
  }/* wend */
  return ret;
}
/**********************************
  PUBLIC : get calendar name
  
**********************************/
const gchar *calendar_get_name (gint num, APP_data *data)
{
  GList *l;
  calendar *cal;

  if(num>=0 && num<g_list_length (data->calendars)) {
    l = g_list_nth (data->calendars, num);
    cal = (calendar *) l->data;
    return cal->name;
  }
  else
     return NULL;
}

/**********************************
  PUBLIC : set calendar name
  the name must be freed
**********************************/
void calendar_set_name (gint num, gchar *str, APP_data *data)
{
  GList *l;
  calendar *cal;

  /* >0 because we can't change main camendar name */
  if(num>0 && num<g_list_length (data->calendars)) {
    l = g_list_nth (data->calendars, num);
    cal = (calendar *) l->data;
    cal->name = g_strdup_printf ("%s", str);
  }/* endif */
}
/**********************************
  PUBLIC : add a new calendar to
  database
  flag clone if is requested
 duplication of  datas from main calendar
************************************************/
void calendars_new_calendar (gchar *str, gboolean clone, APP_data *data)
{
  calendar *cal, *cal_main;
  calendar_element *element;
  gint i, j, total;
  GList *l;
  cal = g_malloc (sizeof (calendar));

  cal->id = calendars_new_id (data);
  cal->list = NULL;
  cal->name = g_strdup_printf ("%s", str); 
  for(i=0;i<7;i++) {
    total = 510; /* 510 minutes = 8:30 */
    for(j=0;j<4;j++) {
      cal->fDays[i][j] = FALSE;
      cal->schedules[i][j][0] = total;
      cal->schedules[i][j][1] = total+120;
      total = total+180;
    }
  }
  data->calendars = g_list_append (data->calendars, cal);
  /* duplicate datas ? */
  if(clone) {
     l = g_list_nth (data->calendars, 0);
     cal_main = (calendar *) l->data;
     cal->list = g_list_copy (cal_main->list);
  }
}

/**********************************
  PUBLIC : add a new calendar to
  database, for example when we
  read from file ; at this step, 
  the list of marked daus is empty
**********************************/
void calendars_add_calendar (gint id, gchar *str, gboolean fDays[7][4], gint schedules[7][4][2], APP_data *data)
{
  calendar *cal;
  gint i, j;
  cal = g_malloc (sizeof (calendar));

  cal->id = id;
  cal->list = NULL;
  cal->name = g_strdup_printf ("%s", str); 
  for(i=0;i<7;i++) {
     for(j=0;j<4;j++) {
       cal->fDays[i][j] = fDays[i][j];
       cal->schedules[i][j][0] = schedules[i][j][0];
       cal->schedules[i][j][1] = schedules[i][j][1];
     }
  }
  data->calendars = g_list_append (data->calendars, cal);
}

/**********************************
  PUBLIC : remove a calendar at
  position
**********************************/
void calendars_remove_calendar (gint position, APP_data *data)
{
  GList *l, *l1;
  calendar *cal;
  rsc_datas *rsc;
  gint i;

  if((position>0) && (position<g_list_length (data->calendars))) {/* because position == 0 for Main calendar */
	l = g_list_nth (data->calendars, position);
	cal = (calendar *) l->data;    
	if(cal->name!=NULL)
		g_free (cal->name);
	if(cal->list!=NULL)
		g_list_free (cal->list);/* easy, nothing inside is dynamically allocated */
        /* now we remove all calendar's reference from ressources */
        for(i=0; i<g_list_length (data->rscList); i++) {
           l1 = g_list_nth (data->rscList, i);
           rsc = (rsc_datas *) l1->data;
           if(rsc->calendar == cal->id)
               rsc->calendar = 0; /* ID for default calendar */
        }/* next i */
	data->calendars = g_list_delete_link (data->calendars, l);
  }/* endif */
}

/************************************
 PUBLIC :
 callbacks for adjustments 
************************************/
/* period #1 starting time */
void on_spin_p1s_hour_changed (GtkSpinButton *spin_button, APP_data *data)
{
  GtkWidget *spinbuttonHP1S, *spinbuttonMP1S, *spinbuttonHP1E, *spinbuttonMP1E;
  gint hour, minutes, hour_lim, min_lim, total_min1, total_min2;
  calendar *cal;
  GList *l;
  gint index; 

  if(fStopEmit)
     return;
  spinbuttonHP1S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP1S"));
  spinbuttonMP1S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP1S"));
  spinbuttonHP1E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP1E"));
  spinbuttonMP1E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP1E"));

  hour = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP1S));
  minutes = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP1S));
  hour_lim = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP1E));
  min_lim = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP1E));

  total_min1 = (hour*60)+minutes;
  total_min2 = (hour_lim*60)+min_lim;

  /* we constraint time */
  if(total_min1>=total_min2) {
    total_min1 = total_min2-1;
    hour = total_min1/60;
    minutes = total_min1 % 60;
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonHP1S), hour);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonMP1S), minutes);
  }

  /* we store in datas */
  index = calendars_get_current_calendar_index (data);
  if(index>=0) {
    l = g_list_nth (data->calendars, index);
    cal = (calendar *) l->data;
    cal->schedules[day_of_week][0][0] = (60*hour)+minutes;
  }

}

/* period #1 ending time */

void on_spin_p1e_hour_changed (GtkSpinButton *spin_button, APP_data *data)
{
  GtkWidget *spinbuttonHP1S, *spinbuttonMP1S, *spinbuttonHP1E, *spinbuttonMP1E, *sw2;
  GtkWidget *spinbuttonHP2S, *spinbuttonMP2S;
  gint hour, minutes, hour_lim, min_lim, total_min1, total_min2, hour_p2, min_p2, total_p2;
  calendar *cal;
  GList *l;
  gint index; 
 
  if(fStopEmit)
     return;

  spinbuttonHP1S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP1S"));
  spinbuttonMP1S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP1S"));
  spinbuttonHP1E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP1E"));
  spinbuttonMP1E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP1E"));
  sw2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP2"));
  spinbuttonHP2S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP2S"));
  spinbuttonMP2S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP2S"));

  hour_lim = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP1S));
  min_lim = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP1S));
  hour = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP1E));
  minutes = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP1E));

  total_min1 = (hour*60)+minutes;
  total_min2 = (hour_lim*60)+min_lim;

  /* first constraint : ending time>starting time */
  if(total_min1<=total_min2) {
    total_min1 = total_min2+1;
    hour = total_min1/60;
    minutes = total_min1 % 60;
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonHP1E), hour);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonMP1E), minutes);
  }
  /* second constraint : endingtime < starting time period #2 ONLY if period #2 is active */
  if(gtk_switch_get_active (GTK_SWITCH(sw2))) {
     hour_p2 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP2S));
     min_p2 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP2S));
     total_p2 = (hour_p2*60)+min_p2;
     if(total_min1>=total_p2) {
        total_min1=total_p2-1;
        hour = total_min1/60;
        minutes = total_min1 % 60;
        gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonHP1E), hour);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonMP1E), minutes);
     }
  }

  /* we store in datas */
  index = calendars_get_current_calendar_index (data);
  if(index>=0) {
    l = g_list_nth (data->calendars, index);
    cal = (calendar *) l->data;
    cal->schedules[day_of_week][0][1] = (60*hour)+minutes;
  }
}

/* period #2 starting time */
void on_spin_p2s_hour_changed (GtkSpinButton *spin_button, APP_data *data)
{
  GtkWidget *spinbuttonHP1E, *spinbuttonMP1E;
  GtkWidget *spinbuttonHP2S, *spinbuttonMP2S, *spinbuttonHP2E, *spinbuttonMP2E;
  gint hour, minutes, hour_lim, min_lim, total, total_lim, hour_p1, min_p1, total_p1;
  calendar *cal;
  GList *l;
  gint index; 

  if(fStopEmit)
     return;

  spinbuttonHP2E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP2E"));
  spinbuttonMP2E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP2E"));
  spinbuttonHP1E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP1E"));
  spinbuttonMP1E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP1E"));
  spinbuttonHP2S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP2S"));
  spinbuttonMP2S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP2S"));

  hour = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP2S));
  minutes = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP2S));
  total = (hour*60)+minutes;

  hour_p1 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP1E));
  min_p1 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP1E));
  total_p1 = (hour_p1*60)+min_p1;

  hour_lim = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP2E));
  min_lim= (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP2E));
  total_lim = (hour_lim*60)+min_lim;

  /* constraint #1 : > time of period #1 ending */
  if(total<total_p1) {
     total = total_p1+1;
     hour = total/60;
     minutes = total % 60;
     gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonHP2S), hour);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonMP2S), minutes);
  }
  /* constraint #2 : time< period #2 ending */
  if(total>total_lim) {
     total = total_lim-1;
     hour = total/60;
     minutes = total % 60;
     gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonHP2S), hour);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonMP2S), minutes);
  }
  /* we store in datas */
  index = calendars_get_current_calendar_index (data);
  if(index>=0) {
    l = g_list_nth (data->calendars, index);
    cal = (calendar *) l->data;
    cal->schedules[day_of_week][1][0] = (60*hour)+minutes;
  }
}

/* period #2 ending time */
void on_spin_p2e_hour_changed (GtkSpinButton *spin_button, APP_data *data)
{
  GtkWidget *spinbuttonHP2S, *spinbuttonMP2S, *spinbuttonHP2E, *spinbuttonMP2E, *spinbuttonHP3S, *spinbuttonMP3S, *sw3;
  gint hour, minutes, total, hour_lim, min_lim, total_lim, hour_p2, min_p2, total_p2;
  calendar *cal;
  GList *l;
  gint index; 

  if(fStopEmit)
     return;

  spinbuttonHP2E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP2E"));
  spinbuttonMP2E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP2E"));
  spinbuttonHP2S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP2S"));
  spinbuttonMP2S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP2S"));
  spinbuttonHP3S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP3S"));
  spinbuttonMP3S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP3S"));
  sw3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP3"));

  hour = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP2E));
  minutes = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP2E));
  total = (hour*60)+minutes;

  hour_p2 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP2S));
  min_p2 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP2S));
  total_p2 = (hour_p2*60)+min_p2;

  hour_lim = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP3S));
  min_lim= (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP3S));
  total_lim = (hour_lim*60)+min_lim;

  /* constraint 1 : p2 end > p2 start */
  if(total<total_p2) {
     total = total_p2+1;
     hour = total/60;
     minutes = total % 60;
     gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonHP2E), hour);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonMP2E), minutes);
  }
  /* constraint 2 : IF period 3 is active, then p2 end < p3 start */
  if(gtk_switch_get_active (GTK_SWITCH(sw3))) {
     if(total>total_lim) {
        total = total_lim-1;
        hour = total/60;
        minutes = total % 60;
        gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonHP2E), hour);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonMP2E), minutes);
     }
  }
  /* we store in datas */
  index = calendars_get_current_calendar_index (data);
  if(index>=0) {
    l = g_list_nth (data->calendars, index);
    cal = (calendar *) l->data;
    cal->schedules[day_of_week][1][1] = (60*hour)+minutes;
  }
}

/* period #3 starting time */
void on_spin_p3s_hour_changed (GtkSpinButton *spin_button, APP_data *data)
{
  GtkWidget *spinbuttonHP2E, *spinbuttonMP2E, *spinbuttonHP3E, *spinbuttonMP3E, *spinbuttonHP3S, *spinbuttonMP3S;
  gint hour, minutes, total, hour_lim, min_lim, total_lim, hour_p2, min_p2, total_p2;
  calendar *cal;
  GList *l;
  gint index; 

  if(fStopEmit)
     return;

  spinbuttonHP2E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP2E"));
  spinbuttonMP2E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP2E"));
  spinbuttonHP3S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP3S"));
  spinbuttonMP3S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP3S"));
  spinbuttonHP3E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP3E"));
  spinbuttonMP3E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP3E"));

  hour = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP3S));
  minutes = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP3S));
  total = (hour*60)+minutes;

  hour_p2 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP2E));
  min_p2 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP2E));
  total_p2 = (hour_p2*60)+min_p2;

  hour_lim = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP3E));
  min_lim= (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP3E));
  total_lim = (hour_lim*60)+min_lim;

  /* constraint 1 : p3 start > p2 end */
  if(total<total_p2) {
     total = total_p2+1;
     hour = total/60;
     minutes = total % 60;
     gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonHP3S), hour);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonMP3S), minutes);
  }

  /* constraint 1 : p3 start < p3 end */
  if(total>total_lim) {
     total = total_lim-1;
     hour = total/60;
     minutes = total % 60;
     gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonHP3S), hour);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonMP3S), minutes);
  }
  /* we store in datas */
  index = calendars_get_current_calendar_index (data);
  if(index>=0) {
    l = g_list_nth (data->calendars, index);
    cal = (calendar *) l->data;
    cal->schedules[day_of_week][2][0] = (60*hour)+minutes;
  }
}


/* period #3 ending time */
void on_spin_p3e_hour_changed (GtkSpinButton *spin_button, APP_data *data)
{
  GtkWidget *spinbuttonHP3E, *spinbuttonMP3E, *spinbuttonHP3S, *spinbuttonMP3S, *spinbuttonHP4S, *spinbuttonMP4S, *sw4;
  gint hour, minutes, total, hour_lim, min_lim, total_lim, hour_p3, min_p3, total_p3;
  calendar *cal;
  GList *l;
  gint index; 

  if(fStopEmit)
     return;

  spinbuttonHP3S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP3S"));
  spinbuttonMP3S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP3S"));
  spinbuttonHP3E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP3E"));
  spinbuttonMP3E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP3E"));
  spinbuttonHP4S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP4S"));
  spinbuttonMP4S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP4S"));
  sw4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP4"));

  hour = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP3E));
  minutes = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP3E));
  total = (hour*60)+minutes;

  hour_p3 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP3S));
  min_p3 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP3S));
  total_p3 = (hour_p3*60)+min_p3;

  hour_lim = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP4S));
  min_lim= (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP4S));
  total_lim = (hour_lim*60)+min_lim;

  /* constraint 1 : p3 end > p3 start */
  if(total<total_p3) {
     total = total_p3+1;
     hour = total/60;
     minutes = total % 60;
     gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonHP3E), hour);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonMP3E), minutes);
  }

  /* constraint 2 : IF period 3 is active, then p2 end < p3 start */
  if(gtk_switch_get_active (GTK_SWITCH(sw4))) {
     if(total>total_lim) {
        total = total_lim-1;
        hour = total/60;
        minutes = total % 60;
        gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonHP3E), hour);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonMP3E), minutes);
     }
  }
  /* we store in datas */
  index = calendars_get_current_calendar_index (data);
  if(index>=0) {
    l = g_list_nth (data->calendars, index);
    cal = (calendar *) l->data;
    cal->schedules[day_of_week][2][1] = (60*hour)+minutes;
  }
}

/* period #4 starting time */
void on_spin_p4s_hour_changed (GtkSpinButton *spin_button, APP_data *data)
{
  GtkWidget *spinbuttonHP3E, *spinbuttonMP3E, *spinbuttonHP4E, *spinbuttonMP4E, *spinbuttonHP4S, *spinbuttonMP4S;
  gint hour, minutes, total, hour_lim, min_lim, total_lim, hour_p3, min_p3, total_p3;
  calendar *cal;
  GList *l;
  gint index; 

  if(fStopEmit)
     return;

  spinbuttonHP3E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP3E"));
  spinbuttonMP3E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP3E"));
  spinbuttonHP4S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP4S"));
  spinbuttonMP4S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP4S"));
  spinbuttonHP4E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP4E"));
  spinbuttonMP4E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP4E"));

  hour = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP4S));
  minutes = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP4S));
  total = (hour*60)+minutes;

  hour_p3 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP3E));
  min_p3 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP3E));
  total_p3 = (hour_p3*60)+min_p3;

  hour_lim = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP4E));
  min_lim= (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP4E));
  total_lim = (hour_lim*60)+min_lim;

  /* constraint 1 : p4 start > p3 end */
  if(total<total_p3) {
     total = total_p3+1;
     hour = total/60;
     minutes = total % 60;
     gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonHP4S), hour);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonMP4S), minutes);
  }

  /* constraint 1 : p4 start < p4 end */
  if(total>total_lim) {
     total = total_lim-1;
     hour = total/60;
     minutes = total % 60;
     gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonHP4S), hour);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonMP4S), minutes);
  }
  /* we store in datas */
  index = calendars_get_current_calendar_index (data);
  if(index>=0) {
    l = g_list_nth (data->calendars, index);
    cal = (calendar *) l->data;
    cal->schedules[day_of_week][3][0] = (60*hour)+minutes;
  }
}

/* period #4 ending time */
void on_spin_p4e_hour_changed (GtkSpinButton *spin_button, APP_data *data)
{
  GtkWidget *spinbuttonHP4E, *spinbuttonMP4E, *spinbuttonHP4S, *spinbuttonMP4S;
  gint hour, minutes, total, hour_p4, min_p4, total_p4;
  calendar *cal;
  GList *l;
  gint index; 

  if(fStopEmit)
     return;

  spinbuttonHP4S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP4S"));
  spinbuttonMP4S = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP4S"));
  spinbuttonHP4E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP4E"));
  spinbuttonMP4E = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP4E"));

  hour = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP4E));
  minutes = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP4E));
  total = (hour*60)+minutes;

  hour_p4 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonHP4S));
  min_p4 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonMP4S));
  total_p4 = (hour_p4*60)+min_p4;

  /* only one test, p4 end > p4 start */
  if(total<total_p4) {
     total = total_p4+1;
     hour = total/60;
     minutes = total % 60;
     gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonHP4E), hour);
     gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinbuttonMP4E), minutes);
  }
  /* we store in datas */
  index = calendars_get_current_calendar_index (data);
  if(index>=0) {
    l = g_list_nth (data->calendars, index);
    cal = (calendar *) l->data;
    cal->schedules[day_of_week][3][1] = (60*hour)+minutes;
  }
}

/**********************************
  PUBLIC : change schedules
  argument : num of calendar,
  rank of day,
  starting grom 0 (zero) for monday 
************************************/
void calendars_update_schedules (gint num, gint day, APP_data *data)
{
  GtkWidget *sw1, *sw2, *sw3, *sw4, *lb1, *lb2, *lb3, *lb4;
  GtkWidget *sphs1, *sphs2, *sphs3, *sphs4;
  GtkWidget *spms1, *spms2, *spms3, *spms4;
  GtkWidget *sphe1, *sphe2, *sphe3, *sphe4;
  GtkWidget *spme1, *spme2, *spme3, *spme4;
  gint index = 0, hours, minutes;/* default calendar */
  GList *l;
  calendar *cal;
  gboolean fsensible;

  
   if((day<0) || (day>6))
     return;

   fStopEmit = TRUE;/* inhibit call backs */

   sw1 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP1"));
   sw2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP2"));
   sw3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP3"));
   sw4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP4"));

   lb1 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelp1"));
   lb2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelp2"));
   lb3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelp3"));
   lb4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelp4"));

   sphs1 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP1S"));
   sphs2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP2S"));
   sphs3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP3S"));
   sphs4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP4S"));

   spms1 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP1S"));
   spms2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP2S"));
   spms3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP3S"));
   spms4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP4S"));

   sphe1 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP1E"));
   sphe2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP2E"));
   sphe3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP3E"));
   sphe4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP4E"));

   spme1 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP1E"));
   spme2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP2E"));
   spme3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP3E"));
   spme4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP4E"));

   /* we access to current calendar */
   if(num>=0) {
     index = num;
   }
   else
     printf("Update schedule forcé calendrier à 0\n");

   /* we access to datas - another useless checking, sorry dear reader */
   if(index<g_list_length (data->calendars)) {
      l = g_list_nth (data->calendars, index);
      cal = (calendar *) l->data;
      /* now we adjust displays - period 1*/
      fsensible = cal->fDays[day][0];/* period #1 of current day */
      gtk_widget_set_sensitive (GTK_WIDGET(lb1), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(sphs1), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(spms1), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(sphe1), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(spme1), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(sw1), TRUE);
      gtk_switch_set_active (GTK_SWITCH(sw1), fsensible);
      /* set values of spin buttons, period 1 */
      hours = cal->schedules[day][0][0]/60;
      minutes = cal->schedules[day][0][0]%60;
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(sphs1), hours);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(spms1), minutes);
      hours = cal->schedules[day][0][1]/60;
      minutes = cal->schedules[day][0][1]%60;
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(sphe1), hours);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(spme1), minutes);

      /* now we adjust displays - period 1*/
      fsensible = cal->fDays[day][1];/* period #2 of current day */
      gtk_widget_set_sensitive (GTK_WIDGET(lb2), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(sphs2), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(spms2), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(sphe2), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(spme2), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(sw2), TRUE);
      gtk_switch_set_active (GTK_SWITCH(sw2), fsensible);
      /* set values of spin buttons, period 2 */
      hours = cal->schedules[day][1][0]/60;
      minutes = cal->schedules[day][1][0]%60;
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(sphs2), hours);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(spms2), minutes);
      hours = cal->schedules[day][1][1]/60;
      minutes = cal->schedules[day][1][1]%60;
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(sphe2), hours);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(spme2), minutes);

      /* now we adjust displays - period 3*/
      fsensible = cal->fDays[day][2];/* period #2 of current day */
      gtk_widget_set_sensitive (GTK_WIDGET(lb3), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(sphs3), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(spms3), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(sphe3), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(spme3), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(sw3), TRUE);
      gtk_switch_set_active (GTK_SWITCH(sw3), fsensible);
      /* set values of spin buttons, period 3 */
      hours = cal->schedules[day][2][0]/60;
      minutes = cal->schedules[day][2][0]%60;
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(sphs3), hours);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(spms3), minutes);
      hours = cal->schedules[day][2][1]/60;
      minutes = cal->schedules[day][2][1]%60;
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(sphe3), hours);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(spme3), minutes);
      /* now we adjust displays - period 4*/
      fsensible = cal->fDays[day][3];/* period #2 of current day */
      gtk_widget_set_sensitive (GTK_WIDGET(lb4), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(sphs4), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(spms4), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(sphe4), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(spme4), fsensible);
      gtk_widget_set_sensitive (GTK_WIDGET(sw4), TRUE);
      gtk_switch_set_active (GTK_SWITCH(sw4), fsensible);
      /* set values of spin buttons, period 4 */
      hours = cal->schedules[day][3][0]/60;
      minutes = cal->schedules[day][3][0]%60;
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(sphs4), hours);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(spms4), minutes);
      hours = cal->schedules[day][3][1]/60;
      minutes = cal->schedules[day][3][1]%60;
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(sphe4), hours);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(spme4), minutes);
   }
   fStopEmit = FALSE; /* enable call backs */
}

/************************************
  PUBLIC callback : day has changed
************************************/
void on_combo_day_changed (GtkComboBox *widget, APP_data *data)
{
  day_of_week = gtk_combo_box_get_active (GTK_COMBO_BOX(widget));
  calendars_update_schedules (calendars_get_current_calendar_index (data), day_of_week, data);
}

/************************************
  PUBLIC :
 callbacks for switches 
 Glade and gtkSwtches are very
  uncommon to set-up, see here :
https://stackoverflow.com/questions/7120940/gtk-switch-activate-signal-not-firing answer n°5
but we have only to go to GObject, the notify signal is present
BE VERY CAREFULL : i's a GObject signal, not a widget signal !
************************************/

void on_switch_period1_event (GObject *widget, GParamSpec *pspec, APP_data *data)
{
   GtkWidget *sw2, *sw3, *sw4, *lb1, *lb2, *lb3, *lb4;
   GtkWidget *sphs1, *sphs2, *sphs3, *sphs4;
   GtkWidget *spms1, *spms2, *spms3, *spms4;
   GtkWidget *sphe1, *sphe2, *sphe3, *sphe4;
   GtkWidget *spme1, *spme2, *spme3, *spme4;
   gint index;
   GList *l;
   calendar *cal;

   index = calendars_get_current_calendar_index (data);
   if(index<0)
      return;
   l = g_list_nth (data->calendars, index);
   cal = (calendar *) l->data;

   sw2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP2"));
   sw3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP3"));
   sw4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP4"));

   lb1 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelp1"));
   lb2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelp2"));
   lb3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelp3"));
   lb4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelp4"));

   sphs1 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP1S"));
   sphs2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP2S"));
   sphs3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP3S"));
   sphs4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP4S"));

   spms1 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP1S"));
   spms2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP2S"));
   spms3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP3S"));
   spms4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP4S"));

   sphe1 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP1E"));
   sphe2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP2E"));
   sphe3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP3E"));
   sphe4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP4E"));

   spme1 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP1E"));
   spme2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP2E"));
   spme3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP3E"));
   spme4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP4E"));

   if(gtk_switch_get_active (GTK_SWITCH(widget))) {
       gtk_widget_set_sensitive (GTK_WIDGET(lb1), TRUE);
       gtk_widget_set_sensitive (GTK_WIDGET(sphs1), TRUE);
       gtk_widget_set_sensitive (GTK_WIDGET(spms1), TRUE);
       gtk_widget_set_sensitive (GTK_WIDGET(sphe1), TRUE);
       gtk_widget_set_sensitive (GTK_WIDGET(spme1), TRUE);
       /* we store new state */
       cal->fDays[day_of_week][0] = TRUE;
   }
   else {/* if switch #1 is inactive, we must inactivate also switches #2, #3 & #4 */
      gtk_switch_set_active (GTK_SWITCH(sw2), FALSE);
      gtk_switch_set_active (GTK_SWITCH(sw3), FALSE);
      gtk_switch_set_active (GTK_SWITCH(sw4), FALSE);
      /* and inhibit widgets */
      gtk_widget_set_sensitive (GTK_WIDGET(lb1), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(lb2), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(lb3), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(lb4), FALSE);
      /* starting hours */
      gtk_widget_set_sensitive (GTK_WIDGET(sphs1), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(sphs2), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(sphs2), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(sphs2), FALSE);
      /* starting minutes */
      gtk_widget_set_sensitive (GTK_WIDGET(spms1), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(spms2), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(spms3), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(spms4), FALSE);
      /* ending hours */
      gtk_widget_set_sensitive (GTK_WIDGET(sphe1), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(sphe2), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(sphe3), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(sphe4), FALSE);
      /* ending minutes */
      gtk_widget_set_sensitive (GTK_WIDGET(spme1), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(spme2), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(spme3), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(spme4), FALSE);
      /* we store new state */
      cal->fDays[day_of_week][0] = FALSE;
   }
}

void on_switch_period2_event (GObject *widget, GParamSpec *pspec, APP_data *data)
{
   GtkWidget *sw1, *sw2, *sw3, *sw4, *lb2, *lb3, *lb4;
   GtkWidget *sphs2, *sphs3, *sphs4;
   GtkWidget *spms2, *spms3, *spms4;
   GtkWidget *sphe2, *sphe3, *sphe4;
   GtkWidget *spme2, *spme3, *spme4;
   gint total_time_before, index;
   GList *l;
   calendar *cal;

   index = calendars_get_current_calendar_index (data);
   if(index<0)
      return;
   l = g_list_nth (data->calendars, index);
   cal = (calendar *) l->data;

   sw1 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP1"));
   sw2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP2"));
   sw3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP3"));
   sw4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP4"));
   lb2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelp2"));
   sphs2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP2S"));
   spms2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP2S"));
   sphe2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP2E"));
   spme2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP2E"));

   fStopEmit = TRUE;
   /* we store DEFAULT new state */
   cal->fDays[day_of_week][1] = FALSE;
   if(gtk_switch_get_active (GTK_SWITCH(widget))) {
       /* if switch period #1 isn't active, we must desactivate switch period #2 */
      if(!gtk_switch_get_active (GTK_SWITCH(sw1))) {
          gtk_switch_set_active (GTK_SWITCH(widget), FALSE);
       }
       else {/* test : is there is sufficient room for a new period */
         GtkWidget *sphe1 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP1E"));
         GtkWidget *spme1 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP1E"));
         GtkWidget *boxErrP2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "boxErrP2"));
         gint hour2 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(sphe1));
         gint min2 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spme1));
         total_time_before = (60*hour2)+min2;/* MINUTESBYDAY 23h59 = 1439 minutes */
         if(MINUTESBYDAY-total_time_before > 2) {
           /* we set actives widgets belonging to period #2 */
           gtk_widget_set_sensitive (GTK_WIDGET(lb2), TRUE);
           gtk_widget_set_sensitive (GTK_WIDGET(sphs2), TRUE);
           gtk_widget_set_sensitive (GTK_WIDGET(spms2), TRUE);
           gtk_widget_set_sensitive (GTK_WIDGET(sphe2), TRUE);
           gtk_widget_set_sensitive (GTK_WIDGET(spme2), TRUE);
           /* we hide error warning */
           gtk_widget_hide (GTK_WIDGET(boxErrP2));
           /* we have to clamp time for new period */
           total_time_before++;
           hour2 = total_time_before/60;
           min2 = total_time_before %60;
           gtk_spin_button_set_value (GTK_SPIN_BUTTON(sphs2), hour2);
           gtk_spin_button_set_value (GTK_SPIN_BUTTON(spms2), min2);
           total_time_before++;
           hour2 = total_time_before/60;
           min2 = total_time_before %60;
           gtk_spin_button_set_value (GTK_SPIN_BUTTON(sphe2), hour2);
           gtk_spin_button_set_value (GTK_SPIN_BUTTON(spme2), min2);
           /* we store new state */
           cal->fDays[day_of_week][1] = TRUE;
         } 
         else {/* error message */
            gtk_switch_set_active (GTK_SWITCH(widget), FALSE);
            /* we show error warning */
            gtk_widget_show (GTK_WIDGET(boxErrP2));
         }
       }

   }
   else {/* if switch #2 is inactive, we must inactivate also switches #33 & #4 */
      gtk_switch_set_active (GTK_SWITCH(sw3), FALSE);
      gtk_switch_set_active (GTK_SWITCH(sw4), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(lb2), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(sphs2), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(spms2), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(sphe2), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(spme2), FALSE);
   }
   fStopEmit = FALSE;
}

void on_switch_period3_event (GObject *widget, GParamSpec *pspec, APP_data *data)
{
   GtkWidget *sw2, *sw3, *sw4, *lb3, *lb4, *boxErrP3, *sphe2, *spme2;
   GtkWidget *sphs3, *spms3, *sphe3, *spme3;
   GtkWidget *sphs4, *spms4, *sphe4, *spme4;
   gint total_time_before, hour3, min3;
   gint index;
   GList *l;
   calendar *cal;

   index = calendars_get_current_calendar_index (data);
   if(index<0)
      return;
   l = g_list_nth (data->calendars, index);
   cal = (calendar *) l->data;

   sw2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP2"));
   sw3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP3"));
   sw4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP4"));
   lb3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelp3"));
   lb4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelp4"));

   sphs3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP3S"));
   spms3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP3S"));
   sphe3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP3E"));
   spme3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP3E"));

   sphs4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP4S"));
   spms4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP4S"));
   sphe4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP4E"));
   spme4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP4E"));

   fStopEmit = TRUE;
   /* we store DEFAULT new state */
   cal->fDays[day_of_week][2] = FALSE;

   if(gtk_switch_get_active (GTK_SWITCH(widget))) {
       /* if switch period #2 isn't active, we must desactivate switch period #3 */
       if(!gtk_switch_get_active (GTK_SWITCH(sw2))) {
          gtk_switch_set_active (GTK_SWITCH(widget), FALSE);
       }
       else {/* test : is there is sufficient room for a new period */
         sphe2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP2E"));
         spme2 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP2E"));
         boxErrP3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "boxErrP3"));
         gint hour3 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(sphe2));
         gint min3 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spme2));
         total_time_before = (60*hour3)+min3;/* MINUTESBYDAY 23h59 = 1439 minutes */

         if(MINUTESBYDAY-total_time_before > 2) {
            /* we set actives widgets belonging to period #3 */
            gtk_widget_set_sensitive (GTK_WIDGET(lb3), TRUE);
            gtk_widget_set_sensitive (GTK_WIDGET(sphs3), TRUE);
            gtk_widget_set_sensitive (GTK_WIDGET(spms3), TRUE);
            gtk_widget_set_sensitive (GTK_WIDGET(sphe3), TRUE);
            gtk_widget_set_sensitive (GTK_WIDGET(spme3), TRUE);
            /* we hide error warning */
            gtk_widget_hide (GTK_WIDGET(boxErrP3));
            /* we have to clamp time for new period */
            total_time_before++;
            hour3 = total_time_before/60;
            min3 = total_time_before %60;
            gtk_spin_button_set_value (GTK_SPIN_BUTTON(sphs3), hour3);
            gtk_spin_button_set_value (GTK_SPIN_BUTTON(spms3), min3);
            total_time_before++;
            hour3 = total_time_before/60;
            min3 = total_time_before %60;
            gtk_spin_button_set_value (GTK_SPIN_BUTTON(sphe3), hour3);
            gtk_spin_button_set_value (GTK_SPIN_BUTTON(spme3), min3);
            /* we store new state */
            cal->fDays[day_of_week][2] = TRUE;
         }/* endif total_time */
         else {
           /* we show error warning */
           gtk_widget_show (GTK_WIDGET(boxErrP3));
           gtk_switch_set_active (GTK_SWITCH(widget), FALSE);
         }/* else error */
       }/* else test */
   }
   else {
      gtk_switch_set_active (GTK_SWITCH(sw4), FALSE); 
      /* we set actives widgets belonging to period #3 */
      gtk_widget_set_sensitive (GTK_WIDGET(lb3), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(sphs3), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(spms3), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(sphe3), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(spme3), FALSE);
   }

   fStopEmit = FALSE;
}

void on_switch_period4_event (GObject *widget, GParamSpec *pspec, APP_data *data)
{
   GtkWidget *sw2, *sw3, *sw4, *lb4, *boxErrP4;
   GtkWidget *sphs4, *spms4, *sphe4, *spme4, *sphe3, *spme3;
   gint total_time_before, hour4, min4;
   gint index;
   GList *l;
   calendar *cal;

   index = calendars_get_current_calendar_index (data);
   if(index<0)
      return;
   l = g_list_nth (data->calendars, index);
   cal = (calendar *) l->data;

   sw3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP3"));
   sw4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchP4"));
   lb4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelp4"));
   sphs4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP4S"));
   spms4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP4S"));
   sphe4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP4E"));
   spme4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP4E"));

   fStopEmit = TRUE;
   /* we store DEFAULT new state */
   cal->fDays[day_of_week][3] = FALSE;

   if(gtk_switch_get_active (GTK_SWITCH(widget))) {
       /* if switch period #3 isn't active, we must desactivate switch period #4 */
       if(!gtk_switch_get_active (GTK_SWITCH(sw3)))
          gtk_switch_set_active (GTK_SWITCH(widget) , FALSE);
       else {/* test : is there is sufficient room for a new period */
          boxErrP4 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "boxErrP4"));
          sphe3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHP3E"));
          spme3 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonMP3E"));
          hour4 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(sphe3));
          min4 = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spme3));
          total_time_before = (60*hour4)+min4;/* MINUTESBYDAY 23h59 = 1439 minutes */
          if(MINUTESBYDAY-total_time_before > 2) {
             gtk_widget_set_sensitive (GTK_WIDGET(lb4), TRUE);
             gtk_widget_set_sensitive (GTK_WIDGET(sphs4), TRUE);
             gtk_widget_set_sensitive (GTK_WIDGET(spms4), TRUE);
             gtk_widget_set_sensitive (GTK_WIDGET(sphe4), TRUE);
             gtk_widget_set_sensitive (GTK_WIDGET(spme4), TRUE);
             /* we hide error warning */
             gtk_widget_hide (GTK_WIDGET(boxErrP4));
             /* we have to clamp time for new period */
             total_time_before++;
             hour4 = total_time_before/60;
             min4 = total_time_before %60;
             gtk_spin_button_set_value (GTK_SPIN_BUTTON(sphs4), hour4);
             gtk_spin_button_set_value (GTK_SPIN_BUTTON(spms4), min4);
             total_time_before++;
             hour4 = total_time_before/60;
             min4 = total_time_before %60;
             gtk_spin_button_set_value (GTK_SPIN_BUTTON(sphe4), hour4);
             gtk_spin_button_set_value (GTK_SPIN_BUTTON(spme4), min4);
             /* we store new state */
             cal->fDays[day_of_week][3] = TRUE;
          }/* endif total_time */
          else {
             /* we show error warning */
             gtk_widget_show (GTK_WIDGET(boxErrP4));
             gtk_switch_set_active (GTK_SWITCH(widget), FALSE);
          }
       }
   }
   else { 
      /* we desactivate components */
      gtk_widget_set_sensitive (GTK_WIDGET(lb4), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(sphs4), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(spms4), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(sphe4), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET(spme4), FALSE);
   }
   fStopEmit = FALSE;
}

/************************************
  PUBLIC :
 init MAIN calendar
************************************/
void calendars_init_main_calendar (APP_data *data)
{
  calendar *cal;
  gint i, j, total;
  cal = g_malloc (sizeof (calendar));

  cal->id = 0;
  cal->list = NULL;
  cal->name = g_strdup_printf ("%s", _("Main Calendar"));
  for(i=0;i<7;i++) {
    total = 510; /* 510 minutes = 8:30 */
    for(j=0;j<4;j++) {
      cal->fDays[i][j] = FALSE;
      cal->schedules[i][j][0] = total;
      cal->schedules[i][j][1] = total+120;
      total = total+180;
    }
  }
  data->calendars = g_list_append (data->calendars, cal);

}

/************************************
  PUBLIC :
  remove all calendars from memory
************************************/
void calendars_free_calendars (APP_data *data)
{
  gint i, k;
  GList *l, *l2;
  calendar *cal;
  calendar_element *element;

  for(i=0; i<g_list_length (data->calendars); i++) {
    l = g_list_nth (data->calendars, i);
    cal = (calendar *) l->data;
    if(cal->name!=NULL)
        g_free (cal->name);
    if(cal->list!=NULL) {
        for(k=0;k<g_list_length (cal->list);k++) {
           l2 = g_list_nth (cal->list, k);
           element = (calendar_element *) l2->data;
           g_free (element);
        }
        g_list_free (cal->list);/* easy, nothing inside is dynamically allocated */
    }
  } 

  /* free list itself */
  g_list_free (data->calendars);
  data->calendars = NULL;
}

/**********************************
  Callback : add a new calendar,
  if possible
**********************************/
void on_calendars_add_clicked (GtkButton *button, APP_data *data)
{
  GtkTreeIter iter, child;
  GtkTreeSelection *selection;
  GtkTreePath *path_tree;
  GtkTreeModel *model;
  gint *indices;

  /* get current line for toggling event */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(data->treeViewCalendars) );
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE );

  if( gtk_tree_selection_get_selected (selection, &model, &iter) ) {
	  gtk_tree_model_get (model, &iter, -1);
	  /* read path  */
	  path_tree = gtk_tree_model_get_path (model, &iter);
          indices = gtk_tree_path_get_indices (path_tree);

          if(gtk_tree_path_get_depth (path_tree)>=1) {
             gtk_tree_path_append_index (path_tree, indices[0]);/* mandatory */
             gtk_tree_model_get_iter_first (model, &iter);
             gtk_tree_store_append (GTK_TREE_STORE(model), &child, &iter);
             gtk_tree_store_set (GTK_TREE_STORE(model), &child,
                     CAL_COLUMN, _("New Calendar"),
                     -1);
             calendars_new_calendar (_("New Calendar"), FALSE, data);
          }
          /* never free 'indices' or 'new_text' */	
	  gtk_tree_path_free (path_tree);	
  }/* endif */
}

/**********************************
  Callback : remove a calendar,
  if possible
**********************************/
void on_calendars_remove_clicked (GtkButton *button, APP_data *data)
{
  GtkTreeIter iter, child;
  GtkTreeSelection *selection;
  GtkTreePath *path_tree;
  GtkTreeModel *model;
  gint *indices;
  gboolean flag = TRUE;
  GtkWidget *dialog, *win, *calendar;
  GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;

  win = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "dialogCalendars"));
  dialog = gtk_message_dialog_new (GTK_WINDOW(win), flags,
                    GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL,
                _("Do tou really want to remove this calendar ?\nThis operation isn't cancellable ..."));
  if(gtk_dialog_run (GTK_DIALOG(dialog))!=GTK_RESPONSE_OK)
         flag=FALSE;
  gtk_widget_destroy (GTK_WIDGET(dialog));
  if(!flag)
     return;

  /* get current line for toggling event */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(data->treeViewCalendars));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  if(gtk_tree_selection_get_selected (selection, &model, &iter)) {
	  gtk_tree_model_get (model, &iter, -1);
	  /* read path  */
	  path_tree = gtk_tree_model_get_path (model, &iter);
          indices = gtk_tree_path_get_indices (path_tree);
          if(gtk_tree_path_get_depth (path_tree)>1) {
              gtk_tree_store_remove (GTK_TREE_STORE(model), &iter);
              calendars_remove_calendar (indices[1]+1, data);
              printf("removed ! at indices %d %d \n", indices[0], indices[1]);
          }
          /* never free 'indices' or 'new_text' */
	  gtk_tree_path_free (path_tree);	
  }/* endif */

}

/**********************************
  Callback : clone main calendar,

**********************************/
void on_calendars_clone_clicked (GtkButton *button, APP_data *data)
{
 GtkTreeIter iter, child;
 GtkTreeSelection *selection;
 GtkTreePath *path_tree;
 GtkTreeModel *model;
 gint *indices;

  /* get current line for toggling event */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(data->treeViewCalendars) );
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE );

  if( gtk_tree_selection_get_selected (selection, &model, &iter) ) {
	  gtk_tree_model_get (model, &iter, -1);
	  /* read path  */
	  path_tree = gtk_tree_model_get_path (model, &iter);
          indices = gtk_tree_path_get_indices (path_tree);

          if(gtk_tree_path_get_depth (path_tree)>=1) {
             gtk_tree_path_append_index (path_tree, indices[0]);/* mandatory */
             gtk_tree_model_get_iter_first (model, &iter);
             gtk_tree_store_append (GTK_TREE_STORE(model), &child, &iter);
             gtk_tree_store_set (GTK_TREE_STORE(model), &child,
                     CAL_COLUMN, _("New Calendar from main"),
                     -1);
             calendars_new_calendar (_("New Calendar from main"), TRUE, data);
             
          }
          /* never free 'indices' or 'new_text' */	
	  gtk_tree_path_free (path_tree);	
  }/* endif */

}

/***********************************
  callback : get current selection
  if selection == root calendar,
  de-activate [remove] button since
  we can't remove main calendar
************************************/
void on_calendars_changed (GtkWidget *widget, APP_data *data) {
    
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreePath *path_tree;
  gchar *value;
  gint *indices, num;
  GtkWidget *btnAdd, *btnRemove, *calendar, *labelName, *btnReset;
  GDate date;

  btnAdd = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "btnAdd"));  
  btnRemove = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "btnRemove"));  
  calendar = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "calendar1"));
  labelName = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelCalName"));

  gtk_widget_hide (GTK_WIDGET (calendar));
  gtk_widget_set_sensitive (GTK_WIDGET (btnRemove), FALSE);

  if(gtk_tree_selection_get_selected (GTK_TREE_SELECTION(widget), &model, &iter)) {
    gtk_tree_model_get (model, &iter, CAL_COLUMN, &value, -1);
    path_tree = gtk_tree_model_get_path (model, &iter);
    indices = gtk_tree_path_get_indices (path_tree);
    if(gtk_tree_path_get_depth (path_tree)>1) {
       gtk_widget_set_sensitive (GTK_WIDGET (btnRemove), TRUE);
       num = indices[1]+1;
   }
   else {
       if(gtk_tree_path_get_depth (path_tree)>0)
           gtk_widget_set_sensitive (GTK_WIDGET (btnRemove), TRUE);          
       num = 0;
   }

   gtk_tree_path_free (path_tree);
   g_free (value);
  }
 
  calendars_update_number_nw_days (num, data);
  gtk_calendar_clear_marks (GTK_CALENDAR(calendar));

  /* we set marks for current month */
  calendar_mark_days_of_month (GTK_CALENDAR(calendar));

  gtk_label_set_text (GTK_LABEL(labelName), calendar_get_name (num, data));
  gtk_widget_show (GTK_WIDGET (calendar));
  /* we update schedules */
  calendars_update_schedules (num, day_of_week, data);
}

/******************************
  CB : calendar title edited
******************************/
void calendar_name_edited_callback (GtkCellRendererText *cell,
                                  gchar  *path_string, gchar *new_text, APP_data *data)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;	
  GtkTreePath *path_tree;
  gint *indices;
  GtkWidget *labelName;

  labelName = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelCalName"));
  /* get current line for toggling event */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(data->treeViewCalendars) );
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE );

  if( gtk_tree_selection_get_selected (selection, &model, &iter ) ) {
	  gtk_tree_model_get (model, &iter, -1);
	  /* read path  */
	  path_tree = gtk_tree_model_get_path (model, &iter);
          indices = gtk_tree_path_get_indices (path_tree);
          if(gtk_tree_path_get_depth (path_tree)>1) {

            if(strlen (new_text)>0) {
               gtk_tree_store_set (GTK_TREE_STORE(model), &iter, CAL_COLUMN, new_text , -1);
               calendar_set_name (indices[1]+1, new_text, data);
               gtk_label_set_text (GTK_LABEL(labelName), new_text);
            }
          }
          /* never free 'indices' or 'new_text' */
	  gtk_tree_path_free (path_tree);	
  }/* endif */

}

/*************************************
  LOCAL : fill with current calendars 
*************************************/
GtkTreeModel *calendars_create_and_fill_model (APP_data *data) {
    
  GtkTreeStore *treestore;
  GtkTreeIter toplevel, child;
  gint i;
  GList *l;
  calendar *cal;

  treestore = gtk_tree_store_new (CAL_NUM_COLS, G_TYPE_STRING);

  /* add all current calendars */
  if(g_list_length (data->calendars) == 0) {/* nothing defined, so we use only main calendar */
     gtk_tree_store_append (treestore, &toplevel, NULL);
     gtk_tree_store_set (treestore, &toplevel, CAL_COLUMN, _("Main calendar"), -1);
  }
  else {
    for(i=0;i<g_list_length (data->calendars);i++) {
      l = g_list_nth (data->calendars, i);
      cal = (calendar *) l->data;
      if(i==0) {/* main calendar */
         gtk_tree_store_append (treestore, &toplevel, NULL);
         gtk_tree_store_set (treestore, &toplevel, CAL_COLUMN, cal->name, -1);
      }
      else {/* others */
         gtk_tree_store_append (treestore, &child, &toplevel);
         gtk_tree_store_set (treestore, &child, CAL_COLUMN, cal->name, -1);
      }
    }/* next */
  }
  return GTK_TREE_MODEL(treestore);
}

/*************************
  create model and view
*************************/
GtkWidget *calendars_create_view_and_model (APP_data *data) 
{   
  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;
  GtkWidget *view;
  GtkTreeModel *model;

  view = gtk_tree_view_new ();

  col = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (col, _("Calendars"));
  gtk_tree_view_append_column (GTK_TREE_VIEW(view), col);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "editable", TRUE, NULL);
  g_signal_connect (renderer, "edited", (GCallback) calendar_name_edited_callback, data);/* YES, since data alread exists */

  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_add_attribute (col, renderer, "text", CAL_COLUMN);

  model = calendars_create_and_fill_model (data);
  gtk_tree_view_set_model (GTK_TREE_VIEW(view), model);
  g_object_unref (model); 

  return view;
}

/**************************
  LOCAL : returns a string
  for current day
**************************/
static gchar *calendar_get_detail (APP_data *data,
                     guint         year,
                     guint         month,
                     guint         day)
{
  GtkTreeSelection *selection;
  gchar *key = NULL;
  GtkTreeModel *model;
  GtkTreePath *path_tree;
  GtkTreeIter iter;
  gint *indices, true_index, marked;

  /* get current line for toggling event */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(data->treeViewCalendars) );
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE );


  if(gtk_tree_selection_get_selected (GTK_TREE_SELECTION(selection), &model, &iter)) {
      gtk_tree_model_get (model, &iter, -1);
      /* read path  */
      path_tree = gtk_tree_model_get_path (model, &iter);
      indices = gtk_tree_path_get_indices (path_tree);
      true_index = 0;/* default, main calendar */
      if(gtk_tree_path_get_depth (path_tree)>1)
         true_index = indices[1]+1;
      /* now we read calendar Glist of days TODO */
      marked = calendars_date_is_marked (day, month+1, year, true_index, data);/* yes, for gtk calendars, month are from 0 to 11 */
      if(marked>0) {
         switch(marked) {
           case CAL_TYPE_FORCE_WORKED: {
             key = g_strdup_printf ("%s", _("<span foreground=\"white\" background=\"#2E2A72\"><small>force working </small></span>"));
             break;
           }
           case CAL_TYPE_NON_WORKED: {
             key = g_strdup_printf ("%s", _("<span foreground=\"white\" background=\"#5E1C71\"><small> misc. </small></span>"));
             break;
           }
           case CAL_TYPE_DAY_OFF: {
             key = g_strdup_printf ("%s", _("<span foreground=\"black\" background=\"#1ED110\"><small> day off </small></span>"));
             break;
           }
           case CAL_TYPE_HOLIDAYS: {
             key = g_strdup_printf ("%s", _("<span foreground=\"black\" background=\"#F7CB16\"><small> holidays </small></span>"));
             break;
           }
           case CAL_TYPE_ABSENT: {
             key = g_strdup_printf ("%s", _("<span foreground=\"white\" background=\"red\"><small> away </small></span>"));
             break;
           }
           case CAL_TYPE_PUBLIC_HOLIDAYS: {
             key = g_strdup_printf ("%s", _("<span foreground=\"black\" background=\"#F7CB16\"><small> publ. hol. </small></span>"));
             break;
           }

         }/* end switch */
      }
      gtk_tree_path_free (path_tree);
  }
  return key;
}

/**********************************
  LOCAL : callback for  displaying
 'details" above each days
**********************************/
static gchar *calendar_detail_cb (GtkCalendar *calendar,
                    guint        year,
                    guint        month,
                    guint        day,
                    APP_data     *data)
{
  return calendar_get_detail (data, year, month, day);
}

/**************************
  PUBLIC : calendar dialog
**************************/
GtkWidget *calendars_dialog (APP_data *data)
{
  GtkWidget *dialog, *calendar, *btn, *labelDefStartTime, *labelDefEndTime;
  GtkBuilder *builder =NULL;
  builder = gtk_builder_new ();
  GError *err = NULL; 
  GtkTreeSelection *selection; 

  if(!gtk_builder_add_from_file (builder, find_ui_file ("calendars.ui"), &err)) {
     g_error_free (err);
     misc_halt_after_glade_failure (data);
  }
  /* Get the dialog from the glade file. */
  gtk_builder_connect_signals (builder, data);
  dialog = GTK_WIDGET(gtk_builder_get_object (builder, "dialogCalendars"));
  data->tmpBuilder = builder;

  /* header bar */

  GtkWidget *hbar = gtk_header_bar_new ();
  gtk_widget_show (hbar);
  gtk_header_bar_set_title (GTK_HEADER_BAR (hbar), _("Calendars & schedules"));
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (hbar), _("Define and modify project's calendars and daily schudules."));
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (hbar), TRUE);
  gtk_window_set_titlebar (GTK_WINDOW(dialog), hbar);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);  
  gtk_header_bar_pack_start (GTK_HEADER_BAR (hbar), box);
  GtkWidget *icon =  gtk_image_new_from_icon_name  ("x-office-calendar", GTK_ICON_SIZE_DIALOG);
  gtk_container_add (GTK_CONTAINER (box), icon);
  gtk_widget_show (box);
  gtk_widget_show (icon);



  data->treeViewCalendars = calendars_create_view_and_model (data);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(data->treeViewCalendars), TRUE);
  gtk_widget_show (data->treeViewCalendars);
  gtk_container_add (GTK_CONTAINER ( GTK_WIDGET(gtk_builder_get_object (builder, "viewportCal"))), 
                     data->treeViewCalendars);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(data->treeViewCalendars));
  btn = GTK_WIDGET(gtk_builder_get_object (builder, "radiobuttonNw"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(btn), TRUE);
  /* schedules */
  labelDefStartTime = GTK_WIDGET(gtk_builder_get_object (builder, "labelDefStartTime"));
  labelDefEndTime = GTK_WIDGET(gtk_builder_get_object (builder, "labelDefEndTime"));

  gtk_label_set_text (GTK_LABEL(labelDefStartTime), g_strdup_printf("%.02d:%.02d", 
                      data->properties.scheduleStart/60,  data->properties.scheduleStart % 60));
  gtk_label_set_text (GTK_LABEL(labelDefEndTime), g_strdup_printf("%.02d:%.02d", 
                      data->properties.scheduleEnd/60,  data->properties.scheduleEnd % 60));

  g_signal_connect(selection, "changed", 
                   G_CALLBACK(on_calendars_changed), data);

  calendar = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "calendar1"));

  gtk_calendar_set_detail_func (GTK_CALENDAR (calendar),
                                  calendar_detail_cb, data, NULL);
  day_of_week = 0; /* monday by default */
  /* update schedules */
  calendars_update_schedules (0, day_of_week, data);/* keep as it, requires twice calls ! */

  gtk_window_set_transient_for (GTK_WINDOW(dialog),
                              GTK_WINDOW(data->appWindow));

  return dialog;
}

/*******************************
  PUBLIC : cb when a day receive
  a double-click
******************************/
void calendars_day_double_click (GtkCalendar *calendar, APP_data *data)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreePath *path_tree;
  GtkTreeIter iter;
  gint *indices, true_index, marked;
  guint day, month, year;

  gtk_calendar_get_date (calendar, &year, &month, &day);
  month++; /* because month are stored from 0 to 11 in gtkcalendars */
  gtk_widget_hide (GTK_WIDGET (calendar));
  // TODO check if day is already selected
  //gtk_calendar_mark_day (calendar, day);
  /* get current line for toggling event */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(data->treeViewCalendars) );
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);


  if(gtk_tree_selection_get_selected (GTK_TREE_SELECTION(selection), &model, &iter)) {
      gtk_tree_model_get (model, &iter, -1);
      /* read path  */
      path_tree = gtk_tree_model_get_path (model, &iter);
      indices = gtk_tree_path_get_indices (path_tree);
      true_index = 0;/* default, main calendar */
      if(gtk_tree_path_get_depth (path_tree)>1)
         true_index = indices[1]+1;
      /* now, we test current stat for this day */
      marked =  calendars_date_is_marked (day, month, year, true_index, data);
      if(marked == -1) {
            /* we add the date */
            calendars_add_new_marked_date (TRUE, true_index, day, month, year, cal_current_type, data);
      }
      else {
            /* we remove the date */
            calendars_remove_marked_date (true_index, marked, data);
      }
      gtk_tree_path_free (path_tree);
  }/* endif */
  gtk_widget_show (GTK_WIDGET (calendar));
}

/*******************************
  PUBLIC : cb when a day is
  selected
******************************/
void calendars_day_selected (GtkCalendar *calendar, APP_data *data)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreePath *path_tree;
  GtkTreeIter iter;
  gint *indices, true_index;
  gint marked;
  guint day, month, year;

  gtk_calendar_get_date (calendar, &year, &month, &day);
  month++;
  gtk_widget_hide (GTK_WIDGET (calendar));
  // TODO check if day is already selected
  //gtk_calendar_mark_day (calendar, day);
  /* get current line for toggling event */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(data->treeViewCalendars) );
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);


  if(gtk_tree_selection_get_selected (GTK_TREE_SELECTION(selection), &model, &iter)) {
      gtk_tree_model_get (model, &iter, -1);
      /* read path  */
      path_tree = gtk_tree_model_get_path (model, &iter);
      indices = gtk_tree_path_get_indices (path_tree);
      true_index = 0;/* default, main calendar */
      if(gtk_tree_path_get_depth (path_tree)>1)
         true_index = indices[1]+1;
      /* now, we test current stat for this day */
      marked =  calendars_date_is_marked (day, month, year, true_index, data);
      if(marked == -1) {
            /* we add the date */
            calendars_add_new_marked_date (TRUE, true_index, day, month, year, CAL_TYPE_NON_WORKED, data);
      }
      else {
            /* we remove the date */
            calendars_remove_marked_date (true_index, marked, data);
      }
      gtk_tree_path_free (path_tree);
  }/* endif */
  gtk_widget_show (GTK_WIDGET (calendar));
}

/******************************************************************
  PUBLIC : cb when month change useful : changes when user
  clicks on days outside current month and when user changes year
******************************************************************/
void calendars_month_changed (GtkCalendar *calendar, APP_data *data)
{
  gtk_calendar_clear_marks (calendar);
  /* we set marks for current month */
  calendar_mark_days_of_month (calendar);
}

/**********************************
  Callback : reset all non-working
  days for CURRENT calendar
**********************************/
void on_calendars_reset_clicked (GtkButton *button, APP_data *data)
{
  GtkWidget *dialog, *win, *calendar;
  GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
  gboolean flag = TRUE;
  GtkTreeIter iter;
  GtkTreeSelection *selection;
  GtkTreePath *path_tree;
  GtkTreeModel *model;
  gint *indices, true_index;

  win = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "dialogCalendars"));
  dialog = gtk_message_dialog_new (GTK_WINDOW(win), flags,
                    GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL,
                _("Do you really want to remove ALL marked days ?\nThis operation isn't cancellable ..."));
  if(gtk_dialog_run (GTK_DIALOG(dialog))!=GTK_RESPONSE_OK)
         flag=FALSE;
  gtk_widget_destroy (GTK_WIDGET(dialog));

  /* we continue if response OK */
  if(flag){
    calendar = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "calendar1"));
    gtk_widget_hide (GTK_WIDGET (calendar));
    /* get current line for event */
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(data->treeViewCalendars));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
    if(gtk_tree_selection_get_selected (GTK_TREE_SELECTION(selection), &model, &iter)) {
      gtk_tree_model_get (model, &iter, -1);
      /* read path  */
      path_tree = gtk_tree_model_get_path (model, &iter);
      indices = gtk_tree_path_get_indices (path_tree);
      true_index = 0;/* default, main calendar */
      if(gtk_tree_path_get_depth (path_tree)>1)
         true_index = indices[1]+1;
      /* now, we know rank of calendar */
      calendars_remove_all_marked_dates (true_index, data);
      gtk_tree_path_free (path_tree);
      calendars_update_number_nw_days (true_index, data);/* lock button */
    }/* endif selection */
    gtk_widget_show (GTK_WIDGET (calendar));
  }/* endif OK */

}

/********************************
  radio buttons signals

********************************/
void on_nworking_toggled (GtkToggleButton *togglebutton, APP_data *data)
{
  if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton)))
    cal_current_type = CAL_TYPE_NON_WORKED;
}

void on_force_working_toggled (GtkToggleButton *togglebutton, APP_data *data)
{
  if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton)))
    cal_current_type = CAL_TYPE_FORCE_WORKED;
}

void on_weekend_toggled (GtkToggleButton *togglebutton, APP_data *data)
{
  if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton)))
    cal_current_type = CAL_TYPE_DAY_OFF;
}

void on_holidays_toggled (GtkToggleButton *togglebutton, APP_data *data)
{
  if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton)))
    cal_current_type = CAL_TYPE_HOLIDAYS;
}

void on_absent_toggled (GtkToggleButton *togglebutton, APP_data *data)
{
  if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton)))
    cal_current_type = CAL_TYPE_ABSENT;
}

void on_public_holiday_toggled (GtkToggleButton *togglebutton, APP_data *data)
{
  if(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton)))
    cal_current_type = CAL_TYPE_PUBLIC_HOLIDAYS;
}
/********************************
  days of week buttons signals

********************************/

void on_monday_clicked (GtkButton *button, APP_data *data)
{
  calendar_mark_days_of_week_for_project (calendars_get_current_calendar_index (data), G_DATE_MONDAY, cal_current_type, data);
}

void on_tuesday_clicked (GtkButton *button, APP_data *data)
{
  calendar_mark_days_of_week_for_project (calendars_get_current_calendar_index (data), 
                                          G_DATE_TUESDAY, cal_current_type, data);
}

void on_wednesday_clicked (GtkButton *button, APP_data *data)
{
  calendar_mark_days_of_week_for_project (calendars_get_current_calendar_index (data), 
                                          G_DATE_WEDNESDAY, cal_current_type, data);
}

void on_thursday_clicked (GtkButton *button, APP_data *data)
{
  calendar_mark_days_of_week_for_project (calendars_get_current_calendar_index (data), 
                                          G_DATE_THURSDAY, cal_current_type, data);
}

void on_friday_clicked (GtkButton *button, APP_data *data)
{
  calendar_mark_days_of_week_for_project (calendars_get_current_calendar_index (data), 
                                          G_DATE_FRIDAY, cal_current_type, data);
}

void on_saturday_clicked (GtkButton *button, APP_data *data)
{
  calendar_mark_days_of_week_for_project (calendars_get_current_calendar_index (data), 
                                          G_DATE_SATURDAY, cal_current_type, data);
}

void on_sunday_clicked (GtkButton *button, APP_data *data)
{
  calendar_mark_days_of_week_for_project (calendars_get_current_calendar_index (data), 
                                          G_DATE_SUNDAY, cal_current_type, data);
}

/**************************
  PUBLIC CB for periods
**************************/
void on_period_start_clicked (GtkButton *button, APP_data *data)
{
  GDate date_interval1, date_start, date_end;
  gboolean fErrDateInterval1 = FALSE;
  GtkWidget *btnStart, *btnEnd, *win;
  gint cmp_date;

  btnStart = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonDateStart"));
  btnEnd = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonDateEnd"));
  win = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "dialogCalendars"));

  gchar *newDate = misc_getDate (gtk_button_get_label (GTK_BUTTON(btnStart)), win);
  gtk_button_set_label (GTK_BUTTON(btnStart), newDate);

  /* we get dates limits from properties */
  g_date_set_parse (&date_start, data->properties.start_date );
  g_date_set_parse (&date_end, gtk_button_get_label (GTK_BUTTON(btnEnd)) );
  g_date_set_parse (&date_interval1, newDate);
  fErrDateInterval1 = g_date_valid (&date_interval1);
  if(!fErrDateInterval1)
      printf ("Internal error calendar date start \n");
  /* selected start date must be inside project limits */
  cmp_date = g_date_compare (&date_interval1, &date_start);
  if(cmp_date<0) {
       misc_ErrorDialog (win, _("<b>Error!</b>\n\nDates mismatch ! 'Starting' date must be <i>inside</i> 'project' limits.\n\nPlease check the dates."));
       gtk_button_set_label (GTK_BUTTON(btnStart), data->properties.start_date);
  }
  /* selected date must be before period's ending date */
  cmp_date = g_date_compare (&date_interval1, &date_end);
  if(cmp_date>=0) {
       misc_ErrorDialog (win, _("<b>Error!</b>\n\nDates mismatch ! 'Starting' date must be <i>before</i> 'period' ending date.\n\nPlease check the dates."));
       gtk_button_set_label (GTK_BUTTON(btnStart), data->properties.start_date);
  }

  g_free (newDate);
}

void on_period_end_clicked (GtkButton *button, APP_data *data)
{
  GDate date_interval1, date_start, date_end;
  gboolean fErrDateInterval1 = FALSE;
  GtkWidget *btnStart, *btnEnd, *win;
  gint  cmp_date;

  btnStart = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonDateStart"));
  btnEnd = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonDateEnd"));
  win = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "dialogCalendars"));

  gchar *newDate = misc_getDate (gtk_button_get_label (GTK_BUTTON(btnEnd)), win);
  gtk_button_set_label (GTK_BUTTON(btnEnd), newDate);

  /* we get dates limits from properties */
  g_date_set_parse (&date_start, data->properties.start_date );
  g_date_set_parse (&date_end, data->properties.end_date );
  g_date_set_parse (&date_interval1, newDate);
  fErrDateInterval1 = g_date_valid (&date_interval1);
  if(!fErrDateInterval1)
      printf ("Internal error calendar date end \n");
  /* selected start date must be inside project limits */
  cmp_date = g_date_compare (&date_interval1, &date_end);
  if(cmp_date>0) {
       misc_ErrorDialog (win, _("<b>Error!</b>\n\nDates mismatch ! 'Ending' date must be <i>inside</i> 'project' limits.\n\nPlease check the dates."));
       gtk_button_set_label (GTK_BUTTON(btnEnd), data->properties.end_date);
  }
  /* selected date must be after period's starting date */
  cmp_date = g_date_compare (&date_interval1, &date_start);
  if(cmp_date<=0) {
       misc_ErrorDialog (win, _("<b>Error!</b>\n\nDates mismatch ! 'Ending' date must be <i>after</i> 'period' starting date.\n\nPlease check the dates."));
       gtk_button_set_label (GTK_BUTTON(btnEnd), data->properties.end_date);
  }
  g_free (newDate);
}

void on_period_apply_clicked (GtkButton *button, APP_data *data)
{
  GtkWidget *btnStart, *btnEnd;
  GDate date_start, date_end;

  /* we get current period limits */
  btnStart = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonDateStart"));
  btnEnd = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonDateEnd"));
  g_date_set_parse (&date_start, gtk_button_get_label (GTK_BUTTON(btnStart)) );
  g_date_set_parse (&date_end, gtk_button_get_label (GTK_BUTTON(btnEnd)) );

  calendar_mark_days_for_period (calendars_get_current_calendar_index (data), cal_current_type, &date_start, &date_end, data);
}

/**********************************************************
  PUBLIC : returns schedule for current "day of week" and 
  calendar- arguments  :
- index = nth of calendar in GList
- day = nth for current day (monday = 0)
returns : minutes equivalent to hour
if not scheduled, returns default values from properties
***********************************************************/
gint calendars_get_starting_hour (gint index, gint day, APP_data *data)
{
  gint ret = data->properties.scheduleStart; /* default error == -1 */
  GList *l;
  calendar *cal;
  
  /* we test if period #1 is activated for current day of week */
  if(index>=0) {
     l = g_list_nth (data->calendars, index);
     cal = (calendar *) l->data;
     if(cal->fDays[day][0]) {
        ret = cal->schedules[day][0][0];
     }
  }
  return ret;
}

/*********************************************************
  PUBLIC : returns schedule for current "day of week" and 
  calendar. arguments :
- index = nth of calendar in GList
- day = nth for current day (monday = 0)
returns : minutes equivalent to hour
if not scheduled, returns default values from properties
*********************************************************/
gint calendars_get_ending_hour (gint index, gint day, APP_data *data)
{
  gint ret, i, minutes; /* default error == -1 */
  GList *l;
  calendar *cal;
   
  ret = data->properties.scheduleEnd;
  /* we test if period #1 is activated for current day of week */
  if(index>=0) {
     l = g_list_nth (data->calendars, index);
     cal = (calendar *) l->data;
     for(i=0;i<4;i++) {
	if(cal->fDays[day][i]) {
           minutes = cal->schedules[day][i][1];
           if(minutes>ret)
	      ret = minutes;
	}
     }/* next i */
  }
  return ret;
} 
