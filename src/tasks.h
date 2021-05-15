/*****************************

  tasks management
 *****************************
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#ifndef TASKS_H
#define TASKS_H

#include <gtk/gtk.h>
#include <gtkspell/gtkspell.h>
#include "support.h"
#include "misc.h"

enum {TASK_PLACEMENT_OK = 0,
      TASK_PLACEMENT_UPDATED_END,
      TASK_PLACEMENT_ERR_DEADLINE,
      TASK_PLACEMENT_ERR_AFTER,
      TASK_PLACEMENT_ERR_FIXED};

/* defines & constants */
/* liststore ressources */
typedef enum {
   USE_RSC_COLUMN,
   AVATAR_RSC_COLUMN,
   NAME_RSC_COLUMN,
   COST_RSC_COLUMN,
   ID_RSC_COLUMN,
   NB_COL_RSC,
} column_type;

/* liststore tasks */
typedef enum {
   USE_TSK_COLUMN,
   AVATAR_TSK_COLUMN,
   NAME_TSK_COLUMN,
   ID_TSK_COLUMN,
   LINK_TSK_COLUMN,
   NB_COL_TSK,
} column_tsk_type;

/* structure describing groups used by tasks */
typedef struct {
   gint id;
   gchar *name;
} tsk_group_type;

/* functions */

void tasks_new (GDate *date, APP_data *data);
void tasks_free_all (APP_data *data);
void tasks_modify (APP_data *data);
void tasks_repeat (APP_data *data);
void tasks_remove (gint position, GtkListBox *box, APP_data *data);
void tasks_remove_all_rows (APP_data *data);
GtkWidget *tasks_new_widget (gchar *name, gchar *periods, gdouble progress, gint run, gint pty, 
                             gint categ, GdkRGBA color, gint status, tasks_data *tmp, APP_data *data);
void tasks_move (gint pos, gint dest, gboolean fCcheck, gboolean fUndo, APP_data *data);
void tasks_group_move (gint group, gint pos, gint dest, gboolean fUndo, APP_data *data);
void tasks_group_new (APP_data *data);
GtkWidget *tasks_new_group_widget (gchar *name, gchar *periods, 
                            gdouble progress, gint pty, gint categ, GdkRGBA color, gint status, tasks_data *tmp, APP_data *data);
void tasks_group_modify_widget (gint position, APP_data *data);

void tasks_move_forward (gint nbDays, gint position, APP_data *data);
void tasks_move_backward (gint nbDays, gint position, APP_data *data);
void tasks_change_duration (gint nbDays, gint position, APP_data *data);

void tasks_modify_widget (gint position, APP_data *data);

void tasks_remove_widgets (GtkWidget *container);

void tasks_milestone_modify_widget (gint position, APP_data *data);
void tasks_milestone_new (GDate *date, APP_data *data);

GtkWidget *tasks_new_milestone_widget (gchar *name, gchar *periods, 
                            gdouble progress, gint pty, gint categ, GdkRGBA color, gint status, tasks_data *tmp, APP_data *data);


#endif /* TASKS_H */
