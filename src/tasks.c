/* -------------------------------------
|   management of Tasks                |
|                                      |
/*------------------------------------*/

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
#include "timeline.h"
#include "assign.h"
#include "dashboard.h"
#include "tasks.h"
#include "tasksdialogs.h"
#include "tasksutils.h"
#include "undoredo.h"
#include "callbacks.h"

/*****************************************************************************************************
   LOCAL :  update links list for tasks
   NOTE : the task_add_dialog must NOT be destroyed UNTIL the program calls this function
*****************************************************************************************************/
static void tasks_update_links_tasks (gint id, GtkWidget *win, APP_data *data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  gboolean valid, status;
  gint i=0, link, sender;
  gchar *str_data, *int_data;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW(data->treeViewTasksTsk));
  /* get list's first iterator */
  valid = gtk_tree_model_get_iter_first (model, &iter);

  /* Ok, we continue, liststore isn't empty */
  while(valid)  {
      gtk_tree_model_get (model, &iter, USE_TSK_COLUMN, &status,
                                        NAME_TSK_COLUMN, &str_data,
                                        ID_TSK_COLUMN, &int_data, -1);
      if(&iter) {
         /* is a links already exists ? */
         sender = atoi (int_data);
         link = links_check_element (id, sender, data);
         if(link<0) {
            // printf(" pas de lien entre %d et %d \n", id, sender);
             if(status) {
               /* here we check for cicularity */
               if(links_test_circularity (sender, id, data)){
                   gchar *msg;                   
                   msg = g_strdup_printf ("%s", _("<b>Can't do that !</b>\n\Tasks conflict !\nThis task can't be at the same time\na predecessor and a successor for another task !"));
                   misc_ErrorDialog (win, msg);
                   g_free (msg);
               } 
               else {
                 //   printf("comme status activé, je crée le lien \n");
                 links_append_element (id, sender, LINK_TYPE_TSK_TSK, LINK_END_START, data);
               }
             }
         }
         else {
            // printf(" existe déjà un lien entre %d et %d \n", id, sender);
             if(!status) {
              //   printf("comme status désactivé, je supprime le lien \n");
                 links_free_at (link, data);
             }
         }
     //    g_print ("ID = %d Rangée %d: (%s,%d)  status %d \n", id, i, str_data, sender,  status);
         g_free (str_data);
         g_free (int_data);
         /* we check if a pair exists */
         i ++;
      }/* endif iter */
      valid = gtk_tree_model_iter_next (model, &iter);
  }/* wend */
}

/*****************************************************************************************************
   LOCAL :  update links list
   NOTE : the task_add_dialog must NOT be destroyed UNTIL the program calls this function
   'id' internal identifier for task
*****************************************************************************************************/
static void tasks_update_links (GtkWidget *win, gint id, APP_data *data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  gboolean valid, status, flag;
  gint i, link, ret = -1, sender, rep;
  GList *l;
  tasks_data *tmp_tsk_datas;
  GDate date_task_start, date_task_end;
  gchar *str_data, *int_data;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW(data->treeViewTasksRsc));
  /* get list's first iterator */
  valid = gtk_tree_model_get_iter_first (model, &iter);
  if(!valid)
     return;

  /* Ok, we continue, listore isn't empty - we get starting & ending date of current task */
  i = 0;
  while((i<g_list_length (data->tasksList)) && (ret<0)) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if(tmp_tsk_datas->id == id) {
        ret = id;
        g_date_set_dmy (&date_task_start, 
                  tmp_tsk_datas->start_nthDay, tmp_tsk_datas->start_nthMonth, tmp_tsk_datas->start_nthYear);
        g_date_set_dmy (&date_task_end, 
                  tmp_tsk_datas->end_nthDay, tmp_tsk_datas->end_nthMonth, tmp_tsk_datas->end_nthYear);
     }
     i++;
  }/* wend */

  while(valid)  {
      gtk_tree_model_get (model, &iter, USE_RSC_COLUMN, &status, NAME_RSC_COLUMN, &str_data,
                                        ID_RSC_COLUMN, &int_data, -1);

      /* do something with those datas */
      if(&iter) {
         /* is a links already exists ? */
         sender = atoi (int_data);
         link = links_check_element (id, sender, data);
         if(link<0) {
         //    printf(" pas de lien entre %d et %d \n", id, sender);
             if(status) {
                 /* now we test if there is a concurrent usage of the same resseource */
                 gint conflict =  links_test_concurrent_rsc_usage (id, sender, -1, date_task_start, date_task_end, data);
                 if(conflict == 0) {/* here we test if concurrent usage is allowed TODO */
               //     printf("comme status activé, je crée le lien \n");
                    links_append_element (id, sender, LINK_TYPE_TSK_RSC, LINK_END_START, data);
                 }
                 else {/* here we can define a specific message when concurrent usage of ressource is allowed */
                    gchar *msg;
                    /* is concurrent usage is allowed for current ressource ? */
                    flag =  rsc_get_concurrent_status_for_id (sender, data);
                    if(flag) {
                        msg = g_strdup_printf (_("<b>Warning !</b>\n\Ressources conflict !\nYou have authorized concurrent usage of\na ressource for more than ONE task.\nAre you sure to really use ressource(s) :\n<b>%s</b>\non several tasks for the same period ?") , 
						rsc_get_name (conflict, data));
                        rep =  misc_QuestionDialog (win, msg);
                        if(rep == GTK_RESPONSE_YES) {
						  links_append_element (id, sender, LINK_TYPE_TSK_RSC, LINK_END_START, data);	
					    }
                    }   
                    else {
						msg = g_strdup_printf (_("<b>Can't do that !</b>\n\Ressources conflict !\nThis task can't use same ressource(s) than other task(s). \nCheck if ressource(s) :\n<b>%s</b>\nisn't used on the same period by an other task, in order to solve this issue.") , 
						rsc_get_name (conflict, data));
                        misc_ErrorDialog (win, msg);
                    }
                    g_free (msg);
                 }/* elseif */
             }
         }
         else {
         //    printf(" existe déjà un lien entre %d et %d \n", id, sender);
             if(!status) {/* can't find ressource */
              //   printf("comme status désactivé, je supprime le lien \n");
                 links_free_at (link, data);
             }
         }
     //    g_print ("ID = %d Rangée %d: (%s,%d)  status %d \n", id, i, str_data, sender,  status);
         g_free (str_data);
         g_free (int_data);
         /* we check if a pair exists */
         i ++;
      }/* endif iter */
      valid = gtk_tree_model_iter_next (model, &iter);
  }/* wend */

}

/*****************************************
  PUBLIC function
  remove all tasks datas from memory
  for example when we quit the
  program or call 'new' menu
****************************************/
void tasks_free_all (APP_data *data)
{
  GList *l = g_list_first (data->tasksList);
  gint i=0;
  tasks_data *tmp_tasks_datas;

  for(l; l!=NULL; l=l->next) {
     tmp_tasks_datas = (tasks_data *)l->data;
     /* free strings */
     if(tmp_tasks_datas->name) { 
         g_free (tmp_tasks_datas->name);
         tmp_tasks_datas->name=NULL;
     }
     if(tmp_tasks_datas->location) { 
         g_free (tmp_tasks_datas->location);
         tmp_tasks_datas->location=NULL;
     }
     /* free Goocanvas items */
     if(tmp_tasks_datas->groupItems) {
        goo_canvas_item_remove (tmp_tasks_datas->groupItems);
        tmp_tasks_datas->groupItems = NULL;
     }          
     /* free element */
     g_free (tmp_tasks_datas);
     i++;
  }/* next l */
  /* free the Glist itself */
  g_list_free (data->tasksList);
  data->tasksList = NULL;
  printf ("* PlanLibre : successfully freed Tasks datas *\n");
}

/**********************************
  PROTECTED : callback for button
  modify packed inside row
**********************************/
static void on_button_modify_task_clicked (GtkButton *button, APP_data *data)
{
  GtkListBox *box;
  /* we go along widget hierarchy inside row */
  GtkWidget *cBox = gtk_widget_get_parent (GTK_WIDGET(button));
  GtkWidget *cadre = gtk_widget_get_parent (GTK_WIDGET(cBox));
  GtkWidget *grid = gtk_widget_get_parent (GTK_WIDGET(cadre));
  GtkWidget *row = gtk_widget_get_parent (GTK_WIDGET(grid));

  gint index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW(row));
  if(index>=0) {
     box = GTK_LIST_BOX(gtk_builder_get_object (data->builder, "listboxTasks"));  
     gtk_list_box_select_row (box, GTK_LIST_BOX_ROW(row));
     data->curTasks = index;
     tasks_modify (data);
  }
}

static void on_button_modify_group_clicked (GtkButton *button, APP_data *data)
{
  GtkListBox *box;
  /* we go along widget hierarchy inside row */
  GtkWidget *grid = gtk_widget_get_parent (GTK_WIDGET(button));
  GtkWidget *cadre = gtk_widget_get_parent (GTK_WIDGET(grid));
  GtkWidget *row = gtk_widget_get_parent (GTK_WIDGET(cadre));

  gint index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW(row));
  if(index>=0) {
     box = GTK_LIST_BOX(gtk_builder_get_object (data->builder, "listboxTasks"));  
     gtk_list_box_select_row (box, GTK_LIST_BOX_ROW(row));
     data->curTasks = index;
     tasks_modify (data);
  }
}

