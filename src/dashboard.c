/**********************************
  management of "dashboard"
 (Kanban)  panel

**********************************/

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
#include "tasks.h"
#include "tasksutils.h"
#include "dashboard.h"
#include "assign.h"
#include "undoredo.h"


/* for DnD */

static GtkTargetEntry entries[] = {
  { "GTK_LIST_BOX_ROW", GTK_TARGET_SAME_APP, 0 }
};

static gint DnDId, countWait = 0, countRun = 0, countDelay = 0, countDone = 0;
static GtkWidget *pCol_wait = NULL, *pCol_run = NULL, *pCol_delay = NULL, *pCol_done = NULL;


/**********************************
  PUBLIC : reset counters
**********************************/
void dashboard_reset_counters (APP_data *data)
{
   countWait = 0;
   countRun = 0;
   countDelay = 0;
   countDone = 0;
}

/**********************************
  PROTECTED : get current status 
  for a task knwon by its internal
  ID
***********************************/
static gint dashboard_get_status (gint id, APP_data *data)
{
  gint i, ret = -1;
  gint tsk_list_len = g_list_length (data->tasksList);
  GList *l;
  tasks_data *tmp_tasks_datas;

  i = 0;
  while(i<tsk_list_len  && ret<0) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     if( tmp_tasks_datas-> id == id) {
        ret = tmp_tasks_datas->status;
     }
     i++;
  }/* wend */
  return ret;
}

/**********************************
  PUBLIC : set current status 
  for a task knwon by its internal
  ID
***********************************/
void dashboard_set_status (gint id, APP_data *data)
{
  gint i, ret = -1, cmp, curStatus = TASKS_STATUS_TO_GO, cmp2;
  gint tsk_list_len = g_list_length (data->tasksList);
  GList *l;
  tasks_data *tmp_tasks_datas;
  GDate date, start_date, end_date;
/*
#define TASKS_STATUS_RUN 0
#define TASKS_STATUS_DONE 1
#define TASKS_STATUS_DELAY 2
#define TASKS_STATUS_TO_GO 3

*/
  i = 0;
  while(i<tsk_list_len  && ret<0) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     if(tmp_tasks_datas-> id == id) {
        ret = tmp_tasks_datas->status;
        /* now we compare to current date */
        g_date_set_time_t (&date, time (NULL)); /* date of today */
        g_date_set_dmy (&start_date, tmp_tasks_datas->start_nthDay, tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
        g_date_set_dmy (&end_date, tmp_tasks_datas->end_nthDay, tmp_tasks_datas->end_nthMonth, tmp_tasks_datas->end_nthYear);
        /* test if task if overdue */
        cmp = g_date_compare (&date, &end_date);
        if(cmp>0) { 
            if(tmp_tasks_datas->progress<100)
                curStatus = TASKS_STATUS_DELAY;
            else
                curStatus = TASKS_STATUS_DONE;
        }
        else {/* at current today date, task ending date isn't reached */
           cmp2 = g_date_compare (&date, &start_date);
           if(cmp2>0) {/* we have passed starting date */
              if(tmp_tasks_datas->progress>0)             
                curStatus = TASKS_STATUS_RUN;
              else
                curStatus = TASKS_STATUS_TO_GO;
           }
           else {/* today date is before starting date */
                curStatus = TASKS_STATUS_TO_GO;
           }
        }/* else if cmp */
        tmp_tasks_datas->status = curStatus;
     }/* endif id */
     i++;
  }/* wend */
  
}


/**********************************
  PROTECTED - drag n drop

**********************************/
static void
drag_begin (GtkWidget      *widget,
            GdkDragContext *context,
            gchar           *data)
{
  GtkWidget *row;
  GtkAllocation alloc;
  cairo_surface_t *surface;
  cairo_t *cr;
  int x, y;

  row = gtk_widget_get_ancestor (widget, GTK_TYPE_LIST_BOX_ROW);
  gtk_widget_get_allocation (row, &alloc);
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, alloc.width, alloc.height);
  cr = cairo_create (surface);

  gtk_style_context_add_class (gtk_widget_get_style_context (row), "drag-icon");
  gtk_widget_draw (row, cr);
  gtk_style_context_remove_class (gtk_widget_get_style_context (row), "drag-icon");

  gtk_widget_translate_coordinates (widget, row, 0, 0, &x, &y);
  cairo_surface_set_device_offset (surface, -x, -y);
  gtk_drag_set_icon_surface (context, surface);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  DnDId = atoi (data);

}

