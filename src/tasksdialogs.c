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
#include "undoredo.h"
#include "report.h"
#include "timeline.h"
#include "assign.h"
#include "tasksutils.h"

/* constants */

/* 
  variables 
*/



/* 
  functions
*/

/*********************************************
  PUBLIC : complete display of a task with 
  its ressources
  id = identifier of 'task' object
  box : pointer on GtkBox to pack ressources
  data : global access for lst of links
*********************************************/
void tasks_add_display_ressources (gint id, GtkWidget *box, APP_data *data)
{
  gint i, link=0, ok=-1;
  GList *l;
  rsc_datas *tmp_rsc_datas;
  GdkPixbuf *pix2;
  gchar *tmpStr;
  GError *err = NULL;

  /* we read all ressources, and when a match is found, we update display */
  for(i=0; i<g_list_length (data->rscList); i++) {
     l = g_list_nth (data->rscList, i);
     tmp_rsc_datas = (rsc_datas *)l->data;
     ok = links_check_element (id, tmp_rsc_datas->id , data);
     /* we test association receiver-sender for current line */
     if(ok>=0) {
        GtkWidget *subBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_pack_start (GTK_BOX (box), subBox, FALSE, FALSE, 0);
        tmpStr = g_markup_printf_escaped ("<small><i> %s  </i></small>", tmp_rsc_datas->name);
        GtkWidget *label = gtk_label_new (tmpStr);
        g_free (tmpStr);
        gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
        gtk_label_set_ellipsize (GTK_LABEL(label), PANGO_ELLIPSIZE_END);
        gtk_label_set_max_width_chars (GTK_LABEL(label), TASKS_RSC_LEN_TEXT);
        gtk_label_set_use_markup (GTK_LABEL(label), TRUE);
        gtk_widget_show (label);
        gtk_box_pack_start (GTK_BOX (subBox), label, FALSE, FALSE, 0);
        /* icon */
        if(tmp_rsc_datas->pix) {   
           pix2 = gdk_pixbuf_scale_simple (tmp_rsc_datas->pix, 
                             TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, GDK_INTERP_BILINEAR);
        }
        else {
          //  pix2 = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE);
          //  gdk_pixbuf_fill (pix2, 0x00000080);/* black 50% transparent */
            pix2 = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("def_avatar.png"), TASKS_RSC_AVATAR_SIZE,
                                                    TASKS_RSC_AVATAR_SIZE, &err);
        }
        GtkWidget *img = gtk_image_new_from_pixbuf (pix2);
        gtk_widget_show (img);
        g_object_unref (pix2);
        gtk_box_pack_start (GTK_BOX (subBox), img, FALSE, FALSE, 0);
        gtk_widget_show_all (subBox);
        gtk_widget_show_all (box);
     }/* endif OK */
  }/* next i */
}

/******************************************************************
  PUBLIC : complete display of a task with its ancestors tasks
  id = identifier of 'task' object
  box : pointer on GtkBox to pack ressources
  data : global access for list of links
******************************************************************/
void tasks_add_display_tasks (gint id, GtkWidget *box, APP_data *data)
{
  gint i, link=0, ok=-1;
  GList *l;
  tasks_data *tmp_tsk_datas;
  GtkWidget *imgCateg;
  GdkPixbuf *pix;
  GError *err = NULL;
  gchar *tmpStr;

  if(box==NULL)
     return;
  /* we read all ressources, and when a match is found, we update display */
  for(i=0; i<g_list_length (data->tasksList); i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     ok = links_check_element (id, tmp_tsk_datas->id , data);
     /* we test association receiver-sender for current line */
     if((ok>=0)  && (tmp_tsk_datas->type != TASKS_TYPE_GROUP)) {
        GtkWidget *subBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_show (subBox);
        gtk_box_pack_start (GTK_BOX (box), subBox, FALSE, FALSE, 0);
        tmpStr = g_markup_printf_escaped ("<small><i> %s  </i></small>", tmp_tsk_datas->name);
        GtkWidget *label = gtk_label_new(tmpStr);
        g_free (tmpStr);
        gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
        gtk_label_set_ellipsize (GTK_LABEL(label), PANGO_ELLIPSIZE_END);
        gtk_label_set_max_width_chars (GTK_LABEL(label), TASKS_RSC_LEN_TEXT);
        gtk_label_set_use_markup (GTK_LABEL(label), TRUE);
        gtk_widget_show (label);
        gtk_box_pack_start (GTK_BOX(subBox), label, FALSE, FALSE, 0); 
        /* icons  category */
        if(tmp_tsk_datas->type == TASKS_TYPE_TASK) {
		switch(tmp_tsk_datas->category) {
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
        }/* endif type == task */
        else {
           pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("milestone.png"), 
	                  TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);         
	   imgCateg = gtk_image_new_from_pixbuf (pix);
	   g_object_unref (pix);
        }
        gtk_widget_show (imgCateg);
        gtk_box_pack_start (GTK_BOX (subBox), imgCateg, FALSE, FALSE, 0); 
     }/* endif OK */
  }/* next i */
}

/******************************************************************
  PUBLIC : complete display of a task with its ancestors tasks
  id = identifier of 'task' object
  box : pointer on GtkBox to pack ressources
  data : global access for list of links
******************************************************************/
void tasks_remove_display_tasks (gint id, gint id_to_remove, GtkWidget *box, APP_data *data)
{
  gint i, link=0, ok=-1;
  GList *l;
  tasks_data *tmp_tsk_datas;
  GtkWidget *imgCateg;
  GdkPixbuf *pix;
  GError *err = NULL;
  gchar *tmpStr;

  if(box==NULL)
     return;
  /* we read all ressources, and when a match is found, we update display */
  for(i=0; i<g_list_length (data->tasksList); i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     ok = links_check_element (id, tmp_tsk_datas->id , data);
     /* we test association receiver-sender for current line */
     if((ok>=0) && (tmp_tsk_datas->id!=id_to_remove) && (tmp_tsk_datas->type != TASKS_TYPE_GROUP)) {
        GtkWidget *subBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_show (subBox);
        gtk_box_pack_start (GTK_BOX (box), subBox, FALSE, FALSE, 0);
        tmpStr = g_markup_printf_escaped ("<small><i> %s  </i></small>", tmp_tsk_datas->name);
        GtkWidget *label = gtk_label_new(tmpStr);
        g_free (tmpStr);
        gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
        gtk_label_set_ellipsize (GTK_LABEL(label), PANGO_ELLIPSIZE_END);
        gtk_label_set_max_width_chars (GTK_LABEL(label), TASKS_RSC_LEN_TEXT);
        gtk_label_set_use_markup (GTK_LABEL(label), TRUE);
        gtk_widget_show (label);
        gtk_box_pack_start (GTK_BOX(subBox), label, FALSE, FALSE, 0); 
        /* icons  category */
        if(tmp_tsk_datas->type == TASKS_TYPE_TASK) {
		switch(tmp_tsk_datas->category) {
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
        }/* endif type == task */
        else {
           pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("milestone.png"), 
	                  TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);         
	   imgCateg = gtk_image_new_from_pixbuf (pix);
	   g_object_unref (pix);
        }
        gtk_widget_show (imgCateg);
        gtk_box_pack_start (GTK_BOX (subBox), imgCateg, FALSE, FALSE, 0); 
     }/* endif OK */
  }/* next i */
}

