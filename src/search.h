/*
 * DO NOT EDIT THIS FILE - it is generated by Glade.
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#ifndef SEARCH_H
#define SEARCH_H

#include <stdlib.h>
#include <gtk/gtk.h>
#include "support.h"

/* functions */
gint search_first_hit (const gchar *str, APP_data *data);
void search_select_task_row (gint id, APP_data *data);
gint search_next_hit (const gchar *str, APP_data *data);
gint search_prev_hit (const gchar *str, APP_data *data);

#endif /* SEARCH_H */