/*************************
 PUBLIC :
 add a new task widget
*************************/
GtkWidget *tasks_new_widget (gchar *name, gchar *periods, gdouble progress, gint run, gint pty, gint categ, 
                             GdkRGBA color, gint status, tasks_data *tmp, APP_data *data)
{
  GtkWidget *cadreGrid, *cadre;
  GtkWidget *iconBox, *textBox, *subTextBox, *participBox, *subparticipBox, *rscBox, *interBox, *interIcoBox, *taskBox;
  GtkWidget *separator1, *separator2, *icon_group, *label_name, *label_periods, *label_progress, *label_rsc, *label_tasks_used;
  GtkWidget *imgPty, *imgCateg, *imgStatus, *label_run;
  GtkWidget *btn;
  GdkPixbuf *pix;
  gchar *sRgba, *tmpStr;
  GError *err = NULL;

  cadre = gtk_frame_new (NULL);
  if(tmp->group>=0)
      g_object_set (cadre, "margin-left", 32, NULL);
  else
      g_object_set (cadre, "margin-left", 4, NULL);
  g_object_set (cadre, "margin-right", 4, NULL);
  g_object_set (cadre, "margin-top", 1, NULL);
  g_object_set (cadre, "margin-bottom", 1, NULL);
  gtk_widget_show (cadre);
  cadreGrid = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  g_object_set (cadreGrid, "margin-left", 4, NULL);
  gtk_container_add (GTK_CONTAINER (cadre), cadreGrid);
  iconBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  textBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
//  interIcoBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  interBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  g_object_set (interBox, "margin-bottom", 0, NULL);
  subTextBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  participBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  subparticipBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  GtkWidget *supTaskBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  taskBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  g_object_set (participBox, "margin-left", 4, NULL);
  rscBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

  separator2 = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
  g_object_set (separator2, "margin-left", 8, NULL);
  g_object_set (separator2, "margin-right", 8, NULL);

  gtk_box_pack_start (GTK_BOX(cadreGrid), textBox, FALSE, FALSE, 0);
//  gtk_box_pack_start (GTK_BOX(cadreGrid), interIcoBox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(cadreGrid), interBox, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX(interBox), participBox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(interBox), separator2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(interBox), supTaskBox, FALSE, FALSE, 0);

  /* text box */
  /* icon for group */
  if(tmp->group>=0) {
      pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("groupedtsk.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);
      icon_group = gtk_image_new_from_pixbuf (pix);
      g_object_unref (pix);
  }
  else {
     pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("ungroup.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);
     icon_group = gtk_image_new_from_pixbuf (pix);
     g_object_unref (pix);
  }
  gtk_box_pack_start (GTK_BOX(textBox), icon_group, FALSE, FALSE, 4);
  /* edit button */
  btn = gtk_button_new_from_icon_name ("view-more-symbolic", GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX(textBox), btn, FALSE, FALSE, 4);
  /* branch callback */
  g_signal_connect ((gpointer)btn, "clicked", G_CALLBACK (on_button_modify_task_clicked), data);


  /* name */
  sRgba = misc_convert_gdkcolor_to_hex (color);
  tmpStr = g_markup_printf_escaped ("<big><span background=\"%s\">    </span><b> %s</b></big>", sRgba, name);
  label_name = gtk_label_new (tmpStr);
  g_free (sRgba);
  g_free (tmpStr);
  gtk_widget_set_halign (label_name, GTK_ALIGN_START);
  g_object_set (label_name, "xalign", 0, NULL);
  gtk_label_set_use_markup (GTK_LABEL(label_name), TRUE);
  gtk_label_set_ellipsize (GTK_LABEL(label_name), PANGO_ELLIPSIZE_END);
  gtk_label_set_width_chars (GTK_LABEL(label_name), 32);
  gtk_label_set_max_width_chars (GTK_LABEL(label_name), 32);
  gtk_box_pack_start (GTK_BOX(textBox), label_name, FALSE, FALSE, 4);
  /* periods */
  label_periods = gtk_label_new(g_strdup_printf ("<i>%s</i>", periods));
  gtk_widget_set_halign (label_periods, GTK_ALIGN_START);
  g_object_set (label_periods,  "yalign", 1.0, NULL);
  gtk_label_set_use_markup (GTK_LABEL(label_periods), TRUE);
  gtk_box_pack_start (GTK_BOX(textBox), label_periods, FALSE, FALSE, 4);

  gtk_box_pack_start (GTK_BOX(textBox), iconBox, FALSE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX(textBox), subTextBox, FALSE, FALSE, 4);
  /* category */
  switch(categ) {
    case 0: {/* mat */
      pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("material.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);         
      imgCateg = gtk_image_new_from_pixbuf (pix);
      g_object_unref (pix);
      break;
    }
    case 1: {/* intell */
      pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("intellectual.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);         
      imgCateg = gtk_image_new_from_pixbuf (pix);
      g_object_unref (pix);
      break;
    }
    case 2: {/* soc */
      pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("relational.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);         
      imgCateg = gtk_image_new_from_pixbuf (pix);
      g_object_unref (pix);
      break;
    }
  }/* end sw categ */
  gtk_widget_set_halign (imgCateg, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (imgCateg, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX(subTextBox), imgCateg, FALSE, FALSE, 2);
  /* progress */
  label_progress = gtk_label_new (g_strdup_printf ("<b>%.1f%</b>", progress));
  gtk_widget_set_halign (label_progress, GTK_ALIGN_START);
  g_object_set (label_progress, "yalign", 1.0, NULL);
  gtk_label_set_use_markup (GTK_LABEL(label_progress), TRUE);
  gtk_box_pack_start (GTK_BOX(subTextBox), label_progress, FALSE, FALSE, 4);

  /* icons - pty */
  switch(pty) {
    case 0: {
       pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("priority_high.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);
       imgPty = gtk_image_new_from_pixbuf (pix);
       g_object_unref (pix);
       break;
    }
    case 1: {
       pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("priority_medium.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);
       imgPty = gtk_image_new_from_pixbuf (pix);
       g_object_unref (pix);
       break;
    }
    case 2: {
       pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("priority_low.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);
       imgPty = gtk_image_new_from_pixbuf (pix);
       g_object_unref (pix);
       break;
    }
  }/* end sw pty */
  gtk_widget_set_halign (imgPty, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (imgPty, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX(iconBox), imgPty, FALSE, FALSE, 2);
  /* icon - status */
  switch(status) {
   case 0: {
       imgStatus = gtk_image_new_from_icon_name ("system-run", GTK_ICON_SIZE_LARGE_TOOLBAR);
       break;
    }
    case 1: {
       imgStatus = gtk_image_new_from_icon_name ("emblem-default", GTK_ICON_SIZE_LARGE_TOOLBAR);
       break;
    }
    case 2: {
       imgStatus = gtk_image_new_from_icon_name ("emblem-urgent", GTK_ICON_SIZE_LARGE_TOOLBAR);
       break;
    }
    case 3: {
       imgStatus = gtk_image_new_from_icon_name ("appointment-new", GTK_ICON_SIZE_LARGE_TOOLBAR);
       break;
    }
  }/* end sw status */

  gtk_widget_set_halign (imgStatus, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (imgStatus, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX(iconBox), imgStatus, FALSE, FALSE, 2);
  /* job done */
  if(run<=0)
    label_run = gtk_label_new (g_strdup_printf (_("<i>Time spent %s</i>"), "--"));
  else
    label_run = gtk_label_new (g_strdup_printf (_("<i>Time spent %.2d:%.2d</i>"), run/60, run % 60));
  gtk_widget_set_halign (label_run, GTK_ALIGN_START);
  g_object_set (label_run, "yalign", 1.0, NULL);
  gtk_label_set_use_markup (GTK_LABEL(label_run), TRUE);
  gtk_box_pack_start (GTK_BOX(textBox), label_run, FALSE, FALSE, 4);
  /* fill rsc bow with ressources used by the task */
  label_rsc  = gtk_label_new (g_strdup_printf ("<b><small>%s</small></b>", _("Ressources")));
  gtk_widget_set_halign (label_rsc, GTK_ALIGN_START);
  gtk_label_set_use_markup (GTK_LABEL(label_rsc), TRUE);
  gtk_box_pack_start (GTK_BOX(participBox), label_rsc, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(participBox), subparticipBox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(subparticipBox), rscBox, FALSE, FALSE, 0);
  /* tasks used by this task */
  label_tasks_used  = gtk_label_new (g_strdup_printf ("<b><small>%s</small></b>", _("Tasks required")));
  gtk_widget_set_halign (label_tasks_used, GTK_ALIGN_START);
  gtk_label_set_use_markup (GTK_LABEL(label_tasks_used), TRUE);
  gtk_box_pack_start (GTK_BOX(supTaskBox), label_tasks_used, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(supTaskBox), taskBox, FALSE, FALSE, 0);
  /* store widgets in temporary variable */
  tmp->cName = label_name;
  tmp->cGroup = icon_group;
  tmp->cPeriod = label_periods;
  tmp->cProgress = label_progress;
  tmp->cImgPty = imgPty;
  tmp->cImgStatus = imgStatus;
  tmp->cImgCateg = imgCateg;
  tmp->cRscBox = rscBox;
  tmp->cTaskBox = taskBox;
  tmp->cadre = cadre;
  tmp->groupItems = NULL;
  tmp->cDashboard = NULL;
  tmp->cRun = label_run;
  gtk_widget_show_all (GTK_WIDGET(cadre)); 
  return cadre;
}

/*******************************************************
  PUBLIC function - modify a widget at a given position
  NOTE : this function must be called
  ONLY when datas are modified in 
  memory, with "tasks_modify_datas()'
*******************************************************/
void tasks_modify_widget (gint position, APP_data *data)
{
  gint rsc_list_len = g_list_length (data->tasksList);
  GList *l;
  GdkPixbuf *pix;
  GError *err = NULL;
  gchar *sRgba, *tmpStr, *startDate = NULL, *human_date, *job_run;
  GDate date_current;

  if((position >=0) && (position<rsc_list_len)) {
     l = g_list_nth (data->tasksList, position);
     tasks_data *tmp_tasks_datas;
     tmp_tasks_datas = (tasks_data *)l->data;
     /* we change labels */
     sRgba = misc_convert_gdkcolor_to_hex(tmp_tasks_datas->color);
     tmpStr = g_markup_printf_escaped ("<big><span background=\"%s\">    </span><b> %s</b></big>",  sRgba, tmp_tasks_datas->name);
     gtk_label_set_markup (GTK_LABEL(tmp_tasks_datas->cName), tmpStr);
     g_free (sRgba);
     g_free (tmpStr);
     /* dates */
     startDate = misc_convert_date_to_str (tmp_tasks_datas->start_nthDay, 
                          tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
 
     g_date_set_dmy (&date_current, tmp_tasks_datas->start_nthDay, 
                          tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
     human_date = misc_convert_date_to_friendly_str (&date_current);
     gchar *str = g_strdup_printf (_("%s, %s"), human_date, 
                              misc_duration_to_friendly_str (tmp_tasks_datas->days, tmp_tasks_datas->hours*60));
     gtk_label_set_markup (GTK_LABEL(tmp_tasks_datas->cPeriod),
                           g_strdup_printf (_("<i>%s</i>"), str)); 
     g_free (human_date);
     g_free (startDate);
     g_free (str);
     /* job run */
     if(tmp_tasks_datas->timer_val>0)
         job_run = g_strdup_printf (_("<i>Time spent %.2d:%.2d</i>"), tmp_tasks_datas->timer_val/60, tmp_tasks_datas->timer_val % 60);
     else
         job_run = g_strdup_printf (_("<i>Time spent %s</i>"), "--");
     gtk_label_set_markup (GTK_LABEL(tmp_tasks_datas->cRun), job_run); 
     g_free (job_run);

     /* progress */
     gtk_label_set_markup (GTK_LABEL(tmp_tasks_datas->cProgress),
                           g_strdup_printf ("<b>%.1f%</b>", tmp_tasks_datas->progress));

     /* icon for group */
     if(tmp_tasks_datas->group>=0) {
        pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("groupedtsk.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, err);
        gtk_image_set_from_pixbuf (tmp_tasks_datas->cGroup, pix);
        g_object_unref (pix);
        g_object_set (tmp_tasks_datas->cadre, "margin-left", 32, NULL);
     }
     else {
        pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("ungroup.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, err);
        gtk_image_set_from_pixbuf (tmp_tasks_datas->cGroup, pix);
        g_object_unref (pix);
        g_object_set (tmp_tasks_datas->cadre, "margin-left", 4, NULL);
     }
     switch(tmp_tasks_datas->category) {
       case 0: {/* material */
         pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("material.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);         
         gtk_image_set_from_pixbuf (GTK_IMAGE(tmp_tasks_datas->cImgCateg), pix);
         g_object_unref (pix);
         break;
       }
       case 1: {/* intell */
         pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("intellectual.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);         
         gtk_image_set_from_pixbuf (GTK_IMAGE(tmp_tasks_datas->cImgCateg), pix);
         g_object_unref (pix);
         break;
       }
       case 2: {/* soc */
         pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("relational.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);         
         gtk_image_set_from_pixbuf (GTK_IMAGE(tmp_tasks_datas->cImgCateg), pix);
         g_object_unref (pix);
         break;
       }
     }/* end sw categ */
     /* icons - pty */
     switch(tmp_tasks_datas->priority) {
       case 0: {
          pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("priority_high.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);
          gtk_image_set_from_pixbuf (tmp_tasks_datas->cImgPty, pix);
          g_object_unref (pix);
          break;
       }
       case 1: {
          pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("priority_medium.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);
          gtk_image_set_from_pixbuf (tmp_tasks_datas->cImgPty, pix);
          g_object_unref (pix);
          break;
       }
       case 2: {
          pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("priority_low.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);
          gtk_image_set_from_pixbuf (tmp_tasks_datas->cImgPty, pix);
          g_object_unref (pix);
          break;
       }
     }/* end sw pty */
     /* icon - status */
     switch(tmp_tasks_datas->status) {
      case 0: {
          gtk_image_set_from_icon_name (tmp_tasks_datas->cImgStatus, "system-run",
                             GTK_ICON_SIZE_LARGE_TOOLBAR);
          break;
       }
       case 1: {
          gtk_image_set_from_icon_name (tmp_tasks_datas->cImgStatus, "emblem-default",
                             GTK_ICON_SIZE_LARGE_TOOLBAR);
          break;
       }
       case 2: {
          gtk_image_set_from_icon_name (tmp_tasks_datas->cImgStatus, "emblem-urgent",
                              GTK_ICON_SIZE_LARGE_TOOLBAR);
          break;
       }
       case 3: {
          gtk_image_set_from_icon_name (tmp_tasks_datas->cImgStatus, "appointment-new",
                              GTK_ICON_SIZE_LARGE_TOOLBAR);
          break;
       }
     }/* end sw status */
  }/* endif */
}

/*******************************************
  PUBLIC : new task
  if date == NULL, we use project's start
  date ; in other case (e.g. click on timeline)
  we use a computed date
***********************************************/
void tasks_new (GDate *date, APP_data *data)
{
  GtkWidget *dialog;
  gint ret, insertion_row, cmp;
  gdouble progress = 0;
  GdkRGBA color;
  GtkWidget *boxTasks, *pBtnColor, *entryName, *comboPriority, *comboCategory, *comboStatus, *spinProgress;
  gint curPty, curCateg, curStatus=0, curDuration = 0, curDays, curHours, curTemp, curGroup;
  GDate date_interval2, date_current, interval3, end_date;
  gchar *name = NULL, *startDate = NULL, *endDate = NULL, *limDate = NULL, *currentDate = NULL, *human_date, *str;
  tasks_data temp;
  const gchar *fooStr = _("Noname task");

  boxTasks = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasks"));

  dialog = tasks_create_dialog (date, FALSE, -1, data);

  ret = gtk_dialog_run (GTK_DIALOG(dialog));
  if(ret==1) {
    tasks_utils_reset_errors (data);
    /* yes, we want to add a new task */
    entryName = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryTaskName"));
    comboPriority = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxTasksPriority"));
    comboCategory = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxTasksCategory"));
    comboStatus = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxTasksStatus"));
    pBtnColor = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "colorbuttonTasks"));
    spinProgress = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonProgress"));

    GtkWidget *pButtonStart = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonStartDate")); 
    GtkWidget *pButton = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonTasksDueDate"));  
    GtkWidget *pComboDuration = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxtextDuration"));
    GtkWidget *pDays = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonDays"));
    GtkWidget *pHours = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHours"));
    GtkWidget *comboGroup = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboGroup"));
    /* we get various useful values */
    name = gtk_entry_get_text (GTK_ENTRY(entryName));
    if(strlen(name) == 0) {
       name = fooStr;
    }
    curPty = gtk_combo_box_get_active (GTK_COMBO_BOX(comboPriority));
    curCateg = gtk_combo_box_get_active (GTK_COMBO_BOX(comboCategory));
    curStatus = gtk_combo_box_get_active (GTK_COMBO_BOX(comboStatus));
    progress = gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinProgress));
    curGroup = gtk_combo_box_get_active (GTK_COMBO_BOX(comboGroup));
    /* test if progress = 100 % */
    if(progress == 100) {
       curStatus = TASKS_STATUS_DONE;
    }
    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(pBtnColor), &color);
    curDays = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(pDays));
    curHours = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(pHours));
    curDuration = gtk_combo_box_get_active (GTK_COMBO_BOX(pComboDuration));

    /* dates */
    if((curDays == 0) && (curHours == 0)) {
       curDays = 1;/* not enough security ! */
    }    
    currentDate = gtk_button_get_label (GTK_BUTTON(pButtonStart));
    g_date_set_parse (&date_current, currentDate);
    g_date_set_parse (&date_interval2, currentDate);
    if(curDays>1) {/* because when task last one day or less, start and date are identical */
        g_date_add_days (&date_interval2, curDays-1); 
    }
    /* we compute final date */
    if((curDuration == TASKS_DEADLINE) || (curDuration == TASKS_AFTER)) {
        limDate = gtk_button_get_label (GTK_BUTTON(pButton));
        g_date_set_parse (&interval3, limDate);
    }
    else {/* we use standard mode with choosen start date */
            limDate = gtk_button_get_label (GTK_BUTTON(pButtonStart));
            g_date_set_parse (&interval3, limDate);
    }

// TODO vérifier qu'en fn type de "duration" les durées et dates soient "possibles"

    human_date = misc_convert_date_to_friendly_str (&date_current);
    temp.type = TASKS_TYPE_TASK;
    temp.group = tasks_get_group_id (curGroup, data);
    temp.timer_val = 0;
    temp.location = NULL;
    str = g_strdup_printf (_("%s, %s"), human_date, misc_duration_to_friendly_str (curDays, curHours*60));
    GtkWidget *cadres = tasks_new_widget (name, g_strdup_printf (_("%s"), str), 
                              progress, 0, curPty, curCateg, color, curStatus, &temp, data);
    g_free (human_date);
    g_free (str);
    /* here, all widget pointers are stored in 'temp' */
    insertion_row = task_get_selected_row (data);
    if(insertion_row<0)
       insertion_row = tasks_datas_len (data);
    else
       insertion_row++;

    insertion_row = tasks_compute_valid_insertion_point (insertion_row, temp.group, FALSE, data);
    gtk_list_box_insert (GTK_LIST_BOX(boxTasks), GTK_WIDGET(cadres), insertion_row);   

    tasks_store_to_tmp_data (insertion_row, name, g_date_get_day (&date_current),
                             g_date_get_month (&date_current), g_date_get_year (&date_current), 
                             g_date_get_day (&date_interval2), g_date_get_month (&date_interval2), 
                             g_date_get_year (&date_interval2), 
                             g_date_get_day (&interval3), g_date_get_month (&interval3), g_date_get_year (&interval3),
                             progress, curPty, curCateg, curStatus, 
                            color, data, -1, TRUE, curDuration, curDays, curHours, 0, 0, 0, &temp);/* minutes = 0 calendar id = 0 */
    /* now, all values are stored in 'temp' */

    tasks_insert (insertion_row, data, &temp);/* here we store id */
    task_set_selected_row (insertion_row, data);
    tasks_autoscroll ((gdouble)insertion_row/g_list_length(data->tasksList), data);

    dashboard_add_task (insertion_row, data);
// TODO check ressources availabilities 

    /* now we store associations task<>ressources */
    tasks_update_links (GTK_WIDGET(dialog), temp.id, data);
    tasks_update_links_tasks (temp.id, dialog, data);
    gint t_links = links_check_task_linked (temp.id, data);
    /* if t_links >0 ther is at least one predecessor task */
    if(t_links>=0) {
         gint rc = tasks_compute_dates_for_linked_new_task (temp.id, curDuration, curDays, insertion_row, 1, 
                                                            &date_current, FALSE, FALSE, dialog, data);
    }/* endif links */
    /* we add ressources to display */
    tasks_add_display_ressources (temp.id, temp.cRscBox, data);
    /* we add ancestors tasks to display */
    tasks_add_display_tasks (temp.id, temp.cTaskBox, data);
    /* update group limits by comparison */
    tasks_update_group_limits (temp.id, -1, temp.group, data);/* we check only if we decided to link to an existing group */
    /* update timeline */
    timeline_remove_all_tasks (data);
    timeline_draw_all_tasks (data);

    /* undo engine - we just have to get a pointer on ID */
    undo_datas *tmp_value;
    tmp_value = g_malloc (sizeof(undo_datas));
    tmp_value->opCode = OP_INSERT_TASK;
    tmp_value->insertion_row = insertion_row;
    tmp_value->id = temp.id;
    tmp_value->datas = NULL;/* nothing to store ? */
    tmp_value->list = NULL;
    tmp_value->groups = NULL;
    tmp_value->nbTasks = 0;
    undo_push (CURRENT_STACK_TASKS, tmp_value, data);
    misc_display_app_status (TRUE, data);
  }
  home_project_tasks (data);
  home_project_milestones (data);
  g_object_unref (data->tmpBuilder);
  gtk_widget_destroy (GTK_WIDGET(dialog));
  tasks_free_list_groups (data);
}

/***************************************
  PUBLIC function - remove a Task datas 
  at a given position ---
***************************************/
void tasks_remove (gint position, GtkListBox *box, APP_data *data)
{
  GtkListBoxRow *row;
  GList *l, *l1;
  gint i, total = g_list_length (data->tasksList);
  GtkWidget *btnModif;
  tasks_data *tmp_tasks_datas, *tmp;
  GdkPixbuf *pix;
  GError *err = NULL;

  btnModif = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonTaskModify"));

  /* we get  pointer on the element */
  if(position < total) {
     l = g_list_nth (data->tasksList, position);
     tmp_tasks_datas = (tasks_data *)l->data;
     /* it's a groupe ? if YES, we also change group's references for tasks */
     if(tmp_tasks_datas->type == TASKS_TYPE_GROUP) {
        for(i=0;i<total;i++) {
           l1 = g_list_nth (data->tasksList, i);
           tmp = (tasks_data *)l1->data;
           if(tmp->group == tmp_tasks_datas->id) {
                 tmp->group = -1;
                 pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("ungroup.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, err);
                 gtk_image_set_from_pixbuf (tmp->cGroup, pix);
                 g_object_unref (pix);
                 g_object_set (tmp->cadre, "margin-left", 4, NULL);
           }
        }/* next i */
     }
     /* free strings */
     if(tmp_tasks_datas->name) {
         g_free (tmp_tasks_datas->name);
         tmp_tasks_datas->name=NULL;
     }

     if(tmp_tasks_datas->location) {
         g_free (tmp_tasks_datas->location);
         tmp_tasks_datas->location=NULL;
     }
     gint s_links = links_check_task_successors (tmp_tasks_datas->id, data);
     if(s_links>=0) {printf ("il y a des tâches à dé-linker \n");
         tasks_utils_remove_receivers (tmp_tasks_datas->id, data);
     }
     links_remove_all_task_links (tmp_tasks_datas->id, data);

     /* remove widget */
     row = gtk_list_box_get_row_at_index (box, position);
     /* timeline */
     gtk_container_remove (GTK_CONTAINER(box), gtk_widget_get_parent (tmp_tasks_datas->cadre));

     /* dashboard - check if it isn't already freed */
printf ("avant dash \n");
     if(tmp_tasks_datas->cDashboard)
        dashboard_remove_task (position, data);
printf ("apres name \n");
     GooCanvasItem *root = goo_canvas_get_root_item (GOO_CANVAS (data->canvasTL));
     if(tmp_tasks_datas->groupItems)
           goo_canvas_item_remove(tmp_tasks_datas->groupItems);

     /* free element */
     g_free (tmp_tasks_datas);
     /* update list */
     data->tasksList = g_list_delete_link (data->tasksList, l); 
  }/* endif */
  if(total<=0) 
        gtk_widget_set_sensitive (GTK_WIDGET(btnModif), FALSE);

}

/*****************************
  PUBLIC : remove all widgets
  inside a Gtkbox
  adapted from : https://stackoverflow.com/questions/9192223/remove-gtk-container-children-repopulate-it-then-refresh
*****************************/
void tasks_remove_widgets (GtkWidget *container)
{
  GList *children, *iter;

  children = gtk_container_get_children (GTK_CONTAINER(container));
  for(iter = children; iter != NULL; iter = g_list_next (iter)) {
      gtk_widget_destroy (GTK_WIDGET(iter->data));
  }
  g_list_free (children);
}

/*****************************
  repeat task on a week basis
*****************************/
void tasks_repeat_weekly (gint freq, gint limit, tasks_data *tmp, gint id_src, GtkWidget *win, GtkWidget *boxTasks, APP_data *data)
{
  GDate date_start, date_end, date_tmp, date_interval2;
  gint days, duration, i, count=0, insertion_row;
  GtkWidget *mon, *tue, *wed, *thu, *fri, *sat, *sun;
  gboolean flags[7];

  /* we get project ending date and current task (used as a basis) starting date */
  g_date_set_dmy (&date_start, tmp->start_nthDay, tmp->start_nthMonth, tmp->start_nthYear);
  g_date_set_dmy (&date_end, data->properties.end_nthDay, data->properties.end_nthMonth, data->properties.end_nthYear);
  /* we clamp date to nextprevious  Monday */
  while(g_date_get_weekday (&date_start)!=G_DATE_MONDAY) {
       g_date_subtract_days (&date_start, 1);
  }
  g_date_add_days (&date_start, 7*freq); /* we jump to 'freq' week(s) ahead, at Monday */
  duration = tmp->days;
  if(duration<1)
     duration = 1;
  /* if new date>project ending date, we exit */
  days = g_date_days_between (&date_start, &date_end);
  if(days<=0) {
     task_repeat_error_dialog (win);
     return;
  }
  /* here date_start points to the monday of the FIRST week requested to repeating, w+1, w+2 and so on */
  /* we get useful pointers */
  mon = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkMo"));
  tue = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkTu"));
  wed = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkWe"));
  thu = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkTh"));
  fri = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkFr"));
  sat = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkSa"));
  sun = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkSu"));
  flags[0] = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(mon));
  flags[1] = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(tue));
  flags[2] = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(wed));
  flags[3] = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(thu));
  flags[4] = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(fri));
  flags[5] = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(sat));
  flags[6] = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(sun));

  /* compute insertion point */
  insertion_row = tasks_get_insertion_line (data);
  /* now we check if current date + task duration if before project's ending date */
  while(g_date_days_between (&date_start, &date_end)>=duration && count!=limit) {/* trick if limit ==-1, NO limit */
     /* we can test if we can add new tasks, for every choosen day of week */
     /* local copy of monday's current date */
     g_date_set_dmy (&date_tmp, g_date_get_day (&date_start), g_date_get_month (&date_start), g_date_get_year (&date_start));
     /* we advance as day is active */
     for(i=0;i<7;i++) {
        if(flags[i]) {
           if(g_date_days_between (&date_tmp, &date_end)>=duration) {
               g_date_set_dmy (&date_interval2, g_date_get_day (&date_tmp), 
                                  g_date_get_month (&date_tmp), g_date_get_year (&date_tmp));
               g_date_add_days (&date_interval2, duration-1);/* now we get final date */
               gchar *human_date = misc_convert_date_to_friendly_str (&date_tmp);
               GtkWidget *cadres = tasks_new_widget (tmp->name, 
                              g_strdup_printf (_("%s, %d day(s)"), human_date, duration), 
                              0, 0, tmp->priority, tmp->category, tmp->color, tmp->status, tmp, data);
               g_free (human_date);

               gtk_list_box_insert (GTK_LIST_BOX(boxTasks), GTK_WIDGET(cadres), insertion_row);
               tasks_store_to_tmp_data (insertion_row, tmp->name, g_date_get_day (&date_tmp),
                            g_date_get_month (&date_tmp), g_date_get_year (&date_tmp), 
                            g_date_get_day (&date_interval2), g_date_get_month (&date_interval2), 
                            g_date_get_year (&date_interval2),
                            -1, -1, -1, 
                            0, tmp->priority, tmp->category, tmp->status, 
                            tmp->color, data, -1, TRUE, TASKS_AS_SOON, duration, 0, 0, 0, 0, tmp);/* hours = 0, mins = 0, calendar id = 0 */
               /* we must clone list of ressources used */
               tasks_insert (insertion_row, data, tmp);/* here we store new id contained in tmp */
               /* duplication of ressources used from source task */
               links_copy_rsc_usage (id_src, tmp->id, date_tmp, date_interval2, data);
                  
               tasks_add_display_ressources (tmp->id, tmp->cRscBox, data);
               dashboard_add_task (insertion_row, data);
               /* undo engine - we just have to get a pointer on ID */
               undo_datas *tmp_value;
               tmp_value = g_malloc (sizeof(undo_datas));
               tmp_value->opCode = OP_REPEAT_TASK;
               tmp_value->insertion_row = insertion_row;
               tmp_value->id = tmp->id;
               tmp_value->datas = NULL;/* nothing to store ? */
               tmp_value->list = NULL;
               tmp_value->groups = NULL;
               undo_push (CURRENT_STACK_TASKS, tmp_value, data);

               insertion_row++;
           }/* endif dates */
        }/* endif flags */
        g_date_add_days (&date_tmp, 1);/* we do a loop for every days of week */
    }/* next i */
    g_date_add_days (&date_start, 7*freq);
    count++;
  }/* wend */
}

/*****************************
  repeat task on a month basis
*****************************/
void tasks_repeat_monthly (gint freq, gint limit, tasks_data *tmp, gint id_src, GtkWidget *win, GtkWidget *boxTasks, APP_data *data)
{
  gboolean flags[31];
  GDate date_start, date_end, date_tmp, date_interval2;
  gint i, days, duration, count=0, insertion_row;
  gchar *tmpStr;
  GtkWidget *wDay;

  /* we get project ending date and current task (used as a basis) starting date */
  g_date_set_dmy (&date_start, 1, tmp->start_nthMonth, tmp->start_nthYear);
  g_date_set_dmy (&date_end, data->properties.end_nthDay, data->properties.end_nthMonth, data->properties.end_nthYear);
  /* we jump to next month desired month, in account of freq, day ONE */
  duration = tmp->days;
  if(duration<1)
     duration = 1;
  for(i=0;i<freq;i++) {
     g_date_add_months (&date_start, 1);
  }
  /* if new date>project ending date, we exit */
  days = g_date_days_between (&date_start, &date_end);
  if(days<=0) {
     task_repeat_error_dialog (win);
     return;
  }
  /* if, for example, the frequency is 2, date_start points on month+2, day ONE of month */
  /* we fill the array */
  for(i=0;i<31;i++) {
     tmpStr = g_strdup_printf ("check%d", i+1);
     wDay = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, tmpStr));
     flags[i] = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(wDay));
     g_free (tmpStr);
  }/* next i */
  /* compute insertion point */
  insertion_row = tasks_get_insertion_line (data);
  /* now we check if current date + task duration if before project's ending date */
  while(g_date_days_between (&date_start, &date_end)>=duration && count!=limit) {/* trick if limit ==-1, NO limit */
     /* we can test if we can add new tasks, for every choosen day of week */
     /* local copy of day ONE's current date */
     g_date_set_dmy (&date_tmp, g_date_get_day (&date_start), g_date_get_month (&date_start), g_date_get_year (&date_start));
     /* we advance as day is active */
     for(i=0;i<31;i++) {
        if(flags[i]) {
          if(g_date_days_between (&date_tmp, &date_end)>=duration) {
            /* very simple test to avoid checking 28, 29, 30 days months */
            if(g_date_get_day (&date_tmp)==(i+1)) {  
               g_date_set_dmy (&date_interval2, g_date_get_day (&date_tmp), 
                                  g_date_get_month (&date_tmp), g_date_get_year (&date_tmp));
               g_date_add_days (&date_interval2, duration-1);/* now we get final date */
               gchar *human_date = misc_convert_date_to_friendly_str (&date_tmp);
               GtkWidget *cadres = tasks_new_widget (tmp->name, 
                              g_strdup_printf (_("%s, %d day(s)"), human_date, duration), 
                              0, 0, tmp->priority, tmp->category, tmp->color, tmp->status, tmp, data);
               g_free (human_date);

               gtk_list_box_insert (GTK_LIST_BOX(boxTasks), GTK_WIDGET(cadres), insertion_row);
               tasks_store_to_tmp_data (insertion_row, tmp->name, g_date_get_day (&date_tmp),
                            g_date_get_month (&date_tmp), g_date_get_year (&date_tmp), 
                            g_date_get_day (&date_interval2), g_date_get_month (&date_interval2), 
                            g_date_get_year (&date_interval2),
                            -1, -1, -1, 
                            0, tmp->priority, tmp->category, tmp->status, 
                            tmp->color, data, -1, TRUE, TASKS_AS_SOON, duration, 0, 0, 0, 0, tmp);/* hours = 0, mins = 0calendar id = 0 */
               /* we must clone list of ressources used */
               tasks_insert (insertion_row, data, tmp);/* here we store new id contained in tmp */
               /* duplication of ressources used from source task */
               links_copy_rsc_usage (id_src, tmp->id, date_tmp, date_interval2, data);
                  
               tasks_add_display_ressources (tmp->id, tmp->cRscBox, data);
               dashboard_add_task (insertion_row, data);
               /* undo engine - we just have to get a pointer on ID */
               undo_datas *tmp_value;
               tmp_value = g_malloc (sizeof(undo_datas));
               tmp_value->opCode = OP_REPEAT_TASK;
               tmp_value->insertion_row = insertion_row;
               tmp_value->id = tmp->id;
               tmp_value->datas = NULL;/* nothing to store ? */
               tmp_value->list = NULL;
               tmp_value->groups = NULL;
               undo_push (CURRENT_STACK_TASKS, tmp_value, data);

               insertion_row++;
            }/* endif valid nth of day */
          }/* endif date */
        }/* endif flags */
        g_date_add_days (&date_tmp, 1);/* we do a loop for every days of week */
     }/* next i */
     g_date_add_months (&date_start, freq);
     count++;
  }/* wend */
}

/*****************************
  repeat task on a year basis
*****************************/
void tasks_repeat_yearly (gint freq, gint limit, tasks_data *tmp, gint id_src, GtkWidget *win, GtkWidget *boxTasks, APP_data *data)
{
  gboolean flags[31];
  GDate date_start, date_end, date_tmp, date_interval2;
  gint i, days, month, duration, count=0, insertion_row;
  gchar *tmpStr;
  GtkWidget *wDay, *wMonth;

  /* we get project ending date and current task (used as a basis) starting date */
  g_date_set_dmy (&date_start, 1, 1, tmp->start_nthYear);
  g_date_set_dmy (&date_end, data->properties.end_nthDay, data->properties.end_nthMonth, data->properties.end_nthYear);
  /* we jump to next year desired year, in account of freq, January 1 */
  duration = tmp->days;
  if(duration<1)
     duration = 1;
  for(i=0;i<freq;i++) {
     g_date_add_years (&date_start, 1);
  }
  /* if new date>project ending date, we exit */
  days = g_date_days_between (&date_start, &date_end);
  if(days<=0) {
     task_repeat_error_dialog (win);
     return;
  }
  /* if, for example, the frequency is 2, date_start points on year+2, day January 1 */
  /* we fill the array */
  for(i=0;i<31;i++) {
     tmpStr = g_strdup_printf ("check%d", i+1);
     wDay = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, tmpStr));
     flags[i] = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(wDay));
     g_free (tmpStr);
  }/* next i */
  /* we read choosen month */
  wMonth = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboMonth"));
  month = gtk_combo_box_get_active (GTK_COMBO_BOX(wMonth))+1;
  /* compute insertion point */
  insertion_row = tasks_get_insertion_line (data);
  /* now we check if current date + task duration if before project's ending date */
  while(g_date_days_between (&date_start, &date_end)>=duration && count!=limit) {/* trick if limit ==-1, NO limit */
     /* we can test if we can add new tasks, for every choosen day of week */
     /* local copy of day ONE's current date */
     g_date_set_dmy (&date_tmp, 1, month, g_date_get_year (&date_start));
     /* we advance as day is active */
     for(i=0;i<31;i++) {
        if(flags[i]) {
          if(g_date_days_between (&date_tmp, &date_end)>=duration) {
            /* very simple test to avoid checking 28, 29, 30 days months */
            if(g_date_get_day (&date_tmp)==(i+1)) {  
               g_date_set_dmy (&date_interval2, g_date_get_day (&date_tmp), 
                                  g_date_get_month (&date_tmp), g_date_get_year (&date_tmp));
               g_date_add_days (&date_interval2, duration-1);/* now we get final date */
               gchar *human_date = misc_convert_date_to_friendly_str (&date_tmp);
               GtkWidget *cadres = tasks_new_widget (tmp->name, 
                              g_strdup_printf (_("%s, %d day(s)"), human_date, duration), 
                              0, 0, tmp->priority, tmp->category, tmp->color, tmp->status, tmp, data);
               g_free (human_date);

               gtk_list_box_insert (GTK_LIST_BOX(boxTasks), GTK_WIDGET(cadres), insertion_row);
               tasks_store_to_tmp_data (insertion_row, tmp->name, g_date_get_day (&date_tmp),
                            g_date_get_month (&date_tmp), g_date_get_year (&date_tmp), 
                            g_date_get_day (&date_interval2), g_date_get_month (&date_interval2), 
                            g_date_get_year (&date_interval2),
                            -1, -1, -1, 
                            0, tmp->priority, tmp->category, tmp->status, 
                            tmp->color, data, -1, TRUE, TASKS_AS_SOON, duration, 0, 0, 0, 0, tmp);/* hours = 0, mins calendar id = 0 */
               /* we must clone list of ressources used */
               tasks_insert (insertion_row, data, tmp);/* here we store new id contained in tmp */
               /* duplication of ressources used from source task */
               links_copy_rsc_usage (id_src, tmp->id, date_tmp, date_interval2, data);
                  
               tasks_add_display_ressources (tmp->id, tmp->cRscBox, data);
               dashboard_add_task (insertion_row, data);
               /* undo engine - we just have to get a pointer on ID */
               undo_datas *tmp_value;
               tmp_value = g_malloc (sizeof(undo_datas));
               tmp_value->opCode = OP_REPEAT_TASK;
               tmp_value->insertion_row = insertion_row;
               tmp_value->id = tmp->id;
               tmp_value->datas = NULL;/* nothing to store ? */
               tmp_value->list = NULL;
               tmp_value->groups = NULL;
               undo_push (CURRENT_STACK_TASKS, tmp_value, data);

               insertion_row++;
            }/* endif valid nth of day */
          }/* endif date */
        }/* endif flags */
        g_date_add_days (&date_tmp, 1);/* we do a loop for every days of week */
     }/* next i */
     g_date_add_years (&date_start, freq);
     count++;
  }/* wend */
}

/*************************
  Public :  repeat current
  task
*************************/
void tasks_repeat (APP_data *data)
{
  GtkWidget *dialog;
  gint ret=0, iRow, active, freq, limit;
  GtkWidget *boxTasks, *row, *comboRepMode, *spinbuttonFreq, *checkLimit, *spinbuttonTimes;
  tasks_data temp;
  tasks_data *tmp_tasks_datas;
  GDate date;
  GList *l;

  boxTasks = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasks"));
  row = gtk_list_box_get_selected_row (boxTasks);
  iRow  = gtk_list_box_row_get_index (row);
  data->curTasks = iRow;

  /* we read true data in memory */
  tasks_get_ressource_datas (iRow, &temp, data);
  if(temp.type == TASKS_TYPE_TASK) {
      g_date_set_dmy (&date, temp.start_nthDay, temp.start_nthMonth, temp.start_nthYear);

      dialog = tasks_create_repeat_dialog (temp.name, misc_convert_date_to_friendly_str (&date), data);
      ret = gtk_dialog_run (GTK_DIALOG(dialog));
      if(ret==1) {
          comboRepMode = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboRepMode"));
          spinbuttonFreq = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonFreq"));
          checkLimit = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkLimits"));
          spinbuttonTimes = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonTimes"));
          active = gtk_combo_box_get_active (GTK_COMBO_BOX(comboRepMode));
          freq = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonFreq));
          limit = -1;
          /* we must avoid any links to predecessor Tasks */
          temp.task_prec = NULL;
          temp.type = TASKS_TYPE_TASK;
          temp.group = -1;
          temp.timer_val = 0;
          temp.location = NULL;/* OK car on passe juste des pteurs dans get_ress */
          /* we get pointer on the element */
          l = g_list_nth (data->tasksList, iRow);
          tmp_tasks_datas = (tasks_data *)l->data;
    
          if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkLimit)))
               limit = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinbuttonTimes));
          switch(active) {
	     case 0:{/* weekly */
               tasks_repeat_weekly (freq, limit, &temp, tmp_tasks_datas->id, dialog, boxTasks, data);
	       break;
	     }
	     case 1:{/* monthly */
               tasks_repeat_monthly (freq, limit, &temp, tmp_tasks_datas->id, dialog, boxTasks, data);
	       break;
	     }
	     case 2:{/* yearly */
               tasks_repeat_yearly (freq, limit, &temp, tmp_tasks_datas->id, dialog, boxTasks, data);
	       break;
	     }
          }/* end switch */
          /* update timeline */
          timeline_remove_all_tasks (data);
          timeline_draw_all_tasks (data);
          misc_display_app_status (TRUE, data);
          home_project_tasks (data);
      }/* endif ret == 1 */
  g_object_unref (data->tmpBuilder);
  gtk_widget_destroy (GTK_WIDGET(dialog));
  }
}
  
