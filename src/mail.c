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
#include <curl/curl.h>
#include "support.h"
#include "misc.h"
#include "links.h"
#include "settings.h"
#include "mail.h"
#include "export.h"
#include "cursor.h"

/* constants */
#define MSG_MAX_LINES 1024
/* from curl website : https://curl.haxx.se/libcurl/c/progressfunc.html */
#if LIBCURL_VERSION_NUM >= 0x073d00
/* In libcurl 7.61.0, support was added for extracting the time in plain
   microseconds. Older libcurl versions are stuck in using 'double' for this
   information so we complicate this example a bit by supporting either
   approach. */ 
#define TIME_IN_US 1  
#define TIMETYPE curl_off_t
#define TIMEOPT CURLINFO_TOTAL_TIME_T
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3000000
#else
#define TIMETYPE double
#define TIMEOPT CURLINFO_TOTAL_TIME
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3
#endif

struct myprogress {
  TIMETYPE lastruntime; /* type depends on version, see above */ 
  CURL *curl;
};

typedef struct {
    gchar *str;
    gint id;
} receptors_elem;

/* 
  variables 
*/

static gchar *payload_text[MSG_MAX_LINES];
static GList *receptors = NULL;
static GtkWidget *progressBar;

struct upload_status {
  int lines_read;
};

/* ********************************************
  Curl progress function since libcurl 7.32

**********************************************/
static int xferinfo(void *p,
                    curl_off_t dltotal, curl_off_t dlnow,
                    curl_off_t ultotal, curl_off_t ulnow)
{
  static gboolean can_pulse = FALSE;
  struct myprogress *myp = (struct myprogress *)p;
  CURL *curl = myp->curl;
  TIMETYPE curtime = 0;
 
  curl_easy_getinfo(curl, TIMEOPT, &curtime);
 
  /* under certain circumstances it may be desirable for certain functionality
     to only run every N seconds, in order to do this the transaction time can
     be used */ 
  if((curtime - myp->lastruntime) >= MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL) {
    myp->lastruntime = curtime;
#ifdef TIME_IN_US
    fprintf(stderr, "TOTAL TIME: %" CURL_FORMAT_CURL_OFF_T ".%06ld\r\n",
            (curtime / 1000000), (long)(curtime % 1000000));
#else
    fprintf(stderr, "TOTAL TIME: %f \r\n", curtime);
#endif
  }
 /*
  fprintf(stderr, "UP: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
          "  DOWN: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
          "\r\n",
          ulnow, ultotal, dlnow, dltotal);
  */
  can_pulse = !can_pulse;
  if(can_pulse) {
     gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR(progressBar), 0.2); 
     gtk_progress_bar_pulse (GTK_PROGRESS_BAR(progressBar)); 
     gtk_main_iteration ();/* mandatory, in other cases system is too fast */
  }
  return 0;
}

/**************************************
  local : free PayLoad 
**************************************/
void collaborate_free_payload (void)
{
  gint i;

  /* completely free gchar datas */
  for(i=0; payload_text[i]; i++) {
    if(payload_text[i]!=NULL) {
       g_free (payload_text[i]);
    }
  }

}

/****************************************
  PUBLIC : 
  general alert message is user attempts
  to send overdue/today/ended and so on
  when ther isn't matching situation.
  for me, it's better than inhibit
  menu
****************************************/
gboolean collaborate_test_not_matching (gint mail_type, APP_data *data)
{
  gboolean flag = TRUE;
  gchar *tmpStr, *complStr;

  switch(mail_type) {
    case MAIL_TYPE_OVERDUE: {
      if(misc_count_tasks_overdue (data)<=0) {
         complStr = g_strdup_printf ("%s", _("overdue"));
         flag = FALSE;
      }
      break;
    }
    case MAIL_TYPE_COMPLETED: {
      if(misc_count_tasks_completed_yesterday (data)<=0) {
         complStr = g_strdup_printf ("%s", _("completed yesterday"));
         flag = FALSE;
      }
      break;
    }
    case MAIL_TYPE_INCOMING: {
      if(misc_count_tasks_tomorrow (data)<=0) {
         complStr = g_strdup_printf ("%s", _("starting tomorrow"));
         flag = FALSE;
      }
      break;
    }
    default: {
      complStr = g_strdup_printf ("%s", _("uncategorized"));
      flag = FALSE;/* force an error */
    }
  }/* end switch */
  if(!flag) {
       tmpStr = g_strdup_printf (_("Sorry !\nThere isn't any task %s.\nOperation aborted."), complStr);
       misc_ErrorDialog (data->appWindow, tmpStr);
       g_free (tmpStr);
       g_free (complStr);
  }
  return flag;
}


/****************************************
  general alert message before sending
  empty mail
  returns TRUE if user persists to send
  an empty mail
****************************************/
gboolean collaborate_alert_empty_message (GtkWidget *win)
{
  GtkWidget *dialog;
  GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
  gboolean flag = FALSE;

  dialog = gtk_message_dialog_new (GTK_WINDOW(win), flags,
                    GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL,
                _("Your current message is empty.\nDo you want to send it anyway ?"));
  if(gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
         flag = TRUE;
  }
  gtk_widget_destroy (GTK_WIDGET(dialog));
  return flag;
}

/**********************************
  LOCAL callback for toggle
  code borrowed from Ella ;-)
***********************************/

static void on_rsc_use_toggled (GtkCellRendererToggle *cell, gchar *path, APP_data *data) 
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreePath *path_tree;
    GtkTreeIter iter;	
    gboolean value;
    gchar *line;
    gint item, line_called;

    /*  we must check if selection and check are at the same rox  */
    line_called = atoi (path);/* yes, in callback, the conversion tree_path -> path is already done */
    /* get current line for toggling event */
    selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW(data->treeViewReportMail) );
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE );

    if(gtk_tree_selection_get_selected (selection, &model, &iter)) {
	     gtk_tree_model_get (model, &iter, -1);
	     /* read path  */
	     path_tree = gtk_tree_model_get_path (model, &iter);
	     line = gtk_tree_path_to_string (path_tree);
             item = atoi (line);
             if(item==line_called) {
		 gtk_tree_model_get (model, &iter, USE_REP_COLUMN, &value, -1);
		 /* change display of cehckbox */
		 value = !value;
		 gtk_list_store_set (GTK_LIST_STORE(model), &iter, USE_REP_COLUMN, value ,-1);
             }/* endif line */
	     g_free (line);	
	     gtk_tree_path_free (path_tree);	
    }/* endif */
}

/*****************************************************************************************************
  LOCAL :
  fill tree view and
  list store for ressources

*****************************************************************************************************/

static void fill_list_rsc (APP_data *data)
{
  GtkListStore *store;
  GtkTreeIter iter, toplevel;
  GtkTreeModel *model;
  gint i;
  GList *l;
  GdkPixbuf *pix2;
  gboolean valid = FALSE;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW(data->treeViewReportMail));
  store = GTK_LIST_STORE (model);

  for(i=0; i<g_list_length (data->rscList); i++) {
     gtk_list_store_append (store, &toplevel);
     l = g_list_nth (data->rscList, i);
     rsc_datas *tmp_rsc_datas;
     tmp_rsc_datas = (rsc_datas *)l->data;

     /* avatar */
     if(tmp_rsc_datas->pix ) {
          pix2 = gdk_pixbuf_scale_simple (tmp_rsc_datas->pix, TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, GDK_INTERP_BILINEAR);
     }
     else {
        pix2 = gdk_pixbuf_new ( GDK_COLORSPACE_RGB, TRUE, 8, TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE);
        gdk_pixbuf_fill (pix2, 0x00000000);/* black totaly transparent */
     }
     gtk_list_store_set (store, &toplevel, AVATAR_REP_COLUMN, pix2, -1);
     g_object_unref (pix2);
     /* by default, all ressources will receive mails */
     gtk_list_store_set (store, &toplevel, USE_REP_COLUMN, TRUE, -1);
     /* name */
     gtk_list_store_set (store, &toplevel, NAME_REP_COLUMN, g_strdup_printf ("%s", tmp_rsc_datas->name), -1);
     /* id */
     gtk_list_store_set (store, &toplevel, ID_REP_COLUMN, g_strdup_printf ("%d", tmp_rsc_datas->id), -1);
     /* mail */
     if(tmp_rsc_datas->mail!=NULL){
        gtk_list_store_set (store, &toplevel, ID_MAIL_COLUMN, g_strdup_printf ("%s", tmp_rsc_datas->mail), -1);
     }
     else {
        gtk_list_store_set (store, &toplevel, ID_MAIL_COLUMN, g_strdup_printf ("%s", "-" ), -1);
     }
  }

}

