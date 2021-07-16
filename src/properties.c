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
#include "properties.h"
#include "timeline.h"
#include "dashboard.h"
#include "undoredo.h"
#include "tasksutils.h"
#include "home.h"
#include "files.h"
#include "savestate.h"

/*#########################

  functions 

#########################*/


/*******************************
  PUBLIC : format time display

*******************************/

/***************************************
  PUBLIC : change formating of minutes
  inspired by Gnome web site
***************************************/
gchar *on_scale_output (GtkScale *scale, gdouble  value, APP_data *data)
{
   gint val = (gint) value;
   return g_strdup_printf ("%02d:%02d", val/60, val %60);
}


void on_scaleStart_value_changed (GtkRange *range, APP_data *data)
{
  gdouble value;
  gint val, val2;
  GtkWidget *endRange;

  value = gtk_range_get_value (range);
  val = (gint) value;
  /* constraint : start value< end value */
  endRange = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "scaleEnd"));
  val2 = (gint) gtk_range_get_value (GTK_RANGE(endRange));
  if(val>val2)
     gtk_range_set_value (range, val2-1);
}

void on_scaleEnd_value_changed (GtkRange *range, APP_data *data)
{
  gdouble value;
  gint val, val2;
  GtkWidget *startRange;

  value = gtk_range_get_value (range);
  val = (gint) value;
  /* constraint : end value> start value */
  startRange = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "scaleStart"));
  val2 = (gint) gtk_range_get_value (GTK_RANGE(startRange));
  if(val<val2)
     gtk_range_set_value (range, val2+1);
}

/*****************************
  callback from Builder
  get new start project date
****************************/
void on_IntervalStartBtn_clicked  (GtkButton  *button,  APP_data *data)
{
  GDate date_interval1, date_interval2;
  gboolean fErrDateInterval1 = FALSE;
  gboolean fErrDateInterval2 = FALSE;

  GtkWidget *dateEntry = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonPropertiesStartDate"));
  GtkWidget *endEntry = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonPropertiesEndDate"));
  GtkWidget *win = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "dialogProperties"));
  gchar* newDate = misc_getDate(gtk_button_get_label(GTK_BUTTON(dateEntry)), win);
  gtk_button_set_label (GTK_BUTTON(dateEntry), newDate);

  /* we must check if dates aren't inverted */

  const gchar *endDate=NULL;
  endDate = gtk_button_get_label(GTK_BUTTON(endEntry));
  g_date_set_parse (&date_interval1, newDate);
  g_date_set_parse (&date_interval2, endDate);
  fErrDateInterval1 = g_date_valid (&date_interval1);
  if(!fErrDateInterval1)
      printf("Internal erreor date Interv 1\n");
  fErrDateInterval2 = g_date_valid (&date_interval2);  
  if(!fErrDateInterval2)
      printf("Internal error date Interv 2\n");
  gint cmp_date = g_date_compare (&date_interval1,&date_interval2);
  if(cmp_date>0)
    {
       misc_ErrorDialog (win, _("<b>Error!</b>\n\nDates mismatch ! 'Starting' date must be <i>before</i> 'Ending' date.\n\nPlease check the dates."));
       gtk_button_set_label (GTK_BUTTON(dateEntry), endDate);
    }

  g_free (newDate);
}

