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
#include "home.h"
#include "settings.h"
#include "mail.h"

/*********************************

  PUBLIC : display today's 
  date in "homé" panel
*********************************/

void home_today_date (APP_data *data)
{
  time_t rawtime; 
  gchar buffer[80], *tmpStr, *server_id, *adress;
  GtkWidget *label, *label_mail, *label_auto_over, *label_auto_today, *label_auto_comp, *label_auto_tom;
  GKeyFile *keyString;
  GDate date;
  gint i, dd, mm, yy, nbOver, nbToday, nbYest, nbTom, port, count_op, ret;
  gboolean fMailStartTom, fMailOver, fMailCpl, fMailEnd, fSslTls, fErrSending;
  gsize written;

  label = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelHomeDate"));  
  label_mail = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelDateAutoMail")); 
  label_auto_over = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelSendAutoOver")); 
  label_auto_today = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelSendAutoToday")); 
  label_auto_comp = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelSendAutoComp")); 
  label_auto_tom = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelSendAutoTom")); 
  /* we get the current date */
  time (&rawtime);
  strftime (buffer, 80, "%A, %x", localtime (&rawtime));/* don't change parameter %x */
  tmpStr = g_strdup_printf ("<big><big><big><i>%s</i></big></big></big>", buffer);
  gtk_label_set_markup (GTK_LABEL(label), tmpStr);
  g_free (tmpStr);

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");
  dd = g_key_file_get_integer (keyString, "collaborate", "last_auto_mailing-day", NULL);
  mm = g_key_file_get_integer (keyString, "collaborate", "last_auto_mailing-month", NULL);
  yy = g_key_file_get_integer (keyString, "collaborate", "last_auto_mailing-year", NULL);
  if(dd<0 || mm<0 || yy<0) 
      tmpStr = g_strdup_printf ("<big><b><i>%s</i></b></big>", _("there isn't any auto mailing for now"));
  else {
      g_date_set_dmy (&date, dd, mm, yy);
      written = g_date_strftime (&buffer, 79, "%x", &date);
      tmpStr = g_strdup_printf ("<big><b><i>%s</i></b></big>", buffer);
  }
  
  gtk_label_set_markup (GTK_LABEL(label_mail), tmpStr);
  g_free (tmpStr);

  /* test if auto-mailing is activated, and display a message if */
  fMailStartTom = g_key_file_get_boolean (keyString, "collaborate", "mail_tasks_start_tomorrow", NULL);
  fMailOver = g_key_file_get_boolean (keyString, "collaborate", "mail_tasks_overdue_today", NULL);
  fMailCpl = g_key_file_get_boolean (keyString, "collaborate", "mail_tasks_completed_yesterday", NULL);
  fMailEnd = g_key_file_get_boolean (keyString, "collaborate", "mail_tasks_ending_today", NULL);
  /* now, according to flags, we check tasks status */
  count_op = 0;
  nbOver = misc_count_tasks_overdue (data);
  if(nbOver>0)
     count_op++;
  nbToday = misc_count_tasks_ending_today (data);
  if(nbToday>0)
     count_op++;
  nbYest = misc_count_tasks_completed_yesterday (data);
  if(nbYest>0)
     count_op++;
  nbTom = misc_count_tasks_tomorrow (data);
  if(nbTom>0)
     count_op++;
  if((fMailOver && (nbOver>0))  || (fMailStartTom && (nbTom>0))
     || (fMailCpl && (nbYest>0)) || (fMailEnd && (nbToday>0)) ) 
  {
     gtk_label_set_markup (GTK_LABEL(label_auto_over), _("<big><b><i>yes</i></b></big>"));
     /* we test if server settings are OK */
     ret = collaborate_test_server_settings (data);
     if(ret != 0) {/* something is wrong about server settings, we exit */
        tmpStr = g_strdup_printf ("%s", _("Sorry !\nServer settings are wrong or incomplete.\nOperation aborted. Please, see :\n<b>Edition>settings</b> menu."));
        misc_ErrorDialog (data->appWindow, tmpStr);
        g_free (tmpStr);
       return;
     }
     /* check if manager mail exists */
     ret = collaborate_test_project_manager_mail (data);
     if(ret!=0)
        return;
     /* we request the password */
     ret = settings_dialog_password (data);
     if(ret == 0)
         return;
     /*  we send mails - we have the nb of mails for each cases */

     server_id = g_key_file_get_string (keyString, "server", "id", NULL);
     adress = g_key_file_get_string (keyString, "server", "adress", NULL);
     port = g_key_file_get_integer (keyString, "server", "port", NULL);
     fSslTls = g_key_file_get_boolean (keyString, "server", "ssltls", NULL);
     fErrSending = FALSE;
     i = 1;
     gtk_label_set_markup (GTK_LABEL(label_auto_over), _("<b><i>No</i></b>"));
     gtk_label_set_markup (GTK_LABEL(label_auto_today), _("<b><i>No</i></b>"));
     gtk_label_set_markup (GTK_LABEL(label_auto_comp), _("<b><i>No</i></b>"));
     gtk_label_set_markup (GTK_LABEL(label_auto_tom), _("<b><i>No</i></b>"));
     cursor_busy (data->appWindow);
     if(fMailOver && (nbOver>0)) {/* we send mail to participants in overdue status */
        tmpStr = g_strdup_printf (_("<big><b><i>on the way (%d of %d)</i></b></big>") , i, count_op);
        gtk_label_set_markup (GTK_LABEL(label_auto_over), tmpStr);
        g_free (tmpStr);
        /* send mails */
        ret = collaborate_send_recipients_tasks_overdue (FALSE, NULL, NULL, adress, port, server_id, 
                                         settings_get_password (), fSslTls, data);
        if(ret!=0 && ret!=8) {/* 8 = err weird FTP data response */
           fErrSending = TRUE;
           tmpStr = g_strdup_printf ("%s", _("<b><i>error</i></b>"));
        }
        else
           tmpStr = g_strdup_printf ("%s", _("<b><i>done</i></b>"));
        gtk_label_set_markup (GTK_LABEL(label_auto_over), tmpStr);
        g_free (tmpStr);
        /* free list */
        collaborate_free_recipients ();
        i++;
     }/* endif overdue */
     if(fMailStartTom && (nbTom>0)){/* we send a reminder for tasks starting tomorrow */
        tmpStr = g_strdup_printf (_("<big><b><i>on the way (%d of %d)</i></b></big>") , i, count_op);
        gtk_label_set_markup (GTK_LABEL(label_auto_tom), tmpStr);
        g_free (tmpStr);
        /* send mails */
        ret = collaborate_send_recipients_tasks_start_tomorrow (FALSE, NULL, NULL, adress, port, server_id, 
                              settings_get_password (), fSslTls, data);
       if(ret!=0 && ret!=8) {/* 8 = err weird FTP data response */
           fErrSending = TRUE;
           tmpStr = g_strdup_printf ("%s", _("<b><i>error</i></b>"));
        }
        else
           tmpStr = g_strdup_printf ("%s", _("<b><i>done</i></b>"));
        gtk_label_set_markup (GTK_LABEL(label_auto_tom), tmpStr);
        g_free (tmpStr);
        /* free list */
        collaborate_free_recipients ();
        i++;
     }/* endif start tomor. */
     if(fMailCpl && (nbYest>0)){/* a mail for congratulations */
        tmpStr = g_strdup_printf (_("<big><b><i>on the way (%d of %d)</i></b></big>"), i, count_op);
        gtk_label_set_markup (GTK_LABEL(label_auto_comp), tmpStr);
        g_free (tmpStr);
        ret = collaborate_send_recipients_tasks_ended_yesterday (FALSE, NULL, NULL, adress, port, server_id, 
                                              settings_get_password (), fSslTls, data);
        if(ret!=0 && ret!=8) {/* 8 = err weird FTP data response */
           fErrSending = TRUE;
           tmpStr = g_strdup_printf ("%s", _("<b><i>error</i></b>"));
        }
        else
           tmpStr = g_strdup_printf ("%s", _("<b><i>done</i></b>"));
        gtk_label_set_markup (GTK_LABEL(label_auto_comp), tmpStr);
        g_free (tmpStr);
        /* free list */
        collaborate_free_recipients ();
        i++;     
     }/* endif congrat. */
     if(fMailEnd && (nbToday>0)){ /* a mail to 'encourage' for tasks ending today */
        tmpStr = g_strdup_printf (_("<big><b><i>on the way (%d of %d)</i></b></big>") , i, count_op);
        gtk_label_set_markup (GTK_LABEL(label_auto_today), tmpStr);
        g_free (tmpStr);
        ret = collaborate_send_recipients_tasks_ending_today (FALSE, NULL, NULL, adress, port, server_id, 
                                     settings_get_password (), fSslTls, data);
       if(ret!=0 && ret!=8) {/* 8 = err weird FTP data response */
           fErrSending = TRUE;
           tmpStr = g_strdup_printf ("%s", _("<b><i>error</i></b>"));
        }
        else
           tmpStr = g_strdup_printf ("%s", _("<b><i>done</i></b>"));
        gtk_label_set_markup (GTK_LABEL(label_auto_today), tmpStr);
        g_free (tmpStr);
        /* free list */
        collaborate_free_recipients ();
        i++;
     }/* endif tasks ending today */
     cursor_normal (data->appWindow);
     /* we display an error dialog if something is wrong */
     if(fErrSending == FALSE) {
        /* we store current date of auto-mailing */
        g_date_set_time_t (&date, time (NULL)); /* date of today */
        g_key_file_set_integer (keyString, "collaborate", "last_auto_mailing-day", g_date_get_day (&date));
        g_key_file_set_integer (keyString, "collaborate", "last_auto_mailing-month", g_date_get_month (&date));
        g_key_file_set_integer (keyString, "collaborate", "last_auto_mailing-year", g_date_get_year (&date));
        /* we activate flag modified in order to "force" storage of mail sending date in file */
        misc_display_app_status (TRUE, data);
     }/* endif fErrSending */
 
     /* we free strings */
     g_free (server_id);
     g_free (adress);
  }

}

