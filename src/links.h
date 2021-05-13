/*
 * various utilities for links objects 
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#ifndef LINKS_H
#define LINKS_H

#include "support.h"
#include "misc.h"

/* constants */

#define LINK_TYPE_TSK_RSC 0
#define LINK_TYPE_TSK_TSK 1
/* linking modes for tasks */
#define LINK_END_START 0
#define LINK_START_START 1
#define LINK_END_END 2
#define LINK_START_END 3

/* functions */
void links_append_element (gint idElem1, gint idElem2, gint type, gint mode, APP_data *data);
void links_free_all (APP_data *data);
void links_modify_datas (gint position, APP_data *data, link_datas values );
void links_get_ressource_datas (gint position, link_datas *values, APP_data *data );
gint links_check_element (gint receiver, gint sender, APP_data *data);
void links_free_at (gint position, APP_data *data);
void links_remove_all_task_links (gint id, APP_data *data);
void links_remove_all_rsc_links (gint id, APP_data *data);
gint links_test_concurrent_rsc_usage (gint id, gint sender, gint id_dest, GDate start, GDate end, APP_data *data );
void links_remove_rsc_task_link (gint idTsk, gint idRsc, APP_data *data);
gint links_check_rsc (gint rsc, APP_data *data);
void links_copy_rsc_usage (gint id_src, gint id_dest, GDate start, GDate end, APP_data *data);
GList *links_copy_all (APP_data *data);

gint links_check_task_linked (gint tsk, APP_data *data);
gint links_check_task_successors (gint tsk, APP_data *data);
gint links_check_task_predecessors (gint tsk, APP_data *data);
gboolean links_test_circularity (gint tsk, gint other_tsk, APP_data *data);
GDate links_get_latest_task_linked (gint tsk, APP_data *data);
void links_remove_tasks_ending_too_late (gint id, GDate *lim_date, APP_data *data);
gint links_get_element_link_mode (gint receiver, gint sender, APP_data *data);

#endif /* LINKS_H */
