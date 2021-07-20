/***************************
  functions to manage files

excellent site en français sur xml : https://www.julp.fr/articles/1-1-presentation.html

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
****************************/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* translations */
#include <libintl.h>
#include <locale.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>
#include <glib/gstdio.h> /* g_fopen, etc */
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include "support.h" /* glade requirement */
#include "links.h"
#include "tasks.h"
#include "tasksutils.h"
#include "ressources.h"
#include "timeline.h"
#include "dashboard.h"
#include "assign.h"
#include "files.h"
#include "report.h" 
#include "calendars.h" 
#include "undoredo.h"

/****************************

  FUNCTIONS

****************************/

static gboolean fStartedSaving = FALSE; /* in order to avoid re-entrant autosave */

/******************************************
 PUBLIC : test if we have entered "save"
 function
*******************************************/
gboolean files_started_saving (APP_data *data)
{
   return fStartedSaving;
}

/******************************************
 PUBLIC : force re-entrant protection
 to a certain state
*******************************************/
void files_set_re_entrant_state (gboolean flag, APP_data *data)
{
   fStartedSaving = flag;
}

/*******************************************
  local function to get a GdkPixbuf encoded 
  as base64 string 
  Parameter : base64 string
  Output : newly allocated GdkPixbuf.
  Use g_object_unref once  useless
*******************************************/
static  GdkPixbuf *get_pixbuf_from_code64 (gchar *valeur2 )
{
  gsize out_len;
  GdkPixbufLoader *loader;
  GdkPixbuf *pixbuf=NULL, *pixbuf2 = NULL;
  gboolean rr;
  GError *err = NULL;

  if(valeur2!=NULL) {
       guchar *pix_decoded = g_base64_decode (valeur2, &out_len);
       loader = gdk_pixbuf_loader_new ();
       rr = gdk_pixbuf_loader_write (loader, pix_decoded, out_len, NULL);
       rr = gdk_pixbuf_loader_close (loader, &err); // mandatory thanks to reply of Didi Kohen here : https://stackoverflow.com/questions/14121166/gdk-pixbuf-load-image-from-memory
       pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
       pixbuf2 = gdk_pixbuf_copy (pixbuf); /* in order to free loader */
       g_free (pix_decoded);
       g_object_unref(loader);
  }
  return pixbuf2;
}

/**********************************************************************************
  LOCAL function to get a value in xml file
  Parameter : complete xpath
  on a key, for example :
"/PlanLibre/Properties/Project/Title/text()"  
  see : http://ymettier.free.fr/articles_lmag/lmag51_briques_en_C17/ar01s09.html
***********************************************************************************/
static gchar *get_xml_value (gchar *path, xmlDocPtr doc)
{
  gchar *value = NULL;
  xmlXPathContextPtr xml_context = NULL;
  xmlXPathObjectPtr xmlobject;

  xmlXPathInit ();
  xml_context = xmlXPathNewContext (doc);

  xmlobject = xmlXPathEval (path, xml_context);

  if((xmlobject->type == XPATH_NODESET) && (xmlobject->nodesetval)) {
      if (xmlobject->nodesetval->nodeNr) {
         xmlNodePtr node;
         node = xmlobject->nodesetval->nodeTab[0];
         if ((node->type == XML_TEXT_NODE) || (node->type == XML_CDATA_SECTION_NODE)) {
            value = g_strdup_printf ("%s", node->content); 
         }
      }
  }
  /* clean */
  xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
  return value;
}

/******************************
  LOCAL : given a Xpath
  and an xml attribute name, 
  returns attributes's value
'attr' is the attributes'name
superbe exmplicatino en python : http://makble.com/how-to-use-xpath-syntax-example-with-python-and-lxml
******************************/

static gchar *get_xml_attribute_value (gchar *path, gchar *attr, xmlDocPtr doc )
{
  gchar *value = NULL;
  xmlXPathContextPtr xml_context = NULL;
  xmlXPathObjectPtr xmlobject;

  xmlXPathInit ();
  xml_context = xmlXPathNewContext (doc);

  xmlobject = xmlXPathEval (path, xml_context);
  if(xmlobject==NULL) {
     xmlXPathFreeContext (xml_context);
     return NULL;
  }

  if ((xmlobject->type == XPATH_NODESET) && (xmlobject->nodesetval)) {
    if (xmlobject->nodesetval->nodeNr) {/* this test is MANDATORY */
        xmlNodePtr node;
        node = xmlobject->nodesetval->nodeTab[0];
        // here we have a path to receiver : test it ; printf("name: %s\n", node->name);
        if ((!xmlStrcmp(node->name, (const xmlChar *)attr))) {
          // printf("name: %s content ", node->name);
           value = g_strdup_printf ("%s", xmlGetProp(node->parent, (const xmlChar *)attr));
        }
     }/* endif nodeNr */
  }
  /* clean */
  xmlXPathFreeObject (xmlobject);
  xmlXPathFreeContext (xml_context);
  return value;
}

/******************************
  local function to retrieve 
  "project's properties" 
  from current xml file
******************************/
static gint get_properties_from_xml (xmlDocPtr doc, APP_data *data)
{
  GKeyFile *keyString;
  gint rc = 0, dd = -1, mm = -1, yy = -1;
  gchar *valeur2 = NULL;

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");
  /* to get a value, we launch a Xpath request */

  /* we get automail info, if exists, and store to config.ini hidden file */
  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Project/@automail_date_day", "automail_date_day", doc);
  if(valeur2) {
     dd = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
     g_free (valeur2);
  }
  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Project/@automail_date_month", "automail_date_month", doc);
  if(valeur2) {
     mm = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
     g_free (valeur2);
  }
  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Project/@automail_date_year", "automail_date_year", doc);
  if(valeur2) {
     yy = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
     g_free (valeur2);
  }
  g_key_file_set_integer (keyString, "collaborate", "last_auto_mailing-day", dd);
  g_key_file_set_integer (keyString, "collaborate", "last_auto_mailing-month", mm);
  g_key_file_set_integer (keyString, "collaborate", "last_auto_mailing-year", yy);

  /* now, we read and store Properties of Document */

  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Project/@Objects", "Objects", doc);
  if(valeur2) {
     data->objectsCounter = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
     g_free (valeur2);
  }

  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Project/@Title", "Title", doc);
  if(valeur2) {
     data->properties.title = g_strdup_printf ("%s", valeur2);
     g_free (valeur2);
  }

  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Project/@Summary", "Summary", doc);
  if(valeur2) {
     data->properties.summary = g_strdup_printf ("%s", valeur2);
     g_free (valeur2);
  }

  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Project/@Location", "Location", doc);
  if(valeur2) {
     data->properties.location = g_strdup_printf ("%s", valeur2);
     g_free (valeur2);
  }

  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Project/@Gps", "Gps", doc);
  if(valeur2) {
     data->properties.gps = g_strdup_printf ("%s", valeur2);
     g_free (valeur2);
  }

  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Project/@Units", "Units", doc);
  if(valeur2) {
     data->properties.units = g_strdup_printf ("%s", valeur2);
     g_free (valeur2);
  }

  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Project/@Website", "Website", doc);
  if(valeur2) {
     data->properties.proj_website = g_strdup_printf ("%s", valeur2);
     g_free (valeur2);
  }

  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Project/@Budget", "Budget", doc);
  if(valeur2) {
     data->properties.budget = g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
     g_free (valeur2);
  }
  
  /* start date components */
  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Project/@Start_date_day", "Start_date_day", doc);
  if(valeur2) {
     data->properties.start_nthDay = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
     g_free (valeur2);
  }

  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Project/@Start_date_month", "Start_date_month", doc);
  if(valeur2) {
     data->properties.start_nthMonth = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
     g_free (valeur2);
  }

  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Project/@Start_date_year", "Start_date_year", doc);
  if(valeur2) {
     data->properties.start_nthYear = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
     g_free (valeur2);
  }

//  g_date_set_dmy  (&date_start, data->properties.start_nthDay,
  //                data->properties.start_nthMonth, data->properties.start_nthYear);
 // g_date_strftime(buffer, 75, _("%x"), &date_start);
  if(data->properties.start_date) {
     g_free (data->properties.start_date);
  }

  data->properties.start_date = misc_convert_date_to_str(data->properties.start_nthDay,
                                      data->properties.start_nthMonth, data->properties.start_nthYear  );
// printf("buffer start =%s dans data=%s \n", buffer, data->properties.start_date);

  /* end date components */
  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Project/@End_date_day", "End_date_day", doc);
  if(valeur2) {
     data->properties.end_nthDay = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
     g_free (valeur2);
  }

  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Project/@End_date_month", "End_date_month", doc);
  if(valeur2) {
     data->properties.end_nthMonth = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
     g_free (valeur2);
  }

  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Project/@End_date_year", "End_date_year", doc);
  if(valeur2) {
     data->properties.end_nthYear = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
     g_free (valeur2);
  }
//  g_date_set_dmy  (&date_end, data->properties.end_nthDay,
  //              data->properties.end_nthMonth, data->properties.end_nthYear);
//  g_date_strftime(buffer, 75, _("%x"), &date_end);
  if(data->properties.end_date) {
     g_free (data->properties.end_date);
  }
  data->properties.end_date = misc_convert_date_to_str(data->properties.end_nthDay,
                                      data->properties.end_nthMonth, data->properties.end_nthYear);
// printf("buffer end =%s dans data=%s \n", buffer, data->properties.end_date);

  /* dashboard columns labels - be careful, at this step, they are initialized, so if valeur2 not NULL, we must free */
 
  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Dashboard/@DashCol0", "DashCol0", doc);
  if(valeur2) {
	 g_free (data->properties.labelDashCol0);
     data->properties.labelDashCol0 = g_strdup_printf ("%s", valeur2);
     g_free (valeur2);
  }
  
  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Dashboard/@DashCol1", "DashCol1", doc);
  if(valeur2) {
	 g_free (data->properties.labelDashCol1);
     data->properties.labelDashCol1 = g_strdup_printf ("%s", valeur2);
     g_free (valeur2);
  }  

  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Dashboard/@DashCol2", "DashCol2", doc);
  if(valeur2) {
	 g_free (data->properties.labelDashCol2);
     data->properties.labelDashCol2 = g_strdup_printf ("%s", valeur2);
     g_free (valeur2);
  }

  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Dashboard/@DashCol3", "DashCol3", doc);
  if(valeur2) {
	 g_free (data->properties.labelDashCol3);
     data->properties.labelDashCol3 = g_strdup_printf ("%s", valeur2);
     g_free (valeur2);
  }
  
  /* schedules */
  data->properties.scheduleStart = 540;
  valeur2 = get_xml_attribute_value("/PlanLibre/Properties/Project/@Schedule_start", "Schedule_start", doc);
  if(valeur2) {
     data->properties.scheduleStart = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
     g_free (valeur2);
  }
  data->properties.scheduleEnd = 1080;
  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Project/@Schedule_end", "Schedule_end", doc);
  if(valeur2) {
     data->properties.scheduleEnd = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
     g_free (valeur2);
  }
  /* manager --------------------------------------------*/
  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Manager/@Name", "Name", doc);
  if(valeur2) {
     data->properties.manager_name = g_strdup_printf ("%s", valeur2);
     g_free (valeur2);
  }

  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Manager/@Mail", "Mail", doc);
  if(valeur2) {
     data->properties.manager_mail = g_strdup_printf ("%s", valeur2);
     g_free (valeur2);
  }

  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Manager/@Phone", "Phone", doc);
  if(valeur2) {
     data->properties.manager_phone = g_strdup_printf ("%s", valeur2);
     g_free (valeur2);
  }
  /* organization -------------------------------*/
  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Organization/@Name", "Name", doc);
  if(valeur2) {
     data->properties.org_name = g_strdup_printf ("%s", valeur2);
     g_free (valeur2);
  }


  /* get organization's logo */
  valeur2 = get_xml_attribute_value ("/PlanLibre/Properties/Organization/@Logo", "Logo", doc);
  if(valeur2) {
    if(strlen(valeur2)>0)
          data->properties.logo = get_pixbuf_from_code64 (valeur2);
      g_free (valeur2);
  }
 
  return rc;
}

static gint get_xml_rsc_as_int (xmlDocPtr doc, gchar *xmlStr, gchar *sPrm)
{
  gchar *valeur2 = NULL;
  gint ret = -1;

  valeur2 = get_xml_attribute_value (xmlStr, sPrm, doc);
  if(valeur2!=NULL) {
       ret = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
       g_free (valeur2);
       valeur2 = NULL;
  }

  return ret;
}

static gdouble get_xml_rsc_as_double (xmlDocPtr doc, gchar *xmlStr, gchar *sPrm)
{
  gchar *valeur2 = NULL;
  gdouble ret = 0;

  valeur2 = get_xml_attribute_value (xmlStr, sPrm, doc);
  if(valeur2!=NULL) {
       ret = g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
       g_free (valeur2);
       valeur2 = NULL;
  }

  return ret;
}