/***********************************
  LOCAL :
  init tree view and
  list store for ressources

***********************************/

static void collaborate_init_list_rsc (APP_data *data)
{
   GtkListStore *store = NULL;
   GtkCellRenderer *renderer;
   GtkTreeViewColumn *column;

  /* toggle */
  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Send to"),
          renderer, "active", USE_REP_COLUMN, NULL);
  g_object_set (renderer,"activatable" , TRUE, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(data->treeViewReportMail), column);

  /* toggle callback */
  g_signal_connect (renderer, "toggled", G_CALLBACK(on_rsc_use_toggled), data);
  /* avatar */
  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Avatar"),
        				renderer,
        				"pixbuf", AVATAR_REP_COLUMN,
        				NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(data->treeViewReportMail), column); 
  /* ressource name */
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT(renderer), "width-chars", 32, NULL);    
  g_object_set (G_OBJECT(renderer),"font","sans 10",NULL); 
  column = gtk_tree_view_column_new_with_attributes (_("Ressource name "),
        			renderer,
        			"text", NAME_REP_COLUMN,
        			NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(data->treeViewReportMail), column);
  /* ressource id */
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT(renderer), "width-chars", 8, NULL);    
  g_object_set (G_OBJECT(renderer),"font","sans 10",NULL); 
  column = gtk_tree_view_column_new_with_attributes (_(" Id "),
        			renderer,
        			"text", ID_REP_COLUMN,
        			NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(data->treeViewReportMail), column); 
  /* mail */
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT(renderer), "width-chars", 42, NULL);    
  g_object_set (G_OBJECT(renderer),"font","sans 10",NULL); 
  column = gtk_tree_view_column_new_with_attributes (_(" Email "),
        			renderer,
        			"text", ID_MAIL_COLUMN,
        			NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(data->treeViewReportMail), column); 
  /*----*/
  store = gtk_list_store_new (NB_COL_REP, G_TYPE_BOOLEAN, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );
  gtk_tree_view_set_model (GTK_TREE_VIEW(data->treeViewReportMail), 
                          GTK_TREE_MODEL(store));

  g_object_unref (store);/* then, will be automatically destroyed with view */

  gtk_widget_show_all (data->treeViewReportMail);
}

/*****************************************************************************************************
   LOCAL 
   update recipient list (for reports)

*****************************************************************************************************/
static void report_update_list (APP_data *data )
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  gboolean valid = FALSE, status;
  gchar *str_data, *int_data, *mail_data;
  receptors_elem *tmpElem;

  gint total = g_list_length (data->rscList);
  model = gtk_tree_view_get_model (GTK_TREE_VIEW(data->treeViewReportMail));
  /* get list's first iterator */
  valid = gtk_tree_model_get_iter_first (model, &iter);
  if( !valid)
     return;

  /* Ok, we continue, listore isn't empty */
  while(valid)  {
      gtk_tree_model_get (model, &iter,  USE_REP_COLUMN, &status,
                                         NAME_REP_COLUMN, &str_data,
                                         ID_REP_COLUMN, &int_data,  
                                         ID_MAIL_COLUMN, &mail_data, -1);

      /* do something with those datas */
      if(&iter) {
         if(status) {
            if(g_ascii_strncasecmp ((gchar*)  mail_data, "-", sizeof(gchar))  !=  0 ) {
               tmpElem = g_malloc (sizeof(receptors_elem));
               tmpElem->str = g_strdup_printf ("%s", mail_data);
               tmpElem->id = atoi (int_data);
               receptors = g_list_append (receptors, tmpElem);/* print mandatory to FREE */
            }
         }/* endif status */
         g_free (str_data);
         g_free (int_data);
         g_free (mail_data);
      }/* endif iter */
      valid = gtk_tree_model_iter_next (model, &iter);
  }/* wend */

}

/**************************************

  from curl website examples, thanks

***************************************/

static size_t payload_source (void *ptr, size_t size, size_t nmemb, void *userp)
{
  struct upload_status *upload_ctx = (struct upload_status *)userp;
  const gchar *data;

  if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
    return 0;
  }

  data = payload_text[upload_ctx->lines_read];

  if(data) {
    size_t len = strlen (data);
    memcpy(ptr, data, len*sizeof(gchar));
    upload_ctx->lines_read++;
    return len;
  }

  return 0;
}
/******************************************

   Prepare a formated mail, compliant
   to RFC standard.
******************************************/

gchar **misc_prepare_mail (gchar *msg)
{
   size_t current_size = 0;
   size_t len = 0, line_count = 0, max_line_len = RFC_LINE_LEN+1;/* 77 because the RFC std line is up to 78 chars */
   gint i, result;
   gchar **array = malloc((current_size+1) * sizeof(*array));
   gchar **tmp;
   gchar str[90];/* buffer for each lines - RFC standard : 78 chers by line */

   len = strlen (msg);
   i = 0;
   while(i<=len) {
     if(msg[i]!='\n') {
        str[line_count] = msg[i];
        line_count++;
        if((line_count>(max_line_len-10)) && (i<=len)) {
            if(msg[i]==' ') {
               str[line_count] = 0;/* EOL */
               line_count = 0;
               if(array) {
                 array[current_size] = g_strdup_printf ("%s\r\n", str);/* \r\n required for RFC compatibility */
                 current_size++;
                 tmp = realloc (array, (current_size+1) * sizeof(*array));
                 if(tmp){
                   array = tmp;
                 }
                 else
                    return NULL; /* memory leaks is better than segfaults */
               }/* endif array */    
            }
        }
        if(line_count>max_line_len) {
           str[line_count] = 0;
           line_count = 0;
           if(array) {
              array[current_size] = g_strdup_printf ("%s\r\n", str);
              current_size++;
              tmp = realloc (array, (current_size+1) * sizeof(*array));
              if(tmp){
                   array = tmp;
              }
              else
                 return NULL; /* memory leaks is better than segfaults */
           }/* endif array */
        }
     }/* endif \n */
     else {
        str[line_count] = 0;
        if(array) {
          array[current_size] = g_strdup_printf ("%s\r\n", str);
          current_size++;
          tmp = realloc (array, (current_size+1) * sizeof(*array));
          if(tmp){
             array = tmp;
          }
          else
            return NULL; /* memory leaks is better than segfaults */
        }/* endif array */
        line_count = 0;
     }
     i++;
   }/* wend */

   /* last line ! */
   str[line_count] = 0;
   if(array) {
          array[current_size] = g_strdup_printf ("%s\r\n", str);
          current_size++;
          tmp = realloc (array, (current_size+1) * sizeof(*array));
          if(tmp){
             array = tmp;
   }
   else
      return NULL; /* memory leaks is better than segfaults */
   }/* endif array */
   array[current_size] = NULL;
   return array;
}


/**********************************
  PUBLIC : free a GList of
  recipients ; it's a very simple
  list of gchar* containing
  e-mail adresses
**********************************/
void collaborate_free_recipients (void)
{
  GList *l;
  receptors_elem *tmpElem;

  if(receptors == NULL)
     return;

  /* free recipents list */
  l = g_list_first (receptors);
  for(l; l!=NULL; l=l->next) {
     tmpElem = (receptors_elem *) l->data;
     if(tmpElem->str != NULL)
        g_free (tmpElem->str);
  }
  g_list_free (receptors);
  receptors = NULL;
}

