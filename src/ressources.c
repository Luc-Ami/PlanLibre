/* -------------------------------------
|   management of ressources           |
|                                      |
/*------------------------------------*/

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
#include "ressources.h"
#include "links.h"
#include "assign.h"
#include "tasks.h"
#include "calendars.h"
#include "undoredo.h"

/*#########################
 functions for list, see the 
 nice tutorial by
 IBM, here : https://developer.ibm.com/tutorials/l-glib/ 
#########################*/

/******************************
  PUBLIC : reads hours values
  and adjust minutes
******************************/
void rsc_hours_changed (GtkSpinButton *spin_button, APP_data *data)
{
   gint hour;
   GtkWidget *spinMinutes;

   hour = gtk_spin_button_get_value_as_int (spin_button);
   /* test if hour==24 then reset 'minutes' part */
   spinMinutes = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonDayMins"));
   if(hour>=24) {
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinMinutes), 0);
   }
}

/******************************
  PUBLIC : reads minutes values
  and check limits
******************************/
void rsc_minutes_changed (GtkSpinButton *spin_button, APP_data *data)
{
   gint minutes, hour;
   GtkWidget *spinMinutes, *spinHours;

   minutes = gtk_spin_button_get_value_as_int (spin_button);
   spinHours = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonDayHours"));
   hour = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spinHours));
   /* test if hour==24 then reset 'minutes' part */
   if(hour>=24) {
      spinMinutes = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonDayMins"));
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinMinutes), 0);
   }
}

/*******************************
  memory management for
  ressources datas
*******************************/
static gchar *rsc_get_cost_and_rate (gdouble cost, gint cost_type)
{
  gchar *label_cost;

  switch (cost_type) {
    case RSC_COST_HOUR: {
      label_cost = g_strdup_printf (_("%.2f <i>per hour</i>"), cost);
      break;
    }
    case RSC_COST_DAY: {
      label_cost = g_strdup_printf (_("%.2f <i>per day</i>"), cost);
      break;
    }
    case RSC_COST_WEEK: {
      label_cost = g_strdup_printf (_("%.2f <i>per week</i>"), cost);
      break;
    }
    case RSC_COST_MONTH: {
      label_cost = g_strdup_printf (_("%.2f <i>per month</i>"), cost);
      break;
    }
    case RSC_COST_FIXED: {
      label_cost = g_strdup_printf (_("%.2f <i>fixed</i>"), cost);
      break;
    }
    default:{
       label_cost = g_strdup_printf (_("%.2f per unknown rate"), cost);
    }
  }/* end switch */

  return label_cost;
}

static gchar *rsc_get_affiliate (gint orga)
{
  gchar *label_aff = NULL;

  switch(orga) {
    case RSC_AFF_OUR_ORG:{
      label_aff = g_strdup_printf ("%s",_("from our organization"));
      break;
    }
    case RSC_AFF_PRO_ORG:{
      label_aff = g_strdup_printf ("%s",_("from other profit organization"));
      break;
    }
    case RSC_AFF_IND:{
      label_aff = g_strdup_printf ("%s",_("Independant, self employed"));
      break;
    }
    case RSC_AFF_VOL:{
      label_aff = g_strdup_printf ("%s",_("Voluntary"));
      break;
    }
    case RSC_AFF_NPO:{
      label_aff = g_strdup_printf ("%s",_("From non-profit organization"));
      break;
    }
    case RSC_AFF_SCO:{
      label_aff = g_strdup_printf ("%s",_("Scholar"));
      break;
    }
    case RSC_AFF_ADMI:{
      label_aff = g_strdup_printf ("%s",_("From public administration"));
      break;
    }
    case RSC_AFF_PLAT:{
      label_aff = g_strdup_printf ("%s",_("Platform worker"));
      break;
    }
    default:{
      label_aff = g_strdup_printf ("%s",_("Unknown affiliation"));
    }
  }/* end switch */

  return label_aff;
}

static gchar *rsc_get_label_type (gint type)
{
  gchar *label_type = NULL;
  switch(type) {
    case RSC_TYPE_HUMAN:{
      label_type = g_strdup_printf ("%s", _("Human work force"));
      break;
    }
    case RSC_TYPE_ANIMAL:{
      label_type = g_strdup_printf ("%s", _("Animal work force"));
      break;
    }
    case RSC_TYPE_EQUIP:{
      label_type = g_strdup_printf ("%s", _("Equipment"));
      break;
    }
    case RSC_TYPE_SURF:{
      label_type = g_strdup_printf ("%s", _("Surface or building"));
      break;
    }
    case RSC_TYPE_MATER: {
      label_type = g_strdup_printf ("%s", _("Material"));
      break;
    }
    default:{
      label_type = g_strdup_printf ("%s", _("Unknown type"));
    }
  }/* end switch */
  
  return label_type;
}

/***************************************
  Local function 
  get datas for ressources at position
****************************************/
void rsc_get_ressource_datas (gint position, rsc_datas *rsc, APP_data *data)
{
  gint i;
  GList *l;

  gint rsc_list_len = g_list_length (data->rscList);

  /* we get  pointer on the element */
  if((position<rsc_list_len) && (position >=0)) {
     l = g_list_nth (data->rscList, position);
     rsc_datas *tmp_rsc_datas;
     tmp_rsc_datas = (rsc_datas *)l->data;
     
     /* get datas strings */
     rsc->name = tmp_rsc_datas->name;
     rsc->mail = tmp_rsc_datas->mail;     
     rsc->phone = tmp_rsc_datas->phone;
     rsc->reminder = tmp_rsc_datas->reminder;
     rsc->location = tmp_rsc_datas->location;
     rsc->affiliate = tmp_rsc_datas->affiliate;
     rsc->type = tmp_rsc_datas->type;  
     rsc->cost = tmp_rsc_datas->cost;  
     rsc->day_hours = tmp_rsc_datas->day_hours;
     rsc->day_mins = tmp_rsc_datas->day_mins;
     rsc->cost_type = tmp_rsc_datas->cost_type;  
     rsc->color = tmp_rsc_datas->color;    
     rsc->cadre = tmp_rsc_datas->cadre;    
     rsc->fPersoIcon = tmp_rsc_datas->fPersoIcon;
     rsc->pix = tmp_rsc_datas->pix;
     rsc->id =  tmp_rsc_datas->id;
     /* working days */
     rsc->fWorkMonday = tmp_rsc_datas->fWorkMonday;
     rsc->fWorkTuesday = tmp_rsc_datas->fWorkTuesday;
     rsc->fWorkWednesday = tmp_rsc_datas->fWorkWednesday;
     rsc->fWorkThursday = tmp_rsc_datas->fWorkThursday;
     rsc->fWorkFriday = tmp_rsc_datas->fWorkFriday;
     rsc->fWorkSaturday = tmp_rsc_datas->fWorkSaturday;
     rsc->fWorkSunday = tmp_rsc_datas->fWorkSunday;
     /* calendar */
     rsc->calendar = tmp_rsc_datas->calendar;
  }/* endif */
}

