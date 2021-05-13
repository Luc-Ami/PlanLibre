#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>

/* translations */
#include <libintl.h>
#include <locale.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <glib/gstdio.h> /* g_fopen, etc */
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include "support.h"
#include "misc.h"
#include "interface.h"
#include "callbacks.h"
#include "ressources.h"
#include "timeline.h"
#include "dashboard.h"
#include "assign.h"
#include "home.h"
#include "report.h"

/* pixmaps */


/*********************************
  setup default values for display
for paddings and removing inner border from entries, many thanks to them : https://github.com/jy2wong/luakit/commit/6dfffb296f562b26c1eb6020c94b1d3e0bde336b
anf for a clean demo for gtktextview tuning : https://gist.github.com/mariocesar/f051ce1dda4ef0041aed

Explnation for changes since Gtk 3.20 here : https://stackoverflow.com/questions/37134276/mono-fonts-for-all-gtktextview-with-css-file-with-gtk-3-20
 cool website to test gradients : https://cssgradient.io/
" #notebook1 tab:focus  { background-image:none; background-color: #4285F4; color: white; } \n" 
************************************/
void set_up_view (GtkWidget *window1, APP_data *data )
{
  guint major, minor;

  GtkCssProvider* css_provider = gtk_css_provider_new();

  /* check Gtk 3.x version */
  major = gtk_get_major_version ();
  minor = gtk_get_minor_version ();

  if(minor>=20) {
	const char css[] = 
	"  menu .menuitem {   padding: 4px 0px; }\n"
        " #eventboxDashWait { border-radius: 4px; background-color : #FBBC04; }\n"
        " #eventboxDashRun { border-radius: 4px; background-color : #4285F4;  color: white; }\n"
        " #eventboxDashDelay { border-radius: 4px; background-color : #EA4335; color: white; }\n"
        " #eventboxDashDone { border-radius: 4px; background-color : #33A752;  color: white; }\n"
	"  .list-row  frame { border-radius : 7px; border-width: 2px; background-color : white; border-color : #ECECEC; color: #4A4A4A; }\n";
        gtk_css_provider_load_from_data (css_provider,css,-1,NULL);
  } 
  else {
	const char css[] =
"  #togNW, #togHO, #togWE, #togAW, #togPO {  border-image: none; border-radius: 40px; }\n"
"  #togNW:active, #togHO:active, #togWE:active, #togAW:active, #togPO:active { border-image: none; border-radius: 40px; background-image:none; background-color: yellow; color: black; }\n"
"  #calendar1:selected { background-color : #4285F4;  color:white; }\n"
// "  #calendar1.view {  border-color: #398ee7; border-radius: 2; border-style: solid; border-width: 1; padding: 2; }\n"
"  #calendar1.highlight { background-color: #398ee7; color: white; font-weight: bold; }\n"
//"  #calendar1.marked { background-color : #4285F4;  color:white; }\n"
"  GtkMenu .menuitem {   padding: 4px 0px; }\n"
" #view { background : #F5F5F5; color: #000000; }\n"
" #frameSearch { border-radius: 4px; }\n"
"  #gridDisplay { background-image: none; background-color: #FFFFFF; }\n"
"  #gridTitle {  background-image: none; background-color:#E9E9E6; color: black; }\n"
"  #recentGrid {  border-style: none; border-width: 5px;  border-radius: 5px;}\n"
"  #myButtonCancel{  font-family: DejaVu Sans;font-style: normal; border-radius: 3px; background-image: none;}\n"
"  #myButtonCancel:hover { background-color: red; }\n"
"  #myButton_label_title_1, #myButton_label_title_2, #myButton_label_title_3, #myButton_label_title_4, #myButton_label_title_5, #myButton_label_title_6, #myButton_label_title_7, #myButton_label_title_8, #myButton_label_title_9, #myButton_label_title_10 {  border-top-left-radius: 8px; border-top-right-radius: 8px; background-color: #6F7DC8; color: white; box-shadow: 2px 2px 4px #888888; }\n"
"  #myButton_label_content_1, #myButton_label_content_2, #myButton_label_content_3, #myButton_label_content_4, #myButton_label_content_5, #myButton_label_content_6, #myButton_label_content_7, #myButton_label_content_8, #myButton_label_content_9, #myButton_label_content_10 { border-radius: 8px; background-color: white; color: black; box-shadow: 2px 2px 4px #888888;}\n"
"  #myButton_1, #myButton_2, #myButton_3, #myButton_4, #myButton_5, #myButton_6, #myButton_7, #myButton_8, #myButton_9, #myButton_10{ color: black; font-family: DejaVu Sans; font-style: normal; border-radius: 8px;  border-image: none;  padding: 0px 0px;}\n"
"  #myButton_1:hover, #myButton_2:hover, #myButton_3:hover, #myButton_4:hover, #myButton_5:hover, #myButton_6:hover, #myButton_7:hover, #myButton_8:hover, #myButton_9:hover, #myButton_10:hover{  border-color: green; border-style: solid; border-image: none; border-top: none; border-left: none; border-right: none; border-width: 4px;}\n"
"  #myButton_1:focus, #myButton_2:focus, #myButton_3:focus, #myButton_4:focus, #myButton_5:focus, #myButton_6:focus, #myButton_7:focus, #myButton_8:focus, #myButton_9:focus, #myButton_10:focus{  border-style: solid; border-color:orange; border-image: none; border-top: none; border-left: none; border-right: none; border-width: 4px;}\n"
" #eventboxHomeBack  { border-radius: 2px; background-image: linear-gradient(38deg, rgba(50,7,79,1) 0%, rgba(204,128,21,1) 100%);  color: white; }\n"
" #eventboxHomeDone { border-radius: 4px; background-color : #33A752;  color: white; }\n"
" #eventHomeRunning { border-radius: 4px; background-color : #4285F4;  color: white; }\n"
" #eventHomeStartToday { border-radius: 4px; background-color : #1AB2AB; }\n"
" #eventHomeOverdue { border-radius: 4px; background-color : #EA4335; color: white; }\n"
" #eventboxMileStone { border-radius: 4px; background-color : #604E84; color: white; }\n"
" #eventHomeTomorrow { border-radius: 4px; background-color : #8214AE;  color: white; }\n"
" #eventboxDashWait { border-radius: 4px; background-color : #FBBC04; }\n"
" #eventboxDashRun { border-radius: 4px; background-color : #4285F4; color: white; }\n"
" #eventboxDashDelay { border-radius: 4px; background-color : #EA4335; color: white; }\n"
" #eventboxDashDone { border-radius: 4px; background-color : #33A752;  color: white; }\n"
" #eventboxToGo { border-radius: 4px; background-color : #FBBC04; color: white; }\n"
"  .list-row  GtkFrame { border-radius : 7px; border-width: 2px; background-color : white; border-color : #ECECEC; color: #4A4A4A; }\n";
        gtk_css_provider_load_from_data (css_provider, css, -1, NULL);
  }

 /*----- css *****/
  GdkScreen* screen = gdk_screen_get_default ();
  gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

/********************************
  check if we have a dark or 
  light theme
*********************************/
void check_up_theme (GtkWidget *window1, APP_data *data_app)
{
  GdkRGBA fg, bg;
  gdouble textAvg, bgAvg;
 
  GdkScreen* screen = gdk_screen_get_default ();
  GtkStyleContext* style_context = gtk_widget_get_style_context (window1);

 /* check if we have a drak or light theme idea from : 
https://lzone.de/blog/Detecting%20a%20Dark%20Theme%20in%20GTK  
*/
  gtk_style_context_get_color (style_context, GTK_STATE_FLAG_NORMAL, &fg);
  gtk_style_context_get_background_color (style_context, GTK_STATE_FLAG_NORMAL, &bg);

  textAvg = fg.red+fg.green+fg.blue;
  bgAvg = bg.red+bg.green+bg.blue;

  if (textAvg > bgAvg)  {
     data_app->fDarkTheme=TRUE;
  }
  else {
     data_app->fDarkTheme=FALSE;
  }
}

/*****************************
 retrieve theme's selection
 color
****************************/
void get_theme_selection_color (GtkWidget *widget)
{
  GdkRGBA  color1, color2;

  GdkScreen* screen = gdk_screen_get_default ();
  GtkStyleContext* style_context = gtk_widget_get_style_context (widget);

  gtk_style_context_get (style_context, GTK_STATE_FLAG_SELECTED,
                       "background-color", &color1, NULL);

  gtk_style_context_lookup_color (style_context, "focus_color", &color2);
  
}
/*****************************
 test Css configuration for
 a widget
******************************/
static gboolean widget_is_dark (GtkWidget *widget) 
{
  GdkRGBA fg, bg;
  gdouble textAvg;
 
  GdkScreen* screen = gdk_screen_get_default();
  GtkStyleContext* style_context = gtk_widget_get_style_context (widget);

  /* check the TEXT color is the best way, since a background can be an image, not a text !*/
  gtk_style_context_get_color (style_context, GTK_STATE_FLAG_NORMAL, &fg);
  
  textAvg = fg.red+fg.green+fg.blue;

  if (textAvg >1.5)  
     return TRUE;
  else 
     return FALSE;
}

/*************************************
  PUBLIC About dialog

*************************************/
GtkWidget *create_aboutPlanlibre (APP_data *data_app)
{
  GtkWidget *dialog;
  GdkPixbuf *about_icon_pixbuf;
  const gchar *authors[] = {_("Project Manager:"),"Luc A. <amiluc_bis@yahoo.fr>", NULL };
  const gchar *artists[] = {"Luc A. <amiluc_bis@yahoo.fr>", _("PlanLibre logos + main icon :"), "Luc A. <amiluc_bis@yahoo.fr>", NULL};

  dialog = gtk_about_dialog_new ();
  gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG(dialog),"");
  about_icon_pixbuf = create_pixbuf ("splash.png");
  if(about_icon_pixbuf) {
      gtk_about_dialog_set_logo (GTK_ABOUT_DIALOG(dialog), about_icon_pixbuf);
      g_object_unref (G_OBJECT(about_icon_pixbuf));
  }
  //gtk_about_dialog_set_version (GTK_ABOUT_DIALOG(dialog),"");
  gtk_about_dialog_set_copyright (GTK_ABOUT_DIALOG(dialog),"");
  gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG(dialog), 
     _("Planning utility written in GTK+ and licensed under GPL v.3"));
  gtk_about_dialog_set_website (GTK_ABOUT_DIALOG(dialog), 
      "https://github.com/Luc-Ami/PlanLibre/wiki");
  gtk_about_dialog_set_license_type (GTK_ABOUT_DIALOG(dialog), GTK_LICENSE_GPL_3_0);

  gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG(dialog), authors);
  gtk_about_dialog_set_artists (GTK_ABOUT_DIALOG(dialog), artists);
  gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(dialog), _("Translators credits"));

  gtk_window_set_transient_for (GTK_WINDOW (dialog),
                               GTK_WINDOW(data_app->appWindow));                               
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_type_hint (GTK_WINDOW (dialog), GDK_WINDOW_TYPE_HINT_DIALOG);
  gtk_widget_show_all (dialog);
  return dialog;
}

