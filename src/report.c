/**********************************
  report.c module
  functions, constants for
  displaying reports on
  a gtktextview

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
#include "support.h"
#include "misc.h"
#include "links.h"
#include "tasks.h"
#include "tasksutils.h"
#include "assign.h"
#include "ressources.h"
#include "calendars.h"
#include "report.h"
#include "files.h"
#include "export.h"

/******************************
  test is there is something to
  do : at least, ONE task and
  ONE ressource
******************************/
gboolean report_is_ready_to_report (APP_data *data)
{
  gboolean rep = TRUE;
  gchar *tmpStr;

  if((misc_count_tasks (data)<1) || (g_list_length (data->rscList)<1)) {
     rep = FALSE;
       tmpStr = g_strdup_printf ("%s", _("Sorry !\nIn order to report something, you must assign.\nat least <u>ONE</u> resource to <u>ONE</u> task.\nPlease, check <b>resources</b> page and <b>tasks</b> page.\nOperation aborted."));
       misc_ErrorDialog (data->appWindow, tmpStr);
       g_free (tmpStr);
  }
  return rep;
}

/*******************************
  PUBLIC - lock/unlock 
  menus and buttons
*******************************/
void report_set_buttons_menus (gboolean fRtf, gboolean fMail, APP_data *data)
{
  GtkWidget *menuSendReport, *buttonSendReport, *menuExportReport, *buttonExportReport;

  /* set sensible menus and buttons */
  menuSendReport = GTK_WIDGET(gtk_builder_get_object (data->builder, "menuSendReport"));
  buttonSendReport = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonSendReport"));
  buttonExportReport = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonExportReport"));
  menuExportReport = GTK_WIDGET(gtk_builder_get_object (data->builder, "menuExportReport"));

  gtk_widget_set_sensitive (GTK_WIDGET(menuSendReport), fMail);
  gtk_widget_set_sensitive (GTK_WIDGET(buttonSendReport), fMail);

  gtk_widget_set_sensitive (GTK_WIDGET(buttonExportReport), fRtf);
  gtk_widget_set_sensitive (GTK_WIDGET(menuExportReport), fRtf);
}

/*******************************
 clear text in memory
*******************************/
void report_clear_text (GtkTextBuffer *buffer, const gchar *tag)
{
  GtkTextIter start, end;

  gtk_text_buffer_set_text (buffer, "", 0); /* Clear text! */
  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_get_end_iter (buffer, &end);
  gtk_text_buffer_apply_tag_by_name (buffer, tag, &start, &end);

}

/*******************************
 display default text 
*******************************/
void report_default_text (GtkTextBuffer *buffer)
{
  GtkTextIter start, end;

  gtk_text_buffer_set_text (buffer, _("There isn't any report for now.\nThis area is editable, you can type ideas, append them to reports, paste text from other applications, and so on ...\n"), -1);
  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_get_end_iter (buffer, &end);
  gtk_text_buffer_apply_tag_by_name (buffer, "bold", &start, &end);
  gtk_text_buffer_apply_tag_by_name (buffer, "italic", &start, &end);

}

/***************************************
  set-up various tags for 
  report/editor's buffer
***************************************/
static void report_setup_text_buffer_tags (GtkTextBuffer *buffer)
{
 GtkTextTag *big_tag, *bold_tag, *italic_tag, *underline_tag, *center_tag, *left_tag, *right_tag, *fill_tag, *std_font_tag;
 GtkTextTag *superscript_tag, *subscript_tag, *highlight_tag, *strikethrough_tag, *quotation_tag;

 /* character formating */
  big_tag = gtk_text_buffer_create_tag (buffer, "title",
                                    "size", 24 * PANGO_SCALE,
				    "weight", PANGO_WEIGHT_BOLD,
                                    NULL);
  bold_tag = gtk_text_buffer_create_tag (buffer, "bold",
				    "weight", PANGO_WEIGHT_BOLD,
                                    NULL);
  italic_tag = gtk_text_buffer_create_tag (buffer, "italic",
				    "style", PANGO_STYLE_ITALIC,
                                    NULL);
  underline_tag = gtk_text_buffer_create_tag (buffer, "underline",
				    "underline", PANGO_UNDERLINE_SINGLE,
                                    NULL);
  /* source : https://github.com/GNOME/gtk/blob/master/demos/gtk-demo/textview.c */
  superscript_tag = gtk_text_buffer_create_tag (buffer, "superscript",
                              "rise", 10 * PANGO_SCALE,   /* 10 pixels */
                              "size", 8 * PANGO_SCALE,    /* 8 points */
                                NULL);
  subscript_tag = gtk_text_buffer_create_tag (buffer, "subscript",
                              "rise", -10 * PANGO_SCALE,   /* 10 pixels */
                              "size", 8 * PANGO_SCALE,     /* 8 points */
                               NULL);

  highlight_tag = gtk_text_buffer_create_tag (buffer, "highlight",
                                              "background", "yellow", NULL);
  strikethrough_tag = gtk_text_buffer_create_tag (buffer, "strikethrough",
                                              "strikethrough", TRUE, NULL);
  quotation_tag =  gtk_text_buffer_create_tag (buffer, "quotation",
                              "family", "courier",
                              "size", 13 * PANGO_SCALE,
                              "foreground", "#5E5353",
                              "background", "#DCDCDC",
                              "indent", 30,
                              "left_margin", 40,
                              "right_margin", 40,
                              "justification", GTK_JUSTIFY_FILL,
                               NULL);
  /* create tags : line formating */

  center_tag = gtk_text_buffer_create_tag (buffer, "center",
				    "justification", GTK_JUSTIFY_CENTER,
                                    NULL);
  left_tag = gtk_text_buffer_create_tag (buffer, "left",
				    "justification", GTK_JUSTIFY_LEFT,
                                    NULL);
  right_tag = gtk_text_buffer_create_tag (buffer, "right",
				    "justification", GTK_JUSTIFY_RIGHT,
                                    NULL);
  fill_tag = gtk_text_buffer_create_tag (buffer, "fill",
				    "justification", GTK_JUSTIFY_FILL,
                                    NULL);

}

/*********************************
  PROTECTED : prepare textview
*********************************/
static GtkWidget *report_prepare_view (void)
{
  GtkWidget *view;

  view = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
  gtk_widget_set_name (view, "view");
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW(view), 8);
  gtk_text_view_set_right_margin (GTK_TEXT_VIEW(view), 8);

  return view;
}

/*******************************
  PUBLIC : initialize
  text view
*******************************/
void report_init_view (APP_data *data)
{
  GtkWidget *view, *swReport;
  GtkTextBuffer *buffer;
  GtkTextIter start, end;

  /* main Gtk text view */
  view = report_prepare_view();
  gtk_widget_show (view);
  swReport = GTK_WIDGET(gtk_builder_get_object (data->builder, "scrolledwindowReport"));
  gtk_container_add (GTK_CONTAINER (swReport), view);

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(view));
  report_setup_text_buffer_tags (buffer);
  data->view = GTK_TEXT_VIEW(view);
  data->buffer = buffer;
  report_clear_text (buffer, "left");

  report_default_text (buffer);
}

/***********************************************
 LOCAL draw central circle for some diagrams
 arguments : cr cairo context
 extents : cairo text extents
red, green, blue flotaing point color components
 str : string to display
*************************************************/
static void report_draw_circle_text (cairo_t *cr, gdouble red, gdouble green, gdouble blue, gchar *tmpStr)
{
  cairo_text_extents_t extents;

  /* central white circle with duration*/
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_arc (cr, REPORT_RSC_COST_WIDTH/2, REPORT_RSC_COST_HEIGHT/2, 70, 0, 2*G_PI);
  cairo_fill (cr);
  cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size (cr, 20);
  cairo_text_extents (cr, tmpStr, &extents);
  cairo_move_to (cr, (REPORT_RSC_COST_WIDTH/2) - extents.width/2, REPORT_RSC_COST_HEIGHT/2+(extents.height/2));  
  cairo_set_source_rgb (cr, red, green, blue);
  cairo_show_text (cr, tmpStr); 

}