/***************************************
  LOCAL function 
  get row selected in GtkLisbox
****************************************/
static gint rsc_get_selected_row (APP_data *data )
{
  GtkListBox *box;
  GtkListBoxRow *row;

  box = GTK_LIST_BOX(GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxRsc"))); 
  row =  gtk_list_box_get_selected_row (box);
  if(row) {
        return gtk_list_box_row_get_index (row);  
  }
  else
    return -1;/* default */
}

/***********************************************
  PUBLIC function. remove all ressources datas 
  from memory. For example when we quit the
  program or call 'new' menu. This function is
 called only after Lisbox row removing !
**********************************************/
void rsc_free_all (APP_data *data)
{
  GList *l = g_list_first (data->rscList);
  GError *error = NULL;
  rsc_datas *tmp_rsc_datas;

  for(l;l!=NULL;l=l->next) {
     tmp_rsc_datas = (rsc_datas *)l->data;
     /* free strings */
     if(tmp_rsc_datas->name!=NULL) {
         g_free (tmp_rsc_datas->name);
         tmp_rsc_datas->name=NULL;
     }

     if(tmp_rsc_datas->mail!=NULL) {
         g_free (tmp_rsc_datas->mail);
         tmp_rsc_datas->mail=NULL;
     }

     if(tmp_rsc_datas->phone!=NULL) {
         g_free (tmp_rsc_datas->phone);
         tmp_rsc_datas->phone=NULL;
     }

     if(tmp_rsc_datas->reminder!=NULL) {
         g_free (tmp_rsc_datas->reminder);
         tmp_rsc_datas->reminder=NULL;
     }
     if(tmp_rsc_datas->location!=NULL) {
         g_free (tmp_rsc_datas->location);
         tmp_rsc_datas->location=NULL;
     }

     /* free pixmap for avatar */
     if(tmp_rsc_datas->pix!=NULL) {
        g_object_unref (tmp_rsc_datas->pix);  
     }
     /* free element */
     g_free (tmp_rsc_datas);
  }
  /* free the Glist itself */
  g_list_free (data->rscList);
  data->rscList = NULL;
  printf ("* PlanLibre : successfully freed Ressources datas *\n");
}

/******************************************
  PUBLIC function. Modify ressources datas 
  at a given position
******************************************/
void rsc_modify_datas (gint position, APP_data *data, rsc_datas *rsc)
{
  gint rsc_list_len = g_list_length (data->rscList);
  GList *l;

  if((position >=0) && (position<rsc_list_len)) {
     l = g_list_nth (data->rscList, position);
     rsc_datas *tmp_rsc_datas;
     tmp_rsc_datas = (rsc_datas *)l->data;
     /* now we change values 'in place' */
     /* 1st, we free PREVIOUS strings */
     if(tmp_rsc_datas->name) {
         g_free (tmp_rsc_datas->name);
         tmp_rsc_datas->name=NULL;
     }

     if(tmp_rsc_datas->mail) {
         g_free (tmp_rsc_datas->mail);
         tmp_rsc_datas->mail=NULL;
     }

     if(tmp_rsc_datas->phone) {
         g_free (tmp_rsc_datas->phone);
         tmp_rsc_datas->phone=NULL;
     }

     if(tmp_rsc_datas->reminder) {
         g_free (tmp_rsc_datas->reminder);
         tmp_rsc_datas->reminder=NULL;
     }
     if(tmp_rsc_datas->location) {
         g_free (tmp_rsc_datas->location);
         tmp_rsc_datas->location=NULL;
     }
     /* update strings */
     tmp_rsc_datas->name = g_strdup_printf ("%s", rsc->name);
     tmp_rsc_datas->reminder = g_strdup_printf ("%s", rsc->reminder);
     tmp_rsc_datas->mail = g_strdup_printf ("%s", rsc->mail);     
     tmp_rsc_datas->phone = g_strdup_printf ("%s", rsc->phone);
     tmp_rsc_datas->location = g_strdup_printf ("%s", rsc->location);
     /* store values from quasi global var */
     tmp_rsc_datas->affiliate = rsc->affiliate;
     tmp_rsc_datas->type = rsc->type;
     tmp_rsc_datas->cost = rsc->cost;
     tmp_rsc_datas->cost_type = rsc->cost_type;
     tmp_rsc_datas->color = rsc->color;
     tmp_rsc_datas->pix = rsc->pix;
     tmp_rsc_datas->cadre = rsc->cadre;
     tmp_rsc_datas->fPersoIcon = rsc->fPersoIcon;
     /* working days */
     tmp_rsc_datas->fWorkMonday = rsc->fWorkMonday;
     tmp_rsc_datas->fWorkTuesday = rsc->fWorkTuesday;
     tmp_rsc_datas->fWorkWednesday = rsc->fWorkWednesday;
     tmp_rsc_datas->fWorkThursday = rsc->fWorkThursday;
     tmp_rsc_datas->fWorkFriday = rsc->fWorkFriday;
     tmp_rsc_datas->fWorkSaturday = rsc->fWorkSaturday;
     tmp_rsc_datas->fWorkSunday = rsc->fWorkSunday;
     /* calandar */
     tmp_rsc_datas->calendar = rsc->calendar;
     tmp_rsc_datas->day_hours =rsc->day_hours;
     tmp_rsc_datas->day_mins =rsc->day_mins;
  }
}

/***********************************
  PUBLIC function - modify a widget 
  at a given position
  NOTE : this function must be called
  ONLY when datas are modified in 
  memory, with "rsc_modify_datas()'
***********************************/
void rsc_modify_widget (gint position, APP_data *data)
{
  gint rsc_list_len = g_list_length (data->rscList);
  GList *l;
  gchar *tmpStr;

  if((position >=0) && (position<rsc_list_len)) {
     l = g_list_nth (data->rscList, position);
     rsc_datas *tmp_rsc_datas;
     tmp_rsc_datas = (rsc_datas *)l->data;
     /* we change labels */
     tmpStr = g_markup_printf_escaped ("<b><big>%s</big></b>", tmp_rsc_datas->name);
     gtk_label_set_markup (GTK_LABEL(tmp_rsc_datas->cName), tmpStr);
     g_free (tmpStr);
     gtk_label_set_text (GTK_LABEL(tmp_rsc_datas->cType), rsc_get_label_type (tmp_rsc_datas->type));
     gtk_label_set_text (GTK_LABEL(tmp_rsc_datas->cAff), rsc_get_affiliate (tmp_rsc_datas->affiliate ));
     gtk_label_set_markup (GTK_LABEL(tmp_rsc_datas->cCost), rsc_get_cost_and_rate (tmp_rsc_datas->cost, tmp_rsc_datas->cost_type));

     /* color */
     gtk_label_set_markup (GTK_LABEL(tmp_rsc_datas->cColor ),  
                           g_strdup_printf ("<big><span background=\"%s\">   </span></big>", 
                                            misc_convert_gdkcolor_to_hex (tmp_rsc_datas->color)
                                          ) 
                          );
     /* set image if exists */
     if(tmp_rsc_datas->fPersoIcon) {
          gtk_image_set_from_pixbuf (GTK_IMAGE(tmp_rsc_datas->cImg), tmp_rsc_datas->pix);
     }
  }
}

/***************************************
  PUBLIC function add a ressources datas 
  at a given position
***************************************/
void rsc_insert (gint position, rsc_datas *rsc, APP_data *data)
{
  if(position>=0) {
    /* we add datas to the GList */
    rsc_datas *tmp_rsc_datas;
    tmp_rsc_datas = g_malloc (sizeof (rsc_datas));
    /* store values from quasi global var */
    tmp_rsc_datas->id = rsc->id;
    tmp_rsc_datas->affiliate = rsc->affiliate;
    tmp_rsc_datas->type = rsc->type;
    tmp_rsc_datas->cost = rsc->cost;
    tmp_rsc_datas->cost_type = rsc->cost_type;
    tmp_rsc_datas->color = rsc->color;
    tmp_rsc_datas->cadre = rsc->cadre;
    /* add datas about 'cadre' widget in order to modify it in place if ressource is modified */
    tmp_rsc_datas->cName = rsc->cName;
    tmp_rsc_datas->cType = rsc->cType;
    tmp_rsc_datas->cAff = rsc->cAff;
    tmp_rsc_datas->cCost = rsc->cCost;
    tmp_rsc_datas->cColor = rsc->cColor;
    tmp_rsc_datas->cImg = rsc->cImg;
    tmp_rsc_datas->fPersoIcon = rsc->fPersoIcon;
    tmp_rsc_datas->pix = rsc->pix;
    /* strings */
    tmp_rsc_datas->name = g_strdup_printf ("%s", rsc->name);
    if(rsc->name)
        g_free (rsc->name);
    tmp_rsc_datas->mail = g_strdup_printf ("%s", rsc->mail);
    if(rsc->mail)
        g_free (rsc->mail);
    tmp_rsc_datas->phone = g_strdup_printf ("%s", rsc->phone);
    if(rsc->phone)
       g_free (rsc->phone);
    tmp_rsc_datas->reminder = g_strdup_printf ("%s", rsc->reminder);
    if(rsc->reminder)
       g_free (rsc->reminder);
    tmp_rsc_datas->location = NULL;
    /* working days */
    tmp_rsc_datas->fWorkMonday = rsc->fWorkMonday;
    tmp_rsc_datas->fWorkTuesday = rsc->fWorkTuesday;
    tmp_rsc_datas->fWorkWednesday = rsc->fWorkWednesday;
    tmp_rsc_datas->fWorkThursday = rsc->fWorkThursday;
    tmp_rsc_datas->fWorkFriday = rsc->fWorkFriday;
    tmp_rsc_datas->fWorkSaturday = rsc->fWorkSaturday;
    tmp_rsc_datas->fWorkSunday = rsc->fWorkSunday;
    /* calendar */
    tmp_rsc_datas->calendar = rsc->calendar;
    tmp_rsc_datas->day_hours = rsc->day_hours;
    tmp_rsc_datas->day_mins = rsc->day_mins;
    /* WARNING ! never unref the pointer to the pixbuf, it must remain on memory */
    /* update list */
    data->rscList = g_list_insert (data->rscList, tmp_rsc_datas, position);
  }
  else
     printf("* PlanLibre critical : can't insert RSC datas at a negative position ! *\n");
}

/***********************************
  PUBLIC function
  remove a ressources datas 
  at a given position
***********************************/
void rsc_remove (gint position, GtkListBox *box, APP_data *data)
{
  GList *l;
  gint rsc_list_len = g_list_length (data->rscList);
  GtkWidget *btnModif, *parent;

  btnModif = GTK_WIDGET(gtk_builder_get_object (data->builder, "buttonRscModify"));

  /* we get  pointer on the element */

  if(position < rsc_list_len) {
     l = g_list_nth (data->rscList, position);
     rsc_datas *tmp_rsc_datas;
     tmp_rsc_datas = (rsc_datas *)l->data;
    
     /* free strings */
     if(tmp_rsc_datas->name!=NULL) {
         g_free (tmp_rsc_datas->name);
         tmp_rsc_datas->name=NULL;
     }

     if(tmp_rsc_datas->mail!=NULL) {
         g_free (tmp_rsc_datas->mail);
         tmp_rsc_datas->mail=NULL;
     }

     if(tmp_rsc_datas->phone!=NULL) {
         g_free (tmp_rsc_datas->phone);
         tmp_rsc_datas->phone=NULL;
     }

     if(tmp_rsc_datas->reminder!=NULL) {
         g_free (tmp_rsc_datas->reminder);
         tmp_rsc_datas->reminder=NULL;
     }
    if(tmp_rsc_datas->location!=NULL) {
         g_free (tmp_rsc_datas->location);
         tmp_rsc_datas->location=NULL;
     }
     /* free pixmap for avatar */
     if(tmp_rsc_datas->pix!=NULL) {
        // g_object_unref (tmp_rsc_datas->pix);  
       // printf("dans remove ligne trouvé pix en position %d\n", i);
     }
     /* remove widget */

     /* we don't have to UNREF GdkPixbuf, it's done by container itself */
     parent = NULL;
     if(tmp_rsc_datas->cadre)
        parent = gtk_widget_get_parent (tmp_rsc_datas->cadre);
     if(parent)
         gtk_container_remove (GTK_CONTAINER(box), parent);
     /* free element */
     g_free (tmp_rsc_datas);
     /* update list */
     data->rscList = g_list_delete_link (data->rscList, l); 
  }

  if(g_list_length (data->rscList)<=0) 
      gtk_widget_set_sensitive (GTK_WIDGET(btnModif), FALSE);

}

/***********************************
  PUBLIC function give total number
  of elements in rsc datas list
***********************************/
gint rsc_datas_len (APP_data *data )
{ 
  return g_list_length (data->rscList);
}

/*************************************
  PUBLIC function - add current datas
  in "data.rsc" utlity variable
***********************************/
void rsc_store_to_app_data (gint position, gchar *name, gchar *mail, gchar *phone, gchar *reminder, 
                            gint  type,  gint affi, gdouble cost, gint cost_type,
                            GdkRGBA  color, GdkPixbuf *pix, 
                            APP_data *data, gint id, rsc_datas *tmprsc, gboolean incrCounter)
{
  /* update internal object's counter */
 if(incrCounter)
      data->objectsCounter++;
 else
     printf ("* add ressource object without increasing internal counter *\n");


  /* WARNING ! never unref the pointer to the pixbuf, it must remain in memory */

  /* now, we store datas - I know, I can avoid all this finction, but it's cleaner ! */
  /* strings */
  if(name!=NULL)
      tmprsc->name = g_strdup_printf ("%s", name);
  else
      tmprsc->name = g_strdup_printf ("%s", "");
  if(mail!=NULL)
      tmprsc->mail = g_strdup_printf ("%s", mail);
  else
      tmprsc->mail = g_strdup_printf ("%s", "");
  if(phone!=NULL)
      tmprsc->phone = g_strdup_printf ("%s", phone);
  else
      tmprsc->phone = g_strdup_printf ("%s", "");
  if(reminder!=NULL)
      tmprsc->reminder = g_strdup_printf ("%s", reminder);
  else
      tmprsc->reminder = g_strdup_printf ("%s", "");
  tmprsc->location = NULL;

  tmprsc->pix = pix;
  tmprsc->color = color;
  tmprsc->type = type;
  tmprsc->affiliate = affi;
  tmprsc->cost_type = cost_type;
  tmprsc->cost = cost;
  if(incrCounter)
      tmprsc->id = data->objectsCounter; 
  else
      tmprsc->id = id;
}
/* //////////////////////////////////////

TODO : double clicks on listbox


////////////////////////////////////// */

gboolean on_widget_button_press_callback (GtkWidget *widget, GdkEvent *event, APP_data *data)
{
 if(event->type==GDK_2BUTTON_PRESS) {
      printf("double button press \n");// attention, c'est sur toute la list box, même là où il n'y a rien ! */
     /* it's a double-click - we musy check if it's on a selected row */

 }
 else 
    printf("simple click ou click droit \n");
return FALSE;/* mandatory in order to force GTk to select the row */
}

/**********************************
  PROTECTED : callback for button
  modify packed inside row
**********************************/
static void on_button_modify_rsc_clicked (GtkButton *button, APP_data *data)
{
  GtkListBox *box;
  /* we go along widget hierarchy inside row */
  GtkWidget *grid = gtk_widget_get_parent (GTK_WIDGET(button));
  GtkWidget *hbox = gtk_widget_get_parent (GTK_WIDGET(grid));
  GtkWidget *cadre = gtk_widget_get_parent (GTK_WIDGET(hbox));
  GtkWidget *row = gtk_widget_get_parent (GTK_WIDGET(cadre));

  gint index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW(row));
  if(index>=0) {
     box = GTK_LIST_BOX(gtk_builder_get_object (data->builder, "listboxRsc"));  
     gtk_list_box_select_row (box, GTK_LIST_BOX_ROW(row));
     data->curRsc = index;
     rsc_modify (data);
  }
}

