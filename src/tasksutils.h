/*
 * various dialogs used by tasks GUI
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#ifndef TASKSUTILS_H
#define TASKSUTILS_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gtkspell/gtkspell.h>
#include "support.h"
#include "misc.h"

/* functions */

GList *tasks_fill_list_groups_for_tasks (gint gr, APP_data *data);
void tasks_free_list_groups (APP_data *data);
gint tasks_count_tasks_linked_to_group (gint id, APP_data *data);
gint tasks_get_group_id (gint rank, APP_data *data);
gint tasks_get_id (gint position, APP_data *data);
gint tasks_get_rank_for_id (gint id, APP_data *data);
gint tasks_get_nearest_group_backward (gint pos, APP_data *data);
gint tasks_get_nearest_group_forward (gint pos, APP_data *data);
void tasks_fill_list_groups (APP_data *data);
gint tasks_get_type (gint position, APP_data *data);
gint tasks_get_duration (gint position, APP_data *data);
gint tasks_get_group (gint position, APP_data *data);
gint tasks_get_group_rank (gint id, APP_data *data);

gboolean tasks_test_increase_duration (gint position, gint incr, APP_data *data);
gboolean tasks_test_decrease_dates (gint position, gint decr, APP_data *data);

void tasks_shift_dates (gint shift, gboolean fGroup, gint id, APP_data *data);
void tasks_group_modify_widget (gint position, APP_data *data);
gint tasks_get_group_rank_in_tasks (gint id, APP_data *data);
gint tasks_check_links_dates_backward (gint position, GDate newStartDate, APP_data *data);
gint tasks_check_links_dates_forward (gint position, GDate newEndDate, APP_data *data);
gint task_get_selected_row (APP_data *data);
void task_set_selected_row (gint pos, APP_data *data);
gint tasks_get_insertion_line (APP_data *data);
gint tasks_datas_len (APP_data *data);
void tasks_get_ressource_datas (gint position, tasks_data *tmp, APP_data *data);
void tasks_store_to_tmp_data (gint position, gchar *name, gint startDay, gint startMonth, gint startYear, 
                            gint endDay, gint endMonth, gint endYear,
                            gint limDay, gint limMonth, gint limYear,
                            gdouble progress, gint pty, gint categ, gint status, 
                            GdkRGBA  color,  APP_data *data, gint id, gboolean incrCounter, gint durMode, gint days, 
                            gint hours, gint minutes, gint calendar, gint delay, tasks_data *tmp);
void tasks_insert (gint position, APP_data *data, tasks_data *tmp);
void tasks_modify_datas (gint position, APP_data *data, tasks_data *tmp);
void tasks_modify_date_datas (gint position, GDate *date_start, GDate *date_end, GDate *date_lim, APP_data *data);
gint tasks_search_group_backward (gint position, APP_data *data);
gint tasks_search_group_forward (gint position, APP_data *data);
gboolean tasks_get_tomato_timer_status (APP_data *data);
void tasks_set_tomato_timer_status (gboolean status, APP_data *data);
void tasks_reset_tomato_timer_value (APP_data *data);
void tasks_store_tomato_timer_ID (gint id, APP_data *data);
void tasks_update_tomato_timer_value_for_ID (gint id, APP_data *data);
void tasks_update_tomato_timer_value (gint secs, APP_data *data);
gint64 tasks_update_get_timer_value (APP_data *data);
gint tasks_get_tomato_timer_ID (APP_data *data);
void tasks_reset_tomato_ellapsed_value (APP_data *data);
void tasks_tomato_update_ellapsed_value (gint secs, APP_data *data);
gint tasks_tomato_get_ellapsed_value (APP_data *data);
void tasks_reset_tomato_paused_value (APP_data *data);
void tasks_tomato_update_paused_value (gint secs, APP_data *data);
gint tasks_tomato_get_paused_value (APP_data *data);
void tasks_tomato_reset_display_status (APP_data *data);
gboolean tasks_tomato_get_display_status (APP_data *data);
void tasks_autoscroll (gdouble value, APP_data *data);
gint tasks_compute_valid_insertion_point (gint pos, gint group, gboolean isGroup, APP_data *data);
void tasks_update_group_limits (gint task_id, gint id_prev_group, gint id_new_group, APP_data *data);

void tasks_utils_reset_errors (APP_data *data);
gint tasks_compute_dates_for_linked_new_task (gint id, gint curDuration, gint curDays, gint insertion_row, gint cancel_rank, 
                                          GDate *current, gboolean fUpdate, gboolean fIsSource, GtkWidget *dlg, APP_data *data);
gint tasks_compute_dates_for_all_linked_task (gint id, gint curDuration, gint curDays, gint insertion_row, 
                                          GDate *current, gint predecessors, gint sucessors, gboolean fUpdate, GtkWidget *dlg, APP_data *data);
void tasks_utils_update_receivers (gint id, APP_data *data);
void tasks_utils_remove_receivers (gint id, APP_data *data);

const gchar *tasks_get_name_for_id (gint id, APP_data *data);
 
#endif /* TASKSUTILS_H */
