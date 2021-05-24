/**************************
  preferences/settings
  module

**************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* translations */
#include <libintl.h>
#include <locale.h>

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "support.h"
#include "misc.h"
#include "savestate.h"
#include "settings.h"
#include "secret.h"

/* local variables */
static gchar *mdpStr = NULL;/* TODO store password */
/********************************
 callbacks

********************************/

/****************************

  main settings dialog

****************************/

static 
GtkWidget *settings_create_dialog (APP_data *data)
{
  GKeyFile *keyString;
  GdkRGBA color;
  GdkPixbuf *pix;   
  gchar *newFont, *tmpStr, *password;
  GtkWidget *dialog;
  GtkWidget *checkPromptBeforeQuit, *checkAutoSave, *checkBeforeOverWrite;
  GtkWidget *checkSsl, *entryServName, *entryServAdress, *entryServId, *spinPort;
  GtkWidget *swMailOver, *swMailEnd, *swMailTom, *swMailCplted;
  GtkWidget *entryUserPwd, *btnTxtColor, *btnTxtFnt, *imageTimer, *spinTimerRun, *spinTimerPause;
  GtkWidget *switchAllowTrelloAcess, *labelTokenStatus, *labelTrelloBtn;
  /* get datas from xml Glade file - CRITICAL : only in activate phase !!!! */
  GtkBuilder *builder = NULL;
  builder = gtk_builder_new ();
  GError *err = NULL; 

  
  if(!gtk_builder_add_from_file (builder, find_ui_file ("settings.ui"), &err)) {
     g_error_free (err);
     misc_halt_after_glade_failure (data);
  }
  /* Get the dialog from the glade file. */
  gtk_builder_connect_signals (builder, data);
  dialog = GTK_WIDGET (gtk_builder_get_object (builder, "dialogSettings"));
  data->tmpBuilder = builder;

  /* header bar */
  GtkWidget *hbar = gtk_header_bar_new ();
  gtk_widget_show (hbar);
  gtk_header_bar_set_title (GTK_HEADER_BAR (hbar), _("Settings"));
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (hbar), _("Define global settings for PlanLibre."));
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (hbar), TRUE);
  gtk_window_set_titlebar (GTK_WINDOW(dialog), hbar);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  
  gtk_header_bar_pack_start (GTK_HEADER_BAR (hbar), box);

  GtkWidget *icon =  gtk_image_new_from_icon_name  ("preferences-desktop", GTK_ICON_SIZE_DIALOG);
  gtk_container_add (GTK_CONTAINER (box), icon);
  gtk_widget_show (box);
  gtk_widget_show (icon);

  keyString = g_object_get_data (G_OBJECT(data->appWindow),"config");

  /* we setup various default values */
  imageTimer = GTK_WIDGET(gtk_builder_get_object (builder, "imagetimer"));
  pix = gdk_pixbuf_new_from_file_at_size (find_pixmap_file ("tomatotimer.png"), 24, 24, &err);
  gtk_image_set_from_pixbuf (GTK_IMAGE(imageTimer), pix);
  g_object_unref (pix);

  checkPromptBeforeQuit = GTK_WIDGET(gtk_builder_get_object (builder, "checkPromptBeforeQuit"));
  checkAutoSave = GTK_WIDGET(gtk_builder_get_object (builder, "checkAutoSave"));
  checkBeforeOverWrite = GTK_WIDGET(gtk_builder_get_object (builder, "checkBeforeOverWrite"));
  btnTxtColor = GTK_WIDGET(gtk_builder_get_object (builder, "colorbuttonTxt"));
  btnTxtFnt = GTK_WIDGET(gtk_builder_get_object (builder, "fontbuttonTxt"));
  checkSsl = GTK_WIDGET(gtk_builder_get_object (builder, "checkSSL"));
  entryServName = GTK_WIDGET(gtk_builder_get_object (builder, "entryServerName"));
  entryServAdress = GTK_WIDGET(gtk_builder_get_object (builder, "entryServerAdress"));
  entryServId = GTK_WIDGET(gtk_builder_get_object (builder, "entryServerUserId"));
  spinPort = GTK_WIDGET(gtk_builder_get_object (builder, "spinbuttonPort"));
  spinTimerRun = GTK_WIDGET(gtk_builder_get_object (builder, "spinTimerRun"));
  spinTimerPause = GTK_WIDGET(gtk_builder_get_object (builder, "spinTimerPause"));
  /* password */
  entryUserPwd = GTK_WIDGET(gtk_builder_get_object (builder, "entryUserPwd"));
  labelTokenStatus = GTK_WIDGET(gtk_builder_get_object (builder, "labelTokenStatus"));
  labelTrelloBtn = GTK_WIDGET(gtk_builder_get_object (builder, "labelTrelloBtn"));
  /* switches */
  switchAllowTrelloAcess = GTK_WIDGET(gtk_builder_get_object (builder, "switchAllowTrelloAcess"));
  swMailOver = GTK_WIDGET(gtk_builder_get_object (builder, "switchEmailOverdue"));
  swMailEnd = GTK_WIDGET(gtk_builder_get_object (builder, "switchEmailToday"));
  swMailTom = GTK_WIDGET(gtk_builder_get_object (builder, "switchEmailTomorrow"));
  swMailCplted = GTK_WIDGET(gtk_builder_get_object (builder, "switchEmailCompleted"));

  /* we are looking for Trello password in Gnome keyring */
  password= find_my_trello_password();
  if(password) {
     printf ("settings - token = %s \n", password);
     gtk_widget_set_sensitive (GTK_WIDGET(switchAllowTrelloAcess), TRUE);
     gtk_label_set_markup (GTK_LABEL (labelTokenStatus),_ ("<i>Stored</i>"));
     gtk_label_set_text (GTK_LABEL (labelTrelloBtn), _("Change user token ..."));     
     g_key_file_set_boolean (keyString, "application", "trello-user-token-defined", TRUE);
     g_free (password);
  }
  else {
     gtk_widget_set_sensitive (GTK_WIDGET(switchAllowTrelloAcess), FALSE);
     gtk_label_set_markup (GTK_LABEL (labelTokenStatus), _("<i>Not defined</i>"));
     gtk_label_set_text (GTK_LABEL (labelTrelloBtn), _("Define user token ..."));
     g_key_file_set_boolean (keyString, "application", "trello-user-token-defined", FALSE);
  }

  gtk_switch_set_active (GTK_SWITCH(switchAllowTrelloAcess),
                       g_key_file_get_boolean (keyString, "application", "trello-user-token-defined", NULL));
  gtk_switch_set_active (GTK_SWITCH(swMailOver),
                       g_key_file_get_boolean (keyString, "collaborate", "mail_tasks_overdue_today", NULL));
  gtk_switch_set_active (GTK_SWITCH(swMailEnd),
                       g_key_file_get_boolean (keyString, "collaborate", "mail_tasks_ending_today", NULL));
  gtk_switch_set_active (GTK_SWITCH(swMailTom),
                       g_key_file_get_boolean (keyString, "collaborate", "mail_tasks_start_tomorrow", NULL));
  gtk_switch_set_active (GTK_SWITCH(swMailCplted),
                       g_key_file_get_boolean (keyString, "collaborate", "mail_tasks_completed_yesterday", NULL));
  /* set values */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkAutoSave), 
           g_key_file_get_boolean (keyString, "application", "interval-save", NULL ) );
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkPromptBeforeQuit),
            g_key_file_get_boolean (keyString, "application", "prompt-before-quit",NULL ));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkBeforeOverWrite),
            g_key_file_get_boolean (keyString, "application", "prompt-before-overwrite",NULL ));

  color.red = g_key_file_get_double (keyString, "editor", "text.color.red", NULL);
  color.green = g_key_file_get_double (keyString, "editor", "text.color.green", NULL);
  color.blue = g_key_file_get_double (keyString, "editor", "text.color.blue", NULL);
  color.alpha = 1;
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER(btnTxtColor), &color);
  gtk_font_chooser_set_font (GTK_FONT_CHOOSER(btnTxtFnt ), 
                 g_key_file_get_string (keyString, "editor", "font", NULL));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkSsl), 
           g_key_file_get_boolean (keyString, "server", "ssltls", NULL ) );

  tmpStr = g_key_file_get_string (keyString, "server", "id", NULL);
  if(g_ascii_strncasecmp ((gchar*)  tmpStr, "?", sizeof(gchar))  !=  0 ) {
     gtk_entry_set_text (GTK_ENTRY(entryServId), tmpStr);
  }  
  if(tmpStr != NULL)
     g_free (tmpStr);

  tmpStr = g_key_file_get_string (keyString, "server", "name", NULL);
  if(g_ascii_strncasecmp ((gchar*)  tmpStr,"?", sizeof(gchar))  !=  0 ) {
     gtk_entry_set_text (GTK_ENTRY(entryServName), tmpStr);
  }  
  if(tmpStr != NULL)
     g_free (tmpStr);

  tmpStr = g_key_file_get_string (keyString, "server", "adress", NULL);
  if(g_ascii_strncasecmp ((gchar*)  tmpStr,"?", sizeof(gchar))  !=  0 ) {
     gtk_entry_set_text (GTK_ENTRY(entryServAdress), tmpStr);
  }  
  if(tmpStr != NULL)
     g_free (tmpStr);

  gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinPort), g_key_file_get_integer (keyString, "server", "port", NULL ));
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinTimerRun), g_key_file_get_integer (keyString, "application", "timer-run-time", NULL ));
  gtk_spin_button_set_value (GTK_SPIN_BUTTON(spinTimerPause), g_key_file_get_integer (keyString, "application", "timer-pause-time", NULL ));

  /* password */
  if(mdpStr != NULL)
      gtk_entry_set_text (GTK_ENTRY(entryUserPwd), mdpStr);
  return dialog;
}


