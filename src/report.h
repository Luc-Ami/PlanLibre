/*
 * various utilities and constants
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#ifndef REPORT_H
#define REPORT_H

#include <gtk/gtk.h>
#include <goocanvas.h>
#include "support.h"

/* constants */

#define REPORT_DIAGRAM_WIDTH 600
#define REPORT_DIAGRAM_NAME_WIDTH 140
#define REPORT_DIAGRAM_HEIGHT 300
#define REPORT_DIAGRAM_LEGEND_HEIGHT 100
#define REPORT_DIAGRAM_INDEX_HEIGHT 40
#define REPORT_RSC_COST_WIDTH 800
#define REPORT_RSC_COST_HEIGHT 400
#define REPORT_RSC_COST_RADIUS 120

/* enumerations */

/* structures */

/* functions */
void report_init_view (APP_data *data);
void report_clear_text (GtkTextBuffer *buffer, const gchar *tag);
void report_default_text (GtkTextBuffer *buffer);
void report_set_buttons_menus (gboolean fRtf, gboolean fMail, APP_data *data);
void report_unique_rsc_charge (gint id, APP_data *data);
void report_all_rsc_charge (APP_data *data);
void report_unique_rsc_cost (gint id, APP_data *data);
void report_all_rsc_cost (APP_data *data);
void report_export_report_to_rtf (APP_data *data);

void report_unique_rsc_overload (gint id, APP_data *data);

#endif /* REPORT_H */