/*****************************
  callback from Builder
  get new end project date
****************************/
void on_IntervalEndBtn_clicked (GtkButton  *button, APP_data *data)
{
  GDate date_interval1, date_interval2;
  gboolean fErrDateInterval1 = FALSE;
  gboolean fErrDateInterval2 = FALSE;
  GtkWidget *win = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "dialogProperties"));
  GtkWidget *dateEntry = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonPropertiesEndDate"));
  gchar* newDate = misc_getDate(gtk_button_get_label(GTK_BUTTON(dateEntry)), win);
  gtk_button_set_label (GTK_BUTTON(dateEntry), newDate);

 /* we must check if dates aren't inverted */
  const gchar *startDate=NULL;
  startDate = gtk_button_get_label (GTK_BUTTON(gtk_builder_get_object (data->tmpBuilder, "buttonPropertiesStartDate")));
  g_date_set_parse (&date_interval1, startDate);
  g_date_set_parse (&date_interval2, newDate);
  fErrDateInterval1 = g_date_valid (&date_interval1);
  if(!fErrDateInterval1)
      printf("Internal error date Interv 1\n");
  fErrDateInterval2 = g_date_valid (&date_interval2);  
  if(!fErrDateInterval2)
      printf("Internal error date Interv 2\n");
  gint cmp_date = g_date_compare (&date_interval1, &date_interval2);
  if(cmp_date>0)
    {
       misc_ErrorDialog (win, _("<b>Error!</b>\n\nDates mismatch ! 'Ending' date must be <i>after</i> 'Starting' date.\n\nPlease check the dates."));
       gtk_button_set_label (GTK_BUTTON(dateEntry), startDate);
    }

  g_free (newDate);
}

/****************************
  reset organization
  logo
****************************/
void properties_reset_organization_logo (GtkButton *button, APP_data *data)
{
  GtkWidget *buttonLogo;

  if(data->properties.logo!=NULL) {
      /* set default icon */
      GtkWidget *tmpImage = gtk_image_new_from_icon_name ("gtk-orientation-portrait", GTK_ICON_SIZE_DIALOG);
      /* we change the button's logo */
      buttonLogo = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonPropertiesLogo"));
      gtk_button_set_image (GTK_BUTTON(buttonLogo), tmpImage);
      data->properties.fRemIcon = TRUE;
  }
}

/****************************
  looking for organization
  logo
****************************/
void properties_choose_organization_logo (GtkButton *button, APP_data *data)
{
  gchar *filename;
  GdkPixbuf *imageOrganizationLogo;
  GError *err=NULL;
  GtkWidget *buttonLogo, *dlg;

  GtkFileFilter *filter = gtk_file_filter_new ();
  gtk_file_filter_add_pattern (filter, "*.png");
  gtk_file_filter_add_pattern (filter, "*.PNG");
  gtk_file_filter_add_pattern (filter, "*.jpg");
  gtk_file_filter_add_pattern (filter, "*.JPG");
  gtk_file_filter_add_pattern (filter, "*.jpeg");
  gtk_file_filter_add_pattern (filter, "*.JPEG");
  gtk_file_filter_add_pattern (filter, "*.svg");
  gtk_file_filter_add_pattern (filter, "*.SVG");

  dlg = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "dialogProperties"));
  /* we get a pointer on button's logo image */
  buttonLogo = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonPropertiesLogo"));

  gtk_file_filter_set_name (filter, _("Image files"));
  GtkWidget *dialog = create_loadFileDialog (dlg, data, _("Import organization logo ..."));
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    gtk_widget_destroy (GTK_WIDGET(dialog)); 
    /* we convert finename's path to URI path */
//    uri_path = g_filename_to_uri (filename, NULL, NULL);
    /* we load and resize logo - Glib does an intelligent reiszing, thanks to devs ! */
    gint width, height;
    gdk_pixbuf_get_file_info (filename, &width, &height);

    if(data->properties.logo!=NULL) {
       g_object_unref (data->properties.logo);
       data->properties.logo = NULL;
    }
    data->properties.logo = gdk_pixbuf_new_from_file_at_size (filename, PROP_LOGO_WIDTH, PROP_LOGO_HEIGHT, &err);
    if(err==NULL && width>0 && height>0) {
        GtkWidget *tmpImage = gtk_image_new_from_pixbuf (data->properties.logo);
        /* we change the button's logo */
        gtk_button_set_image (GTK_BUTTON(buttonLogo), tmpImage);
    }
    else {
        g_error_free (err);
        misc_ErrorDialog (GTK_WIDGET(dlg), _("<b>Error!</b>\n\nProblem with choosen image !\nThe logo remains unchanged."));
    }
    /* cleaning */
    g_free (filename);
  }
  else 
    gtk_widget_destroy (GTK_WIDGET(dialog)); 
}