/*************************
  Public :  modify current
  task
*************************/
void tasks_modify (APP_data *data)
{
  GtkWidget *dialog;
  gint ret, insertion_row, curPty, curCateg, curStatus=0, iRow=-1, curDays, curHours, curDuration, curGroup, cmp;
  gint t_links, s_links, rc;
  gdouble progress=0;
  GdkRGBA color;
  GtkWidget *boxTasks, *row, *pBtnColor, *pButtonStart, *pButton, *pComboDuration, *entryLocation, *comboGroup;
  GtkWidget *entryName, *comboPriority, *comboCategory, *comboStatus, *spinProgress, *pDays, *pHours;
  GDate interval1, interval2, interval3, end_date;
  gchar *name=NULL, *startDate=NULL, *endDate=NULL, *limDate=NULL, *location = NULL, *human_date = NULL;
  const gchar *nonameGroup = _("Nonane group");
  const gchar *nonameMileStone = _("Nonane milestone");
  const gchar *fooStr = _("Noname task");
  const gchar *fooStr2 = _("Unknown location");
  tasks_data temp, newTasks, *undoTasks;
  undo_datas *tmp_value;

  boxTasks = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasks"));
  row = gtk_list_box_get_selected_row (boxTasks);
  iRow  = gtk_list_box_row_get_index (row);
  data->curTasks = iRow;

  /* we read current datas in memory */
  tasks_get_ressource_datas (iRow, &temp, data);
  /* undo engine, part 1 */
  undoTasks = g_malloc (sizeof(tasks_data));
  tasks_get_ressource_datas (iRow, undoTasks, data);
  /* build strings in order to keep "true" values */
  undoTasks->name = NULL;
  undoTasks->location = NULL;
  if(temp.name)
     undoTasks->name = g_strdup_printf ("%s", temp.name);
  if(temp.location) {printf ("loc non nulle \n");
     undoTasks->location = g_strdup_printf ("%s", temp.location);
  }

  if(temp.type == TASKS_TYPE_TASK) {
      dialog = tasks_create_dialog (NULL, TRUE, temp.id, data);
      /* we get access to widgets and modify them */
      entryName = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryTaskName"));
      comboPriority = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxTasksPriority"));
      comboCategory = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxTasksCategory"));
      comboStatus = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxTasksStatus"));
      pBtnColor = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "colorbuttonTasks"));
      spinProgress = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonProgress"));
      pButtonStart = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonStartDate")); 
      pButton = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonTasksDueDate")); 
      pComboDuration = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxtextDuration"));
      pDays = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonDays"));
      pHours = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonHours"));
      comboGroup = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboGroup"));
      curGroup = tasks_get_group_rank (temp.group, data);
      gtk_combo_box_set_active (GTK_COMBO_BOX(comboGroup), curGroup);
      /* we modify displayed datas according to our database */
      gtk_entry_set_text (GTK_ENTRY(entryName), temp.name); 
      gtk_combo_box_set_active (GTK_COMBO_BOX(comboPriority), temp.priority);
      gtk_combo_box_set_active (GTK_COMBO_BOX(comboCategory), temp.category);
      gtk_combo_box_set_active (GTK_COMBO_BOX(comboStatus), temp.status);
      gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER(pBtnColor), &temp.color);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinProgress), temp.progress);

      gtk_spin_button_set_value (GTK_SPIN_BUTTON(pDays), temp.days);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(pHours), temp.hours);

      gtk_combo_box_set_active (GTK_COMBO_BOX(pComboDuration), temp.durationMode);
      if((temp.durationMode != TASKS_DEADLINE) && (temp.durationMode != TASKS_AFTER)) 
          gtk_widget_set_sensitive (GTK_WIDGET (pButton), FALSE);
      else
          gtk_widget_set_sensitive (GTK_WIDGET (pButton), TRUE);
      /*  dates */ 
      startDate = misc_convert_date_to_str (temp.start_nthDay, temp.start_nthMonth, temp.start_nthYear);
      endDate = misc_convert_date_to_str (temp.end_nthDay, temp.end_nthMonth, temp.end_nthYear);

      gtk_button_set_label (GTK_BUTTON(pButtonStart), startDate);
      if((temp.durationMode == TASKS_DEADLINE)  || (temp.durationMode == TASKS_AFTER)) {
        limDate = misc_convert_date_to_str (temp.lim_nthDay, temp.lim_nthMonth, temp.lim_nthYear);
      }
      else {
        limDate = misc_convert_date_to_str (temp.start_nthDay, temp.start_nthMonth, temp.start_nthYear);
      }