/***********************************
  local function to retrieve 
  "project's ressources" 
  from current xml file
  the flag 'append' allow to 
 add ressources from CURRENT index
************************************/
static gint get_ressources_from_xml (xmlDocPtr doc, gboolean append, APP_data *data)
{
  gint rc =0, nbRsc=0, curObj=0, i=0;
  gint id, lenRsc = 0, hId, cal, hour_day, min_day;
  gchar *valeur2 = NULL, *name = NULL, *mail = NULL, *phone = NULL, *reminder = NULL, *location = NULL, *tmpStr;
  gint  type, affi, cost_type, work;
  gdouble cost, quantity = 1;
  GdkRGBA  color;
  GdkPixbuf *pix = NULL; 
  GtkWidget *cadres, *listBoxRessources;
  rsc_datas newRsc;

  /* to get a value, we launch a Xpath request */
  /* now, we read how many ressources are stored in file  */

  valeur2 = get_xml_value ("/PlanLibre/Ressources/Counter/text()", doc);
  if(valeur2 != NULL) {
     nbRsc = g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
     g_free (valeur2);
  }

  if(nbRsc <= 0)
     return -1;
 
  /* we do a loop to read all resources */
  listBoxRessources = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxRsc"));
  lenRsc = g_list_length (data->rscList);
  hId = misc_get_highest_id (data);

//TODO  en mode append, i ne commence plus à 0 ainsi que le test i<nbRsc et il faut connaître le n° le plus élévé des ID 
  for(i = lenRsc; i<lenRsc+nbRsc; i++) {
     id = -1;
     /* resets */
     type = 0;
     cost_type = 0;
     affi = 0;
     cost = 0;
     color.red = 0;
     color.green = 0;
     color.blue = 0;
     newRsc.location = NULL;
     /* default working days */
     newRsc.fWorkMonday = TRUE;
     newRsc.fWorkTuesday = TRUE;
     newRsc.fWorkWednesday = TRUE;
     newRsc.fWorkThursday = TRUE;
     newRsc.fWorkFriday = TRUE;
     newRsc.fWorkSaturday = FALSE;
     newRsc.fWorkSunday = FALSE;
	 /* allow concurrent usage of a ressource - should be modified in 0.2 TODO */
     newRsc.fAllowConcurrent = TRUE;	
     /* calendar */
     newRsc.calendar = 0; /* default */
     newRsc.day_hours = 8;
     newRsc.day_mins = 0;
      /* identifier */
     valeur2 = get_xml_attribute_value (g_strdup_printf ("/PlanLibre/Ressources/Object%d/@Id", i-lenRsc), "Id", doc);
     if(valeur2!=NULL) {
       id = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
       g_free (valeur2);
       valeur2 = NULL;
     }
     if(id < 0)  {
         printf ("* Ressources : critical error, bad object identifier *\n");
         return -1;
     }
     /* in append mode, we recalc a new RSC id */
     if(append) {
  //       printf(" en mode append, l'id valait %d elle est recalculée = %id \n", id, hId+1);
         id = hId+1;
         hId++;
     }
     /* name */ 
     valeur2 = get_xml_attribute_value (g_strdup_printf ("/PlanLibre/Ressources/Object%d/@Name", i-lenRsc), "Name", doc);
     if(valeur2!=NULL) {
       name = g_strdup_printf ("%s", valeur2);
       g_free (valeur2);
       valeur2 = NULL;
     }
     /* mail */
     valeur2 = get_xml_attribute_value (g_strdup_printf ("/PlanLibre/Ressources/Object%d/@Mail", i-lenRsc), "Mail", doc);
     if(valeur2!=NULL) {
       mail = g_strdup_printf ("%s", valeur2);
       g_free (valeur2);
       valeur2 = NULL;
     }
     /* phone */
     valeur2 = get_xml_attribute_value (g_strdup_printf ("/PlanLibre/Ressources/Object%d/@Phone", i-lenRsc), "Phone", doc);
     if(valeur2!=NULL) {
       phone = g_strdup_printf ("%s", valeur2);
       g_free (valeur2);
       valeur2 = NULL;
     }
     /* reminder */
     valeur2 = get_xml_attribute_value (g_strdup_printf ("/PlanLibre/Ressources/Object%d/@Reminder", i-lenRsc), "Reminder", doc);
     if(valeur2!=NULL) {
       reminder = g_strdup_printf ("%s", valeur2);
       g_free (valeur2);
       valeur2 = NULL;
     }

     /* numeric values, for combos and cost */
     tmpStr = g_strdup_printf ("/PlanLibre/Ressources/Object%d/@Affiliate", i-lenRsc);
     affi = get_xml_rsc_as_int (doc, tmpStr, "Affiliate");
     g_free (tmpStr);

     tmpStr = g_strdup_printf ("/PlanLibre/Ressources/Object%d/@Type", i-lenRsc);
     type = get_xml_rsc_as_int (doc, tmpStr, "Type");
     g_free (tmpStr);

     tmpStr = g_strdup_printf ("/PlanLibre/Ressources/Object%d/@Cost_type", i-lenRsc);
     cost_type = get_xml_rsc_as_int (doc, tmpStr, "Cost_type");
     g_free (tmpStr);

     tmpStr = g_strdup_printf ("/PlanLibre/Ressources/Object%d/@Cost_value", i-lenRsc);
     cost = get_xml_rsc_as_double (doc, tmpStr, "Cost_value");
     g_free (tmpStr);
     
     tmpStr = g_strdup_printf ("/PlanLibre/Ressources/Object%d/@Quantity", i-lenRsc);
     quantity = get_xml_rsc_as_double (doc, tmpStr, "Quantity");
     printf ("valeur lue qty =%d \n", (gint) quantity);
     if(quantity <1) {
		 quantity = 1;
     }
     newRsc.quantity = quantity;
     g_free (tmpStr);     
     /* working days */
     tmpStr = g_strdup_printf ("/PlanLibre/Ressources/Object%d/@working_monday", i-lenRsc);
     work = get_xml_rsc_as_int (doc, tmpStr, "working_monday");
     g_free (tmpStr);
     if(work == 0)
           newRsc.fWorkMonday = FALSE;
     else
           newRsc.fWorkMonday = TRUE;

     tmpStr = g_strdup_printf ("/PlanLibre/Ressources/Object%d/@working_tuesday", i-lenRsc);
     work = get_xml_rsc_as_int (doc, tmpStr, "working_tuesday");
     g_free (tmpStr);
     if(work == 0)
           newRsc.fWorkTuesday = FALSE;
     else
           newRsc.fWorkTuesday = TRUE;

     tmpStr = g_strdup_printf ("/PlanLibre/Ressources/Object%d/@working_wednesday", i-lenRsc);
     work = get_xml_rsc_as_int (doc, tmpStr, "working_wednesday");
     g_free (tmpStr);
     if(work == 0)
           newRsc.fWorkWednesday = FALSE;
     else
           newRsc.fWorkWednesday = TRUE;

     tmpStr = g_strdup_printf ("/PlanLibre/Ressources/Object%d/@working_thursday", i-lenRsc);
     work = get_xml_rsc_as_int (doc, tmpStr, "working_thursday");
     g_free (tmpStr);
     if(work == 0)
           newRsc.fWorkThursday = FALSE;
     else
           newRsc.fWorkThursday = TRUE;

     tmpStr = g_strdup_printf ("/PlanLibre/Ressources/Object%d/@working_friday", i-lenRsc);
     work = get_xml_rsc_as_int (doc, tmpStr, "working_friday");
     g_free (tmpStr);
     if(work == 0)
           newRsc.fWorkFriday = FALSE;
     else
           newRsc.fWorkFriday = TRUE;

     tmpStr = g_strdup_printf ("/PlanLibre/Ressources/Object%d/@working_saturday", i-lenRsc);
     work = get_xml_rsc_as_int (doc, tmpStr, "working_saturday");
     g_free (tmpStr);
     if(work == 0)
           newRsc.fWorkSaturday = FALSE;
     else
           newRsc.fWorkSaturday = TRUE;

     tmpStr = g_strdup_printf ("/PlanLibre/Ressources/Object%d/@working_sunday", i-lenRsc);
     work = get_xml_rsc_as_int (doc, tmpStr, "working_sunday");
     g_free (tmpStr);
     if(work == 0)
           newRsc.fWorkSunday = FALSE;
     else
           newRsc.fWorkSunday = TRUE;

     /* calendar calendars forced to 0 (zero default) when we IMPORT (append) ressources !!! */
     tmpStr = g_strdup_printf ("/PlanLibre/Ressources/Object%d/@calendar", i-lenRsc);
     cal = get_xml_rsc_as_int (doc, tmpStr, "calendar");
     g_free (tmpStr);
     if(cal>=0)
              newRsc.calendar = cal;

     /* hours and minutes worked by day */
     valeur2 = get_xml_attribute_value (g_strdup_printf ("/PlanLibre/Ressources/Object%d/@hours", i-lenRsc), "hours", doc);
     if((valeur2!=NULL) && (!append)) {
       hour_day = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
       g_free (valeur2);
       if((hour_day>0) && (hour_day<25))
              newRsc.day_hours = hour_day;
     }

     valeur2 = get_xml_attribute_value (g_strdup_printf ("/PlanLibre/Ressources/Object%d/@minutes", i-lenRsc), "minutes", doc);
     if((valeur2!=NULL) && (!append)) {
       min_day = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
       g_free (valeur2);
       if((min_day>=0) && (min_day<60))
           if(newRsc.day_hours<24)
               newRsc.day_mins = min_day;
           else
               newRsc.day_mins = 0;
     }

     /* color */
     tmpStr = g_strdup_printf ("/PlanLibre/Ressources/Object%d/@Color_red", i-lenRsc);
     color.red = get_xml_rsc_as_double (doc, tmpStr, "Color_red");
     g_free (tmpStr);
     tmpStr = g_strdup_printf ("/PlanLibre/Ressources/Object%d/@Color_green", i-lenRsc);
     color.green = get_xml_rsc_as_double (doc, tmpStr, "Color_green");    
     g_free (tmpStr);
     tmpStr = g_strdup_printf ("/PlanLibre/Ressources/Object%d/@Color_blue", i-lenRsc);
     color.blue = get_xml_rsc_as_double (doc, tmpStr, "Color_blue");
     g_free (tmpStr);
     /* pixbuf - avatar */
     valeur2 = get_xml_attribute_value (g_strdup_printf ("/PlanLibre/Ressources/Object%d/@Icon", i-lenRsc), "Icon", doc);
     if(valeur2!=NULL) {
         if(strlen(valeur2)>0)
            pix = get_pixbuf_from_code64 (valeur2); 
         g_free (valeur2);
      //   printf("données pixbuf largeur=%d hauteur=%d \n", 
        //      gdk_pixbuf_get_width(pix), gdk_pixbuf_get_height(pix)); 
     }
     /* update widgets BEFORE storage */
     cadres = rsc_new_widget (name, type, affi, cost, cost_type, color, data, TRUE, pix, &newRsc);

     /* update listbox */
     gtk_list_box_insert (GTK_LIST_BOX(listBoxRessources), GTK_WIDGET(cadres), i);// TODO mode append attention à i
     /* store in memory */
     rsc_store_to_app_data (i, name, mail, phone, reminder, 
                            type,  affi, cost,  cost_type, quantity, 
                            color, pix,  data, id, &newRsc, FALSE); // TODO mode append attention à i

     /* finalize - i.e. datas are transfered from app to memory */
     rsc_insert (i, &newRsc, data); // TODO mode append attention à i - ajouter &newRsc comme tasks 
     if(append) {
        /* undo engine */
        undo_datas *tmp_value;
        tmp_value = g_malloc (sizeof(undo_datas));
        tmp_value->opCode = OP_IMPORT_LIB_RSC;
        tmp_value->insertion_row = i;
        tmp_value->id = id;
        tmp_value->datas = NULL;/* nothing to store ? */
        tmp_value->list = NULL;
        tmp_value->groups = NULL;
        undo_push (CURRENT_STACK_RESSOURCES, tmp_value, data);
     }

     /* resets */
     if(pix!=NULL) {
        //g_object_unref (pix);/* Don't free, because local pix is only transfered, not reallocated 
     }
     pix = NULL;
     if(name) {
        g_free (name);
     }
     name = NULL;
     if(mail) {
        g_free (mail);
     }
     mail=NULL;
     if(phone) {
        g_free (phone);
     }
     phone=NULL;
     if(reminder) {
        g_free (reminder);
     }
     reminder=NULL;
  }/* next i */
 
  return rc;
}

