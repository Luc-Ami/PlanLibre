/***************************
  functions to undo redo 
  operations
****************************/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* translations */
#include <libintl.h>
#include <locale.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h> /* g_fopen, etc */

#include "support.h" /* glade requirement */
#include "misc.h"
#include "undoredo.h"
#include "timeline.h"
#include "calendars.h"
#include "tasksutils.h"
#include "tasksdialogs.h"
#include "tasks.h"
#include "ressources.h"
#include "properties.h"
#include "assign.h"
#include "links.h"
#include "report.h"
#include "dashboard.h"
#include "home.h"

/****************************
  protected local variables
****************************/
static GList *undoList = NULL;
static GList *redoList = NULL;

/*

  F U N C T I O N S

*/

static gint get_undo_current_op_code (APP_data *data)
{
  gint ret = OP_NONE;
  undo_datas *tmp_undo_datas;
  
  if(undoList) {
    GList *l= g_list_last (undoList);
    tmp_undo_datas = (undo_datas *)l->data;
    ret = tmp_undo_datas->opCode;
  }
  return ret;
}

/*********************************************
   protected : free content of undo datas
*********************************************/
static void undo_free_content (GList *l)
{
  gint op, i, k;
  undo_datas *tmp_undo_datas;
  group_datas *tmpGroup;
  link_datas *tmpLink;
  tasks_data *tmpTsk;
  calendar *cal, *tmp;
  calendar_element *element, *elem2;
  GList *l1, *l2;

  tmp_undo_datas = (undo_datas *)l->data;
  op = tmp_undo_datas->opCode;

  if(op>=0 && op<LAST_TASKS_OP) {/* tasks */
      if(tmp_undo_datas->datas!=NULL) {
               tmpTsk = tmp_undo_datas->datas;
               if(tmpTsk->name)
                   g_free (tmpTsk->name);
               if(tmpTsk->location)
                   g_free (tmpTsk->location);
               g_free (tmp_undo_datas->datas);
      }/* endif datas */
      if(tmp_undo_datas->list) {
           for(i=0;i<g_list_length (tmp_undo_datas->list);i++) {
               l1 = g_list_nth (tmp_undo_datas->list, i);
               tmpLink = (link_datas *) l1->data;
               g_free (tmpLink);
           }
           g_list_free (tmp_undo_datas->list);
      }/* endif list */

      if(tmp_undo_datas->groups) {printf ("je free groups \n");
           for(i=0;i<g_list_length (tmp_undo_datas->groups);i++) {
               l1 = g_list_nth (tmp_undo_datas->groups, i);
               tmpGroup = (group_datas *) l1->data;
               g_free (tmpGroup);          
           }
           g_list_free (tmp_undo_datas->groups);
      }/* endif groups */
  }/* endif op for tasks */

  if(op>LAST_TASKS_OP && op<LAST_RSC_OP) {/* rsc */
      if(tmp_undo_datas->datas!=NULL) {
         rsc_datas *tmpRsc;
         tmpRsc = tmp_undo_datas->datas;
         if(tmp_undo_datas->opCode!=OP_REMOVE_RSC) {/* because rsc_insert already freed strings */
		 if(tmpRsc->name) 
		     g_free (tmpRsc->name);
		 if(tmpRsc->mail)
		     g_free (tmpRsc->mail);
		 if(tmpRsc->phone)
		     g_free (tmpRsc->phone);
		 if(tmpRsc->reminder)
		     g_free (tmpRsc->reminder);
		 if(tmpRsc->location)
		     g_free (tmpRsc->location);
         }
         g_free (tmp_undo_datas->datas);
      }   
      if(tmp_undo_datas->list) {
        for(i=0;i<g_list_length (tmp_undo_datas->list);i++) {
             l1 = g_list_nth (tmp_undo_datas->list, i);
             tmpLink = (link_datas *) l1->data;
             g_free (tmpLink);
        }
        g_list_free (tmp_undo_datas->list);
      }/* endif list */
  }/* endif op for rsc */

  if(op>LAST_RSC_OP && op<LAST_MISC_OP) {/* misc */
      if(op==OP_MODIFY_PROP) {
         rsc_properties *tmp;
         tmp = tmp_undo_datas->datas;
         if(tmp->title)
            g_free (tmp->title);
         if(tmp->summary)
            g_free (tmp->summary);
         if(tmp->location)
            g_free (tmp->location);
         if(tmp->gps)
            g_free (tmp->gps);
         if(tmp->units)
            g_free (tmp->units);
         if(tmp->manager_name)
            g_free (tmp->manager_name);
         if(tmp->manager_mail)
            g_free (tmp->manager_mail);
         if(tmp->manager_phone)
            g_free (tmp->manager_phone);
         if(tmp->org_name)
            g_free (tmp->org_name);
         if(tmp->proj_website)
            g_free (tmp->proj_website);
         if(tmp->start_date)
            g_free (tmp->start_date);
         if(tmp->end_date)
            g_free (tmp->end_date);
         if(tmp->logo)
            g_object_unref (tmp->logo);
         g_free (tmp);
      }/* endif modif prop */

      if(op==OP_MODIFY_CAL) {
         for(i=0;i<g_list_length (tmp_undo_datas->list);i++) {
            l1 = g_list_nth (tmp_undo_datas->list, i);
            cal = (calendar *)l1->data;
            if(cal->name!=NULL) {
               g_free (cal->name);
            }
            if(cal->list!=NULL) {
               for(k=0;k<g_list_length (cal->list);k++) {
                  l2 = g_list_nth (cal->list, k);
                  element = (calendar_element *) l2->data;
                  g_free (element);
               } /* next k */          
               g_list_free (cal->list);/* easy, nothing inside is dynamically allocated */
            }
         }/* next i */
         g_list_free (tmp_undo_datas->list);
      }/* endif calendars */
  }/* endif misc */

  if(op>LAST_MISC_OP && op<LAST_DASH_OP) {
      if(tmp_undo_datas->datas!=NULL) {
               tmpTsk = tmp_undo_datas->datas;
               if(tmpTsk->name)
                   g_free (tmpTsk->name);
               if(tmpTsk->location)
                   g_free (tmpTsk->location);
               g_free (tmp_undo_datas->datas);
      }/* endif datas */
  }/* endif dashboard */
  g_free (tmp_undo_datas);
}


