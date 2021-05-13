#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <stdlib.h>
/* translations */
#include <libintl.h>
#include <locale.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include "support.h"
#include "misc.h"
#include "links.h"
#include "tasksutils.h"

/*******************************
   PUBLIC : check if a "pair"
   exists
*******************************/
gint links_check_element (gint receiver, gint sender, APP_data *data)
{
  gint ret = -1, i = 0;
  GList *l;
  gint links_list_len = g_list_length (data->linksList);
  link_datas *tmp_links_datas;

  /* first, we test receiver */
  while((i<links_list_len) && (ret<0)) {
     l = g_list_nth (data->linksList, i);
     tmp_links_datas = (link_datas *)l->data;
     if(tmp_links_datas->receiver == receiver )
        ret = i;
     i++;
  }/* wend */ 
  /* if not ret >=0 , there isn't receiver match */
  if(ret<0)
     return -1;
  /* we have found receiver, so we continue for sender */
  i = 0;
  ret = -1;
  while ((i<links_list_len) && (ret < 0 )) {
     l = g_list_nth (data->linksList, i);
     tmp_links_datas = (link_datas *)l->data;
     if((tmp_links_datas->sender == sender) && (tmp_links_datas->receiver == receiver) )
        ret = i;
     i++;
  }/* wend */ 
  return ret;
}

/*******************************
   PUBLIC : get "pair"
   link mode for tasks to tasks
*******************************/
gint links_get_element_link_mode (gint receiver, gint sender, APP_data *data)
{
  gint ret = -1, i = 0;
  GList *l;
  gint links_list_len = g_list_length (data->linksList);
  link_datas *tmp_links_datas;

  /* first, we test receiver */
  while((i<links_list_len) && (ret<0)) {
     l = g_list_nth (data->linksList, i);
     tmp_links_datas = (link_datas *)l->data;
     if(tmp_links_datas->receiver == receiver )
        ret = i;
     i++;
  }/* wend */ 
  /* if not ret >=0 , there isn't receiver match */
  if(ret<0)
     return -1;
  /* we have found receiver, so we continue for sender */
  i = 0;
  ret = -1;
  while ((i<links_list_len) && (ret < 0 )) {
     l = g_list_nth (data->linksList, i);
     tmp_links_datas = (link_datas *)l->data;
     if((tmp_links_datas->sender == sender) && (tmp_links_datas->receiver == receiver) )
        ret = tmp_links_datas->mode;
     i++;
  }/* wend */ 
  return ret;
}

/*******************************
   PUBLIC : check if a ressource
   is used by any task
*******************************/
gint links_check_rsc (gint rsc, APP_data *data)
{
  gint ret = -1, i = 0;
  GList *l;
  gint links_list_len = g_list_length (data->linksList);
  link_datas *tmp_links_datas;

  /* we test if ressource is sender anywhere  */
  while( (i<links_list_len) && (ret<0 ) ) {
     l = g_list_nth (data->linksList, i);
     tmp_links_datas = (link_datas *)l->data;
     if(tmp_links_datas->sender == rsc )
        ret = i;
     i++;
  }/* wend */ 

  return ret;
}

/*******************************
   PUBLIC : check if a task
    use any task
*******************************/
gint links_check_task_linked (gint tsk, APP_data *data)
{
  gint ret = -1, i = 0;
  GList *l;
  gint links_list_len = g_list_length (data->linksList);
  link_datas *tmp_links_datas;

  /* we test if ressource is sender anywhere  */
  while( (i<links_list_len) && (ret<0 ) ) {
     l = g_list_nth (data->linksList, i);
     tmp_links_datas = (link_datas *)l->data;
     if((tmp_links_datas->receiver == tsk) && (tmp_links_datas->type==LINK_TYPE_TSK_TSK))
        ret = i;
     i++;
  }/* wend */ 

  return ret;
}