/**********************************
  Callback : button due (deadline) 
  date clicked
***********************************/
void on_TasksDateBtn_clicked (GtkButton *button, APP_data *data)
{
  GDate date_cur_start, date_start, date_end, date_duration, date_deadline;
  gboolean fErrDateInterval1 = FALSE;
  gint cmp_date, cmp_duration, duration_mode, duration;
  const gchar *curDate = NULL;
  const gchar *dueDate = NULL;

  GtkWidget *dateEntry = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonStartDate"));
  GtkWidget *win = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "dialogTasks"));
  GtkWidget *pDays = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonDays"));
  GtkWidget *pButton = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonTasksDueDate")); 
  GtkWidget *pComboDuration = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxtextDuration"));
  gchar *newDate = misc_getDate (gtk_button_get_label (GTK_BUTTON(pButton)), win);
  gtk_button_set_label (GTK_BUTTON(pButton), newDate);

  /* we get dates limits from properties */
  g_date_set_parse (&date_start, data->properties.start_date);
  g_date_set_parse (&date_end, data->properties.end_date);

  duration = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(pDays));
  if(duration<1)
     duration = 1;

  curDate = gtk_button_get_label (GTK_BUTTON(dateEntry));
  dueDate = gtk_button_get_label (GTK_BUTTON(pButton));
  g_date_set_parse (&date_cur_start, curDate);
  g_date_set_parse (&date_duration, curDate);
  g_date_set_parse (&date_deadline, dueDate);
  g_date_add_days (&date_duration, duration-1);

  /* we must check if dates aren't inverted */
  fErrDateInterval1 = g_date_valid (&date_cur_start);
  if(!fErrDateInterval1)
      printf("Internal error date Interv 1\n");

  cmp_date = g_date_compare (&date_cur_start, &date_end);
  cmp_duration = g_date_compare (&date_duration, &date_end);

  if(cmp_date>0) {
       misc_ErrorDialog (win, _("<b>Error!</b>\n\nDates mismatch ! Date must be <i>before</i> project's 'Ending' date.\n\nPlease check the dates or modify project's properties."));
       gtk_button_set_label (GTK_BUTTON(dateEntry), data->properties.end_date);
  }
  /* we check if newdate is BEFORE properties start date of project */

  cmp_date = g_date_compare (&date_cur_start, &date_start);
  if(cmp_date<0) {
      misc_ErrorDialog (win, _("<b>Error!</b>\n\nYou can't set a date <i>before</i> project's starting date!\nPlease, change project properties if you are sure (see <b>Project menu</b>). "));
      gchar *tmpDate = misc_convert_date_to_locale_str (&date_start);
      gtk_button_set_label (GTK_BUTTON(dateEntry), tmpDate);
      g_free (tmpDate);
  }
  /* we check if start+duration if <= deadline in case of we are in dealine mode */
  duration_mode = gtk_combo_box_get_active (GTK_COMBO_BOX(pComboDuration));
/*
#define TASKS_AS_SOON 0
#define TASKS_DEADLINE 1
#define TASKS_AFTER 2
#define TASKS_FIXED 3

*/

  if(duration_mode>0) {
    if(duration_mode == TASKS_DEADLINE) {
       cmp_duration = g_date_compare (&date_duration, &date_deadline);
       if(cmp_duration>0) {
          misc_ErrorDialog (win, _("<b>Error !</b>\n\nYou can't set a date+duration <i>after</i> task's deadline date!\nPlease, change deadline date or task duration."));
          gtk_button_set_label (GTK_BUTTON(dateEntry), data->properties.end_date);
          gtk_spin_button_set_value (GTK_SPIN_BUTTON(pDays), 0);
       }
    }
    if(duration_mode == TASKS_AFTER) {
       cmp_date = g_date_compare (&date_cur_start, &date_deadline);
       if(cmp_date<0) {
          misc_ErrorDialog (win, _("<b>Error !</b>\n\nYou can't set a <i>don't start before</i> date <u>after</u> <i><b>task start date</b></i> date!\nPlanlibre has automatically changed due date."));
          gchar *tmpDate = misc_convert_date_to_locale_str (&date_cur_start);
          gtk_button_set_label (GTK_BUTTON(pButton), tmpDate);
          g_free (tmpDate);
       }
    }
  }
  g_free (newDate);

}

/**********************************
  PUBLIC : button starting date 
  clicked for standard tasks
***********************************/
void on_TasksStartDateBtn_clicked (GtkButton *button, APP_data *data)
{
  GDate date_interval1, date_start, date_end, date_duration, date_deadline;
  gboolean fErrDateInterval1 = FALSE;
  gint cmp_date, cmp_duration, duration_mode, duration;
  const gchar *curDate = NULL;
  const gchar *dueDate = NULL;

  GtkWidget *dateEntry = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonStartDate"));
  GtkWidget *win = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "dialogTasks"));
  GtkWidget *pComboDuration = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxtextDuration"));
  GtkWidget *pDays = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonDays"));
  GtkWidget *pButton = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonTasksDueDate")); 
  gchar *newDate = misc_getDate (gtk_button_get_label (GTK_BUTTON(dateEntry)), win);
  gtk_button_set_label (GTK_BUTTON(dateEntry), newDate);

  /* we get dates limits from properties */

  g_date_set_parse (&date_start, data->properties.start_date);
  g_date_set_parse (&date_end, data->properties.end_date);

  duration = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(pDays));
  if(duration<1)
     duration = 1;

  curDate = gtk_button_get_label (GTK_BUTTON(dateEntry));
  dueDate = gtk_button_get_label (GTK_BUTTON(pButton));

  g_date_set_parse (&date_interval1, curDate);
  g_date_set_parse (&date_duration, curDate);
  g_date_set_parse (&date_deadline, dueDate);
  g_date_add_days (&date_duration, duration-1);

  /* we must check if dates aren't inverted */
  fErrDateInterval1 = g_date_valid (&date_interval1);
  if(!fErrDateInterval1)
      printf("Internal error date Interv 1\n");


  cmp_date = g_date_compare (&date_interval1, &date_end);
  cmp_duration = g_date_compare (&date_duration, &date_end);
  /* here we test if date, or date+duration are before project's ending date */
  if(cmp_date>0 || cmp_duration>0) {
       misc_ErrorDialog (win, _("<b>Error!</b>\n\nDates mismatch ! Date and date+duration must be <i>before</i> project's 'Ending' date.\n\nPlease check the dates or modify project's properties."));
       gtk_button_set_label (GTK_BUTTON(dateEntry), curDate);
       gtk_spin_button_set_value (GTK_SPIN_BUTTON(pDays), 0);
  }
  /* we check if newdate is BEFORE properties start date of project */

  cmp_date = g_date_compare (&date_interval1, &date_start);
  if(cmp_date<0) {
      misc_ErrorDialog (win, _("<b>Error!</b>\n\nYou can't set a date <i>before</i> project's starting date!\nPlease, change project properties if you are sure (see <b>File menu</b>). "));
      gchar *tmpDate = misc_convert_date_to_locale_str (&date_start);
      gtk_button_set_label (GTK_BUTTON(dateEntry), tmpDate);
      g_free (tmpDate);
  }
  /* we check if start+duration if <= deadline in case of we are in dealine mode */
  duration_mode = gtk_combo_box_get_active (GTK_COMBO_BOX(pComboDuration));

  /* check backward and forward for linked tasks */
  /* forward : date+duration can't overlap children task starting date */
  cmp_duration = tasks_check_links_dates_forward (data->curTasks,  date_duration, data);
  if(cmp_duration != 0) {
        misc_ErrorDialog (win, _("<b>Error!</b>\n\nThis task is a <b>predecessor</b> of one or more task(s).\nYou can't <i>overlap</i> linked task's starting date!\nPlease, change current task starting date or duration."));
        gtk_button_set_label (GTK_BUTTON(dateEntry), data->properties.start_date);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON(pDays), 0);
  }
  /* backward : starting date can't overlap ancestor linked task ending date */
  cmp_duration = tasks_check_links_dates_backward (data->curTasks,  date_interval1, data);
  if(cmp_duration != 0) {
        misc_ErrorDialog (win, _("<b>Error!</b>\n\nThis task is a <b>linked and dependant</b> to one or more task(s).\nYou can't <i>overlap</i> linked task's ending date!\nPlease, change current task starting date."));
        gtk_button_set_label (GTK_BUTTON(dateEntry), data->properties.start_date);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON(pDays), 0);
  }

  /* check date due in specific mode */
  if(duration_mode>0) {
     if(duration_mode == TASKS_DEADLINE) {
        cmp_duration = g_date_compare (&date_duration, &date_deadline);
        if(cmp_duration>0) {
           misc_ErrorDialog (win, _("<b>Error!</b>\n\nYou can't set a date+duration <i>after</i> task's deadline date!\nPlease, change deadline date or task duration."));
           gtk_button_set_label (GTK_BUTTON(dateEntry), data->properties.start_date);
           gtk_spin_button_set_value (GTK_SPIN_BUTTON(pDays), 0);
        }
     }
     if(duration_mode == TASKS_AFTER) {
        cmp_date = g_date_compare (&date_interval1, &date_deadline);
        if(cmp_date<0) {
           misc_ErrorDialog (win, _("<b>Error!</b>\n\nYou can't set a start date <i>before</i> project's <i><b>don't start before</b></i> date!\nPlanlibre has automatically changed start date."));
           gchar *tmpDate = misc_convert_date_to_locale_str (&date_deadline);
           gtk_button_set_label (GTK_BUTTON(dateEntry), tmpDate);
           g_free (tmpDate);
        }
     }
  }
  g_free (newDate);

}

