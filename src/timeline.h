/*
 * various utilities and constants
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#ifndef TIMELINE_H
#define TIMELINE_H

#include <gtk/gtk.h>
#include <goocanvas.h>
#include "support.h"

/* constants */

#define TIMELINE_VIEW_MAX_WIDTH 13200
#define TIMELINE_VIEW_MAX_HEIGHT 18000
#define TIMELINE_MAX_WEEKS 52 /* 4 years = 208 weeks */
#define TIMELINE_MAX_DAYS 365 /* 4 years - 1456 days */
#define TIMELINE_SCROLL_STEP 10
#define TIMELINE_AVATAR_WIDTH 64
#define TIMELINE_AVATAR_HEIGHT 48
#define TIMELINE_CALENDAR_HEADER_HEIGHT 64
#define TIMELINE_CALENDAR_DAY_WIDTH 36 
#define TIMELINE_EXPORT_PDF 0
#define TIMELINE_EXPORT_SVG 1
#define TIMELINE_EXPORT_PNG 2

/* enumerations */

/* structures */

/* functions */
void timeline_canvas (GtkWidget *win, APP_data *data);
void timeline_draw_calendar_ruler (gdouble ypos, APP_data *data);
void on_timeline_vadj_changed (GtkAdjustment *adjustment, APP_data *data);
void timeline_draw_all_tasks (APP_data *data);
void timeline_draw_task (gint index, GooCanvasItem *root, GDate date_proj_start, tasks_data *tmp_tasks_datas, APP_data *data );
void timeline_remove_all_tasks (APP_data *data);
GtkWidget *timeline_create_timeshift_dialog (gboolean groupMode, gint position, APP_data *data);

void timeshift_calendars_day_selected (GtkCalendar *calendar, APP_data *data);
gdouble timeline_get_max_x (void);
gdouble timeline_get_max_y (void);
gint timeline_export (gint type, APP_data *data);
void timeline_store_date (gint dd, gint mm, gint yy);
void timeline_reset_store_date (void);
void timeline_get_date (gint *dd, gint *mm, gint *yy);
void timeline_remove_ruler (APP_data *data);
gdouble timeline_get_ypos (APP_data *data);
void on_combo_TL_zoom_changed (GtkComboBox *widget, APP_data *data);
void timeline_store_zoom (gdouble z);
void timeline_hide_cursor (APP_data *data);

#endif /* TIMELINE_H */