/****************************************
  main properties dialog
  * at startup, start date and end date
  * have already been defined by
  * function misc_init_vars
*****************************************/
GtkWidget *properties_create_dialog (APP_data *data)
{
  GtkWidget *dialog;
  /* get datas from xml Glade file - CRITICAL : only in activate phase !!!! */
  GtkBuilder *builder = NULL;
  builder = gtk_builder_new ();
  GError *err = NULL; 
  
  if(!gtk_builder_add_from_file (builder, find_ui_file ("properties.ui"), &err)) {
     g_error_free (err);
     misc_halt_after_glade_failure (data);
  }
  /* Get the dialog from the glade file. */
  gtk_builder_connect_signals (builder, data);
  dialog = GTK_WIDGET(gtk_builder_get_object (builder, "dialogProperties"));
  data->tmpBuilder = builder;

  /* header bar */

  GtkWidget *hbar = gtk_header_bar_new ();
  gtk_widget_show (hbar);
  gtk_header_bar_set_title (GTK_HEADER_BAR (hbar), _("Project's properties"));
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (hbar), _("Define and modify project's properties and various global values."));
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (hbar), FALSE);
  gtk_window_set_titlebar (GTK_WINDOW(dialog), hbar);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  
  gtk_header_bar_pack_start (GTK_HEADER_BAR (hbar), box);
  GtkWidget *icon =  gtk_image_new_from_icon_name  ("document-properties", GTK_ICON_SIZE_DIALOG);
  gtk_container_add (GTK_CONTAINER (box), icon);
  gtk_widget_show (box);
  gtk_widget_show (icon);

  /* we setup various default values */

  GtkWidget *projectTitleEntry = GTK_WIDGET(gtk_builder_get_object (builder, "entryPropertiesTitle"));
  GtkWidget *projectDummaryEntry = GTK_WIDGET(gtk_builder_get_object (builder, "entryPropertiesSummary"));
  GtkWidget *startEntry = GTK_WIDGET(gtk_builder_get_object (builder, "buttonPropertiesStartDate"));
  GtkWidget *endEntry = GTK_WIDGET(gtk_builder_get_object (builder, "buttonPropertiesEndDate"));
  GtkWidget *locationEntry = GTK_WIDGET(gtk_builder_get_object (builder, "entryPropertiesLocation"));
  GtkWidget *gpsEntry = GTK_WIDGET(gtk_builder_get_object (builder, "entryPropertiesGPS"));
  GtkWidget *startRange = GTK_WIDGET(gtk_builder_get_object (builder, "scaleStart"));
  GtkWidget *endRange = GTK_WIDGET(gtk_builder_get_object (builder, "scaleEnd"));

  gtk_entry_set_text (GTK_ENTRY(projectTitleEntry), _("title")); 
  gtk_entry_set_text (GTK_ENTRY(locationEntry), _("Project's main location")); 
  gtk_entry_set_text (GTK_ENTRY(gpsEntry), _("Project's GPS location - optionnal")); 
  gtk_button_set_label (GTK_BUTTON(startEntry), data->properties.start_date);
  gtk_button_set_label (GTK_BUTTON(endEntry), data->properties.end_date);

  gtk_range_set_value (GTK_RANGE(startRange), data->properties.scheduleStart);
  gtk_range_set_value (GTK_RANGE(endRange), data->properties.scheduleEnd);
  
  gtk_window_set_transient_for (GTK_WINDOW(dialog),
                              GTK_WINDOW(data->appWindow));  
  return dialog;
}