/**********************************
  PUBLIC : button due date 
  clicked for milestones
***********************************/
void on_MilestoneDateBtn_clicked (GtkButton *button, APP_data *data)
{
  GDate start_date, end_date, cur_date;
  GtkWidget *dateEntry = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonDate"));
  GtkWidget *win = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "dialogMilestone"));
  const gchar *curDate = NULL;

  gchar *newDate = misc_getDate (gtk_button_get_label (GTK_BUTTON(dateEntry)), win);
  gtk_button_set_label (GTK_BUTTON(dateEntry), newDate);

  g_date_set_dmy (&start_date, data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
  g_date_set_dmy (&end_date, data->properties.end_nthDay, data->properties.end_nthMonth, data->properties.end_nthYear);

  curDate = gtk_button_get_label (GTK_BUTTON(dateEntry));
  g_date_set_parse (&cur_date, curDate);
  g_free (newDate);
}

/************************************
  PUBLIC : callback when user
  changes type of duration combobox
************************************/
void on_combo_style_duration_changed (GtkComboBox *widget, APP_data *data)
{
  gint active, cmp;
  GDate date_cur_start, date_start, date_end, date_duration, date_deadline;
  const gchar *curDate = NULL;
  const gchar *dueDate = NULL;

  GtkWidget *dateEntry = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonStartDate"));
  GtkWidget *pButton = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonTasksDueDate"));   
  active = gtk_combo_box_get_active (GTK_COMBO_BOX(widget));

  curDate = gtk_button_get_label (GTK_BUTTON(dateEntry));
  dueDate = gtk_button_get_label (GTK_BUTTON(pButton));

  g_date_set_parse (&date_start, data->properties.start_date);
  g_date_set_parse (&date_end, data->properties.end_date);

  g_date_set_parse (&date_cur_start, curDate);
  g_date_set_parse (&date_deadline, dueDate);

  /* if active>0 we unlock date button */
  if(active==1 || active ==2) {
     gtk_widget_set_sensitive (GTK_WIDGET (pButton), TRUE);
     /* test1 : in duration mode 1 - don't end after - due date should be AFTER start date */
     if(active == 1) {
         cmp = g_date_compare (&date_deadline, &date_cur_start);
         if(cmp<=0) {
            gtk_button_set_label (GTK_BUTTON(pButton), data->properties.end_date);
         }
     }
     /* test 2 : in duration mode 2 - don't start before - due date should be BEFORE start date */
     if(active == 2) {
         cmp = g_date_compare (&date_deadline, &date_cur_start);
         if(cmp>=0) {
            gtk_button_set_label (GTK_BUTTON(pButton), data->properties.start_date);
         }
     }
  }
  else
     gtk_widget_set_sensitive (GTK_WIDGET (pButton), FALSE);

}

/*********************************
  PUBLIC : callback when user
  changes  duration
  of task
*********************************/
void on_spin_days_duration_changed (GtkSpinButton *spin_button, APP_data *data)
{
  GDate date_interval1, date_start, date_end, date_duration, date_deadline;
  gint cmp_duration, duration_mode, duration, active;
  const gchar *curDate = NULL;
  const gchar *dueDate = NULL;

  GtkWidget *pDays = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonDays"));
  GtkWidget *pButton = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonTasksDueDate"));  
  GtkWidget *pComboDuration = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxtextDuration"));
  GtkWidget *dateEntry = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonStartDate"));
  GtkWidget *win = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "dialogTasks"));

  duration = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(pDays));
  active = gtk_combo_box_get_active (GTK_COMBO_BOX(pComboDuration));

  /* we get dates limits from properties */

  g_date_set_parse (&date_start, data->properties.start_date );
  g_date_set_parse (&date_end, data->properties.end_date );

  if(duration<1)
     duration = 1;

  curDate = gtk_button_get_label (GTK_BUTTON(dateEntry));
  dueDate = gtk_button_get_label (GTK_BUTTON(pButton));

  g_date_set_parse (&date_interval1, curDate);
  g_date_set_parse (&date_duration, curDate);
  g_date_set_parse (&date_deadline, dueDate);
  g_date_add_days (&date_duration, duration-1);

  /* test if start date+duration <= project's ending date */
  cmp_duration = g_date_compare (&date_duration, &date_end);
  if(cmp_duration>0) {
       misc_ErrorDialog (win, _("<b>Error!</b>\n\nDates mismatch ! Starting date+duration must be <i>before</i> project's 'Ending' date.\n\nPlease check the dates or modify project's properties."));
       gtk_spin_button_set_value (GTK_SPIN_BUTTON(pDays), 0);
  }

  /* test with children tasks */
  cmp_duration = tasks_check_links_dates_forward (data->curTasks,  date_duration, data);
  if(cmp_duration != 0) {
        misc_ErrorDialog (win, _("<b>Error!</b>\n\nThis task is a <b>predecessor</b> of one or more task(s).\nYou can't set a duration <i>overlaping</i> linked task's starting date!\nPlease, change current task starting date or duration."));
        gtk_button_set_label (GTK_BUTTON(dateEntry), data->properties.start_date);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON(pDays), 0);
  }
  /* test, in case of duration mode == dealine, that date+duration <= due date */
  duration_mode = gtk_combo_box_get_active (GTK_COMBO_BOX(pComboDuration));
  if(duration_mode>0) {
     cmp_duration = g_date_compare (&date_duration, &date_deadline);
     if(cmp_duration>0) {
        misc_ErrorDialog (win, _("<b>Error!</b>\n\nYou can't set a date+duration <i>after</i> task's deadline date!\nPlease, change deadline date or task duration."));
        gtk_button_set_label (GTK_BUTTON(dateEntry), data->properties.start_date);
        gtk_spin_button_set_value (GTK_SPIN_BUTTON(pDays), 0);
     }
  }
}