void settings (APP_data *data)
{
  GKeyFile *keyString;
  GdkRGBA colorTxt;
  gchar *newFont, *tmpStr, *password;
  GtkWidget *dialog;
  GtkWidget *checkPromptBeforeQuit, *checkAutoSave, *checkBeforeOverWrite;
  GtkWidget *checkSsl, *entryServName, *entryServAdress, *entryServId, *spinPort;
  GtkWidget *entryUserPwd, *btnTxtColor, *btnTxtFnt, *imgAutoSave, *spinTimerRun, *spinTimerPause;
  GtkWidget *switchAllowTrelloAcess, *menuitemExportTrello;
  gboolean fMailOver, fMailToday, fMailYest, fMailTom;
  gint ret=0;


  dialog = settings_create_dialog (data);

  /* we get some flags in order to prepare a message dialog about immediate mail sending */
  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config"); 
  fMailOver = g_key_file_get_boolean (keyString, "collaborate", "mail_tasks_overdue_today", NULL);
  fMailToday = g_key_file_get_boolean (keyString, "collaborate", "mail_tasks_ending_today", NULL);
  fMailYest = g_key_file_get_boolean (keyString, "collaborate", "mail_tasks_completed_yesterday", NULL);
  fMailTom = g_key_file_get_boolean (keyString, "collaborate", "mail_tasks_start_tomorrow", NULL);

  ret = gtk_dialog_run (GTK_DIALOG(dialog));
  /* ret==1 for [OK] */
  if(ret==1) {
     /* we setup various default values */
     spinTimerRun = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinTimerRun"));
     spinTimerPause = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinTimerPause"));
     checkPromptBeforeQuit = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkPromptBeforeQuit"));
     checkAutoSave = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkAutoSave"));
     checkBeforeOverWrite = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkBeforeOverWrite"));
     btnTxtColor = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "colorbuttonTxt"));
     btnTxtFnt = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "fontbuttonTxt"));
     imgAutoSave = GTK_WIDGET(gtk_builder_get_object (data->builder, "image_task_due"));
     checkSsl = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "checkSSL"));
     entryServName = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryServerName"));
     entryServAdress = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryServerAdress"));
     entryServId = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryServerUserId"));
     spinPort = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "spinbuttonPort"));
     entryUserPwd = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "entryUserPwd"));
     /* help message if user activated previously INACTIVES mailing facilities */
     if( ((!fMailOver) && (g_key_file_get_boolean (keyString, "collaborate", "mail_tasks_overdue_today", NULL)))
        || ((!fMailToday) && (g_key_file_get_boolean (keyString, "collaborate", "mail_tasks_ending_today", NULL)))
        || ((!fMailYest) && (g_key_file_get_boolean (keyString, "collaborate", "mail_tasks_completed_yesterday", NULL)))
        || ((!fMailTom) && (g_key_file_get_boolean (keyString, "collaborate", "mail_tasks_start_tomorrow", NULL)))
       ) {
        tmpStr = g_strdup_printf ("%s", _("Warning !\n\nYou've activated one or more auto-mailing settings.\nNow, you should launch <b>manually</b> your mailings (see menu <b>collaborate</b>),\nor wait until tomorrow for <b>automatic</b> mailings."));
        misc_ErrorDialog (GTK_WIDGET(dialog), tmpStr );
        g_free (tmpStr);

     }
     /* trello settings */
     switchAllowTrelloAcess = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "switchAllowTrelloAcess"));
     menuitemExportTrello = GTK_WIDGET(gtk_builder_get_object (data->builder, "menuitemExportTrello"));
     password = find_my_trello_password ();
     if(password) {
        g_key_file_set_boolean (keyString, "application", "trello-user-token-defined", TRUE);  
        gtk_widget_set_sensitive (GTK_WIDGET(switchAllowTrelloAcess), TRUE);
        gtk_widget_set_sensitive (GTK_WIDGET(menuitemExportTrello), TRUE);
        g_free (password);
     }
     else {
        g_key_file_set_boolean (keyString, "application", "trello-user-token-defined", FALSE);  
        gtk_widget_set_sensitive (GTK_WIDGET(switchAllowTrelloAcess), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET(menuitemExportTrello), FALSE);
     }
     /* server settings */
     g_key_file_set_integer (keyString, "server", "port", (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinPort)) );
     g_key_file_set_boolean (keyString, "server", "ssltls",  
                 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkSsl)));

     if(gtk_entry_get_text (GTK_ENTRY(entryServName)) != NULL) 
           g_key_file_set_string(keyString, "server", "name", gtk_entry_get_text (GTK_ENTRY(entryServName)));
     else
           g_key_file_set_string(keyString, "server", "name", "?");

     if(gtk_entry_get_text (GTK_ENTRY(entryServAdress)) != NULL) 
           g_key_file_set_string(keyString, "server", "adress", gtk_entry_get_text (GTK_ENTRY(entryServAdress)));
     else
           g_key_file_set_string(keyString, "server", "adress", "?");

     if(gtk_entry_get_text (GTK_ENTRY(entryServId)) != NULL) 
           g_key_file_set_string(keyString, "server", "id", gtk_entry_get_text (GTK_ENTRY(entryServId)));
     else
           g_key_file_set_string(keyString, "server", "id", "?");

     /* password */
     if(gtk_entry_get_text (GTK_ENTRY(entryUserPwd)) != NULL) {
         if(mdpStr != NULL)
            g_free (mdpStr);
         mdpStr = g_strdup_printf ("%s", gtk_entry_get_text (GTK_ENTRY(entryUserPwd)) );
     }

     /* we get the current RGBA color */
     gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(btnTxtColor), &colorTxt);

     /* global prefs */
     g_key_file_set_boolean (keyString, "application", "interval-save",  
                 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkAutoSave)));
     g_key_file_set_boolean (keyString, "application", "prompt-before-quit",  
                 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkPromptBeforeQuit)));
     g_key_file_set_boolean (keyString, "application", "prompt-before-overwrite",  
                 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkBeforeOverWrite)));
     g_key_file_set_integer (keyString, "application", "timer-run-time", 
                            (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinTimerRun)));
     g_key_file_set_integer (keyString, "application", "timer-pause-time", 
                            (gint) gtk_spin_button_get_value (GTK_SPIN_BUTTON(spinTimerPause)));
     /* diagrams legends color */
     g_key_file_set_double (keyString, "editor", "text.color.red", colorTxt.red);
     g_key_file_set_double (keyString, "editor", "text.color.green", colorTxt.green);
     g_key_file_set_double (keyString, "editor", "text.color.blue", colorTxt.blue);
     /* modify editor's textview default font */
     const gchar *fntFamily = NULL;
     gint fntSize = 12;
     PangoFontDescription *desc;
     newFont = gtk_font_chooser_get_font (GTK_FONT_CHOOSER( btnTxtFnt));
     g_key_file_set_string (keyString, "editor", "font", newFont);
     if(newFont != NULL) { 
        desc = pango_font_description_from_string (newFont);
        if(desc != NULL) {
          fntFamily = pango_font_description_get_family (desc);
          fntSize = pango_font_description_get_size (desc)/1000;
        }
        g_free(newFont);
     }/* endif newfont */
     /* now we use CSS to modify textview font */
     misc_set_font_color_settings (data);

     /* auto-save */
     if(g_key_file_get_boolean (keyString, "application", "interval-save",  NULL) ) {
       gtk_widget_show (imgAutoSave);
     }
     else
      gtk_widget_hide (imgAutoSave);

  }/* endif [Ok] */

  g_object_unref (data->tmpBuilder);
  gtk_widget_destroy (GTK_WIDGET(dialog));  /* please note : with a Builder, you can only use destroy in case of the properties 'destroy with parent' isn't activated for the dualog */
}
/*

  reacts to changes in password tuped, and display convenient icon

*/
void on_password_entry_changed (GtkEditable *editable, APP_data *data)
{
  GtkWidget *entry, *image;
  const gchar *str;
  gint i = 0;
  gboolean fErr = FALSE;
  
  image = GTK_WIDGET(gtk_builder_get_object (data->tmpBuilder, "imagePasswordOk")); 
  str = gtk_entry_get_text (GTK_ENTRY(editable));
  /* we check entry and modify the right side icon */
  for(i=0; i<=strlen(str); i++) {
    if(str[i] == ' ') {
       fErr = TRUE;
    }
    if(str[i] < 0) {/* yes, we are ont signed shortint universe, and 128 == -1 */
       fErr = TRUE;
    }
  }
  if(fErr)
     gtk_image_set_from_icon_name (GTK_IMAGE(image), "gtk-dialog-warning", GTK_ICON_SIZE_MENU);
  else
     gtk_image_set_from_icon_name (GTK_IMAGE(image), "gtk-ok", GTK_ICON_SIZE_MENU);

}