/********************************************
  set up the "widget" for GtkListBox
  I named this GtkBox widget 'cadre' (frame 
  in french), because it's a container
********************************************/
GtkWidget *rsc_new_widget (gchar *name, gint type, gint orga, 
                         gdouble cost, gint cost_type, GdkRGBA color, APP_data *data, 
                         gboolean imgFromXml, GdkPixbuf *pixxml, rsc_datas *rsc)
{
  GtkWidget *hbox, *grid, *btn;
  GtkWidget *grid_image, *img, *label_name, *label_type;
  GtkWidget *label_aff, *label_cost, *label_color;
  GdkPixbuf *pixbuf;
  GError *err = NULL;
  gchar *sRgba, *tmpStr;

  GtkWidget *cadre = gtk_frame_new (NULL);
  g_object_set (cadre, "margin-left", 24, NULL);
  g_object_set (cadre, "margin-right", 4, NULL);
  g_object_set (cadre, "margin-top", 1, NULL);
  g_object_set (cadre, "margin-bottom", 1, NULL);
  gtk_widget_show (cadre);
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_container_add (GTK_CONTAINER (cadre), hbox);
  grid = gtk_grid_new ();
  grid_image = gtk_grid_new ();
  gtk_widget_set_halign (grid_image, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (grid_image, GTK_ALIGN_CENTER);
  gtk_grid_set_column_spacing (GTK_GRID(grid), 4);

  gtk_box_pack_start (GTK_BOX(hbox), grid_image, FALSE, FALSE, 4);
  gtk_box_pack_start (GTK_BOX(hbox), grid, TRUE, TRUE,  4);
  gtk_grid_set_column_homogeneous (GTK_GRID(grid), FALSE);
  /* image part */

  if((data->path_to_pixbuf) && (imgFromXml==FALSE)) {
     printf("chemin vers pixbuf=%s\n",data->path_to_pixbuf );
     pixbuf = gdk_pixbuf_new_from_file_at_scale (data->path_to_pixbuf, -1, RSC_AVATAR_HEIGHT*0.8, TRUE, &err);
     g_free (data->path_to_pixbuf);
     data->path_to_pixbuf = NULL;
     img = gtk_image_new_from_pixbuf (pixbuf);
     //data->rsc.pix = pixbuf;
     //data->rsc.fPersoIcon = TRUE;
     rsc->pix = pixbuf;
     rsc->fPersoIcon = TRUE;
  }
  else {
    if(imgFromXml==FALSE) {
       img = gtk_image_new_from_icon_name ("gtk-orientation-portrait", GTK_ICON_SIZE_DIALOG);
       //data->rsc.pix = NULL;
       //data->rsc.fPersoIcon = FALSE;
       rsc->pix = NULL;
       rsc->fPersoIcon = FALSE;
    }
    else {printf("I set an image from xml */\n");
          if(pixxml!=NULL) {
             img = gtk_image_new_from_pixbuf (pixxml);
             //data->rsc.pix = pixxml;
            // data->rsc.fPersoIcon = TRUE;// il faudra faire un double de pixxml !!! 
             rsc->pix = pixxml;
             rsc->fPersoIcon = TRUE;
          }
          else {printf ("I can't find xml default icon \n");
                 img = gtk_image_new_from_icon_name ("gtk-orientation-portrait", GTK_ICON_SIZE_DIALOG);
                // data->rsc.pix = NULL;
                // data->rsc.fPersoIcon = FALSE;
                 rsc->pix = NULL;
                 rsc->fPersoIcon = FALSE;
          }
    }/* xml TRUE */
  }

  gtk_grid_attach (GTK_GRID(grid_image), img, 0, 0, 1, 1);
  /* edit button */
  btn = gtk_button_new_from_icon_name ("view-more-symbolic", GTK_ICON_SIZE_MENU);
  gtk_grid_attach (GTK_GRID(grid), btn, 0, 0, 1, 1);
  /* branch callback */
  g_signal_connect ((gpointer)btn, "clicked", G_CALLBACK (on_button_modify_rsc_clicked), data);

  /* color */ 
  sRgba = misc_convert_gdkcolor_to_hex (color);
  label_color = gtk_label_new (g_strdup_printf ("<big><span background=\"%s\">   </span></big>", sRgba));
  g_free (sRgba);
  gtk_label_set_use_markup (GTK_LABEL(label_color), TRUE);
  gtk_grid_attach (GTK_GRID(grid), label_color, 1, 0, 1, 1);
  tmpStr = g_markup_printf_escaped ("<b><big>%s</big></b>", name);
  label_name = gtk_label_new (tmpStr);
  g_free (tmpStr);
  gtk_widget_set_halign (label_name, GTK_ALIGN_START);
  gtk_label_set_use_markup (GTK_LABEL(label_name), TRUE);
  gtk_grid_attach (GTK_GRID(grid), label_name, 2, 0, 1, 1);
  /* affiliation */
  label_aff = gtk_label_new (rsc_get_affiliate (orga) );
  gtk_widget_set_halign (label_aff, GTK_ALIGN_START);
  gtk_widget_set_valign (label_aff, GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID(grid), label_aff, 3, 0, 1, 1);
  /* type */
  label_type = gtk_label_new (rsc_get_label_type (type));
  gtk_widget_set_halign (label_type, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID(grid), label_type, 2, 1, 1, 1);

  /* cost, cost type */
  label_cost = gtk_label_new (rsc_get_cost_and_rate (cost, cost_type));
  g_object_set (label_cost, "margin-left", 8, NULL);
  gtk_label_set_use_markup (GTK_LABEL(label_cost), TRUE);
  gtk_grid_attach (GTK_GRID(grid), label_cost, 3, 1, 1, 1);
  gtk_widget_show_all (hbox);

 // g_signal_connect(G_OBJECT(label_name), "button-press-event",
//					 G_CALLBACK( on_widget_button_press_callback), data);

  /* we store widgets */
//  data->rsc.cadre = cadre;
//  data->rsc.cName = label_name;
//  data->rsc.cType = label_type;
//  data->rsc.cAff = label_aff;
//  data->rsc.cCost = label_cost;
//  data->rsc.cColor = label_color;
//  data->rsc.cImg = img;

  rsc->cadre = cadre;
  rsc->cName = label_name;
  rsc->cType = label_type;
  rsc->cAff = label_aff;
  rsc->cCost = label_cost;
  rsc->cColor = label_color;
  rsc->cImg = img;

  return cadre;
}

/*************************
  new ressource
*************************/
void rsc_new (APP_data *data)
{
  GtkWidget *dialog;
  gint ret=0, insertion_row, iCost, iAffi, iType, iCal;
  GdkRGBA color;
  gchar *name, *mail, *phone, *reminder;
  GtkWidget *entryName, *entryMail, *entryPhone, *entryRemind;
  GtkWidget *comboCost, *comboAffi, *comboType, *pBtnColor, *spinBtnCost;
  GtkWidget *pMonday, *pThursday, *pWednesday, *pTuesday, *pFriday, *pSaturday, *pSunday, *pCalendar, *pDayHours, *pDayMins;
  GtkWidget *cadres, *listBoxRessources;
  gdouble cost;
  rsc_datas rsc, newRsc;
  GtkListBox *box; 
  GtkListBoxRow *row;

  box = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxRsc"));
  dialog = ressources_create_dialog (FALSE, data);
  /* working days are set by default */
  ret = gtk_dialog_run (GTK_DIALOG(dialog));
  if(ret==1) {
    /* yes, we want to add a new ressource */
    entryName = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryRscName"));
    entryMail =GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryRscMail"));
    entryPhone = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryRscPhone"));
    entryRemind =GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryRscReminder"));
    comboCost = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxCost"));
    comboAffi = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxAffi"));
    comboType = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxType"));
    pBtnColor = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "colorbuttonRsc"));
    spinBtnCost = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonCost"));
    pCalendar = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxtextCal"));
    pDayHours = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonDayHours"));
    pDayMins = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonDayMins"));
    /* working days for current ressource */
    pMonday = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkMonday"));
    pTuesday = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkTuesday"));
    pWednesday = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkWednesday"));
    pThursday = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkThursday"));
    pFriday = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkFriday"));
    pSaturday = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkSaturday"));
    pSunday = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkSunday"));

    /* we get various useful values */
    name = gtk_entry_get_text (GTK_ENTRY(entryName));
    if(strlen(name)==0) {
       name = g_strdup_printf ("%s", _("Noname"));
    }
    mail = gtk_entry_get_text (GTK_ENTRY(entryMail));
    phone = gtk_entry_get_text (GTK_ENTRY(entryPhone));
    reminder = gtk_entry_get_text (GTK_ENTRY(entryRemind));
    iCost = gtk_combo_box_get_active (GTK_COMBO_BOX(comboCost));
    iAffi = gtk_combo_box_get_active (GTK_COMBO_BOX(comboAffi));
    iType = gtk_combo_box_get_active (GTK_COMBO_BOX(comboType));/* of RSC, work force for example */
    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(pBtnColor), &color);
    cost = gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinBtnCost));
    newRsc.location = NULL;
    /* working days */
    newRsc.fWorkMonday = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pMonday));
    newRsc.fWorkTuesday = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pTuesday));
    newRsc.fWorkWednesday = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pWednesday));
    newRsc.fWorkThursday = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pThursday));
    newRsc.fWorkFriday = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pFriday));
    newRsc.fWorkSaturday = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pSaturday));
    newRsc.fWorkSunday = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pSunday));

    /* calendar */
    iCal = gtk_combo_box_get_active (GTK_COMBO_BOX(pCalendar));    
    newRsc.calendar = calendars_get_id (iCal, data);
    newRsc.day_hours = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(pDayHours));
    newRsc.day_mins = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(pDayMins));

    /* we update the listbox */
    cadres = rsc_new_widget (name, iType, iAffi, cost, iCost, color, data, FALSE, NULL, &newRsc);
    /* here we have datas on widgets stored in 'rsc' variable */
    listBoxRessources = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxRsc"));

    /* new element is appended */
    /* we check cursor's position */
    data->curRsc = rsc_get_selected_row (data);
    if(data->curRsc<0)
       insertion_row = rsc_datas_len (data);
    else
       insertion_row = data->curRsc+1;

    gtk_list_box_insert (GTK_LIST_BOX(listBoxRessources), GTK_WIDGET(cadres), insertion_row);
    /* we store datas */
