/*
 * various utilities and constants
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#ifndef MISC_H
#define MISC_H

#include <goocanvas.h>
#include <gtkspell/gtkspell.h>
#include "support.h"

#define MAX_RECENT_FILES 10
#define CURRENT_VERSION "0.1"
#define TOMATO_BEAT 10
#define AUTOSAVE_BEAT 300

/* notebook & undo constants */
#define CURRENT_STACK_HOME 0
#define CURRENT_STACK_TIMELINE 1
#define CURRENT_STACK_TASKS 2
#define CURRENT_STACK_RESSOURCES 3
#define CURRENT_STACK_DASHBOARD 4
#define CURRENT_STACK_ASSIGN 5
#define CURRENT_STACK_REPORT 6
#define CURRENT_STACK_MISC 7

/* ressources */
#define RSC_AFF_OUR_ORG 0
#define RSC_AFF_PRO_ORG 1
#define RSC_AFF_IND 2
#define RSC_AFF_VOL 3
#define RSC_AFF_NPO 4
#define RSC_AFF_SCO 5
#define RSC_AFF_ADMI 6
#define RSC_AFF_PLAT 7

#define RSC_COST_HOUR 0
#define RSC_COST_DAY 1
#define RSC_COST_WEEK 2
#define RSC_COST_MONTH 3
#define RSC_COST_FIXED 4

#define RSC_TYPE_HUMAN 0
#define RSC_TYPE_ANIMAL 1
#define RSC_TYPE_EQUIP 2
#define RSC_TYPE_SURF 3
#define RSC_TYPE_MATER 4

#define RSC_AVATAR_WIDTH 48
#define RSC_AVATAR_HEIGHT 48

/* tasks */
#define TASKS_WORKING_DAY 8
#define TASKS_AS_SOON 0
#define TASKS_DEADLINE 1
#define TASKS_AFTER 2
#define TASKS_FIXED 3
#define TASKS_RSC_AVATAR_SIZE 24
#define TASKS_RSC_LEN_TEXT 10
#define TASKS_TYPE_TASK 0
#define TASKS_TYPE_GROUP 1
#define TASKS_TYPE_MILESTONE 2
#define TASKS_STATUS_RUN 0
#define TASKS_STATUS_DONE 1
#define TASKS_STATUS_DELAY 2
#define TASKS_STATUS_TO_GO 3
/* properties */

#define PROP_LOGO_WIDTH 96
#define PROP_LOGO_HEIGHT 96

/* enumerations */

/* structures */

/* structure to store 'links' between 2 objects, for example
   receiver = id of a task, sender = id of a ressource */
typedef struct  {
  gint type; /* 0 = receiver : task, sender : ressource ; 1 = receiver & sender : task */
  gint receiver;
  gint sender;
  gint mode; /* for example : end of preceding to start of succeding task */
} link_datas;

typedef struct {
   gint id;/* identifier of a task using current group */
} group_datas;

typedef struct  {
  gint curStack;
  gint opCode;
  gint id; /* in order to retrieve even if visual position has changed */
  gint type_tsk; /* in order to undo remove */
  gint insertion_row; /* for tasks and ressources */
  gint new_row; /* for tasks linked to groups */
  gint t_shift;
  gint nbTasks; /* for tasks linked to a group */
  gint s_day;
  gint s_month;
  gint s_year;
  gint e_day;
  gint e_month;
  gint e_year;
  gpointer datas; /* a pointer on convenient structure, i.e. properties, tasks, rsc ... */
  GList *list;
  GList *groups;/* GList of associations tasks<>groups */
} undo_datas;

typedef struct {
  gchar *title;
  gchar *summary;
  gchar *location;
  gchar *gps;
  gchar *manager_name;
  gchar *manager_mail;
  gchar *manager_phone;
  gchar *org_name;
  gchar *proj_website;
  gchar *units;
  gdouble budget;
  GdkPixbuf *logo;
  gchar *start_date;
  gint start_nthDay;
  gint start_nthMonth;
  gint start_nthYear;
  gchar *end_date;
  gint end_nthDay;
  gint end_nthMonth;
  gint end_nthYear;
  gint scheduleStart;
  gint scheduleEnd;
  gboolean fRemIcon;
} rsc_properties;

