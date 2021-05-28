/*
 * various utilities and constants
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#ifndef ASSIGN_H
#define ASSIGN_H

#include <gtk/gtk.h>
#include <goocanvas.h>
#include "support.h"

/* constants */

#define ASSIGN_VIEW_MAX_WIDTH 5500
#define ASSIGN_VIEW_MAX_HEIGHT 18000
#define ASSIGN_MAX_WEEKS 6 /* 10 weeks, 2 months */
#define ASSIGN_MAX_DAYS 1456 /* 4 years - 1456 days */
#define ASSIGN_SCROLL_STEP 10
#define ASSIGN_AVATAR_WIDTH 64
#define ASSIGN_AVATAR_HEIGHT 200
#define ASSIGN_DEMI_HEIGHT 80
#define ASSIGN_CALENDAR_HEADER_HEIGHT 64
#define ASSIGN_CALENDAR_DAY_WIDTH 128 
#define ASSIGN_CALENDAR_RSC_WIDTH 128 
#define ASSIGN_RSC_CHARGE_WIDTH 400
#define ASSIGN_RSC_CHARGE_HEIGHT 400
#define ASSIGN_RSC_CHARGE_RADIUS 120
#define ASSIGN_EXPORT_PDF 0
#define ASSIGN_EXPORT_SVG 1
#define ASSIGN_EXPORT_PNG 2

/* enumerations */

/* structures */

typedef struct  {
  gint task_id; /* Id of a task where resource is assigned */
  gint s_day;/* starting date */
  gint s_month;
  gint s_year;
  gint e_day;/* ending date */
  gint e_month;
  gint e_year;
} assign_data;/* structure used when we try to verify if a resource is overloaded */



/* functions */
void assign_canvas (GtkWidget *win, APP_data *data);
void assign_draw_calendar_ruler (gdouble ypos, gint dd, gint mm, gint yy, APP_data *data);
void on_assign_vadj_changed (GtkAdjustment *adjustment, APP_data *data);
void on_assign_hadj_changed (GtkAdjustment *adjustment, APP_data *data);
void assign_draw_rsc_ruler ( gdouble xpos,  APP_data *data);
void assign_store_date (gint dd, gint mm, gint yy);
void assign_remove_all_tasks (APP_data *data);
void on_BtnPrevMonth_clicked  (GtkButton *button, APP_data *data);
void on_BtnNextMonth_clicked  (GtkButton *button, APP_data *data);
void assign_draw_task (gint index, gint id, GooCanvasItem *root, GDate *date, GdkRGBA color, APP_data *data );
void assign_draw_all_tasks (APP_data *data );
void assign_draw_all_visuals (APP_data *data);
gint assign_export (gint type, APP_data *data);

#endif /* ASSIGN_H */