if(newRsc.pix==NULL)
      printf ("dans new pix vaut null \n");
    rsc_store_to_app_data (insertion_row, name, mail, phone, reminder, 
                            iType, iAffi, cost, iCost, color, newRsc.pix,
                            data , -1, &newRsc, TRUE);
    rsc_insert (insertion_row, &newRsc, data);
    rsc_autoscroll ((gdouble)(insertion_row+1)/g_list_length (data->rscList), data);
    /* we select the row */
    row = gtk_list_box_get_row_at_index (GTK_LIST_BOX(box), insertion_row);
    gtk_list_box_select_row (GTK_LIST_BOX(box), row);
    /* undo engine */
    undo_datas *tmp_value;
    tmp_value = g_malloc (sizeof(undo_datas));
    tmp_value->opCode = OP_INSERT_RSC;
    tmp_value->insertion_row = insertion_row;
    tmp_value->id = newRsc.id;
    tmp_value->datas = NULL;/* nothing to store ? */
    tmp_value->list = NULL;
    tmp_value->groups = NULL;
    undo_push (CURRENT_STACK_RESSOURCES, tmp_value, data);
    misc_display_app_status (TRUE, data);
  }
  g_object_unref (data->tmpBuilder);
  gtk_widget_destroy (GTK_WIDGET(dialog));  /* please note : with a Builder, you can only use destroy in case of the properties 'destroy with parent' isn't activated for the dualog */

}