/**********************************************
   PUBLIC : check if a task
    has any task successor - returns >=0 if OK
***********************************************/
gint links_check_task_successors (gint tsk, APP_data *data)
{
  gint ret = -1, i = 0;
  GList *l;
  gint links_list_len = g_list_length (data->linksList);
  link_datas *tmp_links_datas;

  /* we test if ressource is sender anywhere  */
  while( (i<links_list_len) && (ret<0 ) ) {
     l = g_list_nth (data->linksList, i);
     tmp_links_datas = (link_datas *)l->data;
     if((tmp_links_datas->sender == tsk) && (tmp_links_datas->type==LINK_TYPE_TSK_TSK))
        ret = i;
     i++;
  }/* wend */ 

  return ret;
}

/*******************************
   PUBLIC : check if a task
    has any task predecessor
*******************************/
gint links_check_task_predecessors (gint tsk, APP_data *data)
{
  gint ret = -1, i = 0;
  GList *l;
  gint links_list_len = g_list_length (data->linksList);
  link_datas *tmp_links_datas;

  /* we test if ressource is sender anywhere  */
  while( (i<links_list_len) && (ret<0 ) ) {
     l = g_list_nth (data->linksList, i);
     tmp_links_datas = (link_datas *)l->data;
     if((tmp_links_datas->receiver == tsk) && (tmp_links_datas->type==LINK_TYPE_TSK_TSK))
        ret = i;
     i++;
  }/* wend */ 

  return ret;
}

/*******************************************************************
  PUBLIC : avoid circularity, i.e. a task should not be at the same
  time a sender and a receiver for another task ; returns TRUE
  if circularity is detected
*******************************************************************/
gboolean links_test_circularity (gint tsk, gint other_tsk, APP_data *data)
{
  gboolean ret = FALSE;
  GList *l;
  gint i, links_list_len = g_list_length (data->linksList);
  link_datas *tmp_links_datas;

  for(i=0;i<links_list_len;i++) {
     l = g_list_nth (data->linksList, i);
     tmp_links_datas = (link_datas *)l->data;
     if((tmp_links_datas->receiver == tsk) && (tmp_links_datas->type==LINK_TYPE_TSK_TSK) && (tmp_links_datas->sender == other_tsk)) {
        ret = TRUE;
     }
  }/* next i */
  return ret;
}

/*******************************
   PUBLIC : check latest
   GDate of task tsk
*******************************/
GDate links_get_latest_task_linked (gint tsk, APP_data *data)
{
  gint ret = -1, i = 0, sender, j, cmp;
  GList *l, *l1;
  gint links_list_len = g_list_length (data->linksList);
  link_datas *tmp_links_datas;
  tasks_data *temp;
  GDate date, cur_date, s_date, e_date;

  g_date_set_dmy (&date, data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);

  /* we test if ressource is sender anywhere  */
  while(i<links_list_len) {
     l = g_list_nth (data->linksList, i);
     tmp_links_datas = (link_datas *)l->data;
     if((tmp_links_datas->receiver == tsk) && (tmp_links_datas->type==LINK_TYPE_TSK_TSK)) {
        sender = tmp_links_datas->sender;/* ID of sender */
        j=0;
        ret = -1;
        while((j<g_list_length (data->tasksList)) && (ret<0)) {
           l1 = g_list_nth (data->tasksList, j);
           temp = (tasks_data *) l1->data;
           if(temp->id == sender) {//printf ("ds links test pour sender = %d dates début =%d %d %d fin = %d %d %d \n", sender,  temp->start_nthDay, temp->start_nthMonth, temp->start_nthYear,
// temp->end_nthDay, temp->end_nthMonth, temp->end_nthYear);
              /* rustine ! ouh ! */
              g_date_set_dmy (&s_date, temp->start_nthDay, temp->start_nthMonth, temp->start_nthYear);
              g_date_set_dmy (&e_date, temp->end_nthDay, temp->end_nthMonth, temp->end_nthYear);
              cmp = g_date_compare (&e_date, &s_date);
              if(cmp<=0) {printf ("* rustine correction ! *\n");
                 g_date_set_dmy (&e_date, temp->start_nthDay, temp->start_nthMonth, temp->start_nthYear);
                 //g_date_add_days (&e_date, temp->days-1);
                 temp->end_nthDay = g_date_get_day (&e_date);
                 temp->end_nthMonth = g_date_get_month (&e_date);
                 temp->end_nthYear = g_date_get_year (&e_date);
              }
              ret = j;
              g_date_set_dmy (&cur_date, temp->end_nthDay, temp->end_nthMonth, temp->end_nthYear);
              cmp = g_date_compare (&cur_date, &date);
              if(cmp>0)
                 g_date_set_dmy (&date, temp->end_nthDay, temp->end_nthMonth, temp->end_nthYear);
           }/* endif */
           j++;
        }/* wend tsk */
     }/* endif */
     i++;
  }/* wend links */ 

  return date;
}