/******************************
  local function to retrieve 
  "project's tasks" 
  from current xml file
******************************/
static gint get_tasks_from_xml (xmlDocPtr doc, APP_data *data)
{
  gint rc =0, nbTasks=0, curObj=0, i=0, id;
  gint s_day, s_month, s_year, e_day, e_month, e_year, l_day, l_month, l_year, days, hours, minutes, durationMode, calendar, group;
  gint64 timer_val;
  gchar *startDate = NULL, *endDate = NULL, *human_date, *tmpStr;
  GDate date_start, date_end;
  gchar *valeur2 = NULL, *name = NULL, *location = NULL;
  gint  type=0, priority=0,  status=0, category=0, delay = 0;
  gdouble progress;
  GdkRGBA  color;
  GtkWidget *cadres, *listBoxTasks;
  tasks_data temp, *tmp;
  GList *l;

  /* to get a value, we launch a Xpath request */
  /* now, we read how many tasks are stored in file  */
  
  valeur2 = get_xml_value ("/PlanLibre/Tasks/Counter/text()", doc);
  if(valeur2!=NULL) {
     nbTasks = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
     g_free (valeur2);
  }

  if(nbTasks<=0)
     return -1;
  /* we do a loop to read all resources */
  listBoxTasks = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasks"));

  for(i=0; i<nbTasks; i++) {
     id = -1;
      /* identifier */
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Id", i);
     id = get_xml_rsc_as_int  (doc, tmpStr, "Id");
     g_free (tmpStr);
     if(id<0)  {
          printf("* Tasks : critical error, bad object identifier *\n");
          return -1;
     }

     /* name */    
     valeur2 = get_xml_attribute_value (g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Name", i), "Name", doc);
     if(valeur2!=NULL) {
       name = g_strdup_printf ("%s", valeur2);
       g_free (valeur2);
       valeur2 = NULL;
     }

     /* location */   
     valeur2 = get_xml_attribute_value (g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Location", i), "Location", doc);
     if(valeur2!=NULL) {
       location = g_strdup_printf ("%s", valeur2);
       g_free (valeur2);
       valeur2 = NULL;
     }

     /* numeric values, for combos and progress */
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Type", i);
     type = get_xml_rsc_as_int (doc, tmpStr, "Type");
     g_free (tmpStr);
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Status", i);
     status = get_xml_rsc_as_int (doc, tmpStr, "Status");
     g_free (tmpStr);
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Priority", i);
     priority = get_xml_rsc_as_int (doc, tmpStr, "Priority");
     g_free (tmpStr);
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Category", i);
     category = get_xml_rsc_as_int (doc, tmpStr, "Category");
     g_free (tmpStr);
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Progress", i);
     progress = get_xml_rsc_as_double (doc, tmpStr, "Progress");
     g_free (tmpStr);

     /* timer logs */
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Timer_value", i);
     timer_val = get_xml_rsc_as_int (doc, tmpStr, "Timer_value");
     g_free (tmpStr);  
      if(timer_val<0)
        temp.timer_val = 0;
     else
        temp.timer_val = timer_val;
     /* color */
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Color_red", i);
     color.red = get_xml_rsc_as_double (doc, tmpStr, "Color_red");
     g_free (tmpStr);
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Color_green", i);
     color.green = get_xml_rsc_as_double (doc, tmpStr, "Color_green");
     g_free (tmpStr);
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Color_blue", i);
     color.blue = get_xml_rsc_as_double (doc, tmpStr, "Color_blue");  
     g_free (tmpStr);

     /* start date components */
     s_day =1;
     s_month = 1;
     s_year = 2018;

     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Start_date_day", i);
     s_day = get_xml_rsc_as_int (doc, tmpStr, "Start_date_day");
     g_free (tmpStr);
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Start_date_month", i);
     s_month = get_xml_rsc_as_int (doc, tmpStr, "Start_date_month");
     g_free (tmpStr);
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Start_date_year", i);
     s_year = get_xml_rsc_as_int (doc, tmpStr, "Start_date_year");
     g_free (tmpStr);

     g_date_set_dmy (&date_start, s_day, s_month, s_year);

     startDate = misc_convert_date_to_str (s_day, s_month, s_year);

     /* end date components */
     e_day = 1;
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@End_date_day", i);
     e_day = get_xml_rsc_as_int (doc, tmpStr, "End_date_day");
     g_free (tmpStr);

     e_month = 1;
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@End_date_month", i);
     e_month = get_xml_rsc_as_int (doc, tmpStr, "End_date_month");
     g_free (tmpStr);

     e_year = s_year+1;
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@End_date_year", i);
     e_year = get_xml_rsc_as_int (doc, tmpStr, "End_date_year");
     g_free (tmpStr);

     g_date_set_dmy (&date_end, e_day, e_month, e_year);
     endDate = misc_convert_date_to_str (e_day, e_month, e_year);

     /* deadline date components */
     l_day = 1;
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Lim_date_day", i);
     l_day = get_xml_rsc_as_int (doc, tmpStr, "Lim_date_day");
     g_free (tmpStr);

     l_month = 1;
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Lim_date_month", i);
     l_month = get_xml_rsc_as_int (doc, tmpStr, "Lim_date_month");
     g_free (tmpStr);

     l_year = s_year+1;
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Lim_date_year", i);
     l_year = get_xml_rsc_as_int (doc, tmpStr, "Lim_date_year");
     g_free (tmpStr);
// printf ("lim %d %d %d \n", l_day, l_month, l_year);
     /* delays */
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Delay", i);
     delay = get_xml_rsc_as_int (doc, tmpStr, "Delay");
     g_free (tmpStr);

     /* durations */
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Duration", i);
     durationMode = get_xml_rsc_as_int (doc, tmpStr, "Duration");
     g_free (tmpStr);
     if(durationMode<0 || durationMode>3) {
             durationMode = 0;/* set to default */
     }

     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Days", i);
     days = get_xml_rsc_as_int (doc, tmpStr, "Days");
     g_free (tmpStr);
     if(days<0)
        days = 0;

     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Hours", i);
     hours = get_xml_rsc_as_int (doc, tmpStr, "Hours");
     g_free (tmpStr);
     if(hours<0)
        hours = 0;
     if(days==0 && hours == 0)
         days = 1;

     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Minutes", i);
     minutes = get_xml_rsc_as_int (doc, tmpStr, "Minutes");
     g_free (tmpStr);
     if(minutes<0)
        minutes = 0;

     /* we check if task is tODAY in overdue status */
     if(misc_task_is_overdue (progress, &date_end) ) {
         status = TASKS_STATUS_DELAY;
     }

     /* calendar */
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Calendar", i);
     calendar = get_xml_rsc_as_int (doc, tmpStr, "Calendar");
     g_free (tmpStr);
     if(calendar<0)
        calendar = 0;
     /* update widgets BEFORE storage */
     human_date = misc_convert_date_to_friendly_str (&date_start);
     temp.type = type;
     temp.location = location;
     temp.group = -1; /* -1 means NO group */
     /* group */
     tmpStr = g_strdup_printf ("/PlanLibre/Tasks/Object%d/@Group", i);
     group = get_xml_rsc_as_int (doc, tmpStr, "Group");
     g_free (tmpStr);
     if(group>=0)
        temp.group = group;
     if(type==TASKS_TYPE_TASK) {
          cadres = tasks_new_widget (name, 
                           g_strdup_printf (_("%s, %s"), human_date, misc_duration_to_friendly_str (days, hours*60)), 
                                     progress, timer_val, priority, category, color, status, &temp, data);
     }

     if(type==TASKS_TYPE_GROUP) {
          tmpStr = g_strdup_printf (_("%d day(s)"), days);
          cadres = tasks_new_group_widget (name, tmpStr, 0, 0, 0, color, 0, &temp, data);
          g_free (tmpStr);
     }

     if(type==TASKS_TYPE_MILESTONE) {
          cadres = tasks_new_milestone_widget (name, human_date, 0, 0, 0, color, 0, &temp, data);
     }

     /* update listbox */
     gtk_list_box_insert (GTK_LIST_BOX(listBoxTasks), GTK_WIDGET(cadres), i);
     /* store in memory */
     tasks_store_to_tmp_data (i, name, g_date_get_day (&date_start), g_date_get_month (&date_start), g_date_get_year (&date_start), 
                            g_date_get_day (&date_end), g_date_get_month (&date_end), 
                            g_date_get_year (&date_end),
                            l_day, l_month, l_year, 
                            progress, priority, category, status, 
                            color, data, id, FALSE, durationMode, days, hours, minutes, calendar, delay, &temp);
    /* now, all values are stored in 'temp' */
    tasks_insert (i, data, &temp);
    tasks_add_display_ressources (temp.id, temp.cRscBox, data);

    /* resets */
    calendar = 0;
    days = 0;
    hours = 0;
    durationMode = 0;
    type = 0;
    status = 0;
    category = 0;
    priority = 0;
    progress = 0;
    color.red = 0;
    color.green = 0;
    color.blue = 0;
    if(name) {
        g_free (name);
    }
    name = NULL;

    if(location) {
        g_free (location);
    }
    location = NULL;

    g_free (startDate);
    g_free (endDate);
    g_free (human_date);
  }/* next i */
  /* we update groups */
  if(nbTasks>0) {
     for(i=0;i<nbTasks;i++) {
        l = g_list_nth (data->tasksList, i);
        tmp = (tasks_data *) l->data;
        if(tmp->type == TASKS_TYPE_GROUP) {
           tasks_update_group_limits (tmp->id, tmp->id, tmp->id, data);/* update also widgets mainly used for empty groups */
        }
        /* we add ancestors tasks to display */
printf ("avant display pour %s \n", tmp->name);
        tasks_add_display_tasks (tmp->id, tmp->cTaskBox, data);
     }/* next i */
  }
  return rc;
}

/******************************
  local function to retrieve 
  "project's calendars" 
  from current xml file
******************************/
static gint get_calendars_from_xml (xmlDocPtr doc, APP_data *data)
{
  gint rc =0, i=0, j, k, id, val, total = 510;
  gint nbCal = 0, nbDays, day, month, year, type = CAL_TYPE_NON_WORKED;
  gchar *valeur2 = NULL, *name = NULL, *tmpStr;
  gboolean fDays[7][4];/* flag : if days hasn't standard schedule */
  gint schedules[7][4][2]; /* minutes equivalent [day of week] [period] [start/end] */

  /* to get a value, we launch a Xpath request */
  /* now, we read how many links are stored in file  */

  valeur2 = get_xml_value ("/PlanLibre/Calendars/Counter/text()", doc);
  if(valeur2!=NULL) {
     nbCal = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
     g_free (valeur2);
  }

  /* now we read all calendars */
  for(i=0; i<nbCal; i++) {
     /* we get genereic values for each calendars  */
     tmpStr = g_strdup_printf ("/PlanLibre/Calendars/Object%d/@id", i);
     id = get_xml_rsc_as_int (doc, tmpStr, "id");
     g_free (tmpStr);

     tmpStr = g_strdup_printf ("/PlanLibre/Calendars/Object%d/@days", i);
     nbDays = get_xml_rsc_as_int (doc, tmpStr, "days");
     g_free (tmpStr);

     valeur2 = get_xml_attribute_value (g_strdup_printf ("/PlanLibre/Calendars/Object%d/@name", i), "name", doc);
     if(valeur2!=NULL) {
       name = g_strdup_printf ("%s", valeur2);
       g_free (valeur2);
     }
     else
        name = g_strdup_printf ("%s", _("Noname calendar"));

     /* schedules */

     for(j=0;j<7;j++) {
        for(k=0;k<4;k++) {
           fDays[j][k] = FALSE;
           schedules[j][k][0]= total;
           schedules[j][k][1]= total+120;
           total = total+180;

           tmpStr = g_strdup_printf ("/PlanLibre/Calendars/Object%d/Schedule%d/Period%d/@use", i, j, k);
           val = get_xml_rsc_as_int (doc, tmpStr, "use");
           g_free (tmpStr);
           if(val==1)
                  fDays[j][k] = TRUE;

           tmpStr = g_strdup_printf ("/PlanLibre/Calendars/Object%d/Schedule%d/Period%d/@start", i, j, k);
           val = get_xml_rsc_as_int (doc, tmpStr, "start");
           g_free (tmpStr);
           if(val>=0)
                  schedules[j][k][0]= val;

           tmpStr = g_strdup_printf ("/PlanLibre/Calendars/Object%d/Schedule%d/Period%d/@end", i, j, k);
           val = get_xml_rsc_as_int (doc, tmpStr, "end");
           g_free (tmpStr);
           if(val>=0)
                  schedules[j][k][1]= val;
        }/* next k */
     }/* next j */
     
     calendars_add_calendar (id, name, fDays, schedules, data);

     g_free (name);
     /* loop for each days */
     for(j=0;j<nbDays;j++) {
        day = 1;
        month = 1;
        year = 2010;

        tmpStr = g_strdup_printf ("/PlanLibre/Calendars/Object%d/Day%d/@day", i, j);
        day = get_xml_rsc_as_int (doc, tmpStr, "day");
        g_free (tmpStr);

        tmpStr = g_strdup_printf ("/PlanLibre/Calendars/Object%d/Day%d/@month", i, j);
        month = get_xml_rsc_as_int (doc, tmpStr, "month");
        g_free (tmpStr);

        tmpStr = g_strdup_printf ("/PlanLibre/Calendars/Object%d/Day%d/@year", i, j);
        year = get_xml_rsc_as_int (doc, tmpStr, "year");
        g_free (tmpStr);

        tmpStr = g_strdup_printf ("/PlanLibre/Calendars/Object%d/Day%d/@type", i, j);
        type = get_xml_rsc_as_int (doc, tmpStr, "type");
        g_free (tmpStr);

// printf ("jour de rang %d la date vaut %d/%d/%d le type est %d \n", j, day, month, year, type);

        calendars_add_new_marked_date (FALSE, i, day, month, year, type, data);
     }/* next j */
  }/* next i */
  return rc;
}

/******************************
  local function to retrieve 
  "project's links" 
  from current xml file
******************************/
static gint get_links_from_xml (xmlDocPtr doc, APP_data *data)
{
  gint rc =0, i=0;
  gint nbLinks, receiver, sender, type = LINK_TYPE_TSK_RSC, mode;
  gchar *valeur2 = NULL;

  /* to get a value, we launch a Xpath request */
  /* now, we read how many links are stored in file  */

  valeur2 = get_xml_value ("/PlanLibre/Links/Counter/text()", doc);
  if(valeur2) {
     nbLinks = (gint) g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
     g_free (valeur2);
  }

//  printf("compteur links ds fichier =%d\n", nbLinks);

  /* now, we add all links to database */
  for(i=0; i<nbLinks; i++) {
     sender = -1;
     receiver = -1;
     type = -1;
     mode = LINK_END_START;
     /* we get attributes for each 'link' object */
     valeur2 = get_xml_attribute_value (g_strdup_printf ("//Object%d/@receiver", i), "receiver", doc);
     if(valeur2) {
       receiver = g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
       g_free (valeur2);
     }

     valeur2 = get_xml_attribute_value (g_strdup_printf ("//Object%d/@sender", i), "sender", doc);
     if(valeur2) {
       sender = g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
       g_free (valeur2);
     }

     valeur2 = get_xml_attribute_value (g_strdup_printf ("//Object%d/@type", i), "type", doc);
     if(valeur2) {
       type = g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
       g_free (valeur2);
     }

     valeur2 = get_xml_attribute_value (g_strdup_printf ("//Object%d/@mode", i), "mode", doc);
     if(valeur2) {
       mode = g_ascii_strtod (g_strdup_printf ("%s", valeur2), NULL);
       g_free (valeur2);
     }
// printf("pour link n°=%d receiver = %d  sender =%d type =%d \n", i, receiver, sender, type);
     /* sender and receiver must be positive values ! */

     if((sender<0) || (receiver<0) || (type<0)) {
         fprintf (stderr, "* PlanLibre critical : bad values for link object #%d *\n", i);
     }
     else { /* we push (append) datas */
        links_append_element (receiver, sender, type, mode, data);
     }
  }/* next */
// printf("------ LINKS total objects pushed = %d ************\n",  g_list_length (data->linksList));
/* TODO : at this stage, we can't know if ID are correct since tasks and ressources aren't yet known */
  return rc;
}

/******************************
 Open a file
 from : https://www.julp.fr/articles/1-2-manipulation-d-un-document-xml-avec-la-methode-dom.html thanks !
******************************/
gint open_file (gchar *path_to_file, APP_data *data)
{
  xmlDocPtr doc;
  xmlNodePtr root;
  GError *err = NULL;
  gint rr = 0;
  GtkAdjustment *vadj;

  timeline_remove_ruler (data);
  /* clear calendars */
  calendars_free_calendars (data);
  /* clear dashboard */
  dashboard_remove_all (TRUE, data);
  /* clear "links" list */
  links_free_all (data);
  /* clear all tasks datas */
  tasks_remove_all_rows (data);
  tasks_free_all (data);/* goocanvasitem are removed here */

  /* clear all ressource datas */
  rsc_remove_all_rows (data);
  rsc_free_all (data);
  /* clear project's properties */
  properties_free_all (data);
  properties_set_default_dates (data);
  properties_set_default_dashboard (data);
  /* clear assignments */
  assign_remove_all_tasks (data);
  /* clear report area */
  report_clear_text (data->buffer, "left");
  report_default_text (data->buffer);
  /* set non'sensible menus and buttons */
  report_set_buttons_menus (FALSE, FALSE, data);

  /* OPen XML file */
  doc = xmlParseFile (path_to_file);
  if (doc == NULL) {
        fprintf (stderr, "* Critical : invalid XML Document *\n");
        return EXIT_FAILURE;
  }
  /* get ROOT access */
  root = xmlDocGetRootElement (doc);
  if (root == NULL) {
        fprintf (stderr, "* Critical : empty XML Document *\n");
        xmlFreeDoc (doc);
        return EXIT_FAILURE;
  }
  /* test if it's a PlanLibre document ? */
  if(strcmp (root->name, "PlanLibre") != 0) {
        fprintf (stderr, "* Critical : it isn't a PlanLibre XML Document *\n");
        xmlFreeDoc (doc);
        return EXIT_FAILURE;
  }

  /* read properties */
  rr = get_properties_from_xml (doc, data);
  /*  read links */
  rr = get_links_from_xml (doc, data);
  /* read ressources */
  rr = get_ressources_from_xml (doc, FALSE, data);
  /* read tasks */
  rr = get_tasks_from_xml (doc, data);
  /* read calendars */
  rr = get_calendars_from_xml (doc, data);  

  /* free memory */
  xmlFreeDoc (doc);

  /* we store extrems dates to update timeline */
  data->earliest = misc_get_earliest_date (data);
  data->latest = misc_get_latest_date (data);

  /* home page */
  home_today_date (data);
  home_project_infos (data);
  home_project_tasks (data);
  home_project_milestones (data);

  /* dashboard */
  dashboard_init (data);

  /* update assignments */
  assign_draw_all_visuals (data);

  /* update timeline */
  vadj = 
    gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(gtk_builder_get_object (data->builder, "scrolledwindowTimeline")));
  gtk_adjustment_set_value (GTK_ADJUSTMENT(vadj), 0);
//  timeline_hide_cursor (data); NO, because cursor is atatched to ruler !
  timeline_draw_calendar_ruler (0, data);
  timeline_draw_all_tasks (data);

  data->fProjectModified = FALSE;
  data->fProjectHasName = TRUE;
  misc_display_app_status (FALSE, data);
  return rr;
}

/**********************************
  Local function
  save properties part 
*********************************/
static gint save_properties (xmlNodePtr properties_node, APP_data *data)
{
  GKeyFile *keyString;
  gint ret = 0, dd, mm, yy;
  gchar buffer[100];/* for gdouble conversions */
  GError *error = NULL;
  xmlNodePtr prop_project = NULL, prop_manager = NULL, prop_orga = NULL, prop_dash = NULL;

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");
  dd = g_key_file_get_integer (keyString, "collaborate", "last_auto_mailing-day", NULL);
  mm = g_key_file_get_integer (keyString, "collaborate", "last_auto_mailing-month", NULL);
  yy = g_key_file_get_integer (keyString, "collaborate", "last_auto_mailing-year", NULL);

  prop_project = xmlNewChild (properties_node, NULL, BAD_CAST "Project", NULL);
  prop_manager = xmlNewChild (properties_node, NULL, BAD_CAST "Manager", NULL);
  prop_orga = xmlNewChild (properties_node, NULL, BAD_CAST "Organization", NULL);
  prop_dash = xmlNewChild (properties_node, NULL, BAD_CAST "Dashboard", NULL);

  /* nodes and attributes for project properties */
  /* contents project */
            if(data->properties.title) {
               xmlNewProp (prop_project, BAD_CAST "Title", BAD_CAST g_strdup_printf ("%s", data->properties.title));
            }
            else {
	          xmlNewProp (prop_project, BAD_CAST "Title", BAD_CAST "");
            }

            if(data->properties.summary) {
               xmlNewProp (prop_project, BAD_CAST "Summary", BAD_CAST g_strdup_printf ("%s", data->properties.summary));
            }
            else {
	          xmlNewProp (prop_project, BAD_CAST "Summary", BAD_CAST "");
            }

            if(data->properties.location) {
               xmlNewProp (prop_project,  BAD_CAST "Location", BAD_CAST g_strdup_printf ("%s", data->properties.location));
            }
            else {
	          xmlNewProp (prop_project,  BAD_CAST "Location", BAD_CAST "");
            }
            if(data->properties.gps) {
               xmlNewProp (prop_project, BAD_CAST "Gps", BAD_CAST g_strdup_printf ("%s", data->properties.gps));
            }
            else {
	          xmlNewProp (prop_project, BAD_CAST "Gps", BAD_CAST "");
            }
            if(data->properties.units) {
               xmlNewProp (prop_project, BAD_CAST "Units", BAD_CAST g_strdup_printf ("%s", data->properties.units));
            }
            else {
	         xmlNewProp (prop_project, BAD_CAST "Units", BAD_CAST "");
            }

            if(data->properties.proj_website) {
               xmlNewProp (prop_project, BAD_CAST "Website", BAD_CAST g_strdup_printf ("%s", data->properties.proj_website));
            }
            else {
	           xmlNewProp (prop_project, BAD_CAST "Website", BAD_CAST "");
            }
            
            /* convert budget value to string with 'forced' decimal point separator */
            if(data->properties.budget==0) {
              xmlNewProp (prop_project, BAD_CAST "Budget", BAD_CAST "0.0");
            } else {
               g_ascii_dtostr (buffer, 50, data->properties.budget );
               xmlNewProp (prop_project, BAD_CAST "Budget", BAD_CAST buffer);
            }
            /* project's dates - first, human form, second in TRUE internal format */
            if(data->properties.start_date) {
               xmlNewProp (prop_project, BAD_CAST "Start_date", BAD_CAST g_strdup_printf ("%s", data->properties.start_date));
            }
            else {
	          xmlNewProp (prop_project, BAD_CAST "Start_date", BAD_CAST "");
            }
            xmlNewProp (prop_project, BAD_CAST "Start_date_day", BAD_CAST g_strdup_printf ("%d", data->properties.start_nthDay));
            xmlNewProp (prop_project, BAD_CAST "Start_date_month", BAD_CAST g_strdup_printf ("%d", data->properties.start_nthMonth));
            xmlNewProp (prop_project, BAD_CAST "Start_date_year", BAD_CAST g_strdup_printf ("%d", data->properties.start_nthYear));

            if(data->properties.end_date) {
               xmlNewProp (prop_project, BAD_CAST "End_date", BAD_CAST g_strdup_printf ("%s", data->properties.end_date));
            }
            else {
	          xmlNewProp (prop_project, BAD_CAST "End_date", BAD_CAST "");
            }
            xmlNewProp (prop_project, BAD_CAST "End_date_day", BAD_CAST g_strdup_printf ("%d", data->properties.end_nthDay));
            xmlNewProp (prop_project, BAD_CAST "End_date_month", BAD_CAST g_strdup_printf ("%d", data->properties.end_nthMonth));
            xmlNewProp (prop_project, BAD_CAST "End_date_year", BAD_CAST g_strdup_printf ("%d", data->properties.end_nthYear));
            xmlNewProp (prop_project, BAD_CAST "Objects", BAD_CAST g_strdup_printf ("%d", data->objectsCounter));
            /* schedules */
            xmlNewProp (prop_project, BAD_CAST "Schedule_start", BAD_CAST g_strdup_printf ("%d", data->properties.scheduleStart));
            xmlNewProp (prop_project, BAD_CAST "Schedule_end", BAD_CAST g_strdup_printf ("%d", data->properties.scheduleEnd));
        /* automail */
        xmlNewProp (prop_project, BAD_CAST "automail_date_day", BAD_CAST g_strdup_printf ("%d", dd));
        xmlNewProp (prop_project, BAD_CAST "automail_date_month", BAD_CAST g_strdup_printf ("%d", mm));
        xmlNewProp (prop_project, BAD_CAST "automail_date_year", BAD_CAST g_strdup_printf ("%d", yy));

        /* nodes and attributes for manager properties */
        /* contents manager */
            if(data->properties.manager_name) {
               xmlNewProp (prop_manager, BAD_CAST "Name", BAD_CAST g_strdup_printf ("%s", data->properties.manager_name));
            }
            else {
	          xmlNewProp (prop_manager, BAD_CAST "Name", BAD_CAST "");
            }

            if(data->properties.manager_mail) {
               xmlNewProp (prop_manager, BAD_CAST "Mail", BAD_CAST g_strdup_printf ("%s", data->properties.manager_mail));
            }
            else {
	          xmlNewProp (prop_manager, BAD_CAST "Mail", BAD_CAST "");
            }

            if(data->properties.manager_phone) {
               xmlNewProp (prop_manager, BAD_CAST "Phone", BAD_CAST g_strdup_printf ("%s", data->properties.manager_phone));
            }
            else {
	          xmlNewProp (prop_manager, BAD_CAST "Phone", BAD_CAST "");
            }

        /* nodes and attributes for organization properties */
        /* contents organization */
            if(data->properties.org_name) {
               xmlNewProp (prop_orga, BAD_CAST "Name", BAD_CAST g_strdup_printf ("%s", data->properties.org_name));
            }
            else {
	          xmlNewProp (prop_orga, BAD_CAST "Name", BAD_CAST "");
            }           
            
            if(data->properties.logo) {
               /* conversion to base64 - we have a pointer on a gdkPixbuf */
// see : https://stackoverflow.com/questions/9453996/sending-an-image-to-ftp-server-from-gchar-buffer-libcurl
               gchar *buffer;
              // buffer = g_malloc0 (256000);
               gsize size;
               gboolean rc = gdk_pixbuf_save_to_buffer (data->properties.logo,
                           &buffer, &size, "jpeg", &error, NULL);
               /* we convert buffer to base64 */
               gchar *result;
               result = g_base64_encode (buffer, size);
               xmlNewProp (prop_orga, BAD_CAST "Logo", BAD_CAST g_strdup_printf ("%s", result));
               g_free (result);
               g_free (buffer);
            }
            else {
               xmlNewProp (prop_orga, BAD_CAST "Logo", BAD_CAST "");
            }
            
        /* dashboard */
            if(data->properties.labelDashCol0) {
               xmlNewProp (prop_dash, BAD_CAST "DashCol0", BAD_CAST g_strdup_printf ("%s", data->properties.labelDashCol0));	    
            }
            else {
               xmlNewProp (prop_dash, BAD_CAST "DashCol0", BAD_CAST _("To Do"));
            }
            
            if(data->properties.labelDashCol1) {
               xmlNewProp (prop_dash, BAD_CAST "DashCol1", BAD_CAST g_strdup_printf ("%s", data->properties.labelDashCol1));	    
            }
            else {
               xmlNewProp (prop_dash, BAD_CAST "DashCol1", BAD_CAST _("In progress"));
            }

            
            if(data->properties.labelDashCol2) {
               xmlNewProp (prop_dash, BAD_CAST "DashCol2", BAD_CAST g_strdup_printf ("%s", data->properties.labelDashCol2));	    
            }
            else {
               xmlNewProp (prop_dash, BAD_CAST "DashCol2", BAD_CAST _("Overdue"));
            }
           
            if(data->properties.labelDashCol3) {
               xmlNewProp (prop_dash, BAD_CAST "DashCol3", BAD_CAST g_strdup_printf ("%s", data->properties.labelDashCol3));	    
            }
            else {
               xmlNewProp (prop_dash, BAD_CAST "DashCol3", BAD_CAST _("Completed"));
            }

 

  return ret;
}

/**********************************
  Local function
  save Tasks part
*********************************/
static gint save_tasks (xmlNodePtr tasks_node, APP_data *data)
{
  gint i = 0, ret = 0;
  gchar buffer[100];/* for gdouble conversions */
  gchar *startDate = NULL, *endDate = NULL;
  GDate start_date, end_date;
  GList *l;
  tasks_data *tmp_tasks_datas;
  GError *error = NULL;
  xmlNodePtr tasks_count = NULL, tasks_object = NULL;

  /* tasks counter */
  tasks_count = xmlNewChild (tasks_node, NULL, BAD_CAST "Counter", NULL);
  xmlNodeSetContent (tasks_count, BAD_CAST g_strdup_printf ("%d", g_list_length (data->tasksList)));

  /* main loop */

  for(i=0;i< g_list_length (data->tasksList);i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     /* object identifier */
     tasks_object = xmlNewChild (tasks_node, NULL, BAD_CAST g_strdup_printf ("Object%d", i), NULL);
     xmlNewProp (tasks_object, BAD_CAST "Id", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->id));
     /* name */
     if(tmp_tasks_datas->name) {
        xmlNewProp (tasks_object, BAD_CAST "Name", BAD_CAST g_strdup_printf ("%s", tmp_tasks_datas->name));;
     }
     else
        xmlNewProp (tasks_object, BAD_CAST "Name", BAD_CAST "");
     /* location */
     if(tmp_tasks_datas->location) {
        xmlNewProp (tasks_object, BAD_CAST "Location", BAD_CAST g_strdup_printf ("%s", tmp_tasks_datas->location));
     }
     else
        xmlNewProp (tasks_object, BAD_CAST "Location", BAD_CAST "");

     /* type */
     xmlNewProp (tasks_object, BAD_CAST "Type", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->type));
     /* timer value */
     xmlNewProp (tasks_object, BAD_CAST "Timer_value", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->timer_val));
     /*  status */
     xmlNewProp (tasks_object, BAD_CAST "Status", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->status));
     /*  priority */
     xmlNewProp (tasks_object, BAD_CAST "Priority", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->priority));
     /*  category */
     xmlNewProp (tasks_object, BAD_CAST "Category", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->category));
     /*  progress value */
     g_ascii_dtostr (buffer, 50, tmp_tasks_datas->progress );
     xmlNewProp (tasks_object, BAD_CAST "Progress", BAD_CAST buffer);
     /* color */
     g_ascii_dtostr (buffer, 50, tmp_tasks_datas->color.red );
     xmlNewProp (tasks_object, BAD_CAST "Color_red", BAD_CAST buffer);
     g_ascii_dtostr (buffer, 50, tmp_tasks_datas->color.green );
     xmlNewProp (tasks_object, BAD_CAST "Color_green", BAD_CAST buffer);
     g_ascii_dtostr (buffer, 50, tmp_tasks_datas->color.blue );
     xmlNewProp (tasks_object, BAD_CAST "Color_blue", BAD_CAST buffer);
     /* dates */
     startDate = misc_convert_date_to_str (tmp_tasks_datas->start_nthDay, 
                                           tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
     g_date_set_dmy (&start_date, tmp_tasks_datas->start_nthDay, 
                                           tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
     if(startDate) {
        xmlNewProp (tasks_object, BAD_CAST "Start_date", BAD_CAST g_strdup_printf ("%s", startDate));
     }
     else {
	xmlNewProp (tasks_object, BAD_CAST "Start_date", BAD_CAST "");
     }
     xmlNewProp (tasks_object, BAD_CAST "Start_date_day", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->start_nthDay));
     xmlNewProp (tasks_object, BAD_CAST "Start_date_month", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->start_nthMonth));
     xmlNewProp (tasks_object, BAD_CAST "Start_date_year", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->start_nthYear));

     g_date_set_dmy (&end_date, tmp_tasks_datas->end_nthDay, 
                                           tmp_tasks_datas->end_nthMonth, tmp_tasks_datas->end_nthYear);
     endDate = misc_convert_date_to_str(tmp_tasks_datas->end_nthDay, tmp_tasks_datas->end_nthMonth, tmp_tasks_datas->end_nthYear);
     if(endDate) {
        xmlNewProp (tasks_object, BAD_CAST "End_date", BAD_CAST g_strdup_printf ("%s", endDate));
     }
     else {
        xmlNewProp (tasks_object, BAD_CAST "End_date", BAD_CAST "");
     }
     xmlNewProp (tasks_object, BAD_CAST "End_date_day", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->end_nthDay));
     xmlNewProp (tasks_object, BAD_CAST "End_date_month", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->end_nthMonth));
     xmlNewProp (tasks_object, BAD_CAST "End_date_year", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->end_nthYear));
     /* deadline and not before date */
     xmlNewProp (tasks_object, BAD_CAST "Lim_date_day", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->lim_nthDay));
     xmlNewProp (tasks_object, BAD_CAST "Lim_date_month", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->lim_nthMonth));
     xmlNewProp (tasks_object, BAD_CAST "Lim_date_year", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->lim_nthYear));
     /* delays */
     xmlNewProp (tasks_object, BAD_CAST "Delay", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->delay));
     g_free (startDate);
     g_free (endDate);
     startDate = NULL;
     endDate = NULL;
     /* if sart == end we must add 1 day to end */
