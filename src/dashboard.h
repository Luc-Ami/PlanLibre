/*
 * DASHBOARD (Kanban) utilities and constants
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <goocanvas.h>
#include <gtkspell/gtkspell.h>
#include "support.h"

/* functions */
void dashboard_reset_counters (APP_data *data);
void dashboard_init (APP_data *data);
void dashboard_remove_all (gboolean fReset, APP_data *data);
void dashboard_add_task (gint position, APP_data *data);
void dashboard_remove_task (gint position, APP_data *data);
void dashboard_set_status (gint id, APP_data *data);

#endif /* DASHBOARD_H */