void
drag_data_get (GtkWidget        *widget,
               GdkDragContext   *context,
               GtkSelectionData *selection_data,
               guint             info,
               guint             time,
               gchar          *data)
{
  gtk_selection_data_set (selection_data,
                          gdk_atom_intern_static_string ("GTK_LIST_BOX_ROW"),
                          32,
                          (const guchar *)&widget,
                          sizeof (gpointer));
printf ("dans data get =%s \n", data);// donne tj la bonne référence d'objet au départ et à l'arrivée
}



/************************************
 PROTECTED - DnD reception
/************************************/
static void
drag_data_received (GtkWidget        *widget,
                    GdkDragContext   *context,
                    gint              x,
                    gint              y,
                    GtkSelectionData *selection_data,
                    guint             info,
                    guint32           time,
                    APP_data          *data)
{
  GtkWidget *target, *row, *source;
  GtkWidget *labelWait, *labelRun, *labelDelay, *labelDone;
  gint i, ret, pos, origin, col_recept, prev_status;
  gdouble prev_progress;
  const gchar *strBoxName;
  tasks_data *tmp_tasks_datas;

  if(DnDId == -1) 
     return;
  target = widget;
  pos = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (target));
  
  /* we compute nth of column according to its name */
  strBoxName = gtk_widget_get_name (GTK_WIDGET(gtk_widget_get_parent (target)));
  if(g_strcmp0 (strBoxName, "col_wait")==0) {
     col_recept = 0;
  }
  if(g_strcmp0 (strBoxName, "col_run")==0) {
     col_recept = 1;
  }
  if(g_strcmp0 (strBoxName, "col_delay")==0) {
     col_recept = 2;
  }
  if(g_strcmp0 (strBoxName, "col_done")==0) {
     col_recept = 3;
  }
printf ("objet d'ID %d reçu dans colonne =%d pos =%d\n", DnDId, col_recept, pos);

  row = (gpointer)* (gpointer*)gtk_selection_data_get_data (selection_data);
  source = gtk_widget_get_ancestor (row, GTK_TYPE_LIST_BOX_ROW);

  origin = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (source));
  prev_status = dashboard_get_status (DnDId, data);
printf ("status d'origine =%d pour cet ID =%d \n",   prev_status , DnDId   );
  /* if status changed, we change it in place */

  gint tsk_list_len = g_list_length (data->tasksList);
  GList *l;

  i = 0;
  ret = -1;
  
  while(i<tsk_list_len  && ret<0) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     if(tmp_tasks_datas->id == DnDId) {
        ret = 1;
        pos = i;
        prev_progress = tmp_tasks_datas->progress;
        switch(col_recept) {
           case 0:{
             tmp_tasks_datas->status = TASKS_STATUS_TO_GO;
             tmp_tasks_datas->progress = 0;
             break;
           }
           case 1:{
             tmp_tasks_datas->status = TASKS_STATUS_RUN;
             if(prev_progress == 100)
                tmp_tasks_datas->progress = 90;
             break;
           }
           case 2:{
             tmp_tasks_datas->status = TASKS_STATUS_DELAY;
             if(prev_progress == 100)
                 tmp_tasks_datas->progress = 99;
             break;
           }
           case 3:{
             tmp_tasks_datas->status =  TASKS_STATUS_DONE;
             tmp_tasks_datas->progress = 100;
             break;
           }
        }/* end switch */
        /* we change tasks pane if new status is changed from prev status */
        if(prev_status != tmp_tasks_datas->status) {
           /* undo engine */
           undo_datas *tmp_value;
           tasks_data temp, *undoTasks;
           undoTasks = g_malloc (sizeof(tasks_data));
           tasks_get_ressource_datas (pos, undoTasks, data);
           tmp_value = g_malloc (sizeof(undo_datas));
           tmp_value->opCode = OP_MOVE_DASH;
           undoTasks->name = NULL;
           undoTasks->location = NULL;
           undoTasks->progress = prev_progress;
           tmp_value->insertion_row = pos;
           tmp_value->id = tmp_tasks_datas->id;
           undoTasks->status = prev_status;
           tmp_value->datas = undoTasks;
           tmp_value->list = NULL;
           tmp_value->groups = NULL;
           undo_push (CURRENT_STACK_TASKS, tmp_value, data);
	   /* get pointers on labels */
	   labelWait = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelWait"));
	   labelRun = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelRun"));
	   labelDelay = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelDelay"));
	   labelDone = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelDone"));
           /* change status counts */
           if(prev_status == TASKS_STATUS_TO_GO)
                countWait--;
           if(prev_status == TASKS_STATUS_RUN)
                countRun--;
           if(prev_status == TASKS_STATUS_DELAY)
                countDelay--;
           if(prev_status == TASKS_STATUS_DONE)
                countDone--;
           /* increment new status */
           if(tmp_tasks_datas->status == TASKS_STATUS_TO_GO)
                countWait++;
           if(tmp_tasks_datas->status == TASKS_STATUS_RUN)
                countRun++;
           if(tmp_tasks_datas->status == TASKS_STATUS_DELAY)
                countDelay++;
           if(tmp_tasks_datas->status == TASKS_STATUS_DONE)
                countDone++;
	   /* we change labels */
	   gtk_label_set_text (GTK_LABEL(labelWait), g_strdup_printf ("%d", countWait));
	   gtk_label_set_text (GTK_LABEL(labelRun), g_strdup_printf ("%d", countRun));
	   gtk_label_set_text (GTK_LABEL(labelDelay), g_strdup_printf ("%d", countDelay));
	   gtk_label_set_text (GTK_LABEL(labelDone), g_strdup_printf ("%d", countDone));
           gtk_label_set_markup (GTK_LABEL(tmp_tasks_datas->cDashProgress), 
                               g_strdup_printf ("<b>%.1f%</b>", tmp_tasks_datas->progress));
           tasks_modify_widget (pos, data);

           /* we update assignments */
           assign_remove_all_tasks (data);
           assign_draw_all_visuals (data);
        }
     }/* endif ID */
     i++;
  }/* wend */

  if(source == target)
    return;
  report_clear_text (data->buffer, "left");
  misc_display_app_status (TRUE, data);
  g_object_ref (source);
  gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (source)), source);
  gtk_list_box_insert (GTK_LIST_BOX (gtk_widget_get_parent (target)), source, pos);
  g_object_unref (source);

}