/*********************************
 PROTECTED : draw legend
*********************************/
static void report_draw_legend (cairo_t *cr, APP_data *data)
{
  GKeyFile *keyString;
  cairo_text_extents_t extents;

  keyString = data->keystring;
     /* legend */
      cairo_set_source_rgb (cr, 0.2, 0.65, 0.32);
      cairo_rectangle (cr, 0, 0, 16, 16);
      cairo_fill (cr);

      cairo_set_source_rgb (cr, 0.92, 0.26, 0.21);
      cairo_rectangle (cr, 0, 20, 16, 16);
      cairo_fill (cr);

      cairo_set_source_rgb (cr, 0.98, 0.74, 0.02);
      cairo_rectangle (cr, 0, 40, 16, 16);
      cairo_fill (cr);

      cairo_set_source_rgb (cr, 0.26, 0.52, 0.96);
      cairo_rectangle (cr, 0, 60, 16, 16);
      cairo_fill (cr);

      cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size(cr, 14);
      cairo_set_source_rgb (cr, g_key_file_get_double (keyString, "editor", "text.color.red", NULL), 
                                g_key_file_get_double (keyString, "editor", "text.color.green", NULL), 
                                g_key_file_get_double (keyString, "editor", "text.color.blue", NULL));
      cairo_text_extents (cr, "Ap", &extents);
      cairo_move_to (cr, 20 , 0+extents.height);
      cairo_show_text (cr, _("completed"));
      cairo_move_to (cr, 20 , 20+extents.height);
      cairo_show_text (cr, _("Overdue"));    
      cairo_move_to (cr, 20 , 40+extents.height);
      cairo_show_text (cr, _("To go"));  
      cairo_move_to (cr, 20 , 60+extents.height);
      cairo_show_text (cr, _("In progress"));  
}

/***********************************
  PUBLIC : ressource usage
  analysis, for ONE ressource
**********************************/
void report_unique_rsc_charge (gint id, APP_data *data)
{
  GKeyFile *keyString;
  gchar *tmpStr;
  GtkTextIter start, end, iter;
  GdkPixbuf *pixbuf2;
  gint total, completed, overdue, to_go, on_progress;
  GtkWidget *notebook;
  cairo_surface_t *surf = NULL;
  cairo_t *cr = NULL;
  cairo_text_extents_t extents;
  gdouble angle1, angle2;

  keyString = data->keystring;
  /* when the right-click happens, the convenient row is selected on "ressources" panel ; so, we know the ressource */
  report_clear_text (data->buffer, "left");
  misc_set_font_color_settings (data);
  gtk_text_buffer_get_iter_at_mark (data->buffer, &iter, gtk_text_buffer_get_insert (data->buffer));

  /* we get GtkTextIter in order to insert nex text */
  tmpStr = g_strdup_printf ("%s %s\n", _("Assignments for Resource "), rsc_get_name (id, data));
  gtk_text_buffer_insert_with_tags_by_name (data->buffer, &iter, tmpStr, -1, "title", "underline", NULL);
  g_free (tmpStr);

  /* display avatar */
  pixbuf2 = rsc_get_avatar (id, data);
  if(pixbuf2) {
     gtk_text_buffer_insert_pixbuf (data->buffer, &iter, pixbuf2);
     g_object_unref (pixbuf2);
  }

  tmpStr = g_strdup_printf (_("\nThe current project has %d task(s)\n"), misc_count_tasks (data));
  gtk_text_buffer_insert (data->buffer, &iter, tmpStr, -1);
  g_free (tmpStr);

  total = rsc_get_number_tasks (id, data);
  tmpStr = g_strdup_printf (_("%s is assigned to %d task(s)\n"), rsc_get_name (id, data),
                                total );
  gtk_text_buffer_insert (data->buffer, &iter, tmpStr, -1);
  g_free (tmpStr);

  if(total<=0) {
     gtk_text_buffer_insert (data->buffer, &iter, _("No more operations - Resource hasn't any task !"), -1);
  }

  completed = rsc_get_number_tasks_done (id, data);
  tmpStr = g_strdup_printf (_("%s has completed %d task(s)\n"), rsc_get_name (id, data),
                                completed);
  gtk_text_buffer_insert (data->buffer, &iter, tmpStr, -1);
  g_free (tmpStr);

  overdue = rsc_get_number_tasks_overdue (id, data);
  tmpStr = g_strdup_printf (_("%s if overdue for %d task(s)\n"), rsc_get_name (id, data),
                                overdue);
  gtk_text_buffer_insert (data->buffer, &iter, tmpStr, -1);
  g_free (tmpStr);

  to_go = rsc_get_number_tasks_to_go (id, data);
  tmpStr = g_strdup_printf (_("%s if on go for %d task(s)\n"), rsc_get_name (id, data),
                               to_go );
  gtk_text_buffer_insert (data->buffer, &iter, tmpStr, -1);
  g_free (tmpStr);

  on_progress = total-overdue-to_go-completed;

  tmpStr = g_strdup_printf (_("The average progress for his/her task(s) is %.2f %\n\n"), 
                             rsc_get_number_tasks_average_progress (id, data) );
  gtk_text_buffer_insert (data->buffer, &iter, tmpStr, -1);
  g_free (tmpStr);

  /* we "draw" in memory */
  if(total>0) {
	  surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, ASSIGN_RSC_CHARGE_WIDTH, ASSIGN_RSC_CHARGE_HEIGHT);
	  /* now we can create the context */
	  cr = cairo_create (surf);
	  cairo_set_source_rgb (cr, 0.5, 0.5, 0.5);
	  cairo_set_line_width (cr, 1.5);
	  cairo_arc (cr, ASSIGN_RSC_CHARGE_WIDTH/2, ASSIGN_RSC_CHARGE_HEIGHT/2,  ASSIGN_RSC_CHARGE_RADIUS, 0, 2*G_PI);
	  cairo_fill (cr);
	  /* draw some pie parts */
	  angle1 = G_PI* 3 / 2;
	  angle2 = angle1;
	  /* completed part */
	  angle2 = angle2+(2 *G_PI* (gdouble) completed/total);
	  cairo_move_to (cr, ASSIGN_RSC_CHARGE_WIDTH/2, ASSIGN_RSC_CHARGE_HEIGHT/2);
	  cairo_set_source_rgb (cr, 0.2, 0.65, 0.32);
	  cairo_arc (cr, ASSIGN_RSC_CHARGE_WIDTH/2, ASSIGN_RSC_CHARGE_HEIGHT/2, ASSIGN_RSC_CHARGE_RADIUS, angle1, angle2);
	  cairo_move_to (cr, ASSIGN_RSC_CHARGE_WIDTH/2, ASSIGN_RSC_CHARGE_HEIGHT/2);
	  cairo_fill (cr);     
	  /* overdue */
	  angle1 = angle2;
	  angle2 = angle2+(2 * G_PI* (gdouble) overdue/total);    
	  cairo_move_to (cr, ASSIGN_RSC_CHARGE_WIDTH/2, ASSIGN_RSC_CHARGE_HEIGHT/2);
	  cairo_set_source_rgb (cr, 0.92, 0.26, 0.21);
	  cairo_arc (cr, ASSIGN_RSC_CHARGE_WIDTH/2, ASSIGN_RSC_CHARGE_HEIGHT/2, ASSIGN_RSC_CHARGE_RADIUS, angle1, angle2);
	  cairo_move_to (cr, ASSIGN_RSC_CHARGE_WIDTH/2, ASSIGN_RSC_CHARGE_HEIGHT/2);
	  cairo_fill (cr);  
	  /* to-go */
	  angle1 = angle2;
	  angle2 = angle2+(2 *G_PI* (gdouble) to_go/total);    
	  cairo_move_to (cr, ASSIGN_RSC_CHARGE_WIDTH/2, ASSIGN_RSC_CHARGE_HEIGHT/2);
	  cairo_set_source_rgb (cr, 0.98, 0.74, 0.02);
	  cairo_arc (cr, ASSIGN_RSC_CHARGE_WIDTH/2, ASSIGN_RSC_CHARGE_HEIGHT/2, ASSIGN_RSC_CHARGE_RADIUS, angle1, angle2);
	  cairo_move_to (cr, ASSIGN_RSC_CHARGE_WIDTH/2, ASSIGN_RSC_CHARGE_HEIGHT/2);
	  cairo_fill (cr);  
	  /* progress */     
	  angle1 = angle2;
	  angle2 = angle2+(2 *G_PI* (gdouble) on_progress/total);    
	  cairo_move_to (cr, ASSIGN_RSC_CHARGE_WIDTH/2, ASSIGN_RSC_CHARGE_HEIGHT/2);
	  cairo_set_source_rgb (cr, 0.26, 0.52, 0.96);
	  cairo_arc (cr, ASSIGN_RSC_CHARGE_WIDTH/2, ASSIGN_RSC_CHARGE_HEIGHT/2, ASSIGN_RSC_CHARGE_RADIUS, angle1, angle2);
	  cairo_move_to (cr, ASSIGN_RSC_CHARGE_WIDTH/2, ASSIGN_RSC_CHARGE_HEIGHT/2);
	  cairo_fill (cr); 

	  /* central white circle with percent*/
	  tmpStr = g_strdup_printf (_("%.2f%%"), rsc_get_number_tasks_average_progress (id, data));
	  report_draw_circle_text (cr, g_key_file_get_double(keyString, "editor", "text.color.red", NULL), 
		                       g_key_file_get_double(keyString, "editor", "text.color.green", NULL), 
		                       g_key_file_get_double(keyString, "editor", "text.color.blue", NULL), tmpStr);
	  g_free (tmpStr);
	  /* legend */
	  report_draw_legend (cr, data);

	  cairo_destroy (cr);
	  pixbuf2 = gdk_pixbuf_get_from_surface (surf, 0, 0, ASSIGN_RSC_CHARGE_WIDTH, ASSIGN_RSC_CHARGE_HEIGHT);

	  cairo_surface_destroy (surf);
	  gtk_text_buffer_insert_pixbuf (data->buffer, &iter, pixbuf2);
	  
	  g_object_unref (pixbuf2);
  }
  /* switch to report notebook page */
  notebook = GTK_WIDGET(gtk_builder_get_object (data->builder, "notebook1"));
  gtk_notebook_set_current_page (GTK_NOTEBOOK(notebook), 6);
  /* set sensible menus and buttons */
  report_set_buttons_menus (TRUE, FALSE, data);
}

