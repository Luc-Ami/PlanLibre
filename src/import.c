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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>
#include <glib/gstdio.h> /* g_fopen, etc */
#include "support.h" /* glade requirement */
#include "misc.h"
#include "tasks.h"
#include "files.h"
#include "ressources.h" 
#include "import.h"
#include "undoredo.h"

/****************************

  FUNCTIONS

****************************/


/****************************************
  LOCAL : zstrktoK 
since strtok is limited, I use zstrtok, example here ;
https://stackoverflow.com/questions/3889992/how-does-strtok-split-the-string-into-tokens-in-c
thanks to developers
****************************************/
gchar *zStrtok (gchar *str, const gchar *delim) {
    static gchar *static_str=0;      /* var to store last address */
    int index=0, strlength=0;           /* integers for indexes */
    int found = 0;                  /* check if delim is found */

    /* delimiter cannot be NULL
    * if no more char left, return NULL as well
    */
    if (delim==0 || (str == 0 && static_str == 0))
        return 0;

    if (str == 0)
        str = static_str;

    /* get length of string */
    while(str[strlength])
        strlength++;

    /* find the first occurance of delim */
    for (index=0;index<strlength;index++)
        if (str[index]==delim[0]) {
            found=1;
            break;
        }

    /* if delim is not contained in str, return str */
    if (!found) {
        static_str = 0;
        return str;
    }

    /* check for consecutive delimiters
    *if first char is delim, return delim
    */
    if (str[0]==delim[0]) {
        static_str = (str + 1);
        return (gchar *)delim;
    }

    /* terminate the string
    * this assignmetn requires char[], so str has to
    * be char[] rather than *char
    */
    str[index] = '\0';

    /* save the rest of the string */
    if ((str + index + 1)!=0)
        static_str = (str + index + 1);
    else
        static_str = 0;

        return str;
}

/***********************************
 PUBLIC : Open a SCV file
 datas arre appended to ressources
 importa routine from :
https://stackoverflow.com/questions/32884789/import-csv-into-an-array-using-c
 thanks !
for getline : https://pubs.opengroup.org/onlinepubs/9699919799/functions/getline.html

 the datas must be separated by a semi-colon ;
 first line of CSV is only descriptive
 13 columns are used :

NAME;MAIL;PHONE;REMINDER;MAX DAILY WORKING TIME;HOUR COST;MONDAY;TUESDAY;WEDNESDAY;THURSDAY;FRIDAY;SATURDAY;SUNDAY
since strtok is limited, I use zstrtok, example here ;
https://stackoverflow.com/questions/3889992/how-does-strtok-split-the-string-into-tokens-in-c
thanks to developers
**********************************************/