/*********************************
  PUBLIC : callback when user
  changes  progress of task
*********************************/
void on_spinbuttonProgress_changed (GtkSpinButton *spin_button, APP_data *data)
{
  GtkWidget *pProgress = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonProgress"));
  GtkWidget *pStatus = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxTasksStatus"));

  gint progress = (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(pProgress));
  gint status = gtk_combo_box_get_active (GTK_COMBO_BOX(pStatus));

  /* now, we change status according to current progress */
  if((progress==100) && (status != TASKS_STATUS_DONE))
      gtk_combo_box_set_active (GTK_COMBO_BOX(pStatus), TASKS_STATUS_DONE);
  if((progress<100) && (status == TASKS_STATUS_DONE)) {
      gtk_combo_box_set_active (GTK_COMBO_BOX(pStatus), TASKS_STATUS_RUN);// avec dates, passer à délayed au besoin 
  }
}

/*********************************
  PUBLIC : callback when user
  changes  status of task
*********************************/
void on_combo_status_changed (GtkComboBox *widget, APP_data *data)
{
  gint active;
  GtkWidget *pProgress = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonProgress"));

  active = gtk_combo_box_get_active (GTK_COMBO_BOX(widget));
  /* if active>0 we change progress value */
  if(active == TASKS_STATUS_DONE)
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(pProgress), 100);
  else
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(pProgress), 0);
}

/*********************************
  PUBLIC : callback when user
  changes  group of task
*********************************/
void on_combo_group_changed (GtkComboBox *widget, APP_data *data)
{
  gint active, position, curGroup, prevGroup;
  GtkListBox *box;
  GtkListBoxRow *row;
  GList *l;
  tasks_data *tmp, temp, *undoTasks;
  undo_datas *tmp_value;

  active = gtk_combo_box_get_active (GTK_COMBO_BOX(widget));
  box = GTK_LIST_BOX(GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasks"))); 
  row = gtk_list_box_get_selected_row (box);
  if(row) {
     position = gtk_list_box_row_get_index (row);  
  }
  if(position>=0) {
     l = g_list_nth (data->tasksList, position);
     tmp = (tasks_data *) l->data;
     if(tmp->type == TASKS_TYPE_TASK && active>=0) {
        tasks_free_list_groups (data);
        tasks_fill_list_groups (data);
        curGroup = tasks_get_group_id (active, data);

        if(tmp->group!=curGroup) {
           /* we read current datas in memory */
	   tasks_get_ressource_datas (position, &temp, data);
	   /* undo engine */
	   undoTasks = g_malloc (sizeof(tasks_data));
	   tasks_get_ressource_datas (position, undoTasks, data);
	   /* build strings in order to keep "true" values */
	   if(temp.name)
	        undoTasks->name = g_strdup_printf ("%s", temp.name);
	   else
	        undoTasks->name = NULL;
	   if(temp.location)
	        undoTasks->location = g_strdup_printf ("%s", temp.location);
	   else
	        undoTasks->location = NULL;
           /* undo engine - continued - we just have to get a pointer on ID */
	   tmp_value = g_malloc (sizeof(undo_datas));
	   tmp_value->insertion_row = position;
	   tmp_value->new_row = position;
	   tmp_value->id = temp.id;
	   tmp_value->datas = undoTasks;
	   tmp_value->groups = NULL;
	   /* duplicate links */
	   tmp_value->list = links_copy_all (data);
           /* we store new group */
           prevGroup = tmp->group;
           tmp->group = curGroup;
           gint newRow = tasks_compute_valid_insertion_point (position, curGroup, FALSE, data);
           if(newRow>=g_list_length (data->tasksList))
              newRow = g_list_length (data->tasksList)-1;
           if(newRow<0)
              newRow = 0;
printf ("new row = %d \n", newRow);
           tasks_move (position, newRow, FALSE, FALSE, data);
           tmp_value->new_row = newRow;
           tmp_value->opCode = OP_MODIFY_TASK_GROUP;
           undo_push (CURRENT_STACK_TASKS, tmp_value, data);
           tasks_update_group_limits (tmp->id, prevGroup, prevGroup, data);/* update PREVious group */
           tasks_update_group_limits (tmp->id, tmp->group, tmp->group, data);/* update New group */
        }
        tasks_free_list_groups (data);
     }
  }
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
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(data->treeViewTasksRsc));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

    if(gtk_tree_selection_get_selected (selection, &model, &iter)) {
	     gtk_tree_model_get (model, &iter, -1);
	     /* read path  */
	     path_tree = gtk_tree_model_get_path (model, &iter);
	     line = gtk_tree_path_to_string (path_tree);
             item = atoi(line);
             if(item==line_called) {
		 gtk_tree_model_get (model, &iter, USE_RSC_COLUMN, &value, -1);
		 /* change display of cehckbox */
		 value = !value;
		 gtk_list_store_set (GTK_LIST_STORE(model), &iter, USE_RSC_COLUMN, value ,-1);
             }/* endif line */
	     g_free (line);	
	     gtk_tree_path_free (path_tree);	
    }/* endif */
}

/**********************************
  LOCAL callback for toggle
  code borrowed from Ella ;-)
***********************************/
static void on_tasks_use_toggled (GtkCellRendererToggle *cell, gchar *path, APP_data *data) 
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
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(data->treeViewTasksTsk));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

    if(gtk_tree_selection_get_selected (selection, &model, &iter)) {
	     gtk_tree_model_get (model, &iter, -1);
	     /* read path  */
	     path_tree = gtk_tree_model_get_path (model, &iter);
	     line = gtk_tree_path_to_string (path_tree);
             item = atoi (line);
             if(item==line_called) {
		 gtk_tree_model_get (model, &iter, USE_TSK_COLUMN, &value, -1);
		 /* change display of cehckbox */
		 value = !value;
		 gtk_list_store_set (GTK_LIST_STORE(model), &iter, USE_TSK_COLUMN, value ,-1);
             }/* endif line */
	     g_free (line);	
	     gtk_tree_path_free (path_tree);	
    }/* endif */
}

/***********************************
  LOCAL : init tree view and
  list store for ressources
***********************************/
static void tasks_init_list_rsc (APP_data *data)
{
  GtkListStore *store = NULL;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* toggle */
  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Use"),
          renderer, "active", USE_RSC_COLUMN, NULL);
  g_object_set (renderer,"activatable" , TRUE, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (data->treeViewTasksRsc), column);

  /* toggle callback */
  g_signal_connect (renderer, "toggled", G_CALLBACK(on_rsc_use_toggled), data);
  /* avatar */
  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Avatar"),
        				renderer, "pixbuf", AVATAR_RSC_COLUMN,
        				NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (data->treeViewTasksRsc), column); 
  /* ressource name */
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT(renderer), "width-chars", 32, NULL);    
  g_object_set (G_OBJECT(renderer),"font","sans 10", NULL); 
  column = gtk_tree_view_column_new_with_attributes (_("Ressource name "),
        			renderer, "text", NAME_RSC_COLUMN,
        			NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (data->treeViewTasksRsc), column);
  /* ressource cost */
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT(renderer),"font","sans 10", NULL); 
  g_object_set (G_OBJECT(renderer), "alignment", PANGO_ALIGN_RIGHT, "align-set", TRUE, "foreground", "#F36410",
               "foreground-set", TRUE, NULL);    
  column = gtk_tree_view_column_new_with_attributes (_(" Cost "),
        			renderer, "text", COST_RSC_COLUMN,
        			NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(data->treeViewTasksRsc), column);  
  /* ressource id */
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT(renderer), "width-chars", 8, NULL);    
  g_object_set (G_OBJECT(renderer),"font","sans 10", NULL); 
  column = gtk_tree_view_column_new_with_attributes (_(" Id "),
        			renderer, "text", ID_RSC_COLUMN,
        			NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (data->treeViewTasksRsc), column); 
  store = gtk_list_store_new (NB_COL_RSC, G_TYPE_BOOLEAN, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  gtk_tree_view_set_model (GTK_TREE_VIEW(data->treeViewTasksRsc), GTK_TREE_MODEL(store));

  g_object_unref (store);/* then, will be automatically destroyed with view */

  gtk_widget_show_all (data->treeViewTasksRsc);
}

