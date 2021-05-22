#ifndef CONFIG_H
#define CONFIG_H


#include <gtk/gtk.h>
#include "misc.h"

/* funciions */

void on_edition_prefs_activate  (GtkMenuItem  *menuitem, APP_data *data_app);

gboolean timeout_quick_save( APP_data *data);
gboolean timeout_tomato_timer (APP_data *data);
void on_about1_activate (GtkMenuItem  *menuitem, APP_data *data);
void on_wiki1_activate (GtkMenuItem  *menuitem, APP_data *data);
void on_keyHelp1_activate (GtkMenuItem  *menuitem, APP_data *data);
gboolean on_window_main_destroy(GtkWidget *window1, GdkEvent *event, APP_data *data);
gboolean on_menu_quit_activate (GtkMenuItem  *menuitem, APP_data *data);
void on_file_properties_activate (GtkMenuItem  *menuitem, APP_data *data);
void on_file_new_activate (GtkMenuItem  *menuitem, APP_data *data);
void on_file_open_activate (GtkMenuItem  *menuitem, APP_data *data);
void on_file_save_activate (GtkMenuItem  *menuitem, APP_data *data);
void on_file_save_as_activate (GtkMenuItem  *menuitem, APP_data *data);
void on_edit_reload_activate (GtkMenuItem *menuitem, APP_data *data);

void on_button_undo_clicked (GtkButton *button, APP_data *data);
void
on_edit_undo_activate (GtkMenuItem *menuitem, APP_data *data);
void
on_edit_redo_activate (GtkMenuItem *menuitem, APP_data *data);

void on_button_add_ressources_activate (GtkButton  *button, APP_data *user_data);
void 
   on_button_modify_ressources_activate (GtkButton *button, APP_data *user_data);
void
   on_button_clone_ressources_activate (GtkButton *button, APP_data *data);
void 
   on_button_multiply_ressources_activate (GtkButton *button, APP_data *data);

void on_page_switched (GtkNotebook *notebook,
                       GtkWidget   *page,
                       guint        page_num,
                       APP_data *data);
void on_ressources_row_selected (GtkListBox *box,
                            GtkListBoxRow *row,
                            APP_data *data);
void
   on_button_add_task_activate (GtkButton  *button, APP_data *user_data);
void 
   on_menu_append_task_activate (GtkMenuItem *menuitem, APP_data *data);
void
   on_menu_export_task_activate (GtkMenuItem *menuitem, APP_data *data);
void
on_button_add_group_activate (GtkButton  *button, APP_data *user_data);
void
on_button_modify_task_activate (GtkButton  *button, APP_data *user_data);
void
on_button_repeat_task_activate (GtkButton *button, APP_data *data);
void 
  on_buttonRemove_tasks_clicked (GtkButton *button, APP_data *data);

void
on_button_append_task_activate (GtkButton *button, APP_data *data);
void 
  on_button_add_milestone_activate (GtkButton *button, APP_data *data);
void 
  on_button_tomato_timer_task_activate (GtkButton *button, APP_data *data);

void on_tasks_row_selected (GtkListBox *box,
                            GtkListBoxRow *row,
                            APP_data *data);
void
   on_rsc_append_from_library (GtkMenuItem *menuitem, APP_data *data);
void 
   on_button_rsc_append_from_library (GtkMenuItem *menuitem, APP_data *data);
void
   on_rsc_append_from_csv (GtkMenuItem *menuitem, APP_data *data);
void 
   on_rsc_append_from_thunderbird (GtkMenuItem *menuitem, APP_data *data);
void
   on_button_export_report_clicked (GtkButton  *button, APP_data *data);
void
   on_menu_export_report_activate (GtkMenuItem  *menuitem, APP_data *data);
void
on_timeline_export_pdf_activate (GtkMenuItem *menuitem, APP_data *data);
void
on_timeline_export_image_activate (GtkMenuItem *menuitem, APP_data *data);
void
on_timeline_export_drawing_activate (GtkMenuItem *menuitem, APP_data *data);
void
on_timeline_time_shift_activate (GtkMenuItem *menuitem, APP_data *data);
void on_trello_export_project_activate (GtkMenuItem *menuitem, APP_data *data);
void 
   on_group_time_shift_activate (GtkButton *button, APP_data *data);
void
on_button_export_pdf_clicked (GtkButton *button, APP_data *data);
void
on_button_export_image_clicked (GtkButton *button, APP_data *data);
void
on_button_export_drawing_clicked (GtkButton *button, APP_data *data);

void on_button_TL_go_start_clicked (GtkButton *button, APP_data *data);

void
on_button_TL_prev_page_clicked (GtkButton *button, APP_data *data);
void
on_button_TL_prev_year_clicked (GtkButton *button, APP_data *data);
void
on_button_TL_next_page_clicked (GtkButton *button, APP_data *data);
void
on_button_TL_next_year_clicked (GtkButton *button, APP_data *data);
void
  on_project_calendar_activate (GtkMenuItem *menuitem, APP_data *data);
void
  on_project_export_agenda (GtkMenuItem *menuitem, APP_data *data);
void
on_button_export_assign_pdf_clicked (GtkButton *button, APP_data *data);
void
on_menu_export_assign_image_clicked (GtkMenuItem *menuitem, APP_data *data);
void
on_menu_export_assign_drawing_clicked (GtkMenuItem *menuitem, APP_data *data);

#endif /* CALLBACKS_H */