gint import_csv_file_to_rsc (gchar *path_to_file, APP_data *data)
{
  gint rr = -1, i, id, lenRsc = 0, hId, hour_day, min_day, wDay;
  gint type, affi, cost_type, rowIndex = 0, colIndex, lineIndex = 0;
  size_t len = 0;
  ssize_t read;
  gboolean fWDay;
  gdouble cost;
  gchar *line = NULL, *token = NULL, *sTime = NULL, *sToken = NULL, *sCost;
  gchar *name=NULL, *mail=NULL, *phone=NULL, *reminder=NULL;
  GtkWidget *cadres, *listBoxRessources;
  rsc_datas newRsc;
  GdkRGBA  color;

  /* current ressources' datas  */
  listBoxRessources = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxRsc"));
  lenRsc = g_list_length (data->rscList);
  rowIndex = lenRsc;
  hId = misc_get_highest_id (data);
  /* open file */
  FILE *fp = fopen (path_to_file, "r");
  if(fp != NULL)  {
    while(( read = getline (&line, &len, fp)) != -1)
    {
      colIndex = 0;
      type = 0;
      affi = 0;
      cost_type = 0;
      cost = 0;
      /* default working days */
      color.red = 0;
      color.green = 0;
      color.blue = 0;
      newRsc.color = color;
      newRsc.pix = NULL;
      newRsc.fWorkMonday = TRUE;
      newRsc.fWorkTuesday = TRUE;
      newRsc.fWorkWednesday = TRUE;
      newRsc.fWorkThursday = TRUE;
      newRsc.fWorkFriday = TRUE;
      newRsc.fWorkSaturday = FALSE;
      newRsc.fWorkSunday = FALSE;
      /* calendar */
      newRsc.calendar = 0; /* default */
      newRsc.day_hours = 8;
      newRsc.day_mins = 0;
      /* we manage datas only starting with 2nd line */
      if(lineIndex>0) {
         for(token = zStrtok (line, ";"); token != NULL && colIndex < MAXRSCVARS; token = zStrtok (NULL, ";")) {
           if(colIndex==0) {/* name */
              if(strlen(token)>0) {
                /* token must be cleaned ! TODO */
                name = g_strdup_printf ("%s", token);
              }
              else
                name = g_strdup_printf ("?");

           }/* colIndex = 0 */
           if(colIndex==1) {/* mail */
              if(strlen(token)>0) {
                /* token must be cleaned ! TODO */
                mail = g_strdup_printf ("%s", token);
              }
              else
                mail = g_strdup_printf ("?");

           }/* colIndex = 1 */
           if(colIndex==2) {/* phone */
              if(strlen(token)>0) {
                /* token must be cleaned ! TODO */
                phone = g_strdup_printf ("%s", token);
              }
              else
                phone = g_strdup_printf ("?");

           }/* colIndex = 2 */
           if(colIndex==3) {/* reminder */
              if(strlen(token)>0) {
                /* token must be cleaned ! TODO */
                reminder = g_strdup_printf ("%s", token);
              }
              else
                reminder = g_strdup_printf ("?");

           }/* colIndex = 3 */
           if(colIndex==4) {/* max daily time */
              if(strlen(token)>0) {
                /* time value must be in this format : 12H35  H (or h) used as delimiter */
                sTime = g_strdup_printf ("%s", token);
                /* force to upper case */
                for(i=0;i<strlen (sTime);i++) {
                   if(sTime[i]=='h')
                     sTime[i]='H';
                }
                sToken = strtok (sTime, "H");
                hour_day =  (gint) ABS (g_ascii_strtod (g_strdup_printf("%s", sToken), NULL));
                sToken = strtok (NULL, "H");  
                min_day =  (gint) ABS (g_ascii_strtod (g_strdup_printf("%s", sToken), NULL));              
                g_free (sTime);
                if(hour_day>23)
                     hour_day = 23;
                if(min_day>59)
                     min_day = 59;
              }
              else {
                hour_day = 8;
                min_day = 0;
              }
              newRsc.day_hours = hour_day;
              newRsc.day_mins = min_day;
           }/* colIndex = 4 */

           if(colIndex==5) {/* hour cost */
              if(strlen(token)>0) {
                sCost = g_strdup_printf ("%s", token);
                /* force to decimal point */
                for(i=0;i<strlen (sCost);i++) {
                   if(sCost[i]==',')
                     sCost[i]='.';
                }
                cost = g_ascii_strtod (sCost, NULL);
                g_free (sCost);
              }
              else
                cost = 0;
           }/* colIndex = 5 */

           /* days */
           if(colIndex>5) {
              if(strlen (token)>0) {
                wDay = (gint) g_ascii_strtod (g_strdup_printf("%s", token), NULL);
                fWDay = FALSE;
                if(wDay==1)
                   fWDay = TRUE;
                switch(colIndex) {
                  case 6: {
                     newRsc.fWorkMonday = fWDay;
                     break;
                  }
                  case 7: {
                     newRsc.fWorkTuesday = fWDay;
                     break;
                  }
                  case 8: {
                     newRsc.fWorkWednesday = fWDay;
                     break;
                  }
                  case 9: {
                     newRsc.fWorkThursday = fWDay;
                     break;
                  }
                  case 10: {
                     newRsc.fWorkFriday = fWDay;
                     break;
                  }
                  case 11: {
                     newRsc.fWorkSaturday = fWDay;
                     break;
                  }
                  case 12: {
                     newRsc.fWorkSunday = fWDay;
                     break;
                  }
                }/* end switch */
              }
           }/* endif colIndex >5 */
           colIndex++;
         }/* next token */
         /* TODO seulement si une valeur est lue ! */
         id = hId+1;
         hId++;
        /* update widgets BEFORE storage */
        cadres = rsc_new_widget (name, type, affi, cost, cost_type, color, data, FALSE, NULL, &newRsc);
        /* update listbox */
        gtk_list_box_insert (GTK_LIST_BOX(listBoxRessources), GTK_WIDGET(cadres), rowIndex);
        /* store in memory */
        rsc_store_to_app_data (rowIndex, name, mail, phone, reminder, 
                              type, affi, cost, cost_type, color, NULL, data, id, &newRsc, TRUE);
        /* finalize - i.e. datas are transfered from app to memory */
        rsc_insert (rowIndex, &newRsc, data);
        /* undo engine */
        undo_datas *tmp_value;
        tmp_value = g_malloc (sizeof(undo_datas));
        tmp_value->opCode = OP_APPEND_RSC;
        tmp_value->insertion_row = rowIndex;
        tmp_value->id = newRsc.id;
        tmp_value->datas = NULL;/* nothing to store ? */
        tmp_value->list = NULL;
        tmp_value->groups = NULL;
        undo_push (CURRENT_STACK_RESSOURCES, tmp_value, data);
      }/* endif rowindex >0 */
      rowIndex++;
      lineIndex++;
      /* resets */
      if(name!=NULL) {
        g_free (name);
      }
      name=NULL;
      if(mail!=NULL) {
        g_free (mail);
      }
      mail=NULL;
      if(phone!=NULL) {
        g_free (phone);
      }
      phone=NULL;
      if(reminder!=NULL) {
        g_free (reminder);
      }
      reminder=NULL;
    }/* wend */
    if(line!=NULL)
       g_free (line);

    fclose (fp);
    gchar *msg = g_strdup_printf ( _("<b><big>%d</big></b> ressource(s)\nsucessfully appended."), lineIndex-1);
    misc_InfoDialog (data->appWindow, msg);
    g_free (msg);
    rr = 0;
  }/* endif FP */

  misc_display_app_status (TRUE, data);
  return rr;
}