typedef struct {
  gint id;
  gchar     *name;
  gchar     *mail;
  gchar     *phone;
  gchar     *reminder;
  gchar     *location;
  gint      affiliate;
  gint      type; /* of rsc */
  gdouble   cost;
  gint      day_hours;
  gint      day_mins;
  gint      cost_type;
  GdkRGBA   color;
  gboolean  fPersoIcon;
  GdkPixbuf *pix;
  GtkWidget *cadre; /* in order to remove ! */
  GtkWidget *cName; /* inside 'cadre' widget */
  GtkWidget *cType;
  GtkWidget *cAff;
  GtkWidget *cCost;
  GtkWidget *cColor;
  GtkWidget *cImg;
  gboolean fWorkMonday;
  gboolean fWorkTuesday;
  gboolean fWorkWednesday;
  gboolean fWorkThursday;
  gboolean fWorkFriday;
  gboolean fWorkSaturday;
  gboolean fWorkSunday;
  gboolean fAllowConcurrent;/* in order to allow a ressource to be assigned to more 1 task at same time */
  gint calendar;
} rsc_datas;

typedef struct {
   gint id;/* UNIQUE internal IDentifier, used by all tasks - never change between sessions, automatically incremented to avoid conflicts  */
   gchar *name;
   gchar *location;
   gchar *gps;
   gint type;
   gint status; /* 4 levels from 'to go' to 'delayed' */
   gint priority; /* 3 levels */
   gint category;
   GdkRGBA color;
   gdouble progress; /* in percent */
   gchar *start_date;
   gint start_nthDay;
   gint start_nthMonth;
   gint start_nthYear;
   gchar *end_date;
   gint end_nthDay;
   gint end_nthMonth;
   gint end_nthYear;

   gint lim_nthDay;
   gint lim_nthMonth;
   gint lim_nthYear;
   gint durationMode;
   gint days;
   gint hours;
   gint minutes;
   gint calendar;
   gint group; /* -1 if tasks no belongs to any group, internal ID in other cases */
   gint64 timer_val;
   gint delay; /* delay positive or negative in minutes, not in days */
   GList *rsc_used;
   GList *task_prec;
   GtkWidget *cadre; 
   GtkWidget *cGroup;
   GtkWidget *cName;
   GtkWidget *cPeriod;
   GtkWidget *cProgress;
   GtkWidget *cImgPty;
   GtkWidget *cImgStatus;
   GtkWidget *cImgCateg;
   GtkWidget *cRscBox;
   GtkWidget *cTaskBox;
   GtkWidget *cDashboard;
   GtkWidget *cDashProgress;
   GtkWidget *cRun;/* minutes of job run */
   GooCanvasItem *groupItems;
} tasks_data;

typedef struct {
  GtkBuilder  *builder;
  GtkBuilder  *tmpBuilder;
  gint objectsCounter;
  gboolean button_pressed;
  gboolean fDarkTheme;
  gboolean fProjectModified;
  gboolean fProjectHasName;
  gboolean fProjectHasSnapshot; /* flag for storage of last snapshot of current project */

  GtkApplication *app;
  GtkWidget    *window;
  GtkWidget    *appWindow;
  GtkWidget    *pBtnUndo;
  gint         clipboardMode;  /* 0= text 1=picture */

  gchar *path_to_pixbuf;
  gchar *path_to_snapshot;
  GtkWidget *statusbar1;
  GtkWidget *statusbar2;
  GtkWidget *treeViewTasksRsc;
  GtkWidget *treeViewTasksTsk;
  GtkWidget *treeViewReportMail;
  GtkWidget *treeViewCalendars;
  gchar *gConfigFile;
  GKeyFile *keystring;
  GtkTextView *view;
  GtkTextBuffer *buffer;
  GtkTextIter *curpos;
  gint curRsc;
  gint curTasks;
  gint curPDFpage;
  gint totalPDFpages;
  gdouble PDFWidth;
  gdouble PDFHeight;
  gdouble PDFratio;
  gint currentStack;
  GList *rscList;
  GList *tasksList;
  GList *undoList;
  GList *linksList;
  GList *calendars;
  GList *groups;
  rsc_properties properties;
  GtkSpellChecker* spell;
  GDate earliest;
  GDate latest;
  GtkWidget  *canvasTL;
  GtkWidget  *canvasAssign;
  GooCanvasItem *ruler;
  GooCanvasItem *week_endsTL;
  GooCanvasItem *rulerAssign;
  GooCanvasItem *rscAssign;
  GooCanvasItem *rscAssignMain;  
} APP_data;