/*******************************************
 PUBLIC : remove all associations
  ressource 'ID' <> tasks, and acts visually
 ONLY by modifying task widgets
********************************************/
void rsc_remove_tasks_widgets (gint id, APP_data *data)
{
  gint i, link;
  GList *l;
  gint tsk_list_len = g_list_length (data->tasksList);
  tasks_data *tmp_tasks_datas;

  for(i=0; i<tsk_list_len; i++ ) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     /* we test if for current task, the ressource 'ID' is a sender */
     link = links_check_element (tmp_tasks_datas->id, id, data);
     if(link>=0) {/* OK, ressource 'ID" is sender */
          /* ressources - first we remove all children from rscBox */
          tasks_remove_widgets (tmp_tasks_datas->cRscBox);
          /* we remove ONLY current link discovered above */
          links_remove_rsc_task_link (tmp_tasks_datas->id, id, data);
          /* and update it */
          tasks_add_display_ressources (tmp_tasks_datas->id, tmp_tasks_datas->cRscBox, data);
     }/* endif link */
  }/* next i */
}

/*******************************************
 PUBLIC : modify all representations
  (name, icon) of
  ressource 'ID' <> tasks, and acts visually
 ONLY by modifying task widgets
When this function is called, ressource datas
 are already modified
********************************************/
void rsc_modify_tasks_widgets (gint id, APP_data *data)
{
  gint i, link;
  GList *l;
  gint tsk_list_len = g_list_length (data->tasksList);
  tasks_data *tmp_tasks_datas;

  for(i=0; i<tsk_list_len; i++ ) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     /* we test if for current task, the ressource 'ID' is a sender */
     link = links_check_element (tmp_tasks_datas->id, id, data);
     if(link>=0) {/* OK, ressource 'ID" is sender */
          /* ressources - first we remove all children from rscBox */
          tasks_remove_widgets (tmp_tasks_datas->cRscBox);
          /* here, internal datas (ressource) are yet modified, so we have nothing to do */
          /* we only update displays */
          tasks_add_display_ressources (tmp_tasks_datas->id, tmp_tasks_datas->cRscBox, data);
     }/* endif link */
  }/* next i */
}

/*************************
  Public :  modify current
  ressource
*************************/
void rsc_modify (APP_data *data)
{
  GtkWidget *dialog;
  gint ret=0, iRow=-1, i;
  GtkListBox *box;
  GtkListBoxRow *row;
  GtkWidget *entryName, *entryMail, *entryPhone, *entryRemind, *comboCost, *comboAffi, *comboType;
  GtkWidget *pBtnColor, *buttonLogo, *spinBtnCost;
  GtkWidget *pMonday, *pThursday, *pWednesday, *pTuesday, *pFriday, *pSaturday, *pSunday, *pCalendar, *pDayHours, *pDayMins;
  rsc_datas tmpRsc, newRsc, *undoRsc;
  gchar *name, *mail, *phone, *reminder;
  GdkRGBA color;
  gint iCost, iAffi, iType, iCal;
  gdouble cost;
  GdkPixbuf *pixbuf;
  GError *err = NULL;
  undo_datas *tmp_value;

  if(data->curRsc<0)
     return;
  /* for now, we get some datas */
  box = GTK_LIST_BOX(gtk_builder_get_object (data->builder, "listboxRsc"));  
  row = gtk_list_box_get_selected_row (box);
  iRow  = gtk_list_box_row_get_index (row);
  data->curRsc = iRow;

  /* we read true data in memory - please note that strings are only passed as pointers and must be cloned */
  rsc_get_ressource_datas (iRow, &tmpRsc, data);
  /* store for undo */
  undoRsc = g_malloc (sizeof(rsc_datas));
  GdkPixbuf *pix2 = NULL;
  if(tmpRsc.pix) {printf ("poir undo je duplique pix \n");
      pix2 = gdk_pixbuf_copy (tmpRsc.pix);
  }
  rsc_get_ressource_datas (iRow, undoRsc, data);
  undoRsc->pix = pix2;
  /* we must remove some chars forbidden in XML and g_markup - & < > " ' */
  if(tmpRsc.name) {
      undoRsc->name = g_strdup_printf ("%s", tmpRsc.name);

  }
  else
     undoRsc->name = NULL;
  if(tmpRsc.mail)
      undoRsc->mail = g_strdup_printf ("%s", tmpRsc.mail);
  else
     undoRsc->mail = NULL;
  if(tmpRsc.phone)
      undoRsc->phone = g_strdup_printf ("%s", tmpRsc.phone);
  else
     undoRsc->phone = NULL;
  if(tmpRsc.reminder)
      undoRsc->reminder = g_strdup_printf ("%s", tmpRsc.reminder);
  else
     undoRsc->reminder = NULL;
  undoRsc->location = NULL; /* future improvements */

  /* we display dialog */
  dialog = ressources_create_dialog (TRUE, data);
  /* we get access to widgets and modify them */

  entryName = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryRscName"));
  entryMail = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryRscMail"));
  entryPhone = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryRscPhone"));
  entryRemind = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryRscReminder"));
  comboCost = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxCost"));
  comboAffi = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxAffi"));
  comboType = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxType"));
  pBtnColor = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "colorbuttonRsc"));
  spinBtnCost = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonCost"));
  buttonLogo = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonRscLogo"));
  pCalendar = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "comboboxtextCal"));
  pDayHours = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonDayHours"));
  pDayMins = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonDayMins"));

  /* we modify displayed datas according to our database */
  gtk_entry_set_text (GTK_ENTRY(entryName ), tmpRsc.name); 
  gtk_entry_set_text (GTK_ENTRY(entryMail ), tmpRsc.mail);
  gtk_entry_set_text (GTK_ENTRY(entryPhone ), tmpRsc.phone); 
  gtk_entry_set_text (GTK_ENTRY(entryRemind ), tmpRsc.reminder);
  gtk_combo_box_set_active (GTK_COMBO_BOX( comboType), tmpRsc.type);
  gtk_combo_box_set_active (GTK_COMBO_BOX( comboAffi), tmpRsc.affiliate);
  gtk_combo_box_set_active (GTK_COMBO_BOX( comboCost), tmpRsc.cost_type);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinBtnCost), tmpRsc.cost);
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER(pBtnColor), &tmpRsc.color);
  if(tmpRsc.pix) {/* there is already an user ùodified image */
     GtkWidget *tmpImage = gtk_image_new_from_pixbuf (tmpRsc.pix);
     /* we change the button's logo */
     gtk_button_set_image (GTK_BUTTON(buttonLogo), tmpImage);
  }

  /* working days for current ressource */
  pMonday = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkMonday"));
  pTuesday = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkTuesday"));
  pWednesday = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkWednesday"));
  pThursday = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkThursday"));
  pFriday = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkFriday"));
  pSaturday = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkSaturday"));
  pSunday = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkSunday"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pMonday), tmpRsc.fWorkMonday);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pTuesday), tmpRsc.fWorkTuesday);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pWednesday), tmpRsc.fWorkWednesday);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pThursday), tmpRsc.fWorkThursday);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pFriday), tmpRsc.fWorkFriday);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pSaturday), tmpRsc.fWorkSaturday);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pSunday), tmpRsc.fWorkSunday);
  /* display ressource's calendar - search for correct nth in calendar list, known calendar unique Id*/
  gtk_combo_box_set_active (GTK_COMBO_BOX(pCalendar), 
                            calendars_get_nth_for_id (tmpRsc.calendar, data));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(pDayHours), tmpRsc.day_hours);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(pDayMins), tmpRsc.day_mins);

  /* we run the dialog */
  ret = gtk_dialog_run (GTK_DIALOG(dialog));

  /* code 2 = remove , code 1 = OK */
  switch(ret) {
    case 1: { 
      /* undo engine */
      tmp_value = g_malloc (sizeof(undo_datas));
      tmp_value->opCode = OP_MODIFY_RSC;
      tmp_value->insertion_row = iRow;
      tmp_value->id = undoRsc->id;
      tmp_value->datas = undoRsc;
      tmp_value->list = NULL;
      tmp_value->groups = NULL;
      undo_push (CURRENT_STACK_RESSOURCES, tmp_value, data);
      /* we get modified datas */
      name = gtk_entry_get_text (GTK_ENTRY(entryName));
      if(strlen(name)==0) {
         name = g_strdup_printf ("%s", _("Noname"));
      }
      mail = gtk_entry_get_text (GTK_ENTRY(entryMail));
      phone = gtk_entry_get_text (GTK_ENTRY(entryPhone));
      reminder = gtk_entry_get_text (GTK_ENTRY(entryRemind));
      iCost = gtk_combo_box_get_active (GTK_COMBO_BOX(comboCost));
      iAffi = gtk_combo_box_get_active (GTK_COMBO_BOX(comboAffi));
      iType = gtk_combo_box_get_active (GTK_COMBO_BOX(comboType));/* of RSC, work force for example */
      gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(pBtnColor), &color);
      cost = gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinBtnCost));
      /* default for images */
      newRsc.pix = NULL;
      newRsc.fPersoIcon = FALSE;

      if(data->path_to_pixbuf ) {
         /* 2 cases : when isn't yet a logo, or the existing logo should be changed ... */
         if(tmpRsc.fPersoIcon) {
            printf ("* Attempt to replace icon *\n");
              /* we have already a pointer on current pixbuf, stored in tmpRsc.pix */
            g_object_unref (tmpRsc.pix);
            tmpRsc.pix = NULL;
         }
         else {
           /* here we simply replace 'default" icon by the choosen image */
           printf ("* Attempt to change default icon *\n");
         }/* elseif */
         /* now we can really change image datas */

          pixbuf = gdk_pixbuf_new_from_file_at_scale (data->path_to_pixbuf, -1, RSC_AVATAR_HEIGHT, TRUE, &err);
          g_free (data->path_to_pixbuf);
          data->path_to_pixbuf = NULL;
          newRsc.pix = pixbuf;
          newRsc.fPersoIcon = TRUE;
      }/* elseif path to pixbuf */
      else {/* we preserve current pixbuf */
         newRsc.pix = tmpRsc.pix;
         newRsc.fPersoIcon = tmpRsc.fPersoIcon;
      }
      /* we store datas */
      newRsc.cadre = tmpRsc.cadre;
      newRsc.name = name;
      newRsc.mail = mail;
      newRsc.phone = phone;
      newRsc.reminder = reminder;
      newRsc.location = NULL;
      newRsc.cost = cost;
      newRsc.affiliate = iAffi;
      newRsc.type = iType;
      newRsc.cost_type = iCost;
      newRsc.color = color;
      /* working days */
      newRsc.fWorkMonday = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pMonday));
      newRsc.fWorkTuesday = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pTuesday));
      newRsc.fWorkWednesday = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pWednesday));
      newRsc.fWorkThursday = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pThursday));
      newRsc.fWorkFriday = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pFriday));
      newRsc.fWorkSaturday = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pSaturday));
      newRsc.fWorkSunday = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pSunday));
      /* calendar */
      iCal = gtk_combo_box_get_active (GTK_COMBO_BOX(pCalendar));    
      newRsc.calendar = calendars_get_id (iCal, data);
      newRsc.day_hours = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(pDayHours));
      newRsc.day_mins = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(pDayMins));

      rsc_modify_datas (iRow, data, &newRsc);/* here, datas transfered to current (index) data in memory FROm newRsc */
      /* don't free memory for 'newRsc' strings : they're only pointers to widgets */

      rsc_modify_widget (iRow, data);
      /* we update tasks display */
      rsc_modify_tasks_widgets (tmpRsc.id, data);
      /* we update assignments */
      assign_remove_all_tasks (data);
      assign_draw_all_visuals (data);
      report_clear_text (data->buffer, "left");
      /* update timeline */
      timeline_remove_all_tasks (data);
      timeline_draw_all_tasks (data);
      misc_display_app_status (TRUE, data);
      break;
    }/* end case == 1*/
    case 2: {   // Gtk >=3.14 : gtk_list_box_unselect_all (box);  
      /* undo engine */
      tmp_value = g_malloc (sizeof(undo_datas));
      tmp_value->opCode = OP_REMOVE_RSC;
      tmp_value->insertion_row = iRow;
      tmp_value->id = undoRsc->id;
      tmp_value->datas = undoRsc;
      tmp_value->list = links_copy_all (data);
      tmp_value->groups = NULL;
      undo_push (CURRENT_STACK_RESSOURCES, tmp_value, data);
      /* we remove ressources before destroying links ! */ 
      rsc_remove_tasks_widgets (tmpRsc.id, data);
      /* we update all links */
      links_remove_all_rsc_links (tmpRsc.id, data);// TODO : remove this call ?
      /* we remove the ressource itself */
      rsc_remove (data->curRsc, box, data);  
      /* we update assignments */
      assign_remove_all_tasks (data);
      assign_draw_all_visuals (data);
      report_clear_text (data->buffer, "left");
      /* update timeline */
      timeline_remove_all_tasks (data);
      timeline_draw_all_tasks (data);
      misc_display_app_status (TRUE, data);
      break;
    }/* end case == 2 > remove */
    default: {/* we must remove path to logo, if exists, because user has cancelled */
         printf ("* PlanLibre - Ressources module - default switch value ! */\n");
      /* free temporary undo datas */
      if(undoRsc->name)
          g_free (undoRsc->name);
      if(undoRsc->mail)
          g_free (undoRsc->mail);
      if(undoRsc->phone)
          g_free (undoRsc->phone);
      if(undoRsc->reminder)
          g_free (undoRsc->reminder);
      if(undoRsc->location)
          g_free (undoRsc->location);
      if(undoRsc->pix)
          g_object_unref (undoRsc->pix);
      g_free (undoRsc);
      if(data->path_to_pixbuf) {
           g_free (data->path_to_pixbuf);
           data->path_to_pixbuf = NULL;
      }
    }/* end case ==0 */
  }/* end switch */
  g_object_unref (data->tmpBuilder);
  gtk_widget_destroy (GTK_WIDGET(dialog));  /* please note : with a Builder, you can only use destroy in case of the properties 'destroy with parent' isn't activated for the dualog */
}