/*
     gint cmp1 = g_date_compare (&start_date, &end_date);
     if(cmp1<=0) {
        g_date_set_dmy (&end_date, tmp_tasks_datas->start_nthDay, 
                                           tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
        g_date_add_days (&end_date, 1);
        tmp_tasks_datas->end_nthDay = g_date_get_day (&end_date);
        tmp_tasks_datas->end_nthMonth = g_date_get_month (&end_date);
        tmp_tasks_datas->end_nthYear = g_date_get_year (&end_date);
     }
*/

     /* duration, in hours */
     xmlNewProp (tasks_object, BAD_CAST "Duration", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->durationMode));
     xmlNewProp (tasks_object, BAD_CAST "Days", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->days));
     xmlNewProp (tasks_object, BAD_CAST "Hours", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->hours));
     xmlNewProp (tasks_object, BAD_CAST "Minutes", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->minutes));
     /* calendar */
     xmlNewProp (tasks_object, BAD_CAST "Calendar", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->calendar));
     /* group */
     xmlNewProp (tasks_object, BAD_CAST "Group", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->group));
  }/* next i */

  return ret;
}

/**********************************
  Local function
  save ressources part 
*********************************/
static gint save_ressources (xmlNodePtr rsc_node, APP_data *data)
{
  gint i = 0, ret=0;
  gchar buffer[100];/* for gdouble conversions */
  GList *l;
  GError *error = NULL;
  xmlNodePtr rsc_count = NULL, rsc_object = NULL, rsc_object_sub = NULL;
  rsc_datas *tmp_rsc_datas;

  /* ressources counter */
  rsc_count = xmlNewChild (rsc_node, NULL, BAD_CAST "Counter", NULL);
  xmlNodeSetContent (rsc_count, BAD_CAST g_strdup_printf ("%d", g_list_length (data->rscList)));

  /* main loop */

  for(i=0;i< g_list_length (data->rscList);i++) {
     l = g_list_nth (data->rscList, i);
     tmp_rsc_datas = (rsc_datas *)l->data;
     rsc_object = xmlNewChild (rsc_node, NULL, BAD_CAST g_strdup_printf ("Object%d", i), NULL);
     xmlNewProp (rsc_object, BAD_CAST "Id", BAD_CAST g_strdup_printf ("%d",tmp_rsc_datas->id));
     /* name */
     if(tmp_rsc_datas->name) {
        xmlNewProp (rsc_object, BAD_CAST "Name", BAD_CAST g_strdup_printf ("%s",tmp_rsc_datas->name));
     }
     else
        xmlNewProp (rsc_object, BAD_CAST "Name", BAD_CAST "");
     /* mail */
     if(tmp_rsc_datas->mail) {
        xmlNewProp (rsc_object, BAD_CAST "Mail", BAD_CAST g_strdup_printf ("%s",tmp_rsc_datas->mail));
     }
     else
        xmlNewProp (rsc_object, BAD_CAST "Mail", BAD_CAST "");
     /* phone */
     if(tmp_rsc_datas->phone) {
        xmlNewProp (rsc_object, BAD_CAST "Phone", BAD_CAST g_strdup_printf ("%s",tmp_rsc_datas->phone));
     }
     else
        xmlNewProp (rsc_object, BAD_CAST "Phone", BAD_CAST "");
     /* reminder */
     if(tmp_rsc_datas->reminder) {
        xmlNewProp (rsc_object, BAD_CAST "Reminder", BAD_CAST g_strdup_printf ("%s",tmp_rsc_datas->reminder));
     }
     else
        xmlNewProp (rsc_object, BAD_CAST "Reminder", BAD_CAST "");
     /* integer & float */
     /*  affiliate */
     xmlNewProp (rsc_object, BAD_CAST "Affiliate", BAD_CAST g_strdup_printf ("%d",tmp_rsc_datas->affiliate));
     /*  type */
     xmlNewProp (rsc_object, BAD_CAST "Type", BAD_CAST g_strdup_printf ("%d",tmp_rsc_datas->type));
     /*  cost_type */
     xmlNewProp (rsc_object, BAD_CAST "Cost_type", BAD_CAST g_strdup_printf ("%d",tmp_rsc_datas->cost_type));
     /*  cost value */
     g_ascii_dtostr (buffer, 50, tmp_rsc_datas->cost);
     xmlNewProp (rsc_object, BAD_CAST "Cost_value", BAD_CAST buffer);
     /* quantity per resource - TODO */
     g_ascii_dtostr (buffer, 50, tmp_rsc_datas->quantity);
     xmlNewProp (rsc_object, BAD_CAST "Quantity", BAD_CAST buffer);     
     /* color */
     g_ascii_dtostr (buffer, 50, tmp_rsc_datas->color.red );
     xmlNewProp (rsc_object, BAD_CAST "Color_red", BAD_CAST buffer);
     g_ascii_dtostr (buffer, 50, tmp_rsc_datas->color.green );
     xmlNewProp (rsc_object, BAD_CAST "Color_green", BAD_CAST buffer);
     g_ascii_dtostr (buffer, 50, tmp_rsc_datas->color.blue );
     xmlNewProp (rsc_object, BAD_CAST "Color_blue", BAD_CAST buffer);
     /* concurrent usage */
     if(tmp_rsc_datas->fAllowConcurrent)     
		xmlNewProp (rsc_object, BAD_CAST "concurrent_usage", BAD_CAST "1"); 
	 else
		xmlNewProp (rsc_object, BAD_CAST "concurrent_usage", BAD_CAST "0"); 	 
     /* working days */
     if(tmp_rsc_datas->fWorkMonday)
          xmlNewProp (rsc_object, BAD_CAST "working_monday", BAD_CAST "1");       
     else
          xmlNewProp (rsc_object, BAD_CAST "working_monday", BAD_CAST "0");  

     if(tmp_rsc_datas->fWorkTuesday)
          xmlNewProp (rsc_object, BAD_CAST "working_tuesday", BAD_CAST "1");       
     else
          xmlNewProp (rsc_object, BAD_CAST "working_tuesday", BAD_CAST "0");  

     if(tmp_rsc_datas->fWorkWednesday)
          xmlNewProp (rsc_object, BAD_CAST "working_wednesday", BAD_CAST "1");          
     else
          xmlNewProp (rsc_object, BAD_CAST "working_wednesday", BAD_CAST "0");   

     if(tmp_rsc_datas->fWorkThursday)
          xmlNewProp (rsc_object, BAD_CAST "working_thursday", BAD_CAST "1");          
     else
          xmlNewProp (rsc_object, BAD_CAST "working_thursday", BAD_CAST "0"); 

     if(tmp_rsc_datas->fWorkFriday)
          xmlNewProp (rsc_object, BAD_CAST "working_friday", BAD_CAST "1");          
     else
          xmlNewProp (rsc_object, BAD_CAST "working_friday", BAD_CAST "0"); 

     if(tmp_rsc_datas->fWorkSaturday)
          xmlNewProp (rsc_object, BAD_CAST "working_saturday", BAD_CAST "1");          
     else
          xmlNewProp (rsc_object, BAD_CAST "working_saturday", BAD_CAST "0"); 

     if(tmp_rsc_datas->fWorkSunday)
          xmlNewProp (rsc_object, BAD_CAST "working_sunday", BAD_CAST "1");          
     else
          xmlNewProp (rsc_object, BAD_CAST "working_sunday", BAD_CAST "0"); 

     xmlNewProp (rsc_object, BAD_CAST "calendar", BAD_CAST g_strdup_printf ("%d", tmp_rsc_datas->calendar));
     xmlNewProp (rsc_object, BAD_CAST "hours", BAD_CAST g_strdup_printf ("%d", tmp_rsc_datas->day_hours));
     xmlNewProp (rsc_object, BAD_CAST "minutes", BAD_CAST g_strdup_printf ("%d", tmp_rsc_datas->day_mins));

     /* icon - avatar */
     if(tmp_rsc_datas->pix) {
         /* conversion to base64 - we have a pointer on a gdkPixbuf */
         gchar *buffer2;
         gsize size;
         gboolean rc = gdk_pixbuf_save_to_buffer (tmp_rsc_datas->pix,
                           &buffer2, &size, "jpeg", &error, NULL);
         /* we convert buffer to base64 */
         gchar *result;
         result = g_base64_encode (buffer2, size);
         xmlNewProp (rsc_object, BAD_CAST "Icon", BAD_CAST g_strdup_printf ("%s", result));
         g_free (result);
         g_free (buffer2);
     }
     else {
         xmlNewProp (rsc_object, BAD_CAST "Icon", BAD_CAST "");
     }
  }/* next i */
  return ret;
}