/*****************************************
  LOCAL :
  init tree view and list store for tasks
******************************************/
static void tasks_init_list_tasks (APP_data *data)
{
  GtkListStore *store = NULL;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* toggle */
  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Use"), renderer, "active", USE_TSK_COLUMN, NULL);
  g_object_set (renderer,"activatable" , TRUE, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (data->treeViewTasksTsk), column);
  /* toggle callback */
  g_signal_connect (renderer, "toggled", G_CALLBACK(on_tasks_use_toggled), data);

  /* avatar */
  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes (_(" Categ. "),
        				renderer, "pixbuf", AVATAR_TSK_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (data->treeViewTasksTsk), column); 

  /* task name */
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT(renderer), "width-chars", 32, NULL);    
  g_object_set (G_OBJECT(renderer),"font","sans 10",NULL); 
  column = gtk_tree_view_column_new_with_attributes (_(" Task name "),
        			renderer, "text", NAME_TSK_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (data->treeViewTasksTsk), column);
 
  /* task id */
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT(renderer), "width-chars", 8, NULL);    
  g_object_set (G_OBJECT(renderer),"font","sans 10",NULL); 
  column = gtk_tree_view_column_new_with_attributes (_(" Id "),
        			renderer, "text", ID_TSK_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(data->treeViewTasksTsk), column); 

  /* task link mode  */
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT(renderer), "width-chars", 8, NULL);    
  g_object_set (G_OBJECT(renderer),"font","sans 10",NULL); 
  column = gtk_tree_view_column_new_with_attributes (_("Link"),
        			renderer, "text", LINK_TSK_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW(data->treeViewTasksTsk), column);

  store = gtk_list_store_new (NB_COL_TSK, G_TYPE_BOOLEAN, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  gtk_tree_view_set_model (GTK_TREE_VIEW(data->treeViewTasksTsk), GTK_TREE_MODEL(store));

  g_object_unref (store);/* then, will be automatically destroyed with view */

  gtk_widget_show_all (data->treeViewTasksTsk);
}

/*****************************************************************************************************
  LOCAL : fill tree view and list store for ressources
  see : https://gtk.developpez.com/doc/fr/gtk/GtkTreeModel.html
  if modify == FALSE, it's a new task, so we don't have to check current 'pairs' receiver/sender
*****************************************************************************************************/
static void tasks_fill_list_rsc (gboolean modify, gint id, APP_data *data)
{
  GtkListStore *store;
  GtkTreeIter iter, toplevel;
  GtkTreeModel *model;
  gint i;
  GList *l;
  GdkPixbuf *pix2;
  gboolean valid = FALSE;
  GError *err = NULL;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (data->treeViewTasksRsc));
  store = GTK_LIST_STORE (model);

  for(i=0; i<g_list_length (data->rscList); i++) {
     gtk_list_store_append (store, &toplevel);
     l = g_list_nth (data->rscList, i);
     rsc_datas *tmp_rsc_datas;
     tmp_rsc_datas = (rsc_datas *)l->data;

     /* avatar */
     if (tmp_rsc_datas->pix) {
          pix2 = gdk_pixbuf_scale_simple (tmp_rsc_datas->pix, TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, GDK_INTERP_BILINEAR);
     }
     else {
   //     pix2 = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE);
     //   gdk_pixbuf_fill (pix2, 0x00000080);/* black 50% transparent */
            pix2 = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("def_avatar.png"), TASKS_RSC_AVATAR_SIZE,
                                                    TASKS_RSC_AVATAR_SIZE, &err);
     }
     gtk_list_store_set (store, &toplevel, AVATAR_RSC_COLUMN, pix2, -1);
     g_object_unref (pix2);
     /* use - toggle */
     if(modify) {
         /* we test association receiver-sender for current line */
         gint ok = links_check_element (id, tmp_rsc_datas->id , data);
         if(ok<0)
             gtk_list_store_set (store, &toplevel, USE_RSC_COLUMN, FALSE, -1);
         else
             gtk_list_store_set (store, &toplevel, USE_RSC_COLUMN, TRUE, -1);
     }
     else {
         gtk_list_store_set (store, &toplevel, USE_RSC_COLUMN, FALSE, -1);
     }
     /* name */
     gtk_list_store_set (store, &toplevel, NAME_RSC_COLUMN, g_strdup_printf ("%s", tmp_rsc_datas->name ), -1);
     /* cost */
     gtk_list_store_set (store, &toplevel, COST_RSC_COLUMN, g_strdup_printf ("%.2f", tmp_rsc_datas->cost ), -1);
     /* id */
     gtk_list_store_set (store, &toplevel, ID_RSC_COLUMN, g_strdup_printf ("%d", tmp_rsc_datas->id ), -1);
  }


// accès direct
return;
gint compteur_ligne=0;
  gchar *str_data;
   gchar *int_data;
   gboolean status;
   gtk_tree_model_get_iter_from_string (model, &iter, g_strdup_printf ("%d",0));
      gtk_tree_model_get (model, &iter, 
                            USE_RSC_COLUMN, &status,
                          NAME_RSC_COLUMN, &str_data,
                          ID_RSC_COLUMN, &int_data,
                          -1);
printf("par accès direct : \n");
      g_print ("Rangée %d: flag = %d (%s,%d)\n", compteur_ligne, status, str_data, atoi(int_data));
// status 0 = FALSE, status 1 = TRUE
      g_free (str_data);
      g_free (int_data);
}

