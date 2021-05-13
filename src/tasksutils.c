#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <stdlib.h>
/* translations */
#include <libintl.h>
#include <locale.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtkspell/gtkspell.h>
#include "support.h"
#include "misc.h"
#include "links.h"
#include "tasksdialogs.h"
#include "tasks.h"
#include "ressources.h"
#include "tasksutils.h"
#include "dashboard.h"
#include "undoredo.h"

/* constants */




/* 
  variables 
*/
static gboolean fTomatoTimer = FALSE;
static gint64 timer_val = 0;/* in seconds */
static gint timer_task_id = -1; /* store an identifier ID on current task using timer */
static gint ellapsed = 0; /* local ellapsed time value */
static gint paused = 0; /* local paused time value */
static gboolean fTomatoUpdateDisplay = TRUE; /* flag in order to update displays */
static gboolean fErrEnd = FALSE, fErrDeadline = FALSE, fErrFixed = FALSE, fErrAfter = FALSE;

/* 
  functions
*/

/***********************************
 PUBLIC : reset errors flags 
***********************************/
void tasks_utils_reset_errors (APP_data *data)
{
  fErrEnd = FALSE;
  fErrDeadline = FALSE;
  fErrFixed = FALSE;
  fErrAfter = FALSE;
}
/*******************************************
  PUBLIC : build a GList with tasks using
  current group (argument gr for group ID)
  datas are only used by Undo engines, 
  and freed by Underedo.c module
*******************************************/
GList *tasks_fill_list_groups_for_tasks (gint gr, APP_data *data)
{
   GList *l = NULL, *l1;
   gint i;
   group_datas *tmpGroup;
   tasks_data *tmp_tsk_datas;

   for(i=0;i<g_list_length (data->tasksList);i++) {
      l1 = g_list_nth (data->tasksList, i);
      tmp_tsk_datas = (tasks_data *) l1->data;
      if(tmp_tsk_datas->type == TASKS_TYPE_TASK) {
         if(tmp_tsk_datas->group == gr) {
            tmpGroup = g_malloc (sizeof(group_datas));
            tmpGroup->id = tmp_tsk_datas->id;
            l = g_list_append (l, tmpGroup);
printf ("le groupe %d est associé à la taĉhe %d \n", gr, tmp_tsk_datas->id);
         }
      }
   }
   return l;
}

/*****************************************
  PUBLIC : compute how many tasks belongs
  to a group ;
  argument : internal ID of group
*****************************************/
gint tasks_count_tasks_linked_to_group (gint id, APP_data *data)
{
  gint i, ret = 0;
  GList *l;
  tasks_data *tmp;

  for(i=0;i<g_list_length (data->tasksList);i++) {
     l = g_list_nth (data->tasksList, i);
     tmp = (tasks_data *) l->data;
     if(tmp->type == TASKS_TYPE_TASK) {
        if(tmp->group == id)
          ret++;
     }
  }
  return ret;
}

/***************************************
  PUBLIC : free GList with groups IDs
***************************************/
void tasks_free_list_groups (APP_data *data)
{
  gint i;
  GList *l;
  tsk_group_type *tmp;

  for(i=0;i<g_list_length (data->groups);i++) {
     l = g_list_nth (data->groups, i);
     tmp = (tsk_group_type *) l->data;
     if(tmp->name)
        g_free (tmp->name);
     g_free (tmp);
  }
  g_list_free (data->groups); /* easy, nothing inside is dynamically allocated, trings are only pointers to others strings */
  data->groups = NULL;
}

/**************************************
 PUBLIC : given rank on GList of groups,
 returns TRUE Id of group object
***************************************/
gint tasks_get_group_id (gint rank, APP_data *data)
{
  GList *l;
  tsk_group_type *tmp;
  gint ret = -1;/* security */

  if(rank>=0) {
      l = g_list_nth (data->groups, rank);
      tmp = (tsk_group_type *) l->data;
      ret = tmp->id;
  }
  return ret;
}

/***************************************
  PUBLIC : fill GList with groups IDs
***************************************/
void tasks_fill_list_groups (APP_data *data)
{
   gint i;
   GList *l;
   tasks_data *tmp_tsk_datas;
   tsk_group_type *tmp;

   /* first element _always for "none" group */
   tmp = g_malloc (sizeof(tsk_group_type));
   tmp->id = -1;
   tmp->name = NULL;
   data->groups = g_list_append (data->groups, tmp);
   /* other elements */
   for(i=0;i<g_list_length (data->tasksList);i++) {
      l = g_list_nth (data->tasksList, i);
      tmp_tsk_datas = (tasks_data *) l->data;
      if(tmp_tsk_datas->type == TASKS_TYPE_GROUP) {
         tmp = g_malloc (sizeof(tsk_group_type));
         tmp->id = tmp_tsk_datas->id;
         tmp->name = g_strdup_printf ("%s", tmp_tsk_datas->name);
         data->groups = g_list_append (data->groups, tmp);
      }
   }
}

/*********************************************
  PUBLIC : returns 'rank' of task known
  is ID
*********************************************/
gint tasks_get_rank_for_id (gint id, APP_data *data)
{
  gint ret = -1, i = 0;
  GList *l;
  tasks_data *tmp_tsk_datas;

  while((i<g_list_length (data->tasksList)) && (ret<0)) {
      l = g_list_nth (data->tasksList, i);
      tmp_tsk_datas = (tasks_data *) l->data;
      if(tmp_tsk_datas->id == id)
          ret = i;
      i++;
  }
  return ret;
}

/*********************************************
  PUBLIC : returns 'type' of task (standard,
   group or milestone, or -1 if error
*********************************************/
gint tasks_get_type (gint position, APP_data *data)
{
  gint ret = -1;
  tasks_data *tmp_tasks_datas;
  gint tsk_list_len = g_list_length (data->tasksList);
  GList *l;

  if((position >=0) && (position < tsk_list_len)) {
     l = g_list_nth (data->tasksList, position);
     tmp_tasks_datas = (tasks_data *)l->data;
     ret = tmp_tasks_datas->type;
  }
  return ret;
}

/*********************************************
  PUBLIC : returns 'type' of duration (as soon,
   not after or fixed, or -1 if error
*********************************************/
gint tasks_get_duration (gint position, APP_data *data)
{
  gint ret = -1;
  tasks_data *tmp_tasks_datas;
  gint tsk_list_len = g_list_length (data->tasksList);
  GList *l;

  if((position >=0) && (position < tsk_list_len)) {
     l = g_list_nth (data->tasksList, position);
     tmp_tasks_datas = (tasks_data *)l->data;
     ret = tmp_tasks_datas->durationMode;
  }
  return ret;
}

/*********************************************
  PUBLIC : returns 'ID' of task (standard,
   group or milestone, or -1 if error
*********************************************/
gint tasks_get_id (gint position, APP_data *data)
{
  gint ret = -1;
  tasks_data *tmp_tasks_datas;
  gint tsk_list_len = g_list_length (data->tasksList);
  GList *l;

  if((position >=0) && (position < tsk_list_len)) {
     l = g_list_nth (data->tasksList, position);
     tmp_tasks_datas = (tasks_data *)l->data;
     ret = tmp_tasks_datas->id;
  }
  return ret;
}

/*********************************************
  PUBLIC : returns 'group' of task 
*********************************************/
gint tasks_get_group (gint position, APP_data *data)
{
  gint ret = -1;
  tasks_data *tmp_tasks_datas;
  gint tsk_list_len = g_list_length (data->tasksList);
  GList *l;

  if((position >=0) && (position < tsk_list_len)) {
     l = g_list_nth (data->tasksList, position);
     tmp_tasks_datas = (tasks_data *)l->data;
     ret = tmp_tasks_datas->group;
  }
  return ret;
}

/**************************************
 PUBLIC : given position of current
 group, retrieve the nearest group backward
 returns -1 if none
***************************************/
gint tasks_get_nearest_group_backward (gint pos, APP_data *data)
{
  gint cur_pos, ret = -1;
  GList *l;
  tasks_data *tmp;

  if(pos<=0)
    return -1;

  cur_pos = pos-1;
  while((cur_pos>=0) && (ret == -1)) {
     l = g_list_nth (data->tasksList, cur_pos);
     tmp = (tasks_data *)l->data;
     if(tmp->type == TASKS_TYPE_GROUP) {
         ret = cur_pos;
     }
     cur_pos--;
  }
  return ret;
}

/**************************************
 PUBLIC : given position of current
 group, retrieve the nearest group forward
 returns -1 if none
***************************************/
gint tasks_get_nearest_group_forward (gint pos, APP_data *data)
{
  gint cur_pos, ret = -1, total, group;
  GList *l;
  tasks_data *tmp;

  if(pos<0)
    return -1;

  total = g_list_length (data->tasksList);
  if(pos>total)
    return total;

  cur_pos = pos+1;
  while((cur_pos<total) && (ret == -1)) {
     l = g_list_nth (data->tasksList, cur_pos);
     tmp = (tasks_data *)l->data;
     if(tmp->type == TASKS_TYPE_GROUP) {
         group = tmp->id;
         ret = cur_pos;
         cur_pos++;
         /* we seek until we reach a task withour any linj with 'group' */
         while(cur_pos<total) {
            l = g_list_nth (data->tasksList, cur_pos);
            tmp = (tasks_data *)l->data;
            if(tmp->group == group)
               ret = cur_pos;
            cur_pos++;
         }
     }
     cur_pos++;
  }
  return ret;
}

