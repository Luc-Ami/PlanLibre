/*
 *****************************
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#ifndef RESSOURCES_H
#define RESSOURCES_H

#include <gtk/gtk.h>
#include <gtkspell/gtkspell.h>
#include "support.h"
#include "misc.h"

/***************************
 * drag n drop
 * ************************/
/* DnD internal Ids for declarations, 2rd field of GtkTargetEntry */
enum  {
      RSC_TARGET_STRING,
      RSC_TARGET_URI,
      RSC_TARGET_HTML,
      RSC_TARGET_TYPE_PIXMAP,
      RSC_TARGET_IMAGE_JPEG,
      RSC_TARGET_IMAGE_PNG,
      RSC_TARGET_IMAGE_SVG,
      RSC_NB_TARGETS
};


/* functions */

void 
   ressources_choose_ressource_logo (GtkButton  *button, APP_data *data );
GtkWidget 
   *ressources_create_dialog (gboolean modifiy, APP_data *data );
GtkWidget 
   *rsc_new_widget (gchar *name, 
                   gint type, gint orga, gdouble cost, gint cost_type, GdkRGBA color, 
                   APP_data *data, gboolean imgFromXml, GdkPixbuf *pixxml, rsc_datas *rsc);
void rsc_modify (APP_data *data);
void rsc_clone (gint index, APP_data *data);
void rsc_new (APP_data *data);
void rsc_free_all (APP_data *data);

gint rsc_datas_len (APP_data *data );
void rsc_store_to_app_data (gint position, gchar *name, gchar *mail, gchar *phone, gchar *reminder, 
                            gint  type,  gint affi, gdouble cost,  gint cost_type,
                            GdkRGBA  color, GdkPixbuf *pix, 
                            APP_data *data, gint id, rsc_datas *tmprsc, gboolean incrCounter);
void 
   rsc_insert (gint position, rsc_datas *rsc, APP_data *data);
void 
   rsc_remove (gint position, GtkListBox *box, APP_data *data);
void
   rsc_get_ressource_datas (gint position, rsc_datas *rsc, APP_data *data);
void 
   rsc_modify_datas (gint position, APP_data *data, rsc_datas *rsc);
void 
   rsc_modify_widget (gint position, APP_data *data);
void rsc_modify_tasks_widgets (gint id, APP_data *data);
gboolean 
   on_widget_button_press_callback (GtkWidget *widget, GdkEvent *event, APP_data *data);
void rsc_remove_all_rows (APP_data *data);
gchar 
   *rsc_get_name (gint id, APP_data *data);
gint 
   rsc_get_number_tasks (gint id, APP_data *data);
gint 
   rsc_get_absolute_max_tasks (APP_data *data);

gint rsc_get_absolute_max_total_tasks (APP_data *data);

gint 
   rsc_get_number_tasks_done (gint id, APP_data *data);
gint 
   rsc_get_number_tasks_overdue (gint id, APP_data *data);
gint 
   rsc_get_number_tasks_to_go (gint id, APP_data *data);
gdouble 
   rsc_get_number_tasks_average_progress (gint id, APP_data *data);
GdkPixbuf 
   *rsc_get_avatar (gint id, APP_data *data);
void 
   rsc_hours_changed (GtkSpinButton *spin_button,
                        APP_data *data);
void rsc_minutes_changed (GtkSpinButton *spin_button,
                        APP_data *data);
gint 
   rsc_get_max_daily_working_time (gint id, APP_data *data);
gdouble 
   rsc_compute_cost (gint id, gint total_minutes, APP_data *data);
gboolean 
   rsc_is_available_for_day (rsc_datas *tmp_rsc_datas, gint day_of_week);
gint rsc_get_rank_for_id (gint id, APP_data *data);
void rsc_autoscroll (gdouble value, APP_data *data);

#endif /* RESSOURCES_H */