/****************************************
  clone ressource
  argument : starting value for counting 
  and suffixing cloned rsc
****************************************/
void rsc_clone (gint index, APP_data *data)
{
  gint iRow=-1, insertion_row=-1;
  gchar *tmpStr;
  GtkListBox *box;
  GtkListBoxRow *row;
  GtkWidget *cadres, *listBoxRessources;
  rsc_datas tmpRsc;

  if(data->curRsc<0)
     return;
  /* for now, we get some datas */
  box = GTK_LIST_BOX(gtk_builder_get_object (data->builder, "listboxRsc"));  
  row = gtk_list_box_get_selected_row (box);
  iRow  = gtk_list_box_row_get_index (row);
  data->curRsc = iRow;

  /* we read true data in memory */
  rsc_get_ressource_datas (iRow, &tmpRsc, data);

  /* here we have datas on widgets stored in 'rsc' variable */
  listBoxRessources = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxRsc"));
  /* new element is appended -  we check cursor's position */
  data->curRsc = rsc_get_selected_row (data);
  insertion_row = rsc_datas_len (data);
  /* we update the listbox */
  tmpStr = g_strdup_printf("%s-%d", tmpRsc.name, index);
  cadres = rsc_new_widget (tmpStr, tmpRsc.type, tmpRsc.affiliate, 
                           tmpRsc.cost, tmpRsc.cost_type, tmpRsc.color, data, FALSE, NULL, &tmpRsc); 
  gtk_list_box_insert (GTK_LIST_BOX(listBoxRessources), GTK_WIDGET(cadres), insertion_row);
  /* we store datas */
  rsc_store_to_app_data (insertion_row, tmpStr, 
                            tmpRsc.mail, tmpRsc.phone, tmpRsc.reminder, 
                            tmpRsc.type, tmpRsc.affiliate, tmpRsc.cost, tmpRsc.cost_type,
                            tmpRsc.color, NULL,
                            data , -1, &tmpRsc, TRUE);
  rsc_insert (insertion_row, &tmpRsc, data);
  g_free (tmpStr);
  rsc_autoscroll ((gdouble)(insertion_row+1)/g_list_length (data->rscList), data);
  /* we select the row */
  row = gtk_list_box_get_row_at_index (GTK_LIST_BOX(box), insertion_row);
  gtk_list_box_select_row (GTK_LIST_BOX(box), row);
  /* undo engine */
  undo_datas *tmp_value;
  tmp_value = g_malloc (sizeof(undo_datas));
  tmp_value->opCode = OP_CLONE_RSC;
  tmp_value->insertion_row = insertion_row;
  tmp_value->id = tmpRsc.id;
  tmp_value->datas = NULL;/* nothing to store ? */
  tmp_value->list = NULL;
  tmp_value->groups = NULL;
  undo_push (CURRENT_STACK_RESSOURCES, tmp_value, data);

  misc_display_app_status (TRUE, data);

}