/**********************************
  LOCAL : set-up recipients
  list for CURRENT (id) tasks
  whatever is the task' status
**********************************/
void collaborate_set_recipients_list (gint id, APP_data *data)
{
  GList *l;
  rsc_datas *tmp_rsc_datas;
  receptors_elem *tmpElem;
  gint i, ok = -1;
  gint total = g_list_length (data->rscList);
 
  for(i=0; i<total; i++) {
     l = g_list_nth (data->rscList, i);
     tmp_rsc_datas = (rsc_datas *)l->data;
     ok = links_check_element (id, tmp_rsc_datas->id , data);
     if(ok>=0) {
        if(tmp_rsc_datas->mail != NULL) {
            tmpElem = g_malloc (sizeof(receptors_elem));
            tmpElem->id = tmp_rsc_datas->id;
            tmpElem->str = g_strdup_printf ("%s", tmp_rsc_datas->mail);
            receptors = g_list_append (receptors, tmpElem);/* print mandatory to FREE */
        }
     }/* endif OK */ 
  }/* next i */
}
/**********************************
  PUBLIC :
  counts how many tasks are overdue
  and send mails
  PLEASE NOTE : use only when
  status are updated
**********************************/
gint collaborate_send_recipients_tasks_overdue (gboolean isFree, gchar *subject, gchar **tabMsg, gchar *server, gint port, gchar *username, gchar *password, gboolean f_isSsl, APP_data *data)
{
  gchar *tmpSubject, *messages[10];
  tasks_data *tmp_tsk_datas;
  GList *l;
  gint i, ret = 0;
  gint total = g_list_length (data->tasksList);
  GDate date;
  CURLcode res = CURLE_OK;

  for(i=0; i<total; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if( tmp_tsk_datas->type == TASKS_TYPE_TASK) {
        /* test if overdue */
        g_date_set_dmy (&date, tmp_tsk_datas->end_nthDay, tmp_tsk_datas->end_nthMonth, tmp_tsk_datas->end_nthYear);
        if( misc_task_is_overdue (tmp_tsk_datas->progress, &date) ) {
           collaborate_free_recipients ();
           /* now, we looking for ALL assigned ressources for current task */
           collaborate_set_recipients_list (tmp_tsk_datas->id, data);
           /* and send an email to THOSE ressources for THIS task */
           if(!isFree) {
		   tmpSubject = g_strdup_printf ("%s", _("Task(s) overdue"));
		   messages[0] = g_strdup_printf ("%s", _("Dear participant,\r\n"));
		   messages[1] = g_strdup_printf (_("As a member of project %s, \r\n"), data->properties.title);
		   messages[2] = g_strdup_printf (_("you're assigned to task : %s.\r\n"), tmp_tsk_datas->name);
		   messages[3] = g_strdup_printf ("%s", _("We've noticed that this task is, today, overdue.\r\n"));
		   messages[4] = g_strdup_printf ("%s", _("We hope that a significant progress will be accomplished.\r\n"));
		   messages[5] = g_strdup_printf ("%s", _("Best regards.\r\n"));
		   messages[6] = g_strdup_printf ("%s", _("Your project manager.\r\n"));
		   messages[7] = NULL;
           }
           else {
              if(subject != NULL)
                  tmpSubject = g_strdup_printf ("%s", subject);
              else
                  tmpSubject = g_strdup_printf (_("Project information about task %s"), tmp_tsk_datas->name);
              /* and now we build a 'clean' message from GtkTextView content */
           }

           /* send mail for current task */
           if(g_list_length (receptors)>0){
               printf("------------  expédie pour tâche %s--------------\n", tmp_tsk_datas->name);
               if(!isFree) {  
                 res = collaborate_send_email (tmpSubject, data->properties.manager_mail, receptors, 
                            messages, server, port, username, password, f_isSsl );
               }
               else {
                 res = collaborate_send_email (tmpSubject, data->properties.manager_mail, receptors, 
                            tabMsg, server, port, username, password, f_isSsl );
               }
               if((gint)res != 0)
                   ret = (gint)res;
           }/* endif recptors */

           if(!isFree) {
              g_free (messages[0]);
              g_free (messages[1]);
              g_free (messages[2]);
              g_free (messages[3]);
              g_free (messages[4]);
              g_free (messages[5]);
              g_free (messages[6]);
           }
           g_free (tmpSubject); 
        }
     }/* endif type task */
  }/* next */
  return ret;
}

/**********************************
  PUBLIC :
  counts how many tasks which
  starts tomorrow
  and send mails
  PLEASE NOTE : use only when
  status are updated
**********************************/
gint collaborate_send_recipients_tasks_start_tomorrow (gboolean isFree, gchar *subject, gchar **tabMsg, gchar *server, gint port, gchar *username, gchar *password, gboolean f_isSsl, APP_data *data)
{
  gchar *tmpSubject, *messages[10];
  tasks_data *tmp_tsk_datas;
  GList *l;
  gint i, ret = 0;
  gint total = g_list_length (data->tasksList);
  GDate date, current_date;
  CURLcode res = CURLE_OK;

  g_date_set_time_t (&current_date, time (NULL)); /* date of today */

  for(i=0; i<total; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if( tmp_tsk_datas->type == TASKS_TYPE_TASK) {
        /* test if overdue */
        g_date_set_dmy (&date, tmp_tsk_datas->start_nthDay, tmp_tsk_datas->start_nthMonth, tmp_tsk_datas->start_nthYear);
        if( g_date_compare (&date, &current_date)==1 ) {
           collaborate_free_recipients ();
           /* now, we looking for ALL assigned ressources for current task */
           collaborate_set_recipients_list (tmp_tsk_datas->id, data);
           /* and send an email to THOSE ressources for THIS task */
           if(!isFree) {
		   tmpSubject = g_strdup_printf ("%s", _("Task(s) starting tomorrow"));
		   messages[0] = g_strdup_printf ("%s", _("Dear participant,\r\n"));
		   messages[1] = g_strdup_printf (_("As a member of project %s, \r\n"), data->properties.title);
		   messages[2] = g_strdup_printf (_("you're assigned to task : %s.\r\n"), tmp_tsk_datas->name);
		   messages[3] = g_strdup_printf ("%s", _("We want to remind you that this task will start tomorrow.\r\n"));
		   messages[4] = g_strdup_printf ("%s", _("Do not hesitate to let us know if you have any needs .\r\n"));
		   messages[5] = g_strdup_printf ("%s", _("Best regards.\r\n"));
		   messages[6] = g_strdup_printf ("%s", _("Your project manager.\r\n"));
		   messages[7] = NULL;
           }

           /* send mail for current task */
           if(g_list_length (receptors)>0){
               printf("------------  expédie pour tâche %s--------------\n", tmp_tsk_datas->name);
               if(!isFree) {  
                 res = collaborate_send_email (tmpSubject, data->properties.manager_mail, receptors, 
                            messages, server, port, username, password, f_isSsl );
               }
               else {
                 res = collaborate_send_email (tmpSubject, data->properties.manager_mail, receptors, 
                            tabMsg, server, port, username, password, f_isSsl );
               }
               if((gint)res != 0)
                   ret = (gint)res;
           }/* endif recptors */

           if(!isFree) {
              g_free (messages[0]);
              g_free (messages[1]);
              g_free (messages[2]);
              g_free (messages[3]);
              g_free (messages[4]);
              g_free (messages[5]);
              g_free (messages[6]);
           }
           g_free (tmpSubject); 
        }
     }/* endif type task */
  }/* next */
  return ret;
}