printf ("lim date =%s \n", limDate);
      gtk_button_set_label (GTK_BUTTON(pButton), limDate);

      g_free (startDate);
      g_free (endDate);
      g_free (limDate);
  }
  else {
     if(temp.type == TASKS_TYPE_GROUP) {
        dialog = tasks_create_group_dialog (TRUE, temp.id, data);
        entryName = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryGroupName"));
        gtk_entry_set_text (GTK_ENTRY(entryName), temp.name); 
     }
     else {/* milestone */
        dialog = tasks_create_milestone_dialog (TRUE, temp.id, data);
        entryName = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryMilestoneName"));
        pButtonStart = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonDate"));
        entryLocation = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryMilestoneLocation"));
        gtk_entry_set_text (GTK_ENTRY(entryName), temp.name);
        startDate = misc_convert_date_to_str (temp.start_nthDay, temp.start_nthMonth, temp.start_nthYear);
        gtk_button_set_label (GTK_BUTTON(pButtonStart), startDate);
        g_free (startDate);
        if(temp.location)
           gtk_entry_set_text (GTK_ENTRY(entryLocation), temp.location);
     }
  }

  ret = gtk_dialog_run (GTK_DIALOG(dialog));
  /* code 2 = remove , code 1 = OK */
  if(ret<=0) {/* cancel, mandatory in order to free unused undo datas -ret == -4 if user closes window */
      if(undoTasks->name)
         g_free (undoTasks->name);
      if(undoTasks->location)
         g_free (undoTasks->location);
      g_free (undoTasks);
  }
  if(ret==2) {/* remove */  // Gtk >=3.14 : gtk_list_box_unselect_all (box);  
      /* undo engine - part 2 of 3 - we just have to get a pointer on ID */
      tmp_value = g_malloc (sizeof(undo_datas));
      tmp_value->insertion_row = iRow;
      tmp_value->new_row = iRow;
      tmp_value->id = temp.id;
      tmp_value->type_tsk = temp.type;
      tmp_value->datas = undoTasks;
      tmp_value->groups = NULL;
      tmp_value->nbTasks = 0;
      /* duplicate links */
      tmp_value->list = links_copy_all (data);
      if(undoTasks->type == TASKS_TYPE_TASK) {
          tmp_value->opCode = OP_REMOVE_TASK;
      }
      if(undoTasks->type == TASKS_TYPE_GROUP) {
          tmp_value->opCode = OP_REMOVE_GROUP;
          /* now we build a GList of tasks using this group */
          tmp_value->groups = tasks_fill_list_groups_for_tasks (temp.id, data);
      }
      if(undoTasks->type == TASKS_TYPE_MILESTONE) {
          tmp_value->opCode = OP_REMOVE_MILESTONE;
      }
      undo_push (CURRENT_STACK_TASKS, tmp_value, data);

      tasks_remove (iRow, boxTasks, data);  /* dashboard is managed inside - common for tasks, groups an milestones */
      /* update timeline - please note, the task is removed by 'task_remove()' function */
      timeline_remove_all_tasks (data);/* YES, for other tasks */
      timeline_draw_all_tasks (data);
      /* we update assignments */
      assign_remove_all_tasks (data);
      assign_draw_all_visuals (data);
      report_clear_text (data->buffer, "left");
      misc_display_app_status (TRUE, data);
  }
  if(ret==1) {/* we get various useful values */
    tasks_utils_reset_errors (data);
    /* undo engine - part 2 of 3 - we just have to get a pointer on ID */
    tmp_value = g_malloc (sizeof(undo_datas));
    tmp_value->insertion_row = iRow;
    tmp_value->new_row = iRow;
    tmp_value->id = temp.id;
    tmp_value->datas = undoTasks;
    tmp_value->groups = NULL;
    /* duplicate links */
    tmp_value->list = links_copy_all (data);

    name = gtk_entry_get_text (GTK_ENTRY(entryName));
    /* short case : Groups */
    if(temp.type == TASKS_TYPE_GROUP) {
        if(strlen (name)==0) {
           name = nonameGroup;
        }    
        newTasks.name = name;
        newTasks.location = NULL;
        newTasks.group = -1;
        /* dates */
	newTasks.start_nthDay = temp.start_nthDay;
	newTasks.start_nthMonth = temp.start_nthMonth;
	newTasks.start_nthYear = temp.start_nthYear;
	newTasks.end_nthDay = temp.end_nthDay;
	newTasks.end_nthMonth = temp.end_nthMonth;
	newTasks.end_nthYear = temp.end_nthYear;
        /* duration evaluation - TODO : true group duration */
        g_date_set_dmy (&interval1, newTasks.start_nthDay, newTasks.start_nthMonth, newTasks.start_nthYear);
        g_date_set_dmy (&interval2, newTasks.end_nthDay, newTasks.end_nthMonth, newTasks.end_nthYear);
        newTasks.days = g_date_days_between (&interval1, &interval2)+1;

        newTasks.calendar = 0; /* TODO : version 0.2, variable calendar */
        /* we modify datas in memory */
        tasks_modify_datas (iRow, data, &newTasks);
        tasks_group_modify_widget (iRow, data);
        tmp_value->opCode = OP_MODIFY_GROUP;
        tmp_value->nbTasks = 0;
        undo_push (CURRENT_STACK_TASKS, tmp_value, data);
    }/* endif groups */

    if(temp.type == TASKS_TYPE_MILESTONE) {
        if(strlen(name)==0) {
           name = nonameMileStone;
        }    
        newTasks.name = name;
        newTasks.calendar = 0;
        newTasks.group = -1;
        newTasks.days = 0;
        newTasks.hours = 0;
        location = gtk_entry_get_text (GTK_ENTRY(entryLocation));
        if(strlen(location)==0) {
           location = fooStr2;
        }
        newTasks.location = location;
        /* dates */
        startDate = gtk_button_get_label (GTK_BUTTON(pButtonStart));
        g_date_set_parse (&interval1, startDate);
        newTasks.start_nthDay = g_date_get_day (&interval1);
        newTasks.start_nthMonth = g_date_get_month (&interval1);
        newTasks.start_nthYear = g_date_get_year (&interval1);
        newTasks.end_nthDay = g_date_get_day (&interval1);
        newTasks.end_nthMonth = g_date_get_month (&interval1);
        newTasks.end_nthYear = g_date_get_year (&interval1);
        /* we modify datas in memory */
        tasks_modify_datas (iRow, data, &newTasks);
        tasks_milestone_modify_widget (iRow, data);
        /* we update links */
        tasks_update_links_tasks (temp.id, dialog, data);
        /* if t_links >0 ther is at least one predecessor task */
        t_links = links_check_task_linked (temp.id, data);
        s_links = links_check_task_successors (temp.id, data);
        if(t_links>=0 || s_links>=0) {
           rc = tasks_compute_dates_for_all_linked_task (temp.id, TASKS_AS_SOON, 1, iRow, 
                                                                    &interval1, t_links, s_links, TRUE, dialog, data);
        }/* endif links */
        /* undo */
        tmp_value->nbTasks = 0;
        tmp_value->opCode = OP_MODIFY_MILESTONE;
        undo_push (CURRENT_STACK_TASKS, tmp_value, data);
    }/* endif milestone */

    if(temp.type == TASKS_TYPE_TASK) {    
        if(strlen (name)==0) {
           name = fooStr;
        }
        curPty = gtk_combo_box_get_active (GTK_COMBO_BOX(comboPriority));
        curCateg = gtk_combo_box_get_active (GTK_COMBO_BOX(comboCategory));
        curStatus = gtk_combo_box_get_active (GTK_COMBO_BOX(comboStatus));
        progress =  gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinProgress));
        gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(pBtnColor), &color);
        curGroup = gtk_combo_box_get_active (GTK_COMBO_BOX(comboGroup));
        curDays = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(pDays));
        curHours = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(pHours));
        curDuration = gtk_combo_box_get_active (GTK_COMBO_BOX(pComboDuration));
        if((curDays==0) && (curHours ==0))
           curDays = 1;
        /* if curDuration different of default value, we read button's date */
        if((curDuration == TASKS_DEADLINE) || (curDuration == TASKS_AFTER)) {
            limDate = gtk_button_get_label (GTK_BUTTON(pButton));
            g_date_set_parse (&interval3, limDate);
            startDate = gtk_button_get_label (GTK_BUTTON(pButtonStart));
            g_date_set_parse (&interval1, startDate);
        }
        else {/* we use standard mode with choosen start date */
            startDate = gtk_button_get_label (GTK_BUTTON(pButtonStart));
            g_date_set_parse (&interval1, startDate);

        }
        /* we store datas - we use temporary variable */
        newTasks.name = name;
        newTasks.location = NULL;
        newTasks.group = tasks_get_group_id (curGroup, data);/* previous value in temp.group */
        newTasks.color = color;
        newTasks.priority = curPty;
        newTasks.category = curCateg;
        newTasks.progress = progress;
        /* test if progress = 100 % */
        if(progress == 100) {
            curStatus = TASKS_STATUS_DONE;
        }
        newTasks.status = curStatus; 
        newTasks.days = curDays;
        newTasks.hours = curHours;
        newTasks.durationMode = curDuration;

        /* dates */
        startDate = gtk_button_get_label (GTK_BUTTON(pButtonStart));
        endDate = gtk_button_get_label (GTK_BUTTON(pButton));
        g_date_set_parse (&interval1, startDate);
        g_date_set_parse (&interval2, startDate);
        if(curDays>1) {/* because when task last one day or less, start and date are identical */
            g_date_add_days (&interval2, curDays-1); 
        }
        newTasks.start_nthDay = g_date_get_day (&interval1);
        newTasks.start_nthMonth = g_date_get_month (&interval1);
        newTasks.start_nthYear = g_date_get_year (&interval1);

        newTasks.end_nthDay = g_date_get_day (&interval2);
        newTasks.end_nthMonth = g_date_get_month (&interval2);
        newTasks.end_nthYear = g_date_get_year (&interval2);

        if((curDuration == TASKS_DEADLINE)  || (curDuration == TASKS_AFTER)) {
          newTasks.lim_nthDay = g_date_get_day (&interval3);
          newTasks.lim_nthMonth = g_date_get_month (&interval3);
          newTasks.lim_nthYear = g_date_get_year (&interval3);
        }
        else {
          newTasks.lim_nthDay = -1;
          newTasks.lim_nthMonth = -1;
          newTasks.lim_nthYear = -1;
        }
        /*  dashboard - yes, removing before other oprations */
        dashboard_remove_task (iRow, data);
        /* we modify datas in memory */
        newTasks.calendar = 0; /* TODO : version 0.2, enable variable calendar */
        tasks_modify_datas (iRow, data, &newTasks);
        tasks_modify_widget (iRow, data); 
        /*  dashboard */
        dashboard_add_task (iRow, data);
        /* we update links */
        tasks_update_links (GTK_WIDGET(dialog), temp.id, data);
        tasks_update_links_tasks (temp.id, dialog, data);

        /* update dates according to due date mode */
        rc = 0;
	    t_links = links_check_task_linked (temp.id, data);
        s_links = links_check_task_successors (temp.id, data);
		/* if t_links >0 ther is at least one predecessor task */
		if(t_links>=0 || s_links>=0) {// TODO change to wider function !!! 
				rc = tasks_compute_dates_for_all_linked_task (temp.id, curDuration, curDays, iRow, 
																		&interval1, t_links, s_links, TRUE, dialog, data);
		}/* endif links */

        /* ressources - first we remove all children from rscBox */
        tasks_remove_widgets (temp.cRscBox);
        tasks_remove_widgets (temp.cTaskBox);
        /* and update it */
        tasks_add_display_ressources (temp.id, temp.cRscBox, data);
        tasks_add_display_tasks (temp.id, temp.cTaskBox, data);
        /* we look for successors in case of name's change or so on in order to update widgets */
        if(s_links>0) {
           /* undo tasks->name contient l'ancien le nouveau newTasks.name*/
           if(g_strcmp0 (undoTasks->name, newTasks.name)!=0) {
               tasks_utils_update_receivers (temp.id, data);
           }
        }
        /* if group has changed, we shall 'move' widget */
        if(temp.group != newTasks.group) {
           insertion_row = task_get_selected_row (data);
           gint newRow = tasks_compute_valid_insertion_point (insertion_row, newTasks.group, FALSE, data);
           tasks_move (insertion_row, newRow, FALSE, FALSE, data);
           tmp_value->new_row = newRow;
        }
        /* undo last part */