/******************************
  properties set-up
*****************************/
void properties_modify (APP_data *data)
{
  GtkWidget *dialog;
  GDate date_interval1, date_interval2, date_start, date_end;
  gboolean fStartDatechanged = FALSE, fEndDateChanged = FALSE;
  gint ret, i;
  GList *l;
  GtkWidget *entryManager, *entryTitle, *entrySummary, *entryManagerMail, *entryManagerPhone;
  GtkWidget *entryOrgaName, *entryProjWebsite, *entryUnits, *spinBudget, *buttonLogo;
  GtkWidget *startRange, *endRange, *btnStartDate, *btnEndDate;
  GtkWidget *labelDashCol0, *labelDashCol1, *labelDashCol2, *labelDashCol3;
  GKeyFile *keyString;
  struct lconv *locali;
  const gchar *startDate = NULL;
  const gchar *endDate = NULL;
  GdkPixbuf *pix = NULL;

  locali = localeconv ();

  if(data->properties.units == NULL) {
        data->properties.units = g_strdup_printf ("%s", locali->int_curr_symbol);
  }
  
  data->properties.fRemIcon = FALSE;

  dialog = properties_create_dialog (data);
  /* we get datas */
  entryTitle = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryPropertiesTitle"));
  entrySummary = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryPropertiesSummary"));

  GtkWidget *locationEntry = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryPropertiesLocation"));
  GtkWidget *gpsEntry = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryPropertiesGPS"));

  entryManager = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryPropertiesManager"));
  entryManagerMail = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryPropertiesManagerMail"));
  entryManagerPhone = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryPropertiesManagerPhone"));

  entryOrgaName = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryPropertiesOrganization"));
  buttonLogo = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonPropertiesLogo"));
  entryProjWebsite = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryProjectWebsite"));

  entryUnits 	= GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryPropertiesUnits"));
  spinBudget 	= GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonPropertiesBudget"));

  btnStartDate 	= GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonPropertiesStartDate"));
  btnEndDate 	= GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonPropertiesEndDate"));
  startRange 	= GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "scaleStart"));
  endRange 	= GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "scaleEnd"));
  
  labelDashCol0	= GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryDashCol0"));
  labelDashCol1	= GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryDashCol1"));  
  labelDashCol2	= GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryDashCol2"));
  labelDashCol3	= GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryDashCol3"));
  
  
  /* set current values  */
  /* project */
  if(data->properties.title) {
       gtk_entry_set_text (GTK_ENTRY(entryTitle), data->properties.title);
  }
  if(data->properties.summary) {
       gtk_entry_set_text (GTK_ENTRY(entrySummary), data->properties.summary);
  }

  if(data->properties.location) {
       gtk_entry_set_text (GTK_ENTRY(locationEntry), data->properties.location);
  }
  if(data->properties.gps) {
       gtk_entry_set_text (GTK_ENTRY(gpsEntry), data->properties.gps);
  }

  if(data->properties.units) {
       gtk_entry_set_text (GTK_ENTRY(entryUnits), data->properties.units);
  }
  if(data->properties.proj_website) {
       gtk_entry_set_text (GTK_ENTRY(entryProjWebsite), data->properties.proj_website);
  }

  gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinBudget), data->properties.budget);

  if(data->properties.start_date) {
         gtk_button_set_label (GTK_BUTTON(btnStartDate), data->properties.start_date);
  }
  if(data->properties.end_date) {
         gtk_button_set_label (GTK_BUTTON(btnEndDate), data->properties.end_date);
  }
  /* manager */
  if(data->properties.manager_name) {
       gtk_entry_set_text (GTK_ENTRY(entryManager), data->properties.manager_name);
  }
  if(data->properties.manager_mail) {
       gtk_entry_set_text (GTK_ENTRY(entryManagerMail), data->properties.manager_mail);
  }
  if(data->properties.manager_phone) {
       gtk_entry_set_text (GTK_ENTRY(entryManagerPhone), data->properties.manager_phone);
  }
  /* organization */
  if(data->properties.org_name) {
       gtk_entry_set_text (GTK_ENTRY(entryOrgaName), data->properties.org_name);
  }
  /* logo */
  if(data->properties.logo) {
     pix = gdk_pixbuf_copy (data->properties.logo);
     GtkWidget *tmpImage = gtk_image_new_from_pixbuf (data->properties.logo);
        /* we change the button's logo */
     gtk_button_set_image (GTK_BUTTON(buttonLogo), tmpImage);
  }

  /* dashboard */
  if(data->properties.labelDashCol0) {
     gtk_entry_set_text (GTK_ENTRY(labelDashCol0), data->properties.labelDashCol0);
  }	  
  if(data->properties.labelDashCol1) {
     gtk_entry_set_text (GTK_ENTRY(labelDashCol1), data->properties.labelDashCol1);
  }	  
  if(data->properties.labelDashCol2) {
     gtk_entry_set_text (GTK_ENTRY(labelDashCol2), data->properties.labelDashCol2);
  }	  
  if(data->properties.labelDashCol3) {
     gtk_entry_set_text (GTK_ENTRY(labelDashCol3), data->properties.labelDashCol3);
  }	  
  
  
  
  ret = gtk_dialog_run (GTK_DIALOG(dialog));
  /* ret==1 for [OK] */
  if(ret == 1) {
      /* undo engine - we just have to get a pointer on ID */
      undo_datas *tmp_value;
      tmp_value = g_malloc (sizeof(undo_datas));
      tmp_value->opCode = OP_MODIFY_PROP;
      rsc_properties *tmp;
      tmp = g_malloc (sizeof(rsc_properties));
      tmp->title 	= g_strdup_printf ("%s", data->properties.title);
      tmp->summary 	= g_strdup_printf ("%s", data->properties.summary);
      tmp->location 	= g_strdup_printf ("%s", data->properties.location);
      tmp->gps 		= g_strdup_printf ("%s", data->properties.gps);
      tmp->units 	= g_strdup_printf ("%s", data->properties.units);
      tmp->manager_name = g_strdup_printf ("%s", data->properties.manager_name);
      tmp->manager_mail = g_strdup_printf ("%s", data->properties.manager_mail);
      tmp->manager_phone = g_strdup_printf ("%s", data->properties.manager_phone);
      tmp->org_name 	= g_strdup_printf ("%s", data->properties.org_name);
      tmp->proj_website = g_strdup_printf ("%s", data->properties.proj_website);
      tmp->budget 	= data->properties.budget;
      tmp->scheduleStart = data->properties.scheduleStart;
      tmp->scheduleEnd 	= data->properties.scheduleEnd;
      tmp->start_date 	= g_strdup_printf ("%s", data->properties.start_date);
      tmp->end_date 	= g_strdup_printf ("%s", data->properties.end_date);
      tmp->start_nthDay = data->properties.start_nthDay;
      tmp->start_nthMonth = data->properties.start_nthMonth;
      tmp->start_nthYear = data->properties.start_nthYear;
      tmp->end_nthDay 	= data->properties.end_nthDay;
      tmp->end_nthMonth = data->properties.end_nthMonth;
      tmp->end_nthYear 	= data->properties.end_nthYear;
      tmp->logo 	= NULL;
      if(pix!=NULL) {/* previous pixbuf alreday duplicated */
          tmp->logo = pix;
      }
      /* dashboard */
      tmp->labelDashCol0 = g_strdup_printf ("%s", data->properties.labelDashCol0);
      tmp->labelDashCol1 = g_strdup_printf ("%s", data->properties.labelDashCol1);
      tmp->labelDashCol2 = g_strdup_printf ("%s", data->properties.labelDashCol2);
      tmp->labelDashCol3 = g_strdup_printf ("%s", data->properties.labelDashCol3);
                     
      tmp_value->datas = tmp;
      tmp_value->groups = NULL;
      tmp_value->list = NULL;
      undo_push (CURRENT_STACK_MISC, tmp_value, data);

      if(data->properties.title) {
            g_free (data->properties.title);
      }
      data->properties.title = g_strdup_printf ("%s", gtk_entry_get_text (GTK_ENTRY(entryTitle)));

      if(data->properties.summary) {
            g_free (data->properties.summary);
      }
      data->properties.summary = g_strdup_printf ("%s", gtk_entry_get_text (GTK_ENTRY(entrySummary)));
      /* the summary is updated in 'savestate' module */
      keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");
      gchar *summary = files_get_project_summary (data);
      store_current_file_in_keyfile (keyString, g_key_file_get_string (keyString, "application", "current-file", NULL), 
                                     summary);
      g_free (summary);

      if(data->properties.location) {
            g_free (data->properties.location);
      }
      data->properties.location = g_strdup_printf ("%s", gtk_entry_get_text (GTK_ENTRY(locationEntry)));

      if(data->properties.gps) {
            g_free (data->properties.gps);
      }
      data->properties.gps = g_strdup_printf ("%s", gtk_entry_get_text (GTK_ENTRY(gpsEntry)));

      if(data->properties.units) {
            g_free (data->properties.units);
      }
      data->properties.units = g_strdup_printf ("%s", gtk_entry_get_text (GTK_ENTRY(entryUnits)));

      if(data->properties.manager_name) {
            g_free (data->properties.manager_name);
      }
      data->properties.manager_name = g_strdup_printf ("%s", gtk_entry_get_text (GTK_ENTRY(entryManager)));

      if(data->properties.manager_mail) {
            g_free (data->properties.manager_mail);
      }
      data->properties.manager_mail = g_strdup_printf ("%s", gtk_entry_get_text (GTK_ENTRY(entryManagerMail)));

      if(data->properties.manager_phone) {
            g_free (data->properties.manager_phone);
      }
      data->properties.manager_phone = g_strdup_printf ("%s", gtk_entry_get_text (GTK_ENTRY(entryManagerPhone)));

      if(data->properties.org_name) {
            g_free (data->properties.org_name);
      }
      data->properties.org_name = g_strdup_printf ("%s", gtk_entry_get_text (GTK_ENTRY(entryOrgaName)));

      if(data->properties.proj_website) {
            g_free (data->properties.proj_website);
      }
      data->properties.proj_website = g_strdup_printf ("%s", gtk_entry_get_text (GTK_ENTRY(entryProjWebsite)));

      data->properties.budget = gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinBudget));

      /* dashboard */
      if(data->properties.labelDashCol0) {
	    g_free (data->properties.labelDashCol0);
      }
      data->properties.labelDashCol0 = g_strdup_printf ("%s", gtk_entry_get_text (GTK_ENTRY(labelDashCol0)));
      if(data->properties.labelDashCol1) {
	    g_free (data->properties.labelDashCol1);
      }
      data->properties.labelDashCol1 = g_strdup_printf ("%s", gtk_entry_get_text (GTK_ENTRY(labelDashCol1)));      
      if(data->properties.labelDashCol2) {
	    g_free (data->properties.labelDashCol2);
      }
      data->properties.labelDashCol2 = g_strdup_printf ("%s", gtk_entry_get_text (GTK_ENTRY(labelDashCol2)));
      if(data->properties.labelDashCol3) {
	    g_free (data->properties.labelDashCol3);
      }
      data->properties.labelDashCol3 = g_strdup_printf ("%s", gtk_entry_get_text (GTK_ENTRY(labelDashCol3)));
      dashboard_update_column_labels (data);
      /* schedules */
      data->properties.scheduleStart = (gint) gtk_range_get_value (GTK_RANGE(startRange));
      data->properties.scheduleEnd = (gint) gtk_range_get_value (GTK_RANGE(endRange));
      /* dates */
      /* current values */
      g_date_set_parse (&date_start, data->properties.start_date);
      g_date_set_parse (&date_end, data->properties.end_date);      
      /* dates in dialog */
      startDate = gtk_button_get_label (GTK_BUTTON(gtk_builder_get_object (data->tmpBuilder, "buttonPropertiesStartDate")));
      endDate = gtk_button_get_label (GTK_BUTTON(gtk_builder_get_object (data->tmpBuilder, "buttonPropertiesEndDate")));
      data->properties.start_date = g_strdup_printf ("%s", startDate);
      data->properties.end_date = g_strdup_printf ("%s", endDate);
      g_date_set_parse (&date_interval1, startDate);
      g_date_set_parse (&date_interval2, endDate);
      /* we check if dates are in limits */
      if(g_date_compare (&date_interval1, &date_start) != 0) {
        fStartDatechanged = TRUE;
      }
      if(g_date_compare (&date_interval2, &date_end) != 0) {
        fEndDateChanged = TRUE;
      }
      /* explode dates into components nthDay, nthMonth, year */
      data->properties.start_nthDay 	= g_date_get_day (&date_interval1);
      data->properties.start_nthMonth 	= g_date_get_month (&date_interval1);
      data->properties.start_nthYear 	= g_date_get_year (&date_interval1);
      data->properties.end_nthDay 	    = g_date_get_day (&date_interval2);
      data->properties.end_nthMonth 	= g_date_get_month (&date_interval2);
      data->properties.end_nthYear 	    = g_date_get_year (&date_interval2);
      /* main icon */
      if(data->properties.fRemIcon == TRUE) {
          g_object_unref (data->properties.logo);
          data->properties.logo = NULL;
      }
      /* check for empty groups */
      if(fEndDateChanged || fStartDatechanged) {
	  /* we update groups */
          gint nbTasks = g_list_length (data->tasksList);
          tasks_data *tmp;
		  if(nbTasks>0) {
      	    for(i = 0; i < nbTasks; i++) {
			l = g_list_nth (data->tasksList, i);
			tmp = (tasks_data *) l->data;
				if(tmp->type == TASKS_TYPE_GROUP) {
				   tasks_update_group_limits (tmp->id, tmp->id, tmp->id, data);/* update also widgets mainly used for empty groups */
				}
			 }/* next i */
		  }/* endif */
		  /* if dates changed, we redraw the timeline */
		  timeline_remove_ruler (data);
		  GtkAdjustment *adjustment
		       = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowTimeline")));
        //  gtk_adjustment_set_value (GTK_ADJUSTMENT(adjustment), 0);
          timeline_store_date (data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
          timeline_draw_calendar_ruler (timeline_get_ypos (data), data);
		  timeline_remove_all_tasks (data);
		  timeline_draw_all_tasks (data);		  
      }/* endif dates */

      misc_display_app_status (TRUE, data);
  }

  g_object_unref (data->tmpBuilder);
  home_project_infos (data);
  gtk_widget_destroy (GTK_WIDGET(dialog));  /* please note : with a Builder, you can only use destroy in case of the properties 'destroy with parent' isn't activated for the dualog */
}