/************************************
  LOCAL : draw bars for current
  ressource
************************************/
static void report_draw_bar (gint nrsc, gdouble x, gdouble y, gdouble step, gdouble units, cairo_t *cr, APP_data *data)
{
  GList *l;
  rsc_datas *tmp_rsc_datas;
  gint id, total, completed, overdue, to_go, on_progress;
  gdouble xpos;
  if(nrsc>=g_list_length (data->rscList))
     return;

  l = g_list_nth (data->rscList, nrsc);
  tmp_rsc_datas = (rsc_datas *)l->data; 
  id = tmp_rsc_datas->id;

  total = rsc_get_number_tasks (id, data);
  completed = rsc_get_number_tasks_done (id, data);
  overdue = rsc_get_number_tasks_overdue (id, data);
  to_go = rsc_get_number_tasks_to_go (id, data);
  on_progress = total-overdue-to_go-completed;
  xpos = x;

  /* we draw all bars in order : completed, progress, overdue, to_go */

  /* completed */
  cairo_set_source_rgb (cr, 0.2, 0.65, 0.32);
  cairo_rectangle (cr, xpos, y+2, completed*step, units-4);
  cairo_fill (cr);
  xpos = xpos+completed*step;

  /* overdue */
  cairo_set_source_rgb (cr, 0.92, 0.26, 0.21);
  cairo_rectangle (cr, xpos, y+2, overdue*step, units-4);
  cairo_fill (cr);
  xpos = xpos+overdue*step;

  /* to-go */
  cairo_set_source_rgb (cr, 0.98, 0.74, 0.02);
  cairo_rectangle (cr, xpos, y+2, to_go*step, units-4);
  cairo_fill (cr);
  xpos = xpos+to_go*step;

  /* progress*/
  cairo_set_source_rgb (cr, 0.26, 0.52, 0.96);
  cairo_rectangle (cr, xpos, y+2, on_progress*step, units-4);
  cairo_fill (cr);
  xpos = xpos+on_progress*step;

}

/************************************
  PUBLIC : ressource usage
  analysis, for ALL ressource
**********************************/
void report_all_rsc_charge (APP_data *data)
{
  GKeyFile *keyString;
  gchar *tmpStr;
  GtkTextIter start, end, iter;
  GdkPixbuf *pixbuf2;
  GList *l;
  rsc_datas *tmp_rsc_datas;;
  gint total, i, j, count, height, cur_rsc, completed, overdue, to_go, on_progress;
  GtkWidget *notebook;
  cairo_surface_t *surf = NULL;
  cairo_t *cr = NULL;
  cairo_text_extents_t extents;
  gdouble units, tsk_average, hStep;

   report_clear_text (data->buffer, "left");
   /* we get GtkTextIter in order to insert nex text */
   gtk_text_buffer_get_iter_at_mark (data->buffer, &iter, gtk_text_buffer_get_insert (data->buffer));
   /* test is there is tasks and/or ressources */
   if(!report_is_ready_to_report (data))
      return;
   misc_set_font_color_settings (data);
   keyString = data->keystring;

   /* when the right-click happens, the convenient row is selected on "ressources" panel ; so, we know the ressource */
   total = g_list_length (data->rscList);
   if(total<=0) {
      gtk_text_buffer_insert_with_tags_by_name (data->buffer, &iter, _("No datas for reporting ! Quit."), -1, "title", "underline", NULL);
      return;
   }
   else {
      tmpStr = g_strdup_printf (_("Assignments for the %d Resources \n\n"), total);
      gtk_text_buffer_insert_with_tags_by_name (data->buffer, &iter, tmpStr, -1, "title", "underline", NULL);
      g_free (tmpStr);
   }

   tmpStr = g_strdup_printf (_("\nProject \"%s\" has %d task(s),\n"), data->properties.title, misc_count_tasks (data));
   gtk_text_buffer_insert (data->buffer, &iter, tmpStr, -1);
   g_free (tmpStr);
   
   tsk_average = (gdouble) rsc_get_absolute_max_total_tasks (data)/total;
   tmpStr = g_strdup_printf (_("with an average of %.2f task(s) by resource.\n"), tsk_average);
   gtk_text_buffer_insert (data->buffer, &iter, tmpStr, -1);
   g_free (tmpStr);

   /* each diagram has at maximum 10 ressources for example, if we have 20 ressources, we get 20/2 = 2 diagrams */
   units = REPORT_DIAGRAM_HEIGHT/10;
   hStep = (gdouble)(REPORT_DIAGRAM_WIDTH-REPORT_DIAGRAM_NAME_WIDTH) / rsc_get_absolute_max_tasks (data);
   count = total/10;
   cur_rsc = 0;

   for(i=0; i<=count; i++) {
      tmpStr = g_strdup_printf (_("\nDiagram %d of %d\n"), i+1, count+1);
      gtk_text_buffer_insert_with_tags_by_name (data->buffer, &iter, tmpStr, -1, "bold", NULL);
      g_free (tmpStr);
      /* diagram */
      surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, REPORT_DIAGRAM_WIDTH, 
                                          REPORT_DIAGRAM_LEGEND_HEIGHT+REPORT_DIAGRAM_HEIGHT+REPORT_DIAGRAM_INDEX_HEIGHT);
      /* now we can create the context */
      cr = cairo_create (surf);
      /* compute useful values for texts */
      cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size (cr, 14);
      cairo_text_extents (cr, "Ap", &extents);
      height = (gint)extents.height;
      if(height<units)
         height = units;
      /* legend */
      report_draw_legend (cr, data);
      /* diagram"s background */
      cairo_set_source_rgb (cr, 0.91, 0.94, 0.96);/* light blue : #E7F0F5 */
      cairo_set_line_width (cr, 1.5);
      cairo_rectangle (cr, REPORT_DIAGRAM_NAME_WIDTH, REPORT_DIAGRAM_LEGEND_HEIGHT, 
                       REPORT_DIAGRAM_WIDTH-REPORT_DIAGRAM_NAME_WIDTH-2, REPORT_DIAGRAM_HEIGHT);
      cairo_fill (cr);
      /* we draw ressources names and bars */
      j = 0;
      while((j<10) && (cur_rsc<total)) {
         l = g_list_nth (data->rscList, cur_rsc);
         tmp_rsc_datas = (rsc_datas *)l->data; 
         cairo_text_extents (cr, tmp_rsc_datas->name, &extents);
         cairo_set_source_rgb (cr, g_key_file_get_double (keyString, "editor", "text.color.red", NULL), 
                                g_key_file_get_double (keyString, "editor", "text.color.green", NULL), 
                                g_key_file_get_double (keyString, "editor", "text.color.blue", NULL));
         cairo_move_to (cr, REPORT_DIAGRAM_NAME_WIDTH-extents.width-8, REPORT_DIAGRAM_LEGEND_HEIGHT+((j+1)*height )-2);         
         cairo_show_text (cr, tmp_rsc_datas->name);
         /* bars */
         report_draw_bar (cur_rsc, REPORT_DIAGRAM_NAME_WIDTH, REPORT_DIAGRAM_LEGEND_HEIGHT+(j*height ), hStep, units, cr, data);
         j++;
         cur_rsc++;
      }/* wend */

      /* we draw average line */
      cairo_set_source_rgb (cr, 0.36, 0.61, 0.83);/* intense blue  */
      cairo_set_line_width (cr, 1);
      cairo_move_to (cr, REPORT_DIAGRAM_NAME_WIDTH+(hStep*tsk_average), REPORT_DIAGRAM_LEGEND_HEIGHT+1 );
      cairo_line_to (cr, REPORT_DIAGRAM_NAME_WIDTH+(hStep*tsk_average), REPORT_DIAGRAM_LEGEND_HEIGHT+REPORT_DIAGRAM_HEIGHT );
      cairo_stroke (cr);
      /* average */
      cairo_set_source_rgb (cr, g_key_file_get_double(keyString, "editor", "text.color.red", NULL), 
                                g_key_file_get_double(keyString, "editor", "text.color.green", NULL), 
                                g_key_file_get_double(keyString, "editor", "text.color.blue", NULL));
      tmpStr = g_strdup_printf ("%s", _("Avg"));
      cairo_text_extents (cr, tmpStr, &extents);
      cairo_move_to (cr, REPORT_DIAGRAM_NAME_WIDTH+(hStep*tsk_average)-(extents.width/2),
                        REPORT_DIAGRAM_LEGEND_HEIGHT+REPORT_DIAGRAM_HEIGHT+2*extents.height+4);
      cairo_show_text (cr, tmpStr);
      g_free (tmpStr);
      /* diagram title */
      tmpStr = g_strdup_printf ("%s", _("Tasks by resources"));
      cairo_text_extents (cr, tmpStr, &extents);
      cairo_move_to (cr, (REPORT_DIAGRAM_NAME_WIDTH + REPORT_DIAGRAM_WIDTH)/2- (extents.width/2),
                        REPORT_DIAGRAM_LEGEND_HEIGHT-extents.height);
      cairo_show_text (cr, tmpStr);
      g_free (tmpStr);
      /* we draw index numbers */
      cairo_set_line_width (cr, 1);
      for(j=0; j<rsc_get_absolute_max_tasks (data); j++) {
         cairo_set_source_rgb (cr, 1, 1, 1);
         cairo_move_to (cr, REPORT_DIAGRAM_NAME_WIDTH+(hStep*j), REPORT_DIAGRAM_LEGEND_HEIGHT+1 );
         cairo_line_to (cr, REPORT_DIAGRAM_NAME_WIDTH+(hStep*j), REPORT_DIAGRAM_LEGEND_HEIGHT+REPORT_DIAGRAM_HEIGHT );
         cairo_stroke (cr);
         cairo_set_source_rgb (cr, g_key_file_get_double(keyString, "editor", "text.color.red", NULL), 
                                g_key_file_get_double(keyString, "editor", "text.color.green", NULL), 
                                g_key_file_get_double(keyString, "editor", "text.color.blue", NULL));
         tmpStr = g_strdup_printf ("%d", j);
         cairo_text_extents (cr, tmpStr, &extents);
         cairo_move_to (cr, REPORT_DIAGRAM_NAME_WIDTH+(hStep*j)-(extents.width/2), 
                        REPORT_DIAGRAM_LEGEND_HEIGHT+REPORT_DIAGRAM_HEIGHT+extents.height+4);
         cairo_show_text (cr, tmpStr);
         g_free (tmpStr);
       }/* next j */

      /* we relize it and insert in gtktextview */
      cairo_destroy (cr);
      pixbuf2 = gdk_pixbuf_get_from_surface (surf, 0, 0, REPORT_DIAGRAM_WIDTH, 
                                              REPORT_DIAGRAM_LEGEND_HEIGHT+REPORT_DIAGRAM_HEIGHT+REPORT_DIAGRAM_INDEX_HEIGHT);
      cairo_surface_destroy (surf);
      gtk_text_buffer_insert_pixbuf (data->buffer, &iter, pixbuf2);
      g_object_unref (pixbuf2);

   }/* next i */
   /* switch to report notebook page */
   notebook = GTK_WIDGET(gtk_builder_get_object (data->builder, "notebook1"));
   gtk_notebook_set_current_page (GTK_NOTEBOOK(notebook), 6);
   /* set sensible menus and buttons */
   report_set_buttons_menus (TRUE, TRUE, data);
}

