/*
 * various dialogs used by tasks GUI
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#ifndef TASKSDIALOGS_H
#define TASKSDIALOGS_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gtkspell/gtkspell.h>
#include "support.h"
#include "misc.h"

/* functions */

void tasks_add_display_ressources (gint id,  GtkWidget *box, APP_data *data);
void tasks_add_display_tasks (gint id, GtkWidget *box, APP_data *data);
void tasks_remove_display_tasks (gint id, gint id_to_remove, GtkWidget *box, APP_data *data);
void on_TasksDateBtn_clicked (GtkButton *button, APP_data *data);
void on_TasksStartDateBtn_clicked (GtkButton *button, APP_data *data);
void on_MilestoneDateBtn_clicked (GtkButton *button, APP_data *data);
void on_combo_style_duration_changed (GtkComboBox *widget, APP_data *data);
void on_spin_days_duration_changed (GtkSpinButton *spin_button, APP_data *data);
void on_combo_status_changed (GtkComboBox *widget, APP_data *data);
void on_combo_group_changed (GtkComboBox *widget, APP_data *data);
void on_spinbuttonProgress_changed (GtkSpinButton *spin_button, APP_data *data);
GtkWidget *tasks_create_dialog (GDate *date, gboolean modify, gint id, APP_data *data);
GtkWidget *tasks_create_group_dialog (gboolean modify, gint id, APP_data *data);
GtkWidget *tasks_create_milestone_dialog (gboolean modify, gint id, APP_data *data);
void on_check_limit_toggled (GtkToggleButton *togglebutton, APP_data *data);
void on_combo_rep_mode_changed (GtkComboBox *widget, APP_data *data);
void on_combo_rep_month_changed (GtkComboBox *widget, APP_data *data);
GtkWidget *tasks_create_repeat_dialog (gchar *name, gchar *strDate, APP_data *data);
void task_repeat_error_dialog (GtkWidget *win);
void on_tasks_remove (APP_data *data);

#endif /* TASKSDIALOGS_H */