static void undo_free_first_data (APP_data *data)
{
   GList *l = g_list_first (undoList);

   undo_free_content (l);
   undoList = g_list_delete_link (undoList, l);  
}

/*************************************
  PROTECTED : free all datas according
  according to "class" of operations
  given by opCode
**************************************/
static void undo_free_last_data (APP_data *data)
{
   GList *l = g_list_last (undoList);
  
   undo_free_content (l);
   undoList = g_list_delete_link (undoList, l);  
}

/*********************************
  update undo menu
*********************************/
static void update_undo_menu (gint op, APP_data *data)
{
  GtkWidget *menuEditUndo, *menuEditRedo;
gint len = g_list_length (undoList);
  menuEditUndo = GTK_WIDGET(gtk_builder_get_object (data->builder, "menuEditUndo"));
//  menuEditRedo = GTK_WIDGET(gtk_builder_get_object (data->builder, "menuEditRedo"));
  if(g_list_length (undoList)>0) {
/*
     switch(op) {
        case OP_INSERT_TASK: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo insert new task"));
           break;
        }
        case OP_REMOVE_TASK: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo remove task"));
           break;
        }
        case OP_APPEND_TASK: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo append task from CSV"));
           break;
        }
        case OP_MODIFY_TASK: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo modify task"));
           break;
        }
        case OP_TIMESHIFT_TASKS: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo global tasks timeshift"));
           break;
        }
        case OP_TIMESHIFT_GROUP: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo group of tasks timeshift"));
           break;
        }
        case OP_REPEAT_TASK: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo insert last repeated task"));
           break;
        }
        case OP_ADVANCE_TASK: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo advance task"));
           break;
        }
        case OP_DELAY_TASK: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo delay task"));
           break;
        }
        case OP_REDUCE_TASK: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo decrease task duration"));
           break;
        }
        case OP_INCREASE_TASK: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo increase task duration"));
           break;
        }
        case OP_MOVE_TASK: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo move task"));
           break;
        }
        case OP_MOVE_GROUP_TASK: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo move group"));
           break;
        }
        case OP_MODIFY_TASK_GROUP: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo modify task's group"));
           break;
        }
        case OP_INSERT_GROUP: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo insert new group"));
           break;
        }
        case OP_MODIFY_GROUP: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo modify group"));
           break;
        }
        case OP_REMOVE_GROUP: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo remove group"));
           break;
        }
        case OP_INSERT_MILESTONE: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo insert new milestone"));
           break;
        }
        case OP_MODIFY_MILESTONE: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo modify milestone"));
           break;
        }
        case OP_REMOVE_MILESTONE: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo remove milestone"));
           break;
        }
        case OP_INSERT_RSC: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo insert new ressource"));
           break;
        }
        case OP_REMOVE_RSC: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo remove ressource"));
           break;
        }
        case OP_IMPORT_LIB_RSC:
        case OP_APPEND_RSC: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo insert last appended ressource"));
           break;
        }
        case OP_CLONE_RSC: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo insert new cloned ressource"));
           break;
        }
        case OP_MODIFY_RSC: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo modify ressource"));
           break;
        }
        case OP_MODIFY_PROP: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo modify properties"));
           break;
        }
        case OP_MODIFY_CAL: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo modify calendars"));
           break;
        }
        case OP_MOVE_DASH: {
           gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Undo modify task status on dashboard"));
           break;
        }
     }/* end switch */ 
  gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), g_strdup_printf ("nbre tâches à annuler =%d ", len));
     gtk_widget_set_sensitive (GTK_WIDGET (menuEditUndo), TRUE);
  }
  else {
     gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Nothing to undo"));
     gtk_widget_set_sensitive (GTK_WIDGET (menuEditUndo), FALSE);
  }
}

/*******************************
 PROTECTED : update menu
*******************************/
static void undo_update_menu_after_pop (APP_data *data)
{
  GtkWidget *menuEditUndo, *menuEditRedo;
  menuEditUndo = GTK_WIDGET(gtk_builder_get_object (data->builder, "menuEditUndo"));

  /* if #elements ==0 then lock button */
  if(g_list_length (undoList)==0) {
    gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Nothing to undo"));  
    gtk_widget_set_sensitive (GTK_WIDGET (menuEditUndo), FALSE);
  }
  else {
    GList *l = g_list_last (undoList);
    undo_datas *tmp_undo_datas;
    tmp_undo_datas = (undo_datas *)l->data;
    update_undo_menu (tmp_undo_datas->opCode, data);
  }  
}

static void undo_push_tasks (undo_datas *value, APP_data *data)
{
  gint undo_list_len = g_list_length (undoList);

  /* if current list's length==max allowed length for undo buffer, we delete the first element */
  if(undo_list_len>=MAX_UNDO_OPERATIONS) {
     printf ("* Undo buffer is full, I delete the first element *\n");
     if(undoList!=NULL)
        undo_free_first_data (data);
  }

  /* store datas contained in 'values' in GList */
  undoList = g_list_append (undoList, value);
  update_undo_menu (value->opCode, data);
}

static void undo_push_rsc (undo_datas *value, APP_data *data)
{
  gint undo_list_len = g_list_length (undoList);

  /* if current list's length==max allowed length for undo buffer, we delete the first element */
  if(undo_list_len>=MAX_UNDO_OPERATIONS) {
     printf ("* Undo buffer is full, I delete the first element *\n");
     if(undoList!=NULL)
        undo_free_first_data (data);
  }

  /* store datas contained in 'values' in GList */
  undoList = g_list_append (undoList, value);
  update_undo_menu (value->opCode, data);
}