/*****************************
  free properties datas
*****************************/
void properties_free_all (APP_data *data)
{
 if(data->properties.title) {
    g_free (data->properties.title);
 }
 data->properties.title = NULL;

 if(data->properties.summary) {
    g_free ( data->properties.summary);
 }
 data->properties.summary = NULL;

 if(data->properties.location) {
    g_free (data->properties.location);
 }
 data->properties.location = NULL;

 if(data->properties.gps) {
    g_free (data->properties.gps);
 }
 data->properties.gps = NULL;

 if(data->properties.manager_name) {
    g_free (data->properties.manager_name);
 }
 data->properties.manager_name = NULL;

 if(data->properties.manager_mail) {
    g_free (data->properties.manager_mail);
 }
 data->properties.manager_mail = NULL;

 if(data->properties.manager_phone) {
    g_free (data->properties.manager_phone);
 }
 data->properties.manager_phone = NULL;

 if(data->properties.org_name) {
    g_free (data->properties.org_name);
 } 
 data->properties.org_name = NULL;

 if(data->properties.proj_website) {
    g_free (data->properties.proj_website);
 } 
 data->properties.proj_website = NULL;

 if(data->properties.units) {
    g_free (data->properties.units);
 }
 data->properties.units = NULL;

 /* dashboard */
 if(data->properties.labelDashCol0) {
	g_free (data->properties.labelDashCol0);
 }	
 data->properties.labelDashCol0 = NULL;

 if(data->properties.labelDashCol1) {
	g_free (data->properties.labelDashCol1);
 }	
 data->properties.labelDashCol1 = NULL;

 if(data->properties.labelDashCol2) {
	g_free (data->properties.labelDashCol2);
 }	
 data->properties.labelDashCol2 = NULL;

 if(data->properties.labelDashCol3) {
	g_free (data->properties.labelDashCol3);
 }	
 data->properties.labelDashCol3 = NULL;

 if(data->properties.start_date) {
    g_free (data->properties.start_date);
 }
 data->properties.start_date = NULL;

 if(data->properties.end_date) {
    g_free (data->properties.end_date);
 }
 data->properties.end_date = NULL;

 if(data->properties.logo) {
    g_object_unref (data->properties.logo);
 }
 data->properties.logo = NULL;

 data->objectsCounter = 0;/* internal counter, never decremented, in order to give UNIQUE id. to objects */

 printf("* PlanLibre : successfully freed properties datas *\n");
}

