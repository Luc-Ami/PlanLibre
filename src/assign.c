/**********************************
  assign.c module
  functions, constants for
  displaying ressources on
  a virtual week grid

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
#include <goocanvas.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <cairo-svg.h>
#include <cairo-pdf.h>


#include "support.h"
#include "misc.h"
#include "links.h"
#include "tasks.h"
#include "assign.h"
#include "ressources.h"
#include "report.h"
#include "files.h"

/* =====================
  protected variables 
======================*/
static gint assignDay, assignMonth, assignYear, maxTaskByRsc, currentRscId;
static gint calDay, calMonth, calYear;/* used to store top left value of current calendar, as displayed */
static gdouble canvas_max_x = 0, canvas_max_y = 0;


/************************************************
 * PROTECTED : retrive overload flag for
 * resource 'ID'
 * ********************************************/
static gboolean assign_get_overload_flag_for_id (gint id, APP_data *data)
{
  gboolean ret = FALSE;
  gint i = 0, rc = -1;
  GList *l = NULL;
  rsc_datas *tmp_rsc;	

  while( (i < g_list_length (data->rscList)) && (rc < 0)) {
	  l = g_list_nth (data->rscList, i);
	  tmp_rsc = (rsc_datas*) l->data;
	  if(tmp_rsc->id == id) {
		  ret = tmp_rsc->fOverload;
		  rc = 1;
	  }
	  i++;
  }	
  return ret;	
}

/********************************
  LOCAL : compute correction
  for each tasks height
********************************/
static gdouble assign_compute_correction (APP_data *data)
{
   if(maxTaskByRsc > 10)
     return (gdouble) maxTaskByRsc/10;
   else
     return 1;
}

/************************
  LOCAL : get extremas
************************/
static gdouble assign_get_max_x (void)
{
  gdouble tmp = canvas_max_x + (7*ASSIGN_CALENDAR_DAY_WIDTH);
  if(tmp>ASSIGN_VIEW_MAX_WIDTH) {
    tmp = ASSIGN_VIEW_MAX_WIDTH;
  }
  return tmp;
}

static gdouble assign_get_max_y (void)
{
  gdouble tmp = canvas_max_y + ASSIGN_AVATAR_HEIGHT;
  if(tmp>ASSIGN_VIEW_MAX_HEIGHT) {
     tmp = ASSIGN_VIEW_MAX_HEIGHT;
  }
  return tmp;
}
/*****************************************
 LOCAL : callbacks for righ-click

*****************************************/

/*************************************
  CB : edit/modify ressource
**************************************/
static void on_menuAssignEdit (GtkMenuItem *menuitem, APP_data *data)
{
  rsc_modify (data);
}

/*************************************
  CB : compute charges for ressource
**************************************/
static void on_menuAssignCharge (GtkMenuItem *menuitem, APP_data *data)
{
  report_unique_rsc_charge (currentRscId, data);
}

/*************************************
  CB : compute charges for ressource
**************************************/
static void on_menuAssignCost (GtkMenuItem *menuitem, APP_data *data)
{
  report_unique_rsc_cost (currentRscId, data);
}

/************************************************
 * PROTECTED : call a report about overload
 * for current resource
 * ********************************************/

static void on_menuAssignOverload (GtkMenuItem *menuitem, APP_data *data)
{
  /* we jnow ID for selected row : currentRscId */	 
  report_unique_rsc_overload (currentRscId, data);
}




/****************************************** 
 LOCAL
 PopUp menu after a right-click. 
 * we have a static glo-cal variable for
 * know the ID of selected resource :
 * currentRscId
 ******************************************/
static GtkWidget *create_menu_assign (GtkWidget *win, APP_data *data)
{
  GtkWidget *menu1Assign, *menuAssignEdit, *separator1;
  GtkWidget *menuCancelAssign, *menuAssignCharge, *menuAssignCost, *menuAssignOverload;
  gboolean flag;

  menu1Assign = gtk_menu_new (); 

  menuAssignEdit = gtk_menu_item_new_with_mnemonic (_("_Modify resource ... "));
  gtk_widget_show (menuAssignEdit);
  gtk_container_add (GTK_CONTAINER (menu1Assign), menuAssignEdit);

  menuAssignCharge = gtk_menu_item_new_with_mnemonic (_("_Resource charge ... "));
  gtk_widget_show (menuAssignCharge);
  gtk_container_add (GTK_CONTAINER (menu1Assign), menuAssignCharge);

  menuAssignCost = gtk_menu_item_new_with_mnemonic (_("_Resource cost ... "));
  gtk_widget_show (menuAssignCost);
  gtk_container_add (GTK_CONTAINER (menu1Assign), menuAssignCost);

  menuAssignOverload = gtk_menu_item_new_with_mnemonic (_("Overload details ... "));
  gtk_widget_show (menuAssignOverload);
  gtk_container_add (GTK_CONTAINER (menu1Assign), menuAssignOverload);
  /* we retrive overload flag for current resource */
  flag = assign_get_overload_flag_for_id (currentRscId, data);
  gtk_widget_set_sensitive (menuAssignOverload, flag);

  separator1 = gtk_separator_menu_item_new ();
  gtk_widget_show (separator1);
  gtk_container_add (GTK_CONTAINER (menu1Assign), separator1);
  gtk_widget_set_sensitive (separator1, FALSE);

  menuCancelAssign = gtk_menu_item_new_with_mnemonic (_("C_ancel"));
  gtk_widget_show (menuCancelAssign);
  gtk_container_add (GTK_CONTAINER (menu1Assign), menuCancelAssign);

  /* callbacks */

  g_signal_connect ((gpointer) menuAssignEdit, "activate",
                    G_CALLBACK (on_menuAssignEdit),
                    data);

  g_signal_connect ((gpointer) menuAssignCharge, "activate",
                    G_CALLBACK (on_menuAssignCharge),
                    data);

  g_signal_connect ((gpointer) menuAssignCost, "activate",
                    G_CALLBACK (on_menuAssignCost),
                    data);
                    
  g_signal_connect ((gpointer) menuAssignOverload, "activate",
                    G_CALLBACK (on_menuAssignOverload),
                    data);                    
                    

  return menu1Assign;
}
/******************************************

  PUBLIC : store month value

*******************************************/
void assign_store_date (gint dd, gint mm, gint yy)
{
  assignDay = dd;
  assignMonth = mm;
  assignYear = yy;
}