/*************************
 PUBLIC :
 add a new task widget
*************************/
GtkWidget *dashboard_new_widget (gchar *name, gchar *periods, 
                            gdouble progress, gint pty, gint categ, GdkRGBA color, gint status, gint id, tasks_data *tmp, APP_data *data)
{
  GtkWidget *row, *handle, *cadreGrid, *cadre;
  GtkWidget *iconBox, *textBox, *subTextBox, *participBox, *subparticipBox, *rscBox, *interBox, *interIcoBox, *taskBox;
  GtkWidget *separator1, *separator2;
  GtkWidget *label_name, *label_periods, *label_progress, *imgPty, *imgCateg;
  GdkPixbuf *pix;
  gchar *sRgba, *tmpStr;
  GError *err = NULL;

  row = gtk_list_box_row_new ();
  /* event box is mandatory */
  handle = gtk_event_box_new ();
  g_object_set (row, "margin", 1, NULL);
  gtk_container_add (GTK_CONTAINER (row), handle);
  gtk_widget_show (handle);

  cadre = gtk_frame_new (NULL);
  g_object_set (cadre, "margin-left", 2, NULL);
  g_object_set (cadre, "margin-right", 2, NULL);
  g_object_set (cadre, "margin-top", 2, NULL);
  g_object_set (cadre, "margin-bottom", 2, NULL);
  gtk_widget_show (cadre);
  gtk_container_add (GTK_CONTAINER (handle), cadre);


  cadreGrid = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0 );
  g_object_set (cadreGrid, "margin-left", 4, NULL);
  gtk_container_add (GTK_CONTAINER (cadre), cadreGrid);

  iconBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0 );
  textBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0 );
  interIcoBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0 );
  interBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0 );
  g_object_set (interBox, "margin-bottom", 4, NULL);
  subTextBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0 );
  participBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0 );
  subparticipBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0 );
  GtkWidget *supTaskBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0 );
  g_object_set (participBox, "margin-left", 4, NULL);
  rscBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0 );

  separator2 = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
  g_object_set (separator2, "margin-left", 8, NULL);
  g_object_set (separator2, "margin-right", 8, NULL);

  gtk_box_pack_start (GTK_BOX(cadreGrid), textBox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(cadreGrid), interIcoBox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(cadreGrid), interBox, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX(interBox), participBox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(interBox), separator2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(interBox), supTaskBox, FALSE, FALSE, 0);

  /* text box */
  /* name */
  sRgba = misc_convert_gdkcolor_to_hex (color);
  tmpStr = g_markup_printf_escaped ("<big><span background=\"%s\">    </span><b> %s</b></big>", sRgba, name);
  label_name = gtk_label_new (tmpStr);
  g_free (sRgba);
  g_free (tmpStr);
  gtk_widget_set_halign (label_name, GTK_ALIGN_START);
  g_object_set (label_name,  "xalign", 0, NULL);
  gtk_label_set_use_markup (GTK_LABEL(label_name), TRUE);
  gtk_label_set_ellipsize (GTK_LABEL(label_name), PANGO_ELLIPSIZE_END);
  gtk_label_set_width_chars (GTK_LABEL(label_name), 28);
  gtk_label_set_max_width_chars (GTK_LABEL(label_name), 28);
  gtk_box_pack_start (GTK_BOX(textBox), label_name, FALSE, FALSE, 4);
  /* periods */
  label_periods = gtk_label_new (g_strdup_printf ("<i>%s</i>", periods));
  gtk_widget_set_halign (label_periods, GTK_ALIGN_START);
  g_object_set (label_periods,  "yalign", 0.5, NULL);
  gtk_label_set_use_markup (GTK_LABEL(label_periods), TRUE);
  gtk_box_pack_start (GTK_BOX(textBox), label_periods, FALSE, FALSE, 4);

  gtk_box_pack_start (GTK_BOX(interIcoBox), iconBox, FALSE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX(interIcoBox), subTextBox, FALSE, FALSE, 4);
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
  gtk_label_set_use_markup (GTK_LABEL(label_progress), TRUE);
  gtk_box_pack_start (GTK_BOX(subTextBox), label_progress, FALSE, FALSE, 4);
  tmp->cDashProgress = label_progress;
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
   /* fill rsc bow with ressources used by the task */
 
  gtk_box_pack_start (GTK_BOX(participBox), subparticipBox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(subparticipBox), rscBox, FALSE, FALSE, 0);
  /* tasks used by this task */

  gtk_widget_show_all (GTK_WIDGET(cadre)); 

  /* drag n drop - thanks to gnome blog ! */
  gtk_drag_source_set (handle, GDK_BUTTON1_MASK, entries, 1, GDK_ACTION_MOVE);
  g_signal_connect (handle, "drag-begin", G_CALLBACK (drag_begin), g_strdup_printf ("%d", id));
  g_signal_connect (handle, "drag-data-get", G_CALLBACK (drag_data_get), g_strdup_printf ("%d", id));
  /* we store the "status" parameter in order to "guess" the reception ListBox */
  gtk_drag_dest_set (row, GTK_DEST_DEFAULT_ALL, entries, 1, GDK_ACTION_MOVE);
  g_signal_connect (row, "drag-data-received", G_CALLBACK (drag_data_received), data);

  return row;
}

