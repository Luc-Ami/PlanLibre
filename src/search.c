#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* translations */
#include <libintl.h>
#include <locale.h>

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "support.h"
#include "misc.h"
#include "search.h"

/*******************************
  variables glo-cales
*******************************/

static gint currentId = -1;

/********************************
 callbacks

********************************/



/******************************
  LOCAL : test link AND
  string presence for current
  task ID

******************************/
static gint search_search_in_rsc (gint id, const gchar *str, APP_data *data)
{
  gint ret = -1, i, total, ok;
  GList *l;
  rsc_datas *tmp_rsc_datas;
  gchar *tmpStr = NULL, *tmpString = NULL;
  
  tmpStr = g_utf8_casefold( str, -1); /* case independant */
  total = g_list_length ( data->rscList);
  i = 0;
  
  while( (i<total) && (ret<0) ) {
     l = g_list_nth (data->rscList, i);
     tmp_rsc_datas = (rsc_datas *)l->data;
     ok = links_check_element ( id, tmp_rsc_datas->id , data);
     if(ok>=0) {
        tmpString = g_utf8_casefold( tmp_rsc_datas->name, -1); /* case independant */
        if(g_strrstr (tmpString, tmpStr)!=NULL ) {/* be careful, case sensitive ! */
           ret = tmp_rsc_datas->id;
        }
        if(tmpString!=NULL) {
           g_free(tmpString);
           tmpString = NULL;
        }
     }
     i++;
  }/* wend */

  g_free( tmpStr);
  return ret;
}


/******************************
  PUBLIC : known last
  search result (in 'currentId'
  variable), searches next 
  occurence
******************************/
gint search_next_hit (const gchar *str, APP_data *data)
{
  gint ret = -1, total, i, link;
  GList *l;
  tasks_data *tmp_tsk_datas;
  gchar *tmpStr = NULL, *tmpString = NULL;

  total = g_list_length ( data->tasksList);
  i = 0;
  ret = -1;
  /* first, we read the list until we reach current hit */
  while( (i<total) && (ret<0)) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if(tmp_tsk_datas->id == currentId)
        ret = currentId; 
     i++;
  }/* wend */
  /* if not found, users has removed task and we return */
  if(ret<0)
     return ret;
  /* now, we search another hit, with an identifier != currentId ; 'i' index is at position */
  ret = -1;
  tmpStr = g_utf8_casefold( str, -1); /* case independant */
  while( (i<total) && (ret<0) ) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     /* is str in name ? */
     tmpString = g_utf8_casefold( tmp_tsk_datas->name, -1); /* case independant */
     if(g_strrstr (tmpString, tmpStr)!=NULL ) {/* be careful, case sensitive ! */
         ret = tmp_tsk_datas->id;
         currentId = ret;
     }
     else { /* is str in ressource name ? */
        /* convenience local function to search inside task's ressources */
        if(tmpString!=NULL) {
           g_free(tmpString);
           tmpString = NULL;
        }
        link = search_search_in_rsc (tmp_tsk_datas->id, str, data);
        if(link>=0) {
           ret = tmp_tsk_datas->id;
           currentId = ret;
        }
     }
     i++;
  }
  g_free(tmpStr);
  return ret;

}