/*****************************************************************************************************
  LOCAL : fill tree view and list store for tasks
  if modify == FALSE, it's a new task, so we don't have to check current 'pairs' receiver/sender
  if check_date == FALSE, we allow to use ANY preceding task, in auto-placement mode
*****************************************************************************************************/
static void tasks_fill_list_tasks (gboolean modify, gint id, gboolean check_date, APP_data *data)
{
  GtkListStore *store;
  GtkTreeIter  iter, toplevel, child;
  GtkTreeModel *model;
  gint i, ret = -1, cmp_date, mode;
  GList *l;
  GError *err = NULL;
  GdkPixbuf *pix;
  tasks_data *tmp_tsk_datas;
  GDate date_task_start_receiver, date_task_end_sender;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (data->treeViewTasksTsk));
  store = GTK_LIST_STORE (model);

  /* default dates if list length == 0 or == 1 */
  g_date_set_time_t (&date_task_start_receiver, time (NULL)); /* date of today */
  /* we get starting date of current task */
  i = 0;
  while( (i<g_list_length (data->tasksList) ) && (ret<0)) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     if(tmp_tsk_datas->id == id) {
        ret = id;
        g_date_set_dmy (&date_task_start_receiver, 
                  tmp_tsk_datas->start_nthDay, tmp_tsk_datas->start_nthMonth, tmp_tsk_datas->start_nthYear);
     }
     i++;
  }/* wend */

  for(i=0; i<g_list_length (data->tasksList); i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tsk_datas = (tasks_data *)l->data;
     /* we check if potential "sending" task has a compatible ending date */
     g_date_set_dmy (&date_task_end_sender, 
                  tmp_tsk_datas->end_nthDay, tmp_tsk_datas->end_nthMonth, tmp_tsk_datas->end_nthYear);

     cmp_date = g_date_compare (&date_task_end_sender, &date_task_start_receiver);
     /* a simple trick */
printf ("je teste pour %s \n", tmp_tsk_datas->name);
     if(!check_date)
       cmp_date = -1;/* force to NOT check */
     if((cmp_date <0) && (id != tmp_tsk_datas->id) && (tmp_tsk_datas->type != TASKS_TYPE_GROUP)) {
       gtk_list_store_append(store, &toplevel);
       if(tmp_tsk_datas->type == TASKS_TYPE_TASK) {
	       /* avatar - category */
	       switch(tmp_tsk_datas->category) {
		 case 0: {/* mat */
		   pix = gdk_pixbuf_new_from_file_at_size ( find_pixmap_file ("material.png"), 
		                  TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);
		   gtk_list_store_set (store, &toplevel, AVATAR_TSK_COLUMN, pix, -1);
		   g_object_unref (pix);
		   break;
		 }
		 case 1: {/* intell */
		    pix = gdk_pixbuf_new_from_file_at_size ( find_pixmap_file ("intellectual.png"), 
		                  TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);
		   gtk_list_store_set (store, &toplevel, AVATAR_TSK_COLUMN, pix, -1);
		   g_object_unref (pix);
		   break;
		 }
		 case 2: {/* soc */
		   pix = gdk_pixbuf_new_from_file_at_size ( find_pixmap_file ("relational.png"), 
		                  TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);
		   gtk_list_store_set (store, &toplevel, AVATAR_TSK_COLUMN, pix, -1);
		   g_object_unref (pix); 
		   break;
		 }
	       }/* end sw categ */
       }
       else {/* milestones */
          pix = gdk_pixbuf_new_from_file_at_size ( find_pixmap_file ("milestone.png"), 
		                  TASKS_RSC_AVATAR_SIZE, TASKS_RSC_AVATAR_SIZE, &err);
	  gtk_list_store_set (store, &toplevel, AVATAR_TSK_COLUMN, pix, -1);
	  g_object_unref (pix); 
       }
       /* use - toggle */
        if(modify) {
            /* we test association receiver-sender for current line */
            gint ok = links_check_element (id, tmp_tsk_datas->id , data);
            if(ok<0)
                gtk_list_store_set (store, &toplevel, USE_TSK_COLUMN, FALSE, -1);
            else
                gtk_list_store_set (store, &toplevel, USE_TSK_COLUMN, TRUE, -1);
        }
        else {
            gtk_list_store_set (store, &toplevel, USE_TSK_COLUMN, FALSE, -1);
        }
        /* name */
        gtk_list_store_set (store, &toplevel, NAME_TSK_COLUMN, g_strdup_printf ("%s", tmp_tsk_datas->name), -1);
        /* id */
        gtk_list_store_set (store, &toplevel, ID_TSK_COLUMN, g_strdup_printf ("%d", tmp_tsk_datas->id), -1);
        /* link */
        mode = links_get_element_link_mode (id, tmp_tsk_datas->id, data);
        switch(mode) {
           case LINK_END_START: {
             gtk_list_store_set (store, &toplevel, LINK_TSK_COLUMN, _("E>>S"), -1);/* 8 chars max */
             break;
           }
           case LINK_START_START: {
             gtk_list_store_set (store, &toplevel, LINK_TSK_COLUMN, _("S>>S"), -1);
             break;
           }
           case LINK_END_END: {
             gtk_list_store_set (store, &toplevel, LINK_TSK_COLUMN, _("E>>E"), -1);
             break;
           }
           case LINK_START_END: {
             gtk_list_store_set (store, &toplevel, LINK_TSK_COLUMN, _("S>>E"), -1);
             break;
           }
           default: {
             gtk_list_store_set (store, &toplevel, LINK_TSK_COLUMN, _("---"), -1);
           }
        } /* end switch */ 
     }/* endif ID */
  }/* next */

}

/*************************************************
  PUBLIC :main tasks dialog if date!=NULL, we use
  date as parameter
*************************************************/
GtkWidget *tasks_create_dialog (GDate *date, gboolean modify, gint id, APP_data *data)
{
  GtkWidget *dialog;
  /* get datas from xml Glade file - CRITICAL : only in activate phase !!!! */
  GtkBuilder *builder = NULL;
  builder = gtk_builder_new ();
  GError *err = NULL;
  gint i;
  GList *l;
  tsk_group_type *tmp;
  gchar *tmpStr, buffer[164];
  GdkPixbuf *pix;

  if(!gtk_builder_add_from_file (builder, find_ui_file ("tasks.ui"), &err)) {
     g_error_free (err);
     misc_halt_after_glade_failure (data);
  }
  /* Get the dialog from the glade file. */
  gtk_builder_connect_signals (builder, data);
  dialog = GTK_WIDGET(gtk_builder_get_object (builder, "dialogTasks"));
  data->tmpBuilder = builder;

  /* header bar */
  GtkWidget *hbar = gtk_header_bar_new ();
  gtk_widget_show (hbar);
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (hbar), _("Define or modify a task."));
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (hbar), TRUE);
  gtk_window_set_titlebar (GTK_WINDOW(dialog), hbar);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  
  gtk_header_bar_pack_start (GTK_HEADER_BAR (hbar), box);

  pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("process.svg"), 48, 48, &err);
  GtkWidget *icon = gtk_image_new_from_pixbuf (pix);
  g_object_unref (pix);

  // GtkWidget *icon =  gtk_image_new_from_icon_name  ("gtk-execute", GTK_ICON_SIZE_DIALOG);
  gtk_container_add (GTK_CONTAINER (box), icon);
  gtk_widget_show (box);
  gtk_widget_show (icon);


  /* we set-tup the listStores */
  data->treeViewTasksRsc = gtk_tree_view_new();
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(data->treeViewTasksRsc), TRUE);
  gtk_container_add (GTK_CONTAINER (GTK_WIDGET(gtk_builder_get_object (builder, "viewportTasksRsc"))), 
                     data->treeViewTasksRsc);
  data->treeViewTasksTsk = gtk_tree_view_new();
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(data->treeViewTasksTsk), TRUE);
  gtk_container_add (GTK_CONTAINER (GTK_WIDGET(gtk_builder_get_object (builder, "viewportTasksTsk"))), 
                     data->treeViewTasksTsk);
  /* initialize treeviews */
  tasks_init_list_rsc (data);
  tasks_init_list_tasks (data);
  /* fill treeviews */
  tasks_fill_list_rsc (modify, id, data);
  tasks_fill_list_tasks (modify, id, FALSE, data);
  /* we setup various default values */

  GtkWidget *entryTaskName = GTK_WIDGET(gtk_builder_get_object (builder, "entryTaskName"));
  GtkWidget *comboPriority = GTK_WIDGET(gtk_builder_get_object (builder, "comboboxTasksPriority"));
  GtkWidget *comboCategory = GTK_WIDGET(gtk_builder_get_object (builder, "comboboxTasksCategory"));
  GtkWidget *pBtnColor = GTK_WIDGET(gtk_builder_get_object (builder, "colorbuttonTasks"));
  GtkWidget *comboGroup = GTK_WIDGET(gtk_builder_get_object (builder, "comboGroup"));
  /* groups */
  tasks_fill_list_groups (data);
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(comboGroup), _("None"));
  gtk_combo_box_set_active (GTK_COMBO_BOX(comboGroup), 0);/* default */  
  for(i=1;i<g_list_length (data->groups);i++) {
      l = g_list_nth (data->groups, i);
      tmp = (tsk_group_type *) l->data;
      if(strlen(tmp->name)<40)
          gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(comboGroup), tmp->name);    
      else {
         //tmpStr = g_strdup_printf ("%.39s...", tmp->name);
         g_utf8_strncpy (buffer, tmp->name, 39);
         gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(comboGroup), buffer);
         //g_free (tmpStr); 
      }
  }

  GtkWidget *pButtonStart = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonStartDate"));  
  if(date) {
    gtk_button_set_label (GTK_BUTTON(pButtonStart), misc_convert_date_to_locale_str (date));
  }
  else {
     gtk_button_set_label (GTK_BUTTON(pButtonStart), data->properties.start_date);
  }
  GtkWidget *pButton = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonTasksDueDate"));  
  gtk_button_set_label (GTK_BUTTON(pButton), data->properties.start_date);

  /* invalidate some widgets, according to 'modify' flag */
  if(modify) {
    //   gtk_window_set_title (GTK_WINDOW(dialog), _("Modify a task" ));
    gtk_header_bar_set_title (GTK_HEADER_BAR (hbar), _("Modify a task"));
  }
  else {
    //    gtk_window_set_title (GTK_WINDOW(dialog), _("New task" ));
    gtk_header_bar_set_title (GTK_HEADER_BAR (hbar), _("New task"));
  }

  gtk_window_set_transient_for (GTK_WINDOW(dialog),
                              GTK_WINDOW(data->appWindow));
  return dialog;
}

