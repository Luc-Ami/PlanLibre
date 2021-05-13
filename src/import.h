/*
 * File: import.c header
 * Description: Contains config importation specific functions 
 *
 *
 *
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef IMPORT_H
#define IMPORT_H

#include <gtk/gtk.h>
#include "misc.h"

/* Local macros */

#define MAXRSCVARS 13
#define MAXADRESSBOOKVARS 8
#define MAXTSKVARS 8

/* functions */

gint import_csv_file_to_rsc (gchar *path_to_file, APP_data *data);
void import_tasks_from_csv (APP_data *data);
gint import_adress_book_file_to_rsc (gchar *path_to_file, APP_data *data);


#endif /* IMPORT_H */