/******************************
  PUBLIC : known last
  search result (in 'currentId'
  variable), searches previous 
  occurence
******************************/
gint search_prev_hit (const gchar *str, APP_data *data)
{
  gint ret = -1, total, i, link;
  GList *l;
  tasks_data *tmp_tsk_datas;
  gchar *tmpStr = NULL, *tmpString = NULL;

  total = g_list_length ( data->tasksList);
  i = 0;
  ret = -1;
  /* first, we read the list until we reach current hit */
  while( (i<total) && (ret<0)) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if(tmp_tsk_datas->id == currentId)
        ret = currentId; 
     i++;
  }/* wend */
  /* if not found, users has removed task and we return */
  if(ret<0)
     return ret;
  /* now, we search another hit, with an identifier != currentId ; 'i' index is at position */
  ret = -1;
  i = i-2;
  tmpStr = g_utf8_casefold( str, -1); /* case independant */
  while( (i>=0) && (ret<0) ) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     /* is str in name ? */
     tmpString = g_utf8_casefold( tmp_tsk_datas->name, -1); /* case independant */
     if(g_strrstr (tmpString, tmpStr)!=NULL ) {/* be careful, case sensitive ! */
         ret = tmp_tsk_datas->id;
         currentId = ret;
     }
     else { /* is str in ressource name ? */
        /* convenience local function to search inside task's ressources */
        if(tmpString!=NULL) {
           g_free(tmpString);
           tmpString = NULL;
        }
        link = search_search_in_rsc (tmp_tsk_datas->id, str, data);
        if(link>=0) {
           ret = tmp_tsk_datas->id;
           currentId = ret;
        }
     }
     i--;
  }
  g_free(tmpStr);
  return ret;

}

/**********************************************
  PUBLIC : given a task ID, this function
  selects the convenient row in listbox

**********************************************/
void search_select_task_row (gint id, APP_data *data)
{
  GtkListBox *box;
  GtkListBoxRow *row;
  GtkWidget *entry;
  gint i, total, ret, start_pos, end_pos;
  GList *l;
  tasks_data *tmp_tsk_datas;

  box = GTK_LIST_BOX(gtk_builder_get_object(data->builder, "listboxTasks")); 
  entry = GTK_WIDGET(gtk_builder_get_object(data->builder, "searchentryTasks")); 
  /* we search index */
  ret = -1;
  i = 0;
  total = g_list_length ( data->tasksList);
  while( (i<total) && (ret<0) ) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if(tmp_tsk_datas->id == id)
        ret = i;
     i++;
  }/* wend */

  /* we select the row */
  if(ret>=0) {
     row = gtk_list_box_get_row_at_index (GTK_LIST_BOX(box), ret);
     gtk_list_box_select_row (GTK_LIST_BOX(box), row);
     /* equivalent to gtk 3.16+ function to grab without selection */
     gtk_widget_grab_focus (GTK_WIDGET (entry));
     gboolean ok = gtk_editable_get_selection_bounds (GTK_EDITABLE(entry), &start_pos, &end_pos);
     gtk_editable_set_position (GTK_EDITABLE(entry), end_pos+1);
  }
}

/***********************************************************
  PUBLIC :  this search function searches the first hit in
  "tasks" datas, containing somewhere the "str" string 
  (in names, in ressources ...)
  returns the internal ID, so
  if users changes ordering, we keep a static (constant) index
  ************************************************************/
gint search_first_hit (const gchar *str, APP_data *data)
{
  gint ret = -1, total, i, link;
  GList *l;
  tasks_data *tmp_tsk_datas;
  gchar *tmpStr = NULL, *tmpString = NULL;

  currentId = -1;
  total = g_list_length (data->tasksList);
  i = 0;
  tmpStr = g_utf8_casefold (str, -1); /* case independant */
  while((i<total) && (ret<0)) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     /* is str in name ? */
     tmpString = g_utf8_casefold (tmp_tsk_datas->name, -1); /* case independant */
     if(g_strrstr (tmpString, tmpStr)!=NULL ) {/* be careful, case sensitive ! */
         ret = tmp_tsk_datas->id;
         currentId = ret;
     }
     else { /* is str in ressource name ? */
        /* convenience local function to search inside task's ressources */
        if(tmpString!=NULL) {
           g_free (tmpString);
           tmpString = NULL;
        }
        link = search_search_in_rsc (tmp_tsk_datas->id, str, data);
        if(link>=0) {
           ret = tmp_tsk_datas->id;
           currentId = ret;
        }
     }
     i++;
  }
  g_free (tmpStr);
  return ret;
}