/**********************************
  Local function
  save links part 
*********************************/
static gint save_links (xmlNodePtr links_node, APP_data *data)
{
  gint i = 0, ret=0;
  gchar buffer[100];/* for gdouble conversions */
  GList *l;
  link_datas *tmp_links_datas;
  GError *error = NULL;
  xmlNodePtr links_count = NULL;
  xmlNodePtr links_object = NULL;

  /* links counter */
  links_count = xmlNewChild (links_node, NULL, BAD_CAST "Counter", NULL);
  xmlNodeSetContent (links_count, BAD_CAST g_strdup_printf ("%d", g_list_length (data->linksList)));

  /* main loop */
  for(i=0 ; i< g_list_length (data->linksList); i++) {
     // printf("SAVE : blc links= %d \n", i);
     l = g_list_nth (data->linksList, i);
     tmp_links_datas = (link_datas *)l->data;
     links_object = xmlNewChild (links_node, NULL, BAD_CAST g_strdup_printf ("Object%d", i), NULL);

     xmlNewProp (links_object, BAD_CAST "receiver", BAD_CAST  g_strdup_printf ("%d", tmp_links_datas->receiver));
     xmlNewProp (links_object, BAD_CAST "sender", BAD_CAST g_strdup_printf ("%d", tmp_links_datas->sender));    
     xmlNewProp (links_object, BAD_CAST "type", BAD_CAST g_strdup_printf ("%d", tmp_links_datas->type));    
     xmlNewProp (links_object, BAD_CAST "mode", BAD_CAST g_strdup_printf ("%d", LINK_END_START));   
  }/* next */

  return ret;
}

/*********************************
  Local function
  save calendars part 
*********************************/
static gint save_calendars (xmlNodePtr calendars_node, APP_data *data)
{
  gint i = 0, ret=0, j, k, val, hour1, hour2;
  gchar buffer[100];/* for gdouble conversions */
  GList *l, *l1;
  calendar *tmp_calendars_datas;
  GError *error = NULL;
  xmlNodePtr calendars_count = NULL, calendars_object = NULL, schedule_object =NULL, period_object = NULL;
  xmlNodePtr days_object = NULL;
  calendar_element *element;

  /* links counter */
  calendars_count = xmlNewChild (calendars_node, NULL, BAD_CAST "Counter", NULL);
  xmlNodeSetContent (calendars_count, BAD_CAST g_strdup_printf ("%d", g_list_length (data->calendars)));

  /* main loop */
  for(i=0 ; i< g_list_length (data->calendars); i++) {
     l = g_list_nth (data->calendars, i);
     tmp_calendars_datas = (calendar *)l->data;
     calendars_object = xmlNewChild (calendars_node, NULL, BAD_CAST g_strdup_printf ("Object%d", i), NULL);

     xmlNewProp (calendars_object, BAD_CAST "id", BAD_CAST  g_strdup_printf ("%d", tmp_calendars_datas->id));
     xmlNewProp (calendars_object, BAD_CAST "name", BAD_CAST g_strdup_printf ("%s", tmp_calendars_datas->name));       
     xmlNewProp (calendars_object, BAD_CAST "days", BAD_CAST g_strdup_printf ("%d", g_list_length(tmp_calendars_datas->list)));
     /* schedules */
     for(j=0;j<7;j++) {
        schedule_object = xmlNewChild (calendars_object, NULL, BAD_CAST g_strdup_printf ("Schedule%d", j), NULL);
        for(k = 0; k < 4; k++) {
          period_object = xmlNewChild (schedule_object, NULL, BAD_CAST g_strdup_printf ("Period%d", k), NULL); 
	      val = 0;
	      if(tmp_calendars_datas->fDays[j][k])
	          val = 1;
          hour1 = tmp_calendars_datas->schedules[j][k][0];
          hour2 = tmp_calendars_datas->schedules[j][k][1];

          if(hour1>1438)
            hour1 = 1438;
          if(hour2>1439)
            hour2 = 1439;
 
	      xmlNewProp (period_object, BAD_CAST "use", BAD_CAST  g_strdup_printf ("%d", val));
	      xmlNewProp (period_object, BAD_CAST "start", BAD_CAST  g_strdup_printf ("%d", hour1));
	      xmlNewProp (period_object, BAD_CAST "end", BAD_CAST  g_strdup_printf ("%d", hour2));
        }/* next k */
     }/* next j */
     /* each days */
     for(j=0; j<g_list_length (tmp_calendars_datas->list); j++) {
       days_object = xmlNewChild (calendars_object, NULL, BAD_CAST g_strdup_printf ("Day%d", j), NULL);
       l1 = g_list_nth (tmp_calendars_datas->list, j);
       element = (calendar_element *) l1->data;
       xmlNewProp (days_object, BAD_CAST "type", BAD_CAST  g_strdup_printf ("%d", element->type));
       xmlNewProp (days_object, BAD_CAST "day", BAD_CAST  g_strdup_printf ("%d", element->day));
       xmlNewProp (days_object, BAD_CAST "month", BAD_CAST  g_strdup_printf ("%d", element->month));
       xmlNewProp (days_object, BAD_CAST "year", BAD_CAST  g_strdup_printf ("%d", element->year));
     }/* next j */
  }/* next */

  return ret;
}

/**********************************
  save current project datas 
  in an XML compliant file
**********************************/
void save (gchar *filename, APP_data *data)
{
    gchar buff[256], buffer[100];/* for gdouble conversions */
    xmlDocPtr doc = NULL;       /* document pointer */
    xmlNodePtr root_node = NULL, node = NULL, node1 = NULL;/* node pointers */
    xmlNodePtr properties_node =NULL, rsc_node = NULL, tasks_node = NULL, partner_node = NULL, data_node = NULL;/* node pointers */
    xmlNodePtr links_node = NULL, calendars_node = NULL;
    gint i, j, ret;

    /* avoid re-entrant call */
    fStartedSaving = TRUE;

    LIBXML_TEST_VERSION;
    /* 
     * Creates a new document, a node and set it as a root node
     */
    doc = xmlNewDoc (BAD_CAST "1.0");
    root_node = xmlNewNode (NULL, BAD_CAST "PlanLibre");
    xmlDocSetRootElement (doc, root_node);
    /*
     * Creates a DTD declaration. Isn't mandatory. 
     */
    xmlCreateIntSubset (doc, BAD_CAST "PlanLibre", NULL, BAD_CAST "PlanLibre.dtd");
    
    /* we build the main nodes */
    /* properties */
    properties_node = xmlNewChild (root_node, NULL, BAD_CAST "Properties", NULL);
    ret = save_properties (properties_node , data);

    /* ressources */
    rsc_node = xmlNewChild (root_node, NULL, BAD_CAST "Ressources", NULL);
    ret = save_ressources (rsc_node, data);

    /* tasks */   
    tasks_node = xmlNewChild (root_node, NULL, BAD_CAST "Tasks", NULL);
    ret = save_tasks (tasks_node, data);

    /* links */
    links_node = xmlNewChild (root_node, NULL, BAD_CAST "Links", NULL);
    ret = save_links (links_node, data);

    /* calendars */
    calendars_node = xmlNewChild (root_node, NULL, BAD_CAST "Calendars", NULL);
    ret = save_calendars (calendars_node, data);

    /* Dumping document to file */
    xmlSaveFormatFileEnc (filename, doc, "UTF-8", 1);

    /*free the document */
    xmlFreeDoc (doc);

    /* Free the global variables that may
     have been allocated by the parser. */
    xmlCleanupParser ();
    /* set-up flags */
    data->fProjectModified = FALSE; 
    data->fProjectHasName = TRUE;
    misc_display_app_status (FALSE, data);

    fStartedSaving = FALSE;
}

/*********************************
  load from file dialog 
*********************************/
GtkWidget *create_loadFileDialog (GtkWidget *win, APP_data *data, gchar *sFileType)
{
  GtkWidget *loadFileDialog, *dialog_vbox6, *dialog_action_area6;
  GtkWidget *button43, *image43, *button44, *image44;

  loadFileDialog = gtk_file_chooser_dialog_new (sFileType, 
                              GTK_WINDOW(win), GTK_FILE_CHOOSER_ACTION_OPEN, NULL);
  g_object_set (loadFileDialog, "local-only", FALSE,  NULL);
  gtk_window_set_modal (GTK_WINDOW (loadFileDialog), TRUE);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (loadFileDialog), TRUE);
  gtk_window_set_icon_name (GTK_WINDOW (loadFileDialog), "gtk-file");
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (loadFileDialog), TRUE);
  gtk_window_set_type_hint (GTK_WINDOW (loadFileDialog), GDK_WINDOW_TYPE_HINT_DIALOG);

  dialog_vbox6 = gtk_dialog_get_content_area (GTK_DIALOG (loadFileDialog));
  gtk_widget_show (dialog_vbox6);

  dialog_action_area6 = gtk_dialog_get_action_area (GTK_DIALOG (loadFileDialog));
  gtk_widget_show (dialog_action_area6);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area6), GTK_BUTTONBOX_END);

  button43 = gtk_button_new_with_label (_("Cancel"));
  image43 = gtk_image_new_from_icon_name ("gtk-cancel",  GTK_ICON_SIZE_BUTTON);
  gtk_button_set_always_show_image (GTK_BUTTON (button43), TRUE);
  gtk_button_set_image (GTK_BUTTON (button43), image43);
  gtk_widget_show (button43);
  gtk_dialog_add_action_widget (GTK_DIALOG (loadFileDialog), button43, GTK_RESPONSE_CANCEL);
  gtk_widget_set_can_default (button43, TRUE);

  button44 = gtk_button_new_with_label (_("Open"));
  image44 = gtk_image_new_from_icon_name ("gtk-open",  GTK_ICON_SIZE_BUTTON);
  gtk_button_set_always_show_image (GTK_BUTTON (button44), TRUE);
  gtk_button_set_image (GTK_BUTTON (button44), image44);
  gtk_widget_show (button44);
  gtk_dialog_add_action_widget (GTK_DIALOG (loadFileDialog), button44, GTK_RESPONSE_OK);
  gtk_widget_set_can_default (button44, TRUE);

  gtk_widget_grab_default (button44);
  return loadFileDialog;
}

/*********************************
  save to file dialog 
*********************************/
GtkWidget*
create_saveFileDialog (gchar *sFileType, APP_data *data)
{
  GtkWidget *saveFileDialog, *dialog_vbox6;
  GtkWidget *dialog_action_area6, *button43;
  GtkWidget *image43, *button44, *image44;

  saveFileDialog = gtk_file_chooser_dialog_new (sFileType, 
                                    GTK_WINDOW(data->appWindow),
                                    GTK_FILE_CHOOSER_ACTION_SAVE, NULL);
  g_object_set (saveFileDialog, "local-only", FALSE,  NULL);
  gtk_window_set_modal (GTK_WINDOW (saveFileDialog), TRUE);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (saveFileDialog), TRUE);
  gtk_window_set_icon_name (GTK_WINDOW (saveFileDialog), "gtk-file");
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (saveFileDialog), TRUE);
  gtk_window_set_type_hint (GTK_WINDOW (saveFileDialog), GDK_WINDOW_TYPE_HINT_DIALOG);

  dialog_vbox6 = gtk_dialog_get_content_area (GTK_DIALOG (saveFileDialog));
  gtk_widget_show (dialog_vbox6);

  dialog_action_area6 = gtk_dialog_get_action_area (GTK_DIALOG (saveFileDialog));
  gtk_widget_show (dialog_action_area6);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area6), GTK_BUTTONBOX_END);

  button43 = gtk_button_new_with_label (_("Cancel"));
  image43 = gtk_image_new_from_icon_name ("gtk-cancel",  GTK_ICON_SIZE_BUTTON);
  gtk_button_set_always_show_image (GTK_BUTTON (button43), TRUE);
  gtk_button_set_image (GTK_BUTTON (button43), image43);
  gtk_widget_show (button43);
  gtk_dialog_add_action_widget (GTK_DIALOG (saveFileDialog), button43, GTK_RESPONSE_CANCEL);
  gtk_widget_set_can_default (button43, TRUE);

  button44 = gtk_button_new_with_label (_("Save"));
  image44 = gtk_image_new_from_icon_name ("gtk-save",  GTK_ICON_SIZE_BUTTON);
  gtk_button_set_always_show_image (GTK_BUTTON (button44), TRUE);
  gtk_button_set_image (GTK_BUTTON (button44), image44);
  gtk_widget_show (button44);
  gtk_dialog_add_action_widget (GTK_DIALOG (saveFileDialog), button44, GTK_RESPONSE_OK);
  gtk_widget_set_can_default (button44, TRUE);
 
  gtk_widget_grab_default (button44);
  return saveFileDialog;
}

/***********************************************
  append from a ressource library

***********************************************/
gint files_append_rsc_from_library (gchar *path_to_file, APP_data *data)
{
  xmlDocPtr doc;
  xmlNodePtr root;
  gint rr = 0;
 
  /* OPen XML file */
  doc = xmlParseFile (path_to_file);
  if (doc == NULL) {
        fprintf (stderr, "* Critical : invalid XML Document *\n");
        return EXIT_FAILURE;
  }
  /* get ROOT access */
  root = xmlDocGetRootElement (doc);
  if (root == NULL) {
        fprintf (stderr, "* Critical : empty XML Document *\n");
        xmlFreeDoc (doc);
        return EXIT_FAILURE;
  }
  /* test if it's a PlanLibre document ? */
  if(strcmp(root->name, "PlanLibre")!=0) {
        fprintf (stderr, "* Critical : it isn't a PlanLibre XML Document *\n");
        xmlFreeDoc (doc);
        return EXIT_FAILURE;
  }

  rr = get_ressources_from_xml (doc, TRUE, data);

  rsc_autoscroll (0.8, data);
  /* free memory */
  xmlFreeDoc (doc);

  data->fProjectModified = TRUE;
  misc_display_app_status (TRUE, data);
  return rr;
}