/****************************
  PUBLIC 
  main tasks group dialog
****************************/
GtkWidget *tasks_create_group_dialog (gboolean modify, gint id, APP_data *data)
{
  GtkWidget *dialog;
  /* get datas from xml Glade file - CRITICAL : only in activate phase !!!! */
  GtkBuilder *builder =NULL;
  builder = gtk_builder_new();
  GError *err = NULL;
  GdkPixbuf *pix; 

  if(!gtk_builder_add_from_file (builder, find_ui_file ("group.ui"), &err)) {
     g_error_free (err);
     misc_halt_after_glade_failure (data);
  }
  pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("group.png"), 48, 48, &err);
  /* Get the dialog from the glade file. */
  gtk_builder_connect_signals (builder, data);
  dialog = GTK_WIDGET(gtk_builder_get_object (builder, "dialogGroup"));
  data->tmpBuilder=builder;

 /* header bar */
  GtkWidget *hbar = gtk_header_bar_new ();
  gtk_widget_show (hbar);
  gtk_header_bar_set_title (GTK_HEADER_BAR (hbar), _("Group of tasks"));
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (hbar), _("Define or modify a group of tasks"));
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (hbar), TRUE);
  gtk_window_set_titlebar (GTK_WINDOW(dialog), hbar);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  
  gtk_header_bar_pack_start (GTK_HEADER_BAR (hbar), box);

  GtkWidget *icon =  gtk_image_new_from_pixbuf (pix);
  g_object_unref (pix);

  gtk_container_add (GTK_CONTAINER (box), icon);
  gtk_widget_show (box);
  gtk_widget_show (icon);

 
  /* invalidate some widgets, according to 'modify' flag */
  if(modify) {
     gtk_widget_show (GTK_WIDGET(gtk_builder_get_object (builder, "buttonRemoveGroup"))); 
  //   gtk_window_set_title (GTK_WINDOW(dialog),_("Modify a Group" ));
  gtk_header_bar_set_title (GTK_HEADER_BAR (hbar), _("Modify a Group"));
  }
  else {
     gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object (builder, "buttonRemoveGroup"))); 
   //  gtk_window_set_title (GTK_WINDOW(dialog),_("New Group" ));
  gtk_header_bar_set_title (GTK_HEADER_BAR (hbar), _("New Group"));
  }
  
  gtk_window_set_transient_for (GTK_WINDOW(dialog),
                              GTK_WINDOW(data->appWindow));
  return dialog;
}

/****************************
  PUBLIC 
  main milestone dialog
****************************/
GtkWidget *tasks_create_milestone_dialog (gboolean modify, gint id, APP_data *data)
{
  GtkWidget *dialog;
  /* get datas from xml Glade file - CRITICAL : only in activate phase !!!! */
  GtkBuilder *builder = NULL;
  builder = gtk_builder_new ();
  GError *err = NULL;
  GdkPixbuf *pix;  

  if(!gtk_builder_add_from_file (builder, find_ui_file ("milestone.ui"), &err)) {
     g_error_free (err);
     misc_halt_after_glade_failure (data);
  }
  /* Get the dialog from the glade file. */
  gtk_builder_connect_signals (builder, data);

  dialog = GTK_WIDGET(gtk_builder_get_object (builder, "dialogMilestone"));
  data->tmpBuilder=builder;

  pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("milestone.png"), 48, 48, &err);
 /* header bar */
  GtkWidget *hbar = gtk_header_bar_new ();
  gtk_widget_show (hbar);
  gtk_header_bar_set_title (GTK_HEADER_BAR (hbar), _("Milestone"));
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (hbar), _("Define or modify a milestone"));
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (hbar), TRUE);
  gtk_window_set_titlebar (GTK_WINDOW(dialog), hbar);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  
  gtk_header_bar_pack_start (GTK_HEADER_BAR (hbar), box);

  GtkWidget *icon =  gtk_image_new_from_pixbuf (pix);
  g_object_unref (pix);

  gtk_container_add (GTK_CONTAINER (box), icon);
  gtk_widget_show (box);
  gtk_widget_show (icon);

  /* invalidate some widgets, according to 'modify' flag */
  if(modify) {
     gtk_widget_show (GTK_WIDGET(gtk_builder_get_object (builder, "buttonRemove"))); 
  //   gtk_window_set_title (GTK_WINDOW(dialog),_("Modify a Milestone" ));
  gtk_header_bar_set_title (GTK_HEADER_BAR (hbar), _("Modify a Milestone"));
  }
  else {
     gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object (builder, "buttonRemove"))); 
  //  gtk_window_set_title (GTK_WINDOW(dialog),_("New Milestone" ));
  gtk_header_bar_set_title (GTK_HEADER_BAR (hbar), _("New Milestone"));
  }

  /* we set-tup the listStores */
  data->treeViewTasksTsk = gtk_tree_view_new ();
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(data->treeViewTasksTsk), TRUE);
  gtk_container_add (GTK_CONTAINER (GTK_WIDGET(gtk_builder_get_object (builder, "viewportTasksTsk"))), 
                     data->treeViewTasksTsk);
  /* initialize treeviews */
  tasks_init_list_tasks (data);
  /* fill treeviews */
  tasks_fill_list_tasks (modify, id, FALSE, data);
  /* we setup various default values */

  gtk_window_set_transient_for (GTK_WINDOW(dialog),
                              GTK_WINDOW(data->appWindow));
  return dialog;
}