/*****************************************
 PUBLIC : callback for button
 prev month
*****************************************/
void on_BtnPrevMonth_clicked (GtkButton *button, APP_data *data)
{
  gint month, year;
  gdouble hvalue, vvalue;
  GDate *date;

  date = g_date_new_dmy (1, assignMonth, assignYear);
  g_date_subtract_months (date, 1);
  month = g_date_get_month (date);
  year = g_date_get_year (date);
  assign_store_date (1, month, year);

  assign_remove_all_tasks (data);
  GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment 
                                 (GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowAsst")));
  GtkAdjustment *hadj = gtk_scrolled_window_get_hadjustment 
                                 (GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowAsst")));
  gtk_adjustment_set_value (GTK_ADJUSTMENT(hadj), 0);
  vvalue = gtk_adjustment_get_value (GTK_ADJUSTMENT(vadj));
  hvalue = gtk_adjustment_get_value (GTK_ADJUSTMENT(hadj));
  assign_draw_calendar_ruler (vvalue, -1, month, year, data);
  assign_draw_rsc_ruler (hvalue, data);
  assign_draw_all_tasks (data);
}

/*****************************************
 PUBLIC : callback for button
 next month
*****************************************/
void on_BtnNextMonth_clicked (GtkButton *button, APP_data *data)
{
  gint month, year;
  gdouble hvalue, vvalue;
  GDate *date;

  date = g_date_new_dmy (1, assignMonth, assignYear);
  g_date_add_months (date, 1);
  month = g_date_get_month (date);
  year = g_date_get_year (date);
  assign_store_date (1, month, year);

  assign_remove_all_tasks (data);
  GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment 
                                 (GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowAsst")));
  GtkAdjustment *hadj = gtk_scrolled_window_get_hadjustment 
                                 (GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowAsst")));
  gtk_adjustment_set_value (GTK_ADJUSTMENT(hadj), 0);
  vvalue = gtk_adjustment_get_value (GTK_ADJUSTMENT(vadj));
  hvalue = gtk_adjustment_get_value (GTK_ADJUSTMENT(hadj));

  assign_draw_calendar_ruler (vvalue, -1, month, year, data);
  assign_draw_rsc_ruler (hvalue, data);
  assign_draw_all_tasks (data);
}

/*****************************************
  LOCAL :
 now we compute the item's index 
 ****************************************/
static gint assign_get_item_index (gint id, APP_data *data)
{
   gint i, ret = -1;
   GList *l;
   rsc_datas *tmp_rsc_datas;

   i = 0;
   while ((i<g_list_length (data->rscList)) && (ret<0)) {
      l = g_list_nth (data->rscList, i);
      tmp_rsc_datas = (rsc_datas *)l->data;  
      if(id == tmp_rsc_datas->id) {
         ret = i;
      }
      i++;
   }/* wend */
  return ret;
}

/****************************************** 
 LOCAL
 This handles button presses in item views. 
 ******************************************/
static gboolean on_assign_button_press (GooCanvasItem  *item,
                      GooCanvasItem  *target,
                      GdkEventButton *event,
                      APP_data        *data)
{
   gchar *tmpStr;
   gint i, ret = -1, id = -1;
   gboolean isTask = TRUE;
   GList *l;
   GtkListBox *box;
   GtkListBoxRow *row;

 
   g_object_get (item, "title", &tmpStr, NULL );/* we get a string representation of unique IDentifier */
   if(tmpStr != NULL) {
      id = atoi (tmpStr);
      g_free (tmpStr);
      currentRscId = id;
   }

   /* now we compute the item's index */
   ret = assign_get_item_index (id, data);
   printf("l'index de l'événement est =%d correspond à l'item %d \n", ret, id);
   /* we select according row */
   box = GTK_LIST_BOX(GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxRsc"))); 
   /* TODO : unselect */
   row = gtk_list_box_get_row_at_index (GTK_LIST_BOX(box), ret);
   gtk_list_box_select_row (GTK_LIST_BOX(box), row);

   if(event->type == GDK_2BUTTON_PRESS && ret >= 0) {
      /* now we can call modify task or modify group */
      rsc_modify (data);
   }
   else {
      if(event->button==3 && ret>=0) {/* right-click */
	     GtkMenu *menu;
	     menu = GTK_MENU(create_menu_assign (data->appWindow, data));
	     gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
      }
   }
  return TRUE;
}

/******************************************
 PROTECTED : draw an element for 
  ressource's tasks
******************************************/
static void 
    assign_draw_bar (gint nbDays, gint duration, gint index, gint total, gchar *sRgba, GdkRGBA color, 
                      gchar *tmpStr, GooCanvasItem *group_task, APP_data *data)
{
  gdouble bar_height = (assign_compute_correction (data)*ASSIGN_AVATAR_HEIGHT)/(maxTaskByRsc+1);
  gdouble avatar_height = assign_compute_correction (data)*ASSIGN_AVATAR_HEIGHT;
  const gchar *light_color = "white";
  const gchar *dark_color = "black";
  gchar *Str;
  
  GooCanvasItem *rect_item = 
            goo_canvas_rect_new (group_task, ASSIGN_CALENDAR_RSC_WIDTH+(nbDays*ASSIGN_CALENDAR_DAY_WIDTH), 
		                 ASSIGN_CALENDAR_HEADER_HEIGHT+(avatar_height*index)+(gint) (bar_height*total ), 
		                                duration*ASSIGN_CALENDAR_DAY_WIDTH, bar_height,
		                           "line-width", 1.0, "radius-x", 2.0, "radius-y", 2.0,
		                           "stroke-color", "#D9DADD", "fill-color", sRgba,
		                           NULL); 

  /* extrema */
  if(ASSIGN_CALENDAR_RSC_WIDTH+(nbDays*ASSIGN_CALENDAR_DAY_WIDTH)+duration*ASSIGN_CALENDAR_DAY_WIDTH >canvas_max_x)
      canvas_max_x = ASSIGN_CALENDAR_RSC_WIDTH + (nbDays*ASSIGN_CALENDAR_DAY_WIDTH) + duration*ASSIGN_CALENDAR_DAY_WIDTH;
  if(ASSIGN_CALENDAR_HEADER_HEIGHT+(avatar_height*index)+(gint) (bar_height*total )+bar_height >canvas_max_y)
      canvas_max_y = ASSIGN_CALENDAR_HEADER_HEIGHT + (avatar_height*index) + (gint) (bar_height*total ) + bar_height;
/*
  GooCanvasItem *small_rect = goo_canvas_rect_new (group_task, ASSIGN_CALENDAR_RSC_WIDTH+(nbDays*ASSIGN_CALENDAR_DAY_WIDTH)+6 ,
		                                ASSIGN_CALENDAR_HEADER_HEIGHT+(ASSIGN_AVATAR_HEIGHT*index)+12, 
		                                16, ASSIGN_DEMI_HEIGHT-18,
		                           "line-width", 1.0,
		                           "radius-x", 3.0,
		                           "radius-y", 1.0,
		                           "stroke-color", sRgba,
		                           "fill-color", sRgba,	
 	                           NULL);*/
  Str = light_color;
  if((color.red + color.green+color.blue) > 0.7) {
    Str = dark_color;  
  }
  
  GooCanvasItem *task_text = goo_canvas_text_new (group_task, (const gchar*) tmpStr, 
		                           ASSIGN_CALENDAR_RSC_WIDTH + (nbDays*ASSIGN_CALENDAR_DAY_WIDTH) + 2,
		                           ASSIGN_CALENDAR_HEADER_HEIGHT + (avatar_height * index) + (gint) (bar_height * total ) + 2,
		                           -1,
		                           GOO_CANVAS_ANCHOR_NW,
		                           "font", "Sans 9", "fill-color", Str, 
		                           NULL); 
}

/******************************************
  PUBLIC : redraw calender ruler
  when a scroll event fires
******************************************/
void
on_assign_vadj_changed (GtkAdjustment *adjustment, APP_data *data)
{
  if(data->rulerAssign != NULL )
      g_object_set (data->rulerAssign, "y", gtk_adjustment_get_value (adjustment), NULL);

}

void
on_assign_hadj_changed (GtkAdjustment *adjustment, APP_data *data)
{
  if(data->rscAssign != NULL) {
      g_object_set (data->rscAssign, "x", gtk_adjustment_get_value (adjustment), NULL);  
      goo_canvas_item_raise (data->rscAssign, NULL);
  }
}

 
 
/********************************************
 * protected : test overload for a resource
 * arguments :
 * data_app structure
 * id of resource to test
 * output : TRUE if overload
 * *****************************************/
 
static gboolean assign_test_rsc_overload (gint id, APP_data *data)
 {
	 gboolean ret = FALSE;
	 gboolean coincStart, coincEnd;
	 gint i = 0, rc = -1, link, j;
	 gint cmpst1, cmpst2, cmped1, cmped2;
	 GList *l = NULL;
	 GList *l2 = NULL;	 
     tasks_data *tmp_tsk_datas;
     rsc_datas *tmp_rsc;
     assign_data *tmp_assign_datas, *tmp_assign2;
     GDate date_task_start, date_task_end, start, end;

     /* we do a loop to find every tasks where resource 'id' is used and we build a GList*/
     for( i = 0; i < g_list_length (data->tasksList); i++) {
	    l = g_list_nth (data->tasksList, i);
        tmp_tsk_datas = (tasks_data *)l->data;
        /* now we check if the resource 'id' is used by current task ; if Yes, we upgrade Glist */
        link = links_check_element (tmp_tsk_datas->id, id, data);
        if(link >= 0) {
		 //  printf ("trouvé un lien entre tâche %s et ressource %d \n", tmp_tsk_datas->name, id);
		   tmp_assign_datas = g_malloc (sizeof(assign_data));
		   tmp_assign_datas->task_id = tmp_tsk_datas->id;
		   tmp_assign_datas->s_day = tmp_tsk_datas->start_nthDay;
		   tmp_assign_datas->s_month = tmp_tsk_datas->start_nthMonth;
		   tmp_assign_datas->s_year = tmp_tsk_datas->start_nthYear;		   

		   tmp_assign_datas->e_day = tmp_tsk_datas->end_nthDay;
		   tmp_assign_datas->e_month = tmp_tsk_datas->end_nthMonth;
		   tmp_assign_datas->e_year = tmp_tsk_datas->end_nthYear;
		   		   
		   l2 = g_list_append(l2, tmp_assign_datas);	
		}	
     }/* next i */
     
     /* now, if list l2 has zero or one element, we haven't any overloading */
     if(g_list_length (l2)<2) {
		 printf ("liste vide ou égale 1 pas de surcharge \n");
	 }
	 else {
	     printf ("liste >1 donc surcharge \n");
	     /* now we do loop on list in order to test oveloads */
	     printf ("voici la liste des tâches utilisant la ressource %d\n", id);
	     for( i = 0; i < g_list_length (l2); i++) {
		    l = g_list_nth (l2, i);
            tmp_assign_datas = (assign_data *)l->data;
          //  printf ("i=%d tsk=%d \n", i, tmp_assign_datas->task_id);
            /* now we do a while/wend loop in order to check an oveleao */
            /* it's a decreasing loop, because we are indexed on 'i'    */
            /* we use date at rank i as reference                       */
            g_date_set_dmy (&start, tmp_assign_datas->s_day, tmp_assign_datas->s_month, tmp_assign_datas->s_year);
            g_date_set_dmy (&end, tmp_assign_datas->e_day, tmp_assign_datas->e_month, tmp_assign_datas->e_year);
            /* now we will compare to any date at rank AFTER i */
            j = i+1;
            
            while( j < g_list_length (l2)) {
			   l = g_list_nth (l2, j);
               tmp_assign2 = (assign_data *)l->data;			   
			   /* we get dates for current tasks */
			   g_date_set_dmy (&date_task_start, tmp_assign2->s_day, tmp_assign2->s_month, tmp_assign2->s_year);
			   g_date_set_dmy (&date_task_end, tmp_assign2->e_day, tmp_assign2->e_month, tmp_assign2->e_year);
			   /* now we test overlaping with start and end dates */
			   /*
							   #########################  tsk
								   [start rsc to test]
			   */
		       cmpst1 =  g_date_compare (&start, &date_task_start); /* if <0 NOT concurrents */
		       cmpst2 =  g_date_compare (&start, &date_task_end); /* if >0 NOT concurrents */
		       coincStart = FALSE;
			   if((cmpst1 >= 0) && (cmpst2 <= 0)) {
				 coincStart = TRUE;
			   }
			   /*
							   #########################  tsk
								   [end rsc to test]
			   */
			   cmped1 =  g_date_compare (&end, &date_task_start); /* if <0 NOT concurrents */
			   cmped2 =  g_date_compare (&end, &date_task_end); /* if >0 NOT concurrents */
			   coincEnd = FALSE;
			   if((cmped1 >= 0) && (cmped2 <= 0)) {
				 coincEnd = TRUE;
			   }
			   if((coincStart) || (coincEnd)) {
				  ret = TRUE;   
			   }
			   j++;	
		    }/* wend */
            
	     }/* nexr i */
	 }
	 /* we store value on flag */
	 if(ret == TRUE) {
		 
	 }	 
	 
     /* we free datas */
     l2 = g_list_first (l2);
     for(l2 ; l2 != NULL; l2 = l2->next) {
        tmp_assign_datas = (assign_data *)l2->data;
        g_free (tmp_assign_datas);
     }/* next l2*/
     
     /* free the Glist itself */
     g_list_free (l2);     
     
     printf ("liste des surchages vidée \n");
     
	 return ret;

 }
 
 
/******************************************
  PUBLIC : draw ressources ruler 
******************************************/
void assign_draw_rsc_ruler (gdouble xpos, APP_data *data)
{
  GooCanvasItem *root, *sep_line, *calendar_header, *sep_days, *rscAssign, *charge, *rscRuler ;
  gint i;
  gdouble height;
  GdkPixbuf *pix2;
  GError *err = NULL;
  GList *l;
  rsc_datas *tmp_rsc_datas;
  gchar *tmpStr = NULL, buffer[50];/* for utf/8 : 4*maxlen */
  const gchar *grey_day = "#3E3E3E";
  const gchar *blue_day = "#1DD6F2";
  gboolean fOverload = FALSE;

  root = goo_canvas_get_root_item (GOO_CANVAS(data->canvasAssign));
  /* reset max tasks for all ressources */
  maxTaskByRsc = 0;
  /*  ressources rectangle */
  rscRuler = goo_canvas_group_new (root, NULL);
  data->rscAssign = rscRuler;
  rscAssign = goo_canvas_rect_new (rscRuler, xpos, 0, 
                                    ASSIGN_CALENDAR_RSC_WIDTH, ASSIGN_VIEW_MAX_HEIGHT,
                                   "line-width", 0.0, "radius-x", 0.0,
                                   "radius-y", 0.0, "stroke-color", "white",
                                   "fill-color", "white", NULL);
  sep_line = goo_canvas_polyline_new_line (rscRuler, xpos+ASSIGN_CALENDAR_RSC_WIDTH, 0, 
                                                     xpos+ASSIGN_CALENDAR_RSC_WIDTH, ASSIGN_VIEW_MAX_HEIGHT,
                                                        "stroke-color", "#D7D7D7",
                                                        "line-width", 1.5, NULL);  
  /* compute absolute max number of tasks for ONe ressource */
  maxTaskByRsc = rsc_get_absolute_max_tasks (data);
  height = assign_compute_correction (data) * ASSIGN_AVATAR_HEIGHT;
  /* draw all ressources names */
  for(i=0; i< g_list_length (data->rscList); i++) {
     l = g_list_nth (data->rscList, i);
     tmp_rsc_datas = (rsc_datas *)l->data;

     GooCanvasItem *rscGroup = goo_canvas_group_new (rscRuler, NULL);

     /* if overload use red pencil */
     fOverload = assign_test_rsc_overload (tmp_rsc_datas->id, data);
     printf ("valeur retour over =%d \n", fOverload);
     /* we store in flag for next usage */
     tmp_rsc_datas->fOverload = fOverload;
     if(fOverload) {
		g_utf8_strncpy (&buffer, tmp_rsc_datas->name, 8); 
        tmpStr = g_strdup_printf ("%s <span background=\"red\" color=\"yellow\"><b> ! </b></span>", buffer);
         GooCanvasItem *sep_names = goo_canvas_text_new (rscGroup, tmpStr, 
                                    xpos+4, ASSIGN_CALENDAR_HEADER_HEIGHT+4+(i*(gint)height), -1,
                                   GOO_CANVAS_ANCHOR_NW,
                                   "font", "Sans 12", "fill-color", "#F00000",
                                   "use-markup", TRUE, 
                                   NULL);
     }
     else {
	    g_utf8_strncpy (&buffer, tmp_rsc_datas->name, 12);	 
        tmpStr = g_strdup_printf ("%s", buffer);
        GooCanvasItem *sep_names = goo_canvas_text_new (rscGroup, tmpStr, 
                                    xpos+4, ASSIGN_CALENDAR_HEADER_HEIGHT+4+(i*(gint)height), -1,
                                   GOO_CANVAS_ANCHOR_NW,
                                   "font", "Sans 12", "fill-color", "#282828", 
                                   NULL); 
     }
 
     if(tmpStr){
        g_free (tmpStr);
        tmpStr = NULL;
     }
     /* avatar */
     if(tmp_rsc_datas->pix) {   
           pix2 = gdk_pixbuf_scale_simple (tmp_rsc_datas->pix, 
                             TASKS_RSC_AVATAR_SIZE+10, TASKS_RSC_AVATAR_SIZE+10, GDK_INTERP_BILINEAR);
     }
     else {
        pix2 = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("def_avatar.png"), 
                                        TASKS_RSC_AVATAR_SIZE+10, TASKS_RSC_AVATAR_SIZE+10, &err);
     }
     /* converts to rounded avatar */
     cairo_surface_t *surf = NULL;
     cairo_t *cr = NULL;
     surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, TASKS_RSC_AVATAR_SIZE+10, TASKS_RSC_AVATAR_SIZE+10);
     /* now we can create the context */
     cr = cairo_create (surf);
     gdk_cairo_set_source_pixbuf (cr, pix2, 0, 0);
     cairo_arc (cr, (TASKS_RSC_AVATAR_SIZE+10)/2, (TASKS_RSC_AVATAR_SIZE+10)/2, (TASKS_RSC_AVATAR_SIZE+10)/2-2, 0, 2*3.1416);
     cairo_clip (cr);
     cairo_paint (cr);
     cairo_destroy (cr);
     GdkPixbuf *pixbuf2 = gdk_pixbuf_get_from_surface (surf, 0, 0,
                                      TASKS_RSC_AVATAR_SIZE+10, TASKS_RSC_AVATAR_SIZE+10);

     cairo_surface_destroy (surf);

     goo_canvas_image_new (rscGroup, pixbuf2, xpos+4, ASSIGN_CALENDAR_HEADER_HEIGHT+24+(i*(gint)height), NULL);
     g_object_unref (pixbuf2);
     g_object_unref (pix2);

     /* we count total tasks for current ressource */
     gint n_tasks = rsc_get_number_tasks (tmp_rsc_datas->id, data);
     //if(n_tasks>maxTaskByRsc)
       // maxTaskByRsc = n_tasks;/* don't remove global variable !!! */

     tmpStr = g_strdup_printf (_("%d/%d task(s)"), rsc_get_number_tasks_done (tmp_rsc_datas->id, data),
                               n_tasks);
     charge = goo_canvas_text_new (rscGroup, tmpStr, 
                                    xpos+12+TASKS_RSC_AVATAR_SIZE+10, ASSIGN_CALENDAR_HEADER_HEIGHT+24+(i*(gint)height), -1,
                                   GOO_CANVAS_ANCHOR_NW,
                                   "font", "Sans 10", "fill-color", "#282828", 
                                   NULL);  
     if(tmpStr) {
        g_free (tmpStr);
        tmpStr = NULL;
     }
     g_object_set (rscGroup, "title", g_strdup_printf ("%d", tmp_rsc_datas->id), NULL);
     /* Connect a signal handler for the rectangle item. */
     g_signal_connect ((gpointer) rscGroup, "button_press_event", G_CALLBACK (on_assign_button_press), data);
     /* thin line */
     sep_line =  goo_canvas_polyline_new_line (rscRuler, ASSIGN_CALENDAR_RSC_WIDTH, 
                                                     (i*(gint)height)+ASSIGN_CALENDAR_HEADER_HEIGHT,
                                                     ASSIGN_VIEW_MAX_WIDTH, (i*(gint)height)+ASSIGN_CALENDAR_HEADER_HEIGHT,
                                                        "stroke-color", "#AFB2F1",
                                                        "line-width", 1.0, NULL);  
  }/* next i */

}
/***************************************************
  PUBLIC : draw calendar ruler
  paramaters : a month and a year
  dd == -1 to "force" using of first day of month,
  or another value to force to true start of week
***************************************************/
void assign_draw_calendar_ruler (gdouble ypos, gint dd, gint mm, gint yy, APP_data *data)
{
  GooCanvasItem *root, *sep_line, *calendar_header, *sep_days, *rscAssign;
  gint i, j, week = 0, month, year = 0;
  gdouble demiWidth, sevenWidth, demiHeight;
  gboolean flag = FALSE;
  gchar *tmpStr = NULL;
  GDate *date;
  const gchar *grey_day = "#3E3E3E";
  const gchar *blue_day = "#AAA4BB";

  root = goo_canvas_get_root_item (GOO_CANVAS (data->canvasAssign));
  /* machine time gains ;-) */
  demiWidth = ASSIGN_CALENDAR_DAY_WIDTH/2;
  sevenWidth = ASSIGN_CALENDAR_DAY_WIDTH*7;
  demiHeight = ASSIGN_AVATAR_WIDTH/2;

  if(dd<0)
     date = g_date_new_dmy (1, mm, yy);
  else
     date = g_date_new_dmy (dd, mm, yy);

  /* we clamp to monday of the week */
  while(g_date_get_weekday (date)!=G_DATE_MONDAY ) {
     g_date_subtract_days (date, 1);
     flag = TRUE;
  }
  week = g_date_get_day_of_year (date)/7+1;
  month = g_date_get_month (date);
  year = g_date_get_year (date);
  /* here we have correct dates d/m/y for the top left graduation of calendar */
  calDay = g_date_get_day (date);/* required for tasks' drawing, don't remove ! */
  calMonth = month;
  calYear = year;
 // assign_store_date (g_date_get_day ( date), month, year);
  /* ruler calendar header group */
  GooCanvasItem *CalendarRuler = goo_canvas_group_new (root, NULL);
  data->rulerAssign = CalendarRuler;
  calendar_header = goo_canvas_rect_new (CalendarRuler, ASSIGN_CALENDAR_RSC_WIDTH, 
                                   ypos, ASSIGN_VIEW_MAX_WIDTH-ASSIGN_CALENDAR_RSC_WIDTH, ASSIGN_CALENDAR_HEADER_HEIGHT,
                                   "line-width", 0.0, "radius-x", 0.0, "radius-y", 0.0,
                                   "stroke-color", "white", "fill-color", "white",
                                   NULL);
  /* draw a separation line for calendar header */
  sep_line =  goo_canvas_polyline_new_line (CalendarRuler, ASSIGN_CALENDAR_RSC_WIDTH, ypos+demiHeight,
                                                     ASSIGN_VIEW_MAX_WIDTH+ASSIGN_CALENDAR_RSC_WIDTH, ypos+demiHeight,
                                                        "stroke-color", "#D7D7D7",
                                                        "line-width", 1.5, NULL);  
  /* draw text - each weeks - date is incremented day by day in the 2nd loop */
  for(i=0; i<ASSIGN_MAX_WEEKS; i++) { 
    GooCanvasItem *sep_vert = goo_canvas_polyline_new_line (CalendarRuler, 
                                                     ASSIGN_CALENDAR_RSC_WIDTH+i*sevenWidth, 
                                                     ypos,
                                                     ASSIGN_CALENDAR_RSC_WIDTH+i*sevenWidth, ypos+ASSIGN_CALENDAR_HEADER_HEIGHT,
                                                        "stroke-color", "#D7D7D7",
                                                        "line-width", 1.5, NULL); 
    /* dates are in properties data at start, and/or in tasks when a file is loaded */
    gchar *tmpMonth, *tmpMonthPrev;

    tmpMonth = misc_get_month_of_year (g_date_get_month (date));
    
    if(week>52) {
       week = 1;
       year++;
       flag = TRUE;
    }
     
    if(flag) {
       flag = FALSE;
       gint tmpMm = month+1;
       if(tmpMm>12)
          tmpMm = 1;
       tmpMonthPrev = misc_get_month_of_year (tmpMm);
       tmpStr = g_strdup_printf (_("week %d of %d - %s/%s"), week, year, tmpMonth, tmpMonthPrev);
       g_free (tmpMonthPrev);
    }
    else
      tmpStr = g_strdup_printf (_("week %d of %d - %s"), week, year, tmpMonth);

    g_free (tmpMonth);
    GooCanvasItem *sep_weeks = goo_canvas_text_new (CalendarRuler, tmpStr, 
                                   ASSIGN_CALENDAR_RSC_WIDTH+i*sevenWidth+12, ypos+4, -1,
                                   GOO_CANVAS_ANCHOR_NW,
                                   "font", "Sans 12", "fill-color", "#282828", 
                                   NULL);  

    g_free (tmpStr);
    week++;

    /* draw days inside week */
    for(j=0; j<7; j++) {
       gint dd = g_date_get_weekday (date);
       gchar *tmpDay;
       tmpDay = misc_get_short_day_of_week (dd);
       tmpStr = g_strdup_printf ("%s %d", tmpDay, g_date_get_day (date));
       g_free (tmpDay);
       if(j<5)
            sep_days = goo_canvas_text_new (CalendarRuler, tmpStr, 
                                   ASSIGN_CALENDAR_RSC_WIDTH+i*sevenWidth+(j*ASSIGN_CALENDAR_DAY_WIDTH)+demiWidth, 
                                   ypos+4+(ASSIGN_CALENDAR_HEADER_HEIGHT/2), -1,
                                   GOO_CANVAS_ANCHOR_NORTH,
                                   "font", "Sans 10", "fill-color", grey_day, 
                                   NULL);  
       else
            sep_days = goo_canvas_text_new (CalendarRuler, tmpStr, 
                                   ASSIGN_CALENDAR_RSC_WIDTH+i*sevenWidth+(j*ASSIGN_CALENDAR_DAY_WIDTH)+demiWidth, 
                                   ypos+4+(ASSIGN_CALENDAR_HEADER_HEIGHT/2), -1,
                                   GOO_CANVAS_ANCHOR_NORTH,
                                   "font", "Sans 10", "fill-color", blue_day, 
                                   NULL);  
       g_free (tmpStr);
       g_date_add_days (date, 1);
    }/* next day */
  }/* next week */
  /* we adjust prev month & next month labels */
  GtkWidget *labelPrev = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelPrevMonth"));
  GtkWidget *labelNext = GTK_WIDGET(gtk_builder_get_object (data->builder, "labelNextMonth"));
  /* test previous month */
  gint prev = mm-1;
  if(prev<=0)
     prev = 12;
  gint next = mm+1;
  if(next>12)
       next = 1;

  gtk_label_set_markup (GTK_LABEL(labelPrev), g_strdup_printf ("%s", misc_get_month_of_year (prev) ));
  gtk_label_set_markup (GTK_LABEL(labelNext), g_strdup_printf ("%s", misc_get_month_of_year (next) ));

}

/***************************************
  PUBLIC ! initialize ASSIGN drawing
****************************************/
void assign_canvas (GtkWidget *win, APP_data *data)
{
  GtkWidget  *canvas;
  GooCanvasItem *root, *rect_item, *calendar_header, *sep_line, *text_item;
  gint i, day =0, month, year = 0, duration = 0;
  gdouble hvalue;
  GDate *date;

  canvas = goo_canvas_new ();
  data->canvasAssign = canvas;
  g_object_set (canvas, "background-color" , "#F3F6F6", NULL);

  goo_canvas_set_bounds (GOO_CANVAS (canvas), 0, 0, ASSIGN_VIEW_MAX_WIDTH, ASSIGN_VIEW_MAX_HEIGHT);
  gtk_widget_show (canvas);
  gtk_container_add (GTK_CONTAINER (win), canvas);

  root = goo_canvas_get_root_item (GOO_CANVAS (canvas));

  date = g_date_new_dmy (data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
  day = g_date_get_day_of_year (date);

  /* we clamp to monday of the week */
  while (g_date_get_weekday (date)!=G_DATE_MONDAY) {
     g_date_subtract_days (date, 1);
     duration++ ; /* yes, thus we have for free the duration since monday and starting day */
  }

  day = g_date_get_day_of_year (date);
  month = g_date_get_month (date);
  year = g_date_get_year (date);
 
  assignDay = g_date_get_day (date);
  assignMonth = month;
  assignYear = year;
  /* ruler calendar header group */

  assign_draw_calendar_ruler (0, assignDay, month, year, data);// for tests TODO : 2e paramètre = mois en cours
  assign_draw_rsc_ruler (0, data);

  /* draw text - weeks */
  for(i=0; i<11; i++) { /*  2 months = 2*5 weeks+1 */
   /* we draw week_ends - not in ruler */

    GooCanvasItem *week_ends = goo_canvas_rect_new (root, 
                                ASSIGN_CALENDAR_RSC_WIDTH+i*(ASSIGN_CALENDAR_DAY_WIDTH*7 )+(5*ASSIGN_CALENDAR_DAY_WIDTH), 
                                ASSIGN_CALENDAR_HEADER_HEIGHT, 
                                (ASSIGN_CALENDAR_DAY_WIDTH*2 ), 
                                ASSIGN_VIEW_MAX_HEIGHT-ASSIGN_CALENDAR_HEADER_HEIGHT,
                                "line-width", 0.0, "radius-x", 0.0, "radius-y", 0.0,
                                "stroke-color", "#E9EEEE",
                                "fill-color", "#E9EEEE",
                                NULL);
  }
  /* an empty group in order to avoid segfaults */
  data->rscAssignMain = goo_canvas_group_new (root, NULL);
  
}

/******************************************

  PUBLIC : remove ALL tasks objects

******************************************/
void assign_remove_all_tasks (APP_data *data)
{
  gint i;
  GList *l;
  GooCanvasItem *root;

  root = goo_canvas_get_root_item (GOO_CANVAS (data->canvasAssign));
  /* we remove ressources ruler */
  if(data->rscAssign) {
     goo_canvas_item_remove (data->rscAssign);
  }
  data->rscAssign = NULL;
  /* and same for calendar */
  if(data->rulerAssign) {
     goo_canvas_item_remove (data->rulerAssign);
  }
  data->rulerAssign = NULL;
  /* remove all tasks */
  if(data->rscAssignMain) {
     goo_canvas_item_remove (data->rscAssignMain);
  }
  data->rscAssignMain = NULL;
}

/**************************************************
  PUBLIC : all task objects for current ressource
  argumentts : index = nth of ressource
   id = internal id of ressource
  root = root of goocanvas for assignments
  date= first date of current périod
*************************************************/
void assign_draw_task (gint index, gint id, GooCanvasItem *root, GDate *date, GdkRGBA color, APP_data *data)
{
  GList *l;
  tasks_data *tmp_tasks_datas;
  gint i, link, cmp1, cmp2, duration, nbDays, totalTasks = 0, currentTotal = 0;
  GDate date_start, date_end, date_start_period, date_end_period;
  GooCanvasItem *group_task;
  gchar *sRgba;

  /* we compute ending bound period */
  g_date_set_dmy (&date_start_period,  g_date_get_day (date), g_date_get_month (date), g_date_get_year (date) );
  g_date_set_dmy (&date_end_period,  g_date_get_day (date), g_date_get_month (date), g_date_get_year (date) );
  g_date_add_months (&date_end_period, 1);
  totalTasks = g_list_length (data->tasksList); 

// printf("date début période assign = %d /%d /%d \n", g_date_get_day (date), g_date_get_month (date), g_date_get_year (date));
  for(i=0; i<totalTasks; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     /* test if current task is between time bounds */
     g_date_set_dmy (&date_start, 
                         tmp_tasks_datas->start_nthDay, tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
     g_date_set_dmy (&date_end, 
                         tmp_tasks_datas->end_nthDay, tmp_tasks_datas->end_nthMonth, tmp_tasks_datas->end_nthYear);
     cmp1 = g_date_compare (&date_end, date);
     cmp2 = g_date_compare (&date_start, &date_end_period);
     if(!(cmp1<0) || (cmp2>0)){
        /* now, we check if ressource 'id' is used by current task */
         link = links_check_element (tmp_tasks_datas->id, id, data);
         if(link>=0) {
             /* date start is before period start ? */
             cmp1 = g_date_compare (&date_start, date);
             if(cmp1<0) { /* task starts before current period */
                nbDays = 0;
                /* is projets ends AFTER period ? */
                cmp2 = g_date_compare (&date_end, &date_end_period);
                if(cmp2>0) {
                   duration = g_date_days_between (date, &date_end_period);
                }
                else {
                   duration = g_date_days_between (date, &date_end);
                }/* duration */
             }
             else {/* projets starts during period */
                nbDays = g_date_days_between (date, &date_start);
                cmp2 = g_date_compare (&date_end, &date_end_period);
                if(cmp2>0) {
                   duration = g_date_days_between (&date_start, &date_end_period);
                }
                else {
                   duration = g_date_days_between (&date_start, &date_end);
                }/* duration */
             }/* elseif cmp1 */
             /* now we draw  rectangle & text*/
             //duration++;/* mandatory when task lasts only ONE day, natural duration equal 0 */
             group_task = goo_canvas_group_new (root, NULL);
             sRgba = misc_convert_gdkcolor_to_hex (tmp_tasks_datas->color);
             currentTotal++; /* TODO fractions of days ! */
             duration = tmp_tasks_datas->days;
             if(duration<=0)
                duration = 1;
	     assign_draw_bar (nbDays, duration, index, currentTotal, 
                               sRgba, tmp_tasks_datas->color, tmp_tasks_datas->name, group_task, data);       
	     g_free (sRgba);
         }
     }/* endif cmp dates */

  }/* next */
}
/******************************************
  PUBLIC : draw ALL tasks objects
  IMPORTANT : data->rscAssignMain is
  a GooCanvasItem 'group' used to
 anchor all tasks drawings
******************************************/
void assign_draw_all_tasks (APP_data *data)
{
  gint i, rscId;
  GList *l;
  rsc_datas *tmp_rsc_datas;
  GooCanvasItem *root;
  GDate date; /* to store oldest date of periode */

  root = goo_canvas_get_root_item (GOO_CANVAS (data->canvasAssign));
  g_date_set_dmy (&date, calDay, calMonth, calYear);
  data->rscAssignMain = goo_canvas_group_new (root, NULL);/* mandatory, it was detroyed each tume we erase all tasks ! */
  for(i=0; i<g_list_length (data->rscList); i++) {
     l = g_list_nth (data->rscList, i);
     tmp_rsc_datas = (rsc_datas *)l->data;
     rscId = tmp_rsc_datas->id;
     assign_draw_task (i, rscId, data->rscAssignMain, &date, tmp_rsc_datas->color, data);
  }/* next */
  /* we raise calendar above ALL*/
  goo_canvas_item_raise (data->rulerAssign, NULL);
  goo_canvas_item_raise (data->rscAssign, NULL);

}

/******************************************
  PUBLIC : redraw assignments after
  data change
******************************************/
void assign_draw_all_visuals (APP_data *data)
{
  gint day =0, month, year = 0, duration = 0;
  GtkAdjustment *vadj, *hadj;
  GDate *date;
  GDate dateCal;

  vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowAsst")));
  hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowAsst")));

  gtk_adjustment_set_value (GTK_ADJUSTMENT(vadj), 0);
  gtk_adjustment_set_value (GTK_ADJUSTMENT(hadj), 0);

  date = g_date_new_dmy (data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
  day = g_date_get_day (date);
  month = g_date_get_month (date);
  year = g_date_get_year (date);
  assign_store_date (day, month, year);

  assign_draw_calendar_ruler (0, day, month, year, data);
  assign_draw_rsc_ruler (0, data);
  assign_draw_all_tasks (data);

  /* we scroll horizontally to starting day */
  g_date_set_dmy (&dateCal, calDay, calMonth, calYear);
  duration = g_date_days_between (&dateCal, date);
  if(duration<0)
     duration = 0;

  gtk_adjustment_set_value (GTK_ADJUSTMENT(hadj), (gdouble) duration*ASSIGN_CALENDAR_DAY_WIDTH);

}

/*****************************************
  PUBLIC : general routine to export
  assignments diagram
  ARGUMENTS: type of export (see assign.h)
  returns 0 if success
*****************************************/
gint assign_export (gint type, APP_data *data)
{
  gint ret = 0;
  GKeyFile *keyString;
  gchar *filename, *tmpFileName;
  GtkFileFilter *filter = gtk_file_filter_new ();
  GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
  GtkWidget *alertDlg, *dialog;
  GooCanvasBounds bounds;
  cairo_surface_t *surface;
  cairo_t *cr;

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");

  switch(type) {
    case ASSIGN_EXPORT_PDF: {
      gtk_file_filter_add_pattern (filter, "*.pdf");
      gtk_file_filter_set_name (filter, _("Documents PDF files"));
      dialog = create_saveFileDialog (_("Export current assignments to a PDF file"), data);
      break;
    }
    case ASSIGN_EXPORT_SVG: {
      gtk_file_filter_add_pattern (filter, "*.svg");
      gtk_file_filter_set_name (filter, _("SVG drawings files"));
      dialog = create_saveFileDialog (_("Export current assignments to a SVG file"), data);
      break;
    }
    case ASSIGN_EXPORT_PNG: {
      gtk_file_filter_add_pattern (filter, "*.png");
      gtk_file_filter_set_name (filter, _("PNG image files"));
      dialog = create_saveFileDialog (_("Export current assignments to a PNG file"), data);
      break;
    }
    default: return -1;
  }/* end switch */

  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir());
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  /* run dialog for file selection */
  if(gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    /* test suffix */
     switch(type) {
	    case ASSIGN_EXPORT_PDF: {
              if(!g_str_has_suffix (filename, ".pdf" ) && !g_str_has_suffix (filename, ".PDF" )) {
                  /* we correct the filename */
                  tmpFileName = g_strdup_printf ("%s.pdf", filename);
                  g_free (filename);
                  filename = tmpFileName;
              }
	      break;
	    }
	    case ASSIGN_EXPORT_SVG: {
              if (!g_str_has_suffix (filename, ".svg" ) && !g_str_has_suffix (filename, ".SVG" )) {
                  /* we correct the filename */
                  tmpFileName = g_strdup_printf ("%s.svg", filename);
                  g_free (filename);
                  filename = tmpFileName;
              }
	      break;
	    }
	    case ASSIGN_EXPORT_PNG: {
              if (!g_str_has_suffix (filename, ".png" ) && !g_str_has_suffix (filename, ".PNG" )) {
                  /* we correct the filename */
                  tmpFileName = g_strdup_printf ("%s.png", filename);
                  g_free (filename);
                  filename = tmpFileName;
              }
	      break;
	    }
    }/* end switch extension */

    /* we must check if a file with same name exists */
    if(g_file_test (filename, G_FILE_TEST_EXISTS) 
        && g_key_file_get_boolean (keyString, "application", "prompt-before-overwrite",NULL )) {
           /* dialog to avoid overwriting */
           alertDlg = gtk_message_dialog_new (GTK_WINDOW(dialog),
                          flags,
                          GTK_MESSAGE_ERROR,
                          GTK_BUTTONS_OK_CANCEL,
                          _("The file :\n%s\n already exists !\nDo you really want to overwrite this file ?"),
                          filename);
           if(gtk_dialog_run (GTK_DIALOG(alertDlg)) == GTK_RESPONSE_CANCEL) {
              gtk_widget_destroy (GTK_WIDGET(alertDlg));
              gtk_widget_destroy (GTK_WIDGET(dialog));
              g_free (filename);
              return -1;
           }
    }/* endif file exists */

    /* now, we really export */
     ret = 0;
     switch(type) {
	    case ASSIGN_EXPORT_PDF: {
              surface = cairo_pdf_surface_create (filename, assign_get_max_x (), assign_get_max_y ());
	      break;
	    }
	    case ASSIGN_EXPORT_SVG: {
              surface = cairo_svg_surface_create (filename, assign_get_max_x (), assign_get_max_y ());
	      break;
	    }
	    case ASSIGN_EXPORT_PNG: {
              surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, assign_get_max_x (), assign_get_max_y ());
	      break;
	    }
    }/* end switch surfaces */
    cr = cairo_create (surface);
    cairo_translate (cr, 0, 0);
    /* canvas */
    GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment 
                               (GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowAsst")));
    GtkAdjustment *hadj = gtk_scrolled_window_get_hadjustment 
                               (GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowAsst")));

    bounds.x1 = gtk_adjustment_get_value (GTK_ADJUSTMENT(hadj));/* TODO repositionner colonne avatars */
    bounds.y1 = gtk_adjustment_get_value (GTK_ADJUSTMENT(vadj));
    bounds.x2 = assign_get_max_x ();
    bounds.y2 = assign_get_max_y ();
    goo_canvas_render (GOO_CANVAS (data->canvasAssign), cr, &bounds, 0.0);/* don't work without this include : #include <cairo-pdf.h> */ /* because goocanvas waits for this macro : CAIRO_HAS_PDF_SURFACE */
    switch(type) {
	    case ASSIGN_EXPORT_PDF: {
              cairo_show_page (cr);
              cairo_surface_destroy (surface);
              cairo_destroy (cr);
	      break;
	    }
	    case ASSIGN_EXPORT_SVG: {
              cairo_surface_destroy (surface);
              cairo_destroy (cr);
	      break;
	    }
	    case ASSIGN_EXPORT_PNG: {
              cairo_surface_write_to_png (surface, filename);
              cairo_surface_destroy (surface);
              cairo_destroy (cr);
	      break;
	    }
    }/* end switch surfaces */
    if(ret!=0) {
        gchar *msg = g_strdup_printf ("%s", _("Sorry !\nAn error happened during assignment exporting.\nOperation aborted. You can try again, or\ncheck your system."));
        misc_ErrorDialog (data->appWindow, msg);
        g_free (msg);
    }
    g_free (filename);
  }

  gtk_widget_destroy (GTK_WIDGET(dialog)); 


  return ret;
}