printf ("retour i.e. total done vaut =%d \n", rc);
        tmp_value->opCode = OP_MODIFY_TASK;
        tmp_value->nbTasks = rc;
        undo_push (CURRENT_STACK_TASKS, tmp_value, data);

        /* update group limits by comparison */
        tasks_update_group_limits (temp.id, temp.group, newTasks.group, data);
    }/* elseif task std */
    /* we update assignments */
    assign_remove_all_tasks (data);
    assign_draw_all_visuals (data);
    /* clear report */
    report_clear_text (data->buffer, "left");
    /* update timeline */
    timeline_remove_all_tasks (data);/* YES, for other tasks */
    timeline_draw_all_tasks (data);

    misc_display_app_status (TRUE, data);
  }
  home_project_tasks (data);
  home_project_milestones (data);
  g_object_unref (data->tmpBuilder);
  gtk_widget_destroy (GTK_WIDGET(dialog));
  tasks_free_list_groups (data);
}

/***********************************
  remove all row of the current
  GtkListbox containing tasks
***********************************/
void tasks_remove_all_rows (APP_data *data)
{
   gint total, i;
   GtkListBox *box; 

   total = g_list_length (data->tasksList);
   box = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasks")); 

   for(i=total; i>0; i--) {
      tasks_remove (0, box, data);/* yes, always the FIRST */        
   }
}

/*******************************************
  PUBLIC : move visually up or down a group
  arguments ! group for group ID
  pos, dest start and destination of moving
*******************************************/
void tasks_group_move (gint group, gint pos, gint dest, gboolean fUndo, APP_data *data)
{
  gint i, nb_tasks, nearest_group, true_dest;
  GtkListBox *box; 
  GtkListBoxRow *row;
  undo_datas *tmp_value;
  tasks_data *undoTasks;

  if(dest==pos)
     return;

  /* we search last standard task belonging to current group */
  i = pos+1;
  while((i<g_list_length (data->tasksList)) && (tasks_get_group (i, data)==group)) {
     i++;
  }
  nb_tasks = i-pos-1;
printf("position groupe=%d nbre tâches =%d  \n", pos, nb_tasks);
  box = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasks"));

  if(fUndo) {
   /* undo engine - part 2 of 3 - we just have to get a pointer on ID */
    undoTasks = g_malloc (sizeof(tasks_data));
    tasks_get_ressource_datas (pos, undoTasks, data);
    undoTasks->name = NULL;/* useless */
    undoTasks->location = NULL;/* useless */
    tmp_value = g_malloc (sizeof(undo_datas));
    tmp_value->insertion_row = pos;
    tmp_value->id = group;
    tmp_value->nbTasks = nb_tasks;
    tmp_value->datas = undoTasks;
    tmp_value->groups = NULL;
    tmp_value->list = NULL;
  }

  if(dest<pos) {/* move Up */
     /* we look for nearest group backward */
     nearest_group = tasks_get_nearest_group_backward (pos, data);
     if(nearest_group>=0)
        true_dest = nearest_group;
     else
        true_dest = dest;

     tasks_move (pos, true_dest, FALSE, FALSE, data);
     i = 1;
     while(nb_tasks>0) {
        tasks_move (pos+i, true_dest+i, FALSE, FALSE, data);
        nb_tasks--;
        i++;
     }/* wend nb tsks */
     row = gtk_list_box_get_row_at_index (box, true_dest);
  }/* endif Up */
  else {/* move down */
     /* we look for nearest group forward */   
     nearest_group = tasks_get_nearest_group_forward (pos, data);
     if(nearest_group>=0)
        true_dest = nearest_group;
     else
        true_dest = dest;
printf  ("descendre ds move dest=%d nearest =%d \n", dest, nearest_group);
     tasks_move (pos, true_dest, FALSE, FALSE, data);
     i = 1;
     while(nb_tasks>0) {
        tasks_move (pos, true_dest, FALSE, FALSE, data);/* yes, because we are in INSERT mode */
        nb_tasks--;
        i++;
     }/* wend nb tsks */ 
     row = gtk_list_box_get_row_at_index (box, true_dest-i+1);
  }/* elseif down */

  /* undo engine final part */
  if(fUndo) {
    tmp_value->new_row = true_dest;
    tmp_value->opCode = OP_MOVE_GROUP_TASK;
    undo_push (CURRENT_STACK_TASKS, tmp_value, data);
  }
          
  gtk_list_box_select_row (box, row);  
  tasks_autoscroll ((gdouble)true_dest/g_list_length(data->tasksList), data);

}