/************************************
  PUBLIC : callback when user
  (un)checks limit times repeat button
***************************************/
void on_check_limit_toggled (GtkToggleButton *togglebutton, APP_data *data)
{
  GtkWidget *boxTimes = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "boxTimes")); 
  if(gtk_toggle_button_get_active(togglebutton))
      gtk_widget_set_sensitive (GTK_WIDGET(boxTimes), TRUE);
  else
      gtk_widget_set_sensitive (GTK_WIDGET(boxTimes), FALSE);
}

/************************************
  PUBLIC : callback when user
  changes month combobox
************************************/
void on_combo_rep_month_changed (GtkComboBox *widget, APP_data *data)
{
  gint month;
  GtkWidget *check30, *check31;

  month = gtk_combo_box_get_active (GTK_COMBO_BOX(widget))+1;
  check30 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "check30"));
  check31 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "check31"));
  /* we activate-deactivate some widgets ; 30 and 31 */
  if(month == 1 || month ==3 || month ==5 || month ==7 || month ==8 || month ==10 || month ==12) {
       gtk_widget_show (GTK_WIDGET(check30));
       gtk_widget_show (GTK_WIDGET(check31));
  }
  else {
    if(month ==2) {
       gtk_widget_hide (GTK_WIDGET(check30));
       gtk_widget_hide (GTK_WIDGET(check31));
    }
    else {
       gtk_widget_show (GTK_WIDGET(check30));
       gtk_widget_hide (GTK_WIDGET(check31));
    }
  }

}
/************************************
  PUBLIC : callback when user
  changes repeating mode combobox
************************************/
void on_combo_rep_mode_changed (GtkComboBox *widget, APP_data *data)
{
  gint active, month;

  GtkWidget *boxDays = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "boxDays"));
  GtkWidget *gridNumDays = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "gridNumDays"));
  GtkWidget *labelTypeInterval = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "labelTypeInterval"));
  GtkWidget *boxMonth = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "boxMonth"));
  GtkWidget *check30 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "check30"));
  GtkWidget *check31 = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "check31"));
  GtkWidget *comboMonth= GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboMonth"));
  active = gtk_combo_box_get_active (GTK_COMBO_BOX(widget));
  switch(active) {
     case 0:{/* weekly */
       gtk_widget_show (GTK_WIDGET(boxDays));
       gtk_widget_hide (GTK_WIDGET(gridNumDays));
       gtk_widget_hide (GTK_WIDGET(boxMonth));
       gtk_label_set_text (GTK_LABEL(labelTypeInterval), _("week(s)"));
       break;
     }
     case 1:{/* monthly */
       gtk_widget_hide (GTK_WIDGET(boxDays));
       gtk_widget_show (GTK_WIDGET(gridNumDays));
       gtk_widget_hide (GTK_WIDGET(boxMonth));
       gtk_widget_show (GTK_WIDGET(check30));
       gtk_widget_show (GTK_WIDGET(check31));
       gtk_combo_box_set_active (GTK_COMBO_BOX(comboMonth), 0);/* force to january and 31 days - backgrounded */
       gtk_label_set_text (GTK_LABEL(labelTypeInterval), _("month(s)"));
       break;
     }
     case 2:{/* yearly */
       gtk_widget_hide (GTK_WIDGET(boxDays));
       gtk_widget_show (GTK_WIDGET(gridNumDays));
       gtk_widget_show (GTK_WIDGET(boxMonth));
       gtk_label_set_text (GTK_LABEL(labelTypeInterval), _("year(s)"));
       break;
     }
  }/* end switch */

}

/****************************
  PUBLIC 
  task repeat dialog
****************************/
GtkWidget *tasks_create_repeat_dialog (gchar *name, gchar *strDate, APP_data *data)
{
  GtkWidget *dialog, *lblName, *lblDate, *image1;
  GdkPixbuf *pix;
  /* get datas from xml Glade file - CRITICAL : only in activate phase !!!! */
  GtkBuilder *builder =NULL;
  builder = gtk_builder_new();
  GError *err = NULL; 

  if(!gtk_builder_add_from_file (builder, find_ui_file ("taskrepeat.ui"), &err)) {
     g_error_free (err);
     misc_halt_after_glade_failure (data);
  }

  /* Get the dialog from the glade file. */
  gtk_builder_connect_signals (builder, data);
  dialog = GTK_WIDGET(gtk_builder_get_object (builder, "dialogTskRep"));
  data->tmpBuilder=builder;
  /* header bar */

  GtkWidget *hbar = gtk_header_bar_new ();
  gtk_widget_show (hbar);
  gtk_header_bar_set_title (GTK_HEADER_BAR (hbar), _("Recurring task"));
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (hbar), _("The task will be repeated, according to your choices."));
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (hbar), TRUE);
  gtk_window_set_titlebar (GTK_WINDOW(dialog), hbar);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);  
  gtk_header_bar_pack_start (GTK_HEADER_BAR (hbar), box);
  pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("repeat_calendar.png"), 48, 48, &err);
  GtkWidget *icon =  gtk_image_new_from_pixbuf (pix);
  gtk_container_add (GTK_CONTAINER (box), icon);
  gtk_widget_show (box);
  gtk_widget_show (icon);
  g_object_unref (pix);

  lblName = GTK_WIDGET(gtk_builder_get_object (builder, "labelTskRepName"));
  lblDate = GTK_WIDGET(gtk_builder_get_object (builder, "labelDate"));

  if(name)
      gtk_label_set_text (GTK_LABEL(lblName), name);
  else
      gtk_label_set_text (GTK_LABEL(lblName), _("Name: ? "));

  if(strDate)
      gtk_label_set_text (GTK_LABEL(lblDate), strDate);
  else
      gtk_label_set_text (GTK_LABEL(lblDate), _("Date : ? "));

  gtk_window_set_transient_for (GTK_WINDOW(dialog),
                              GTK_WINDOW(data->appWindow));      

  return dialog;
}

/**************************************
  standard err msg for task repeating
**************************************/
void task_repeat_error_dialog (GtkWidget *win)
{
  gchar *msg;

  msg = g_strdup_printf ("%s",
         _("<b>Error!</b>\n\nYou've asked to place a task <i>after</i> project's end.\n\n\nPlease check the dates, or modify project's properties to unlock dates."));
  misc_ErrorDialog (GTK_WIDGET(win), msg);
  g_free (msg);
}

/*************************
  Public :  remove current
  task/milestone/group
*************************/
void on_tasks_remove (APP_data *data)
{
  gint iRow=-1;
  tasks_data temp, newTasks, *undoTasks;
  undo_datas *tmp_value;
  GtkWidget *boxTasks;
  GtkListBoxRow *row;

  boxTasks = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasks"));
  row = gtk_list_box_get_selected_row (GTK_LIST_BOX(boxTasks));
  iRow  = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW(row));
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

  tasks_remove (iRow, GTK_LIST_BOX(boxTasks), data);  /* dashboard is managed inside - common for tasks, groups an milestones */
  /* update timeline - please note, the task is removed by 'task_remove()' function */
  timeline_remove_all_tasks (data);/* YES, for other tasks */
  timeline_draw_all_tasks (data);
  /* we update assignments */
  assign_remove_all_tasks (data);
  assign_draw_all_visuals (data);
  report_clear_text (data->buffer, "left");
  misc_display_app_status (TRUE, data);

}