/************************************
 LOCAL : compute how many days
 a ressource is used
 parameter : ID of ressource
************************************/
static gdouble report_get_tasks_duration (gint id, APP_data *data)
{
  gint i, link;
  gdouble ret = 0;
  GList *l;
  gint tsk_list_len = g_list_length (data->tasksList);
  tasks_data *tmp_tasks_datas;

  for(i=0; i<tsk_list_len; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     /* we test if for current task, the ressource 'ID' is a sender */
     link = links_check_element (tmp_tasks_datas->id, id, data);
     if(link>=0) { 
        if(tmp_tasks_datas->days>0)       
           ret = ret +tmp_tasks_datas->days;/* nothing to correct, it's a "gross" value */
        else
           ret = ret+1;
     }/* endif link */
  }/* next i */

  return ret;

}

/************************************
 LOCAL : compute how many hours
 a ressource is used
 parameter : ID of ressource
************************************/
static gdouble report_get_tasks_hours (gint id, APP_data *data)
{
  gint i, link;
  gdouble ret = 0;
  GList *l;
  gint tsk_list_len = g_list_length (data->tasksList);
  tasks_data *tmp_tasks_datas;

  for(i=0; i<tsk_list_len; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     /* we test if for current task, the ressource 'ID' is a sender */
     link = links_check_element (tmp_tasks_datas->id, id, data);
     if(link>=0) {        
        ret = ret +tmp_tasks_datas->hours;/* nothing to correct, it's a "gross" value */
     }/* endif link */
  }/* next i */

  return ret;

}

/************************************
 LOCAL : compute how many days
 a ressource is used, minus
 non-working days
 argument : ID of ressource
************************************/
static gdouble report_get_tasks_duration_corrected (gint id, APP_data *data)
{
  gint i, link, duration, rr = 0;
  gdouble ret = 0;
  GDate date;
  GList *l, *l2;
  gint tsk_list_len = g_list_length (data->tasksList);
  tasks_data *tmp_tasks_datas;
  rsc_datas *tmp_rsc_datas;

  /* we search ressource datas pointer with current id */
  i = 0;
  while((i<g_list_length (data->rscList)) && (rr==0)) {
    l2 = g_list_nth (data->rscList, i); 
    tmp_rsc_datas = (rsc_datas *)l2->data;
    if(tmp_rsc_datas->id == id) {
        rr = 1;
    }
    i++;
  }/* wend */
  
  for(i=0;i<tsk_list_len;i++) {
     l = g_list_nth (data->tasksList, i);   
     tmp_tasks_datas = (tasks_data *)l->data;
     /* we test if for current task, the ressource 'ID' is a sender */
     link = links_check_element (tmp_tasks_datas->id, id, data);
     if(link>=0) {
        ret = ret+misc_correct_duration (tmp_tasks_datas->start_nthDay, tmp_tasks_datas->start_nthMonth, 
                                    tmp_tasks_datas->start_nthYear, tmp_tasks_datas->days,
                                    tmp_rsc_datas->fWorkMonday, tmp_rsc_datas->fWorkTuesday, 
                                    tmp_rsc_datas->fWorkWednesday, tmp_rsc_datas->fWorkThursday,
                                    tmp_rsc_datas->fWorkFriday, tmp_rsc_datas->fWorkSaturday,
                                    tmp_rsc_datas->fWorkSunday, tmp_tasks_datas->calendar, tmp_rsc_datas->calendar, data);
     }/* endif link */
  }/* next i */

  return ret;

}

/***************************************************************
 LOCAL : compute how many minutes
 a ressource is used, minus
 non-working days and with true periods of work in account
 argument : ID of ressource
*****************************************************************/
static gdouble report_get_tasks_duration_corrected_with_periods (gint id, APP_data *data)
{
  gint i, link, duration, rr = 0, max_minutes;
  gdouble ret = 0, tmpVal;
  GDate date;
  GList *l, *l2;
  gint tsk_list_len = g_list_length (data->tasksList);
  tasks_data *tmp_tasks_datas;
  rsc_datas *tmp_rsc_datas;

  max_minutes = rsc_get_max_daily_working_time (id, data);
  /* we search ressource datas pointer with current id */
  i = 0;
  while((i<g_list_length (data->rscList)) && (rr==0)) {
    l2 = g_list_nth (data->rscList, i); 
    tmp_rsc_datas = (rsc_datas *)l2->data;
    if(tmp_rsc_datas->id == id) {
      rr = 1;
    }
    i++;
  }/* wend */

  for(i=0;i<tsk_list_len;i++) {
     l = g_list_nth (data->tasksList, i);   
     tmp_tasks_datas = (tasks_data *)l->data;
     /* we test if for current task, the ressource 'ID' is a sender */
     link = links_check_element (tmp_tasks_datas->id, id, data);
     if(link>=0) {
        tmpVal = misc_correct_duration_with_periods (max_minutes, tmp_tasks_datas->start_nthDay, tmp_tasks_datas->start_nthMonth, 
                                    tmp_tasks_datas->start_nthYear, tmp_tasks_datas->days, tmp_tasks_datas->hours, 
                                    tmp_rsc_datas->fWorkMonday, tmp_rsc_datas->fWorkTuesday, 
                                    tmp_rsc_datas->fWorkWednesday, tmp_rsc_datas->fWorkThursday,
                                    tmp_rsc_datas->fWorkFriday, tmp_rsc_datas->fWorkSaturday,
                                    tmp_rsc_datas->fWorkSunday, tmp_tasks_datas->calendar, tmp_rsc_datas->calendar, data);
       ret = ret+tmpVal;
     }/* endif link */
  }/* next i */

  return ret;
}

