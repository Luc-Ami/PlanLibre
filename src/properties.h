/*
 * DO NOT EDIT THIS FILE - it is generated by Glade.
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <gtk/gtk.h>
#include <gtkspell/gtkspell.h>
#include "support.h"
#include "misc.h"

/* functions */
void on_IntervalStartBtn_clicked (GtkButton *button, APP_data *data);
void on_IntervalEndBtn_clicked (GtkButton *button, APP_data *data);
GtkWidget *properties_create_dialog (APP_data *data);
void properties_modify (APP_data *data);
void properties_free_all (APP_data *data);
void properties_reset_organization_logo (GtkButton *button, APP_data *data);
void properties_set_default_dashboard (APP_data *data);
void properties_set_default_dates (APP_data *data);
void properties_set_default_schedules (APP_data *data);
void properties_shift_dates (gint shift, APP_data *data);
gchar *on_scale_output (GtkScale *scale,
                         gdouble  value,  APP_data *data);
void on_scaleStart_value_changed (GtkRange *range, APP_data *data);
void on_scaleEnd_value_changed (GtkRange *range, APP_data *data);

#endif /* PROPERTIES_H */