/***************************
  functions
***************************/
void misc_colorinvert_picture(GdkPixbuf *pb);
gchar *misc_get_pango_string(const gchar *key, const gint modifier);
void undo_popup_menu(GtkWidget *attach_widget, GtkMenu *menu);

void misc_init_vars (APP_data *data );
void misc_init_spell_checker (GtkTextView *view, APP_data *data );

void misc_prepare_timeouts (APP_data *data );
void misc_set_font_color_settings (APP_data *data );
void misc_ErrorDialog (GtkWidget *widget, const gchar* msg);
void misc_InfoDialog (GtkWidget *widget, const gchar* msg);
void misc_display_app_status (gboolean modified, APP_data *data );
void misc_display_tips_status (const gchar *str, APP_data *data );

gchar *misc_convert_gdkcolor_to_hex (GdkRGBA color );
gchar *misc_force_decimal_point (gdouble val);
GtkWidget *misc_create_calendarDialog (GtkWidget *win);
gchar *misc_getDate (const gchar *curDate, GtkWidget *win);
gchar *misc_convert_date_to_str (gint dd, gint mm, gint yy);
GDate misc_get_earliest_date (APP_data *data);
GDate misc_get_latest_date (APP_data *data);
gint misc_get_highest_id (APP_data *data);
gchar *misc_convert_date_to_locale_str (GDate *date);
gboolean misc_alert_before_clearing (APP_data *data);
gdouble misc_date_to_duration (GDate *date1, GDate *date2);
gchar *misc_duration_to_friendly_str (gint days, gint minutes);
gchar *misc_convert_date_to_friendly_str (GDate *date);
gboolean misc_task_is_overdue (gint progress, GDate *end_date);
gint 
   misc_count_tasks_overdue (APP_data *data);
gint misc_count_tasks_run (APP_data *data);
gint misc_count_tasks_today (APP_data *data);
gint misc_count_milestones_today (APP_data *data);
gint misc_count_tasks_tomorrow (APP_data *data);
gint misc_count_tasks_ending_today (APP_data *data);
gint misc_count_tasks_completed (APP_data *data);
gint misc_count_tasks_completed_yesterday (APP_data *data);
gint misc_count_tasks (APP_data *data);
gdouble misc_correct_duration (gint day, gint month, gint year, gint dur, gboolean monday, gboolean tuesday, gboolean wednesday,
                              gboolean thursday, gboolean friday, gboolean saturday, gboolean sunday, 
                              gint task_calendar, gint calendar, APP_data *data);
gdouble misc_correct_duration_with_periods (gint max_day_minutes, gint day, gint month, gint year, gint dur, gint hours,
                                            gboolean monday, gboolean tuesday, gboolean wednesday,
                                            gboolean thursday, gboolean friday, gboolean saturday, gboolean sunday, 
                                            gint task_calendar, gint calendar, APP_data *data);
gchar *misc_get_month_of_year (gint month );
gchar *misc_get_short_day_of_week (gint dd);
gint  
   misc_get_calendar_day_from_glib (gint wd);
gchar *misc_erase_c (gchar *p, int ch);
void misc_halt_after_glade_failure (APP_data *data);

#endif /* MISC_H */