/********************************
  PUBLIC : mini dialog to get
  the password
  returns -1 if user cancelled,
  otherwise the new password is
  store in config file (in memory)
*********************************/
gint settings_dialog_password (APP_data *data)
{
  gint ret = 0;
  GKeyFile *keyString;
  GtkWidget *dialog, *entry;
  gchar *tmpStr;
  const gchar *entryStr;
  GtkBuilder *builder = NULL;
  builder = gtk_builder_new ();
  GError *err = NULL; 

  
  if(!gtk_builder_add_from_file (builder, find_ui_file ("password.ui"), &err)) {
     g_error_free (err);
     misc_halt_after_glade_failure (data);
  }

  /* Get the dialog from the glade file. */
  gtk_builder_connect_signals (builder, data);
  dialog = GTK_WIDGET(gtk_builder_get_object (builder, "dialogRequestPswd"));
  data->tmpBuilder = builder;

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config"); 

  ret = gtk_dialog_run (GTK_DIALOG(dialog));
  if(ret==1) {
     entry = GTK_WIDGET (gtk_builder_get_object (data->tmpBuilder, "entryPassWord")); 
     /* some entry checking */
     entryStr = gtk_entry_get_text (GTK_ENTRY(entry));
     if(strlen (entryStr) == 0) {
        ret = 0; /* in order to exit from mail sending process */
        tmpStr = g_strdup_printf ("%s", _("Can't do that !\n\nServers <b>request</b> a password.\nMail sending aborted."));
        misc_ErrorDialog (GTK_WIDGET(dialog), tmpStr );
        g_free (tmpStr);
     }
     else {
         if(mdpStr != NULL) {
            g_free (mdpStr);
         }
         mdpStr = g_strdup_printf ("%s", entryStr );
     }
  }

  g_object_unref (data->tmpBuilder);
  gtk_widget_destroy (GTK_WIDGET(dialog));  /* please note : with a Builder, you can only use destroy in case of the properties 'destroy with parent' isn't activated for the dualog */

  return ret;
}

/***************************
  PUBLIC : returns current
  password
**************************/
const gchar *settings_get_password (void)
{
  return mdpStr;
}
