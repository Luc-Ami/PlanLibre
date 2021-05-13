/****************************************

  Utilities to access Trello® oard


*****************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef TRELLO_H
#define TRELLO_H

#include <gtk/gtk.h>
#include "misc.h"

/* Local macros */

#define API_KEY "5883f69ff6d87e2058b5a2e16a4c1b41"

/* functions */

gint trello_export_project (APP_data *data);


#endif /* TRELLO_H */
