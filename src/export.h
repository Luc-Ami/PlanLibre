/*
 * File: export.c header
 * Description: Contains croutines to export GtkTextview to RTF word processor
 *              format
 *
 *
 *
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef EXPORT_H
#define EXPORT_H

#include <gtk/gtk.h>
#include "misc.h"

/* Local macros */

#define MAX_TAGS_STACK_SIZE 256 /* maximum size of the stack containg Gtk Tags during conversion */
#define STACK_TAG_NONE 0
#define STACK_TAG_BOLD 1
#define STACK_TAG_ITALIC 2
#define STACK_TAG_UNDERLINE 4
#define STACK_TAG_STRIKE 8
#define STACK_TAG_HIGHLIGHT 16
#define STACK_TAG_LEFT 32
#define STACK_TAG_CENTER 64
#define STACK_TAG_RIGHT 128
#define STACK_TAG_FILL 256
#define STACK_TAG_SUPERSCRIPT 512
#define STACK_TAG_SUBSCRIPT 1024
#define STACK_TAG_QUOTATION 2048


gint 
   save_RTF_rich_text (gchar *filename, APP_data *data_app);
gint 
   save_project_to_icalendar (gchar *filename, APP_data *data);
gint 
   save_project_to_icalendar_for_rsc (gchar *filename, gint id, APP_data *data);
gint 
   save_tasks_CSV (APP_data *data);

#endif /* EXPORT_H */