/******************************************
  PUBLIC : move visually up or down a task
******************************************/
void tasks_move (gint pos, gint dest, gboolean fCcheck, gboolean fUndo, APP_data *data)
{
  GtkListBox *box; 
  GtkListBoxRow *row;
  gint i, dest_group, nbLinks;
  GList *l;
  tasks_data temp, *tmp, *tmp_tasks_datas2, *undoTasks, *tmp_tsk_datas;
  gchar *startDate, *tmpStr;
  GtkWidget *cadres;
  undo_datas *tmp_value;
  GDate date;

  box = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasks"));
  /* test if request to move visually up */
  if((dest<pos) && (pos<=0))
     return;
  /* test if request to move visually down */
  if((dest>pos) && (pos >= tasks_datas_len (data)))
     return;

  /* undo engine */
  if(fUndo) {
     l = g_list_nth (data->tasksList, pos);
     tmp_tsk_datas = (tasks_data *)l->data;
     undoTasks = g_malloc (sizeof(tasks_data));
     tasks_get_ressource_datas (pos, undoTasks, data);
     tmp_value = g_malloc (sizeof(undo_datas));
     tmp_value->opCode = OP_MOVE_TASK;
     tmp_value->insertion_row = pos;
     tmp_value->new_row = dest;
     tmp_value->id = tmp_tsk_datas->id;
     tmp_value->datas = undoTasks;
     tmp_value->list = NULL;
     tmp_value->groups = NULL;
     undoTasks->name = g_strdup_printf ("%s", tmp_tsk_datas->name);
     undoTasks->location = g_strdup_printf ("%s", tmp_tsk_datas->location);
     undo_push (CURRENT_STACK_TASKS, tmp_value, data);
  }
  /* we search group reference at 'dest' position */
printf ("avant back dest == %d \n", dest+1);
  if(dest<pos)
      dest_group = tasks_search_group_backward (dest, data);// TODO test dans les 2 sens !!!! ds taksutils.c
  else
      dest_group = tasks_search_group_forward (dest, data);// TODO test dans les 2 sens !!!! ds taksutils.c
printf ("passé avec des group == %d \n", dest_group);
// printf("valeur groupe pour destination =%d \n", dest_group);
  /* we search a pointer on current widget */
  l = g_list_nth (data->tasksList, pos);
  tmp = (tasks_data *)l->data;
  /* we set-up empty data */
  tmp_tasks_datas2 = g_malloc (sizeof(tasks_data));
  /* we copy datas */
  tmp_tasks_datas2->id = tmp->id;
  tmp_tasks_datas2->timer_val = tmp->timer_val;
  tmp_tasks_datas2->type = tmp->type;
  tmp_tasks_datas2->name = NULL;
  tmp_tasks_datas2->location = NULL;
  if(tmp->name)
      tmp_tasks_datas2->name = g_strdup_printf ("%s", tmp->name);
  if(tmp->location)
     tmp_tasks_datas2->location = g_strdup_printf ("%s", tmp->location);
  tmp_tasks_datas2->status = tmp->status;
  tmp_tasks_datas2->priority = tmp->priority;
  tmp_tasks_datas2->category = tmp->category;
  if(fCcheck) {
     tmp_tasks_datas2->group = dest_group;
     temp.group = dest_group;
  }
  else {
     tmp_tasks_datas2->group = tmp->group;
     temp.group = tmp->group;
  }

  tmp_tasks_datas2->color = tmp->color;
  tmp_tasks_datas2->progress = tmp->progress;
  tmp_tasks_datas2->start_nthDay = tmp->start_nthDay;
  tmp_tasks_datas2->start_nthMonth = tmp->start_nthMonth;
  tmp_tasks_datas2->start_nthYear = tmp->start_nthYear;
  tmp_tasks_datas2->end_nthDay = tmp->end_nthDay;
  tmp_tasks_datas2->end_nthMonth = tmp->end_nthMonth;
  tmp_tasks_datas2->end_nthYear = tmp->end_nthYear;
  tmp_tasks_datas2->lim_nthDay = tmp->lim_nthDay;
  tmp_tasks_datas2->lim_nthMonth = tmp->lim_nthMonth;
  tmp_tasks_datas2->lim_nthYear = tmp->lim_nthYear;
  tmp_tasks_datas2->cadre = tmp->cadre;
  tmp_tasks_datas2->cRscBox = tmp->cRscBox;
  tmp_tasks_datas2->cTaskBox = tmp->cTaskBox;
  tmp_tasks_datas2->cRun = tmp->cRun;
  tmp_tasks_datas2->cName = tmp->cName;
  tmp_tasks_datas2->cGroup = tmp->cGroup;
  tmp_tasks_datas2->cPeriod = tmp->cPeriod;
  tmp_tasks_datas2->cProgress = tmp->cProgress;
  tmp_tasks_datas2->cImgPty = tmp->cImgPty;
  tmp_tasks_datas2->cImgStatus = tmp->cImgStatus;
  tmp_tasks_datas2->cImgCateg = tmp->cImgCateg;
  tmp_tasks_datas2->cDashboard = tmp->cDashboard;
  tmp_tasks_datas2->groupItems = tmp->groupItems;
  tmp_tasks_datas2->durationMode = tmp->durationMode;
  tmp_tasks_datas2->days = tmp->days;
  tmp_tasks_datas2->hours = tmp->hours;
  tmp_tasks_datas2->minutes = tmp->minutes;
  tmp_tasks_datas2->calendar = tmp->calendar;
  tmp_tasks_datas2->delay = tmp->delay;
  /* we remove widget  */ 
  gtk_container_remove (GTK_CONTAINER (box), gtk_widget_get_parent (tmp->cadre));
  /* we free previous datas */
  if(tmp->name) {
      g_free (tmp->name);
      tmp->name=NULL;
  }
  if(tmp->location) {
      g_free (tmp->location);
      tmp->location=NULL;
  }

  /* update list */
  data->tasksList = g_list_delete_link (data->tasksList, l); 

  /* we rebuild row */
  g_date_set_dmy (&date, tmp_tasks_datas2->start_nthDay, tmp_tasks_datas2->start_nthMonth, tmp_tasks_datas2->start_nthYear);
  startDate = misc_convert_date_to_friendly_str (&date);

  /* switch according type of row */
  if(tmp_tasks_datas2->type == TASKS_TYPE_TASK) {
         cadres = tasks_new_widget (tmp_tasks_datas2->name, 
                              g_strdup_printf (_("%s, %d day(s)"), startDate, tmp_tasks_datas2->days), 
                              tmp_tasks_datas2->progress, tmp_tasks_datas2->timer_val, tmp_tasks_datas2->priority, 
                              tmp_tasks_datas2->category, tmp_tasks_datas2->color, tmp_tasks_datas2->status, &temp, data);
  }
  else {
         if(tmp_tasks_datas2->type == TASKS_TYPE_GROUP) {
            if(tasks_count_tasks_linked_to_group (tmp_tasks_datas2->id, data)>0) {
                tmpStr = g_strdup_printf (_("%d day(s)"), tmp->days);
            }
            else {
                tmpStr = g_strdup_printf ("%s", _("Empty"));
            }
            cadres = tasks_new_group_widget (tmp_tasks_datas2->name, tmpStr, 0, 0, 0, tmp_tasks_datas2->color, 0, &temp, data);
            g_free (tmpStr);
         }
         else /* milestones */
            cadres = tasks_new_milestone_widget (tmp_tasks_datas2->name, startDate, 0, 0, 0, tmp_tasks_datas2->color, 0, &temp, data);
  }

  tmp_tasks_datas2->cadre = temp.cadre;
  tmp_tasks_datas2->cRscBox = temp.cRscBox;
  tmp_tasks_datas2->cTaskBox = temp.cTaskBox;
  tmp_tasks_datas2->cRun = temp.cRun;
  tmp_tasks_datas2->cName = temp.cName;
  tmp_tasks_datas2->cGroup = temp.cGroup;
  tmp_tasks_datas2->cPeriod = temp.cPeriod;
  tmp_tasks_datas2->cProgress = temp.cProgress;
  tmp_tasks_datas2->cImgPty = temp.cImgPty;
  tmp_tasks_datas2->cImgStatus = temp.cImgStatus;
  tmp_tasks_datas2->cImgCateg = temp.cImgCateg;
  gtk_list_box_insert (GTK_LIST_BOX(box), GTK_WIDGET(cadres), dest);

  /* we add ressources to display */
  if(tmp_tasks_datas2->type == TASKS_TYPE_TASK) {
         tasks_add_display_ressources (tmp_tasks_datas2->id, tmp_tasks_datas2->cRscBox, data);
         tasks_add_display_tasks (tmp_tasks_datas2->id, tmp_tasks_datas2->cTaskBox, data);
  }
  /* we move new datas in position */
  data->tasksList = g_list_insert (data->tasksList, tmp_tasks_datas2, dest);
  /* we select the row */
  if(dest<g_list_length(data->tasksList)) {
    row = gtk_list_box_get_row_at_index (GTK_LIST_BOX(box), dest);
    gtk_list_box_select_row (GTK_LIST_BOX(box), row);
  }
  g_free (startDate);
  /* check & update predeccors or successors when task is linked */
  if(dest<pos) {/* move Up */
     nbLinks = links_check_task_predecessors (tmp->id, data);
     printf ("montée links predec =%d \n", nbLinks);
  }
  else {
     nbLinks = links_check_task_successors (tmp->id, data); 
     printf ("descente links succ =%d \n", nbLinks);
  }
  /* free element */
  g_free (tmp);
  /* update timeline */
  timeline_remove_all_tasks (data);
  timeline_draw_all_tasks (data);
  /* we update assignments */
  assign_remove_all_tasks (data);
  assign_draw_all_visuals (data);
  misc_display_app_status (TRUE, data);
}

/*****************************
 PUBLIC :
 add a new group task widget
******************************/
GtkWidget *tasks_new_group_widget (gchar *name, gchar *periods, 
                            gdouble progress, gint pty, gint categ, GdkRGBA color, gint status, tasks_data *tmp, APP_data *data)
{
  GtkWidget *cadreGrid, *cadre, *label_name, *label_periods, *icon, *tmpStr, *btn;
  GdkPixbuf *pix = NULL;
  GError *err = NULL;

  cadre = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  g_object_set (cadre, "margin-left", 4, NULL);
  g_object_set (cadre, "margin-right", 4, NULL);
  g_object_set (cadre, "margin-top", 12, NULL);
  g_object_set (cadre, "margin-bottom", 12, NULL);
  gtk_widget_show (cadre);
  cadreGrid = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  g_object_set (cadreGrid, "margin-left", 4, NULL);
  gtk_box_pack_start (GTK_BOX(cadre), cadreGrid, FALSE, FALSE, 4);

  pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("group.png"),
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);         
  icon = gtk_image_new_from_pixbuf (pix);
  g_object_unref (pix);
  gtk_widget_set_halign (icon, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (icon, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX(cadreGrid), icon, FALSE, FALSE, 2);

  /* edit button */
  btn = gtk_button_new_from_icon_name ("view-more-symbolic", GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX(cadreGrid), btn, FALSE, FALSE, 4);
  /* branch callback */
  g_signal_connect ((gpointer)btn, "clicked", G_CALLBACK (on_button_modify_group_clicked), data);

  tmpStr = g_markup_printf_escaped ("<u><big><big><b>%s</b></big></big></u>", name);
  label_name = gtk_label_new (tmpStr);
  g_free (tmpStr);
  gtk_widget_set_halign (label_name, GTK_ALIGN_START);
  g_object_set (label_name, "xalign", 0, NULL);
  gtk_label_set_use_markup (GTK_LABEL(label_name), TRUE);
  gtk_label_set_ellipsize (GTK_LABEL(label_name), PANGO_ELLIPSIZE_END);
  gtk_label_set_width_chars (GTK_LABEL(label_name), 32);
  gtk_label_set_max_width_chars (GTK_LABEL(label_name), 64);
  gtk_box_pack_start (GTK_BOX(cadreGrid), label_name, FALSE, FALSE, 4);
  /* duration */
  tmpStr = g_markup_printf_escaped ("<i>%s</i>", periods);
  label_periods = gtk_label_new (tmpStr);
  g_free (tmpStr);
  gtk_widget_set_halign (label_periods, GTK_ALIGN_START);
  g_object_set (label_periods, "xalign", 0, NULL);
  gtk_label_set_use_markup (GTK_LABEL(label_periods), TRUE);
  gtk_box_pack_start (GTK_BOX(cadreGrid), label_periods, FALSE, FALSE, 4);

  gtk_widget_show_all (GTK_WIDGET(cadreGrid)); 
  /* store widgets in temporary variable */
  tmp->cName = label_name;
  tmp->cGroup = NULL;
  tmp->cPeriod = label_periods;
  tmp->cProgress = NULL;
  tmp->cImgPty = NULL;
  tmp->cImgStatus = NULL;
  tmp->cImgCateg = NULL;
  tmp->cRscBox = NULL;
  tmp->cTaskBox = NULL;
  tmp->cRun = NULL;
  tmp->cDashboard = NULL;
  tmp->cadre = cadre;

  return cadre;
}