/**********************************************
 LOCAL: draw a pie-chart about costs
 parameter : TRUE, sunday/monday, correction;
 you are responsible for 'duration'
 argument : if correct==TRUE, so "duration"
 should be corrected !
 returns a newly allocated GdkPixbuf
 which must be unref later
***********************************************/
static GdkPixbuf *report_draw_diagram (gint id, gdouble duration, gboolean correct, APP_data *data)
{
  GKeyFile *keyString;
  gchar *tmpStr, buffer[60];/* 12 chars coded in utf8 */
  GdkPixbuf *pixbuf2 = NULL;
  GList *l, *l2;
  tasks_data *tmp_tsk_datas;
  rsc_datas *tmp_rsc_datas;
  gint total, i, tsk_list_len, link, count_x, count_y, ret = 0, max_minutes, total_minutes;
  gdouble value, angle1, angle2, tmpVal;
  cairo_surface_t *surf = NULL;
  cairo_t *cr = NULL;
  cairo_text_extents_t extents;

  max_minutes = rsc_get_max_daily_working_time (id, data);
  /* we search ressource with current id */
  i = 0;
  while((i<g_list_length (data->rscList)) && (ret==0)) {
    l2 = g_list_nth (data->rscList, i); 
    tmp_rsc_datas = (rsc_datas *)l2->data;
    if(tmp_rsc_datas->id == id) {
      ret = 1;
    }
    i++;
  }/* wend */

  keyString = data->keystring;
  surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, REPORT_RSC_COST_WIDTH, REPORT_RSC_COST_HEIGHT);
  /* now we can create the context */
  cr = cairo_create (surf);
  cairo_set_source_rgb (cr, 0.5, 0.5, 0.5);
  cairo_set_line_width (cr, 1.5);
  cairo_arc (cr, REPORT_RSC_COST_WIDTH/2, REPORT_RSC_COST_HEIGHT/2, REPORT_RSC_COST_RADIUS, 0, 2*G_PI);
  cairo_fill (cr);
  /* origin */
  angle1 = G_PI * 3 / 2;
  angle2 = angle1;
  /* we draw all parts */
  tsk_list_len = g_list_length (data->tasksList);
  count_x = 0;
  count_y = 0;
  cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, 12);
  cairo_set_line_width (cr, 1);
  cairo_text_extents (cr, "Jp", &extents);
  /* beware of division by zero ! */
  if(duration>0) {
     for(i=0; i<tsk_list_len; i++) {
        l = g_list_nth (data->tasksList, i);
        tmp_tsk_datas = (tasks_data *)l->data;
        link = links_check_element (tmp_tsk_datas->id, id, data);
        if(link>=0) {
	  /* current part */
          if(correct) {
             tmpVal = misc_correct_duration_with_periods (max_minutes,
                         tmp_tsk_datas->start_nthDay, tmp_tsk_datas->start_nthMonth, 
                         tmp_tsk_datas->start_nthYear, tmp_tsk_datas->days, tmp_tsk_datas->hours,
                         tmp_rsc_datas->fWorkMonday, tmp_rsc_datas->fWorkTuesday, 
                         tmp_rsc_datas->fWorkWednesday, tmp_rsc_datas->fWorkThursday,
                         tmp_rsc_datas->fWorkFriday, tmp_rsc_datas->fWorkSaturday,
                         tmp_rsc_datas->fWorkSunday, tmp_tsk_datas->calendar, tmp_rsc_datas->calendar, data);
         //   printf("tâche %svaleur baturelle %d valeur corrigée =%.2f \n", tmp_tsk_datas->name, tmp_tsk_datas->days, tmpVal);
             value = tmpVal/duration; /* TODO : hours */
          }
          else {
            if(tmp_tsk_datas->days>0)
                value = tmp_tsk_datas->days/duration; /* TODO : hours */
          }
	  angle2 = angle2+(2*G_PI*value);
	  cairo_move_to (cr, REPORT_RSC_COST_WIDTH/2, REPORT_RSC_COST_HEIGHT/2);
          /* we convert gdk rGba to RGB */
	  cairo_set_source_rgb (cr, tmp_tsk_datas->color.red, tmp_tsk_datas->color.green, tmp_tsk_datas->color.blue);
	  cairo_arc (cr, REPORT_RSC_COST_WIDTH/2, REPORT_RSC_COST_HEIGHT/2, REPORT_RSC_COST_RADIUS, angle1, angle2);
	  cairo_move_to (cr, REPORT_RSC_COST_WIDTH/2, REPORT_RSC_COST_HEIGHT/2);
	  cairo_fill (cr);    
	  cairo_set_source_rgb (cr, 1, 1, 1);

	  cairo_move_to (cr, REPORT_RSC_COST_WIDTH/2, REPORT_RSC_COST_HEIGHT/2);
	  cairo_arc (cr, REPORT_RSC_COST_WIDTH/2, REPORT_RSC_COST_HEIGHT/2, REPORT_RSC_COST_RADIUS+1, angle1, angle2);
	  cairo_move_to (cr, REPORT_RSC_COST_WIDTH/2, REPORT_RSC_COST_HEIGHT/2);
	  cairo_stroke (cr);
          /* legend - when 'correct' only for value <>0 */ 
          if(value>0) {
             g_utf8_strncpy (&buffer, tmp_tsk_datas->name, 10);
	     cairo_set_source_rgb (cr, tmp_tsk_datas->color.red, tmp_tsk_datas->color.green, tmp_tsk_datas->color.blue);
             cairo_rectangle (cr, count_x*100, count_y*20, 16, 16);
             cairo_fill (cr);
             cairo_move_to (cr, count_x*100+20, count_y*20+extents.height);
             cairo_set_source_rgb (cr, g_key_file_get_double (keyString, "editor", "text.color.red", NULL), 
                                g_key_file_get_double (keyString, "editor", "text.color.green", NULL), 
                                g_key_file_get_double (keyString, "editor", "text.color.blue", NULL));
             cairo_show_text (cr, buffer); 
             count_x++;
             if(count_x>3) {
                count_x = 0;
                count_y++;
             }
          }/* endif value */
	  angle1 = angle2;
        }/* endif link */
     }/* next i */
  }/* endif duration */
  /* central white circle with duration*/
  if(!correct)
      total_minutes = rsc_get_max_daily_working_time (tmp_rsc_datas->id, data)*duration;
  else
      total_minutes = duration;

  tmpStr = g_strdup_printf (_("%.2dH%.2dm."), total_minutes/60, total_minutes % 60);
  report_draw_circle_text (cr,  g_key_file_get_double (keyString, "editor", "text.color.red", NULL), 
                                g_key_file_get_double (keyString, "editor", "text.color.green", NULL), 
                                g_key_file_get_double (keyString, "editor", "text.color.blue", NULL), tmpStr); 
  g_free (tmpStr);
     
  /* flush & draw */
  cairo_destroy (cr);
  pixbuf2 = gdk_pixbuf_get_from_surface (surf, 0, 0, REPORT_RSC_COST_WIDTH, REPORT_RSC_COST_HEIGHT);
  cairo_surface_destroy (surf);

  return pixbuf2;
}