/*********************************************************
   PUBLIC : remove tasks ending too late for current task
   arguments : 
   - Id of task
   - GDate limit
*********************************************************/
void links_remove_tasks_ending_too_late (gint id, GDate *lim_date, APP_data *data)
{
  link_datas *tmp_links_datas;
  tasks_data *tmp_tsk;
  GList *l, *l1;
  gint i, total, sender, index, cmp;
  GDate end_date;

  total = g_list_length (data->linksList);

  for(i=total-1; i>=0; i--) {
     l = g_list_nth (data->linksList, i);
     tmp_links_datas = (link_datas *) l->data;
     if(tmp_links_datas->receiver == id) {
         sender = tmp_links_datas->sender;
         /* now we are looking for ending date of sender task */
         index = tasks_get_rank_for_id (sender, data);
         if(index>=0) {
            l1 = g_list_nth (data->tasksList, index);
            tmp_tsk = (tasks_data *) l1->data;
            g_date_set_dmy (&end_date, tmp_tsk->end_nthDay, tmp_tsk->end_nthMonth, tmp_tsk->end_nthYear);
            cmp = g_date_compare (lim_date, &end_date);
            if(cmp<=0) {/* we should remove link */
                links_free_at (i, data);// pb lg liste change !!! */
            }/* endif cmp */
         }/* endif index */
     }/* endif id*/
  }/* next */
}

/*************************************************************
 PUBLIC
  add a link at the end of the  links Glist
 Indeed, with the checking,   we haven't to add a link
  at a defined position ;-)

 The Glist is in data structure  idElem are gint values about
 objects (tasks, ressources ...)
**************************************************************/
void links_append_element (gint idElem1, gint idElem2, gint type, gint mode, APP_data *data)
{
  if(idElem1>=0 && idElem2>=0) {
    /* we add datas to the GList */
    link_datas *tmp_links_datas;
    tmp_links_datas = g_malloc(sizeof(link_datas));
    /* store values from quasi global var */

    // TODO check if the pair elem1 eleme2 already exists !!!!
    tmp_links_datas->receiver = idElem1;
    tmp_links_datas->sender = idElem2;
    tmp_links_datas->type = type;
    tmp_links_datas->mode = mode;
    /* update list */
    data->linksList = g_list_append(data->linksList, tmp_links_datas);
  }
}

/*****************************************
  PUBLIC function. Remove all links datas 
  from memory
  for example when we quit the
  program or call 'new' menu
*****************************************/
void links_free_all (APP_data *data)
{
  link_datas *tmp_links_datas;
  GList *l = g_list_first (data->linksList);

  for(l; l!=NULL; l=l->next) {
     tmp_links_datas = (link_datas *)l->data;
     g_free (tmp_links_datas);
  }/* next */
  /* free the Glist itself */
  g_list_free (data->linksList);
  data->linksList = NULL;

  printf("* PlanLibre : successfully freed Links datas *\n");
}

/*****************************************
  PUBLIC function. Duplicate all links datas 
  from memory to a new list
*****************************************/
GList *links_copy_all (APP_data *data)
{
  link_datas *tmp_links_datas, *tmp;
  GList *newList = NULL;
  GList *l = g_list_first (data->linksList);

  for(l; l!=NULL; l=l->next) {
     tmp_links_datas = (link_datas *)l->data;
     tmp = g_malloc (sizeof(link_datas));
     tmp->type = tmp_links_datas->type;
     tmp->sender = tmp_links_datas->sender;
     tmp->receiver = tmp_links_datas->receiver;
     tmp->mode = tmp_links_datas->mode;
     newList = g_list_append (newList, tmp);
  }/* next */

  return newList;
}