/**************************************
  PlanLibre preparation for a standard
  g_application
**************************************/
void planlibre_prepare_GUI (GApplication *app, APP_data *data)
{
  GtkWidget *mainWindow, *image, *comboTL;
  GdkPixbuf *pix;
  GError *err = NULL;

  mainWindow = GTK_WIDGET(gtk_builder_get_object (data->builder, "window_main"));
  data->appWindow = mainWindow;
  gtk_window_set_application (GTK_WINDOW(mainWindow), GTK_APPLICATION(app));
  /* gtkbuilder can manage - if I understood - only widgets INSIDE the g_application, converselly of standard gtk_windows 
   requires a spécific option in src/Makefile.am 
     planlibre_LDFLAGS = \
	-Wl,--export-dynamic
   */
  gtk_builder_connect_signals (data->builder, data);/* now all calls will get data structure as first parameter */

  gtk_window_set_icon_name (GTK_WINDOW (data->appWindow), "planlibre");

  set_up_view (mainWindow, data);
  
  /* change some icons */
  /* home */
  image = GTK_WIDGET(gtk_builder_get_object (data->builder, "imgHome"));
  pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("home.svg"), 32, 32, &err);
  gtk_image_set_from_pixbuf (GTK_IMAGE(image), pix);

  /* timeline */
  image = GTK_WIDGET(gtk_builder_get_object (data->builder, "imgTimeLine"));
  pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("timeline.svg"), 32, 32, &err);
  gtk_image_set_from_pixbuf (GTK_IMAGE(image), pix);

  /* tasks */
  image = GTK_WIDGET(gtk_builder_get_object (data->builder, "imgTasks"));
  pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("task.svg"), 32, 32, &err);
  gtk_image_set_from_pixbuf (GTK_IMAGE(image), pix);

  /* ressources */
  image = GTK_WIDGET(gtk_builder_get_object (data->builder, "imgRsc"));
  pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("rsc.svg"), 32, 32, &err);
  gtk_image_set_from_pixbuf (GTK_IMAGE(image), pix);

  /* dashboard */
  image = GTK_WIDGET(gtk_builder_get_object (data->builder, "imgDash"));
  pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("dashboard.svg"), 32, 32, &err);
  gtk_image_set_from_pixbuf (GTK_IMAGE(image), pix);

  /* assignments */
  image = GTK_WIDGET(gtk_builder_get_object (data->builder, "imgAssign"));
  pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("assign.svg"), 32, 32, &err);
  gtk_image_set_from_pixbuf (GTK_IMAGE(image), pix);

  /* report */
  image = GTK_WIDGET(gtk_builder_get_object (data->builder, "imgReports"));
  pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("report.svg"), 32, 32, &err);
  gtk_image_set_from_pixbuf (GTK_IMAGE(image), pix);


  image = GTK_WIDGET(gtk_builder_get_object (data->builder, "imageAssignPDF"));
  pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("pdf_logo.png"), 16, 16, &err);
  gtk_image_set_from_pixbuf (GTK_IMAGE(image), pix);

  image = GTK_WIDGET(gtk_builder_get_object (data->builder, "imageTLPDF"));
  gtk_image_set_from_pixbuf (GTK_IMAGE(image), pix);
  g_object_unref (pix);

  image = GTK_WIDGET(gtk_builder_get_object (data->builder, "imageGroup"));
  pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("group.png"), 20, 20, &err);
  gtk_image_set_from_pixbuf (GTK_IMAGE(image), pix);
  g_object_unref (pix);

  image = GTK_WIDGET(gtk_builder_get_object (data->builder, "imageMilestone"));
  pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("milestone.png"), 20, 20, &err);
  gtk_image_set_from_pixbuf (GTK_IMAGE(image), pix);
  g_object_unref (pix);

  image = GTK_WIDGET(gtk_builder_get_object (data->builder, "imageTimer"));
  pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("tomatotimer.png"), 20, 20, &err);
  gtk_image_set_from_pixbuf (GTK_IMAGE(image), pix);
  g_object_unref (pix);

  /* default zoom level - 2 == 100%*/
  comboTL = GTK_WIDGET(gtk_builder_get_object (data->builder, "comboboxTLzoom"));
  gtk_combo_box_set_active (GTK_COMBO_BOX(comboTL), 2);
  /* create canvas */
  timeline_canvas (GTK_WIDGET(gtk_builder_get_object (data->builder, "scrolledwindowTimeline")), data);
  assign_canvas (GTK_WIDGET(gtk_builder_get_object (data->builder, "scrolledwindowAsst")), data);

  /* prepare report window */
  report_init_view (data);

}