/***************************************
 PUBLIC 
 build a "summary" for current project
 outputs a 100 chars limited string
****************************************/
gchar *files_get_project_summary (APP_data *data)
{
  gchar *tmpStr = NULL, *title = NULL, *summary = NULL, *str = NULL;
  gchar buffer[204];/* for utf/8 : 4xmaxlen */

  if(data->properties.title == NULL)
     title = g_strdup_printf ("%s", _("Project without title"));
  else
     title = g_strdup_printf ("%s", data->properties.title);

  if(data->properties.summary==NULL)
           summary = g_strdup_printf ("%s", _("No summary"));
     else
           summary = g_strdup_printf ("%s", data->properties.summary);

  str = g_strdup_printf (_("Title : %s\nSummary : %s"), title, summary);
  g_utf8_strncpy  (&buffer, str, 99);
  tmpStr = g_strdup_printf ("%s", buffer);

  g_free (str);
  g_free (title);
  g_free (summary);

  return tmpStr;

}

/********************************************
  PUBLIC : given a path to a PlanLibre file,
  copy a snapshot in system temporary
  directory, update menus 
  if no error, returns 0 (zero)
********************************************/
gint files_copy_snapshot (gchar *path_to_source, APP_data *data)
{
  gint c, rc;
  FILE *src1, *dest1;
  gchar *path_to_dest, *new_filename;
  GtkWidget *menuItem;

  if (!path_to_source)
     return -1;

  if ((src1 = fopen (path_to_source, "rb")) == NULL) {
	    printf ("* PLanLibre Critical : could not open file to take a snapshot ! *\n");
	    return -1;
  }   

  /* we build destination path */
  new_filename = g_strdup_printf ("%s%s", _("backup_of_"), g_path_get_basename (path_to_source));
  path_to_dest = g_build_filename (g_get_tmp_dir (), new_filename, NULL);


  if ((dest1 = fopen (path_to_dest, "wb")) == NULL) {
	    printf ("* PLanLibre Critical : could not open a path in temporary directory for snapshot ! *\n");
            g_free (path_to_dest);
            g_free (new_filename);
	    return -1;
  }   

  while ((c=fgetc(src1))!= EOF) {
    fputc (c, dest1);
  }

  /* we remove previous snapshot if exists */
  if(data->fProjectHasSnapshot) {
     if(data->path_to_snapshot) {
         rc = g_remove (data->path_to_snapshot);
         if(rc!=0) {
            printf ("* PlanLibre critical : can't remove snapshot from temporary directory *\n");
         }
         g_free (data->path_to_snapshot);
         data->path_to_snapshot = NULL;
     }
     data->fProjectHasSnapshot = FALSE;
  }
  /* we store new path to snapshot */
  data->path_to_snapshot = g_strdup_printf ("%s", path_to_dest);
  data->fProjectHasSnapshot = TRUE;
  menuItem = GTK_WIDGET(gtk_builder_get_object (data->builder, "menuRestoreSnapshot"));
  gtk_widget_set_sensitive (GTK_WIDGET (menuItem), TRUE);

  fclose (src1);
  fclose (dest1);
  g_free (path_to_dest);
  g_free (new_filename);
  return 0;
}



/*********************************
  Local function
  save calendars part to project
  * xml file
*********************************/
static gint save_tasks_to_project (xmlNodePtr tasks_node, APP_data *data)
{
  gint i = 0, ret = 0, wbs_count = 0, priority, hour1, min1, hour2, min2;
  GDate current_date;
  GDateTime *my_time;
  gchar buffer[100];/* for gdouble conversions */
  gchar *startDate = NULL, *endDate = NULL;
  GDate start_date, end_date;
  GList *l;
  tasks_data *tmp_tasks_datas;
  GError *error = NULL;
  xmlNodePtr tasks_object = NULL, UID_node = NULL, id_node = NULL, name_node = NULL, type_node = NULL, is_null_node = NULL;
  xmlNodePtr milestone_node = NULL, creation_node = NULL, wbs_node = NULL, outline_number_node = NULL, outline_level_node = NULL;
  xmlNodePtr priority_node = NULL, start_node = NULL, finish_node = NULL, duration_node = NULL, duration_format_node = NULL;
  xmlNodePtr resume_node = NULL, effort_node = NULL, recur_node = NULL, over_node = NULL, estimated_node = NULL; 
  xmlNodePtr summary_node = NULL, critical_node = NULL, sub_node = NULL, sub_read_node = NULL, external_node = NULL;
  xmlNodePtr fixed_accrual_node = NULL, remain_dur_node = NULL, constraint_type_node = NULL, cal_UID_node = NULL;
  xmlNodePtr constraint_date_node = NULL, level_assign_node = NULL, leval_can_split_node = NULL, leval_delay_node = NULL;
  xmlNodePtr level_delay_format_mode = NULL, ignore_rsc_cal_mode = NULL, hidebar_node = NULL, rollup_node = NULL;
  xmlNodePtr earned_value_node = NULL;
  
  g_date_set_time_t (&current_date, time (NULL)); /* date of today */
  /* get current time, should be unrefed once used */
  my_time = g_date_time_new_now_local ();  
  /* main loop */

  for(i = 0; i < g_list_length (data->tasksList); i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     /* task start of declaration */
     tasks_object = xmlNewChild (tasks_node, NULL, BAD_CAST "Task", NULL);
     tmp_tasks_datas = (tasks_data *)l->data;
     /* task identifier */
     UID_node = xmlNewChild (tasks_object, NULL, BAD_CAST "UID", NULL);
     id_node = xmlNewChild (tasks_object, NULL, BAD_CAST "ID", NULL);
     
     xmlNodeSetContent (UID_node, BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->id));
     xmlNodeSetContent (id_node, BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->id));      
     
     /* name */
     name_node = xmlNewChild (tasks_object, NULL, BAD_CAST "Name", NULL);
     if(tmp_tasks_datas->name) {
        xmlNodeSetContent (name_node,  BAD_CAST g_strdup_printf ("%s", tmp_tasks_datas->name));
     }
     else
        xmlNodeSetContent (name_node,  BAD_CAST g_strdup_printf ("T%d", tmp_tasks_datas->id));
     /* type */
     type_node = xmlNewChild (tasks_object, NULL, BAD_CAST "Type", NULL);
     xmlNodeSetContent (type_node, BAD_CAST "0");/* fixed units */
     /* flag */
     is_null_node = xmlNewChild (tasks_object, NULL, BAD_CAST "IsNull", NULL);
     xmlNodeSetContent (is_null_node, BAD_CAST "0");
     /* creation date, not critical */
     creation_node = xmlNewChild (tasks_object, NULL, BAD_CAST "CreateDate", NULL);
     xmlNodeSetContent (creation_node, BAD_CAST 
                       g_strdup_printf ("%d-%02d-%02dT%02d:%02d:00", g_date_get_year (&current_date), g_date_get_month (&current_date), 
                                        g_date_get_day (&current_date),
                                        g_date_time_get_hour (my_time),
                                        g_date_time_get_minute (my_time)
                       ));
                       
     /* WBS - very simplified for now */
     if(tmp_tasks_datas->type != TASKS_TYPE_GROUP) {
		wbs_count++;
		wbs_node = xmlNewChild (tasks_object, NULL, BAD_CAST "WBS", NULL);
		xmlNodeSetContent (wbs_node, BAD_CAST  g_strdup_printf ("%d", wbs_count));
     }
     /* same, outline number simplified for now */
     if(tmp_tasks_datas->type != TASKS_TYPE_GROUP) {
		outline_number_node = xmlNewChild (tasks_object, NULL, BAD_CAST "OutlineNumber", NULL);
		xmlNodeSetContent (outline_number_node, BAD_CAST  g_strdup_printf ("%d", wbs_count));
     }
     /* same for outline level fixed at level 1 */
     if(tmp_tasks_datas->type != TASKS_TYPE_GROUP) {
		outline_level_node = xmlNewChild (tasks_object, NULL, BAD_CAST "OutlineLevel", NULL);
		xmlNodeSetContent (outline_level_node, BAD_CAST  g_strdup_printf ("%d", 1));
     }     
      

     /* priority for Project, midle level = 500 >500 is a higher level */
     /* for planlibre, 0 = high, 1 = medium, 2 = low */
     priority_node = xmlNewChild (tasks_object, NULL, BAD_CAST "Priority", NULL);
     switch(tmp_tasks_datas->priority) {
		case 0: {
			priority = 1000;
		    break;
		}
		case 1: {
			priority = 500;
			break;
		}
		case 2: {
			priority = 100;
			break;
		}
		default: {
			priority = 500;
		}
     }/* end switch */
     xmlNodeSetContent (priority_node, BAD_CAST  g_strdup_printf ("%d", priority)); 
 
     /* start - TODO correct starting hour */
     hour1 = data->properties.scheduleStart/60;
     min1 =  data->properties.scheduleStart - 60*hour1;
   
     start_node = xmlNewChild (tasks_object, NULL, BAD_CAST "Start", NULL);
     xmlNodeSetContent (start_node, BAD_CAST  g_strdup_printf ("%02d:%02d:%02dT%02d:%02d:00",
                              tmp_tasks_datas->start_nthYear,
                              tmp_tasks_datas->start_nthMonth,
                              tmp_tasks_datas->start_nthDay,
                              hour1, min1)
     
                       );      
     /* finish - don't used for now */ 
     hour2 = data->properties.scheduleEnd/60;
     min2 =  data->properties.scheduleEnd - 60*hour2;
     
     finish_node = xmlNewChild (tasks_object, NULL, BAD_CAST "Finish", NULL);
     xmlNodeSetContent (finish_node, BAD_CAST  g_strdup_printf ("%02d:%02d:%02dT%02d:%02d:00",
                              tmp_tasks_datas->end_nthYear,
                              tmp_tasks_datas->end_nthMonth,
                              tmp_tasks_datas->end_nthDay,
                              hour2, min2)
     
                       );       
     /* duration */
     duration_node = xmlNewChild (tasks_object, NULL, BAD_CAST "Duration", NULL);
     min1 = tmp_tasks_datas->days*(data->properties.scheduleEnd-data->properties.scheduleStart); /* here we have a time in minutes */
     min1 = min1 + 60*tmp_tasks_datas->minutes;
     hour2 = min1/60;
     min2 = min1-60*hour2;
     xmlNodeSetContent (duration_node, BAD_CAST  g_strdup_printf ("PT%dH%dM0S", hour2, min2));
     /* duration format */
     duration_format_node = xmlNewChild (tasks_object, NULL, BAD_CAST "DurationFormat", NULL);
     xmlNodeSetContent (duration_format_node, BAD_CAST  "5");/* 5 means hours */                                          
     /* test if Milestone - Gnome Planner requires deault value */
     milestone_node = xmlNewChild (tasks_object, NULL, BAD_CAST "Milestone", NULL);     
     if(tmp_tasks_datas->type == TASKS_TYPE_MILESTONE) {
        xmlNodeSetContent (milestone_node, BAD_CAST "1");		 
     }
     else {
        xmlNodeSetContent (milestone_node, BAD_CAST "0");
     }	
     /* resume or not a task */
     resume_node = xmlNewChild (tasks_object, NULL, BAD_CAST "ResumeValid", NULL);      
     xmlNodeSetContent (resume_node, BAD_CAST "0");	     
     /* effort drivent flag */
     effort_node = xmlNewChild (tasks_object, NULL, BAD_CAST "EffortDriven", NULL);      
     xmlNodeSetContent (effort_node, BAD_CAST "1");	      
     /* recurring flag */
     recur_node = xmlNewChild (tasks_object, NULL, BAD_CAST "Recurring", NULL);      
     xmlNodeSetContent (recur_node, BAD_CAST "0");
     /* overallocating flag - equivalent to oberload for PlanLibre, like in lanLibre, it's allowed */
     over_node = xmlNewChild (tasks_object, NULL, BAD_CAST "OverAllocated", NULL);      
     xmlNodeSetContent (over_node, BAD_CAST "1");     
     /* set flag duration as estimated */
     estimated_node = xmlNewChild (tasks_object, NULL, BAD_CAST "Estimated", NULL);      
     xmlNodeSetContent (estimated_node, BAD_CAST "1");     
     /* summary ? */
     summary_node = xmlNewChild (tasks_object, NULL, BAD_CAST "Summary", NULL);      
     xmlNodeSetContent (summary_node, BAD_CAST "0");     
     /* flag for critical path */      
     critical_node = xmlNewChild (tasks_object, NULL, BAD_CAST "Critical", NULL);      
     xmlNodeSetContent (critical_node, BAD_CAST "1");     
     /* flags sub project */
     sub_node = xmlNewChild (tasks_object, NULL, BAD_CAST "IsSubproject", NULL);      
     xmlNodeSetContent (sub_node, BAD_CAST "0");      
     sub_read_node = xmlNewChild (tasks_object, NULL, BAD_CAST "IsSubprojectReadOnly", NULL);      
     xmlNodeSetContent (sub_read_node, BAD_CAST "0");
     external_node = xmlNewChild (tasks_object, NULL, BAD_CAST "ExternalTask", NULL);      
     xmlNodeSetContent (external_node, BAD_CAST "0");
     /* fixed costs accrual ??? */
     fixed_accrual_node = xmlNewChild (tasks_object, NULL, BAD_CAST "FixedCostAccrual", NULL);      
     xmlNodeSetContent (fixed_accrual_node, BAD_CAST "2");     
     /* remaining duration to complete task - TODO to improve because PlanLibre can manage that */
     remain_dur_node = xmlNewChild (tasks_object, NULL, BAD_CAST "RemainingDuration", NULL);        
     xmlNodeSetContent (remain_dur_node, BAD_CAST  g_strdup_printf ("PT%dH%dM0S", hour2, min2));     
     /* constraints on start and finsih dates  TODO for now I use only as soon as code 0, finish no later = 7 start not earlier 4 must start on 2*/
     constraint_type_node = xmlNewChild (tasks_object, NULL, BAD_CAST "ConstraintType", NULL);  
     xmlNodeSetContent (constraint_type_node, BAD_CAST "0");           
     /* calendar UID, not used for now TODO */
     cal_UID_node = xmlNewChild (tasks_object, NULL, BAD_CAST "CalendarUID", NULL);  
     xmlNodeSetContent (cal_UID_node, BAD_CAST "-1");           
     /* constraint date - to improve, PlanLibre can manage it */
     constraint_date_node = xmlNewChild (tasks_object, NULL, BAD_CAST "ConstraintDate", NULL);       
     xmlNodeSetContent (constraint_date_node, BAD_CAST "1970-01-01T00:00:00");      
     /* flag for assignments */
     level_assign_node = xmlNewChild (tasks_object, NULL, BAD_CAST "LevelAssignments", NULL);       
     xmlNodeSetContent (level_assign_node, BAD_CAST "0"); 
     leval_can_split_node = xmlNewChild (tasks_object, NULL, BAD_CAST "LevelingCanSplit", NULL);       
     xmlNodeSetContent (leval_can_split_node, BAD_CAST "0");  
     /* leveling delay management */
     leval_delay_node = xmlNewChild (tasks_object, NULL, BAD_CAST "LevelingDelay", NULL);       
     xmlNodeSetContent (leval_delay_node, BAD_CAST "0");     
     level_delay_format_mode = xmlNewChild (tasks_object, NULL, BAD_CAST "LevelingDelayFormat", NULL);       
     xmlNodeSetContent (level_delay_format_mode, BAD_CAST "20");  
     /* flag ignore reources */
     ignore_rsc_cal_mode = xmlNewChild (tasks_object, NULL, BAD_CAST "IgnoreResourceCalendar", NULL);       
     xmlNodeSetContent (ignore_rsc_cal_mode, BAD_CAST "0");     
     /* hidebar flag */
     hidebar_node = xmlNewChild (tasks_object, NULL, BAD_CAST "HideBar", NULL);       
     xmlNodeSetContent (hidebar_node, BAD_CAST "0");  
     /* rollup flag */
     rollup_node = xmlNewChild (tasks_object, NULL, BAD_CAST "Rollup", NULL);       
     xmlNodeSetContent (rollup_node, BAD_CAST "0");
     earned_value_node = xmlNewChild (tasks_object, NULL, BAD_CAST "EarnedValueMethod", NULL); /* 0 means use % completed */      
     xmlNodeSetContent (earned_value_node, BAD_CAST "0");
   
     
       
     /* type */
  //   xmlNewProp (tasks_object, BAD_CAST "Type", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->type));
     /* timer value */
  //   xmlNewProp (tasks_object, BAD_CAST "Timer_value", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->timer_val));
     /*  status */
  //   xmlNewProp (tasks_object, BAD_CAST "Status", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->status));
     /*  priority */
  //   xmlNewProp (tasks_object, BAD_CAST "Priority", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->priority));
     /*  category */
  //   xmlNewProp (tasks_object, BAD_CAST "Category", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->category));
     /*  progress value */
  //   g_ascii_dtostr (buffer, 50, tmp_tasks_datas->progress );
  //   xmlNewProp (tasks_object, BAD_CAST "Progress", BAD_CAST buffer);
     /* color */
  //   g_ascii_dtostr (buffer, 50, tmp_tasks_datas->color.red );
  //   xmlNewProp (tasks_object, BAD_CAST "Color_red", BAD_CAST buffer);
  //   g_ascii_dtostr (buffer, 50, tmp_tasks_datas->color.green );
  //   xmlNewProp (tasks_object, BAD_CAST "Color_green", BAD_CAST buffer);
  //   g_ascii_dtostr (buffer, 50, tmp_tasks_datas->color.blue );
   //  xmlNewProp (tasks_object, BAD_CAST "Color_blue", BAD_CAST buffer);
     /* dates */
  //   startDate = misc_convert_date_to_str (tmp_tasks_datas->start_nthDay, 
                                       //    tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
  /*   g_date_set_dmy (&start_date, tmp_tasks_datas->start_nthDay, 
                                           tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
     if(startDate) {
        xmlNewProp (tasks_object, BAD_CAST "Start_date", BAD_CAST g_strdup_printf ("%s", startDate));
     }
     else {
	    xmlNewProp (tasks_object, BAD_CAST "Start_date", BAD_CAST "");
     }
     xmlNewProp (tasks_object, BAD_CAST "Start_date_day", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->start_nthDay));
     xmlNewProp (tasks_object, BAD_CAST "Start_date_month", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->start_nthMonth));
     xmlNewProp (tasks_object, BAD_CAST "Start_date_year", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->start_nthYear));

     g_date_set_dmy (&end_date, tmp_tasks_datas->end_nthDay, 
                                           tmp_tasks_datas->end_nthMonth, tmp_tasks_datas->end_nthYear);
     endDate = misc_convert_date_to_str(tmp_tasks_datas->end_nthDay, tmp_tasks_datas->end_nthMonth, tmp_tasks_datas->end_nthYear);
     if(endDate) {
        xmlNewProp (tasks_object, BAD_CAST "End_date", BAD_CAST g_strdup_printf ("%s", endDate));
     }
     else {
        xmlNewProp (tasks_object, BAD_CAST "End_date", BAD_CAST "");
     }
     xmlNewProp (tasks_object, BAD_CAST "End_date_day", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->end_nthDay));
     xmlNewProp (tasks_object, BAD_CAST "End_date_month", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->end_nthMonth));
     xmlNewProp (tasks_object, BAD_CAST "End_date_year", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->end_nthYear));
     /* deadline and not before date */
 /*    xmlNewProp (tasks_object, BAD_CAST "Lim_date_day", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->lim_nthDay));
     xmlNewProp (tasks_object, BAD_CAST "Lim_date_month", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->lim_nthMonth));
     xmlNewProp (tasks_object, BAD_CAST "Lim_date_year", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->lim_nthYear));
     /* delays */
 /*    xmlNewProp (tasks_object, BAD_CAST "Delay", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->delay));
     g_free (startDate);
     g_free (endDate);
     startDate = NULL;
     endDate = NULL;
     /* if sart == end we must add 1 day to end */
