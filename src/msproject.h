/*
 * File: msproject.c header
 * Description: Contains functions to manipulate ms project
 * xml files
 *
 *
 *
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef MSPROJECT_H
#define MSPROJECT_H

#include <gtk/gtk.h>
#include "misc.h"



/* functions */



void save_to_project (gchar *filename, APP_data *data);

#endif /* MSPROJECT_H */
