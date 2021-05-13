/*
 * various utilities and constants
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#ifndef HOME_H
#define HOME_H

#include <goocanvas.h>
#include <gtkspell/gtkspell.h>
#include "support.h"

/* functions */
void home_today_date (APP_data *data);
void home_project_infos (APP_data *data);
void home_project_tasks (APP_data *data);
void home_set_app_logo ( APP_data *data);
void home_project_milestones (APP_data *data);

#endif /* HOME_H */