/*
     gint cmp1 = g_date_compare (&start_date, &end_date);
     if(cmp1<=0) {
        g_date_set_dmy (&end_date, tmp_tasks_datas->start_nthDay, 
                                           tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
        g_date_add_days (&end_date, 1);
        tmp_tasks_datas->end_nthDay = g_date_get_day (&end_date);
        tmp_tasks_datas->end_nthMonth = g_date_get_month (&end_date);
        tmp_tasks_datas->end_nthYear = g_date_get_year (&end_date);
     }
*/

     /* duration, in hours */
  /*   xmlNewProp (tasks_object, BAD_CAST "Duration", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->durationMode));
     xmlNewProp (tasks_object, BAD_CAST "Days", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->days));
     xmlNewProp (tasks_object, BAD_CAST "Hours", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->hours));
     xmlNewProp (tasks_object, BAD_CAST "Minutes", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->minutes));
     /* calendar */
  //   xmlNewProp (tasks_object, BAD_CAST "Calendar", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->calendar));
     /* group */
  //   xmlNewProp (tasks_object, BAD_CAST "Group", BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->group));
  }/* next i */
 
  
  return ret;
}

/*********************************
  Local function
  save calendars part to project
  * xml file
*********************************/
static gint save_calendars_to_project (xmlNodePtr calendars_node, APP_data *data)
{
  gint i = 0, ret=0, j, k, val, hour1, hour2, flag, day, hour, min;
  gboolean fschedule = FALSE;
  gchar buffer[100];/* for gdouble conversions */
  GList *l, *l1;
  calendar *tmp_calendars_datas;
  GError *error = NULL;
  xmlNodePtr calendars_UID = NULL, calendars_object = NULL, calendar_name = NULL, schedule_object =NULL, period_object = NULL;
  xmlNodePtr is_base_node = NULL, week_node = NULL, weekday_node = NULL, day_type_node = NULL, day_working_node = NULL;
  xmlNodePtr working_day_times_node = NULL, working_time_node = NULL, from_time_node = NULL, to_time_node = NULL;
  xmlNodePtr exceptions_list_node = NULL, exception_node = NULL, occurrences_node = NULL, time_period_node = NULL;
  xmlNodePtr occur_node = NULL, exception_name_node = NULL, exception_type_node = NULL;
  xmlNodePtr from_date_node = NULL, to_date_node = NULL;
  xmlNodePtr days_object = NULL;
  calendar_element *element;


  /* main loop for all defined calendars */
//   for(i=0 ; i< g_list_length (data->calendars); i++) {
  for(i = 0; i < 1; i++) {	/* TODO allow access to call calendars */  
     l = g_list_nth (data->calendars, i);
     tmp_calendars_datas = (calendar *)l->data;
     calendars_object = xmlNewChild (calendars_node, NULL, BAD_CAST "Calendar", NULL);
     calendars_UID = xmlNewChild (calendars_object, NULL, BAD_CAST "UID", NULL);/* I use internal count as UID */
     /* from Microsoft : For its other parent elements, UID is an integer. UID values are unique only within a project, not across multiple projects. 
      * it's possible to use the same value for UID and identifier */
     xmlNodeSetContent (calendars_UID, BAD_CAST "1");/* only for default calendar */   
     calendar_name = xmlNewChild (calendars_object, NULL, BAD_CAST "Name", NULL);/* I use internal count as UID */       
     xmlNodeSetContent (calendar_name, BAD_CAST  tmp_calendars_datas->name);       
     /* flag for base calendar, i.e. default calendar rank 0 for PlanLibre */
     is_base_node = xmlNewChild (calendars_object, NULL, BAD_CAST "IsBaseCalendar", NULL);/* I use internal count as UID */       
     flag = 0;
     if(i==0) {
		 flag = 1;
     }
     xmlNodeSetContent (is_base_node, BAD_CAST  g_strdup_printf ("%d", flag)); 
     /* sub node  weekdays */
     week_node = xmlNewChild (calendars_object, NULL, BAD_CAST "WeekDays", NULL);
     /* now, all days are a sub node of week_node */
     /* coding : from Sunday = 1 to saturday = 7 ; working day flag 1, non-working flag = 0 */
     /* for PlanLibre, monday = 0 to sunday = 6 */
     /* with PlanLibre, schedules are defined for all days of week, and other days are managed like "exceptions" for ms project */
     
     for(j=0;j<7;j++) {
		weekday_node = xmlNewChild (week_node, NULL, BAD_CAST "WeekDay", NULL);
		day_type_node = xmlNewChild (weekday_node, NULL, BAD_CAST "DayType", NULL);
		if(j==0)
		   day = 2;
		if(j==1)
		   day = 3;
		if(j==2)
		   day = 4;
		if(j==3)
		   day = 5;
		if(j==4)
		   day = 6;
		if(j==5)
		   day = 7;
		if(j==6)
		   day = 1;      
		xmlNodeSetContent (day_type_node, BAD_CAST  g_strdup_printf ("%d", day)); 
		/* we must set all days as worked because we transode from PlanLibe to Ms project */
		day_working_node = xmlNewChild (weekday_node, NULL, BAD_CAST "DayWorking", NULL); 
		xmlNodeSetContent (day_working_node, BAD_CAST  "1");
		/* if something is defined in schedule, we change working hours, in other cases we use default schedules */
		working_day_times_node = xmlNewChild (weekday_node, NULL, BAD_CAST "WorkingTimes", NULL);
		/* Planlibre allows to defineup to  4 periods of work by day */
		fschedule = FALSE;
		for(k=0;k<4;k++) {
		  if(tmp_calendars_datas->fDays[j][k]) {
			    fschedule = TRUE;	
				/* we must test if any of 4 period is defined - in other cases ? */
				working_time_node = xmlNewChild (working_day_times_node, NULL, BAD_CAST "WorkingTime", NULL);
				hour1 = tmp_calendars_datas->schedules[j][k][0];
				hour2 = tmp_calendars_datas->schedules[j][k][1];
				if(hour1>1438)
					hour1 = 1438;
				if(hour2>1439)
					hour2 = 1439;
				/* now we convert in HH:MM:SS format */
				hour = hour1/60;
				min = hour1-60*hour;
				from_time_node = xmlNewChild (working_time_node, NULL, BAD_CAST "FromTime", NULL);
				xmlNodeSetContent (from_time_node, BAD_CAST  g_strdup_printf ("%02d:%02d:00", hour, min));
				
				hour = hour2/60;
				min = hour2-60*hour;
				to_time_node = xmlNewChild (working_time_node, NULL, BAD_CAST "ToTime", NULL);
				xmlNodeSetContent (to_time_node, BAD_CAST  g_strdup_printf ("%02d:%02d:00", hour, min));
	        }
        }/* next k */
       if(!fschedule) {
		  /* we use global schedule deined in properties */
		  working_time_node = xmlNewChild (working_day_times_node, NULL, BAD_CAST "WorkingTime", NULL);
		  hour1 = data->properties.scheduleStart;
		  hour2 = data->properties.scheduleEnd;
		  if(hour1>1438)
					hour1 = 1438;
	 	  if(hour2>1439)
					hour2 = 1439;
		  /* now we convert in HH:MM:SS format */
		  hour = hour1/60;
		  min = hour1-60*hour;
		  from_time_node = xmlNewChild (working_time_node, NULL, BAD_CAST "FromTime", NULL);
		  xmlNodeSetContent (from_time_node, BAD_CAST  g_strdup_printf ("%02d:%02d:00", hour, min));
				
		  hour = hour2/60;
		  min = hour2-60*hour;
		  to_time_node = xmlNewChild (working_time_node, NULL, BAD_CAST "ToTime", NULL);
		  xmlNodeSetContent (to_time_node, BAD_CAST  g_strdup_printf ("%02d:%02d:00", hour, min));   
	   } 
	 } /* next j */
     /* each days - equivalent "exceptions" for MS Project */
     if(g_list_length (tmp_calendars_datas->list) > 0) {
		 
	   exceptions_list_node = xmlNewChild (calendars_object, NULL, BAD_CAST "Exceptions", NULL);
		 
       for(j = 0; j < g_list_length (tmp_calendars_datas->list); j++) {
         l1 = g_list_nth (tmp_calendars_datas->list, j);
         element = (calendar_element *) l1->data;
         exception_node = xmlNewChild (exceptions_list_node, NULL, BAD_CAST "Exception", NULL);
         occurrences_node = xmlNewChild (exception_node, NULL, BAD_CAST "EnteredByOccurrences", NULL); 
         xmlNodeSetContent (occurrences_node, BAD_CAST  "0");  
         time_period_node = xmlNewChild (exception_node, NULL, BAD_CAST "TimePeriod", NULL); 
         /* detail of period */
         from_date_node = xmlNewChild (time_period_node, NULL, BAD_CAST "FromDate", NULL);
         //         <FromDate>2007-11-22T00:00:00</FromDate>
         xmlNodeSetContent (from_date_node, BAD_CAST  
                          g_strdup_printf ("%d-%02d-%02dT00:00:00", element->year, element->month, element->day)
                           );           
         to_date_node = xmlNewChild (time_period_node, NULL, BAD_CAST "ToDate", NULL);
         xmlNodeSetContent (to_date_node, BAD_CAST  
                          g_strdup_printf ("%d-%02d-%02dT23:59:00", element->year, element->month, element->day)
                           );          
         
         occur_node = xmlNewChild (exception_node, NULL, BAD_CAST "Occurrences", NULL);  
         xmlNodeSetContent (occur_node, BAD_CAST  "1");  
                 
         exception_name_node = xmlNewChild (exception_node, NULL, BAD_CAST "Name", NULL);            
         
         /* PlanLibre will only use type 1, fixed date */
         
         exception_type_node = xmlNewChild (exception_node, NULL, BAD_CAST "Type", NULL);
         xmlNodeSetContent (exception_type_node, BAD_CAST  "1");
         /* day worked or not ? */
         /* we must set all days as worked because we transode from PlanLibe to Ms project */
         /* here is coding :
            CAL_TYPE_FORCE_WORKED = 1,
              CAL_TYPE_NON_WORKED,
               CAL_TYPE_DAY_OFF,
               CAL_TYPE_HOLIDAYS,
               CAL_TYPE_ABSENT,
               CAL_TYPE_PUBLIC_HOLIDAYS
         */
		 day_working_node = xmlNewChild (exception_node, NULL, BAD_CAST "DayWorking", NULL);
		 if(element->type == 1) {
		     xmlNodeSetContent (day_working_node, BAD_CAST  "1");
		     xmlNodeSetContent (exception_name_node, BAD_CAST  g_strdup_printf ("%s", _("Worked day"))); 
		 }
		 else {
		     xmlNodeSetContent (day_working_node, BAD_CAST  "0");
		     xmlNodeSetContent (exception_name_node, BAD_CAST  g_strdup_printf ("%s", _("Non Worked")));              
		 }

       }/* next j for all exception days */
     }/* endif list */
  }/* next i */

  return ret;
}