/************************************
  PUBLIC : ressource cost
  analyssis, for ONE ressource
**********************************/
void report_unique_rsc_cost (gint id, APP_data *data)
{
  gchar *tmpStr;
  GtkTextIter start, end, iter;
  GdkPixbuf *pixbuf2;
  gint total, i, tsk_list_len, link, total_minutes;
  gdouble duration, duration_corrected, value, hours;
  GtkWidget *notebook;

   /* when the right-click happens, the convenient row is selected on "ressources" panel ; so, we know the ressource */
   report_clear_text (data->buffer, "left");
   misc_set_font_color_settings (data);
   gtk_text_buffer_get_iter_at_mark (data->buffer, &iter, gtk_text_buffer_get_insert (data->buffer));

   /* we get GtkTextIter in order to insert nex text */
   tmpStr = g_strdup_printf ("%s %s\n", _("Costs for Resource "), rsc_get_name (id, data));
   gtk_text_buffer_insert_with_tags_by_name (data->buffer, &iter, tmpStr, -1, "title", "underline", NULL);
   g_free (tmpStr);

   /* display avatar */
   pixbuf2 = rsc_get_avatar (id, data);
   if(pixbuf2) {
      gtk_text_buffer_insert_pixbuf (data->buffer, &iter, pixbuf2);
      g_object_unref (pixbuf2);
   }

   tmpStr = g_strdup_printf (_("\nThe current project has %d task(s)\n"), misc_count_tasks (data));
   gtk_text_buffer_insert (data->buffer, &iter, tmpStr, -1);
   g_free (tmpStr);

   total = rsc_get_number_tasks (id, data);
   tmpStr = g_strdup_printf (_("%s is assigned to %d task(s)\n"), rsc_get_name (id, data),
                                total );
   gtk_text_buffer_insert (data->buffer, &iter, tmpStr, -1);
   g_free (tmpStr);
  
   if(total<=0) {
      gtk_text_buffer_insert (data->buffer, &iter, _("No more operations - Resource hasn't any task !"), -1);
   }

   /* total worked hours  */
   duration = report_get_tasks_duration (id, data);
   hours = report_get_tasks_hours (id, data);
   total_minutes = rsc_get_max_daily_working_time (id, data)*duration+60*hours;

   /* total worked hours minus non-working days */
   duration_corrected = report_get_tasks_duration_corrected (id, data);
//   total_minutes = rsc_get_max_daily_working_time (id, data)*duration_corrected;/* replace TODO */
   total_minutes = report_get_tasks_duration_corrected_with_periods (id, data);

   tmpStr = g_strdup_printf (
        _("%s works (estimated excl. non-working days, using true effective schedule, clamped) %d hour(s) and %d minute(s)\n"), 
        rsc_get_name (id, data), total_minutes/60, total_minutes % 60);
   gtk_text_buffer_insert (data->buffer, &iter, tmpStr, -1);
   g_free (tmpStr);
   /* monetary cost */
   value = rsc_compute_cost (id, total_minutes, data);
   tmpStr = g_strdup_printf (_("%s costs (estimated excl. non-working days) : %s %.2f\n"), 
                                rsc_get_name (id, data), data->properties.units, value);
   gtk_text_buffer_insert (data->buffer, &iter, tmpStr, -1);
   g_free (tmpStr);   

   /* graphical representation : net datas */
   gtk_text_buffer_insert_with_tags_by_name (data->buffer, &iter, _("\nDiagram : net duration repartition\n"), -1, "bold", NULL);

   pixbuf2 = report_draw_diagram (id, total_minutes, TRUE, data);
   gtk_text_buffer_insert_pixbuf (data->buffer, &iter, pixbuf2);
   g_object_unref (pixbuf2);
   /* switch to report notebook page */
   notebook = GTK_NOTEBOOK(gtk_builder_get_object (data->builder, "notebook1"));
   gtk_notebook_set_current_page (GTK_NOTEBOOK(notebook), 6);
   /* set sensible menus and buttons */
   report_set_buttons_menus (TRUE, FALSE, data);
}

/****************************************************
 PROTECTED : draw old school diagram
 parameter : if TRUE, correct datas
 output : a GdkPixbuf, newly allocated,
to be unref -ed
the parameter 'duration' means duration OR 
duration_corrected, according to 'correct' parameter
*****************************************************/
static GdkPixbuf *report_old_school_diagram (gboolean correct, gdouble duration, APP_data *data )
{
  GKeyFile *keyString;
  gchar *tmpStr, buffer[60];
  GdkPixbuf *pixbuf2;
  GList *l;
  rsc_datas *tmp_rsc_datas;
  gdouble value;
  gint total, i, j, pos_x, pos_y, count_x, count_y;
  cairo_surface_t *surf = NULL;
  cairo_t *cr = NULL;
  cairo_text_extents_t extents;

  keyString =  data->keystring;
  /* drawing - an old good representation in a 10x10 square */
  total = g_list_length (data->rscList);
  pos_x = 0;
  pos_y = 0;
  count_x = 0;
  count_y = 0;
  surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, REPORT_RSC_COST_WIDTH, REPORT_RSC_COST_HEIGHT);
  /* now we can create the context & draw the grey square */
  cr = cairo_create (surf);
  cairo_set_source_rgb (cr, 0.84, 0.84, 0.84);
  cairo_set_line_width (cr, 1.5);
  cairo_rectangle (cr, 0, 120, REPORT_RSC_COST_WIDTH, REPORT_RSC_COST_HEIGHT-120);
  cairo_fill (cr);
  /* default size for text */
  cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, 12);
  cairo_set_line_width (cr, 1);
  cairo_text_extents (cr, "Jp", &extents);
  /* we do the job for all ressources */
  for(i=0; i<total; i++) {
    l = g_list_nth (data->rscList, i);
    tmp_rsc_datas = (rsc_datas *)l->data; 
    /* legend */
    g_utf8_strncpy (&buffer, tmp_rsc_datas->name, 10);
    cairo_set_source_rgb (cr, tmp_rsc_datas->color.red, tmp_rsc_datas->color.green, tmp_rsc_datas->color.blue);
    cairo_rectangle (cr, count_x*100, count_y*20, 16, 16);
    cairo_fill (cr);
    cairo_move_to (cr, count_x*100+20, count_y*20+extents.height);
    cairo_set_source_rgb (cr, g_key_file_get_double(keyString, "editor", "text.color.red", NULL), 
                                g_key_file_get_double(keyString, "editor", "text.color.green", NULL), 
                                g_key_file_get_double(keyString, "editor", "text.color.blue", NULL));
    cairo_show_text (cr, buffer); 
    count_x++;
    if(count_x>3) {
                count_x = 0;
                count_y++;
    }/* endif count_x */
    /* values & bars */
    cairo_set_source_rgb (cr, tmp_rsc_datas->color.red, tmp_rsc_datas->color.green, tmp_rsc_datas->color.blue);
    /* we compute the percents */
    if(correct) {
          value = (report_get_tasks_duration_corrected (tmp_rsc_datas->id, data)/duration )*100;/* value is in days !!! */
    }
    else {
          value = (report_get_tasks_duration (tmp_rsc_datas->id, data)/duration )*100;
    }
 printf ("la ressource %s occupe %d % (%.2f) écart =%.2f du projet \n", 
                   tmp_rsc_datas->name, (gint) value, value, (gdouble) value-(gint)value);
    for(j=0; j<(gint)value; j++) {
       cairo_rectangle (cr, pos_x*40, 120+pos_y*28, 38, 26);
       cairo_fill (cr);
       pos_x++;
       if(pos_x>9) {
         pos_x = 0;
         pos_y++;
       }
    }
  }

  /* flush & draw */
  cairo_destroy (cr);
  pixbuf2 = gdk_pixbuf_get_from_surface (surf, 0, 0, REPORT_RSC_COST_WIDTH, REPORT_RSC_COST_HEIGHT);
  cairo_surface_destroy (surf);
  return pixbuf2;
}