/*****************************
 PUBLIC :
 add a new milestone widget
******************************/
GtkWidget *tasks_new_milestone_widget (gchar *name, gchar *periods, gdouble progress, gint pty, 
                                       gint categ, GdkRGBA color, gint status, tasks_data *tmp, APP_data *data)
{
  GtkWidget *cadreGrid, *cadre, *label_name, *label_period, *icon, *btn;
  GdkPixbuf *pix;
  GError *err = NULL;
  gchar *tmpStr;

  cadre = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  g_object_set (cadre, "margin-left", 4, NULL);
  g_object_set (cadre, "margin-right", 4, NULL);
  g_object_set (cadre, "margin-top", 12, NULL);
  g_object_set (cadre, "margin-bottom", 12, NULL);
  gtk_widget_show (cadre);
  cadreGrid = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  g_object_set (cadreGrid, "margin-left", 4, NULL);
  gtk_box_pack_start (GTK_BOX(cadre), cadreGrid, FALSE, FALSE, 4);

  pix = gdk_pixbuf_new_from_file_at_size ( find_pixmap_file ("milestone.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);         
  icon = gtk_image_new_from_pixbuf (pix);
  g_object_unref (pix);
  gtk_widget_set_halign (icon, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (icon, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX(cadreGrid), icon, FALSE, FALSE, 2);

  /* edit button */
  btn = gtk_button_new_from_icon_name ("view-more-symbolic", GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX(cadreGrid), btn, FALSE, FALSE, 4);
  /* branch callback */
  g_signal_connect ((gpointer)btn, "clicked", G_CALLBACK (on_button_modify_group_clicked), data);/* yes same as group */

  tmpStr = g_markup_printf_escaped ("<big><b>%s</b></big>", name);
  label_name = gtk_label_new (tmpStr);
  g_free (tmpStr);
  gtk_widget_set_halign (label_name, GTK_ALIGN_START);
  g_object_set (label_name, "xalign", 0, NULL);
  gtk_label_set_use_markup (GTK_LABEL(label_name), TRUE);
  gtk_label_set_ellipsize (GTK_LABEL(label_name), PANGO_ELLIPSIZE_END);
  gtk_label_set_width_chars (GTK_LABEL(label_name), 32);
  gtk_label_set_max_width_chars (GTK_LABEL(label_name), 32);
  gtk_box_pack_start (GTK_BOX(cadreGrid), label_name, FALSE, FALSE, 4);

  label_period = gtk_label_new (g_strdup_printf ("<i><b>%s</b></i>", periods));
  g_object_set (label_period, "xalign", 0, NULL);
  g_object_set (label_period, "yalign", 1.0, NULL);
  gtk_label_set_use_markup (GTK_LABEL(label_period), TRUE);
  gtk_label_set_ellipsize (GTK_LABEL(label_period), PANGO_ELLIPSIZE_END);
  gtk_label_set_width_chars (GTK_LABEL(label_period), 64);
  gtk_label_set_max_width_chars (GTK_LABEL(label_period), 64);
  gtk_box_pack_start (GTK_BOX(cadreGrid), label_period, FALSE, FALSE, 4);

  gtk_widget_show_all (GTK_WIDGET(cadreGrid)); 

  /* store widgets in temporary variable */
  tmp->cName = label_name;
  tmp->cPeriod = label_period;
  tmp->cGroup = NULL;
  tmp->cProgress = NULL;
  tmp->cImgPty = icon;/* we reuse an existing field */
  tmp->cImgStatus = NULL;
  tmp->cImgCateg = NULL;
  tmp->cRscBox = NULL;
  tmp->cTaskBox = NULL;
  tmp->cRun = NULL;
  tmp->cDashboard = NULL;
  tmp->cadre = cadre;

  return cadre;
}

/************************************************
  PUBLIC function
  modify a milestone widget at a given position
  NOTE : this function must be called
  ONLY when datas are modified in 
  memory, with "tasks_modify_datas()'
*************************************************/
void tasks_milestone_modify_widget (gint position, APP_data *data)
{
  gint rsc_list_len = g_list_length (data->tasksList);
  GList *l;
  gchar *human_date=NULL, *tmpStr;
  GDate date;

  if((position >=0) && (position<=rsc_list_len)) {
     l = g_list_nth (data->tasksList, position);
     tasks_data *tmp_tasks_datas;
     tmp_tasks_datas = (tasks_data *)l->data;
     g_date_set_dmy (&date, tmp_tasks_datas->start_nthDay, 
                          tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
     human_date = misc_convert_date_to_friendly_str (&date);
     /* we change labels */
     tmpStr = g_markup_printf_escaped ("<big><b>%s</b></big>", tmp_tasks_datas->name);
     gtk_label_set_markup (GTK_LABEL(tmp_tasks_datas->cName), tmpStr);
     g_free (tmpStr);
     gtk_label_set_markup (GTK_LABEL(tmp_tasks_datas->cPeriod), 
                  g_strdup_printf ("<i><b>%s</b></i>", human_date));
     g_free (human_date);
  }/* endif */
}

/****************************
 PUBLIC :
 add a new milestone 
*****************************/
void tasks_milestone_new (GDate *date, APP_data *data)
{
  GtkWidget *dialog, *boxTasks, *entryName, *dateEntry, *locationEntry;
  GDate today, date_current;
  gint ret = 0, insertion_row;
  gchar *name = NULL, *location = NULL, *human_date = NULL;
  tasks_data temp;
  GdkRGBA color = {0, 0, 1};
  const *currentDate;
  const gchar *fooStr = _("Noname Milestone");
  const gchar *fooStr2 = _("Location unknown");

  boxTasks = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasks"));

  dialog = tasks_create_milestone_dialog (FALSE, -1, data);
  dateEntry = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonDate"));
  if(date == NULL) {
     //  g_date_set_time_t (&today, time (NULL)); /* date of today */
      /* we get the project starting date by default */
      g_date_set_dmy (&today, data->properties.start_nthDay, data->properties.start_nthMonth, 
                              data->properties.start_nthYear);
      gtk_button_set_label (GTK_BUTTON(dateEntry), misc_convert_date_to_locale_str (&today));
  }
  else {
      gtk_button_set_label (GTK_BUTTON(dateEntry), misc_convert_date_to_locale_str (date));
  }
  
  ret = gtk_dialog_run (GTK_DIALOG(dialog));
  if(ret == 1) {
    tasks_utils_reset_errors (data);
    /* yes, we want to add a new milestone */
    entryName = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryMilestoneName")); 
    locationEntry = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryMilestoneLocation")); 
    /* we get various useful values */
    name = gtk_entry_get_text (GTK_ENTRY(entryName));
    if(strlen (name) == 0) {
       name = fooStr;
    }
    location = gtk_entry_get_text (GTK_ENTRY(locationEntry));
    if(strlen (location) == 0) {
       location = fooStr2;
    }

    currentDate = gtk_button_get_label (GTK_BUTTON(dateEntry));
    g_date_set_parse (&date_current, currentDate);
    human_date = misc_convert_date_to_friendly_str (&date_current);

  //  color.red = 0;
  //  color.green = 0;
  //  color.blue = 1;
    temp.type = TASKS_TYPE_MILESTONE;
    temp.location = location;
    temp.group = -1;
    temp.days = 0;
    temp.hours = 0;
    temp.calendar = 0;
    temp.timer_val = 0;
    GtkWidget *cadres = tasks_new_milestone_widget (name, human_date, 0, 0, 0, color, 0, &temp, data);
    /* here, all widget pointers are stored in 'temp' */
    insertion_row = task_get_selected_row (data)+1;
    /* we insert a simple widget - only title and period human string */ 
    gtk_list_box_insert (GTK_LIST_BOX(boxTasks), GTK_WIDGET(cadres), insertion_row);

    tasks_store_to_tmp_data (insertion_row, name, g_date_get_day (&date_current),
                            g_date_get_month (&date_current), g_date_get_year (&date_current), 
                            g_date_get_day (&date_current), g_date_get_month (&date_current), 
                            g_date_get_year (&date_current),
                            -1, -1, -1, 
                            0, 0, 0, 0, color, data, -1, TRUE, TASKS_AS_SOON, 1, 0, 0, 0, -1, &temp);/* -1 = delay */
    tasks_insert (insertion_row, data, &temp);/* here we store id */
    /* update display */
    if(g_list_length (data->tasksList) == 1) {
        task_set_selected_row (0, data); 
    }
    else {
        task_set_selected_row (insertion_row, data);
    }
    tasks_autoscroll ((gdouble)insertion_row/g_list_length (data->tasksList), data);
    /* we update links */
    tasks_update_links_tasks (temp.id, dialog, data);
    gint t_links = links_check_task_linked (temp.id, data);
    /* if t_links >0 ther is at least one predecessor task */
    if(t_links>=0) {printf ("milestone entree ds test \n");
         gint rc = tasks_compute_dates_for_linked_new_task (temp.id, TASKS_AS_SOON, 1, insertion_row, 0,
                                                            &date_current, FALSE, FALSE, dialog, data);
    }/* endif links */
    /* update timeline */
    timeline_remove_all_tasks (data);
    timeline_draw_all_tasks (data);
    g_free (human_date);
    home_project_tasks (data);
    home_project_milestones (data);
    misc_display_app_status (TRUE, data);
    /* undo engine - we just have to get a pointer on ID */
    undo_datas *tmp_value;
    tmp_value = g_malloc (sizeof(undo_datas));
    tmp_value->opCode = OP_INSERT_MILESTONE;
    tmp_value->insertion_row = insertion_row;
    tmp_value->id = temp.id;
    tmp_value->datas = NULL;/* nothing to store ? */
    tmp_value->groups = NULL;
    tmp_value->list = NULL;
    undo_push (CURRENT_STACK_TASKS, tmp_value, data);
  }

  g_object_unref (data->tmpBuilder);
  gtk_widget_destroy (GTK_WIDGET(dialog));
}

/*************************
 PUBLIC :
 add a new group widget
*************************/
void tasks_group_new (APP_data *data)
{
  GtkWidget *dialog, *boxTasks, *entryName;
  gint ret=0, insertion_row, days;
  const gchar *name = NULL;
  const gchar *fooStr2 = _("Noname Group");
  gchar *tmpStr;
  tasks_data temp;
  GdkRGBA color;
  GDate date_interval1, date_interval2;

  boxTasks = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasks"));
  dialog = tasks_create_group_dialog (FALSE, -1, data);
  ret = gtk_dialog_run (GTK_DIALOG(dialog));
  if(ret == 1) {
    /* yes, we want to add a new task */
    entryName = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryGroupName")); 
    
    /* we get various useful values */
    name = gtk_entry_get_text (GTK_ENTRY(entryName));
    if(strlen (name) == 0) {
       name = fooStr2;
    }

    temp.type = TASKS_TYPE_GROUP;
    temp.location = NULL;
    temp.group = -1;
    temp.timer_val = 0;
    /* duration evaluation - with true group duration */
    g_date_set_parse (&date_interval1, data->properties.start_date);
    g_date_set_parse (&date_interval2, data->properties.end_date);
    days = g_date_days_between (&date_interval1, &date_interval2)+1;
    tmpStr = g_strdup_printf ("%s", _("Empty"));
    GtkWidget *cadres = tasks_new_group_widget (name, tmpStr, 0, 0, 0, color, 0, &temp, data);
    g_free (tmpStr);
    /* here, all widget pointers are stored in 'temp' */
    insertion_row = task_get_selected_row (data)+1;
    if(insertion_row >= g_list_length (data->tasksList)) {
	   insertion_row++;	
    }
    insertion_row = tasks_compute_valid_insertion_point (insertion_row, -1, TRUE, data);
        
    /* we insert a simple widget - only title */
    gtk_list_box_insert (GTK_LIST_BOX(boxTasks), GTK_WIDGET(cadres), insertion_row);

    tasks_store_to_tmp_data (insertion_row, name, g_date_get_day (&date_interval1),
                             g_date_get_month (&date_interval1), g_date_get_year (&date_interval1), 
                             g_date_get_day (&date_interval2), g_date_get_month (&date_interval2), 
                             g_date_get_year (&date_interval2),
                             -1, -1, -1, 
                             0, 0, 0, 0, color, data, -1, TRUE, 0, days, 0, 0, 0, -1, &temp);// TODO days hours 1 == days , -1 delay

    tasks_insert (insertion_row, data, &temp);/* here we store id */
    /* update display */
    if(g_list_length (data->tasksList) == 1) {
        task_set_selected_row (0, data); 
    }
    else {
        task_set_selected_row (insertion_row, data);
    }
    tasks_autoscroll ((gdouble)insertion_row/g_list_length (data->tasksList), data);    
    /* update timeline */
    timeline_remove_all_tasks (data);
    timeline_draw_all_tasks (data);
    misc_display_app_status (TRUE, data);
    /* undo engine - we just have to get a pointer on ID */
    undo_datas *tmp_value;
    tmp_value = g_malloc (sizeof(undo_datas));
    tmp_value->opCode = OP_INSERT_GROUP;
    tmp_value->insertion_row = insertion_row;
    tmp_value->id = temp.id;
    tmp_value->datas  = NULL;/* nothing to store ? */
    tmp_value->groups = NULL;
    tmp_value->list   = NULL;
    undo_push (CURRENT_STACK_TASKS, tmp_value, data);
  }

  g_object_unref (data->tmpBuilder);
  gtk_widget_destroy (GTK_WIDGET(dialog));
}

/*************************************
  PUBLIC  move FORWARD a task
  nbDays = days to move
***********************************/
void tasks_move_forward (gint nbDays, gint position, APP_data *data)
{
  GDate date_task_start, date_task_end, date_proj_start, date_proj_end;
  GDate newEndDate, newStartDate, limDate;
  gint tsk_list_len = g_list_length (data->tasksList);
  GList *l;

  /* we must know the official end date of project - it's the LIMIT for forwarding */
  g_date_set_dmy (&date_proj_start, 
                  data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
  g_date_set_dmy (&date_proj_end, 
                  data->properties.end_nthDay, data->properties.end_nthMonth, data->properties.end_nthYear);
  /* now, same for the current selected row */
  if((position >=0) && (position < tsk_list_len) && (nbDays>0)) {
     l = g_list_nth (data->tasksList, position);
     tasks_data *tmp_tasks_datas, temp, *undoTasks;
     /* undo engine */
     tmp_tasks_datas = (tasks_data *)l->data;
     g_date_set_dmy (&date_task_start, 
                  tmp_tasks_datas->start_nthDay, tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
     g_date_set_dmy (&date_task_end, 
                  tmp_tasks_datas->end_nthDay, tmp_tasks_datas->end_nthMonth, tmp_tasks_datas->end_nthYear);
     /* now we attempt to modify those dates */
     newEndDate = date_task_end;
     newStartDate = date_task_start;
     g_date_add_days (&newEndDate, nbDays);
     g_date_add_days (&newStartDate, nbDays);
     gint cmp_date = g_date_compare (&newEndDate, &date_proj_end);
     if(cmp_date>0) {/* we are OFF limits */
         misc_ErrorDialog (data->appWindow, _("<b>Please note!</b>\n\nCurrent task 'Ending' date should be <i>before</i> project's 'Ending' date.\n\nPlanLibre has automatically updated project's ending date."));
        /* we update ending date */
        data->properties.end_nthDay = g_date_get_day (&newEndDate);
        data->properties.end_nthMonth = g_date_get_month (&newEndDate);
        data->properties.end_nthYear = g_date_get_year (&newEndDate);
        if(data->properties.end_date)
           g_free (data->properties.end_date);
        data->properties.end_date = misc_convert_date_to_locale_str (&newEndDate);
     }
    /* check if current task is in deadline duration mode */
    /* security if the tasks is a sender for another task ! */
/*
    cmp_date = tasks_check_links_dates_forward (position, newEndDate, data);
    if((cmp_date!=0)  && (tmp_tasks_datas->durationMode == TASKS_DEADLINE)) {printf ("coucou \n");
       misc_ErrorDialog (data->appWindow, _("<b>Can't do that !</b>\n\nDates mismatch !\nThis task is a predecessor for one or more other tasks.\nCurrent task 'Ending' date must be <i>before</i> children task 'Starting' date.\n\nPlease check the dates, or modify children's dates to fix this issue."));
       return;
    }*/
    /* we can continue */
    /* Undo engine - we read current datas in memory */
    tasks_get_ressource_datas (position, &temp, data);
    undoTasks = g_malloc (sizeof(tasks_data));
    tasks_get_ressource_datas (position, undoTasks, data);
    /* build strings in order to keep "true" values */
    undoTasks->name = NULL;
    undoTasks->location = NULL;
    if(temp.name)
        undoTasks->name = g_strdup_printf ("%s", temp.name);
    if(temp.location)
        undoTasks->location = g_strdup_printf ("%s", temp.location);

    undo_datas *tmp_value;
    tmp_value = g_malloc (sizeof(undo_datas));
    tmp_value->insertion_row = position;
    tmp_value->new_row = position;
    tmp_value->id = temp.id;
    tmp_value->datas = undoTasks;
    tmp_value->list = NULL;
    tmp_value->groups = NULL;
    tmp_value->opCode = OP_DELAY_TASK;
    tmp_value->s_day = g_date_get_day (&date_proj_start);
    tmp_value->s_month = g_date_get_month (&date_proj_start);
    tmp_value->s_year = g_date_get_year (&date_proj_start);
    tmp_value->e_day = g_date_get_day (&date_proj_end);
    tmp_value->e_month = g_date_get_month (&date_proj_end);
    tmp_value->e_year = g_date_get_year (&date_proj_end);
    undo_push (CURRENT_STACK_TASKS, tmp_value, data);
    /* end of Undo engine */

    tmp_tasks_datas->start_nthDay = g_date_get_day (&newStartDate);
    tmp_tasks_datas->start_nthMonth = g_date_get_month (&newStartDate);
    tmp_tasks_datas->start_nthYear = g_date_get_year (&newStartDate);

    tmp_tasks_datas->end_nthDay = g_date_get_day (&newEndDate);
    tmp_tasks_datas->end_nthMonth = g_date_get_month (&newEndDate);
    tmp_tasks_datas->end_nthYear = g_date_get_year (&newEndDate);

    if(tmp_tasks_datas->type == TASKS_TYPE_TASK) {
      /* update dashboard */
      dashboard_remove_task (position, data);
      /*  update tasks list */
      tasks_modify_widget (position, data); 
      /*  dashboard */
      dashboard_add_task (position, data);
      tasks_update_group_limits (tmp_tasks_datas->group, tmp_tasks_datas->group, tmp_tasks_datas->group, data);
    }
    if(tmp_tasks_datas->type == TASKS_TYPE_MILESTONE) {
        tasks_milestone_modify_widget (position, data);
    }

    /* update timeline */
    timeline_remove_all_tasks (data);/* YES, for other tasks */
    timeline_draw_all_tasks (data);    
    /* we update assignments */
    assign_remove_all_tasks (data);
    assign_draw_all_visuals (data); 
  }/* endif */
}

/*************************************
  PUBLIC  move Backward a task
  nbDays = days to move
***********************************/
void tasks_move_backward (gint nbDays, gint position, APP_data *data)
{
  GDate date_task_start, date_task_end, date_proj_start, date_proj_end;
  GDate newEndDate, newStartDate;
  gint tsk_list_len = g_list_length (data->tasksList);
  GList *l;

  /* we must know the official end date of project - it's the LIMIT for forwarding */
  g_date_set_dmy (&date_proj_start, 
                  data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
  g_date_set_dmy (&date_proj_end, 
                  data->properties.end_nthDay, data->properties.end_nthMonth, data->properties.end_nthYear);
  /* now, same for the current selected row */
  if((position >=0) && (position < tsk_list_len) && (nbDays>0)) {
     l = g_list_nth (data->tasksList, position);
     tasks_data *tmp_tasks_datas, temp, *undoTasks;
     tmp_tasks_datas = (tasks_data *)l->data;
     g_date_set_dmy (&date_task_start, 
                  tmp_tasks_datas->start_nthDay, tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
     g_date_set_dmy (&date_task_end, 
                  tmp_tasks_datas->end_nthDay, tmp_tasks_datas->end_nthMonth, tmp_tasks_datas->end_nthYear);
     /* now we attempt to modify those dates */
     newEndDate = date_task_end;
     newStartDate = date_task_start;
     g_date_subtract_days (&newEndDate, nbDays);
     g_date_subtract_days (&newStartDate, nbDays);

     gint cmp_date = g_date_compare (&newStartDate, &date_proj_start);
     if(cmp_date<0) {/* we are OFF limits */
       misc_ErrorDialog (data->appWindow, _("<b>Please note!</b>\n\nCurrent task 'Starting' date should be <i>after or equal</i>   project's 'Starting' date.\n\nPlanLibre has automatically updated project's starting date."));
       data->properties.start_nthDay = g_date_get_day (&newStartDate);
       data->properties.start_nthMonth = g_date_get_month (&newStartDate);
       data->properties.start_nthYear = g_date_get_year (&newStartDate);
       if(data->properties.start_date)
           g_free (data->properties.start_date);
       data->properties.start_date = misc_convert_date_to_locale_str (&newStartDate);
     }
    /* security if the tasks is a sender for another task ! */
    cmp_date = tasks_check_links_dates_backward (position, newStartDate, data);
    if(cmp_date!=0) {
       misc_ErrorDialog (data->appWindow, _("<b>Can't do that !</b>\n\nDates mismatch !\nThis task has predecessor(s).\nCurrent task 'Starting' date must be <i>after</i> predecessor task 'Ending' date.\n\nPlease check the dates, or modify predecessor's dates to solve this issue."));
       return;
    }

    /* we can continue */
    /* Undo engine - we read current datas in memory */
    tasks_get_ressource_datas (position, &temp, data);
    undoTasks = g_malloc (sizeof(tasks_data));
    tasks_get_ressource_datas (position, undoTasks, data);
    /* build strings in order to keep "true" values */
    undoTasks->name = NULL;
    undoTasks->location = NULL;
    if(temp.name)
        undoTasks->name = g_strdup_printf ("%s", temp.name);
    if(temp.location)
        undoTasks->location = g_strdup_printf ("%s", temp.location);

    undo_datas *tmp_value;
    tmp_value = g_malloc (sizeof(undo_datas));
    tmp_value->insertion_row = position;
    tmp_value->new_row = position;
    tmp_value->id = temp.id;
    tmp_value->datas = undoTasks;
    tmp_value->list = NULL;
    tmp_value->groups = NULL;
    tmp_value->opCode = OP_DELAY_TASK;
    tmp_value->s_day = g_date_get_day (&date_proj_start);
    tmp_value->s_month = g_date_get_month (&date_proj_start);
    tmp_value->s_year = g_date_get_year (&date_proj_start);
    tmp_value->e_day = g_date_get_day (&date_proj_end);
    tmp_value->e_month = g_date_get_month (&date_proj_end);
    tmp_value->e_year = g_date_get_year (&date_proj_end);
    undo_push (CURRENT_STACK_TASKS, tmp_value, data);
    /* end of Undo engine */

    tmp_tasks_datas->start_nthDay = g_date_get_day (&newStartDate);
    tmp_tasks_datas->start_nthMonth = g_date_get_month (&newStartDate);
    tmp_tasks_datas->start_nthYear = g_date_get_year (&newStartDate);

    tmp_tasks_datas->end_nthDay = g_date_get_day (&newEndDate);
    tmp_tasks_datas->end_nthMonth = g_date_get_month (&newEndDate);
    tmp_tasks_datas->end_nthYear = g_date_get_year (&newEndDate);

    if(tmp_tasks_datas->type == TASKS_TYPE_TASK) {
       /* dashboard before */
       dashboard_remove_task (position, data);
       /*  update tasks list */
       tasks_modify_widget (position, data); 
       /*  dashboard */
       dashboard_add_task (position, data);
       tasks_update_group_limits (tmp_tasks_datas->group, tmp_tasks_datas->group, tmp_tasks_datas->group, data);
    }
    if(tmp_tasks_datas->type == TASKS_TYPE_MILESTONE) {
        tasks_milestone_modify_widget (position, data);
    }

    /* update timeline */
    timeline_remove_all_tasks (data);/* YES, for other tasks */
    timeline_draw_all_tasks (data);   
    /* we update assignments */
    assign_remove_all_tasks (data);
    assign_draw_all_visuals (data);  
  }/* endif */
}

/**********************************************
  PUBLIC  increase/decrease duration of a task
  nbDays = dauys to add/substract
**********************************************/
void tasks_change_duration (gint nbDays, gint position, APP_data *data)
{
  GDate date_task_start, date_task_end, date_proj_start, date_proj_end;
  GDate newEndDate, newStartDate;
  tasks_data *tmp_tasks_datas, temp, *undoTasks;
  gint cmp_date, tsk_list_len = g_list_length (data->tasksList);
  GList *l;

  /* we must know the official end date of project - it's the LIMIT for forwarding */
  g_date_set_dmy (&date_proj_start, data->properties.start_nthDay, data->properties.start_nthMonth, 
                  data->properties.start_nthYear);
  g_date_set_dmy (&date_proj_end, data->properties.end_nthDay, data->properties.end_nthMonth, 
                  data->properties.end_nthYear);
  /* now, same for the current selected row */
  if((position >=0) && (position < tsk_list_len)) {
     /* we read current datas in memory */
     tasks_get_ressource_datas (position, &temp, data);
     l = g_list_nth (data->tasksList, position);
     tmp_tasks_datas = (tasks_data *)l->data;
     g_date_set_dmy (&date_task_start, 
                  tmp_tasks_datas->start_nthDay, tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
     g_date_set_dmy (&date_task_end, 
                  tmp_tasks_datas->end_nthDay, tmp_tasks_datas->end_nthMonth, tmp_tasks_datas->end_nthYear);
     /* now we attempt to modify those dates */
     newEndDate = date_task_end;
     newStartDate = date_task_start;
     if(nbDays<0)
        g_date_subtract_days (&newEndDate, (-1*nbDays));
     else 
        g_date_add_days (&newEndDate, nbDays);

     /* check if the is at least a duration of one day since task start day */
     cmp_date = g_date_compare (&newEndDate, &date_task_start);    
     if(cmp_date<0) {/* we are OFF task start date  */
       misc_ErrorDialog (data->appWindow, _("<b>Error!</b>\n\nDates mismatch !\n Current task 'Ending' date must be <i>after</i> task's 'Starting' date.\n\nPlease check the dates, or modify tasks's dates."));
       return;
     }

     /* security if the tasks is a sender for another task ! We check in cas of increeasing only */
     if(nbDays>0) {
       cmp_date =  tasks_check_links_dates_forward (position, newEndDate, data);
cmp_date = 0;
       if(cmp_date!=0) {
          misc_ErrorDialog (data->appWindow, _("<b>Can't do that !</b>\n\nDates mismatch !\nThis task is a predecessor for one or more other tasks.\nCurrent task 'Ending' date must be <i>before</i> children task 'Starting' date.\n\nPlease check the dates, or modify children's dates to solve this issue."));
          return;
       }/* endif error */
     }/* endif nbdays */

     /* undo engine */
     undoTasks = g_malloc (sizeof(tasks_data));
     tasks_get_ressource_datas (position, undoTasks, data);
     /* build strings in order to keep "true" values */
     undoTasks->name = NULL;
     if(temp.name)
        undoTasks->name = g_strdup_printf ("%s", temp.name);
     undoTasks->location = NULL;   
     if(temp.location)
        undoTasks->location = g_strdup_printf ("%s", temp.location);

     undo_datas *tmp_value;
     tmp_value = g_malloc (sizeof(undo_datas));
     tmp_value->insertion_row = position;
     tmp_value->new_row = position;
     tmp_value->id = temp.id;
     tmp_value->datas = undoTasks;
     tmp_value->groups = NULL;
     tmp_value->list = NULL;
     if(nbDays<0)
         tmp_value->opCode = OP_REDUCE_TASK;
     else
         tmp_value->opCode = OP_INCREASE_TASK;
     tmp_value->s_day = g_date_get_day (&date_proj_start);
     tmp_value->s_month = g_date_get_month (&date_proj_start);
     tmp_value->s_year = g_date_get_year (&date_proj_start);
     tmp_value->e_day = g_date_get_day (&date_proj_end);
     tmp_value->e_month = g_date_get_month (&date_proj_end);
     tmp_value->e_year = g_date_get_year (&date_proj_end);
     undo_push (CURRENT_STACK_TASKS, tmp_value, data);
     /* end of Undo part */

     cmp_date = g_date_compare (&newEndDate, &date_proj_end);
     /* check if we are in limits with project end date */
     if(cmp_date>0) {/* we are OFF limits */
        misc_ErrorDialog (data->appWindow, _("<b>Please note!</b>\n\nCurrent task 'Ending' date should be <i>before</i> project's 'Ending' date.\n\nPlanLibre has automatically updated project's ending date."));
        /* we update ending date */
        data->properties.end_nthDay = g_date_get_day (&newEndDate);
        data->properties.end_nthMonth = g_date_get_month (&newEndDate);
        data->properties.end_nthYear = g_date_get_year (&newEndDate);
        if(data->properties.end_date)
           g_free (data->properties.end_date);
        data->properties.end_date =  misc_convert_date_to_locale_str (&newEndDate);
     }

    /* we can continue */
    tmp_tasks_datas->end_nthDay = g_date_get_day (&newEndDate);
    tmp_tasks_datas->end_nthMonth = g_date_get_month (&newEndDate);
    tmp_tasks_datas->end_nthYear = g_date_get_year (&newEndDate);
    /*  update tasks list TODO : recal duration ! */
    tmp_tasks_datas->days = g_date_days_between (&date_task_start, &newEndDate)+1;
    tasks_modify_widget (position, data); 
    tasks_update_group_limits (tmp_tasks_datas->id, tmp_tasks_datas->group, tmp_tasks_datas->group, data);
    /* update timeline */
    timeline_remove_all_tasks (data);/* YES, for other tasks */
    timeline_draw_all_tasks (data);     
    /* we update assignments */
    assign_remove_all_tasks (data);
    assign_draw_all_visuals (data);
  }/* endif */
}