/********************************************************
  PUBLIC : import tasks from a CSV file
  returns <>0 if an error happened
VARS : NAME	DAY	MONTH	YEAR	DURATION_DAYS	DURATION_HOURS	PRIORITY	PROGRESS
 lim = MAXTSKVARS
********************************************************/
static gint import_csv_file_to_tasks (gchar *path_to_file, APP_data *data)
{
  gint rr = -1, lineIndex = 0, colIndex, rowIndex = 0, i, id, hId;
  gchar *name = NULL, *line = NULL, *token = NULL, *sInt = NULL, *human_date;
  gint day, month, year, dur_days, dur_hours, priority, status, curPty;
  gdouble progress;
  size_t len = 0;
  ssize_t read;
  GDate cur_date, start_date, end_date, today;
  GtkWidget *cadres, *boxTasks;
  tasks_data temp;
  GdkRGBA  color;

  g_date_set_time_t (&today, time (NULL)); /* date of today */
  g_date_set_dmy (&start_date, data->properties.start_nthDay, data->properties.start_nthMonth, data->properties.start_nthYear);
  g_date_set_dmy (&end_date, data->properties.end_nthDay, data->properties.end_nthMonth, data->properties.end_nthYear);
  boxTasks = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxTasks"));
  color.red = 0;
  color.green = 0;
  color.blue = 0;
  rowIndex = tasks_datas_len (data);
  hId = misc_get_highest_id (data);

  /* open file */
  FILE *fp = fopen (path_to_file, "r");
  if(fp != NULL)  {
    while(( read = getline (&line, &len, fp)) != -1) {
      if(lineIndex>0) {
         colIndex = 0;
         for(token = zStrtok (line, ";"); token != NULL && colIndex < MAXTSKVARS; token = zStrtok (NULL, ";")) {
           if(colIndex==0) {/* name */
              if(strlen (token)>0) {
                sInt = g_strdup_printf ("%s", token);
                misc_erase_c (sInt, '"');
                name = g_strdup_printf ("%s", sInt);
                g_free (sInt);
              } else
                   name = g_strdup_printf ("%s", _("Noname imported task"));
           }/* colIndex = 0 */
           if(colIndex==1) {/* day */
              if(strlen(token)>0) {
                sInt = g_strdup_printf ("%s", token);
                day = ABS (g_ascii_strtod (sInt, NULL));
                g_free (sInt);
              }
              else
                day = 1;
           }/* colIndex = 1 */
           if(colIndex==2) {/* month */
              if(strlen (token)>0) {
                sInt = g_strdup_printf ("%s", token);
                month = ABS (g_ascii_strtod (sInt, NULL));
                g_free (sInt);
              }
              else
                month = 1;
           }/* colIndex = 2 */
           if(colIndex==3) {/* year */
              if(strlen (token)>0) {
                sInt = g_strdup_printf ("%s", token);
                year = ABS (g_ascii_strtod (sInt, NULL));
                g_free (sInt);
              }
              else
                year = 1970;
           }/* colIndex = 3 */
           if(colIndex==4) {/* duration day */
              if(strlen (token)>0) {
                sInt = g_strdup_printf ("%s", token);
                dur_days = ABS (g_ascii_strtod (sInt, NULL));
                g_free (sInt);
              }
              else
                dur_days = 1;
           }/* colIndex = 4 */
           if(colIndex==5) {/* duration hours */
              if(strlen (token)>0) {
                sInt = g_strdup_printf ("%s", token);
                dur_hours = ABS (g_ascii_strtod (sInt, NULL));
                g_free (sInt);
              }
              else
                dur_hours = 0;
              if(dur_hours>23)
                 dur_hours = 23;
           }/* colIndex = 5 */
           if(colIndex==6) {/* priority */
              if(strlen (token)>0) {
                sInt = g_strdup_printf ("%s", token);
                priority = ABS(g_ascii_strtod (sInt, NULL));
                g_free (sInt);
              }
              else
                priority = 1;
              if(priority<1)
                 priority = 1;
              if(priority>3)
                priority = 3;
           }/* colIndex = 6 */
           if(colIndex==7) {/* progress */
              if(strlen (token)>0) {
                sInt = g_strdup_printf ("%s", token);
                /* force to decimal point */
                for(i=0;i<strlen (sInt);i++) {
                   if(sInt[i]==',')
                     sInt[i]='.';
                }
                progress = ABS (g_ascii_strtod (sInt, NULL));
                g_free (sInt);
              }
              else
                progress = 0;
              if(progress>100)
                  progress = 100;
           }/* colIndex = 7 */

           colIndex++;           
         }/* next token */
         /* check dates validity */
printf("date lue =%d %d %d \n", day, month, year);
         g_date_set_dmy (&cur_date, day, month, year);
         if(!g_date_valid (&cur_date)) {
            g_date_set_dmy (&cur_date, data->properties.start_nthDay, 
                         data->properties.start_nthMonth, data->properties.start_nthYear);    
            day = g_date_get_day (&cur_date);
            month = g_date_get_month (&cur_date);
            year = g_date_get_year (&cur_date);
         }
         /* check if cur date>= project's start date */
         if(g_date_compare (&cur_date, &start_date)<0) {
              g_date_set_dmy (&cur_date, data->properties.start_nthDay, data->properties.start_nthMonth, 
                              data->properties.start_nthYear);  
            day = g_date_get_day (&cur_date);
            month = g_date_get_month (&cur_date);
            year = g_date_get_year (&cur_date);        
         }
         /* check if date+duration<project ending date */
         g_date_add_days (&cur_date, dur_days);
         if(g_date_compare (&cur_date, &end_date)>0) {
             dur_hours = 0; /* to 'force" */
             while(g_date_compare (&cur_date, &end_date)>0) {
                dur_days--;
                g_date_subtract_days (&cur_date, 1);
             }
         }
         if(dur_days<=0)
             dur_days = 1;

         /* store */
         id = hId+1;
         hId++;
         g_date_set_dmy (&cur_date, day, month, year);
         human_date = misc_convert_date_to_friendly_str (&cur_date);
         temp.type = TASKS_TYPE_TASK;
         temp.location = NULL;
         if(progress==0)
            status = TASKS_STATUS_TO_GO;
         if(progress>0 && progress<100)
            status = TASKS_STATUS_RUN;
         if(progress==100)
            status = TASKS_STATUS_DONE;
         g_date_add_days (&cur_date, dur_days);
         if((g_date_compare (&cur_date, &today)<0) && (progress<100))
            status = TASKS_STATUS_DELAY;
         if(priority==1)
           curPty = 2;
         if(priority==2)
           curPty = 1;
         if(priority==3)
           curPty = 0;
         cadres = tasks_new_widget (name, 
                              g_strdup_printf (_("%s, %d day(s)"), human_date, dur_days), 
                              progress, curPty, 0, 0, color, status, &temp, data);/* categ 0= material */
         g_free (human_date);
         /* here, all widget pointers are stored in 'temp' */
         gtk_list_box_insert (GTK_LIST_BOX(boxTasks), GTK_WIDGET(cadres), rowIndex);  
         tasks_store_to_tmp_data (rowIndex, name, day, month, year, 
                            g_date_get_day (&cur_date), 
                            g_date_get_month (&cur_date), 
                            g_date_get_year (&cur_date),
                            -1, -1, -1, 
                            progress, curPty, 0, status, 
                            color, data, id, FALSE, TASKS_AS_SOON, dur_days, dur_hours, 0, 0, 0, &temp );/* calendar id = 0 */
        tasks_insert (rowIndex, data, &temp);/* here we store id */
        dashboard_add_task (rowIndex, data);/* nothing is drawn until at least ONE ressource is affected */
        /* undo engine - we just have to get a pointer on ID */
        undo_datas *tmp_value;
        tmp_value = g_malloc (sizeof(undo_datas));
        tmp_value->opCode = OP_APPEND_TASK;
        tmp_value->insertion_row = rowIndex;
        tmp_value->id = temp.id;
        tmp_value->datas = NULL;/* nothing to store ? */
        tmp_value->list = NULL;
        tmp_value->groups = NULL;
        undo_push (CURRENT_STACK_TASKS, tmp_value, data);
      }/* endif lineIndex>0 */

      lineIndex++;
      rowIndex++;
      /* resets */
      if(name!=NULL) {
        g_free (name);
      }
      name=NULL;
    }/* wend */
    if(line!=NULL)
       g_free (line);
    fclose (fp);
    /* update timeline */
    timeline_remove_all_tasks (data);
    timeline_draw_all_tasks (data);
    if(lineIndex>1) {
       gchar *msg = g_strdup_printf ( _("<b><big>%d</big></b> task(s)\nsucessfully appended.\nDashboard will be updated <u>only</u> when ressoources are assigned to new task(s)."), lineIndex-1);
       misc_InfoDialog (data->appWindow, msg);
      g_free (msg);
      rr = 0;
      misc_display_app_status (TRUE, data);
    }
  }/* endif FP */
  return rr;
}