/**********************************
  save current project datas 
  in an XML compliant file
**********************************/
void save_to_project (gchar *filename, APP_data *data)
{
	GDate current_date;
	GDateTime *my_time;
	struct lconv *locali;
    gchar buff[256], buffer[100];/* for gdouble conversions */
    xmlDocPtr doc = NULL;       /* document pointer */
    xmlNodePtr root_node = NULL, node = NULL, node1 = NULL;/* node pointers */
    /* project - see here : https://docs.microsoft.com/en-us/office-project/xml-data-interchange/project-elements-and-xml-structure?view=project-client-2016 */
    xmlNodePtr save_version_node = NULL;
    xmlNodePtr uuid_node = NULL, title_node = NULL, subject_node = NULL, company_node = NULL;
    xmlNodePtr manager_node = NULL;
    xmlNodePtr schedule_from_start_node = NULL, start_date_node = NULL, finish_date_node = NULL;
    xmlNodePtr critical_slack_limit_node = NULL, currency_digits_node = NULL, currency_symbol_node = NULL, currency_code_node = NULL;
    xmlNodePtr calendarUID_node = NULL, default_start_time_node = NULL, MinutesPerDay_node = NULL;
    xmlNodePtr DefaultTaskType_node = NULL, DefaultStandardRate_node = NULL, DefaultOvertimeRate_node = NULL;
    xmlNodePtr DurationFormat_node = NULL, WorkFormat_node = NULL, EditableActualCosts_node = NULL, HonorConstraints_node = NULL;
    xmlNodePtr EarnedValueMethod_node = NULL, InsertedProjectsLikeSummary_node = NULL, MultipleCriticalPaths_node = NULL, NewTasksEffortDriven_node = NULL;
    xmlNodePtr NewTasksEstimated_node = NULL, SplitsInProgressTasks_node = NULL, SpreadActualCost_node = NULL, SpreadPercentComplete_node = NULL;
    xmlNodePtr TaskUpdatesResource_node = NULL, FiscalYearStart_node = NULL, WeekStartDay_node = NULL, MoveCompletedEndsBack_node = NULL; 
    xmlNodePtr MoveRemainingStartsBack_node = NULL, MoveRemainingStartsForward_node = NULL, MoveCompletedEndsForward_node = NULL;
    xmlNodePtr BaselineForEarnedValue_node = NULL, AutoAddNewResourcesAndTasks_node = NULL, CurrentDate_node = NULL, MicrosoftProjectServerURL_node = NULL;
    xmlNodePtr Autolink_node = NULL, NewTaskStartDate_node = NULL, DefaultTaskEVMethod_node = NULL, ProjectExternallyEdited_node = NULL;
    xmlNodePtr ActualsInSync_node = NULL, RemoveFileProperties_node = NULL, AdminProject_node = NULL, ExtendedAttribute_node = NULL;
    
    /* sub sections */
    xmlNodePtr properties_node = NULL, rsc_node = NULL, tasks_node = NULL;
    xmlNodePtr links_node = NULL, calendars_node = NULL;
    gint i, j, ret;

    /* get current date */
    g_date_set_time_t (&current_date, time (NULL)); /* date of today */
    /* get current time, should be unrefed once used */
    my_time = g_date_time_new_now_local ();
    /* get currency */
    locali = localeconv ();

    /* avoid re-entrant call */
    fStartedSaving = TRUE;

    LIBXML_TEST_VERSION;
    /* 
     * Creates a new document, a node and set it as a root node
     */
    doc = xmlNewDoc (BAD_CAST "1.0");
    root_node = xmlNewNode (NULL, BAD_CAST "Project");
    xmlDocSetRootElement (doc, root_node);
    xmlNewProp (root_node, BAD_CAST "xmlns", BAD_CAST "http://schemas.microsoft.com/project");
    //xmlNodeSetContent (root_node, BAD_CAST " xmlns=\"http://schemas.microsoft.com/project\"&#62;");
   
    /*
     *NEVER  Creates a DTD declaration. Isn't mandatory. 
     */
   // xmlCreateIntSubset (doc, BAD_CAST "PlanLibre", NULL, BAD_CAST "PlanLibre.dtd");
    
    /* we build the main nodes */
    /* save version */
    save_version_node = xmlNewChild (root_node, NULL, BAD_CAST "Saveversion", NULL);
    xmlNodeSetContent (save_version_node, BAD_CAST "12");/* 12 for project 2007 */
    
    /* UUID */
    uuid_node = xmlNewChild (root_node, NULL, BAD_CAST "UID", NULL);
    xmlNodeSetContent (uuid_node, BAD_CAST "1");/* ???? */
   
    /* title */
    title_node = xmlNewChild (root_node, NULL, BAD_CAST "Title", NULL);
    xmlNodeSetContent (title_node, BAD_CAST data->properties.title);    
    /* subject */
    subject_node = xmlNewChild (root_node, NULL, BAD_CAST "Subject", NULL);
    xmlNodeSetContent (subject_node, BAD_CAST data->properties.summary);
    /* Company */
    company_node = xmlNewChild (root_node, NULL, BAD_CAST "Company", NULL);
    xmlNodeSetContent (company_node, BAD_CAST data->properties.org_name);    
    /* Manager */
    manager_node = xmlNewChild (root_node, NULL, BAD_CAST "Manager", NULL);
    xmlNodeSetContent (manager_node, BAD_CAST data->properties.manager_name);
    /* schedule from start */
    schedule_from_start_node = xmlNewChild (root_node, NULL, BAD_CAST "ScheduleFromStart", NULL);
    xmlNodeSetContent (schedule_from_start_node, BAD_CAST "1");
        
    /* start date - required if schedule from start value is TRUE (1) */
    start_date_node = xmlNewChild (root_node, NULL, BAD_CAST "StartDate", NULL);
    xmlNodeSetContent (start_date_node, BAD_CAST     
                       g_strdup_printf ("%d-%02d-%02dT00:00:00", data->properties.start_nthYear, data->properties.start_nthMonth, data->properties.start_nthDay)
                      );
    /* finish date */
    finish_date_node = xmlNewChild (root_node, NULL, BAD_CAST "FinishDate", NULL);// TODO mettre parv défaut ide date début
    xmlNodeSetContent (finish_date_node, BAD_CAST     
                       g_strdup_printf ("%d-%02d-%02dT00:00:00", data->properties.start_nthYear, data->properties.start_nthMonth, data->properties.start_nthDay)
                      );

    /* critical slack limit */
    critical_slack_limit_node = xmlNewChild (root_node, NULL, BAD_CAST "CriticalSlackLimit", NULL);
    xmlNodeSetContent (critical_slack_limit_node, BAD_CAST "0");    
    /* currency digits */
    currency_digits_node = xmlNewChild (root_node, NULL, BAD_CAST "CurrencyDigits", NULL);
    xmlNodeSetContent (currency_digits_node, BAD_CAST "2"); 
    /* currency symbol */
    currency_symbol_node = xmlNewChild (root_node, NULL, BAD_CAST "CurrencySymbol", NULL);
    if(data->properties.units)
        xmlNodeSetContent (currency_symbol_node, BAD_CAST locali->currency_symbol); 
    else
        xmlNodeSetContent (currency_symbol_node, BAD_CAST locali->int_curr_symbol); 
    //     printf ("%s\n", locali->int_curr_symbol);  
    /* currency code must have 3 chars len and mandatory */
    currency_code_node = xmlNewChild (root_node, NULL, BAD_CAST "CurrencyCode", NULL); 
    xmlNodeSetContent (currency_code_node, BAD_CAST g_strdup_printf ("%.3s", locali->int_curr_symbol));// tODO, à lire dans le système
    /* calendar UID */
    calendarUID_node =  xmlNewChild (root_node, NULL, BAD_CAST "CalendarUID", NULL); 
    xmlNodeSetContent (calendarUID_node, BAD_CAST "1"); 
    /* default start time for new tasks */
    default_start_time_node = xmlNewChild (root_node, NULL, BAD_CAST "DefaultStartTime", NULL); 
    xmlNodeSetContent (default_start_time_node, BAD_CAST "08:00:00"); /* TODO !!! */
    /* minutes worked by day */
    MinutesPerDay_node = xmlNewChild (root_node, NULL, BAD_CAST "MinutesPerDay", NULL); 
    xmlNodeSetContent (MinutesPerDay_node, BAD_CAST "480"); /* TODO !!! */
    /* default new tasks */
    DefaultTaskType_node = xmlNewChild (root_node, NULL, BAD_CAST "DefaultTaskType", NULL); 
    xmlNodeSetContent (DefaultTaskType_node, BAD_CAST "0"); 
    /* default standard rate for new resources */
    DefaultStandardRate_node = xmlNewChild (root_node, NULL, BAD_CAST "DefaultStandardRate", NULL);
    xmlNodeSetContent (DefaultStandardRate_node, BAD_CAST "15");
    /* default overtime rate for new resources */
    DefaultOvertimeRate_node = xmlNewChild (root_node, NULL, BAD_CAST "DefaultOvertimeRate", NULL);
    xmlNodeSetContent (DefaultOvertimeRate_node, BAD_CAST "18");    
    /* default duration mode - 7 means "days" */
    DurationFormat_node = xmlNewChild (root_node, NULL, BAD_CAST "DurationFormat", NULL);
    xmlNodeSetContent (DurationFormat_node, BAD_CAST "7");   
    /* default work format - 2 means hours */
    WorkFormat_node = xmlNewChild (root_node, NULL, BAD_CAST "WorkFormat", NULL);
    xmlNodeSetContent (WorkFormat_node, BAD_CAST "2");   
    /* actual costs */
    EditableActualCosts_node = xmlNewChild (root_node, NULL, BAD_CAST "EditableActualCosts", NULL);
    xmlNodeSetContent (EditableActualCosts_node, BAD_CAST "0");      
    /* constraints */
    HonorConstraints_node = xmlNewChild (root_node, NULL, BAD_CAST "HonorConstraints", NULL);
    xmlNodeSetContent (HonorConstraints_node, BAD_CAST "0");
    /* value method */
    EarnedValueMethod_node = xmlNewChild (root_node, NULL, BAD_CAST "EarnedValueMethod", NULL);
    xmlNodeSetContent (EarnedValueMethod_node, BAD_CAST "0");         
    /* inserted projects */
    InsertedProjectsLikeSummary_node = xmlNewChild (root_node, NULL, BAD_CAST "InsertedProjectsLikeSummary", NULL);
    xmlNodeSetContent (InsertedProjectsLikeSummary_node, BAD_CAST "0");
    /* critical path */
    MultipleCriticalPaths_node = xmlNewChild (root_node, NULL, BAD_CAST "MultipleCriticalPaths", NULL);
    xmlNodeSetContent (MultipleCriticalPaths_node, BAD_CAST "0");
    /* flag tasj effort driven */    
    NewTasksEffortDriven_node = xmlNewChild (root_node, NULL, BAD_CAST "NewTasksEffortDriven", NULL);
    xmlNodeSetContent (NewTasksEffortDriven_node, BAD_CAST "0");            
    /* allow or not estimated duration for new tasks */
    NewTasksEstimated_node = xmlNewChild (root_node, NULL, BAD_CAST "NewTasksEstimated", NULL);
    xmlNodeSetContent (NewTasksEstimated_node, BAD_CAST "1");       
    /* split tasks */
    SplitsInProgressTasks_node = xmlNewChild (root_node, NULL, BAD_CAST "SplitsInProgressTasks", NULL);
    xmlNodeSetContent (SplitsInProgressTasks_node, BAD_CAST "0");    
    /* spread costs */
    SpreadActualCost_node = xmlNewChild (root_node, NULL, BAD_CAST "SpreadActualCost", NULL);
    xmlNodeSetContent (SpreadActualCost_node, BAD_CAST "0");      
    /* percents completed flag */
    SpreadPercentComplete_node = xmlNewChild (root_node, NULL, BAD_CAST "SpreadPercentComplete", NULL);
    xmlNodeSetContent (SpreadPercentComplete_node, BAD_CAST "0");     
    /* flag update tasks and resources */
    TaskUpdatesResource_node = xmlNewChild (root_node, NULL, BAD_CAST "TaskUpdatesResource", NULL);
    xmlNodeSetContent (TaskUpdatesResource_node, BAD_CAST "1");     
    /* fiscal year */
    FiscalYearStart_node = xmlNewChild (root_node, NULL, BAD_CAST "FiscalYearStart", NULL);
    xmlNodeSetContent (FiscalYearStart_node, BAD_CAST "0");     
    /* start day for week, 1 means monday */
    WeekStartDay_node = xmlNewChild (root_node, NULL, BAD_CAST "WeekStartDay", NULL);
    xmlNodeSetContent (WeekStartDay_node, BAD_CAST "1");     
    /* ---- */
    MoveCompletedEndsBack_node = xmlNewChild (root_node, NULL, BAD_CAST "MoveCompletedEndsBack", NULL);
    xmlNodeSetContent (MoveCompletedEndsBack_node, BAD_CAST "0");     
    /* --- */
    MoveRemainingStartsBack_node = xmlNewChild (root_node, NULL, BAD_CAST "MoveRemainingStartsBack", NULL);
    xmlNodeSetContent (MoveRemainingStartsBack_node, BAD_CAST "0");    
    /* ----*/ 
    MoveRemainingStartsForward_node = xmlNewChild (root_node, NULL, BAD_CAST "MoveRemainingStartsForward", NULL);
    xmlNodeSetContent (MoveRemainingStartsForward_node, BAD_CAST "0");    
    /* --- */
    MoveCompletedEndsForward_node = xmlNewChild (root_node, NULL, BAD_CAST "MoveCompletedEndsForward", NULL);
    xmlNodeSetContent (MoveCompletedEndsForward_node, BAD_CAST "0");    
    /* --- */
    BaselineForEarnedValue_node = xmlNewChild (root_node, NULL, BAD_CAST "BaselineForEarnedValue", NULL);
    xmlNodeSetContent (BaselineForEarnedValue_node, BAD_CAST "0");
    /* --- */
    AutoAddNewResourcesAndTasks_node = xmlNewChild (root_node, NULL, BAD_CAST "AutoAddNewResourcesAndTasks", NULL);
    xmlNodeSetContent (AutoAddNewResourcesAndTasks_node, BAD_CAST "1");    
    /* current date : when this file has been generated */
    CurrentDate_node = xmlNewChild (root_node, NULL, BAD_CAST "CurrentDate", NULL);
    xmlNodeSetContent (CurrentDate_node, BAD_CAST 
                       g_strdup_printf ("%d-%02d-%02dT%02d:%02d:00", g_date_get_year (&current_date), g_date_get_month (&current_date), 
                                        g_date_get_day (&current_date),
                                        g_date_time_get_hour (my_time),
                                        g_date_time_get_minute (my_time)
                       ));  
    
    g_date_time_unref (my_time); 
    /* */
    MicrosoftProjectServerURL_node = xmlNewChild (root_node, NULL, BAD_CAST "MicrosoftProjectServerURL", NULL);
    xmlNodeSetContent (MicrosoftProjectServerURL_node, BAD_CAST "1");    
    /* */
    Autolink_node = xmlNewChild (root_node, NULL, BAD_CAST "Autolink", NULL);
    xmlNodeSetContent (Autolink_node, BAD_CAST "1");     
    /* date choosen when a new task is defined */
    NewTaskStartDate_node = xmlNewChild (root_node, NULL, BAD_CAST "NewTaskStartDate", NULL);
    xmlNodeSetContent (NewTaskStartDate_node, BAD_CAST "0");    
    /* */
    DefaultTaskEVMethod_node = xmlNewChild (root_node, NULL, BAD_CAST "DefaultTaskEVMethod", NULL);
    xmlNodeSetContent (DefaultTaskEVMethod_node, BAD_CAST "0");    
    /* */
    ProjectExternallyEdited_node = xmlNewChild (root_node, NULL, BAD_CAST "ProjectExternallyEdited", NULL);
    xmlNodeSetContent (ProjectExternallyEdited_node, BAD_CAST "0");         
    /* */
    ActualsInSync_node = xmlNewChild (root_node, NULL, BAD_CAST "ActualsInSync", NULL);
    xmlNodeSetContent (ActualsInSync_node, BAD_CAST "0");     
    /* */
    RemoveFileProperties_node = xmlNewChild (root_node, NULL, BAD_CAST "RemoveFileProperties", NULL);
    xmlNodeSetContent (RemoveFileProperties_node, BAD_CAST "0");     
    /* flag for 'administrative' projects */
    AdminProject_node = xmlNewChild (root_node, NULL, BAD_CAST "AdminProject", NULL);
    xmlNodeSetContent (AdminProject_node, BAD_CAST "0");     
    /* */
    ExtendedAttribute_node = xmlNewChild (root_node, NULL, BAD_CAST "ExtendedAttribute", NULL);
    xmlNodeSetContent (ExtendedAttribute_node, BAD_CAST "");

    /* calendars */
    calendars_node = xmlNewChild (root_node, NULL, BAD_CAST "Calendars", NULL);
    ret = save_calendars_to_project (calendars_node, data);
    
    /* tasks */
    tasks_node =  xmlNewChild (root_node, NULL, BAD_CAST "Tasks", NULL);
    ret = save_tasks_to_project (tasks_node, data);
    /* ressources */
    rsc_node = xmlNewChild (root_node, NULL, BAD_CAST "Ressources", NULL);
  //  ret = save_ressources (rsc_node, data);

 

    /* Dumping document to file */
    xmlSaveFormatFileEnc (filename, doc, "UTF-8", 1);

    /*free the document */
    xmlFreeDoc (doc);

    /* Free the global variables that may
     have been allocated by the parser. */
    xmlCleanupParser ();
    /* set-up flags */
    data->fProjectModified = FALSE; 
    data->fProjectHasName = TRUE;
    misc_display_app_status (FALSE, data);

    fStartedSaving = FALSE;
}