/***********************************
  PUBLIC :
  add an element to the relevant
  listbox ; the switch is the
  "status" field of the task data
  position = # of data in GList
***********************************/
void dashboard_add_task (gint position, APP_data *data)
{
  gint tsk_list_len = g_list_length (data->tasksList);
  GList *l;
  gchar *startDate = NULL, *endDate = NULL, *human_date;
  GDate date_start, date_end;
  GtkWidget *box, *row, *labelWait, *labelRun, *labelDelay, *labelDone;
  gchar *tmpStr;

  /* get pointers on labels */
  labelWait = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelWait"));
  labelRun = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelRun"));
  labelDelay = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelDelay"));
  labelDone = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelDone"));

  if((position< tsk_list_len) && (position >=0)) {
     l = g_list_nth (data->tasksList, position);
     tasks_data *tmp_tasks_datas;
     tmp_tasks_datas = (tasks_data *)l->data;
     /* now, we switch according to 'status' field */
     g_date_set_dmy (&date_start, 
                  tmp_tasks_datas->start_nthDay, tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
     g_date_set_dmy (&date_end, 
                  tmp_tasks_datas->end_nthDay, tmp_tasks_datas->end_nthMonth, tmp_tasks_datas->end_nthYear);
             
     startDate = misc_convert_date_to_locale_str (&date_start);
     endDate = misc_convert_date_to_locale_str (&date_end);
     human_date = misc_convert_date_to_friendly_str (&date_start);
     /* now, we switch according to 'status' field */
     switch(tmp_tasks_datas->status) {
        case TASKS_STATUS_RUN:{
             box = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksRun"));
             tmpStr = g_strdup_printf (_("from %s to %s"), human_date, endDate);
             row = dashboard_new_widget (tmp_tasks_datas->name, tmpStr, 
                            tmp_tasks_datas->progress, tmp_tasks_datas->priority, tmp_tasks_datas->category, 
                            tmp_tasks_datas->color, tmp_tasks_datas->status, tmp_tasks_datas->id, tmp_tasks_datas, data);
             g_free (tmpStr);
             gtk_list_box_insert (GTK_LIST_BOX(box), row, -1); 
             gtk_widget_show (row);
             tmp_tasks_datas->cDashboard = row;  
             countRun++;        
           break;
        }
        case TASKS_STATUS_DONE:{
             box  = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksDone"));
             tmpStr = g_strdup_printf (_("from %s to %s"), human_date, endDate);
             row = dashboard_new_widget (tmp_tasks_datas->name, tmpStr, 
                            tmp_tasks_datas->progress, tmp_tasks_datas->priority, tmp_tasks_datas->category, 
                            tmp_tasks_datas->color, tmp_tasks_datas->status, tmp_tasks_datas->id, tmp_tasks_datas, data);
             g_free (tmpStr);
             gtk_list_box_insert (GTK_LIST_BOX(box), row, -1); 
             gtk_widget_show (row);
             tmp_tasks_datas->cDashboard = row;    
             countDone++;
           break;
        }
        case TASKS_STATUS_DELAY:{
             box  = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksDelay"));
             tmpStr = g_strdup_printf (_("from %s to %s"), human_date, endDate);
             row = dashboard_new_widget (tmp_tasks_datas->name, tmpStr, 
                            tmp_tasks_datas->progress, tmp_tasks_datas->priority, tmp_tasks_datas->category,
                            tmp_tasks_datas->color, tmp_tasks_datas->status, tmp_tasks_datas->id, tmp_tasks_datas, data); 
             g_free (tmpStr);
             gtk_list_box_insert (GTK_LIST_BOX(box), row, -1); 
             gtk_widget_show (row);
             tmp_tasks_datas->cDashboard = row;    
             countDelay++;
           break;
        }
        case TASKS_STATUS_TO_GO:{
             box  = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksWait"));
             tmpStr = g_strdup_printf (_("from %s to %s"), human_date, endDate);
             row = dashboard_new_widget (tmp_tasks_datas->name, tmpStr, 
                            tmp_tasks_datas->progress, tmp_tasks_datas->priority, tmp_tasks_datas->category, 
                            tmp_tasks_datas->color, tmp_tasks_datas->status, tmp_tasks_datas->id, tmp_tasks_datas, data);
             g_free (tmpStr);
             gtk_list_box_insert (GTK_LIST_BOX(box), row, -1); 
             gtk_widget_show (row);
             tmp_tasks_datas->cDashboard = row;   
             countWait++; 
           break;
        }
        default: printf ("* PlanLibre critical : unknown task status ! *\n");
     }/* end switch */
     g_free (human_date);
     g_free (endDate);
     g_free (startDate);
  }/* endif */
 /* we change labels */
  tmpStr = g_strdup_printf ("%d", countWait);
  gtk_label_set_text (GTK_LABEL(labelWait), tmpStr);
  g_free (tmpStr);

  tmpStr = g_strdup_printf ("%d", countRun);
  gtk_label_set_text (GTK_LABEL(labelRun), tmpStr);
  g_free (tmpStr);

  tmpStr = g_strdup_printf ("%d", countDelay);
  gtk_label_set_text (GTK_LABEL(labelDelay), tmpStr);
  g_free (tmpStr);

  tmpStr = g_strdup_printf ("%d", countDone);
  gtk_label_set_text (GTK_LABEL(labelDone), tmpStr);
  g_free (tmpStr);
  misc_display_app_status (TRUE, data);
}

/**********************************
  local : set-up a "foo"
  row in order to allow DnD 
  for empty listbox
 0, 1, 2, 3 : nth of columns
**********************************/
static  GtkWidget *dashboard_new_foo_row (gint status, APP_data *data)
{
  GtkWidget *box, *row, *label, *handle;

  /* we must add empty objects to unlock DnD for other objects */
  row = gtk_list_box_row_new ();
  /* event box is mandatory */
  handle = gtk_event_box_new ();
  g_object_set (row, "margin", 1, NULL);
  gtk_container_add (GTK_CONTAINER (row), handle);
  gtk_widget_show (handle);
  /* set unique name to remove element */

  if(status==0) {
      gtk_widget_set_name (handle, "pcol_wait");
      pCol_wait = handle;
  }
  if(status==1) {
      gtk_widget_set_name (handle, "pcol_run");
      pCol_run = handle;
  }
  if(status==2) {
      gtk_widget_set_name (handle, "pcol_delay");
      pCol_delay = handle;
  }
  if(status==3) {
      gtk_widget_set_name (handle, "pcol_done");
      pCol_done = handle;
  }

  label = gtk_label_new ("<big><b>+</b></big>");
  gtk_label_set_use_markup (GTK_LABEL(label), TRUE);
  g_object_set (label, "margin-top", 0, NULL);
  g_object_set (label, "margin-bottom", 0, NULL);
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (handle), label);
  /* drag n drop - thanks to gnome blog ! */
  gtk_drag_source_set (handle, GDK_BUTTON1_MASK, entries, 1, GDK_ACTION_MOVE);
  g_signal_connect (handle, "drag-begin", G_CALLBACK (drag_begin), "-1");
  g_signal_connect (handle, "drag-data-get", G_CALLBACK (drag_data_get), "-1");
  gtk_drag_dest_set (row, GTK_DEST_DEFAULT_ALL, entries, 1, GDK_ACTION_MOVE);
  g_signal_connect (row, "drag-data-received", G_CALLBACK (drag_data_received), data);

  return row;

}