/************************************
  PUBLIC :   set default dashboard
  * hedaers for columns
************************************/
void properties_set_default_dashboard (APP_data *data)
{
  data->properties.labelDashCol0 = g_strdup_printf ("%s", _("To Do"));
  data->properties.labelDashCol1 = g_strdup_printf ("%s", _("In progress"));
  data->properties.labelDashCol2 = g_strdup_printf ("%s", _("Overdue"));  
  data->properties.labelDashCol3 = g_strdup_printf ("%s", _("Completed"));     
}

/********************************
  PUBLIC :   set default dates
*******************************/
void properties_set_default_dates (APP_data *data)
{
  GDate current_date;
  
  g_date_set_time_t (&current_date, time (NULL)); /* date of today */
  /* start date : today */
  data->properties.start_nthDay = g_date_get_day (&current_date);
  data->properties.start_nthMonth = g_date_get_month (&current_date);
  data->properties.start_nthYear = g_date_get_year (&current_date);
  data->properties.start_date = misc_convert_date_to_locale_str (&current_date);
  /* default ending date 14 days ahead */
  g_date_add_days (&current_date, 14);
  data->properties.end_nthDay = g_date_get_day (&current_date);
  data->properties.end_nthMonth = g_date_get_month (&current_date);
  data->properties.end_nthYear = g_date_get_year (&current_date);
  data->properties.end_date =  misc_convert_date_to_locale_str (&current_date);
}