/**********************************
  PUBLIC :
  counts how many tasks which
  ended since yesterday
  and send mails
  PLEASE NOTE : use only when
  status are updated
**********************************/
gint collaborate_send_recipients_tasks_ended_yesterday (gboolean isFree, gchar *subject, gchar **tabMsg, gchar *server, gint port, gchar *username, gchar *password, gboolean f_isSsl, APP_data *data)
{
  gchar *tmpSubject, *messages[10];
  tasks_data *tmp_tsk_datas;
  GList *l;
  gint i, ret = 0;
  gint total = g_list_length (data->tasksList);
  GDate date, current_date;
  CURLcode res = CURLE_OK;

  g_date_set_time_t (&current_date, time (NULL)); /* date of today */

  for(i=0; i<total; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if( tmp_tsk_datas->type == TASKS_TYPE_TASK) {
        /* test if overdue */
        g_date_set_dmy ( &date, tmp_tsk_datas->end_nthDay, tmp_tsk_datas->end_nthMonth, tmp_tsk_datas->end_nthYear);
        if( g_date_compare (&date, &current_date)==-1 ) {
           collaborate_free_recipients ();
           /* now, we looking for ALL assigned ressources for current task */
           collaborate_set_recipients_list (tmp_tsk_datas->id, data);
           /* and send an email to THOSE ressources for THIS task */
           if(!isFree) {
		   tmpSubject = g_strdup_printf ("%s", _("Task(s) completed yesterday"));
		   messages[0] = g_strdup_printf ("%s", _("Dear participant,\r\n"));
		   messages[1] = g_strdup_printf (_("As a member of project %s, \r\n"), data->properties.title);
		   messages[2] = g_strdup_printf (_("you've been assigned to task : %s.\r\n"), tmp_tsk_datas->name);
		   messages[3] = g_strdup_printf ("%s", _("We want to thank you.\r\n"));
		   messages[4] = g_strdup_printf ("%s", _("This task is completed since yesterday.\r\n"));
		   messages[5] = g_strdup_printf ("%s", _("Best regards.\r\n"));
		   messages[6] = g_strdup_printf ("%s", _("Your project manager.\r\n"));
		   messages[7] = NULL;
           }
           /* send mail for current task */
           if(g_list_length (receptors)>0){
               printf("------------  expédie pour tâche %s--------------\n", tmp_tsk_datas->name);
               if(!isFree) {  
                 res = collaborate_send_email (tmpSubject, data->properties.manager_mail, receptors, 
                            messages, server, port, username, password, f_isSsl );
               }
               else {
                 res = collaborate_send_email (tmpSubject, data->properties.manager_mail, receptors, 
                            tabMsg, server, port, username, password, f_isSsl );
               }
               if((gint)res != 0)
                   ret = (gint)res;
           }/* endif recptors */

           if(!isFree) {
              g_free (messages[0]);
              g_free (messages[1]);
              g_free (messages[2]);
              g_free (messages[3]);
              g_free (messages[4]);
              g_free (messages[5]);
              g_free (messages[6]);
           }
           g_free (tmpSubject); 
        }
     }/* endif type task */
  }/* next */
  return ret;
}