/*****************************************
  PUBLIC : import tasks form spreadsheet
  CSV file
****************************************/
void import_tasks_from_csv (APP_data *data)
{
  GKeyFile *keyString;
  gchar *path_to_file, *filename;
  gint ret;
  GtkFileFilter *filter = gtk_file_filter_new ();

  gtk_file_filter_add_pattern (filter, "*.csv");
  gtk_file_filter_set_name (filter, _("Spreadsheets CSV files"));

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");

  GtkWidget *dialog = create_loadFileDialog(data->appWindow, data, _("Append tasks from file ..."));
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir());
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  /* run dialog for file selection */
  if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    gtk_widget_destroy (GTK_WIDGET(dialog)); 
    ret = import_csv_file_to_tasks (filename, data);
    if(ret!=0) {
        gchar *msg = g_strdup_printf ("%s", _("Sorry !\nAn error happened during file importation.\nOperation finished with errors. You can try again, or\ncheck your system."));
        misc_ErrorDialog (data->appWindow, msg);
        g_free (msg);
    }
    g_free (filename);
  }
  else
     gtk_widget_destroy (GTK_WIDGET(dialog)); 
}

/*************************************
  PUNLIC : import adress book
  from CSV file, for example
  from Thunderbird adress book
  VARS : first_name, last_name, name_to_display, email1, email2[not used], scren_name [not used], prof. phone, personal phone [not used], other fields not  used
*************************************/
gint import_adress_book_file_to_rsc (gchar *path_to_file, APP_data *data)
{
  gint rr = -1, i, id, lenRsc = 0, hId, hour_day, min_day, wDay;
  gint type, affi, cost_type, rowIndex = 0, colIndex, lineIndex = 0;
  size_t len = 0;
  ssize_t read;
  gboolean fWDay;
  gdouble cost;
  gchar *line = NULL, *token = NULL, *sTime = NULL, *sToken = NULL, *sCost;
  gchar *name=NULL, *mail=NULL, *phone=NULL, *reminder=NULL;
  GtkWidget *cadres, *listBoxRessources;
  rsc_datas newRsc;
  GdkRGBA  color;

  gchar *first_name = NULL, *mail1 = NULL, *mail2 = NULL, *last_name = NULL;
  gchar *prof_phone =NULL, *pers_phone = NULL, *screen_name = NULL;

  /* current ressources' datas  */
  listBoxRessources = GTK_WIDGET(gtk_builder_get_object (data->builder, "listboxRsc"));
  lenRsc = g_list_length (data->rscList);
  rowIndex = lenRsc;
  hId = misc_get_highest_id (data);
  /* open file */
  FILE *fp = fopen (path_to_file, "r");
  if(fp != NULL)  {
    while(( read = getline (&line, &len, fp)) != -1)
    {
      colIndex = 0;
      type = 0;
      affi = 0;
      cost_type = 0;
      cost = 0;
      hour_day = 8;
      min_day = 0;
      /* default working days */
      color.red = 0;
      color.green = 0;
      color.blue = 0;
      newRsc.color = color;
      newRsc.pix = NULL;
      newRsc.fWorkMonday = TRUE;
      newRsc.fWorkTuesday = TRUE;
      newRsc.fWorkWednesday = TRUE;
      newRsc.fWorkThursday = TRUE;
      newRsc.fWorkFriday = TRUE;
      newRsc.fWorkSaturday = FALSE;
      newRsc.fWorkSunday = FALSE;
      /* calendar */
      newRsc.calendar = 0; /* default */
      newRsc.day_hours = 8;
      newRsc.day_mins = 0;
      /* we manage datas only starting with 2nd line */
      if(lineIndex>0) {
         for(token = zStrtok (line, ","); token != NULL && colIndex < MAXADRESSBOOKVARS; token = zStrtok (NULL, ",")) {
		   if(colIndex==0) {/* first name */
		      if(strlen(token)>0) {
		        /* token must be cleaned ! TODO */
		        first_name = g_strdup_printf("%s", token);
		      }
		      else
		        first_name = g_strdup_printf("?");

		   }/* colIndex = 0 */
		   if(colIndex==1) {/* last name */
		      if(strlen(token)>0) {
		        /* token must be cleaned ! TODO */
		        last_name = g_strdup_printf("%s", token);
		      }
		      else
		        last_name = g_strdup_printf("?");

		   }/* colIndex = 1 */
		   if(colIndex==2) {/* name */
		      if(strlen(token)>0) {
		        /* token must be cleaned ! TODO */
		        name = g_strdup_printf ("%s", token);
		      }
		      else
		        name = g_strdup_printf ("?");

		   }/* colIndex = 2 */
                   /* surname = col 3 */
		   if(colIndex==4) {/* mail1 */
		      if(strlen(token)>0) {
		        /* token must be cleaned ! TODO */
		        mail1 = g_strdup_printf("%s", token);
		      }
		      else
		        mail1 = g_strdup_printf("?");

		   }/* colIndex = 3 */
		   if(colIndex==5) {/* mail2 */
		      if(strlen(token)>0) {
		        /* token must be cleaned ! TODO */
		        mail2 = g_strdup_printf("%s", token);
		      }
		      else
		        mail2 = g_strdup_printf("?");

		   }/* colIndex = 4 */
		   if(colIndex==6) {/* screen name */
		      if(strlen(token)>0) {
		        /* token must be cleaned ! TODO */
		        screen_name = g_strdup_printf("%s", token);
		      }
		      else
		        screen_name = g_strdup_printf("?");

		   }/* colIndex = 5 */

		   if(colIndex==7) {/* professional phone */
		      if(strlen(token)>0) {
		        /* token must be cleaned ! TODO */
		        prof_phone = g_strdup_printf ("%s", token);
		      }
		      else
		        prof_phone = g_strdup_printf ("?");

		   }/* colIndex = 6 */
		   if(colIndex==8) {/* personal phone */
		      if(strlen(token)>0) {
		        /* token must be cleaned ! TODO */
		        pers_phone = g_strdup_printf ("%s", token);
		      }
		      else
		        pers_phone = g_strdup_printf ("?");

		   }/* colIndex = 7 */
		   /* now we check values */

		   colIndex++;
        }/* next token */
        /* TODO seulement si une valeur est lue ! */
        id = hId+1;
        hId++;
        /* we adjust string datas */
        if(name) {
		if(strcmp(name, "?")==0) {/* name is empty */
		   g_free (name);
		   name = g_strdup_printf ("%s %s", first_name, last_name);/* TODO normaliser uft8 */
		}
         }
         else {/* we check first and last name */
             if(last_name) {
               if(first_name) {
                  name = g_strdup_printf ("%s %s", first_name, last_name);
               }
               else {
                 name = g_strdup_printf ("%s", last_name);
               }
             }
             else {
               if(first_name) {
                  name = g_strdup_printf ("%s %d", first_name, rowIndex);
               }
               else {
                   name = g_strdup_printf ("%s %d", _("no name "), rowIndex);
               }
             }
        }/* endif names */

        if(mail1 || mail2) {
           if(mail1) {
              mail = g_strdup_printf ("%s", mail1);
           }
           else {
              if(mail2) {
                 mail = g_strdup_printf ("%s", mail2);
              }
              else {
                  mail = NULL;
              }
           }
        }/* endif mails */

        if(pers_phone || prof_phone) {
           if(pers_phone) {
              phone = g_strdup_printf ("%s", pers_phone);
           }
           else {
              if(prof_phone) {
                 phone = g_strdup_printf ("%s", prof_phone);
              }
              else {
                  phone = NULL;
              }
           }
        }/* endif phones */

 
        if(pers_phone && prof_phone) {
		if(strcmp(prof_phone, "?")==0) {/* professional phone is empty */
		   phone = g_strdup_printf ("%s", pers_phone);
		}
		else {
		   phone = g_strdup_printf ("%s", prof_phone);
		}
        }
        else {
           phone = NULL;
        }
        /* update widgets BEFORE storage */
        cadres = rsc_new_widget (name, type, affi, cost, cost_type, color, data, FALSE, NULL, &newRsc);
        /* update listbox */
        gtk_list_box_insert (GTK_LIST_BOX(listBoxRessources), GTK_WIDGET(cadres), rowIndex);
        /* store in memory */
        rsc_store_to_app_data (rowIndex, name, mail, phone, reminder, 
                              type, affi, cost, cost_type, color, NULL, data, id, &newRsc, TRUE);
        /* finalize - i.e. datas are transfered from app to memory */
        rsc_insert (rowIndex, &newRsc, data);
        /* undo engine */
        undo_datas *tmp_value;
        tmp_value = g_malloc (sizeof(undo_datas));
        tmp_value->opCode = OP_APPEND_RSC;
        tmp_value->insertion_row = rowIndex;
        tmp_value->id = newRsc.id;
        tmp_value->datas = NULL;/* nothing to store ? */
        tmp_value->list = NULL;
        tmp_value->groups = NULL;
        undo_push (CURRENT_STACK_RESSOURCES, tmp_value, data);
      }/* endif rowindex >0 */
      rowIndex++;
      lineIndex++;
      /* resets */
      if(name!=NULL) {
        g_free (name);
      }
      name=NULL;
      if(first_name!=NULL) {
        g_free (first_name);
      }
      first_name=NULL;
      if(last_name!=NULL) {
        g_free (last_name);
      }
      last_name=NULL;
      if(screen_name!=NULL) {
        g_free (screen_name);
      }
      screen_name=NULL;
      if(mail1!=NULL) {
        g_free (mail1);
      }
      mail1=NULL;
      if(mail2!=NULL) {
        g_free (mail2);
      }
      mail2=NULL;
      if(prof_phone!=NULL) {
        g_free (prof_phone);
      }
      prof_phone=NULL;
      if(pers_phone!=NULL) {
        g_free (pers_phone);
      }
      pers_phone=NULL;
      if(phone!=NULL) {
        g_free (phone);
      }
      phone=NULL;
      if(reminder!=NULL) {
        g_free (reminder);
      }
      reminder=NULL;
    }/* wend */
    if(line!=NULL)
       g_free (line);

    fclose (fp);
    gchar *msg = g_strdup_printf ( _("<b><big>%d</big></b> ressource(s)\nsucessfully appended."), lineIndex-1);
    misc_InfoDialog (data->appWindow, msg);
    g_free (msg);
    rr = 0;
  }/* endif FP */

  misc_display_app_status (TRUE, data);
  return rr;
}