/********************************
  PUBLIC : set default schedules
*******************************/
void properties_set_default_schedules (APP_data *data)
{
  data->properties.scheduleStart = 540; /* 09h00m */
  data->properties.scheduleEnd = 1080; /* 18h00m */
}

/********************************
  PUBLIC :   shift dates
*******************************/
void properties_shift_dates (gint shift, APP_data *data)
{
  GDate date_start, date_end;
  
  g_date_set_parse (&date_start, data->properties.start_date);
  g_date_set_parse (&date_end, data->properties.end_date);

  if(shift != 0) {
     if(shift<0) {/* advance date */
        g_date_subtract_days (&date_start, ABS (shift));
        g_date_subtract_days (&date_end, ABS (shift));

     }
     else {/* delay date */
        g_date_add_days (&date_start, shift);
        g_date_add_days (&date_end, shift);
     }
     data->properties.start_nthDay = g_date_get_day (&date_start);
     data->properties.start_nthMonth = g_date_get_month (&date_start);
     data->properties.start_nthYear = g_date_get_year (&date_start);
     g_free (data->properties.start_date);
     data->properties.start_date =  misc_convert_date_to_locale_str (&date_start);

     data->properties.end_nthDay = g_date_get_day (&date_end);
     data->properties.end_nthMonth = g_date_get_month (&date_end);
     data->properties.end_nthYear = g_date_get_year (&date_end);
     g_free (data->properties.end_date);
     data->properties.end_date =  misc_convert_date_to_locale_str (&date_end);

  }/* endif shift !=0 */
}