/**************************************
 PUBLIC : given ID  group, retrieve its
 rank in groups GList
***************************************/
gint tasks_get_group_rank (gint id, APP_data *data)
{
  gint ret = 0, i=1;
  GList *l;
  tsk_group_type *tmp;

  while((i<g_list_length (data->groups)) && (ret==0)) {
     l = g_list_nth (data->groups, i);
     tmp = (tsk_group_type *) l->data;
     if(tmp->id == id)
          ret = i;
     i++;
  }
  return ret;
}

/**************************************
 PUBLIC : given rank  pf task, returns
 if its duration can be increased
***************************************/
gboolean tasks_test_increase_duration (gint position, gint incr, APP_data *data)
{
  gboolean ret = TRUE;/* yes, default, because only tasks with duration mode DEADLINE are tested */
  tasks_data *tmp;
  GList *l;
  GDate lim_date, end_date;
  gint cmp;
  
  if(position>=0 && position<=g_list_length (data->tasksList)) {
     l = g_list_nth (data->tasksList, position);
     tmp = (tasks_data *)l->data;
     if(tmp->durationMode == TASKS_DEADLINE) {
        g_date_set_dmy (&end_date, tmp->end_nthDay, tmp->end_nthMonth, tmp->end_nthYear);
        g_date_add_days (&end_date, incr);
        g_date_set_dmy (&lim_date, tmp->lim_nthDay, tmp->lim_nthMonth, tmp->lim_nthYear);
        cmp = g_date_compare (&end_date, &lim_date);
        if(cmp>0)
          ret = FALSE;
     }/* endif */
  }
  return ret;
}

/**************************************
 PUBLIC : given rank  task, returns
 if its duration can be increased
***************************************/
gboolean tasks_test_decrease_dates (gint position, gint decr, APP_data *data)
{
  gboolean ret = TRUE;/* yes, default, because only tasks with duration mode DEADLINE or NOT BEFORE are tested */
  tasks_data *tmp;
  GList *l;
  GDate lim_date, start_date;
  gint cmp;
  
  if(position>=0 && position<=g_list_length (data->tasksList)) {
     l = g_list_nth (data->tasksList, position);
     tmp = (tasks_data *)l->data;
     if(tmp->durationMode == TASKS_AFTER) {
        g_date_set_dmy (&start_date, tmp->start_nthDay, tmp->start_nthMonth, tmp->start_nthYear);
        g_date_subtract_days (&start_date, ABS(decr));
        g_date_set_dmy (&lim_date, tmp->lim_nthDay, tmp->lim_nthMonth, tmp->lim_nthYear);
        cmp = g_date_compare (&start_date, &lim_date);
        if(cmp<0)
          ret = FALSE;
     }/* endif */
  }
  return ret;
}

/*****************************************************
  PUBLIC : shift all tasks dates
  shift : in days from -oo to +oo
  fGroup : if TRUE, test only for group 'ID'
******************************************************/
void tasks_shift_dates (gint shift, gboolean fGroup, gint id, APP_data *data)
{
  GDate date_start, date_end, lim_date, current_date;
  gint i, total, status, cmp_date;
  GList *l;
  tasks_data *tmp_tsk_datas;
  gchar *human_date;
  GtkWidget *labelWait, *labelRun, *labelDelay, *labelDone;

  total = g_list_length (data->tasksList);
  g_date_set_time_t (&current_date, time (NULL)); /* date of today */

  if((shift != 0) && (total>0)) {

     dashboard_reset_counters (data);
     /* get pointers on labels */
     labelWait = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelWait"));
     labelRun = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelRun"));
     labelDelay = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelDelay"));
     labelDone = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelDone"));
     /* we change labels */
     gtk_label_set_text (GTK_LABEL(labelWait), "0");	  
     gtk_label_set_text (GTK_LABEL(labelRun), "0");
     gtk_label_set_text (GTK_LABEL(labelDelay), "0");
     gtk_label_set_text (GTK_LABEL(labelDone), "0");

     if(shift<0) {/* advance date */
        for(i=0;i<total;i++) {
           l = g_list_nth (data->tasksList, i);
           tmp_tsk_datas = (tasks_data *)l->data;
           status = tmp_tsk_datas->status;
           /* test for group only version */
           if((fGroup==FALSE)  || (fGroup==TRUE && tmp_tsk_datas->group ==id) || (fGroup==TRUE && tmp_tsk_datas->id ==id)) {
             g_date_set_dmy (&date_start, tmp_tsk_datas->start_nthDay, tmp_tsk_datas->start_nthMonth, 
                              tmp_tsk_datas->start_nthYear);
             g_date_set_dmy (&date_end, tmp_tsk_datas->end_nthDay, tmp_tsk_datas->end_nthMonth, tmp_tsk_datas->end_nthYear);
             if(tmp_tsk_datas->durationMode!=TASKS_AS_SOON) {
                 g_date_set_dmy (&lim_date, tmp_tsk_datas->lim_nthDay, tmp_tsk_datas->lim_nthMonth, tmp_tsk_datas->lim_nthYear);
                 g_date_subtract_days (&lim_date, ABS (shift));
                 tmp_tsk_datas->lim_nthDay = g_date_get_day (&lim_date);
                 tmp_tsk_datas->lim_nthMonth = g_date_get_month (&lim_date);
                 tmp_tsk_datas->lim_nthYear = g_date_get_year (&lim_date);
             }
             g_date_subtract_days (&date_start, ABS (shift));
             g_date_subtract_days (&date_end, ABS (shift));
             /* modify in place */
             tmp_tsk_datas->start_nthDay = g_date_get_day (&date_start);
             tmp_tsk_datas->start_nthMonth = g_date_get_month (&date_start);
             tmp_tsk_datas->start_nthYear = g_date_get_year (&date_start);
             human_date = misc_convert_date_to_friendly_str (&date_start);
             if(tmp_tsk_datas->type != TASKS_TYPE_GROUP) {   
                 if(tmp_tsk_datas->type == TASKS_TYPE_TASK)     
                    gtk_label_set_markup (GTK_LABEL(tmp_tsk_datas->cPeriod),
                           g_strdup_printf (_("<i>%s, %d day(s)</i>"), human_date, tmp_tsk_datas->days)); 
                 else
                    gtk_label_set_markup (GTK_LABEL(tmp_tsk_datas->cPeriod), 
                                 g_strdup_printf ("<i><b>%s</b></i>", human_date));
             }
             g_free (human_date);
             tmp_tsk_datas->end_nthDay = g_date_get_day (&date_end);
             tmp_tsk_datas->end_nthMonth = g_date_get_month (&date_end);
             tmp_tsk_datas->end_nthYear = g_date_get_year (&date_end);
           }
           /* check status */
           if((tmp_tsk_datas->type == TASKS_TYPE_TASK && fGroup==FALSE)  || (fGroup==TRUE && tmp_tsk_datas->group ==id)) {
              cmp_date = g_date_compare (&current_date, &date_start);
              if(cmp_date<0) {/* task starts AFTER current date */
                   tmp_tsk_datas->status = TASKS_STATUS_TO_GO;
                   gtk_image_set_from_icon_name (GTK_IMAGE(tmp_tsk_datas->cImgStatus), "appointment-new",
                             GTK_ICON_SIZE_LARGE_TOOLBAR);
              }
              else {
                 cmp_date = g_date_compare (&current_date, &date_end);
                 if(cmp_date>0) {
                     tmp_tsk_datas->status = TASKS_STATUS_DELAY;
                     gtk_image_set_from_icon_name (GTK_IMAGE(tmp_tsk_datas->cImgStatus), "emblem-urgent",
                             GTK_ICON_SIZE_LARGE_TOOLBAR);
                 }
                 else {
                    if(tmp_tsk_datas->progress == 100) {
                       tmp_tsk_datas->status = TASKS_STATUS_DONE;
                       gtk_image_set_from_icon_name (GTK_IMAGE(tmp_tsk_datas->cImgStatus), "emblem-default",
                             GTK_ICON_SIZE_LARGE_TOOLBAR);
                    }
                    else {
                       tmp_tsk_datas->status = TASKS_STATUS_RUN;
                       gtk_image_set_from_icon_name (GTK_IMAGE(tmp_tsk_datas->cImgStatus), "system-run",
                             GTK_ICON_SIZE_LARGE_TOOLBAR);
                    }
                 }
              }	      
              dashboard_add_task (i, data);
           }/* endif status task */
        }/* next i */
     }
     else {/* delay date */
        for(i=0;i<total;i++) {
           l = g_list_nth (data->tasksList, i);
           tmp_tsk_datas = (tasks_data *)l->data;
           status = tmp_tsk_datas->status;
           /* test for group only version */
           if((fGroup==FALSE)  || (fGroup==TRUE && tmp_tsk_datas->group ==id) || (fGroup==TRUE && tmp_tsk_datas->id ==id)) {
             g_date_set_dmy (&date_start, tmp_tsk_datas->start_nthDay, tmp_tsk_datas->start_nthMonth, 
                              tmp_tsk_datas->start_nthYear);
             g_date_set_dmy (&date_end, tmp_tsk_datas->end_nthDay, tmp_tsk_datas->end_nthMonth, tmp_tsk_datas->end_nthYear);
             g_date_add_days (&date_start, shift);
             g_date_add_days (&date_end, shift);
             if(tmp_tsk_datas->durationMode!=TASKS_AS_SOON) {
                 g_date_set_dmy (&lim_date, tmp_tsk_datas->lim_nthDay, tmp_tsk_datas->lim_nthMonth, tmp_tsk_datas->lim_nthYear);
                 g_date_subtract_days (&lim_date, shift);
                 tmp_tsk_datas->lim_nthDay = g_date_get_day (&lim_date);
                 tmp_tsk_datas->lim_nthMonth = g_date_get_month (&lim_date);
                 tmp_tsk_datas->lim_nthYear = g_date_get_year (&lim_date);
             } 
             /* modify in place */
             tmp_tsk_datas->start_nthDay = g_date_get_day (&date_start);
             tmp_tsk_datas->start_nthMonth = g_date_get_month (&date_start);
             tmp_tsk_datas->start_nthYear = g_date_get_year (&date_start);
             human_date = misc_convert_date_to_friendly_str (&date_start);
             if(tmp_tsk_datas->type != TASKS_TYPE_GROUP) {   
                 if(tmp_tsk_datas->type == TASKS_TYPE_TASK)     
                    gtk_label_set_markup (GTK_LABEL(tmp_tsk_datas->cPeriod),
                           g_strdup_printf (_("<i>%s, %d day(s)</i>"), human_date, tmp_tsk_datas->days)); 
                 else
                    gtk_label_set_markup (GTK_LABEL(tmp_tsk_datas->cPeriod), 
                                 g_strdup_printf ("<i><b>%s</b></i>", human_date));
             }
             g_free (human_date);
             tmp_tsk_datas->end_nthDay = g_date_get_day (&date_end);
             tmp_tsk_datas->end_nthMonth = g_date_get_month (&date_end);
             tmp_tsk_datas->end_nthYear = g_date_get_year (&date_end);
           }
           /* check status */
           if((tmp_tsk_datas->type == TASKS_TYPE_TASK && fGroup==FALSE) || (fGroup==TRUE && tmp_tsk_datas->group ==id)) {
              cmp_date = g_date_compare (&current_date, &date_start);
              if(cmp_date<0) {/* task starts AFTER current date */
                  tmp_tsk_datas->status = TASKS_STATUS_TO_GO;
                  gtk_image_set_from_icon_name (GTK_IMAGE(tmp_tsk_datas->cImgStatus), "appointment-new",
                             GTK_ICON_SIZE_LARGE_TOOLBAR);
              }
              else {
                 cmp_date = g_date_compare (&current_date, &date_end);
                 if(cmp_date>0) {
                     tmp_tsk_datas->status = TASKS_STATUS_DELAY;
                     gtk_image_set_from_icon_name (GTK_IMAGE(tmp_tsk_datas->cImgStatus), "emblem-urgent",
                             GTK_ICON_SIZE_LARGE_TOOLBAR);
                 }
                 else {
                    if(tmp_tsk_datas->progress == 100) {
                       tmp_tsk_datas->status = TASKS_STATUS_DONE;
                       gtk_image_set_from_icon_name (GTK_IMAGE(tmp_tsk_datas->cImgStatus), "emblem-default",
                             GTK_ICON_SIZE_LARGE_TOOLBAR);
                    }
                    else {
                       tmp_tsk_datas->status = TASKS_STATUS_RUN;
                       gtk_image_set_from_icon_name (GTK_IMAGE(tmp_tsk_datas->cImgStatus), "system-run",
                             GTK_ICON_SIZE_LARGE_TOOLBAR);
                    }
                 }

              }
	      
              dashboard_add_task (i, data);
           }/* endif task */
        }/* next i */
     }/* else */
  }/* endif shift !=0 */  
}