/****************************
  looking for ressources
  logos
****************************/
void ressources_choose_ressource_logo (GtkButton *button, APP_data *data )
{
  gchar *filename;
  GdkPixbuf *imageOrganizationLogo;
  GError *err = NULL;
  GtkWidget *buttonLogo, *dlg, *dialog;
  gint width, height;

  GtkFileFilter *filter = gtk_file_filter_new ();
  gtk_file_filter_add_pattern (filter, "*.png");
  gtk_file_filter_add_pattern (filter, "*.PNG");
  gtk_file_filter_add_pattern (filter, "*.jpg");
  gtk_file_filter_add_pattern (filter, "*.JPG");
  gtk_file_filter_add_pattern (filter, "*.jpeg");
  gtk_file_filter_add_pattern (filter, "*.JPEG");
  gtk_file_filter_add_pattern (filter, "*.svg");
  gtk_file_filter_add_pattern (filter, "*.SVG");


  dlg = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "dialogEditRessource"));
  /* we get a pointer on button's logo image */
  buttonLogo = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "buttonRscLogo"));

  gtk_file_filter_set_name (filter, _("Image files"));
  dialog = create_loadFileDialog (dlg, data, _("Import ressource logo ..."));
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  if(gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    if(data->path_to_pixbuf)
             g_free (data->path_to_pixbuf);
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    gtk_widget_destroy (GTK_WIDGET(dialog)); 
    /* we convert finename's path to URI path */
    // uri_path = g_filename_to_uri (filename, NULL, NULL);

    gdk_pixbuf_get_file_info (filename, &width, &height);
    /* we load and resize logo - Glib does an intelligent reiszing, thanks to devs ! */
    imageOrganizationLogo = gdk_pixbuf_new_from_file_at_size (filename, PROP_LOGO_WIDTH, PROP_LOGO_HEIGHT, &err);
    if(err==NULL && width>0 && height>0) {
        data->path_to_pixbuf = g_strdup_printf ("%s", filename);/* now we keep an acess ! */
        GtkWidget *tmpImage = gtk_image_new_from_pixbuf (imageOrganizationLogo);
        g_object_unref (imageOrganizationLogo);
        /* we change the button's logo */
        gtk_button_set_image (GTK_BUTTON(buttonLogo), tmpImage);
    }
    else {
        g_error_free (err);
        misc_ErrorDialog (GTK_WINDOW (dlg), _("<b>Error!</b>\n\nProblem with choosen image !\nThe icon remains unchanged."));
    }
    /* cleaning */
    g_free (filename);
  }
  else 
    gtk_widget_destroy (GTK_WIDGET(dialog)); 
}

/****************************
  main ressources dialog
****************************/
GtkWidget *ressources_create_dialog (gboolean modify, APP_data *data)
{
  GtkWidget *dialog;
  /* get datas from xml Glade file - CRITICAL : only in activate phase !!!! */
  GtkBuilder *builder =NULL;
  builder = gtk_builder_new ();
  GError *err = NULL; 
  gint i;
  GList *l;
  calendar *cal;
  
  if(!gtk_builder_add_from_file (builder, find_ui_file ("ressources.ui"), &err)) {
     g_error_free (err);
     misc_halt_after_glade_failure (data);
  }
  /* Get the dialog from the glade file. */
  gtk_builder_connect_signals (builder, data);
  dialog = GTK_WIDGET(gtk_builder_get_object (builder, "dialogEditRessource"));
  data->tmpBuilder = builder;

  /* we setup various default values */

  GtkWidget *entryRscName = GTK_WIDGET(gtk_builder_get_object (builder, "entryRscName"));
  GtkWidget *entryRscReminder = GTK_WIDGET(gtk_builder_get_object (builder, "entryRscReminder"));
  GtkWidget *entryRscMail = GTK_WIDGET(gtk_builder_get_object (builder, "entryRscMail"));
  GtkWidget *entryRscPhone = GTK_WIDGET(gtk_builder_get_object (builder, "entryRscPhone"));
  GtkWidget *comboCalendar = GTK_WIDGET(gtk_builder_get_object (builder, "comboboxtextCal"));

  gtk_entry_set_text (GTK_ENTRY(entryRscPhone ), "?"); 
  gtk_entry_set_text (GTK_ENTRY(entryRscName ), _("Nobody")); 

  /* we fill calendars and set-up to default value */
  for(i=0; i<g_list_length (data->calendars); i++) {
     l = g_list_nth (data->calendars, i);
     cal = (calendar *) l->data;
     gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(comboCalendar), cal->name);
  }
  gtk_combo_box_set_active (GTK_COMBO_BOX(comboCalendar), 0);/* default */

  /* invalidate some widgets, according to 'modify' flag */
  if(modify) {
     gtk_widget_show (GTK_WIDGET(gtk_builder_get_object (builder, "buttonRscRemove"))); 
     gtk_window_set_title (GTK_WINDOW(dialog), _("Modify a ressource" ));
  }
  else {
     gtk_widget_hide (GTK_WIDGET(gtk_builder_get_object (builder, "buttonRscRemove"))); 
     gtk_window_set_title (GTK_WINDOW(dialog), _("New ressource" ));
  }

  return dialog;
}

/***********************************
  remove all row of the current
  GtkListbox contenaing ressources
***********************************/
void rsc_remove_all_rows (APP_data *data)
{
   gint total=0, i;
   GtkListBox *box; 

   total =  g_list_length (data->rscList);
   box = GTK_LIST_BOX(gtk_builder_get_object (data->builder, "listboxRsc")); 

   for(i=total; i>0; i--) {
     rsc_remove (0, box, data);/* yes, always the FIRST */        
   }
}

/***************************************
  PUBLIC : retrieve the human readable
  "name" of a ressource, known by
  its internal ID
  returns a pointer on string, or NULL
  NEVER FREE !
**************************************/
gchar *rsc_get_name (gint id, APP_data *data)
{
  gchar *str = NULL;
  gint i = 0, ret = -1, total;
  GList *l;
  rsc_datas *tmp_rsc_datas;

  total =  g_list_length (data->rscList);
  while ( (i < total) && (ret <0) ) {
    l = g_list_nth (data->rscList, i);
    tmp_rsc_datas = (rsc_datas *)l->data;
    if(tmp_rsc_datas->id == id) {
         if(tmp_rsc_datas->name != NULL)
             str =  tmp_rsc_datas->name;
         ret = 1;
    }/* endif */
    i++;
  }/* wend */

  return str;
}

/***************************************
  PUBLIC : compute the monetary cost
  of a ressource, known by
  its internal ID
  given total minutes worked
  returns value in gdouble format
**************************************/
gdouble rsc_compute_cost (gint id, gint total_minutes, APP_data *data)
{
  gdouble val = 0;
  gint i = 0, ret = -1, total;
  GList *l;// TODO tenir compte de l'horaire maxi de la RSC
  rsc_datas *tmp_rsc_datas;

  total =  g_list_length (data->rscList);
  while ((i < total) && (ret <0)) {
    l = g_list_nth (data->rscList, i);
    tmp_rsc_datas = (rsc_datas *)l->data;
    if(tmp_rsc_datas->id == id) {
         switch(tmp_rsc_datas->cost_type) {
            case RSC_COST_HOUR: {
               val = tmp_rsc_datas->cost*((gdouble)total_minutes/60);
               break;
            }
            case RSC_COST_DAY: {
               /* how many days ? */
               val = (gdouble)total_minutes/((60*tmp_rsc_datas->day_hours)+tmp_rsc_datas->day_mins);
               val = val*tmp_rsc_datas->cost;
               break;
            }
            case RSC_COST_WEEK: {
               /* how many weeks (1 week = 5 days) ? */
               val = (gdouble)total_minutes/((60*tmp_rsc_datas->day_hours)+tmp_rsc_datas->day_mins);
               val = val/5;/* international standard for accounting, 1 week = 5 adys */
               val = val*tmp_rsc_datas->cost;
               break;
            }
            case RSC_COST_MONTH: {
               /* how many months (1 month = 25 days) ? */
               val = (gdouble)total_minutes/((60*tmp_rsc_datas->day_hours)+tmp_rsc_datas->day_mins);
               val = val /30;/* international standard for accounting, 1 month = 30 adys */
               val = val*tmp_rsc_datas->cost;
               break;
            }
            case RSC_COST_FIXED: {
               val = tmp_rsc_datas->cost;
               break;
            }
            default: {/* should NEVER happens ! */
              val = 0;
            }
         }/* end switch */
         ret = 1;
    }/* endif */
    i++;
  }/* wend */

  return val;
}

/************************************
  PUBLIC : compute number of tasks
  of ressource 'Id'
************************************/
gint rsc_get_number_tasks (gint id, APP_data *data)
{
  gint ret =0, i, link;
  GList *l;
  gint tsk_list_len = g_list_length (data->tasksList);
  tasks_data *tmp_tasks_datas;

  for(i=0; i<tsk_list_len; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     /* we test if for current task, the ressource 'ID' is a sender */
     link = links_check_element (tmp_tasks_datas->id, id, data);
     if(link>=0) {
        ret++;
     }/* endif link */
  }/* next i */

  return ret;
}