/*********************************
  PUBLIC : display project's 
  infos
*********************************/
void home_project_infos (APP_data *data)
{
  gchar *tmpStr;
  GtkWidget *labelName;

  labelName = GTK_WIDGET(gtk_builder_get_object(data->builder, "labelHomeProjName"));  

  if(data->properties.title)
     tmpStr = g_strdup_printf ("<big><big><big><i>%s</i></big></big></big>", data->properties.title);
  else
     tmpStr = g_strdup_printf ("<big><big><big><i>%s</i></big></big></big>", _("noname"));

  gtk_label_set_markup (GTK_LABEL(labelName), tmpStr);
  g_free (tmpStr);
}

/*********************************
  PUBLIC : display project's 
  tasks infos
*********************************/
void home_project_tasks (APP_data *data)
{
  gchar *tmpStr;
  gint total = 0, i;
  GtkWidget *label_total_tasks, *label_total_overdue, *label_total_run, *label_start_today, *label_start_tom, *label_completed, *label_to_go;

  /* total number of tasks */

  total = misc_count_tasks (data);  

  label_total_tasks = GTK_WIDGET(gtk_builder_get_object(data->builder, "labelHomeTotalTsks"));
  tmpStr = g_strdup_printf ("<big><big><big><i>%d</i></big></big></big>", total);
  gtk_label_set_markup(GTK_LABEL(label_total_tasks), tmpStr);
  g_free (tmpStr);

  /* number of tasks overdue */
  label_total_overdue = GTK_WIDGET(gtk_builder_get_object(data->builder, "labelHomeTotalOver"));
  tmpStr = g_strdup_printf ("<big><big><big><i>%d</i></big></big></big>", misc_count_tasks_overdue (data));
  gtk_label_set_markup(GTK_LABEL(label_total_overdue), tmpStr);
  g_free (tmpStr);

  /* running tasks : status running and/or status overdue */
  label_total_run = GTK_WIDGET(gtk_builder_get_object(data->builder, "labelHomeRun"));
  tmpStr = g_strdup_printf ("<big><big><big><i>%d</i></big></big></big>", misc_count_tasks_run (data));
  gtk_label_set_markup(GTK_LABEL(label_total_run), tmpStr);
  g_free (tmpStr);

  /* number of tasks starting today */
  label_start_today = GTK_WIDGET(gtk_builder_get_object(data->builder, "labelHomeStartToday"));
  tmpStr = g_strdup_printf ("<big><big><big><i>%d</i></big></big></big>", misc_count_tasks_today (data));
  gtk_label_set_markup(GTK_LABEL(label_start_today), tmpStr);
  g_free (tmpStr);

  /* number of tasks starting tomorrow */
  label_start_tom = GTK_WIDGET(gtk_builder_get_object(data->builder, "labelHomeTomorrow"));
  tmpStr = g_strdup_printf ("<big><big><big><i>%d</i></big></big></big>", misc_count_tasks_tomorrow (data));
  gtk_label_set_markup (GTK_LABEL(label_start_tom), tmpStr);
  g_free (tmpStr);

  /* number of tasks completed */
  label_completed = GTK_WIDGET(gtk_builder_get_object(data->builder, "labelHomeCompleted"));
  tmpStr = g_strdup_printf ("<big><big><big><i>%d</i></big></big></big>", misc_count_tasks_completed (data));
  gtk_label_set_markup (GTK_LABEL(label_completed), tmpStr);
  g_free (tmpStr);

  /* tasks to go */
  label_to_go = GTK_WIDGET(gtk_builder_get_object(data->builder, "labelToGo"));
  tmpStr = g_strdup_printf ("<big><big><big><i>%d</i></big></big></big>", total-misc_count_tasks_run (data));
  gtk_label_set_markup (GTK_LABEL(label_to_go), tmpStr);
  g_free (tmpStr);
}

/*******************************
  PUBLIC : display app logo

*******************************/
void home_set_app_logo (APP_data *data)
{
  GdkPixbuf *app_icon_pixbuf;
  GtkWidget *app_image;

  app_image = GTK_WIDGET(gtk_builder_get_object (data->builder, "imageHomeApp"));
  app_icon_pixbuf = create_pixbuf ("planlibre_home.png");
  if(app_icon_pixbuf) {
      gtk_image_set_from_pixbuf (GTK_IMAGE(app_image), app_icon_pixbuf);
      g_object_unref (G_OBJECT(app_icon_pixbuf));
  }

}

/*********************************
  PUBLIC : display project's 
  milestone infos
*********************************/
void home_project_milestones (APP_data *data)
{
   gchar *tmpStr;
   GtkWidget *label;

   label = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelTodayMileStone"));
   tmpStr = g_strdup_printf ("<big><big><big><i>%d</i></big></big></big>", misc_count_milestones_today (data));
   gtk_label_set_markup (GTK_LABEL(label), tmpStr);
   g_free (tmpStr);

}