/********************************************
  PUBLIC function
  modify a group widget at a given position
  NOTE : this function must be called
  ONLY when datas are modified in 
  memory, with "tasks_modify_datas()'
********************************************/
void tasks_group_modify_widget (gint position, APP_data *data)
{
  gint count, rsc_list_len = g_list_length (data->tasksList);
  GList *l;
  gchar *tmpStr;
  GDate grp_start, grp_end;

  if((position >=0) && (position<=rsc_list_len)) {
     l = g_list_nth (data->tasksList, position);
     tasks_data *tmp_tasks_datas;
     tmp_tasks_datas = (tasks_data *)l->data;
     /* we change labels */
     tmpStr = g_markup_printf_escaped ("<u><big><big><b>%s</b></big></big></u>", tmp_tasks_datas->name);
     gtk_label_set_markup (GTK_LABEL(tmp_tasks_datas->cName), tmpStr);
     g_free (tmpStr);

     count = tasks_count_tasks_linked_to_group (tmp_tasks_datas->id, data);
     if(count>0)
         tmpStr = g_strdup_printf (_("<i>%d day(s)</i>"), tmp_tasks_datas->days);
     else {
         tmpStr = g_strdup_printf ("%s", _("<i>Empty</i>"));
         tmp_tasks_datas->start_nthDay = data->properties.start_nthDay;
         tmp_tasks_datas->start_nthMonth = data->properties.start_nthMonth;
         tmp_tasks_datas->start_nthYear = data->properties.start_nthYear;
         tmp_tasks_datas->end_nthDay = data->properties.end_nthDay;
         tmp_tasks_datas->end_nthMonth = data->properties.end_nthMonth;
         tmp_tasks_datas->end_nthYear = data->properties.end_nthYear;
         g_date_set_dmy (&grp_start, data->properties.start_nthDay, data->properties.start_nthMonth, 
                         data->properties.start_nthYear);
         g_date_set_dmy (&grp_end, data->properties.end_nthDay, data->properties.end_nthMonth, data->properties.end_nthYear);
         tmp_tasks_datas->days = g_date_days_between (&grp_start, &grp_end)+1;
     }
     gtk_label_set_markup (GTK_LABEL(tmp_tasks_datas->cPeriod), tmpStr);
     g_free (tmpStr);
  }/* endif */
}

/**************************************
 PUBLIC : given group ID within tasks, 
 retrieve its rank in tasks GList
***************************************/
gint tasks_get_group_rank_in_tasks (gint id, APP_data *data)
{
  gint ret = 0, i=1;
  GList *l;
  tasks_data *tmp;

  while((i<g_list_length (data->tasksList)) && (ret==0)) {
     l = g_list_nth (data->tasksList, i);
     tmp = (tasks_data *) l->data;
     if(tmp->id == id)
          ret = i;
     i++;
  }
  return ret;
}

/*************************************************************
  PUBLIC function checks if task with predecessor(s)
  can be moved on timeline
  returns a value !=0 it can't
  arguments : position : vertical index on timeline
  id = internal identifier of task
  newStartDate : new date to test with  predecessors
*****************************************************/
gint tasks_check_links_dates_backward (gint position, GDate newStartDate, APP_data *data)
{
  gint i, link, ret = 0;
  gint rsc_list_len = g_list_length (data->tasksList);
  GList *l;
  GDate date_end_link;
  tasks_data *tmp_tsk_datas, *tmp_datas;

  /* now, same for the current selected row */
  if((position >= 0) && (position <= rsc_list_len)) {
     /* we get an access to current task datas */
     l = g_list_nth (data->tasksList, position);
     tmp_datas = (tasks_data *)l->data;
     /* now, we are looking for predecessors tasks */
     for(i=0; i < rsc_list_len ; i++) {
        l = g_list_nth (data->tasksList, i);
        tmp_tsk_datas = (tasks_data *)l->data;

        if((tmp_datas->id != tmp_tsk_datas->id) && (tmp_tsk_datas->type != TASKS_TYPE_GROUP)) {
            link = links_check_element (tmp_datas->id, tmp_tsk_datas->id, data);
            if(link>=0) {/* there is a predecessor */
              /* now we check dates compatibilities */
              g_date_set_dmy (&date_end_link, tmp_tsk_datas->end_nthDay, tmp_tsk_datas->end_nthMonth, tmp_tsk_datas->end_nthYear);
              gint cmp_date = g_date_compare (&newStartDate, &date_end_link);
              if(cmp_date<=0) {
                   ret = -1;/* error */
              }
            }/* endif link */
        }/* endif id */
     }/* next i */
  }/* endif position */

  return ret;
}

/*********************************************
  PUBLIC function check if task with children
  can be moved on timeline
  returns a value !=0 it can't
  position : vertical index on timeline
  id = internal identifier of task
  newStartDate : new date to test with
     predecessors
********************************************/
gint tasks_check_links_dates_forward (gint position, GDate newEndDate, APP_data *data)
{
  gint i, link, ret = 0;
  gint rsc_list_len = g_list_length (data->tasksList);
  GList *l;
  GDate date_start_link;
  tasks_data *tmp_tsk_datas, *tmp_datas;

  /* now, same for the current selected row */
  if((position >= 0) && (position < rsc_list_len)) {
     /* we get an access to current task datas */
     l = g_list_nth (data->tasksList, position);
     tmp_datas = (tasks_data *)l->data;
     /* now, we are looking for children tasks */
     for(i=0; i < rsc_list_len ; i++) {
        l = g_list_nth (data->tasksList, i);
        tmp_tsk_datas = (tasks_data *)l->data;

        if((tmp_datas->id != tmp_tsk_datas->id) && (tmp_tsk_datas->type != TASKS_TYPE_GROUP)) {
            link = links_check_element (tmp_tsk_datas->id, tmp_datas->id, data);
            if(link>=0) {/* there is a children */
               /* now we check dates compatibilities */
               g_date_set_dmy (&date_start_link, tmp_tsk_datas->start_nthDay, tmp_tsk_datas->start_nthMonth, 
                               tmp_tsk_datas->start_nthYear);
               gint cmp_date = g_date_compare (&newEndDate, &date_start_link);
               if(cmp_date>=0) {
                   ret = -1;/* error */
               }
            }/* endif link */
        }/* endif id */
     }/* next i */
  }/* endif position */

  return ret;
}