/************************************
  PUBLIC : compute absolute max tasks
  for ALL rsc
************************************/
gint rsc_get_absolute_max_tasks (APP_data *data)
{
  gint i, maxTaskByRsc = 0, n_tasks ;
  GList *l;
  rsc_datas *tmp_rsc_datas;

  for(i=0; i< g_list_length (data->rscList); i++) {
     l = g_list_nth (data->rscList, i);
     tmp_rsc_datas = (rsc_datas *)l->data;
     n_tasks = rsc_get_number_tasks (tmp_rsc_datas->id, data);
     if(n_tasks>maxTaskByRsc)
        maxTaskByRsc = n_tasks;
  }/* next */

  return maxTaskByRsc;
}

/************************************
  PUBLIC : compute absolute max tasks
  (total) for ALL rsc
************************************/
gint rsc_get_absolute_max_total_tasks (APP_data *data)
{
  gint i, maxTaskByRsc = 0, n_tasks ;
  GList *l;
  rsc_datas *tmp_rsc_datas;

  for(i=0; i< g_list_length (data->rscList); i++) {
     l = g_list_nth (data->rscList, i);
     tmp_rsc_datas = (rsc_datas *)l->data;
     n_tasks = rsc_get_number_tasks (tmp_rsc_datas->id, data);
     maxTaskByRsc = maxTaskByRsc+n_tasks;
  }/* next */

  return maxTaskByRsc;
}

/************************************
  PUBLIC : compute number of 
  COMPLETED tasks
  of ressource 'Id'
************************************/
gint rsc_get_number_tasks_done (gint id, APP_data *data)
{
  gint ret =0, i, link;
  GList *l;
  tasks_data *tmp_tasks_datas;
  gint tsk_list_len = g_list_length (data->tasksList);

  for(i=0; i<tsk_list_len; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     /* we test if for current task, the ressource 'ID' is a sender */
     link = links_check_element (tmp_tasks_datas->id, id, data);
     if(link>=0) {
        if( tmp_tasks_datas->status == TASKS_STATUS_DONE )
           ret++;
     }/* endif link */
  }/* next i */

  return ret;
}

/************************************
  PUBLIC : compute number of 
  OVERDUE tasks  of ressource 'Id'
************************************/
gint rsc_get_number_tasks_overdue (gint id, APP_data *data)
{
  gint ret =0, i, link;
  GList *l;
  gint tsk_list_len = g_list_length (data->tasksList);
  tasks_data *tmp_tasks_datas;

  for(i=0; i<tsk_list_len; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     /* we test if for current task, the ressource 'ID' is a sender */
     link = links_check_element (tmp_tasks_datas->id, id, data);
     if(link>=0) {
        if( tmp_tasks_datas->status == TASKS_STATUS_DELAY )
           ret++;
     }/* endif link */
  }/* next i */

  return ret;
}

/************************************
  PUBLIC : compute number of 
  To Go tasks of ressource 'Id'
************************************/
gint rsc_get_number_tasks_to_go (gint id, APP_data *data)
{
  gint ret =0, i, link;
  GList *l;
  gint tsk_list_len = g_list_length (data->tasksList);
  tasks_data *tmp_tasks_datas;

  for(i=0; i<tsk_list_len; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     /* we test if for current task, the ressource 'ID' is a sender */
     link = links_check_element (tmp_tasks_datas->id, id, data);
     if(link>=0) {
        if( tmp_tasks_datas->status == TASKS_STATUS_TO_GO )
           ret++;
     }/* endif link */
  }/* next i */

  return ret;
}

/************************************
  PUBLIC : compute simple average of 
  PROGRESS tasks of ressource 'Id'
************************************/
gdouble rsc_get_number_tasks_average_progress (gint id, APP_data *data)
{
  gint ret =0, i, link;
  gdouble total = 0;
  GList *l;
  gint tsk_list_len = g_list_length (data->tasksList);
  tasks_data *tmp_tasks_datas;

  for(i=0; i<tsk_list_len; i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     /* we test if for current task, the ressource 'ID' is a sender */
     link = links_check_element (tmp_tasks_datas->id, id, data);
     if(link>=0) {
        total = total+tmp_tasks_datas->progress;
        ret++;
     }/* endif link */
  }/* next i */
  if(ret>0)
    total = total/ret;

  return total;
}

/************************************
  PUBLIC : get avatar 
  of ressource 'Id'
  the pixbuf must be freed
************************************/
GdkPixbuf *rsc_get_avatar (gint id, APP_data *data)
{
   GdkPixbuf *pix = NULL;
   gint i=0, ret = -1;
   GList *l;
   rsc_datas *tmp_rsc_datas;
  
   while((i<g_list_length (data->rscList)) && (ret<0)) {
     l = g_list_nth (data->rscList, i);
     tmp_rsc_datas = (rsc_datas *)l->data;
     if(tmp_rsc_datas->id == id) {
        ret = i;
        if(tmp_rsc_datas->pix) {
             pix = gdk_pixbuf_copy (tmp_rsc_datas->pix);
        }/* endif pix */
     }/* endif 'id' */
     i++;
   }/* wend */

   return pix;
}

/********************************************
  PUBLIC : returns average
  daily working time for ressource 'ID'
  value returned in MINUTES
should be corrected by effective opening time
**********************************************/
gint rsc_get_max_daily_working_time (gint id, APP_data *data)
{
   gint minutes = 60; /* default */
   gint i=0, ret = -1;
   GList *l;
   rsc_datas *tmp_rsc_datas;

   while((i<g_list_length (data->rscList)) && (ret<0)) {
     l = g_list_nth (data->rscList, i);
     tmp_rsc_datas = (rsc_datas *)l->data;
     if(tmp_rsc_datas->id == id) {
        ret = i;
        minutes = (60*tmp_rsc_datas->day_hours)+tmp_rsc_datas->day_mins;
     }/* endif 'id' */
     i++;
   }/* wend */
   return minutes;
}

/***************************************
  PUBLIC : checks if a ressource is
  available for a certain day of week
  parameter : a pointer on a 
    ressource data structure
  returns TRUE is available, FALSE
  in other cases
***************************************/
gboolean rsc_is_available_for_day (rsc_datas *tmp_rsc_datas, gint day_of_week)
{
   gboolean ret = FALSE;

     switch(day_of_week) {
       case G_DATE_MONDAY: {
         if(tmp_rsc_datas->fWorkMonday) 
            ret = TRUE;
         break;
       }
       case G_DATE_TUESDAY: {
         if(tmp_rsc_datas->fWorkTuesday)
            ret = TRUE;
         break;
       }
       case G_DATE_WEDNESDAY: {
         if(tmp_rsc_datas->fWorkWednesday)
            ret = TRUE;
         break;
       }
       case G_DATE_THURSDAY: {
         if(tmp_rsc_datas->fWorkThursday)
            ret = TRUE;
         break;
       }
       case G_DATE_FRIDAY: {
         if(tmp_rsc_datas->fWorkFriday)
            ret++;
         break;
       }
       case G_DATE_SATURDAY: {
         if(tmp_rsc_datas->fWorkSaturday)
            ret = TRUE;
         break;
       }
       case G_DATE_SUNDAY: {
         if(tmp_rsc_datas->fWorkSunday)
            ret = TRUE;
         break;
       }
       case G_DATE_BAD_WEEKDAY: {
          printf ("* Bad date ! *\n");
       }
     }/* end switch */

   return ret;
}

/*******************************
  PUBLIC : known ressource ID,
  returns its rank in Glist
*******************************/
gint rsc_get_rank_for_id (gint id, APP_data *data)
{
  gint ret = -1, i = 0;
  GList *l;
  rsc_datas *tmp_rsc_datas;

  while((i<g_list_length (data->rscList)) && (ret<0)) {
      l = g_list_nth (data->rscList, i);
      tmp_rsc_datas = (rsc_datas *) l->data;
      if(tmp_rsc_datas->id == id)
          ret = i;
      i++;
  }
  return ret;
}

/*****************************************
 PUBLIC : autoscroll a display
 arguments : pointer on a scrolled window
  value from 0.0 to 1.0 according to
  page's position
******************************************/
void rsc_autoscroll (gdouble value, APP_data *data)
{
  GtkWidget *scrolledwindow1;

  scrolledwindow1 = GTK_WIDGET(gtk_builder_get_object (data->builder, "scrolledwindow1"));
  GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(scrolledwindow1));
  gtk_adjustment_set_value (GTK_ADJUSTMENT (adj), gtk_adjustment_get_upper (GTK_ADJUSTMENT(adj))*(value));
}