/**********************************
  PUBLIC :
  counts how many tasks which
  ends today
  and send mails
  PLEASE NOTE : use only when
  status are updated
**********************************/
gint collaborate_send_recipients_tasks_ending_today (gboolean isFree, gchar *subject, gchar **tabMsg, gchar *server, gint port, gchar *username, gchar *password, gboolean f_isSsl, APP_data *data)
{
  gchar *tmpSubject, *messages[10];
  tasks_data *tmp_tsk_datas;
  GList *l;
  gint i, ret = 0;
  gint total = g_list_length (data->tasksList);
  GDate date, current_date;
  CURLcode res = CURLE_OK;

  g_date_set_time_t (&current_date, time (NULL)); /* date of today */

  for(i=0; i<total; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if( tmp_tsk_datas->type == TASKS_TYPE_TASK) {
        /* test if overdue */
        g_date_set_dmy ( &date, tmp_tsk_datas->end_nthDay, tmp_tsk_datas->end_nthMonth, tmp_tsk_datas->end_nthYear);
        if( g_date_compare (&date, &current_date)==0 ) {
           collaborate_free_recipients ();
           /* now, we looking for ALL assigned ressources for current task */
           collaborate_set_recipients_list (tmp_tsk_datas->id, data);
           /* and send an email to THOSE ressources for THIS task */
           if(!isFree) {
		   tmpSubject = g_strdup_printf ("%s", _("Task(s) completed yesterday"));
		   messages[0] = g_strdup_printf ("%s", _("Dear participant,\r\n"));
		   messages[1] = g_strdup_printf (_("As a member of project %s, \r\n"), data->properties.title);
		   messages[2] = g_strdup_printf (_("you've been assigned to task : %s.\r\n"), tmp_tsk_datas->name);
		   messages[3] = g_strdup_printf ("%s", _("We want to remind you that this task should be completed today.\r\n"));
		   messages[4] = g_strdup_printf ("%s", _("Don't hesitate to let us know if you have any needs.\r\n"));
		   messages[5] = g_strdup_printf ("%s", _("Best regards.\r\n"));
		   messages[6] = g_strdup_printf ("%s", _("Your project manager.\r\n"));
		   messages[7] = NULL;
           }
           /* send mail for current task */
           if(g_list_length (receptors)>0){
               printf("------------  expédie pour tâche %s--------------\n", tmp_tsk_datas->name);
               if(!isFree) {  
                 res = collaborate_send_email (tmpSubject, data->properties.manager_mail, receptors, 
                            messages, server, port, username, password, f_isSsl );
               }
               else {
                 res = collaborate_send_email (tmpSubject, data->properties.manager_mail, receptors, 
                            tabMsg, server, port, username, password, f_isSsl );
               }
               if((gint)res != 0)
                   ret = (gint)res;
           }/* endif recptors */

           if(!isFree) {
              g_free (messages[0]);
              g_free (messages[1]);
              g_free (messages[2]);
              g_free (messages[3]);
              g_free (messages[4]);
              g_free (messages[5]);
              g_free (messages[6]);
           }
           g_free (tmpSubject); 
        }
     }/* endif type task */
  }/* next */
  return ret;
}
/**********************************
  PUBLIC : send an email

**********************************/
gint collaborate_send_email (gchar *subject, gchar *from, GList *list_recipients, 
                            gchar **message, gchar *server, gint port, gchar *username, gchar *password, gboolean f_isSsl )
{
  gchar *tmpStr, *result;
  gint i, count, total;
  time_t rawtime; 
  GList *l;
  CURL *curl;
  CURLcode res = CURLE_OK;
  struct curl_slist *recipients = NULL;
  struct upload_status upload_ctx;
  struct myprogress prog;

  count = 0;
  /* we get current time for message identifier */
  time (&rawtime);
  /* we add current date */
  tmpStr = g_strdup_printf ("Date: %s \r\n", ctime (&rawtime));/* in 'C' format for compatibility */
  payload_text[count] = g_strdup_printf ("%s", tmpStr);
  count++;
  g_free (tmpStr);

  /* we append field TO */
  tmpStr = g_strdup_printf ("To: %s \r\n", from);
  payload_text[count] = g_strdup_printf ("%s", tmpStr);
  count++;
  g_free (tmpStr);
  /* we append field FROM */
  tmpStr = g_strdup_printf ("From: %s \r\n", from);
  payload_text[count] = g_strdup_printf ("%s", tmpStr);
  count++;
  g_free (tmpStr);
  /* message identifier - two parts */
  tmpStr = g_strdup_printf ("Message-ID: <%d@", (gint) rawtime);
  payload_text[count] = g_strdup_printf ("%s", tmpStr);
  count++;
  g_free (tmpStr);
  payload_text[count] = g_strdup_printf ("%s", "PlanLibre.gtk.org>\r\n");
  count++;
  /* subject : base 64 encoding */
  result = g_base64_encode (subject, strlen (subject));
  tmpStr = g_strdup_printf ("Subject: =?utf-8?B?%s?= \r\n", result);
  payload_text[count] = g_strdup_printf ("%s", tmpStr);
  count++;
  g_free (tmpStr);
  g_free (result);

  /* coding of content */
  payload_text[count] = g_strdup_printf ("%s", "content-Type: text/plain; charset=utf-8\r\n");
  count++;
  /* message itself */
  /* empty line to divide headers from body, see RFC5322 */
  payload_text[count] = g_strdup_printf ("%s", "\r\n");
  count++;
  /* text part = message */
  for(i=0; message[i]; i++) {
     /* minimal protection against overflow */
     if(count<MSG_MAX_LINES-20) {
        payload_text[count] = g_strdup_printf ("%s", message[i]);
        count++;
     }
  }


  /* a final line to ensure CR/LF coding */
  payload_text[count] = g_strdup_printf ("%s", "\r\n");
  count++;
  /* trailing NULL in order to avoid segfaults */
  payload_text[count] = NULL;

  /* libCurl part of job */
  upload_ctx.lines_read = 0;

  curl = curl_easy_init();
  if(curl) {
    /* threads */
    prog.lastruntime = 0;
    prog.curl = curl;
    curl_easy_setopt (curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
    curl_easy_setopt (curl, CURLOPT_XFERINFODATA, &prog);
    curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 0L);

    curl_easy_setopt (curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt (curl, CURLOPT_USERNAME, username );
    curl_easy_setopt (curl, CURLOPT_PASSWORD, password );
    /* server */
    if(f_isSsl) {
       tmpStr = g_strdup_printf ("smtps://%s:%d", server, port );
    }
    else {
       tmpStr = g_strdup_printf ("smtp://%s.fr:%d", server, port );    
    }
    curl_easy_setopt (curl, CURLOPT_URL, tmpStr);// smpts (with 's' for port 465 because of SSL 
    g_free (tmpStr);

    curl_easy_setopt (curl, CURLOPT_MAIL_FROM, from);
    /* Force PLAIN authentication */ 
    curl_easy_setopt (curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
    /* dynamic list of recipents */
    total = g_list_length (list_recipients);
    /* set-up curlist of recipients */
    for(i=0; i<total; i++) {
      l = g_list_nth (list_recipients, i);
      receptors_elem *tmpElem;
      tmpElem = (receptors_elem *) l->data;
      recipients = curl_slist_append (recipients, tmpElem->str);
    }

    curl_easy_setopt (curl, CURLOPT_MAIL_RCPT, recipients);
    curl_easy_setopt (curl, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt (curl, CURLOPT_READDATA, &upload_ctx);
    curl_easy_setopt (curl, CURLOPT_UPLOAD, 1L);

    res = curl_easy_perform (curl);
    /* Check for errors */
    if(res != CURLE_OK)
       fprintf (stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror (res));

    curl_slist_free_all (recipients);
    curl_easy_cleanup (curl);
  }

  /* completely free gchar datas */
  collaborate_free_payload ();
  return (int)res;
}

/**********************************
  PUBLIC : send an email
  thanks to C++ coders : https://www.codeproject.com/questions/714669/sending-email-with-attachment-using-libcurl
**********************************/
gint collaborate_send_multipart_email (gchar *subject, gchar *from, GList *list_recipients, 
                            gchar **message, gchar *server, gint port, gchar *username, gchar *password, 
                            gboolean f_isSsl, gchar *path_to_file )
{
  size_t filesize, len64;
  guint8 *buffer;
  gchar *tmpStr, *result, *filename;
  glong prev, sz;
  gint i, count, total, j = 0, k = 0, nb_lines, epsilon;
  time_t rawtime; 
  GList *l;
  CURL *curl;
  CURLcode res = CURLE_OK;
  struct curl_slist *recipients = NULL;
  struct upload_status upload_ctx;
  struct myprogress prog;

  count = 0;
  /* we get current time for message identifier */
  time (&rawtime);
  /* we add current date */
  tmpStr = g_strdup_printf ("Date: %s \r\n", ctime (&rawtime));/* in 'C' format for compatibility */
  payload_text[count] = g_strdup_printf ("%s", tmpStr);
  count++;
  g_free (tmpStr);

  /* we append field TO */
  tmpStr = g_strdup_printf ("To: %s \r\n", from);
  payload_text[count] = g_strdup_printf ("%s", tmpStr);
  count++;
  g_free (tmpStr);
  /* we append field FROM */
  tmpStr = g_strdup_printf ("From: %s \r\n", from);
  payload_text[count] = g_strdup_printf ("%s", tmpStr);
  count++;
  g_free (tmpStr);
  /* message identifier - two parts */
  tmpStr = g_strdup_printf ("Message-ID: <%d@", (gint) rawtime);
  payload_text[count] = g_strdup_printf ("%s", tmpStr);
  count++;
  g_free (tmpStr);
  payload_text[count] = g_strdup_printf ("%s", "PlanLibre.gtk.org>\r\n");
  count++;
  /* subject : base 64 encoding */
  result = g_base64_encode(subject, strlen(subject));
  tmpStr = g_strdup_printf ("Subject: =?utf-8?B?%s?= \r\n", result);
  payload_text[count] = g_strdup_printf ("%s", tmpStr);
  count++;
  g_free (tmpStr);
  g_free (result);
  /* coding of content */
  payload_text[count] = g_strdup_printf ("%s", "Content-Type: multipart/mixed;\r\n boundary=\"XXXXXMyBoundry\"\r\n");
  count++;
  payload_text[count] = g_strdup_printf ("%s", "Mime-Version: 1.0\r\n\r\n"); /* empty line to divide headers from body, see RFC5322 */
  count++;
 
  /* message  'text' section */
  payload_text[count] = g_strdup_printf ("%s", "--XXXXXMyBoundry\r\n");/* required section separator */
  count++;
  payload_text[count] = g_strdup_printf ("%s", "Content-Type: text/plain; charset=\"UTF-8\"\r\n");
  count++;
  payload_text[count] = g_strdup_printf ("%s", "Content-Transfer-Encoding: quoted-printable\r\n\r\n");
  count++;

  /* text part = message */
  for(i=0; message[i]; i++) {
     /* minimal protection against overflow */
     if(count < MSG_MAX_LINES-20) {
        payload_text[count] = g_strdup_printf ("%s", message[i]);
        count++;
     }
  }
  /* a line to ensure CR/LF coding */
  payload_text[count] = g_strdup_printf ("%s", "\r\n");
  count++;

  /* embeded file = attached file */
  payload_text[count] = g_strdup_printf ("%s", "--XXXXXMyBoundry\r\n");/* required section separator */
  count++;
  filename = g_filename_display_basename (path_to_file);
  if(filename == NULL)
     return -1;
  /* we compute size of file on disk */
  FILE* inputFile = NULL;
  inputFile = fopen (path_to_file, "rb");
  if(inputFile == NULL) {
          printf("* ERROR : can't open attached file : %s *\n", path_to_file);
          return -1;
  }
  /* we compute the size before dynamically allocate buffer */
  prev = ftell (inputFile);   
  fseek (inputFile, 0L, SEEK_END);
  sz = ftell (inputFile);
  fseek (inputFile, prev, SEEK_SET);
  /* we allocate the buffer */
  if(sz<=0)
        return -1;
  buffer = g_malloc0 (sz*sizeof (guint8)+sizeof (guint8));
  filesize = fread (buffer, sizeof(guint8), sz, inputFile);
  fclose (inputFile);

  /* we declare section for embedded file */
  payload_text[count] = g_strdup_printf ("Content-Type: application/x-msdownload; name=\"%s\"\r\n", filename);/* base name */
  count++;
  payload_text[count] = g_strdup_printf ("%s", "Content-Transfer-Encoding: base64\r\n");
  count++;
  payload_text[count] = g_strdup_printf ("Content-Disposition: attachment; filename=\"%s\"\r\n", filename);
  count++;
  if(filename != NULL)
      g_free (filename);
  /* a line to ensure CR/LF coding */
  payload_text[count] = g_strdup_printf ("%s", "\r\n");
  count++;

  /* we convert buffer containing file's data to base64 */
  result = g_base64_encode (buffer, filesize);
  /* we print content of base64 string in RFC compliant 78 chars widths */
  len64 = strlen (result);
  nb_lines = len64/76;
  epsilon = len64 - (76*nb_lines);


  for(j=0; j<nb_lines; j++) {
     tmpStr = g_strndup (result+k, RFC_LINE_LEN);
     /* minimal protection against overflow */
     if(count<MSG_MAX_LINES-10) {
        payload_text[count] = g_strdup_printf ("%s\r\n", tmpStr);
        count++;
     }
     g_free (tmpStr);
     k = k+RFC_LINE_LEN;
  }
  if(epsilon>0) {
     tmpStr = g_strndup (result+k, epsilon);
     payload_text[count] = g_strdup_printf ("%s\r\n", tmpStr);
     g_free (tmpStr);
     count++;
  }


  g_free (result);
  g_free (buffer);

  payload_text[count] = g_strdup_printf ("%s", "--XXXXXMyBoundry\r\n");/* required section separator */
  count++;

  /* a final line to ensure CR/LF coding */
  payload_text[count] = g_strdup_printf ("%s", "\r\n");
  count++;
  /* trailing NULL in order to avoid segfaults */
  payload_text[count] = NULL;
/* TODO : remove test du contenu */
  for(i=0; payload_text[i]; i++) {
    // printf(payload_text[i]);
  }

  /* libCurl part of job */
  upload_ctx.lines_read = 0;

  curl = curl_easy_init();
  if(curl) {
    /* threads */
    prog.lastruntime = 0;
    prog.curl = curl;
    curl_easy_setopt (curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
    curl_easy_setopt (curl, CURLOPT_XFERINFODATA, &prog);
    curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 0L);

    curl_easy_setopt (curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt (curl, CURLOPT_USERNAME, username );
    curl_easy_setopt (curl, CURLOPT_PASSWORD, password );
    /* server */
    if(f_isSsl) {
       tmpStr = g_strdup_printf ("smtps://%s:%d", server, port );
    }
    else {
       tmpStr = g_strdup_printf ("smtp://%s.fr:%d", server, port );    
    }
    curl_easy_setopt (curl, CURLOPT_URL, tmpStr);// smpts (with 's' for port 465 because of SSL 
    g_free (tmpStr);

    curl_easy_setopt (curl, CURLOPT_MAIL_FROM, from);
    /* Force PLAIN authentication */ 
    curl_easy_setopt (curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
    /* dynamic list of recipents */
    total = g_list_length (list_recipients);
    /* curl list of recipients */
    for(i=0; i<total; i++) {
      l = g_list_nth (list_recipients, i);
      receptors_elem *tmpElem;
      tmpElem = (receptors_elem *) l->data;
      recipients = curl_slist_append (recipients, tmpElem->str);
    }

    curl_easy_setopt (curl, CURLOPT_MAIL_RCPT, recipients);
    curl_easy_setopt (curl, CURLOPT_INFILESIZE, filesize);/* filesize : size of atatched file once converted to base64 */
    curl_easy_setopt (curl, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt (curl, CURLOPT_READDATA, &upload_ctx);
    curl_easy_setopt (curl, CURLOPT_UPLOAD, 1L);

    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK)
      fprintf (stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror (res));

    curl_slist_free_all (recipients);
    curl_easy_cleanup (curl);
  }

  /* completely free gchar datas */
  collaborate_free_payload ();
  return (int)res;
}


/***********************************
  PUBLIC : test project manager
  mail
***********************************/
gint collaborate_test_project_manager_mail (APP_data *data)
{
  gint ret = 0, i, count_arrobas =0 , count_space = 0;
  gchar *tmpStr;
  gboolean fErr = FALSE;

  if(data->properties.manager_mail == NULL) {
      fErr = TRUE;
  }
  else {/* we test if email 'seems' OK */
     for(i=0; i<=strlen (data->properties.manager_mail); i++) {
        if(data->properties.manager_mail[i]==' ')
           count_space++;
        if(data->properties.manager_mail[i]=='@')
           count_arrobas++;
     }/* next */       
  }
  if(count_space>0)
     fErr = TRUE;
  if((count_arrobas ==0) || (count_arrobas>1))
     fErr = TRUE;
  if(fErr) {
        tmpStr = g_strdup_printf ("%s", _("Sorry !\nMail sending requires a valid email adress for project manager.\nOperation aborted. Please, see :\n<b>File>properties</b> menu."));
        misc_ErrorDialog (data->appWindow, tmpStr );
        g_free (tmpStr);
        ret = -1;
  }
  return ret;
}

/***********************************
  PUBLIC : test server settings
 
***********************************/
gint collaborate_test_server_settings (APP_data *data)
{
  GKeyFile *keyString;
  gchar *tmpStr;
  gint ret = 0;

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");

  tmpStr = g_key_file_get_string (keyString, "server", "id", NULL);
  if(g_ascii_strncasecmp ((gchar*)  tmpStr, "?", sizeof(gchar))  ==  0 ) {
        ret = -1;
  }
  if(tmpStr != NULL)
        g_free (tmpStr);

  tmpStr = g_key_file_get_string (keyString, "server", "adress", NULL);
  if(g_ascii_strncasecmp ((gchar*)  tmpStr, "?", sizeof(gchar))  ==  0 ) {
        ret = -1;
  }
  if(tmpStr != NULL)
        g_free (tmpStr);

  /*  error dialog */
  if(ret<0) {
       tmpStr = g_strdup_printf ("%s", _("Sorry !\nServer settings are wrong or incomplete.\nOperation aborted. Please, see :\n<b>Edition>settings</b> menu."));
       misc_ErrorDialog (data->appWindow, tmpStr);
       g_free (tmpStr);
  }
  return ret;
}

/****************************
  LOCAL
  main mail composing dialog

****************************/

GtkWidget *collaborate_compose_email_dialog (APP_data *data)
{
  GtkWidget *dialog;
  /* get datas from xml Glade file - CRITICAL : only in activate phase !!!! */
  GtkBuilder *builder =NULL;
  builder = gtk_builder_new ();
  GError *err = NULL; 

  if(!gtk_builder_add_from_file (builder, find_ui_file ("freemail.ui"), &err)) {
     g_error_free (err);
     misc_halt_after_glade_failure (data);
  }
  /* Get the dialog from the glade file. */
  gtk_builder_connect_signals (builder, data);
  dialog = GTK_WIDGET(gtk_builder_get_object (builder, "dialogFreeMail"));
  data->tmpBuilder = builder;

  /* here we can set-up default values */

  return dialog;
}

/****************************
  LOCAL
  main report mail 
 composing dialog

****************************/

GtkWidget *collaborate_compose_email_report_calendar_dialog (gint mode, APP_data *data)
{
  GtkWidget *dialog, *lblTitle, *lblSubTitle;
  /* get datas from xml Glade file - CRITICAL : only in activate phase !!!! */
  GtkBuilder *builder =NULL;
  builder = gtk_builder_new ();
  GError *err = NULL; 

  if(!gtk_builder_add_from_file (builder, find_ui_file ("reportmail.ui"), &err)) {
     g_error_free (err);
     misc_halt_after_glade_failure (data);
  }

  /* Get the dialog from the glade file. */
  gtk_builder_connect_signals (builder, data);
  dialog = GTK_WIDGET(gtk_builder_get_object (builder, "dialogReportMail"));
  data->tmpBuilder = builder;

  /* here we can set-up default values */
  /* initialize treeviews */
  data->treeViewReportMail = gtk_tree_view_new();
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(data->treeViewReportMail), TRUE);
  gtk_container_add (GTK_CONTAINER ( GTK_WIDGET(gtk_builder_get_object(builder, "viewportReport"))), 
                     data->treeViewReportMail);
  collaborate_init_list_rsc (data);
  fill_list_rsc (data);
  /* change some labels if we are in calendar mode */
  if(mode == MAIL_TYPE_CALENDAR) {
     lblTitle = GTK_WIDGET(gtk_builder_get_object(builder, "labelTitle"));
     lblSubTitle = GTK_WIDGET(gtk_builder_get_object(builder, "labelSubTitle"));
     gtk_label_set_markup (GTK_LABEL (lblTitle), ("<big><b>Send a calendar/agenda by email</b></big>"));
     gtk_label_set_markup (GTK_LABEL (lblSubTitle), 
             ("<i>The <u>current</u> calendar/agenda will be sent.\nBy default, calendar is sent to <u>all</u> recipients. </i>"));
  }

  return dialog;
}
/***********************************
  free email
***********************************/
gint 
   collaborate_send_free_email (gchar *adress, gint port, gchar *server_id, gchar *password, gboolean f_isSsl, APP_data *data)
{
  GtkWidget *dialog, *comboRecip, *subjectEntry;
  GtkWidget *text_view;
  GtkTextBuffer *buffer;
  GtkTextIter start, end;
  gchar *tmpStr, *subject;
  gchar **array;
  gint ret = 0, i, range;
  GList *l;
  rsc_datas *tmp_rsc_datas;
  CURLcode res = CURLE_OK;

  /* non-modal window to type the mail */
  dialog = collaborate_compose_email_dialog (data);

  /* we add spell checking */
  text_view = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "textviewCompMail"));
  misc_init_spell_checker (GTK_TEXT_VIEW(text_view), data);
  ret = gtk_dialog_run (GTK_DIALOG (dialog));

  if(ret == 1) { /* [OK] request to send mail */       
     /* we read recipients scope */
      comboRecip = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboCompMail"));
      range = gtk_combo_box_get_active (GTK_COMBO_BOX(comboRecip));
      /* 0 : ALL, 1 : overdue, 2: completed yesterday, 3: start tomorrow, 4 : ending today */
      subjectEntry = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryCompSubject"));
      /* we get text & subject */
      subject = g_strdup_printf ("%s", gtk_entry_get_text (GTK_ENTRY(subjectEntry)));
      text_view = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "textviewCompMail"));
      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(text_view));
      /* we get the content and convert it to an array of gchar */
      gtk_text_buffer_get_start_iter (buffer, &start);
      gtk_text_buffer_get_end_iter (buffer, &end);
      tmpStr = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
      /*  check if len == 0*/
      if(strlen (tmpStr) ==0) {
         gboolean ok = collaborate_alert_empty_message (dialog);
         if(!ok)
           g_object_unref (data->tmpBuilder);
           gtk_widget_destroy (GTK_WIDGET(dialog)); 
           return -1;
      }
      array = misc_prepare_mail (tmpStr);/* array never NULL, because is allocated with ONE byte by default */

      /* now, we send to recipients, according to 'email_range' parameter */
      if(array) {
        gtk_widget_destroy (GTK_WIDGET(dialog)); 
        /* 0 : ALL, 1 : overdue, 2: completed yesterday, 3: start tomorrow, 4 : ending today */
        switch(range) {
           case 0:{/* all participants */
             collaborate_free_recipients ();
             /* we read all ressources's email */
             for(i=0; i<g_list_length (data->rscList); i++) {
                l = g_list_nth (data->rscList, i);
                tmp_rsc_datas = (rsc_datas *)l->data;
                if(tmp_rsc_datas->mail != NULL) {
                     receptors = g_list_append (receptors, g_strdup_printf ("%s", tmp_rsc_datas->mail));/* print mandatory to FREE */
                }/* endif mail */
             }/* next i */
             /* send mail if receptors list isn't empty - Curl is "smart" , if various ressources have same email, only ONE message is sent */
             if(g_list_length (receptors)>0){                           
                 res = collaborate_send_email (subject, data->properties.manager_mail, receptors, 
                            array, adress, port, server_id, password, f_isSsl );
                 if((gint)res != 0)
                   ret = (gint)res;
             }/* endif receptors */
             else {
                tmpStr = g_strdup_printf ("%s", _("Error !\n\nMail sending aborted because there\nisn't any recipient !"));
	        misc_ErrorDialog (GTK_WIDGET(data->appWindow), tmpStr);
	        g_free (tmpStr);
             }
             break;
           }
           case 1:{
             ret = collaborate_send_recipients_tasks_overdue (TRUE, subject, array, adress, port, server_id, password, 
                                                 f_isSsl, data);
             break;
           }
           case 2:{
             ret = collaborate_send_recipients_tasks_ended_yesterday (TRUE, subject, array, adress, port, server_id, password, 
                                                 f_isSsl, data);
             break;
           }
           case 3:{
             ret = collaborate_send_recipients_tasks_start_tomorrow (TRUE, subject, array, adress, port, server_id, password, 
                                                 f_isSsl, data);
             break;
           }
           case 4:{
             ret = collaborate_send_recipients_tasks_ending_today (TRUE, subject, array, adress, port, server_id, password, 
                                                 f_isSsl, data);
             break;
           }
           default:;/* impossible */
        }/* end switch */
      }/* endif array */

      g_free (array);
      g_free (tmpStr);
      if(subject)
        g_free (subject);
  }/* endif [OK] */
  else
      gtk_widget_destroy (GTK_WIDGET(dialog));  /* please note : with a Builder, you can only use destroy in case of the properties 'destroy with parent' isn't activated for the dualog */  
  /* spell checker, if not ref-ed, is automatically destroyed with gtktextview */
  g_object_unref (data->tmpBuilder);

  return ret;
}