static void undo_push_misc (undo_datas *value, APP_data *data)
{
  gint undo_list_len = g_list_length (undoList);

  /* if current list's length==max allowed length for undo buffer, we delete the first element */
  if(undo_list_len>=MAX_UNDO_OPERATIONS) {
     printf ("* Undo buffer is full, I delete the first element *\n");
     if(undoList!=NULL)
        undo_free_first_data (data);
  }

  /* store datas contained in 'values' in GList */
  undoList = g_list_append (undoList, value);
  update_undo_menu (value->opCode, data);
}

static void undo_push_dash (undo_datas *value, APP_data *data)
{
  gint undo_list_len = g_list_length (undoList);

  /* if current list's length==max allowed length for undo buffer, we delete the first element */
  if(undo_list_len>=MAX_UNDO_OPERATIONS) {
     printf ("* Undo buffer is full, I delete the first element *\n");
     if(undoList!=NULL)
        undo_free_first_data (data);
  }

  /* store datas contained in 'values' in GList */
  undoList = g_list_append (undoList, value);
  update_undo_menu (value->opCode, data);
}

/********************************************
 PUBLIC : main function to call undo engine
 arguments : current_stack identifier of
        PlanLibre 'page'
  value : allocated structure
*******************************************/
void undo_push (gint current_stack, undo_datas *value, APP_data *data)
{
  GtkWidget *menuEditUndo, *menuEditRedo, *mainWin;

  mainWin = GTK_WIDGET(gtk_builder_get_object (data->builder, "window_main"));
  menuEditUndo = GTK_WIDGET(gtk_builder_get_object (data->builder, "menuEditUndo"));
  gtk_widget_grab_focus (mainWin);
//  menuEditRedo = GTK_WIDGET(gtk_builder_get_object (data->builder, "menuEditRedo"));
  gtk_widget_set_sensitive (menuEditUndo, TRUE);
//  gtk_widget_set_sensitive (menuEditUndo, TRUE);

  switch(current_stack) {
    case CURRENT_STACK_TASKS: {
      undo_push_tasks (value, data);
      break;
    }
    case CURRENT_STACK_RESSOURCES: {
      undo_push_rsc (value, data);
      break;
    }
    case CURRENT_STACK_MISC: {
      undo_push_misc (value, data);
      break;
    }
    case CURRENT_STACK_DASHBOARD: {
      undo_push_dash (value, data);
      break;
    }
    default: {
      printf ("* PlanLibre critical - something wrong with UNDO engine source *\n");
    }
  }/* end switch */
}

/**************************************
  PROTECTED : pop ressources elements
**************************************/
static void undo_pop_rsc (gint op, APP_data *data)
{
  undo_datas *tmp_undo_datas;
  link_datas *tmp_links_datas, *tmp;
  rsc_datas tmpRsc, *temp;
  GtkWidget *boxRsc;
  gint rank, i;
  GList *l1;
  GtkWidget *cadres, *listBoxRessources;

  GList *l = g_list_last (undoList);
  tmp_undo_datas = (undo_datas *)l->data;
  boxRsc = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxRsc"));
  rank = rsc_get_rank_for_id (tmp_undo_datas->id, data);
  switch(op) {
        case OP_INSERT_RSC:
        case OP_APPEND_RSC:
        case OP_IMPORT_LIB_RSC:
        case OP_CLONE_RSC:{
          rsc_remove_tasks_widgets (tmp_undo_datas->id, data);
          links_remove_all_rsc_links (tmp_undo_datas->id, data);
          rsc_remove (rank, boxRsc, data);
          assign_remove_all_tasks (data);
          assign_draw_all_visuals (data);
          report_clear_text (data->buffer, "left");
          break;
        }
        case OP_REMOVE_RSC: {
          /* we update links - we have only to get preceding GList of links store in list field */
          links_free_all (data);
          data->linksList = NULL;
          for(i = 0; i < g_list_length (tmp_undo_datas->list); i++) {
            l1 = g_list_nth (tmp_undo_datas->list, i);
            tmp_links_datas = (link_datas *)l1->data;
            tmp = g_malloc (sizeof(link_datas));
            tmp->type = tmp_links_datas->type;
            tmp->sender = tmp_links_datas->sender;
            tmp->receiver = tmp_links_datas->receiver;
            data->linksList = g_list_append (data->linksList, tmp);
          }/* next i */
          /* manage stored pixbuf */
          temp = tmp_undo_datas->datas;
          /* reset row at previous position */
          cadres = rsc_new_widget (temp->name, temp->type, temp->affiliate, 
                                temp->cost, temp->cost_type, temp->color, data, TRUE, temp->pix, temp);
          /* here we have datas on widgets stored in 'rsc' variable */
          listBoxRessources = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxRsc"));
          gtk_list_box_insert (GTK_LIST_BOX(listBoxRessources), GTK_WIDGET(cadres), tmp_undo_datas->insertion_row);
          /* we store datas - we must duplicate pixbuf */
          GdkPixbuf *pix2 = NULL;
          if(temp->pix)
              pix2 = gdk_pixbuf_copy (temp->pix);

          rsc_store_to_app_data (tmp_undo_datas->insertion_row, temp->name, temp->mail, temp->phone, temp->reminder, 
                            temp->type, temp->affiliate, temp->cost, temp->cost_type, temp->quantity, temp->color, pix2,
                            data , temp->id, temp, FALSE);
          rsc_insert (tmp_undo_datas->insertion_row, temp, data);
          /* we update tasks display */
          rsc_modify_tasks_widgets (tmp_undo_datas->id, data);
          /* we update assignments */
          assign_remove_all_tasks (data);
          assign_draw_all_visuals (data);
          report_clear_text (data->buffer, "left");
          /* update timeline */
          timeline_remove_all_tasks (data);
          timeline_draw_all_tasks (data);
          break;
        }
        case OP_MODIFY_RSC: {
          /* manage stored pixbuf */
          rsc_get_ressource_datas (tmp_undo_datas->insertion_row, &tmpRsc, data);
          if(tmpRsc.pix) // données réelles 
               g_object_unref (tmpRsc.pix);

          rsc_modify_datas (tmp_undo_datas->insertion_row, data, tmp_undo_datas->datas);
          /* don't free memory for 'newRsc' strings : they're only pointers to widgets */
          rsc_modify_widget (tmp_undo_datas->insertion_row, data);
          /* we update tasks display */
          rsc_modify_tasks_widgets (tmp_undo_datas->id, data);
          /* we update assignments */
          assign_remove_all_tasks (data);
          assign_draw_all_visuals (data);
          report_clear_text (data->buffer, "left");
          /* update timeline */
          timeline_remove_all_tasks (data);
          timeline_draw_all_tasks (data);
          break;
        }
        default: printf ("* Unmanaged UNDO operation request for ressources *\n");
  }/* end switch */
}