/**********************************************
 * 
 * PUBLIC : update columnus title
 * with content of data->properties structure
 * 
 * ********************************************/
void dashboard_update_column_labels (APP_data *data)
{
  GtkWidget *labelDashCol0, *labelDashCol1, *labelDashCol2, *labelDashCol3;
  
  /* get pointers on columns names */
  labelDashCol0 = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelDashCol0"));
  labelDashCol1 = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelDashCol1"));  
  labelDashCol2 = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelDashCol2"));
  labelDashCol3 = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelDashCol3"));  
  
  /* set default names  */
  gtk_label_set_markup (GTK_LABEL (labelDashCol0), g_strdup_printf ("<big><b>%s</b></big>", data->properties.labelDashCol0));
  gtk_label_set_markup (GTK_LABEL (labelDashCol1), g_strdup_printf ("<big><b>%s</b></big>", data->properties.labelDashCol1));
  gtk_label_set_markup (GTK_LABEL (labelDashCol2), g_strdup_printf ("<big><b>%s</b></big>", data->properties.labelDashCol2)); 
  gtk_label_set_markup (GTK_LABEL (labelDashCol3), g_strdup_printf ("<big><b>%s</b></big>", data->properties.labelDashCol3));  
}

/***********************************
  PUBLIC :
  init (first display) the 
  relevant  listbox ; the switch is the
  "status" field of the task data
  position = # of data in GList
***********************************/
void dashboard_init (APP_data *data)
{
  gint tsk_list_len = g_list_length (data->tasksList);
  gint i;
  GList *l;
  GtkWidget *box, *row, *label_foo, *handle;
  GtkWidget *labelWait, *labelRun, *labelDelay, *labelDone;
  gchar *startDate = NULL, *endDate = NULL, *human_date;
  GDate date_start, date_end;
  tasks_data *tmp_tasks_datas;  
 
  /* set columns titles */
  dashboard_update_column_labels (data);
    
  /* get pointers on labels */
  labelWait = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelWait"));
  labelRun = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelRun"));
  labelDelay = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelDelay"));
  labelDone = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelDone"));

  countWait = 0, countRun = 0, countDelay = 0, countDone = 0;

  row = dashboard_new_foo_row (0, data);/* first column */
  gtk_widget_show (row);  
  box = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksWait"));
  gtk_widget_set_name (GTK_WIDGET(box), "col_wait" );
  gtk_list_box_insert (GTK_LIST_BOX(box), row, -1); 
                   gtk_widget_show (row);

  row = dashboard_new_foo_row (1, data);/* 2nd column */
  gtk_widget_show (row);
  box = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksRun"));
  gtk_widget_set_name (GTK_WIDGET(box), "col_run" );
  gtk_list_box_insert (GTK_LIST_BOX(box), row, -1); 
                   
  row = dashboard_new_foo_row (2, data);
  gtk_widget_show (row);
  box = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksDelay"));
  gtk_widget_set_name (GTK_WIDGET(box), "col_delay" );
  gtk_list_box_insert (GTK_LIST_BOX(box), row, -1); 
  
  row = dashboard_new_foo_row (3, data);
  gtk_widget_show (row);
  box = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksDone"));
  gtk_widget_set_name (GTK_WIDGET(box), "col_done" );
  gtk_list_box_insert (GTK_LIST_BOX(box), row, -1); 

  /****************************/

  for(i=0; i<tsk_list_len; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;// TODO : fix a status for group tasks 
     if(tmp_tasks_datas->type == TASKS_TYPE_TASK ){
	     /* now, we switch according to 'status' field */
         g_date_set_dmy (&date_start, 
                  tmp_tasks_datas->start_nthDay, tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
         g_date_set_dmy (&date_end, 
                  tmp_tasks_datas->end_nthDay, tmp_tasks_datas->end_nthMonth, tmp_tasks_datas->end_nthYear);
             
         startDate = misc_convert_date_to_locale_str (&date_start);
         endDate = misc_convert_date_to_locale_str (&date_end);
         human_date = misc_convert_date_to_friendly_str (&date_start);
	    switch(tmp_tasks_datas->status) {
		case TASKS_STATUS_RUN:{
                   box  = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksRun"));
                   row = dashboard_new_widget (tmp_tasks_datas->name, g_strdup_printf (_("from %s to %s"), human_date, endDate), 
                          tmp_tasks_datas->progress, tmp_tasks_datas->priority, tmp_tasks_datas->category, 
                          tmp_tasks_datas->color, tmp_tasks_datas->status, tmp_tasks_datas->id, tmp_tasks_datas, data);
                   gtk_list_box_insert (GTK_LIST_BOX(box), row, -1); 
                   gtk_widget_show (row);
                   tmp_tasks_datas->cDashboard = row;
                   countRun++;
		   break;
		}
		case TASKS_STATUS_DONE:{
                   box  = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksDone"));
                   row = dashboard_new_widget(tmp_tasks_datas->name, g_strdup_printf (_("from %s to %s"), human_date, endDate), 
                            tmp_tasks_datas->progress, tmp_tasks_datas->priority, tmp_tasks_datas->category, 
                            tmp_tasks_datas->color, tmp_tasks_datas->status, tmp_tasks_datas->id, tmp_tasks_datas, data);
                   gtk_widget_show (row);
                   tmp_tasks_datas->cDashboard = row;
                   gtk_list_box_insert (GTK_LIST_BOX(box), row, -1); 
                   countDone++;
		   break;
		}
		case TASKS_STATUS_DELAY:{
                   box  = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksDelay"));
                   row = dashboard_new_widget(tmp_tasks_datas->name, g_strdup_printf (_("from %s to %s"), human_date, endDate), 
                            tmp_tasks_datas->progress, tmp_tasks_datas->priority, tmp_tasks_datas->category, 
                            tmp_tasks_datas->color, tmp_tasks_datas->status, tmp_tasks_datas->id, tmp_tasks_datas, data);
                   gtk_widget_show (row);
                   tmp_tasks_datas->cDashboard = row;
                   gtk_list_box_insert (GTK_LIST_BOX(box), row, -1);
                   countDelay++;
		   break;
		}
		case TASKS_STATUS_TO_GO:{
                   box  = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksWait"));
                   row = dashboard_new_widget(tmp_tasks_datas->name, g_strdup_printf (_("from %s to %s"), human_date, endDate), 
                            tmp_tasks_datas->progress, tmp_tasks_datas->priority, tmp_tasks_datas->category, 
                            tmp_tasks_datas->color, tmp_tasks_datas->status, tmp_tasks_datas->id, tmp_tasks_datas, data);
                   gtk_widget_show (row);
                   tmp_tasks_datas->cDashboard = row;
                   gtk_list_box_insert (GTK_LIST_BOX(box), row, -1);
                   countWait++;
		   break;
		}
		default: {
                     printf ("* PlanLibre critical : unknown task status ! *\n");
                }
	     }/* end switch */
        g_free (startDate);
        g_free (endDate);
        g_free (human_date);
    }/* endif task */
  }/* next */
  /* we change labels */
  gtk_label_set_text (GTK_LABEL(labelWait), g_strdup_printf ("%d", countWait));
  gtk_label_set_text (GTK_LABEL(labelRun), g_strdup_printf ("%d", countRun));
  gtk_label_set_text (GTK_LABEL(labelDelay), g_strdup_printf ("%d", countDelay));
  gtk_label_set_text (GTK_LABEL(labelDone), g_strdup_printf ("%d", countDone));
}

/***********************************
  PUBLIC :
  remove a single element 
***********************************/
void dashboard_remove_task (gint position, APP_data *data)
{
  GtkWidget *box, *row, *handle;
  gint i;
  gint tsk_list_len = g_list_length (data->tasksList);
  GList *l;
  GtkWidget *labelWait, *labelRun, *labelDelay, *labelDone;
  tasks_data *tmp_tasks_datas;

  /* get pointers on labels */
  labelWait = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelWait"));
  labelRun = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelRun"));
  labelDelay = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelDelay"));
  labelDone = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelDone"));

  if((position >=0) && (position<tsk_list_len)) {
     l = g_list_nth (data->tasksList, position);
     tmp_tasks_datas = (tasks_data *)l->data;// TODO : fix a status for group tasks 

     if(tmp_tasks_datas->type == TASKS_TYPE_TASK){
	     switch(tmp_tasks_datas->status) {
		case TASKS_STATUS_RUN:{
                   box = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksRun"));
                   gtk_container_remove (GTK_CONTAINER(box), tmp_tasks_datas->cDashboard);
                   tmp_tasks_datas->cDashboard = NULL;
                   countRun--;
		   break;
		}
		case TASKS_STATUS_DONE:{
                   box = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksDone"));
                   gtk_container_remove (GTK_CONTAINER(box), tmp_tasks_datas->cDashboard);
                   tmp_tasks_datas->cDashboard = NULL;
                   countDone--;
		   break;
		}
		case TASKS_STATUS_DELAY:{
                   box = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksDelay"));
                   gtk_container_remove (GTK_CONTAINER(box), tmp_tasks_datas->cDashboard);
                   tmp_tasks_datas->cDashboard = NULL;
                   countDelay--;
		   break;
		}
		case TASKS_STATUS_TO_GO:{
                   box = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksWait"));
                   gtk_container_remove (GTK_CONTAINER(box), tmp_tasks_datas->cDashboard);
                   tmp_tasks_datas->cDashboard = NULL;
                   countWait--;
		   break;
		}
		default: printf ("* PlanLibre critical : unknown task status ! *\n");
	     }/* end switch */
      }/* endif */
  }/* endif */
 /* we change labels */
  gtk_label_set_text (GTK_LABEL(labelWait), g_strdup_printf ("%d", countWait));
  gtk_label_set_text (GTK_LABEL(labelRun), g_strdup_printf ("%d", countRun));
  gtk_label_set_text (GTK_LABEL(labelDelay), g_strdup_printf ("%d", countDelay));
  gtk_label_set_text (GTK_LABEL(labelDone), g_strdup_printf ("%d", countDone));
  misc_display_app_status (TRUE, data);
}

/***********************************
  PUBLIC :
  clear the current dashboard
  fReset : set to TRUE before call 
   to dashboard_init ()
***********************************/
void dashboard_remove_all (gboolean fReset, APP_data *data)
{
  GtkWidget *box;
  gint i;
  gint tsk_list_len = g_list_length (data->tasksList);
  GList *l;
  tasks_data *tmp_tasks_datas;

  for(i=0; i<tsk_list_len; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;// TODO : fix a status for group tasks 
     if(tmp_tasks_datas->type == TASKS_TYPE_TASK ){
	     switch(tmp_tasks_datas->status) {
		case TASKS_STATUS_RUN:{
                   box  = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksRun"));
                   gtk_container_remove (GTK_CONTAINER(box ), tmp_tasks_datas->cDashboard );
                   tmp_tasks_datas->cDashboard = NULL;
		   break;
		}
		case TASKS_STATUS_DONE:{
                   box  = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksDone"));
                   gtk_container_remove (GTK_CONTAINER(box ), tmp_tasks_datas->cDashboard );
                   tmp_tasks_datas->cDashboard = NULL;
		   break;
		}
		case TASKS_STATUS_DELAY:{
                   box  = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksDelay"));
                   gtk_container_remove (GTK_CONTAINER(box ), tmp_tasks_datas->cDashboard );
                   tmp_tasks_datas->cDashboard = NULL;
		   break;
		}
		case TASKS_STATUS_TO_GO:{
                   box  = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksWait"));
                   gtk_container_remove (GTK_CONTAINER(box ), tmp_tasks_datas->cDashboard );
                   tmp_tasks_datas->cDashboard = NULL;
		   break;
		}
		default: printf ("* PlanLibre critical : unknown task status ! *\n");
	     }/* end switch */
      }/* endif */
  }/* next */
  /* and we remove the four [+] remaining widgets */

  if(fReset) {
	  box  = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksRun"));
	  if(pCol_run!=NULL)
	     gtk_container_remove (GTK_CONTAINER(box), gtk_widget_get_parent(pCol_run) );


	  box  = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksWait"));
	  if(pCol_wait!=NULL)
	     gtk_container_remove (GTK_CONTAINER(box), gtk_widget_get_parent(pCol_wait) );


	  box  = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksDelay"));
	  if(pCol_delay!=NULL)
	     gtk_container_remove (GTK_CONTAINER(box), gtk_widget_get_parent(pCol_delay) );


	  box  = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasksDone"));
	  if(pCol_done!=NULL)
	     gtk_container_remove (GTK_CONTAINER(box), gtk_widget_get_parent(pCol_done) );
  }

  misc_display_app_status (TRUE, data);  
}