/***************************************
  PUBLIC function 
  get row selected in GtkLisbox
****************************************/
gint task_get_selected_row (APP_data *data)
{
  GtkListBox *box;
  GtkListBoxRow *row;
  gint ret=0;/* security */

  box = GTK_LIST_BOX(GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasks"))); 
  row =  gtk_list_box_get_selected_row (box);
  if(row) {
        ret = gtk_list_box_row_get_index (row);  
  }
  return ret;
}

/***************************************
  PUBLIC function 
  set row selected in GtkLisbox
****************************************/
void task_set_selected_row (gint pos, APP_data *data)
{
  GtkListBox *box;
  GtkListBoxRow *row;

  if(pos>=0 && pos<g_list_length (data->tasksList)) {
     box = GTK_LIST_BOX(GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasks"))); 
     row = gtk_list_box_get_row_at_index (GTK_LIST_BOX(box), pos);
     if(row) {
        gtk_list_box_select_row (GTK_LIST_BOX(box), row);
     }
  }
}

/**********************************
  PUBLIC : compute insertion point 
**********************************/
gint tasks_get_insertion_line (APP_data *data)
{
  gint insertion_row;

  data->curTasks = task_get_selected_row (data);
  if(data->curTasks<0)
      insertion_row = tasks_datas_len (data);
  else {
      insertion_row = data->curTasks;
      if(insertion_row == tasks_datas_len (data)-1) /* it's the end of list */
	  insertion_row = tasks_datas_len (data);
      else
	  insertion_row++;
  }
  return insertion_row;
}

/***********************************
  PUBLIC function
  give total number
  of elements in rsc datas list
***********************************/
gint tasks_datas_len (APP_data *data)
{
  return g_list_length (data->tasksList);
}

/***************************************
  PUBLIC function 
  get datas in Tasks at position
****************************************/
void tasks_get_ressource_datas (gint position, tasks_data *tmp, APP_data *data)
{
  GList *l;
  tasks_data *tmp_tasks_datas;
  gint tasks_list_len = g_list_length (data->tasksList);

  /* we get pointer on the element */
  if((position < tasks_list_len) && (position >=0)) {
     l = g_list_nth (data->tasksList, position);
     tmp_tasks_datas = (tasks_data *)l->data;    
     /* get datas strings */
     tmp->name = tmp_tasks_datas->name;
     if(tmp->type == TASKS_TYPE_MILESTONE) 
        tmp->location = tmp_tasks_datas->location; 
     else
        tmp->location = NULL; 
     /* other datas */
     tmp->id = tmp_tasks_datas->id;
     tmp->type = tmp_tasks_datas->type;
     tmp->timer_val = tmp_tasks_datas->timer_val;
     tmp->status = tmp_tasks_datas->status;
     tmp->priority = tmp_tasks_datas->priority;
     tmp->category = tmp_tasks_datas->category;
     tmp->group = tmp_tasks_datas->group;
     tmp->color = tmp_tasks_datas->color;
     tmp->progress = tmp_tasks_datas->progress;
     tmp->start_nthDay = tmp_tasks_datas->start_nthDay;
     tmp->start_nthMonth = tmp_tasks_datas->start_nthMonth;
     tmp->start_nthYear = tmp_tasks_datas->start_nthYear;
     tmp->end_nthDay = tmp_tasks_datas->end_nthDay;
     tmp->end_nthMonth = tmp_tasks_datas->end_nthMonth;
     tmp->end_nthYear = tmp_tasks_datas->end_nthYear;

     tmp->lim_nthDay = tmp_tasks_datas->lim_nthDay;
     tmp->lim_nthMonth = tmp_tasks_datas->lim_nthMonth;
     tmp->lim_nthYear = tmp_tasks_datas->lim_nthYear;

     tmp->cadre = tmp_tasks_datas->cadre;
     tmp->cRscBox = tmp_tasks_datas->cRscBox;
     tmp->cTaskBox = tmp_tasks_datas->cTaskBox;
     tmp->cName = tmp_tasks_datas->cName;
     tmp->cPeriod = tmp_tasks_datas->cPeriod;
     tmp->cProgress = tmp_tasks_datas->cProgress;
     tmp->cImgPty = tmp_tasks_datas->cImgPty;
     tmp->cImgStatus = tmp_tasks_datas->cImgStatus;
     tmp->cImgCateg = tmp_tasks_datas->cImgCateg;
     tmp->groupItems = tmp_tasks_datas->groupItems;
     tmp->cDashboard = tmp_tasks_datas->cDashboard;
     tmp->durationMode = tmp_tasks_datas->durationMode;
     tmp->days = tmp_tasks_datas->days;
     tmp->hours = tmp_tasks_datas->hours;
     tmp->minutes = tmp_tasks_datas->minutes;
     tmp->calendar = tmp_tasks_datas->calendar;
     tmp->delay = tmp_tasks_datas->delay;
  }/* endif */
}

/**************************************
  PUBLIC function - add current datas
  in "temp tasks_data" utlity variable
**************************************/
void tasks_store_to_tmp_data (gint position, gchar *name, gint startDay, gint startMonth, gint startYear, 
                            gint endDay, gint endMonth, gint endYear,
                            gint limDay, gint limMonth, gint limYear,
                            gdouble progress, gint pty, gint categ, gint status, 
                            GdkRGBA  color,  APP_data *data, gint id, gboolean incrCounter, gint durMode, gint days, 
                            gint hours, gint minutes, gint calendar, gint delay, tasks_data *tmp)
{

  /* update internal object's counter */
  if(incrCounter) {
      data->objectsCounter++;
      tmp->id = data->objectsCounter; 
  }
  else {
    // printf("* add task object without increasing internal counter id =%d *\n", id);
     tmp->id = id;
  }
// printf("date début j=%d m=%d a=%d / fin j=%d m=%d a=%d \n", startDay, startMonth, startYear, endDay, endMonth, endYear);
// printf("compteur objets dans Tasks=%d\n", data->objectsCounter);
  /* store values in 'temp' variable */
  tmp->name = g_strdup_printf ("%s", name);
  tmp->start_nthDay = startDay;
  tmp->start_nthMonth = startMonth;
  tmp->start_nthYear = startYear;
  tmp->end_nthDay = endDay;
  tmp->end_nthMonth = endMonth;
  tmp->end_nthYear = endYear;

  tmp->lim_nthDay = limDay;
  tmp->lim_nthMonth = limMonth;
  tmp->lim_nthYear = limYear;

  tmp->progress = progress;
  tmp->priority = pty;
  tmp->category = categ;
  tmp->status = status;
  tmp->color = color;
  tmp->durationMode = durMode;
  tmp->days = days;
  tmp->hours = hours;
  tmp->minutes = minutes;
  tmp->calendar = calendar;
  tmp->delay = delay;
}

/**************************************
  PUBLIC function
  add a task datas at a given position
***************************************/
void tasks_insert (gint position, APP_data *data, tasks_data *tmp)
{
  gint rsc_list_len = g_list_length (data->tasksList);

  if(position>=0) {
    /* we add datas to the GList */
    tasks_data *tmp_tasks_datas;
    tmp_tasks_datas = g_malloc (sizeof(tasks_data));
    /* we transfer datas */
    tmp_tasks_datas->id = tmp->id;
    tmp_tasks_datas->type = tmp->type;
    if(tmp->name==NULL)
       tmp_tasks_datas->name = NULL;
    else
       tmp_tasks_datas->name = g_strdup_printf ("%s", tmp->name);
    if(tmp->location==NULL)
        tmp_tasks_datas->location = NULL;
    else
        tmp_tasks_datas->location = g_strdup_printf ("%s", tmp->location);

    tmp_tasks_datas->timer_val = tmp->timer_val;
    tmp_tasks_datas->status = tmp->status;
    tmp_tasks_datas->priority = tmp->priority;
    tmp_tasks_datas->category = tmp->category;
    tmp_tasks_datas->group = tmp->group;
    tmp_tasks_datas->color = tmp->color;
    tmp_tasks_datas->progress = tmp->progress;
    tmp_tasks_datas->start_nthDay = tmp->start_nthDay;
    tmp_tasks_datas->start_nthMonth = tmp->start_nthMonth;
    tmp_tasks_datas->start_nthYear = tmp->start_nthYear;
    tmp_tasks_datas->end_nthDay = tmp->end_nthDay;
    tmp_tasks_datas->end_nthMonth = tmp->end_nthMonth;
    tmp_tasks_datas->end_nthYear = tmp->end_nthYear;

    tmp_tasks_datas->lim_nthDay = tmp->lim_nthDay;
    tmp_tasks_datas->lim_nthMonth = tmp->lim_nthMonth;
    tmp_tasks_datas->lim_nthYear = tmp->lim_nthYear;

    tmp_tasks_datas->cadre = tmp->cadre;
    tmp_tasks_datas->cRscBox = tmp->cRscBox;
    tmp_tasks_datas->cTaskBox = tmp->cTaskBox;
    tmp_tasks_datas->cName = tmp->cName;
    tmp_tasks_datas->cGroup = tmp->cGroup;
    tmp_tasks_datas->cPeriod = tmp->cPeriod;
    tmp_tasks_datas->cProgress = tmp->cProgress;
    tmp_tasks_datas->cImgPty = tmp->cImgPty;
    tmp_tasks_datas->cImgStatus = tmp->cImgStatus;
    tmp_tasks_datas->cImgCateg = tmp->cImgCateg;
    tmp_tasks_datas->cRun = tmp->cRun;
    tmp_tasks_datas->groupItems = NULL; /* before any display */
    tmp_tasks_datas->cDashboard = NULL; /* idem */
    tmp_tasks_datas->durationMode = tmp->durationMode;
    tmp_tasks_datas->days = tmp->days;
    tmp_tasks_datas->hours = tmp->hours;
    tmp_tasks_datas->minutes = tmp->minutes;
    tmp_tasks_datas->calendar = tmp->calendar;
    tmp_tasks_datas->delay = tmp->delay;

    data->tasksList = g_list_insert (data->tasksList, tmp_tasks_datas, position);
  }
  else
     printf("* PlanLibre critical : can't insert Tasks datas at a negative position ! *\n");
}

/********************************************
  PUBLIC function  modify a tasks datas 
  at a given position
********************************************/
void tasks_modify_datas (gint position, APP_data *data, tasks_data *tmp)
{
  gint tsk_list_len = g_list_length (data->tasksList);
  GList *l;

  if((position >=0) && (position<=tsk_list_len)) {
     l = g_list_nth (data->tasksList, position);
     tasks_data *tmp_tasks_datas;
     tmp_tasks_datas = (tasks_data *)l->data;
     /* now we change values 'in place' */
     if(tmp_tasks_datas->name!=NULL) {
         g_free (tmp_tasks_datas->name);
         tmp_tasks_datas->name=NULL;
     }
     if(tmp_tasks_datas->location!=NULL) {
         g_free (tmp_tasks_datas->location);
         tmp_tasks_datas->location=NULL;
     }

    /* we transfer datas */
    tmp_tasks_datas->name = g_strdup_printf ("%s", tmp->name);
    tmp_tasks_datas->location = g_strdup_printf ("%s", tmp->location);
    tmp_tasks_datas->status = tmp->status;
    tmp_tasks_datas->priority = tmp->priority;
    tmp_tasks_datas->category = tmp->category;
    tmp_tasks_datas->group = tmp->group;
    tmp_tasks_datas->color = tmp->color;
    tmp_tasks_datas->progress = tmp->progress;
    tmp_tasks_datas->start_nthDay = tmp->start_nthDay;
    tmp_tasks_datas->start_nthMonth = tmp->start_nthMonth;
    tmp_tasks_datas->start_nthYear = tmp->start_nthYear;
    tmp_tasks_datas->end_nthDay = tmp->end_nthDay;
    tmp_tasks_datas->end_nthMonth = tmp->end_nthMonth;
    tmp_tasks_datas->end_nthYear = tmp->end_nthYear;

    tmp_tasks_datas->lim_nthDay = tmp->lim_nthDay;
    tmp_tasks_datas->lim_nthMonth = tmp->lim_nthMonth;
    tmp_tasks_datas->lim_nthYear = tmp->lim_nthYear;

    tmp_tasks_datas->durationMode = tmp->durationMode;
    tmp_tasks_datas->days = tmp->days;
    tmp_tasks_datas->hours = tmp->hours;
    tmp_tasks_datas->minutes = tmp->minutes;
    tmp_tasks_datas->calendar = tmp->calendar;
    tmp_tasks_datas->delay = tmp->delay;
  }
}

/********************************************
  PUBLIC function  modify a tasks dates datas 
  at a given position
********************************************/
void tasks_modify_date_datas (gint position, GDate *date_start, GDate *date_end, GDate *date_lim, APP_data *data)
{
  gint tsk_list_len = g_list_length (data->tasksList);
  GList *l;

  if((position >=0) && (position<=tsk_list_len)) {
     l = g_list_nth (data->tasksList, position);
     tasks_data *tmp_tasks_datas;
     tmp_tasks_datas = (tasks_data *)l->data;
     tmp_tasks_datas->start_nthDay = g_date_get_day (date_start);
     tmp_tasks_datas->start_nthMonth = g_date_get_month (date_start);
     tmp_tasks_datas->start_nthYear = g_date_get_year (date_start);

     tmp_tasks_datas->end_nthDay = g_date_get_day (date_end);
     tmp_tasks_datas->end_nthMonth = g_date_get_month (date_end);
     tmp_tasks_datas->end_nthYear = g_date_get_year (date_end);
     
     if(date_lim) {
       tmp_tasks_datas->lim_nthDay = g_date_get_day (date_lim);
       tmp_tasks_datas->lim_nthMonth = g_date_get_month (date_lim);
       tmp_tasks_datas->lim_nthYear = g_date_get_year (date_lim);
     }
   //  tmp_tasks_datas->durationMode = tmp->durationMode;
  }
}
/*************************************
  PUBLIC : search for nearest group
  known a position
*************************************/
gint tasks_search_group_backward (gint position, APP_data *data)
{
  gint ret = -1, i;
  GList *l;
  tasks_data *tmp;

  if(position>g_list_length (data->tasksList))
         return -1;
  if(position>0) {
    l = g_list_nth (data->tasksList, position);
    tmp = (tasks_data *)l->data;
    if(tmp->type==TASKS_TYPE_TASK)
       ret = tmp->group;
    if(tmp->type==TASKS_TYPE_GROUP) {
       if(position>1) {/* attempt to reach a 'TRUE' task */
          l = g_list_nth (data->tasksList, position-1);
          tmp = (tasks_data *)l->data;
          if(tmp->type==TASKS_TYPE_TASK)
                ret = tmp->group;
       }
    }
  }
  return ret;
}

/*************************************
  PUBLIC : search for nearest group
  known a position
*************************************/
gint tasks_search_group_forward (gint position, APP_data *data)
{
  gint ret = -1, i;
  GList *l;
  tasks_data *tmp;

  if(position>g_list_length (data->tasksList))
         return -1;
  if(position<g_list_length (data->tasksList)) {
    l = g_list_nth (data->tasksList, position);
    tmp = (tasks_data *)l->data;
    if(tmp->type==TASKS_TYPE_TASK)
       ret = tmp->group;
    if(tmp->type==TASKS_TYPE_GROUP) {
       if(position<g_list_length (data->tasksList)-1) {/* attempt to reach a 'TRUE' task */
          l = g_list_nth (data->tasksList, position+1);
          tmp = (tasks_data *)l->data;
          if(tmp->type==TASKS_TYPE_TASK)
                ret = tmp->group;
       }
    }
  }
  return ret;
}


/*********************************
 PUBLIC : function to get state
 of Tomota Timer
*********************************/
gboolean tasks_get_tomato_timer_status (APP_data *data)
{
   return fTomatoTimer;
}

/*********************************
 PUBLIC : function to set state
 of Tomota Timer flag
*********************************/
void tasks_set_tomato_timer_status (gboolean status, APP_data *data)
{
   fTomatoTimer = status;
}

/*********************************
 PUBLIC : function to reset 
 of Tomota Timer value
*********************************/
void tasks_reset_tomato_timer_value (APP_data *data)
{
   timer_val = 0;
}

/*********************************
 PUBLIC : function to update 
 of Tomota Timer value
*********************************/
void tasks_update_tomato_timer_value (gint secs, APP_data *data)
{
   timer_val = timer_val+secs;
}

/*********************************
 PUBLIC : function to get 
 of Tomota Timer value
*********************************/
gint64 tasks_update_get_timer_value (APP_data *data)
{
   return timer_val;
}

/*********************************
 PUBLIC : function to store 
 of Tomota Timer task ID
*********************************/
void tasks_store_tomato_timer_ID (gint id, APP_data *data)
{
   timer_task_id = id;
}

/*********************************
 PUBLIC : function to get 
 of Tomota Timer task ID
*********************************/
gint tasks_get_tomato_timer_ID (APP_data *data)
{
   return timer_task_id;
}

/*********************************
 PUBLIC : given a task ID 
 update its timer_value field
*********************************/
void tasks_update_tomato_timer_value_for_ID (gint id, APP_data *data)
{
  gint i;
  GList *l;
  tasks_data *tmp;
  gchar *job_run;

  for(i=0; i<g_list_length (data->tasksList);i++) {
     l = g_list_nth (data->tasksList, i);
     tmp = (tasks_data *)l->data;
     if(tmp->id == id) {
        tmp->timer_val = tmp->timer_val+timer_val;
        /* we modify widget */
        if(tmp->timer_val>0)
            job_run = g_strdup_printf ("<i>Time spent %.2d:%.2d</i>", (gint)tmp->timer_val/60, (gint)tmp->timer_val % 60);
        else
            job_run = g_strdup_printf ("<i>Time spent %s</i>", "--");
        gtk_label_set_markup (GTK_LABEL(tmp->cRun), job_run); 
        g_free (job_run);
        misc_display_app_status (TRUE, data);
     }
  }/* next i */
}

/*********************************
 PUBLIC : function to reset 
 of Tomota Timer ellapsed value
*********************************/
void tasks_reset_tomato_ellapsed_value (APP_data *data)
{
   ellapsed = 0;
   timer_val = 0;
   fTomatoUpdateDisplay = FALSE;
}

/*********************************
 PUBLIC : function to increase 
 of Tomota Timer ellapsed value
*********************************/
void tasks_tomato_update_ellapsed_value (gint secs, APP_data *data)
{
   ellapsed = ellapsed+secs;
   timer_val = ellapsed;
}

/*********************************
 PUBLIC : function to get 
 Tomota Timer ellapsed value
*********************************/
gint tasks_tomato_get_ellapsed_value (APP_data *data)
{
   return ellapsed;
}

/*********************************
 PUBLIC : function to get 
 Tomota Timer update status
*********************************/
gboolean tasks_tomato_get_display_status (APP_data *data)
{
   return fTomatoUpdateDisplay;
}

/*********************************
 PUBLIC : function to reset 
 Tomota Timer update status
*********************************/
void tasks_tomato_reset_display_status (APP_data *data)
{
   fTomatoUpdateDisplay = FALSE;
}

/*********************************
 PUBLIC : function to reset 
 of Tomota Timer ellapsed value
*********************************/
void tasks_reset_tomato_paused_value (APP_data *data)
{
   paused = 0;
   fTomatoUpdateDisplay = TRUE;
}

/*********************************
 PUBLIC : function to increase 
 of Tomota Timer ellapsed value
*********************************/
void tasks_tomato_update_paused_value (gint secs, APP_data *data)
{
   paused = paused+secs;
}

/*********************************
 PUBLIC : function to get 
 of Tomota Timer ellapsed value
*********************************/
gint tasks_tomato_get_paused_value (APP_data *data)
{
   return paused;
}

/*****************************************
 PUBLIC : autoscroll a display
 arguments : pointer on a scrolled window
  value from 0.0 to 1.0 according to
  page's position
******************************************/
void tasks_autoscroll (gdouble value, APP_data *data)
{
  GtkWidget *scrolledwindow3;

  scrolledwindow3 = GTK_WIDGET(gtk_builder_get_object (data->builder, "scrolledwindow3"));
  GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(scrolledwindow3));
  gtk_adjustment_set_value (GTK_ADJUSTMENT (adj), gtk_adjustment_get_upper (GTK_ADJUSTMENT(adj))*(value));
}

/***************************************************
  PUBLIC : returns valid insertion point
 Arguments ; - position = requested insertion point
   - group = ID of task
*********************************************/
gint tasks_compute_valid_insertion_point (gint pos, gint group, gboolean isGroup, APP_data *data)
{
  gint i, ret = -1;
  gint tsk_list_len = g_list_length (data->tasksList);
  tasks_data *tmp;
  GList *l;

  if(tsk_list_len == 0 || pos<0)
     return 0; /* empty task list, so insertion point = 0 */
  if(pos==0 && group ==-1)
     return 0;
  if(pos>=tsk_list_len)
    return tsk_list_len;

  /* check we are on group line */
  l = g_list_nth (data->tasksList, pos);
  tmp = (tasks_data *)l->data;
  if((tmp->id == group) && (tmp->type == TASKS_TYPE_GROUP))
           return pos+1;

  /* check if current position is group free */
  if(tmp->group == -1)
     return pos;
  /* seek backward */
  if(pos>0) {
     i = pos-1;
     while((i>=0) && (ret<0)) {
        l = g_list_nth (data->tasksList, i);
        tmp = (tasks_data *)l->data;
        if((tmp->group == group) && (tmp->type == TASKS_TYPE_TASK))
           ret = i;
        if((tmp->id == group) && (tmp->type == TASKS_TYPE_GROUP))
           ret = i;
        if((isGroup) && (tmp->type == TASKS_TYPE_GROUP))
           ret = i;
        i--;
     }/* wend */
  }/* endif pos */
  if(ret>=0) {
    if(isGroup) /* it's a group */
       return ret;
    else
       return ret+1;
  }
  /* not found, seek forward */
  if(pos+1>tsk_list_len)
     return tsk_list_len;

  ret = -1;
  i = pos+1;
  while((i<tsk_list_len) && (ret<0)) {
     l = g_list_nth (data->tasksList, i);
     tmp = (tasks_data *)l->data;
     if((tmp->group == group) && (tmp->type == TASKS_TYPE_TASK))
           ret = i;
     if((tmp->id == group) && (tmp->type == TASKS_TYPE_GROUP))
           ret = i+1;
     if((isGroup) && (tmp->type == TASKS_TYPE_GROUP))
           ret = i+1;
     i++;
  }/* wend */  
  if(ret>=0)
     return ret;
  return tsk_list_len;
}

/******************************
  LOCAL get the earliest date
  between all tasks with group 
  ID
******************************/
static GDate get_earliest_date (gint id, APP_data *data)
{
  GDate date, date_start;
  tasks_data *tmp_tasks_datas;
  gint i;
  GList *l;

  g_date_set_dmy (&date, 1, 1, 2100);
  if(g_list_length (data->tasksList)==0) {
      g_date_set_dmy (&date, data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
      return date;
  }
  /* now, we read all startdates */
  for(i=0; i<g_list_length (data->tasksList); i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     if(tmp_tasks_datas->type==TASKS_TYPE_TASK && tmp_tasks_datas->group==id) {        
          g_date_set_dmy (&date_start, 
                         tmp_tasks_datas->start_nthDay, tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
          if(g_date_valid (&date_start)) {
             gint cmp_date = g_date_compare (&date, &date_start);
             if(cmp_date>0) {
                date = date_start;
             }
          }/* date valid */
     }
  }/* next */

  return date;
}

/**********************************
  LOCAL get the latest date
  between all tasks with group ID
***********************************/
static GDate get_latest_date (gint id, APP_data *data)
{
  GDate date, date_end;
  tasks_data *tmp_tasks_datas;
  gint i;
  GList *l;

  g_date_set_dmy (&date, 1, 1, 1970);
  if(g_list_length (data->tasksList)==0) {
      g_date_set_dmy (&date, data->properties.end_nthDay, data->properties.end_nthMonth, data->properties.end_nthYear);
      return date;
  }
  /* now, we read all startdates */
  for(i=0; i<g_list_length (data->tasksList); i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     if(tmp_tasks_datas->type==TASKS_TYPE_TASK  && tmp_tasks_datas->group==id) {        
          g_date_set_dmy (&date_end, 
                         tmp_tasks_datas->end_nthDay, tmp_tasks_datas->end_nthMonth, tmp_tasks_datas->end_nthYear);
          if(g_date_valid (&date_end )) {
             gint cmp_date = g_date_compare (&date, &date_end);
             if(cmp_date<0) {
                date = date_end;
             }
          }/* date valid */
     }
  }/* next */

  return date;
}

/********************************************
  PUBLIC : compute group's limits when
  user changed task group id
********************************************/
void tasks_update_group_limits (gint task_id, gint id_prev_group, gint id_new_group, APP_data *data)
{
   tasks_data *tmpTsk, *tmpGrp;
   GList *l, *l1;
   GDate grp_start, grp_end;
   gint group = -1;

   if(id_prev_group==-1 && id_new_group==-1)
      return;
   if(id_prev_group==-1) {
       if(id_new_group>=0) {
         l1 = g_list_nth (data->tasksList, tasks_get_rank_for_id (id_new_group, data));        
         tmpGrp = (tasks_data *) l1->data;
         g_date_set_dmy (&grp_start, tmpGrp->start_nthDay, tmpGrp->start_nthMonth, tmpGrp->start_nthYear);
         g_date_set_dmy (&grp_end, tmpGrp->end_nthDay, tmpGrp->end_nthMonth, tmpGrp->end_nthYear);
         grp_start = get_earliest_date (id_new_group, data);
         grp_end = get_latest_date (id_new_group, data);
         group = id_new_group;
       }
   }
   else {
      if(id_new_group == -1) {
         l1 = g_list_nth (data->tasksList, tasks_get_rank_for_id (id_prev_group, data));        
         tmpGrp = (tasks_data *) l1->data;
         grp_start = get_earliest_date (id_prev_group, data);
         grp_end = get_latest_date (id_prev_group, data);
         group = id_prev_group;

      }
      else {
         if(id_new_group == id_prev_group) {
           l1 = g_list_nth (data->tasksList, tasks_get_rank_for_id (id_prev_group, data));        
           tmpGrp = (tasks_data *) l1->data;
           grp_start = get_earliest_date (id_prev_group, data);
           grp_end = get_latest_date (id_prev_group, data);
           group = id_new_group;
         }
         else {
           /* we update previous group */
           l1 = g_list_nth (data->tasksList, tasks_get_rank_for_id (id_prev_group, data));        
           tmpGrp = (tasks_data *) l1->data;
           grp_start = get_earliest_date (id_prev_group, data);
           grp_end = get_latest_date (id_prev_group, data);
           tmpGrp->start_nthDay = g_date_get_day (&grp_start);
           tmpGrp->start_nthMonth = g_date_get_month (&grp_start);
           tmpGrp->start_nthYear = g_date_get_year (&grp_start);
           tmpGrp->end_nthDay = g_date_get_day (&grp_end);
           tmpGrp->end_nthMonth = g_date_get_month (&grp_end);
           tmpGrp->end_nthYear = g_date_get_year (&grp_end);
           tmpGrp->days = g_date_days_between (&grp_start, &grp_end)+1;
           tasks_group_modify_widget (tasks_get_rank_for_id (id_prev_group, data), data);
           /* and same for new group */
           l1 = g_list_nth (data->tasksList, tasks_get_rank_for_id (id_new_group, data));        
           tmpGrp = (tasks_data *) l1->data;
           grp_start = get_earliest_date (id_new_group, data);
           grp_end = get_latest_date (id_new_group, data);
           group = id_new_group;
         }
      }      
   }/* else if prev group != -1 */

   /* common */
   tmpGrp->start_nthDay = g_date_get_day (&grp_start);
   tmpGrp->start_nthMonth = g_date_get_month (&grp_start);
   tmpGrp->start_nthYear = g_date_get_year (&grp_start);
   tmpGrp->end_nthDay = g_date_get_day (&grp_end);
   tmpGrp->end_nthMonth = g_date_get_month (&grp_end);
   tmpGrp->end_nthYear = g_date_get_year (&grp_end);
   tmpGrp->days = g_date_days_between (&grp_start, &grp_end)+1;

   /* update Group in task's display */
   if(group>=0) {
       tasks_group_modify_widget (tasks_get_rank_for_id (group, data), data);
   }
}

/****************************************
  PROTECTED - heneral info messages
****************************************/
static void info_updated_end_date (GtkWidget *win, APP_data *data)
{
   if(!fErrEnd) {
     gchar *msg = g_strdup_printf (_("In order to set-up linked tasks,\nPlanLibre changed project ending date.\nYou can check new ending date under <b>project/properties</b> menu."));
     misc_InfoDialog (win, msg);
     g_free (msg);
     fErrEnd = TRUE;
   }
}

static void info_updated_fixed_date (GtkWidget *win, APP_data *data)
{
   if(!fErrFixed) {
      gchar *msg = g_strdup_printf (_("It isn't possible to shift a <b>fixed task</b> !\nNow, you have requested to link it to a task ending <b>too late</b>.\nYou should modify one or more ending date."));
      misc_InfoDialog (win, msg);
      g_free (msg);
      fErrFixed = TRUE;
   }
}

static void info_updated_deadline_date (GtkWidget *win, APP_data *data)
{
   if(!fErrDeadline) {
      gchar *msg = g_strdup_printf (_("It isn't possible to shift this <b>task with deadline</b> !\nNow, you have requested to link it to a task ending <b>too late</b>.\nYou should modify deadline date."));
      misc_InfoDialog (win, msg);
      g_free (msg);
      fErrDeadline = TRUE;
   }
}

/***********************************************************************
  PUBLIC - set final dates when user set-up new task with predecessor(s)
  according to "due date" mode, like "as soon as" and so on
  used ONLY for a task with 1 or more PREDECESSORS tasks
  arguments :
  - ID : task unique identifier
  - current : task starting date before computations
  - curDays : task duration
  - insertion_row : row where the task will be inserted, AFTER groups 
                   computations
  - cancel_rank : number of undo operations from current point, in order
         to automatize undo operations when more than task is involved
- fUpdate : if TRUE, undo engine is used, otherwise undo engine if called
        elsewhere
  - fIsSource : in case id corresponds to source of computation, i.e.
               origin task
**********************************************************************/
gint tasks_compute_dates_for_linked_new_task (gint id, gint curDuration, gint curDays, gint insertion_row, gint cancel_rank,
                                          GDate *current, gboolean fUpdate, gboolean fIsSource, GtkWidget *dlg, APP_data *data)
{
  GDate tmp_date, date_current, date_interval2, interval3, end_date, lim_date;
  gint cmp, cmp2, ret = TASK_PLACEMENT_OK;
  tasks_data temp, *undoTasks;
  undo_datas *tmp_value;

  tasks_get_ressource_datas (insertion_row, &temp, data);
  /* undo engine - part 1 */
  if(fUpdate) {
     undoTasks = g_malloc (sizeof(tasks_data));
     tasks_get_ressource_datas (insertion_row, undoTasks, data);
     /* build strings in order to keep "true" values */
     undoTasks->name = NULL;
     undoTasks->location = NULL;
     if(temp.name)
        undoTasks->name = g_strdup_printf ("%s", temp.name);
     if(temp.location)
        undoTasks->location = g_strdup_printf ("%s", temp.location);
     tmp_value = g_malloc (sizeof(undo_datas));
     tmp_value->insertion_row = insertion_row;
     tmp_value->new_row = insertion_row;
     tmp_value->id = temp.id;
     tmp_value->groups = NULL;
     tmp_value->nbTasks = cancel_rank;
  }

  /* default */
  g_date_set_dmy (&date_interval2, data->properties.end_nthDay, data->properties.end_nthMonth, data->properties.end_nthYear);
  if(temp.lim_nthDay>0 && temp.lim_nthMonth>0 && temp.lim_nthYear>0) {
     g_date_set_dmy (&interval3, temp.lim_nthDay, temp.lim_nthMonth, temp.lim_nthYear);
  }
  g_date_set_dmy (&end_date, data->properties.end_nthDay, data->properties.end_nthMonth, data->properties.end_nthYear);
  g_date_set_dmy (&date_current, g_date_get_day (current), g_date_get_month (current), g_date_get_year (current));
  tmp_date = links_get_latest_task_linked (id, data);
  printf ("pour id =%d nom =%s date la plus lointaine en avant %d %d %d current date =%d %d %d curDays =%d\n", id, temp.name, g_date_get_day (&tmp_date), g_date_get_month (&tmp_date), g_date_get_year (&tmp_date), g_date_get_day (&date_current), g_date_get_month (&date_current), g_date_get_year (&date_current), curDays);

  if(curDuration == TASKS_AS_SOON) {
     g_date_set_dmy (&date_interval2, temp.end_nthDay, temp.end_nthMonth, temp.end_nthYear);
     cmp = g_date_compare (&date_current, &tmp_date);
     if(cmp<=0) {printf ("cas OK \n");
         date_current = tmp_date; /* change starting date */
         if(!fIsSource) {// TODO paramètre à valider avec tests
            g_date_add_days (&date_current, 1);/* jump to day+1*/
         }
         date_interval2 = date_current;/* change ending date */
         g_date_add_days (&date_interval2, curDays-1);
         interval3 = date_current;
     }/* endif cmp */
     /* we check if new ending date is compatible with projec's ending date */
     if(temp.lim_nthDay>0 && temp.lim_nthMonth>0 && temp.lim_nthYear>0) {
        tasks_modify_date_datas (insertion_row, &date_current, &date_interval2, &interval3, data);
     }
     else {
        tasks_modify_date_datas (insertion_row, &date_current, &date_interval2, NULL, data);
     }
     if(temp.type == TASKS_TYPE_TASK) {
        tasks_modify_widget (insertion_row, data);
     }
     if(temp.type == TASKS_TYPE_MILESTONE) {
        tasks_milestone_modify_widget (insertion_row, data);
     }
     cmp = g_date_compare (&end_date, &date_interval2);
     if(cmp<0) {
        data->properties.end_nthDay = g_date_get_day (&date_interval2);
        data->properties.end_nthMonth = g_date_get_month (&date_interval2);
        data->properties.end_nthYear = g_date_get_year (&date_interval2);
        info_updated_end_date (dlg, data);
        ret = TASK_PLACEMENT_UPDATED_END;
     }/* endif cmp */
     if(fUpdate) {
	tmp_value->datas = undoTasks;
        /* duplicate links */
        tmp_value->list = links_copy_all (data);
        tmp_value->opCode = OP_MODIFY_TASK;
        if(temp.type == TASKS_TYPE_MILESTONE) {
           tmp_value->opCode = OP_MODIFY_MILESTONE;
        }
        undo_push (CURRENT_STACK_TASKS, tmp_value, data);
     }
  }/* endif as soon */

  if(curDuration == TASKS_DEADLINE) {
     g_date_set_dmy (&lim_date, temp.lim_nthDay, temp.lim_nthMonth, temp.lim_nthYear);
     /* is current task starting date is after tmp_date ? Then, nothing to do ! */
     cmp = g_date_compare (&tmp_date, &date_current);
printf ("cmp dedaline date current avant %d \n", cmp);
     if(cmp>=0) {
        date_current = tmp_date; /* change starting date */
        if(!fIsSource)
           g_date_add_days (&date_current, 1);/* jump to day+1*/
        date_interval2 = date_current;/* change ending date */
        g_date_add_days (&date_interval2, curDays-1);
        /* is new ending date is compliant with deadline - limit date ? */
        cmp = g_date_compare (&date_interval2, &lim_date);
        /* if date_current > tmp date nothing to do */
        if(cmp<0) {/* OK, just set new values */
          tasks_modify_date_datas (insertion_row, &date_current, &date_interval2, &lim_date, data);
          if(temp.type == TASKS_TYPE_TASK) {
             tasks_modify_widget (insertion_row, data);
          }
          if(temp.type == TASKS_TYPE_MILESTONE) {
		tasks_milestone_modify_widget (insertion_row, data);
	  }
	  if(fUpdate) {
		tmp_value->datas = undoTasks;
		/* duplicate links */
		tmp_value->list = links_copy_all (data);
		tmp_value->opCode = OP_MODIFY_TASK;
                if(temp.type == TASKS_TYPE_MILESTONE) {
                   tmp_value->opCode = OP_MODIFY_MILESTONE;
                }
		undo_push (CURRENT_STACK_TASKS, tmp_value, data);
	  }
        }
        else {/* we can't do that, so we display a message */
           links_remove_tasks_ending_too_late (id, &tmp_date, data);
           info_updated_deadline_date (dlg, data);
           ret = TASK_PLACEMENT_ERR_DEADLINE;
           /* free datas for undo engine */
           if(fUpdate) {
              if(undoTasks->name)
                g_free (undoTasks->name);
              if(undoTasks->location)
                g_free (undoTasks->location);
              g_free (undoTasks);
              g_free (tmp_value);
           }
        }
     }/* endif cmp */
  }/* ending dealine */

  if(curDuration == TASKS_AFTER) {
     /* default */
     g_date_set_dmy (&date_interval2, temp.end_nthDay, temp.end_nthMonth, temp.end_nthYear);
     cmp = g_date_compare (&date_current, &tmp_date);
     /* if tmp_date is after date_current it's OK, since we are in task_after / note before mode so we let's go updating */
     if(cmp<=0) {
         date_current = tmp_date; /* change starting date */
         if(!fIsSource)
            g_date_add_days (&date_current, 1);/* jump to day+1*/
         date_interval2 = date_current;/* change ending date */
         g_date_add_days (&date_interval2, curDays-1);
     }/* endif cmp */
     /* we don't have to change lim date because we are in "after" mode */
     g_date_set_dmy (&interval3, temp.lim_nthDay, temp.lim_nthMonth, temp.lim_nthYear);
     /* we check if new ending date is compatible with projec's ending date */
     if(temp.lim_nthDay>0 && temp.lim_nthMonth>0 && temp.lim_nthYear>0) {
        tasks_modify_date_datas (insertion_row, &date_current, &date_interval2, &interval3, data);
     }
     else {
        tasks_modify_date_datas (insertion_row, &date_current, &date_interval2, NULL, data);
     }
     if(temp.type == TASKS_TYPE_TASK) {
        tasks_modify_widget (insertion_row, data);
     }
     if(temp.type == TASKS_TYPE_MILESTONE) {
        tasks_milestone_modify_widget (insertion_row, data);
     }
     cmp = g_date_compare (&end_date, &date_interval2);
     if(cmp<0) {
           data->properties.end_nthDay = g_date_get_day (&date_interval2);
           data->properties.end_nthMonth = g_date_get_month (&date_interval2);
           data->properties.end_nthYear = g_date_get_year (&date_interval2);
           info_updated_end_date (dlg, data);
           ret = TASK_PLACEMENT_UPDATED_END;
     }/* endif cmp */
     if(fUpdate) {
	tmp_value->datas = undoTasks;
	/* duplicate links */
	tmp_value->list = links_copy_all (data);
	tmp_value->opCode = OP_MODIFY_TASK;
        if(temp.type == TASKS_TYPE_MILESTONE) {
           tmp_value->opCode = OP_MODIFY_MILESTONE;
        }
	undo_push (CURRENT_STACK_TASKS, tmp_value, data);
     }
  }/* ending tasks_after */

  if(curDuration == TASKS_FIXED) {
     /* here we can't move the task - so we check if last linked predecessor ending date is strictly BEFORE task start */
     cmp = g_date_compare (&date_current, &tmp_date);
     if(cmp>0) { /* OK - nothing to do */
         if(fUpdate) {
	   tmp_value->datas = undoTasks;
	   /* duplicate links */
	   tmp_value->list = links_copy_all (data);
	   tmp_value->opCode = OP_MODIFY_TASK;
           if(temp.type == TASKS_TYPE_MILESTONE) {
              tmp_value->opCode = OP_MODIFY_MILESTONE;
           }
	   undo_push (CURRENT_STACK_TASKS, tmp_value, data);
	}
     }
     else {/* we have to remove useless links */
        links_remove_tasks_ending_too_late (id, &date_current, data);
        info_updated_fixed_date (dlg, data);
        ret = TASK_PLACEMENT_ERR_FIXED;
           /* release datas for undo engine */
           if(fUpdate) {
              if(undoTasks->name)
                g_free (undoTasks->name);
              if(undoTasks->location)
                g_free (undoTasks->location);
              g_free (undoTasks);
              g_free (tmp_value);
           }
     }
  }/* ending fixed */

  /* update status for dashboard */
  if(temp.type == TASKS_TYPE_TASK) {
     dashboard_remove_task (insertion_row, data);
     dashboard_set_status (id, data);/* always AFTER dash_remove */
     dashboard_add_task (insertion_row, data);
  }
  return ret;
} 

/***********************************************************************
  PUBLIC : TODO function to compute optimal path for linked tasks when
  user modifies a previous task with predecessor(s)
  greedy algorithm.
  Retuens how many tasks have been updated
***********************************************************************/
gint tasks_compute_dates_for_all_linked_task (gint id, gint curDuration, gint curDays, gint insertion_row, 
                                          GDate *current, gint predecessors, gint sucessors, gboolean fUpdate, 
                                          GtkWidget *dlg, APP_data *data)
{
  gint i, j, rank, nbTsks, *vector, *nodes, *array, total, receiver, rc, *sender, total_done = 1;
  tasks_data temp, *tmpTsk;
  GList *l, *tasks = NULL;

  total = g_list_length (data->tasksList);
  if(total==0)
     return 0;
  /* first, we work with the task ID itself */
printf ("-------------------------\n");
  rc = tasks_compute_dates_for_linked_new_task (id, curDuration, curDays, insertion_row, 1, current, TRUE, FALSE, dlg, data);
printf ("-------------------------\n");
  if(rc>TASK_PLACEMENT_UPDATED_END)
     return 0;
  /* we test for successord */
  rc = links_check_task_successors (id, data);
  if(rc<0) { printf ("je quitte, pas de succ \n");
     return 0;
  }
  /* we count how many tasks, apart groups and milestones we have */
  vector = g_malloc0 (total*sizeof(gint));/* we store here IDs of tasks */
  nodes = g_malloc0 (total*sizeof(gint));/* we store here IDs of tasks nodes when a task has more than 1 successor*/
  nbTsks = 0;
  for(i=0; i<total; i++) {
     l = g_list_nth (data->tasksList, i);
     tmpTsk = (tasks_data *) l->data;
     if(tmpTsk->type == TASKS_TYPE_TASK  || tmpTsk->type == TASKS_TYPE_MILESTONE) {
        vector [nbTsks] = tmpTsk->id;
      //  printf ("ID =%d nom = %s \n", tmpTsk->id, tmpTsk->name);
        nbTsks++;
     }
  }/* next i */
  /* if nbTsks>0 we allocate an array of (n tasks) * (n tasks) where we put dependencies */
  if(nbTsks>0) {
     array = g_malloc0 (nbTsks*nbTsks*sizeof(gint));
     /* now we fill array with dependancies - code == 1 when dependant, or 0 if no link */
     /* in lines, tasks-receuvers, in columns, predecessors */
     for(i=0; i<nbTsks; i++) {
        receiver = vector [i];
        for(j=0; j<nbTsks;j++) {
           if(links_check_element (receiver, vector [j], data)>=0) {
              array [i*nbTsks+j] = 1;
           }
        }/* next j */
     }/* next i for vectors*/
     /* we sum all nodes vertically since senders are arranged in columns */
     for(j=0;j<nbTsks;j++) {
        for(i=0;i<nbTsks;i++) {
           nodes[j]=nodes[j]+array[i*nbTsks+j];
        }/* lines = nodes */
     }/* next j - columns */
/*
			printf ("matrice \n");
			printf("-\t");
			for(i=0;i<nbTsks;i++) {
			printf("Id=%d\t", vector[i]);
			}
			printf("\n");
			for(i=0; i<nbTsks; i++) {
			printf("Id=%d\t", vector[i]);
			  for(j=0; j<nbTsks; j++) {
			     printf("%d\t", array [i*nbTsks+j]);
			  }
			  printf("\n");
			}
			  printf("nodes \t");
                        for(i=0;i<nbTsks;i++) {
                           printf("%d\t", nodes[i]);
                        }
			  printf("\n");
     /* now we compute -  do loop */
     tasks = g_list_prepend (tasks, GINT_TO_POINTER (id));/* first alement is always the ID of modified source task */
  //   status = g_list_prepend (status, GINT_TO_POINTER (-1));/* the is nothing to do with ROOT task */

     while(g_list_length(tasks)>0) {
        l = g_list_nth (tasks, 0); /* always first element of a FIFO stack */
        sender = (gint *) l->data;
        /* we're looking for successors and manage them */
        for(i=0;i<nbTsks;i++) {
            if(links_check_element (vector [i], sender, data)>=0) {
               printf ("%d reçoit de %d \n", vector [i], sender);
               tasks = g_list_prepend (tasks, GINT_TO_POINTER (vector[i]));
               rank = tasks_get_rank_for_id (vector[i], data);
               tasks_get_ressource_datas (rank, &temp, data);
               total_done++;
printf ("****************\n");
               rc = tasks_compute_dates_for_linked_new_task (vector [i], 
                         temp.durationMode, temp.days, rank, total_done+1, current, TRUE, FALSE, dlg, data);
printf ("===================\n");
               //if(rc<=TASK_PLACEMENT_UPDATED_END)/* no critical error */
                 // total_done++;// ds UNDO nbTasks 
            }/* endif links */
        }/* next i */
        tasks = g_list_remove_link (tasks, l);
     }/* wend */
     /* we free memory */
     if(tasks) 
        g_list_free (tasks);
     g_free (array);
  }/* endif >0 */
  g_free (vector);
  g_free (nodes);

  return total_done;
}

/*****************************************
  PUBLIC : check if tasks are receiver
  from 'id' ; if YES, we update 
  tasks widgets' name
*****************************************/
void tasks_utils_update_receivers (gint id, APP_data *data)
{
  gint i, total, ret;
  GList *l;
  tasks_data *tmpTsk;

  total = g_list_length (data->tasksList);
  for(i=0;i<total;i++) {
    l = g_list_nth (data->tasksList, i);
    tmpTsk = (tasks_data *) l->data;
    ret = links_check_element (tmpTsk->id, id, data);
    if(ret>=0) {
       /* we shall update widget - only if it's a task, not a milestone*/
       if(tmpTsk->type == TASKS_TYPE_TASK) {
          tasks_remove_widgets (tmpTsk->cTaskBox);
          tasks_add_display_tasks (tmpTsk->id, tmpTsk->cTaskBox, data);
       }
    }/* endif */
  }/* next i */
}

/*****************************************
  PUBLIC : check if tasks are receiver
  from 'id' ; if YES, we remove 
  occurences of 'id'
*****************************************/
void tasks_utils_remove_receivers (gint id, APP_data *data)
{
  gint i, total, ret;
  GList *l;
  tasks_data *tmpTsk;

  total = g_list_length (data->tasksList);
  for(i=0;i<total;i++) {
    l = g_list_nth (data->tasksList, i);
    tmpTsk = (tasks_data *) l->data;
    ret = links_check_element (tmpTsk->id, id, data);
    if(ret>=0) {
       /* we shall update widget - only if it's a task, not a milestone*/
       if(tmpTsk->type == TASKS_TYPE_TASK) {
          tasks_remove_widgets (tmpTsk->cTaskBox);
          tasks_remove_display_tasks (tmpTsk->id, id, tmpTsk->cTaskBox, data);
       }
    }/* endif */
  }/* next i */
}


