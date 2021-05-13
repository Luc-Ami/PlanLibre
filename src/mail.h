/*
 * various utilities and constants
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#ifndef MAIL_H
#define MAIL_H


#include "support.h"

/* liststore ressources */
typedef enum {
   USE_REP_COLUMN,
   AVATAR_REP_COLUMN,
   NAME_REP_COLUMN,
   ID_REP_COLUMN,
   ID_MAIL_COLUMN,
   NB_COL_REP,
} column_reportmail_type;

/* constants */
#define MAIL_TYPE_FREE 0
#define MAIL_TYPE_REPORT 1
#define MAIL_TYPE_OVERDUE 2
#define MAIL_TYPE_COMPLETED 3
#define MAIL_TYPE_INCOMING 4
#define MAIL_TYPE_CALENDAR 5
#define RFC_LINE_LEN 76
/* functions */
gint collaborate_send_email(gchar *subject, gchar *from, GList *list_recipients, 
                            gchar **message, gchar *server, gint port, gchar *username, gchar *password, gboolean f_isSsl );
gint collaborate_send_multipart_email (gchar *subject, gchar *from, GList *list_recipients, 
                            gchar **message, gchar *server, gint port, gchar *username, gchar *password, gboolean f_isSsl,
                            gchar *path_to_file );

void collaborate_free_recipients (void);
gint collaborate_send_recipients_tasks_overdue (gboolean isFree, gchar *subject, gchar **tabMsg, gchar *server, gint port, gchar *username, gchar *password, gboolean f_isSsl, APP_data *data);
gint collaborate_send_recipients_tasks_start_tomorrow (gboolean isFree, gchar *subject, gchar **tabMsg, gchar *server, gint port, gchar *username, gchar *password, gboolean f_isSsl, APP_data *data);
gint collaborate_send_recipients_tasks_ended_yesterday (gboolean isFree, gchar *subject, gchar **tabMsg, gchar *server, gint port, gchar *username, gchar *password, gboolean f_isSsl, APP_data *data);
gint collaborate_send_recipients_tasks_ending_today (gboolean isFree, gchar *subject, gchar **tabMsg, gchar *server, gint port, gchar *username, gchar *password, gboolean f_isSsl, APP_data *data);

gint collaborate_send_manual_email (gint type_of_mail, APP_data *data);
gint collaborate_test_project_manager_mail (APP_data *data);
gint collaborate_test_server_settings (APP_data *data);
GtkWidget *collaborate_compose_email_dialog (APP_data *data);
gboolean collaborate_alert_empty_message (GtkWidget *win);

gint 
   collaborate_send_report_or_calendar (gint mode, APP_data *data);
gboolean 
   collaborate_test_not_matching (gint mail_type, APP_data *data);

#endif /* MAIL_H */