/***********************************
  PUBLIC : send manual mail
   
***********************************/
gint collaborate_send_manual_email (gint type_of_mail, APP_data *data)
{
  GKeyFile *keyString;
  gchar *tmpStr, *server_id, *adress;
  GtkWidget *statusProgress;
  const gchar *subjectStr;
  gint ret = 0, port;
  gboolean fSslTls;

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");
  /* check if manager mail exists */
  ret = collaborate_test_project_manager_mail (data);
  if(ret!=0)
     return -1;
  /* we must test if password is already present */
  if(settings_get_password () == NULL) {
     ret = settings_dialog_password (data);
     if(ret==0)
         return -1;
  }
  /* TODO test password */

  /* we test server */
  ret = collaborate_test_server_settings (data);
  if(ret != 0) {
       return -1;
  }

  server_id = g_key_file_get_string (keyString, "server", "id", NULL);
  adress = g_key_file_get_string (keyString, "server", "adress", NULL);
  port = g_key_file_get_integer (keyString, "server", "port", NULL);
  fSslTls = g_key_file_get_boolean (keyString, "server", "ssltls", NULL);
  /* progress bar */
  statusProgress = GTK_WIDGET(gtk_builder_get_object (data->builder, "boxProgressMail"));
  gtk_widget_show (GTK_WIDGET(statusProgress));
  cursor_busy (data->appWindow);
  /* initialize pulse function timeout */
  progressBar = GTK_WIDGET(gtk_builder_get_object (data->builder, "progressbarSending"));
  /* mandatory to get some advance time */
  gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR(progressBar), 0.1); // Sets the fraction of total progress bar length to move
  gtk_progress_bar_pulse (GTK_PROGRESS_BAR(progressBar)); 

  /* switch */
  switch(type_of_mail) {
    case MAIL_TYPE_FREE:{
       ret = collaborate_send_free_email (adress, port, server_id, (gchar *) settings_get_password (), fSslTls, data);
      break;
    }
    case MAIL_TYPE_REPORT:{
      ret = collaborate_send_report_or_calendar (MAIL_TYPE_REPORT, data);
      break;
    }
    case MAIL_TYPE_OVERDUE:{
      ret = collaborate_send_recipients_tasks_overdue (FALSE, NULL, NULL, adress, port, server_id, (gchar *) settings_get_password (), 
                                                 fSslTls, data);
      break;
    }
    case MAIL_TYPE_COMPLETED:{
      ret = collaborate_send_recipients_tasks_ended_yesterday (FALSE, NULL, NULL, adress, port, server_id, (gchar *) settings_get_password (), 
                                                 fSslTls, data);
      break;
    }
    case MAIL_TYPE_INCOMING:{
      ret = collaborate_send_recipients_tasks_start_tomorrow (FALSE, NULL, NULL, adress, port, server_id, (gchar *) settings_get_password (), 
                                                 fSslTls, data);
      break;
    }
    case MAIL_TYPE_CALENDAR: {
// TODO
       ret = collaborate_send_report_or_calendar (MAIL_TYPE_CALENDAR, data);
      break;
    }
    default: {
      ret = -1;
    }
  }/* end switch */
  gtk_widget_hide (GTK_WIDGET(statusProgress));
  cursor_normal (data->appWindow);
  return ret;

}