/**********************************
  PROTECTED : undo timeshift
**********************************/
static void undo_timeshift (undo_datas *tmp, gboolean fGroup, APP_data *data)
{
  GtkAdjustment *vadj;
  gint dd, mm, yy;
  GDate new_date;

    dashboard_remove_all (FALSE, data);    
    if(!fGroup) {
       properties_shift_dates (tmp->t_shift*(-1), data);    
       tasks_shift_dates (tmp->t_shift*(-1), FALSE, -1, data);
       /* we store extrems dates to update timeline */
       data->earliest = misc_get_earliest_date (data);
       data->latest = misc_get_latest_date (data);
       g_date_set_dmy (&new_date, data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
       dd = g_date_get_day (&new_date);
       mm = g_date_get_month (&new_date);
       yy = g_date_get_year (&new_date);
       timeline_store_date (dd, mm, yy);
    }
    else {
       tasks_shift_dates (tmp->t_shift*(-1), TRUE, tmp->id, data);
    }
    /* redraw timeline */
    timeline_remove_all_tasks (data);
    vadj = gtk_scrolled_window_get_vadjustment 
                          (GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowTimeline")));
    gtk_adjustment_set_value (GTK_ADJUSTMENT(vadj), 0);
    timeline_draw_calendar_ruler (0, data);
    timeline_draw_all_tasks (data);
    /* redraw assignments */
    assign_remove_all_tasks (data);
    assign_draw_all_visuals (data);

    /* clear report */
    report_clear_text (data->buffer, "left");
    report_default_text (data->buffer);  
}

/***************************************
  PROTECTED : pop tasks elements
  returns 0 if ther isn't any
  pending UNDO operation (for example
  when user cancels multiple tasks move
****************************************/
static gint undo_pop_tasks (gint op, APP_data *data)
{
  undo_datas *tmp_undo_datas;
  GtkWidget *boxTasks, *cadres, *combo;
  GdkPixbuf *pix;
  GError *err = NULL;
  gint rank, i, j, ret = 0;
  GList *l1, *l2;
  link_datas *tmp_links_datas, *tmp;
  tasks_data *tmpTsk, temp, *tmp_tsk;
  group_datas *tmpGroup;
  gchar *human_date;
  GDate current_date, tmp_date;

  GList *l = g_list_last (undoList);
  tmp_undo_datas = (undo_datas *)l->data;
  boxTasks = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasks"));
  rank = tasks_get_rank_for_id (tmp_undo_datas->id, data);

  switch(op) {
        case OP_REPEAT_TASK:
        case OP_APPEND_TASK:
        case OP_INSERT_TASK:{
          tasks_remove (rank, GTK_LIST_BOX(boxTasks), data);
          /* update timeline - please note, the task is removed by 'task_remove()' function */
          timeline_remove_all_tasks (data);/* YES, for other tasks */
          timeline_draw_all_tasks (data);
          /* we update assignments */
          assign_remove_all_tasks (data);
          assign_draw_all_visuals (data);
          break;
        }
        case OP_TIMESHIFT_TASKS: {
          undo_timeshift (tmp_undo_datas, FALSE, data);
          break;
        }
        case OP_TIMESHIFT_GROUP: {
          undo_timeshift (tmp_undo_datas, TRUE, data);
          break;
        }

        case OP_REMOVE_MILESTONE:
        case OP_REMOVE_TASK:{
          tmpTsk = tmp_undo_datas->datas;
          g_date_set_dmy (&current_date, tmpTsk->start_nthDay, tmpTsk->start_nthMonth, tmpTsk->start_nthYear);
          human_date = misc_convert_date_to_friendly_str (&current_date);

          if(tmpTsk->type != TASKS_TYPE_MILESTONE) {
          	  /* we update links - we have only to get preceding GList of links store in list field */
		  links_free_all (data);
		  data->linksList = NULL;
		  for(i=0;i<g_list_length (tmp_undo_datas->list);i++) {
		    l1 = g_list_nth (tmp_undo_datas->list, i);
		    tmp_links_datas = (link_datas *)l1->data;
		    tmp = g_malloc (sizeof(link_datas));
		    tmp->type = tmp_links_datas->type;
		    tmp->sender = tmp_links_datas->sender;
		    tmp->receiver = tmp_links_datas->receiver;
		    data->linksList = g_list_append (data->linksList, tmp);
		  }/* next i */
          }
          if(tmp_undo_datas->type_tsk == TASKS_TYPE_TASK) {
             temp.group = tmpTsk->group;
             cadres = tasks_new_widget (tmpTsk->name, g_strdup_printf (_("%s, %d day(s)"), human_date, tmpTsk->days), 
                              tmpTsk->progress, tmpTsk->timer_val, tmpTsk->priority, tmpTsk->category, 
                              tmpTsk->color, tmpTsk->status, &temp, data);
             gtk_list_box_insert (GTK_LIST_BOX(boxTasks), GTK_WIDGET(cadres), tmp_undo_datas->insertion_row); 
             tasks_store_to_tmp_data (tmp_undo_datas->insertion_row, tmpTsk->name, tmpTsk->start_nthDay,
                             tmpTsk->start_nthMonth, tmpTsk->start_nthYear, 
                             tmpTsk->end_nthDay, tmpTsk->end_nthMonth, tmpTsk->end_nthYear, 
                             tmpTsk->lim_nthDay, tmpTsk->lim_nthMonth, tmpTsk->lim_nthYear, 
                             tmpTsk->progress, tmpTsk->priority, tmpTsk->category, tmpTsk->status, 
                             tmpTsk->color, data, tmp_undo_datas->id, FALSE, tmpTsk->durationMode, tmpTsk->days, 
                             tmpTsk->hours, 0, 0, 0, &temp);/* calendar id = 0 */ 
          //   temp.group = tmpTsk->group;
             temp.type = tmpTsk->type;
             temp.location = tmpTsk->location;
             temp.timer_val = tmpTsk->timer_val;
             tasks_insert (tmp_undo_datas->insertion_row, data, &temp);/* here we store id & timer_val*/
             dashboard_add_task (tmp_undo_datas->insertion_row, data);
             /* we add ressources to display */
             tasks_add_display_ressources (temp.id, temp.cRscBox, data);
             /* we add ancestors tasks to display */
             tasks_add_display_tasks (temp.id, temp.cTaskBox, data);
             /* we update assignments */
             assign_remove_all_tasks (data);
             assign_draw_all_visuals (data);printf ("fin pour tsk \n");
          }
          if(tmp_undo_datas->type_tsk == TASKS_TYPE_GROUP) {

          }
          if(tmp_undo_datas->type_tsk == TASKS_TYPE_MILESTONE) {
             cadres = tasks_new_milestone_widget (tmpTsk->name, human_date, 0, 0, 0, tmpTsk->color, 0, &temp, data);
             gtk_list_box_insert (GTK_LIST_BOX(boxTasks), GTK_WIDGET(cadres), tmp_undo_datas->insertion_row);
             tasks_store_to_tmp_data (tmp_undo_datas->insertion_row, tmpTsk->name, tmpTsk->start_nthDay,
                             tmpTsk->start_nthMonth, tmpTsk->start_nthYear, 
                             tmpTsk->end_nthDay, tmpTsk->end_nthMonth, tmpTsk->end_nthYear, 
                             tmpTsk->lim_nthDay, tmpTsk->lim_nthMonth, tmpTsk->lim_nthYear, 
                             0, 0, 0, 0, 
                             tmpTsk->color, data, tmp_undo_datas->id, FALSE, 0, 1, 0, 0, 0, 0, &temp );/* calendar id = 0 */ 
             temp.type = tmpTsk->type;
             temp.group = tmpTsk->group;
             tasks_insert (tmp_undo_datas->insertion_row, data, &temp);/* here we store id & timer_val*/

          }
          /* update timeline */
          timeline_remove_all_tasks (data);
          timeline_draw_all_tasks (data);
          home_project_tasks (data);
          home_project_milestones (data);
          g_free (human_date);
          break;
        }
        case OP_MODIFY_TASK_GROUP:
        case OP_MODIFY_TASK:{
          ret = tmp_undo_datas->nbTasks;
          if(tmp_undo_datas->new_row != tmp_undo_datas->insertion_row) {/* side effect of groups */
              tasks_move (tmp_undo_datas->new_row, tmp_undo_datas->insertion_row, FALSE, FALSE, data);
          }
          tmpTsk = tmp_undo_datas->datas;
          if(tmpTsk->type == TASKS_TYPE_TASK)
              dashboard_remove_task (tmp_undo_datas->insertion_row, data);
          tasks_get_ressource_datas (tmp_undo_datas->insertion_row, &temp, data);
 printf ("il y a %d points d'annulation de moduf ici tâche nom=%s \n", tmp_undo_datas->nbTasks, temp.name);
          tasks_modify_datas (tmp_undo_datas->insertion_row, data, tmp_undo_datas->datas);
          /*  dashboard */
          if(tmpTsk->type == TASKS_TYPE_TASK)
              dashboard_add_task (tmp_undo_datas->insertion_row, data);
          /* we update links - we have only to get preceding GList of links store in list field */
          links_free_all (data);
          data->linksList = NULL;
          for(i=0;i<g_list_length (tmp_undo_datas->list);i++) {
            l1 = g_list_nth (tmp_undo_datas->list, i);
            tmp_links_datas = (link_datas *)l1->data;
            tmp = g_malloc (sizeof(link_datas));
            tmp->type = tmp_links_datas->type;
            tmp->sender = tmp_links_datas->sender;
            tmp->receiver = tmp_links_datas->receiver;
            data->linksList = g_list_append (data->linksList, tmp);
          }/* next i */
          /* get a pointer on useful datas */// ???? TODO est-ce possible d'arriver ici avec un milestone ???
          if(tmpTsk->type == TASKS_TYPE_TASK)
             tasks_modify_widget (tmp_undo_datas->insertion_row, data);
          /* ressources & linked tasks - first we remove all children from rscBox */
          tasks_remove_widgets (tmpTsk->cRscBox);
          tasks_remove_widgets (tmpTsk->cTaskBox);
          tasks_add_display_ressources (tmpTsk->id, tmpTsk->cRscBox, data);
          tasks_add_display_tasks (tmpTsk->id, tmpTsk->cTaskBox, data);
          /* timeline */
          tasks_update_group_limits (tmpTsk->id, tmpTsk->group, tmpTsk->group, data);
          timeline_remove_all_tasks (data);
          timeline_draw_all_tasks (data);
          /* we update assignments */
          assign_remove_all_tasks (data);
          assign_draw_all_visuals (data);
          home_project_tasks (data);
          home_project_milestones (data);
          /* update combo groups if we undo a group change */
          if(op==OP_MODIFY_TASK_GROUP) {
              tasks_free_list_groups (data);
              tasks_fill_list_groups (data);
              combo = GTK_WIDGET(gtk_builder_get_object (data->builder, "comboTasksGroup"));
              gtk_combo_box_set_active (GTK_COMBO_BOX(combo), tasks_get_group_rank (tmpTsk->group, data));
              tasks_free_list_groups (data);
              tasks_update_group_limits (tmpTsk->id, temp.group, temp.group, data);/* update Canceled group */
          }
          break;
        }
        case OP_DELAY_TASK:
        case OP_ADVANCE_TASK:
        case OP_INCREASE_TASK:
        case OP_REDUCE_TASK:{
          tmpTsk = tmp_undo_datas->datas;
          if(tmpTsk->type == TASKS_TYPE_TASK) {
             dashboard_remove_task (tmp_undo_datas->insertion_row, data);
          }
          tasks_modify_datas (tmp_undo_datas->insertion_row, data, tmp_undo_datas->datas);
          /* force to previous project's dates */
          data->properties.start_nthDay = tmp_undo_datas->s_day;
          data->properties.start_nthMonth = tmp_undo_datas->s_month;
          data->properties.start_nthYear = tmp_undo_datas->s_year;
          if(data->properties.start_date)
             g_free (data->properties.start_date);
          g_date_set_dmy (&tmp_date, tmp_undo_datas->s_day, tmp_undo_datas->s_month, tmp_undo_datas->s_year);
          data->properties.start_date =  misc_convert_date_to_locale_str (&tmp_date);
          data->properties.end_nthDay = tmp_undo_datas->e_day;
          data->properties.end_nthMonth = tmp_undo_datas->e_month;
          data->properties.end_nthYear = tmp_undo_datas->e_year;
          if(data->properties.end_date)
             g_free (data->properties.end_date);
          g_date_set_dmy (&tmp_date, tmp_undo_datas->e_day, tmp_undo_datas->e_month, tmp_undo_datas->e_year);
          data->properties.end_date =  misc_convert_date_to_locale_str (&tmp_date);
          if(tmpTsk->type == TASKS_TYPE_TASK) {
            tasks_modify_widget (tmp_undo_datas->insertion_row, data);
            /*  dashboard */
            dashboard_add_task (tmp_undo_datas->insertion_row, data);
          }
          /* update timeline */

          if(tmpTsk->type == TASKS_TYPE_TASK) {
             tasks_update_group_limits (tmpTsk->id, tmpTsk->group, tmpTsk->group, data);
          }
          timeline_remove_all_tasks (data);
          timeline_draw_all_tasks (data);
          /* we update assignments */
          if(tmpTsk->type == TASKS_TYPE_TASK) {
             assign_remove_all_tasks (data);
             assign_draw_all_visuals (data);
          }
          home_project_tasks (data);
          home_project_milestones (data);
          break;
        }
        case OP_MOVE_TASK: {
          tasks_modify_datas (tmp_undo_datas->new_row, data, tmp_undo_datas->datas);
          tasks_move (tmp_undo_datas->new_row, tmp_undo_datas->insertion_row, FALSE, FALSE, data);
          break;
        }
        case OP_INSERT_GROUP:{
          tasks_remove (rank, GTK_LIST_BOX(boxTasks), data);
          break;
        } 
        case OP_MODIFY_GROUP:{
          tasks_modify_datas (rank, data, tmp_undo_datas->datas);
          tasks_group_modify_widget (rank, data);
          /* update timeline */
          timeline_remove_all_tasks (data);
          timeline_draw_all_tasks (data);
          break;
        }
        case OP_REMOVE_GROUP:{
          group_datas *tmpGroup;
          tmpTsk = tmp_undo_datas->datas;
          g_date_set_dmy (&current_date, tmpTsk->start_nthDay, tmpTsk->start_nthMonth, tmpTsk->start_nthYear);

          cadres = tasks_new_group_widget (tmpTsk->name, "", 0, 0, 0, tmpTsk->color, 0, &temp, data);
          gtk_list_box_insert (GTK_LIST_BOX(boxTasks), GTK_WIDGET(cadres), tmp_undo_datas->insertion_row);
          tasks_store_to_tmp_data (tmp_undo_datas->insertion_row, tmpTsk->name, tmpTsk->start_nthDay,
                             tmpTsk->start_nthMonth, tmpTsk->start_nthYear, 
                             tmpTsk->end_nthDay, tmpTsk->end_nthMonth, tmpTsk->end_nthYear, 
                             tmpTsk->lim_nthDay, tmpTsk->lim_nthMonth, tmpTsk->lim_nthYear, 
                             0, 0, 0, 0, 
                             tmpTsk->color, data, tmp_undo_datas->id, FALSE, 0, tmpTsk->days, 0, 0, 0, 0, &temp );/* calendar id = 0 */
          temp.group = tmpTsk->group;
          temp.type = tmpTsk->type;
          temp.timer_val = tmpTsk->timer_val;
          tasks_insert (tmp_undo_datas->insertion_row, data, &temp);/* here we store id */
          /* reset group<>tasks associations */
          for(i=0;i<g_list_length(tmp_undo_datas->groups);i++) {
              l1 = g_list_nth (tmp_undo_datas->groups, i);
              tmpGroup = (group_datas *) l1->data;
              for(j=0;j<g_list_length (data->tasksList);j++) {
                  l2 = g_list_nth (data->tasksList, j);
                  tmp_tsk = (tasks_data *) l2->data;
                  if(tmp_tsk->id == tmpGroup->id) {
                      tmp_tsk->group = tmpTsk->id;
                      /* update widget */
                      pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("groupedtsk.png"), 
                          TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);
                      gtk_image_set_from_pixbuf (tmp_tsk->cGroup, pix);
                      g_object_unref (pix);
                      g_object_set (tmp_tsk->cadre, "margin-left", 32, NULL);
                  }/* endif */
              }/* next j */
          }/* next i */
          break;
        } 
        case OP_MOVE_GROUP_TASK: {
          if(tmp_undo_datas->insertion_row > tmp_undo_datas->new_row) {
                printf ("demande annuler montée de groupe\n");
                for(i=0; i<=tmp_undo_datas->nbTasks;i++) {
                   tasks_move (tmp_undo_datas->new_row, 
                               tmp_undo_datas->insertion_row + tmp_undo_datas->nbTasks, FALSE, FALSE, data);
                }

          }
          else {
                printf ("demande annuler descente de groupe  origine =%d dest =%d nb tasks %d\n", tmp_undo_datas->insertion_row, tmp_undo_datas->new_row, tmp_undo_datas->nbTasks);
                for(i=0; i<=tmp_undo_datas->nbTasks;i++) {
                   tasks_move (tmp_undo_datas->new_row, 
                               tmp_undo_datas->insertion_row, FALSE, FALSE, data);
                }
          }
          break;
        }
        case OP_INSERT_MILESTONE:{printf ("essai undo insert milestone \n");
          tasks_remove (rank, GTK_LIST_BOX(boxTasks), data);
          /* update timeline - please note, the task is removed by 'task_remove()' function */
          timeline_remove_all_tasks (data);/* YES, for other tasks */
          timeline_draw_all_tasks (data);
          break;
        }
        case OP_MODIFY_MILESTONE:{
          tasks_get_ressource_datas (tmp_undo_datas->insertion_row, &temp, data);// à OTER
printf ("il y a %d points d'annulation de moduf ici tâche jalon nom=%s \n", tmp_undo_datas->nbTasks, temp.name);
          ret = tmp_undo_datas->nbTasks;

          /* we update links - we have only to get preceding GList of links store in list field */
          links_free_all (data);
          data->linksList = NULL;
          for(i=0;i<g_list_length (tmp_undo_datas->list);i++) {
            l1 = g_list_nth (tmp_undo_datas->list, i);
            tmp_links_datas = (link_datas *)l1->data;
            tmp = g_malloc (sizeof(link_datas));
            tmp->type = tmp_links_datas->type;
            tmp->sender = tmp_links_datas->sender;
            tmp->receiver = tmp_links_datas->receiver;
            data->linksList = g_list_append (data->linksList, tmp);
          }/* next i */


          tasks_modify_datas (rank, data, tmp_undo_datas->datas);
          tasks_milestone_modify_widget (rank, data);
          /* update timeline */
          timeline_remove_all_tasks (data);
          timeline_draw_all_tasks (data);
          break;
        }
        default: printf ("* Unmanaged UNDO operation request for tasks *\n");
  }/* end switch */
  return ret;
}

/**********************************
  PROTECTED : pop misc elements
**********************************/
static void undo_pop_misc (gint op, APP_data *data)
{
  undo_datas *tmp_undo_datas;
  gint i, j, k;
  GList *l = g_list_last (undoList);
  GList *l1, *l2;
  calendar *cal, *tmp;
  calendar_element *element, *elem2;
  tmp_undo_datas = (undo_datas *)l->data;

  switch(op) {
      case OP_MODIFY_PROP:{
          rsc_properties *tmp;
          tmp = tmp_undo_datas->datas;
          if(data->properties.title)
             g_free (data->properties.title);
          data->properties.title = g_strdup_printf ("%s", tmp->title);
          if(data->properties.summary)
             g_free (data->properties.summary);
          data->properties.summary = g_strdup_printf ("%s", tmp->summary);
          if(data->properties.location)
             g_free (data->properties.location);
          data->properties.location = g_strdup_printf ("%s", tmp->location);
          if(data->properties.gps)
             g_free (data->properties.gps);
          data->properties.gps = g_strdup_printf ("%s", tmp->gps);
          if(data->properties.units)
             g_free (data->properties.units);
          data->properties.units = g_strdup_printf ("%s", tmp->units);
          if(data->properties.manager_name)
             g_free (data->properties.manager_name);
          data->properties.manager_name = g_strdup_printf ("%s", tmp->manager_name);
          if(data->properties.manager_mail)
             g_free (data->properties.manager_mail);
          data->properties.manager_mail = g_strdup_printf ("%s", tmp->manager_mail);
          if(data->properties.manager_phone)
             g_free (data->properties.manager_phone);
          data->properties.manager_phone = g_strdup_printf ("%s", tmp->manager_phone);
          if(data->properties.org_name)
             g_free (data->properties.org_name);
          data->properties.org_name = g_strdup_printf ("%s", tmp->org_name);
          if(data->properties.proj_website)
             g_free (data->properties.proj_website);
          data->properties.proj_website = g_strdup_printf ("%s", tmp->proj_website);
          data->properties.budget = tmp->budget;
          data->properties.scheduleStart = tmp->scheduleStart;
          data->properties.scheduleEnd = tmp->scheduleEnd;
          data->properties.start_date = g_strdup_printf ("%s", tmp->start_date);
          data->properties.end_date = g_strdup_printf ("%s", tmp->end_date);
          data->properties.start_nthDay = tmp->start_nthDay;
          data->properties.start_nthMonth = tmp->start_nthMonth;
          data->properties.start_nthYear = tmp->start_nthYear;
          data->properties.end_nthDay = tmp->end_nthDay;
          data->properties.end_nthMonth = tmp->end_nthMonth;
          data->properties.end_nthYear = tmp->end_nthYear;
          if(data->properties.logo!=NULL) {/* duplicate pixbuf */
             g_object_unref (data->properties.logo);
             data->properties.logo = NULL;
          }
          if(tmp->logo!=NULL) {
             data->properties.logo = gdk_pixbuf_copy (tmp->logo);
          }
          /* update displays */
          GtkAdjustment *adjustment = GTK_ADJUSTMENT(gtk_builder_get_object (data->builder, "adjTasksTimeline"));
          goo_canvas_item_remove (data->ruler);
          timeline_draw_calendar_ruler (gtk_adjustment_get_value (adjustment), data);
          timeline_remove_all_tasks (data);
          timeline_draw_all_tasks (data);
          misc_display_app_status (TRUE, data);
// TODO transférer vers redo 
          break;
      }
      case OP_MODIFY_CAL:{/* undo calendar(s) modification(s) */
         /* we free current calendars */
         calendars_free_calendars (data);
         data->calendars = NULL;
         /* and simply copy datas from Undo memory */
         for(i=0;i<g_list_length(tmp_undo_datas->list);i++) {
            l1 = g_list_nth (tmp_undo_datas->list, i);
            cal = (calendar *)l1->data;
            tmp = g_malloc (sizeof(calendar));
            tmp->id = cal->id;
            tmp->list = NULL;
            tmp->name = NULL;
            if(i==0) {
              tmp->name = g_strdup_printf ("%s", _("Main Calendar"));
            }
            else {
               if(cal->name) {
                   tmp->name = g_strdup_printf ("%s", cal->name);
               }
            }

            for(k=0;k<7;k++) {
	       for(j=0;j<4;j++) {
	         tmp->fDays[k][j] = cal->fDays[k][j];
	         tmp->schedules[k][j][0] = cal->schedules[k][j][0];
	         tmp->schedules[k][j][0] = cal->schedules[k][j][0];
	      }
            }
            for(k=0;k<g_list_length (cal->list);k++) {
               l2 = g_list_nth (cal->list, k);
               element = (calendar_element *) l2->data;
               elem2 = g_malloc (sizeof(calendar_element));
               elem2->day = element->day;
               elem2->month = element->month;
               elem2->year = element->year;
               elem2->type = element->type;
               tmp->list = g_list_append (tmp->list, elem2);
            }/* next k */

            data->calendars = g_list_append (data->calendars, tmp);
printf("pop cycle =%d \n", i);
         }/* next i */
         break;
      }
      default: printf ("* Unmanaged UNDO operation request for properties or calendars *\n");
  }/* end switch */

}

/**********************************
  PROTECTED : pop dash elements
**********************************/
static void undo_pop_dash (gint op, APP_data *data)
{
  undo_datas *tmp_undo_datas;
  tasks_data *tmpTsk, *tmp_tsk_datas;
  gint pos, total, i;
  GList *l, *l1;
  GtkWidget *labelWait, *labelRun, *labelDelay, *labelDone;


  l = g_list_last (undoList);
  tmp_undo_datas = (undo_datas *)l->data;
  tmpTsk = tmp_undo_datas->datas;
  pos = tmp_undo_datas->insertion_row;
  dashboard_remove_all (FALSE, data);  
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
  total = g_list_length (data->tasksList);

  for(i=0;i<total;i++) {
      l = g_list_nth (data->tasksList, i);
      tmp_tsk_datas = (tasks_data *)l->data;
      if(tmp_tsk_datas->id == tmp_undo_datas->id) {
         tmp_tsk_datas->status = tmpTsk->status;
         tmp_tsk_datas->progress = tmpTsk->progress;
         tasks_modify_widget (pos, data); 
      }
      if(tmp_tsk_datas->type == TASKS_TYPE_TASK)
           dashboard_add_task (i, data);
  }/* next i */

}


/*******************************
 PUBLIC : function to 'POP' i.e.
 cancel last operation
********************************/
void undo_pop (APP_data *data)
{
  gint op, ret, rc;
  GtkWidget *menuEditUndo, *menuEditRedo;

  menuEditUndo = GTK_WIDGET(gtk_builder_get_object (data->builder, "menuEditUndo"));
 // menuEditRedo = GTK_WIDGET(gtk_builder_get_object (data->builder, "menuEditRedo"));

  if(g_list_length (undoList)<1) {
     printf ("* can't do that, UNDO list is empty !*\n");
     gtk_widget_set_sensitive (GTK_WIDGET (menuEditUndo), FALSE);
     return;
  }
  else 
     printf ("lg liste annul =%d \n", g_list_length (undoList));

  op = get_undo_current_op_code (data);

  if(op>=0 && op<LAST_TASKS_OP) {
    /* last op to undo and free */
printf ("avant ret op vaut %d \n", op);
    ret = undo_pop_tasks (op, data)-1;
    printf ("pop undo après premiere annul len list %d -----------ret = %d--- op =%d ---\n", g_list_length (undoList), ret, op);
    if(ret>=0 && undoList!=NULL) {
       undo_free_last_data (data);
       do {  
         if(undoList) {
           op = get_undo_current_op_code (data);
           rc = undo_pop_tasks (op, data);
           undo_free_last_data (data);
         }
         ret--;
       } while (ret>0);
    }/* endif */
// printf ("fin pop len list =%d \n", g_list_length (undoList));
  }
  if(op>LAST_TASKS_OP && op<LAST_RSC_OP) {
     undo_pop_rsc (op, data);

  }
  if(op>LAST_RSC_OP && op<LAST_MISC_OP) {
     undo_pop_misc (op, data);
  }
  if(op>LAST_MISC_OP && op<LAST_DASH_OP) {
     undo_pop_dash (op, data);
  }

  /* we must free last data on undo list */
  if(undoList) {
      undo_free_last_data (data);
  }

  undo_update_menu_after_pop (data); 
}

/*********************************************
  PUBLIC : remove all undo datas from memory
  for example when we quit the
  program or call 'new' menu
*******************************************/
void undo_free_all (APP_data *data)
{
  GtkWidget *menuEditUndo, *menuEditRedo;

  menuEditUndo = GTK_WIDGET(gtk_builder_get_object (data->builder, "menuEditUndo"));
//  menuEditRedo = GTK_WIDGET(gtk_builder_get_object (data->builder, "menuEditRedo"));
  gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditUndo), _("Nothing to undo"));
//  gtk_menu_item_set_label (GTK_MENU_ITEM(menuEditRedo), _("Nothing to redo"));
  gtk_widget_set_sensitive (GTK_WIDGET (menuEditUndo), FALSE);
//  gtk_widget_set_sensitive (GTK_WIDGET (menuEditRedo), FALSE);

  while(undoList) {
     undo_free_last_data (data);
  }
  printf ("* Sucessfully freed Undo datas *\n");
  undoList = NULL;
}