/***********************************
  PUBLIC function
  remove links datas at position
  from memory
***********************************/
void links_free_at (gint position, APP_data *data)
{
  GList *l;
  gint links_list_len = g_list_length (data->linksList);

  if( position < links_list_len) {
     l = g_list_nth (data->linksList, position);
     link_datas *tmp_links_datas;
     tmp_links_datas = (link_datas *)l->data;
     g_free (tmp_links_datas);
     /* update list */
     data->linksList = g_list_delete_link (data->linksList, l); 
  }/* endif */
}

/***********************************
  PUBLIC function
  modify a links datas 
  at a given position

***********************************/
void links_modify_datas (gint position, APP_data *data, link_datas values)
{
  gint links_list_len = g_list_length (data->linksList);
  GList *l;

  if((position >=0) && (position<links_list_len)) {
     l = g_list_nth (data->linksList, position);
     link_datas *tmp_links_datas;
     tmp_links_datas = (link_datas *)l->data;
     tmp_links_datas->receiver = values.receiver;
     tmp_links_datas->sender = values.sender;
     tmp_links_datas->type = values.type;
     tmp_links_datas->mode = values.mode;
  }/* endif */
}

/***************************************
  LOCAL function 
  get datas in links at position
 we pass a POINTER in order to modify
  content of a link values
****************************************/
void links_get_ressource_datas (gint position, link_datas *values, APP_data *data )
{
  gint i;
  GList *l;

  gint links_list_len = g_list_length (data->linksList);

  /* we get  pointer on the element */
  if( (position<links_list_len) && (position >=0) ) {
     l = g_list_nth (data->linksList, position);
     link_datas *tmp_links_datas;
     tmp_links_datas = (link_datas *)l->data;
     
     /* get datas strings */
     values->type = tmp_links_datas->type;  
     values->receiver = tmp_links_datas->receiver;  
     values->sender = tmp_links_datas->sender;
     values->mode = tmp_links_datas->mode;
  }/* endif */
}

/***********************************
  PUBLIC function
  after task removing, remove
  all links with the task
  'id' = internal task identifier
***********************************/
void links_remove_all_task_links (gint id, APP_data *data)
{
  gint i;
  GList *l = g_list_last (data->linksList);
  link_datas *tmp_links_datas;

  while(l) {
     tmp_links_datas = (link_datas *)l->data;
     if((tmp_links_datas->receiver == id) || (tmp_links_datas->sender == id)) {
         g_free (tmp_links_datas);
         /* update list */
         data->linksList = g_list_delete_link (data->linksList, l); /* points on start*/
         l = g_list_last (data->linksList);
     }
     if(l)
        l = l->prev;
  }/* wend */
}

/***********************************
  PUBLIC function
  after ressource removing, remove
  all links with this ressource
  'id' = internal rsc identifier
***********************************/
void links_remove_all_rsc_links (gint id, APP_data *data)
{
  gint i;
  GList *l = g_list_last(data->linksList);
  link_datas *tmp_links_datas;
  gint links_list_len = g_list_length (data->linksList);
// TODO : undo machine !!!!!

  for(l; l!=NULL; l=l->prev) {
     tmp_links_datas = (link_datas *)l->data;
     if((tmp_links_datas->receiver == id) || (tmp_links_datas->sender == id)) {
         g_free (tmp_links_datas);
         /* update list */
         data->linksList = g_list_delete_link (data->linksList, l); /* points on start*/
         l = g_list_last (data->linksList);
     }
  }
}

/***********************************
  PUBLIC function
  this function remove link between
  task id1 and ressource id2
  'id1' & "id2"= internal rsc identifier
**********************************************/
void links_remove_rsc_task_link (gint idTsk, gint idRsc, APP_data *data)
{
  gint i = 0, ret = -1;
  GList *l = g_list_last (data->linksList);
  link_datas *tmp_links_datas;
  gint links_list_len = g_list_length (data->linksList);

  while((i<links_list_len) && (ret<0)) {
     l = g_list_nth (data->linksList, i);
     tmp_links_datas = (link_datas *)l->data;

     if((tmp_links_datas->receiver == idTsk) && (tmp_links_datas->sender == idRsc)) {
         g_free (tmp_links_datas);
         /* update list */
         data->linksList = g_list_delete_link (data->linksList, l); /* points on start*/
         ret = i;
     }
     i++;
  }
}