/************************************
  PUBLIC : send an email with a 
  report
************************************/
gint collaborate_send_report_or_calendar (gint mode, APP_data *data)
{
  GKeyFile *keyString;
  gint i, ret, port, rr;
  GList *l, *l1;
  receptors_elem *elem, *tmpElem;
  gchar *tmpStr, *server_id, *adress, *subject, *tmpFilename, buffer_date[80];
  gboolean fSslTls, fErrReceptors = FALSE;
  gchar** array;
  GtkWidget *dialog, *text_view, *subjectEntry, *sw, *statusProgress;
  GtkTextBuffer *buffer;
  GtkTextIter start, end;

  statusProgress = GTK_WIDGET(gtk_builder_get_object (data->builder, "boxProgressMail"));

  /* we generate a temporary filename */
  time_t rawtime;
  time (&rawtime);
  strftime (buffer_date, 80, "%c", localtime (&rawtime));
  for(i=0; i<=strlen (buffer_date); i++) {
     if(buffer_date[i]==' ' || buffer_date[i]=='.')
         buffer_date[i]='_';
  }

  keyString = g_object_get_data (G_OBJECT (data->appWindow), "config");

  /* check if servers parameters are OK */
  ret = collaborate_test_server_settings (data);
  if(ret != 0) {
       return -1;
  }/* endif ret */
  /* check if manager mail exists */
  ret = collaborate_test_project_manager_mail (data);
  if(ret != 0)
        return -1;
  /* we request the password */
  if(settings_get_password () == NULL) {
     ret = settings_dialog_password (data);
     if(ret==0)
         return -1;
  }

  /* set account parameters */
  server_id = g_key_file_get_string (keyString, "server", "id", NULL);
  adress = g_key_file_get_string (keyString, "server", "adress", NULL);
  port = g_key_file_get_integer (keyString, "server", "port", NULL);
  fSslTls = g_key_file_get_boolean (keyString, "server", "ssltls", NULL);
 
  /* display a dialog to get a free message, last chance to vancel process */
  /* non-modal window to type the mail */
  dialog = collaborate_compose_email_report_calendar_dialog (mode, data);
  /* initialize pulse function timeout */
  progressBar = GTK_WIDGET(gtk_builder_get_object (data->builder, "progressbarSending"));
  /* mandatory to get some advance time */
  gtk_widget_show (GTK_WIDGET(statusProgress));
  gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR(progressBar), 0.05); // Sets the fraction of total progress bar length to move
  gtk_progress_bar_pulse (GTK_PROGRESS_BAR(progressBar)); 

  /* we add spell checking */
  text_view = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "textviewCompMail"));
  misc_init_spell_checker (GTK_TEXT_VIEW(text_view), data);

  ret = gtk_dialog_run (GTK_DIALOG (dialog));
  if(ret == 1) { /* [OK] request to send mail */       
      subjectEntry = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryCompSubject"));
      sw = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "scrolledwindowReport"));
      /* we get text & subject */
      subject = g_strdup_printf ("%s", gtk_entry_get_text (GTK_ENTRY(subjectEntry)));
      text_view = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "textviewCompMail"));
      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(text_view));
      /* we get the content and convert it to an array of gchar */
      gtk_text_buffer_get_start_iter (buffer, &start);
      gtk_text_buffer_get_end_iter (buffer, &end);
      tmpStr = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
      /*  check if len == 0*/
      if(strlen (tmpStr) ==0) {
         gboolean ok = collaborate_alert_empty_message (dialog);
         if(!ok) {
           g_object_unref (data->tmpBuilder);
           gtk_widget_destroy (GTK_WIDGET(dialog)); 
           gtk_widget_hide (GTK_WIDGET(statusProgress));
           return -1;
         }
      }
      array = misc_prepare_mail (tmpStr);/* array never NULL, because is allocated with ONE byte by default */
      if(array) {
         /* choose recipients */
         collaborate_free_recipients ();
         report_update_list (data);
         gtk_widget_destroy (GTK_WIDGET(dialog)); 
         /* put a temporary RTF file to /tmp directory */
         if(mode==MAIL_TYPE_REPORT) {
             tmpFilename = g_strdup_printf (_("%s/report-%s.rtf"), g_get_tmp_dir (), buffer_date);
             rr = save_RTF_rich_text (tmpFilename, data);
             /* send */
             if(g_list_length (receptors)>0){               
		   ret = collaborate_send_multipart_email (subject, data->properties.manager_mail, receptors, 
		                    array, adress, port, server_id, (gchar *) settings_get_password (), 
		                    fSslTls, tmpFilename);
	     }
	     else {
		        fErrReceptors = TRUE;
             }
           g_free (tmpFilename);
         }
         else {/* we send calendars for every choosen recipient (it's a personalized calendar) */
              tmpElem = g_malloc (sizeof(receptors_elem));/* just a pointer on values, never free thos evalues ! */
              l1 = NULL;
              for(i = 0; i < g_list_length (receptors); i++) {
                 tmpFilename = g_strdup_printf (_("%s/calendar-%s.ics"), g_get_tmp_dir (), buffer_date);
                 l = g_list_nth (receptors, i);
                 elem = (receptors_elem *) l->data;
                 rr = save_project_to_icalendar_for_rsc (tmpFilename, elem->id, data);
                 /* we build a GList of ONE element */
                 tmpElem->id = elem->id;
                 tmpElem->str = elem->str;
                 l1 = g_list_append (l1, tmpElem);/* l1 contains only ONE element */
                 ret = collaborate_send_multipart_email (subject, data->properties.manager_mail, l1, 
		                    array, adress, port, server_id, (gchar *) settings_get_password (), 
		                    fSslTls, tmpFilename);
                 g_free (tmpFilename);
                 /* we remove the unique element of l1 */
                 g_list_free (l1);
                 l1 = NULL;
              }             
         }/* endif calendar */
		
         /* clean and manage errors */
         if(subject)
             g_free (subject);

         /* common for reports and calendars ; ret is !=0 if something is wrong */
         if(rr!=0) {
             gchar *msg = g_strdup_printf ("%s", _("Sorry !\nAn error happened during attached file generation.\nOperation aborted. You can try again, or\ncheck your system."));
             misc_ErrorDialog (data->appWindow, msg);
             g_free (msg);
         }
         if(fErrReceptors) {
             tmpStr = g_strdup_printf ("%s", _("Error !\n\nMail sending aborted because there\nisn't any recipient !"));
	         misc_ErrorDialog (GTK_WIDGET(data->appWindow), tmpStr);
	         g_free (tmpStr);
         }
	 if(ret!=0) {
	     tmpStr = g_strdup_printf ("%s", _("Error !\n\nMail sending aborted partially or totally.\nPlease try again after checking your Internet connection and accounts settings."));
	     misc_ErrorDialog (GTK_WIDGET(data->appWindow), tmpStr);
	     g_free (tmpStr);
	 }/* endif ret !=0 */
      }/* endif array */
  }/* endif [OK] */
  else
     gtk_widget_destroy (GTK_WIDGET(dialog)); 
  /* spell checker, if not ref-ed, is automatically destroyed with gtktextview */
  g_object_unref (data->tmpBuilder);
  gtk_widget_hide (GTK_WIDGET(statusProgress));
  return ret;
}