/******************************************
  LOCAL : ressource cost repartition
  for ALL ressources
  used a variable size array as argument
******************************************/
static GdkPixbuf *report_cost_repartition_diagram (gdouble table[], gint total, APP_data *data)
{
  GKeyFile *keyString;
  gchar *tmpStr, buffer[60];/* 12 chars coded in utf8 */
  GdkPixbuf *pixbuf2 = NULL;
  GList *l;
  rsc_datas *tmp_rsc_datas;
  gint i, count_x, count_y, ret = 0;
  gdouble sum = 0, value, angle1, angle2, tmpVal;
  cairo_surface_t *surf = NULL;
  cairo_t *cr = NULL;
  cairo_text_extents_t extents; 

  for(i=0;i<total;i++) {
     sum = sum+table[i];
  }
 
  keyString =  data->keystring;
  surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, REPORT_RSC_COST_WIDTH, REPORT_RSC_COST_HEIGHT);
  /* now we can create the context */
  cr = cairo_create (surf);
  cairo_set_source_rgb (cr, 0.5, 0.5, 0.5);
  cairo_set_line_width (cr, 1.5);
  cairo_arc (cr, REPORT_RSC_COST_WIDTH/2, REPORT_RSC_COST_HEIGHT/2, REPORT_RSC_COST_RADIUS, 0, 2*G_PI);
  cairo_fill (cr);
  /* origin */
  angle1 = G_PI * 3 / 2;
  angle2 = angle1;
  /* we draw all parts */
  count_x = 0;
  count_y = 0;
  cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, 12);
  cairo_set_line_width (cr, 1);
  cairo_text_extents (cr, "Jp", &extents);
  /* beware of division by zero ! */
  if(sum>0) {
     for(i=0; i<total; i++) {
        l = g_list_nth (data->rscList, i);
        tmp_rsc_datas = (rsc_datas *)l->data;
        /* current part */

          value = table[i]/sum;
	  angle2 = angle2+(2*G_PI*value);
	  cairo_move_to (cr, REPORT_RSC_COST_WIDTH/2, REPORT_RSC_COST_HEIGHT/2);
          /* we convert gdk rGba to RGB */
	  cairo_set_source_rgb (cr, tmp_rsc_datas->color.red, tmp_rsc_datas->color.green, tmp_rsc_datas->color.blue);
	  cairo_arc (cr, REPORT_RSC_COST_WIDTH/2, REPORT_RSC_COST_HEIGHT/2, REPORT_RSC_COST_RADIUS, angle1, angle2);
	  cairo_move_to (cr, REPORT_RSC_COST_WIDTH/2, REPORT_RSC_COST_HEIGHT/2);
	  cairo_fill (cr);    
	  cairo_set_source_rgb (cr, 1, 1, 1);

	  cairo_move_to (cr, REPORT_RSC_COST_WIDTH/2, REPORT_RSC_COST_HEIGHT/2);
	  cairo_arc (cr, REPORT_RSC_COST_WIDTH/2, REPORT_RSC_COST_HEIGHT/2, REPORT_RSC_COST_RADIUS+1, angle1, angle2);
	  cairo_move_to (cr, REPORT_RSC_COST_WIDTH/2, REPORT_RSC_COST_HEIGHT/2);
	  cairo_stroke (cr);
          /* legend - when 'correct' only for value <>0 */ 
          if(value>0) {
             g_utf8_strncpy (&buffer, tmp_rsc_datas->name, 10);
	     cairo_set_source_rgb (cr, tmp_rsc_datas->color.red, tmp_rsc_datas->color.green, tmp_rsc_datas->color.blue);
             cairo_rectangle (cr, count_x*100, count_y*20, 16, 16);
             cairo_fill (cr);
             cairo_move_to (cr, count_x*100+20, count_y*20+extents.height);
             cairo_set_source_rgb (cr, g_key_file_get_double (keyString, "editor", "text.color.red", NULL), 
                                g_key_file_get_double (keyString, "editor", "text.color.green", NULL), 
                                g_key_file_get_double (keyString, "editor", "text.color.blue", NULL));
             cairo_show_text (cr, buffer); 
             count_x++;
             if(count_x>3) {
                count_x = 0;
                count_y++;
             }
          }/* endif value */
	  angle1 = angle2;

     }/* next i */
  }/* endif sum */
  /* central white circle with duration*/
  tmpStr = g_strdup_printf (_("%.3f %s"), sum, data->properties.units);
  report_draw_circle_text (cr,  g_key_file_get_double (keyString, "editor", "text.color.red", NULL), 
                                g_key_file_get_double (keyString, "editor", "text.color.green", NULL), 
                                g_key_file_get_double (keyString, "editor", "text.color.blue", NULL), tmpStr);
  g_free (tmpStr);
  /* flush & draw */
  cairo_destroy (cr);
  pixbuf2 = gdk_pixbuf_get_from_surface (surf, 0, 0, REPORT_RSC_COST_WIDTH, REPORT_RSC_COST_HEIGHT);
  cairo_surface_destroy (surf);
  return pixbuf2;
}

/************************************
  PUBLIC : ressource cost
  analyssis, for ALL ressources
************************************/
void report_all_rsc_cost (APP_data *data)
{
  gchar *tmpStr;
  GtkTextIter start, end, iter;
  GdkPixbuf *pixbuf2;
  GList *l;
  rsc_datas *tmp_rsc_datas;
  gdouble day_duration, day_duration_corrected, cost, cost_corrected;
  gint total, i, gross_minutes, gross_total_minutes_corrected, minutes_part_corrected, minutes_full_corrected;
  gint max_day, effective_total_minutes;
  GtkWidget *notebook;

   /* test is there is tasks and/or ressources */
   report_clear_text (data->buffer, "left");
   gtk_text_buffer_get_iter_at_mark (data->buffer, &iter, gtk_text_buffer_get_insert (data->buffer));
   if(!report_is_ready_to_report (data)) {
      gtk_text_buffer_insert_with_tags_by_name (data->buffer, 
                &iter, _("No datas for computations !"), -1, "title", "underline", NULL);
      return;
   }
   misc_set_font_color_settings (data);
   /* we get GtkTextIter in order to insert nex text */
   total = g_list_length (data->rscList);
   tmpStr = g_strdup_printf ( _("Cost for the %d Resources \n\n"), total);
   gtk_text_buffer_insert_with_tags_by_name (data->buffer, &iter, tmpStr, -1, "title", "underline", NULL);
   g_free (tmpStr);

   tmpStr = g_strdup_printf ("%s", _("Please Note !\nDatas are ONLY estimated costs by durations, and converted to money.\nBe careful when you use them.\n\n"));
   gtk_text_buffer_insert_with_tags_by_name (data->buffer, &iter, tmpStr, -1, "italic", "underline", NULL);
   g_free (tmpStr);
   /* table of each ressources' cost */
   tmpStr = g_strdup_printf (_("Table 1 of 2 : Cost per resource (excl. non-working days, clamped), %s \n\n"), 
                              data->properties.units);
   gtk_text_buffer_insert_with_tags_by_name (data->buffer, &iter, tmpStr, -1, "bold", "underline", NULL);
   g_free (tmpStr);
   /* pseudo-dynamic allocation of a table to store intermediate values */
   gdouble t_cost [total];
   /* we compute total worked hours for whole project */
   day_duration = 0;
   day_duration_corrected = 0;
   gross_minutes = 0;
   gross_total_minutes_corrected = 0;
   effective_total_minutes = 0;
   cost = 0;
   cost_corrected = 0;
   for(i=0; i<total; i++) {// TODO revoir
       l = g_list_nth (data->rscList, i);
       tmp_rsc_datas = (rsc_datas *)l->data;
       max_day = rsc_get_max_daily_working_time (tmp_rsc_datas->id, data);
       /* gross values */
       gross_minutes = gross_minutes+max_day*report_get_tasks_duration (tmp_rsc_datas->id, data);
       day_duration = day_duration + report_get_tasks_duration (tmp_rsc_datas->id, data);
       /* partially corrected */
       minutes_part_corrected = max_day*report_get_tasks_duration_corrected (tmp_rsc_datas->id, data);/* we remove n-working days from rsc and tasks calendars */
       /* completely corrected */
       minutes_full_corrected  = report_get_tasks_duration_corrected_with_periods (tmp_rsc_datas->id, data);
       gross_total_minutes_corrected = gross_total_minutes_corrected+minutes_part_corrected;
       effective_total_minutes = effective_total_minutes+minutes_full_corrected;
       day_duration_corrected = day_duration_corrected + report_get_tasks_duration_corrected (tmp_rsc_datas->id, data);
       cost = cost+rsc_compute_cost (tmp_rsc_datas->id, minutes_part_corrected, data);
       gdouble tmp_cost = rsc_compute_cost (tmp_rsc_datas->id, minutes_full_corrected, data);
       cost_corrected = cost_corrected+tmp_cost;
       /* individual costs */
       tmpStr = g_strdup_printf (_("\t%s\t%.3f\n"), tmp_rsc_datas->name, tmp_cost);
       gtk_text_buffer_insert (data->buffer, &iter, tmpStr, -1);
       g_free (tmpStr);
       t_cost [i] = tmp_cost;
printf("rsc=%s durée corrigée=%d  complet corrigée =%d \n", tmp_rsc_datas->name, minutes_part_corrected, minutes_full_corrected);
   }
printf("\n---------\ncout total estimé projet = %.2f totalement corrigé =%.2f\n", cost, cost_corrected);

   /* durations */
   tmpStr = g_strdup_printf ("%s", _("\nTable 2 of 2 : global datas\n"));
   gtk_text_buffer_insert_with_tags_by_name (data->buffer, &iter, tmpStr, -1, "bold", "underline", NULL);
   g_free (tmpStr);

   tmpStr = g_strdup_printf (_("\tDuration : %d hour(s) and %d minute(s) (excl. non-working days, clamped).\n"),
                                effective_total_minutes/60, effective_total_minutes % 60);
   gtk_text_buffer_insert (data->buffer, &iter, tmpStr, -1);
   g_free (tmpStr);
   /* monetary cost */
   tmpStr = g_strdup_printf (_("\tCost excl. non-working days, clamped :\t%.3f\n\n"), cost_corrected);
   gtk_text_buffer_insert (data->buffer, &iter, tmpStr, -1);
   g_free (tmpStr);  

   /* 1st diagram, cost repartition fully corrected */
   tmpStr = g_strdup_printf ("%s", _("Diagram 1 of 2 : repartition of monetary cost (excl. non-working days, clamped). \n\n"));
   gtk_text_buffer_insert_with_tags_by_name (data->buffer, &iter, tmpStr, -1, "bold", "underline", NULL);
   g_free (tmpStr);

   pixbuf2 = report_cost_repartition_diagram (t_cost, total, data);
   gtk_text_buffer_insert_pixbuf (data->buffer, &iter, pixbuf2);
   g_object_unref (pixbuf2);

   /* 3rd diagram, corrected */ // TODO améliorer pour heures
   tmpStr = g_strdup_printf ("%s", _("\n\nDiagram 2 of 2 : repartition of worked days (excl. non-working days). \n\n"));
   gtk_text_buffer_insert_with_tags_by_name (data->buffer, &iter, tmpStr, -1, "bold", "underline", NULL);
   g_free (tmpStr);
   pixbuf2 = report_old_school_diagram (TRUE, day_duration_corrected, data);
   gtk_text_buffer_insert_pixbuf (data->buffer, &iter, pixbuf2);
   g_object_unref (pixbuf2);
   /* switch to report notebook page */
   notebook = GTK_WIDGET(gtk_builder_get_object (data->builder, "notebook1"));
   gtk_notebook_set_current_page (GTK_NOTEBOOK(notebook), 6);
   /* set sensible menus and buttons */
   report_set_buttons_menus (TRUE, TRUE, data);
}