/********************************************
  PUBLIC : test concurrent (conflictual) 
  usage of a ressource.
  Should return 0 (zero) if isn't any conflict
  arguments : id of a task, id of a rsc
  sender, id_src, dates
  id_src should be equal to -1 in order to
   avoid tests. id_src has a true task id
   in other cases
********************************************/
gint links_test_concurrent_rsc_usage (gint id, gint sender, gint id_dest, GDate start, GDate end, APP_data *data)
{
  gint i, ret = 0, cmpst1, cmpst2, cmped1, cmped2;
  gboolean coincStart, coincEnd;
  GList *l;
  link_datas *tmp_links_datas;
  tasks_data *tmp_tsk_datas;
  GDate date_task_start, date_task_end;

  /* we must check if ressource 'sender' isn't yet used */
  if(links_check_rsc (sender, data) <0) 
     return 0;

  /* we get starting & ending date of current task */
  i = 0;
  while((i<g_list_length (data->tasksList)) && (ret == 0)) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if((id != tmp_tsk_datas->id) && (id_dest != tmp_tsk_datas->id) && (tmp_tsk_datas->type == TASKS_TYPE_TASK)) {
	 g_date_set_dmy (&date_task_start, tmp_tsk_datas->start_nthDay, tmp_tsk_datas->start_nthMonth, tmp_tsk_datas->start_nthYear);
	 g_date_set_dmy (&date_task_end, tmp_tsk_datas->end_nthDay, tmp_tsk_datas->end_nthMonth, tmp_tsk_datas->end_nthYear);
	 /* now, we test if there is any concurrence on dates */
         /*
                           #########################  tsk
                               [start rsc to test]
         */
	 cmpst1 =  g_date_compare (&start, &date_task_start); /* if <0 NOT concurrents */
	 cmpst2 =  g_date_compare (&start, &date_task_end); /* if >0 NOT concurrents */
         coincStart = FALSE;
         if((cmpst1>=0) && (cmpst2<=0))
                 coincStart = TRUE;
         /*
                           #########################  tsk
                               [end rsc to test]
         */
	 cmped1 =  g_date_compare (&end, &date_task_start); /* if <0 NOT concurrents */
	 cmped2 =  g_date_compare (&end, &date_task_end); /* if >0 NOT concurrents */
         coincEnd = FALSE;
         if((cmped1>=0) && (cmped2<=0))
                 coincEnd = TRUE;
	 if((coincStart) || (coincEnd)) {/* now we can check potential ressources conflicts */
		gint res = links_check_element (tmp_tsk_datas->id, sender, data);
                if(res!=0) {
                     printf("* ressource conflict for rsc ID=%d \n", sender);
                     ret = sender;
                }
	 }/* endif coinc dates */
     }/* endif */
     i++;
  }/* wend */
  return ret;
}

/****************************************
 PUBLIC : duplicate ressources 
 arguments : 
 - id_src identifier for source task
 - id_dest : identifier for cloned task
****************************************/
void links_copy_rsc_usage (gint id_src, gint id_dest, GDate start, GDate end, APP_data *data)
{
  gint conflict, i;
  gint links_list_len = g_list_length (data->linksList);
  GList *l;
  link_datas *tmp_links_datas;

  if(id_src<0 || id_dest<0)
     return;

  for(i=0;i<links_list_len;i++) {
     l = g_list_nth (data->linksList, i);
     tmp_links_datas = (link_datas *)l->data;
     if((tmp_links_datas->receiver == id_src) && (tmp_links_datas->type==LINK_TYPE_TSK_RSC)) {/* we found Id to copy */
        /* now we append ressource to id_dest */
        conflict =  links_test_concurrent_rsc_usage (id_src, tmp_links_datas->sender, id_dest, start, end, data);
        if(conflict==0)
            links_append_element (id_dest, tmp_links_datas->sender, LINK_TYPE_TSK_RSC, LINK_END_START, data);
     }/* endif ID src */
  }/* next i */

}