/******************************
  PUBLIC : export content of 
  report page to rtf
******************************/
void report_export_report_to_rtf (APP_data *data)
{
  GKeyFile *keyString;
  gchar *filename, *tmpFileName;
  gint ret;
  GtkFileFilter *filter = gtk_file_filter_new ();
  GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
  GtkWidget *alertDlg;

  gtk_file_filter_add_pattern (filter, "*.rtf");
  gtk_file_filter_set_name (filter, _("Word Processors RTF files"));

  keyString = data->keystring;

  GtkWidget *dialog = create_saveFileDialog (_("Save current report to a Rich Text Format file"), data);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir());
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  /* run dialog for file selection */
  if(gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    if(!g_str_has_suffix (filename, ".rtf" ) && !g_str_has_suffix (filename, ".RTF" )) {
         /* we correct the filename */
         tmpFileName = g_strdup_printf ("%s.rtf", filename);
         g_free (filename);
         filename = tmpFileName;
    }/* endif extension XML*/
    /* we must check if a file with same name exists */
    if(g_file_test (filename, G_FILE_TEST_EXISTS) 
        && g_key_file_get_boolean (keyString, "application", "prompt-before-overwrite",NULL )) {
           /* dialog to avoid overwriting */
           alertDlg = gtk_message_dialog_new (GTK_WINDOW(dialog),
                          flags,
                          GTK_MESSAGE_ERROR,
                          GTK_BUTTONS_OK_CANCEL,
                          _("The file :\n%s\n already exists !\nDo you really want to overwrite this RTF file ?"),
                          filename);
           if(gtk_dialog_run (GTK_DIALOG(alertDlg))==GTK_RESPONSE_CANCEL) {
              gtk_widget_destroy (GTK_WIDGET(alertDlg));
              gtk_widget_destroy (GTK_WIDGET(dialog));
              g_free (filename);
              return;
           }
    }/* endif file exists */
    ret = save_RTF_rich_text (filename, data);
    if(ret!=0) {
        gchar *msg = g_strdup_printf ("%s", _("Sorry !\nAn error happened during file exporting.\nOperation aborted. You can try again, or\ncheck your system."));
        misc_ErrorDialog (data->appWindow, msg);
        g_free (msg);
    }
    g_free (filename);
  }

  gtk_widget_destroy (GTK_WIDGET(dialog)); 
}

/********************************************
 * protected : test overload for a resource
 * arguments :
 * data_app structure
 * id of resource to test
 * output : TRUE if overload
 * *****************************************/
 
static void report_test_rsc_overload (gint id, GtkTextIter *iter, APP_data *data)
 {
	 gboolean coincStart, coincEnd;
	 gint i = 0, rc = -1, link, j, id1, id2;
	 gint cmpst1, cmpst2, cmped1, cmped2;
	 GList *l = NULL;
	 GList *l2 = NULL;	 
     tasks_data *tmp_tsk_datas;
     rsc_datas *tmp_rsc;
     assign_data *tmp_assign_datas, *tmp_assign2;
     GDate date_task_start, date_task_end, start, end;
     gchar *tmpStr;

     /* we do a loop to find every tasks where resource 'id' is used and we build a GList*/
     for( i = 0; i < g_list_length (data->tasksList); i++) {
	    l = g_list_nth (data->tasksList, i);
        tmp_tsk_datas = (tasks_data *)l->data;
        /* now we check if the resource 'id' is used by current task ; if Yes, we upgrade Glist */
        link = links_check_element (tmp_tsk_datas->id, id, data);
        if(link >= 0) {
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
     if(g_list_length (l2)>=2) {
	     /* now we do loop on list in order to test oveloads */

	     for( i = 0; i < g_list_length (l2); i++) {
		    l = g_list_nth (l2, i);
            tmp_assign_datas = (assign_data *)l->data;
            /* now we do a while/wend loop in order to check an oveleao */
            /* it's a decreasing loop, because we are indexed on 'i'    */
            /* we use date at rank i as reference                       */
            g_date_set_dmy (&start, tmp_assign_datas->s_day, tmp_assign_datas->s_month, tmp_assign_datas->s_year);
            g_date_set_dmy (&end, tmp_assign_datas->e_day, tmp_assign_datas->e_month, tmp_assign_datas->e_year);
            /* now we will compare to any date at rank AFTER i */
            j = i+1;
            id1 = tmp_assign_datas->task_id;
            
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
				  /* here there is an overload, we have to display the conflict between tasks */
				  /* first, we get names of the tasks */
				  id2 = tmp_assign2->task_id;
				//  printf ("la ressource est en conflit pour les tâches %s et %s \n",
				  //         tasks_get_name_for_id (id1, data),
				    //       tasks_get_name_for_id (id2, data)
				      //   );
				  tmpStr = g_strdup_printf (_("- overload on tasks : %s,  %s \n"), tasks_get_name_for_id (id1, data),
				                                                                    tasks_get_name_for_id (id2, data) );
                  gtk_text_buffer_insert (data->buffer, iter, tmpStr, -1);
                  g_free (tmpStr);        
			   }
			   j++;	
		    }/* wend */
            
	     }/* nexr i */
	 }/* endif */
	 
     /* we free datas */
     l2 = g_list_first (l2);
     for(l2 ; l2 != NULL; l2 = l2->next) {
        tmp_assign_datas = (assign_data *)l2->data;
        g_free (tmp_assign_datas);
     }/* next l2*/
     
     /* free the Glist itself */
     g_list_free (l2);     

 }



/************************************
  PUBLIC : ressource overload
  analyssis, for ONE resource
  * arguments : ID of resource,
  * DATA_app structure
**********************************/
void report_unique_rsc_overload (gint id, APP_data *data)
{
  gchar *tmpStr;
  GtkTextIter start, end, iter;
  GdkPixbuf *pixbuf2;
  gint total, i, tsk_list_len, link, total_minutes;
  gdouble duration, duration_corrected, value, hours;
  GtkWidget *notebook;

   /* when the right-click happens, the convenient row is selected on "ressources" panel ; so, we know the ressource */
   report_clear_text (data->buffer, "left");
   misc_set_font_color_settings (data);
   gtk_text_buffer_get_iter_at_mark (data->buffer, &iter, gtk_text_buffer_get_insert (data->buffer));

   /* we get GtkTextIter in order to insert nex text */
   tmpStr = g_strdup_printf ("%s %s\n", _("Overload for Resource : "), rsc_get_name (id, data));
   gtk_text_buffer_insert_with_tags_by_name (data->buffer, &iter, tmpStr, -1, "title", "underline", NULL);
   g_free (tmpStr);

   /* wexplanation for usert */
   tmpStr = g_strdup_printf ("%s\n", _("There is an overload when a resource is used at same time by at least 2 tasks."));
   gtk_text_buffer_insert_with_tags_by_name (data->buffer, &iter, tmpStr, -1, "italic", NULL);
   g_free (tmpStr);

   /* display avatar */
   pixbuf2 = rsc_get_avatar (id, data);
   if(pixbuf2) {
      gtk_text_buffer_insert_pixbuf (data->buffer, &iter, pixbuf2);
      g_object_unref (pixbuf2);
   }

   tmpStr = g_strdup_printf (_("\nThe current project has %d task(s)\n"), misc_count_tasks (data));
   gtk_text_buffer_insert (data->buffer, &iter, tmpStr, -1);
   g_free (tmpStr);

   total = rsc_get_number_tasks (id, data);
   tmpStr = g_strdup_printf (_("%s is assigned to %d task(s)\n\n"), rsc_get_name (id, data),
                                total );
   gtk_text_buffer_insert (data->buffer, &iter, tmpStr, -1);
   g_free (tmpStr);
   /* now we do loop in order to display all tasks where current resource is in overload */
   report_test_rsc_overload (id, &iter, data);
 
   /* switch to report notebook page */
   notebook = GTK_WIDGET(gtk_builder_get_object (data->builder, "notebook1"));
   gtk_notebook_set_current_page (GTK_NOTEBOOK(notebook), 6);
   /* set sensible menus and buttons */
   report_set_buttons_menus (TRUE, FALSE, data);
}
