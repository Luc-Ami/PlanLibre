/****************************************

  Utilities to access Trello® oard


*****************************************/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* translations */
#include <libintl.h>
#include <locale.h>

#include <glib.h>
#include <glib-object.h>

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include <json-glib/json-glib.h>
#include <curl/curl.h>

#include "support.h"
#include "misc.h"
#include "secret.h"
#include "trello.h"

/* consts */



/* types */

/* from : https://curl.haxx.se/libcurl/c/getinmemory.html */

struct MemoryStruct {
  char *memory;
  size_t size;
};


/* vars */
static GtkWidget *progressBar;

/* functions */

/******************************
  PRIVATE : pulse progress bar

*******************************/
static void trello_pulse (void)
{
     gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR(progressBar), 0.2); 
     gtk_progress_bar_pulse (GTK_PROGRESS_BAR(progressBar)); 
     gtk_widget_queue_draw (GTK_WIDGET (progressBar));
     while (gtk_events_pending ()) {
	  gtk_main_iteration ();
     }
}


static size_t WriteMemoryCallback (void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  char *ptr = realloc (mem->memory, mem->size + realsize + 1);
  if(ptr == NULL) {
    /* out of memory! */ 
    printf ("PlanLibre critical : not enough memory for curl!*\n");
    return 0;
  }
 
  mem->memory = ptr;
  memcpy (&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}

/***********************************************
  PRIVATE
  Trello : create newboard
  arguments : API-key, user_token, board's name
  returns a valid ID string if OK, 
  otherwise returns NULL
  returned string must be freed by user

***********************************************/

static gchar *trello_create_board (gchar *api_key, gchar *user_token, gchar *name)
{
  gchar *tmpStr, *url, *id_found = NULL;
  const gchar *str_value;
  CURLcode ret;
  CURL *hnd;
  struct curl_slist *slist1;
  struct MemoryStruct chunk;
  /* Json */
  JsonParser *parser;
  JsonReader *reader;
  JsonObject *cur_object;
  JsonNode *root, *board_name, *board_id;
  GError *error;
  gint obj_len, i;

  tmpStr = g_uri_escape_string (name, NULL, FALSE);
  url = g_strdup_printf ("https://api.trello.com/1/boards/?key=%s&token=%s&name=%s", api_key, user_token, tmpStr);
  chunk.memory = malloc (1);  /* will be grown as needed by the realloc above */ 
  chunk.size = 0;    /* no data at this point */ 

  hnd = curl_easy_init ();

  curl_easy_setopt (hnd, CURLOPT_URL, url);
  curl_easy_setopt (hnd, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt (hnd, CURLOPT_USERAGENT, "curl/7.35.0");
  curl_easy_setopt (hnd, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt (hnd, CURLOPT_CUSTOMREQUEST, "POST");
  curl_easy_setopt (hnd, CURLOPT_TCP_KEEPALIVE, 1L);
  /* call back to fill string - send all data to this function  */ 
  curl_easy_setopt (hnd, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  /* we pass our 'chunk' struct to the callback function */ 
  curl_easy_setopt (hnd, CURLOPT_WRITEDATA, (void *)&chunk);

  ret = curl_easy_perform (hnd);
  if(!ret) {
     printf ("sortie du fichier depuis mémoire :\n");
     printf ("%s\n\n", chunk.memory);
     /* now we use Json reader to retrieve useful contents */
     parser = json_parser_new ();
     json_parser_load_from_data (parser, &chunk.memory[0], -1, NULL);
     root = json_parser_get_root (parser);/* root node of entire json file */
     reader = json_reader_new (root);
     /* json file is and array containing objects i.e. [{}, {}] - so we must found an array othecase is and error */
     if(json_reader_is_object (reader)) {
         obj_len = json_reader_count_members (reader);
         /* read ID at position 0 */
         json_reader_read_element (reader, 0);
         str_value = json_reader_get_string_value (reader);
         json_reader_end_element (reader);
         if(str_value) {
               id_found = g_strdup_printf ("%s", str_value);
         }
     }/* endif is object */
     else {
          printf ("*PlanLibre critical : Trello Json datas corrupted !*\n");
     }

     g_object_unref (reader);
     g_object_unref (parser);
  }

  curl_easy_cleanup (hnd);
  hnd = NULL;

  g_free (chunk.memory);
  g_free (url);
  g_free (tmpStr);
  return id_found;
}

/*********************************************

  PRIVATE : get a convenient name from project
  in order to use with Trello
  resulting char must be freed
*********************************************/
static gchar *trello_get_project_name (APP_data *data)
{
   gint rc = 0;
   gchar *tmpStr = NULL;

   if(data->properties.title) {
      tmpStr = g_strdup_printf ("%s", data->properties.title);
      if(strlen(tmpStr)<1) {
         g_free (tmpStr);
         tmpStr = NULL;
         rc = -1;
      }
   }
   if(rc!=0) {/* fallback : we display an error message */
      misc_ErrorDialog (data->appWindow, _("An error occured !\nIn order to export to Trello®, your project requires a title.\nPlease, see <b>project>properties</b> menu."));
   }
   return tmpStr;
}

/**********************************************
  PRIVATE
  Trello : add a label to a board
  arguments : api_key, token_id, label name,
              label color
  returns label ID  if OK (must be freed)
**********************************************/
static gchar *trello_add_label (gchar *api_key, gchar *user_token, gchar *board_id, gchar *name, gchar *color)
{
  gint rc = 0;
  gchar *tmpStr, *url, *tmpStr2, *tmpID = NULL;
  const gchar *str_value;
  struct MemoryStruct chunk;
  CURLcode ret;
  CURL *hnd;
  JsonParser *parser;
  JsonReader *reader;
  JsonObject *cur_object;
  JsonNode *root;
  gint obj_len;

  chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
  chunk.size = 0;    /* no data at this point */ 
  tmpStr = g_uri_escape_string (name, NULL, FALSE);
  tmpStr2 = g_uri_escape_string (color, NULL, FALSE);
  url = g_strdup_printf ("https://api.trello.com/1/labels?key=%s&token=%s&name=%s&color=%s&idBoard=%s",
                    api_key, user_token, tmpStr, tmpStr2, board_id);

  hnd = curl_easy_init ();


  curl_easy_setopt (hnd, CURLOPT_URL, url);
  curl_easy_setopt (hnd, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt (hnd, CURLOPT_USERAGENT, "curl/7.35.0");
  curl_easy_setopt (hnd, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt (hnd, CURLOPT_CUSTOMREQUEST, "POST");
  curl_easy_setopt (hnd, CURLOPT_TCP_KEEPALIVE, 1L);
  /* call back to fill string -  send all data to this function  */ 
  curl_easy_setopt (hnd, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  /* we pass our 'chunk' struct to the callback function */ 
  curl_easy_setopt (hnd, CURLOPT_WRITEDATA, (void *)&chunk);

  ret = curl_easy_perform(hnd);

  if(!ret) {
     printf ("ADD label OK sortie du fichier depuis mémoire :\n");
     printf ("%s\n\n", chunk.memory);
     /* now we use Json reader to retrieve useful contents */
     parser = json_parser_new ();
     json_parser_load_from_data (parser, &chunk.memory[0], -1, NULL);
     root = json_parser_get_root (parser);/* root node of entire json file */
     reader = json_reader_new (root);
     /* json file is and array containing objects i.e. [{}, {}] - so we must found an array othecase is and error */
     if(json_reader_is_object (reader)) {
         obj_len = json_reader_count_members (reader);
            /* read ID at position 0 */
            json_reader_read_element (reader, 0);
            str_value = json_reader_get_string_value (reader);
            json_reader_end_element (reader);
            if(str_value) {
                tmpID = g_strdup_printf ("%s", str_value);
               
            }
     }/* endif is object */
     else {
          printf ("*PlanLibre critical : Trello Json datas corrupted !*\n");
     }

     g_object_unref (reader);
     g_object_unref (parser);
  }


  curl_easy_cleanup(hnd);
  hnd = NULL;

  g_free (url);
  g_free (tmpStr);
  g_free (tmpStr2);
  g_free (chunk.memory);
  trello_pulse ();
  return tmpID;
}

/***********************************************
  PRIVATE : Trello : add new list
  arguments : API-key, user_token, board's ID,
  list name.
  the new list is added at bottom (last position)
  returns 0 if OK, 
***********************************************/

static gint trello_add_list (gchar *api_key, gchar *user_token, gchar *id_board, gchar *name)
{
  gchar *tmpStr = NULL, *url;
  struct MemoryStruct chunk;
  CURLcode ret;
  CURL *hnd;

  tmpStr = g_uri_escape_string (name, NULL, FALSE);
  url = g_strdup_printf ("https://api.trello.com/1/boards/%s/lists?key=%s&token=%s&name=%s&pos=bottom",
                    id_board, api_key, user_token, tmpStr);
  chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
  chunk.size = 0;    /* no data at this point */ 

  hnd = curl_easy_init ();


  curl_easy_setopt (hnd, CURLOPT_URL, url);
  curl_easy_setopt (hnd, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt (hnd, CURLOPT_USERAGENT, "curl/7.35.0");
  curl_easy_setopt (hnd, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt (hnd, CURLOPT_CUSTOMREQUEST, "POST");
  curl_easy_setopt (hnd, CURLOPT_TCP_KEEPALIVE, 1L);
  /* call back to fill string - send all data to this function  */ 
  curl_easy_setopt (hnd, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  /* we pass our 'chunk' struct to the callback function */ 
  curl_easy_setopt (hnd, CURLOPT_WRITEDATA, (void *)&chunk);

  ret = curl_easy_perform(hnd);
  if(!ret) {
     printf ("ajout liste fin sortie du fichier depuis mémoire :\n");
     printf ("%s\n\n", chunk.memory);

  }/* endif ret */
  else {
	printf ("*PlanLibre critical : can't add a list to a Trello board !*/");
  }
  curl_easy_cleanup(hnd);
  hnd = NULL;

  g_free (chunk.memory);
  g_free (url);
  g_free (tmpStr);
  return (int) ret;
}

/**********************************************
  PRIVATE
  Trello : change list's name
  arguments : api_key, token_id, list_id, name
  returns 0 if OK
**********************************************/
static gint trello_update_list_name (gchar *api_key, gchar *user_token, gchar *list_id, gchar *name)
{
  gint rc = 0;
  gchar *tmpStr, *url;
  struct MemoryStruct chunk;
  CURLcode ret;
  CURL *hnd;

  chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
  chunk.size = 0;    /* no data at this point */ 
  tmpStr = g_uri_escape_string (name, NULL, FALSE);
  url = g_strdup_printf ("https://api.trello.com/1/lists/%s?key=%s&token=%s&name=%s", list_id, api_key, user_token, tmpStr);

  hnd = curl_easy_init ();


  curl_easy_setopt (hnd, CURLOPT_URL, url);
  curl_easy_setopt (hnd, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt (hnd, CURLOPT_USERAGENT, "curl/7.35.0");
  curl_easy_setopt (hnd, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt (hnd, CURLOPT_CUSTOMREQUEST, "PUT");
  curl_easy_setopt (hnd, CURLOPT_TCP_KEEPALIVE, 1L);

  /* call back to fill string -  send all data to this function  */ 
  curl_easy_setopt (hnd, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  /* we pass our 'chunk' struct to the callback function */ 
  curl_easy_setopt (hnd, CURLOPT_WRITEDATA, (void *)&chunk);

  ret = curl_easy_perform(hnd);

  curl_easy_cleanup(hnd);
  hnd = NULL;

  g_free (chunk.memory);
  g_free (url);
  g_free (tmpStr);
  return (int)ret;
}

/********************************************
  PRIVATE : Trello : get list for a board
  arguments : API_key, user_token, board ID
  returns a GLIst of supped 4 (four) List ids
  (i.e. "columns" in PlanLibre)
  each element of this GList is a string and
  must be freed prior the GList itself
********************************************/
static GList  *trello_lists_request_for_board (gchar *api_key, gchar *user_token, gchar *board_id)
{
  GList *list = NULL;
  gchar *tmpUrl;
  CURLcode ret;
  CURL *hnd;
  struct curl_slist *slist1;
  struct MemoryStruct chunk;
  /* Json */
  JsonParser *parser;
  JsonReader *reader;
  JsonObject *cur_object;
  JsonNode *root, *list_name, *list_id, *list_pos;
  GError *error;
  gint array_len, i;

  tmpUrl = g_strdup_printf ("https://api.trello.com/1/boards/%s/lists?key=%s&token=%s", board_id, api_key, user_token);  
  slist1 = NULL;
  slist1 = curl_slist_append (slist1, "Accept: application/json");

  chunk.memory = g_malloc (1);  /* will be grown as needed by the realloc above */ 
  chunk.size = 0;    /* no data at this point */ 

  hnd = curl_easy_init ();


  curl_easy_setopt (hnd, CURLOPT_URL, tmpUrl);
  curl_easy_setopt (hnd, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt (hnd, CURLOPT_USERAGENT, "curl/7.35.0");
  curl_easy_setopt (hnd, CURLOPT_HTTPHEADER, slist1);
  curl_easy_setopt (hnd, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt (hnd, CURLOPT_CUSTOMREQUEST, "GET");
  curl_easy_setopt (hnd, CURLOPT_TCP_KEEPALIVE, 1L);
  /* call back to fill string -  send all data to this function  */ 
  curl_easy_setopt (hnd, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  /* we pass our 'chunk' struct to the callback function */ 
  curl_easy_setopt (hnd, CURLOPT_WRITEDATA, (void *)&chunk);

  ret = curl_easy_perform (hnd);
  if(!ret) {
     printf ("sortie du fichier depuis mémoire :\n");
     printf ("%s\n\n", chunk.memory);
     /* now we use Json reader to retrieve useful contents */
     parser = json_parser_new ();
     json_parser_load_from_data (parser, &chunk.memory[0], -1, NULL);
     root = json_parser_get_root (parser);/* root node of entire json file */
     reader = json_reader_new (root);
     /* json file is and array containing objects i.e. [{}, {}] - so we must found an array othecase is and error */
     if(json_reader_is_array (reader)) {
	  JsonArray *array =  json_node_get_array (root);
	  array_len = json_array_get_length (array);
          /* if array length <4 there is something wrong ! */
          for(i=0;i<array_len;i++) {
	          /* now we extract all objects inside array */
	          cur_object = json_array_get_object_element (array, i);
	          printf ("Nbre membres objet =%d\n", json_object_get_size (cur_object));
                  list_name = json_object_get_member (cur_object, "name");
                  list_id = json_object_get_member (cur_object, "id");
                  list_pos = json_object_get_member (cur_object, "pos");
                  printf ("valeurs lues dans objet nom =%s id=%s pos=%d\n", json_node_get_string (list_name),
                        json_node_get_string (list_id), (gint)json_node_get_double (list_pos)
                       );
                  /* now we check if the correct board has been found */
                  if(list_name && list_id) {
                     list = g_list_append (list, g_strdup_printf ("%s", json_node_get_string (list_id)));
                 
                  }/* endif */                  
          }/* next */
     }/* endif is array */
     else {
          printf ("*PlanLibre critical : Trello Json datas corrupted !*\n");
     }

     json_reader_end_member (reader);
     g_object_unref (reader);
     g_object_unref (parser);
  }
  curl_easy_cleanup (hnd);
  g_free (chunk.memory);
  hnd = NULL;
  curl_slist_free_all (slist1);
  slist1 = NULL;
  g_free (tmpUrl);
  return list;
}

/**********************************************
  PRIVATE
  Trello : initialize lists' names for a board
  to be compliant with PlanLibre conventions
  arguments : API_key, user_token, board ID
  
**********************************************/
static gint trello_lists_initialize_for_board (gchar *api_key, gchar *user_token, gchar *board_id)
{
  GList *l, *l2;
  gint i, rc =0, ret;
  gchar *tmp;

  l = trello_lists_request_for_board (api_key, user_token, board_id);
  if(l) {
      if(g_list_length (l)>=4) {
          /* we change names for 4 first lists */
          for(i=0;i<4;i++) {
             l2 = g_list_nth (l, i);
             tmp = (gchar*) l2->data;
             if(i==0) {
               ret = trello_update_list_name (api_key, user_token, tmp, _("To Do"));
             }
             if(i==1) {
               ret = trello_update_list_name (api_key, user_token, tmp, _("In Progress"));
             }
             if(i==2) {
               ret = trello_update_list_name (api_key, user_token, tmp, _("Overdue"));
             }
             if(i==3) {
               ret = trello_update_list_name (api_key, user_token, tmp, _("Completed"));
             }

          }/* next i */
          /* we free datas */
	  l2 = g_list_first (l);
	  for(l2;l2!=NULL;l2=l2->next) {
	      tmp = (gchar*) l2->data;
	      g_free (tmp);
	  }
	  g_list_free (l);
      }
      else {
         printf ("*PlanLibre critical : Trello board's lists datas corrupted !*\n");
         rc = -1;
      }
  }
  else {
    printf ("*PlanLibre critical : Trello board's lists datas corrupted !*\n");
    rc = -1;/* error with lists */
  }
  return rc;
}

/**************************************************
  PRIVATE
  Trello : set label for acard
  Please notice : when PlanLibre says "task"
  Trello means "card".
  Arguments : api_key, user_token, id_board, 
               rank of list where we insert card,
               title of card, valid ID for a label
  
**************************************************/
static gint trello_set_card_label (gchar *api_key, gchar *user_token, gchar *card_id, gint priority)
{
  gint rc = 0;
  gchar *url, *tmpColor, *tmpPriority, *tmpStr, *tmpStr2;
  struct MemoryStruct chunk;
  CURLcode ret;
  CURL *hnd;

  chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
  chunk.size = 0;    /* no data at this point */ 
  if(priority<0)
     priority = 2;
  if(priority>3)
     priority = 3;

  if(priority==0) {
     tmpColor = g_strdup_printf ("%s", "red");
     tmpPriority = g_strdup_printf ("%s", _("Priority high"));
  }

  if(priority==1) {
     tmpColor = g_strdup_printf ("%s", "orange");
     tmpPriority = g_strdup_printf ("%s", _("Priority medium"));
  }

  if(priority==2) {
     tmpColor = g_strdup_printf ("%s", "green");
     tmpPriority = g_strdup_printf ("%s", _("Priority low"));
  }

  if(priority==3) {/* for milestones */
     tmpColor = g_strdup_printf ("%s", "blue");
     tmpPriority = g_strdup_printf ("%s", _("Milestone"));
  }

  tmpStr = g_uri_escape_string (tmpColor, NULL, FALSE);
  tmpStr2 = g_uri_escape_string (tmpPriority, NULL, FALSE);

  url = g_strdup_printf ("https://api.trello.com/1/cards/%s/labels?key=%s&token=%s&color=%s&name=%s",
                    card_id, api_key, user_token, tmpStr, tmpStr2);

  hnd = curl_easy_init ();


  curl_easy_setopt (hnd, CURLOPT_URL, url);
  curl_easy_setopt (hnd, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt (hnd, CURLOPT_USERAGENT, "curl/7.35.0");
  curl_easy_setopt (hnd, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt (hnd, CURLOPT_CUSTOMREQUEST, "POST");
  curl_easy_setopt (hnd, CURLOPT_TCP_KEEPALIVE, 1L);
  /* call back to fill string -  send all data to this function  */ 
  curl_easy_setopt (hnd, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  /* we pass our 'chunk' struct to the callback function */ 
  curl_easy_setopt (hnd, CURLOPT_WRITEDATA, (void *)&chunk);

  ret = curl_easy_perform(hnd);
  if(!ret) {
     printf ("Set card label OK sortie du fichier depuis mémoire :\n");
     printf ("%s\n\n", chunk.memory);
  }
  curl_easy_cleanup(hnd);
  hnd = NULL;

  g_free (tmpPriority);
  g_free (tmpColor);
  g_free (tmpStr2);
  g_free (tmpStr);
  g_free (url);
  g_free (chunk.memory);
  return (int)ret;  

}

/**************************************************
  PRIVATE
  Trello : add a new card
  Please notice : when PlanLibre says "task"
  Trello means "card".
  Arguments : api_key, user_token, id_board, 
               rank of list where we insert card,
               title of card, valid ID for a label
  
**************************************************/
static gchar *trello_add_single_card (gchar *api_key, gchar *user_token, gchar *list_id, gchar *card_title, gchar *date_time, gint priority)
{
  gchar *url, *tmpStr, *tmp, *id_found = NULL;
  const gchar *str_value;
  struct MemoryStruct chunk;
  CURLcode ret, rc;
  CURL *hnd;
  /* Json */
  JsonParser *parser;
  JsonReader *reader;
  JsonObject *cur_object;
  JsonNode *root, *board_name, *board_id;
  gint obj_len;

  chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
  chunk.size = 0;    /* no data at this point */ 
  tmpStr = g_uri_escape_string (card_title, NULL, FALSE);
  url = g_strdup_printf ("https://api.trello.com/1/cards?key=%s&token=%s&idList=%s&name=%s&due=%s", 
                          api_key, user_token, list_id, tmpStr, date_time);
  hnd = curl_easy_init ();


  curl_easy_setopt (hnd, CURLOPT_URL, url);
  curl_easy_setopt (hnd, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt (hnd, CURLOPT_USERAGENT, "curl/7.35.0");
  curl_easy_setopt (hnd, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt (hnd, CURLOPT_CUSTOMREQUEST, "POST");
  curl_easy_setopt (hnd, CURLOPT_TCP_KEEPALIVE, 1L);

  /* call back to fill string -  send all data to this function  */ 
  curl_easy_setopt (hnd, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  /* we pass our 'chunk' struct to the callback function */ 
  curl_easy_setopt (hnd, CURLOPT_WRITEDATA, (void *)&chunk);

  ret = curl_easy_perform(hnd);

  if(!ret) {
     printf ("ADD card OK sortie du fichier depuis mémoire :\n");
     printf ("%s\n\n", chunk.memory);
     /* now we use Json reader to retrieve useful contents */
     parser = json_parser_new ();
     json_parser_load_from_data (parser, &chunk.memory[0], -1, NULL);
     root = json_parser_get_root (parser);/* root node of entire json file */
     reader = json_reader_new (root);
     /* json file is and array containing objects i.e. [{}, {}] - so we must found an array othecase is and error */
     if(json_reader_is_object (reader)) {
         obj_len = json_reader_count_members (reader);
            /* read ID at position 0 */
            json_reader_read_element (reader, 0);
            str_value = json_reader_get_string_value (reader);
            json_reader_end_element (reader);
            if(str_value) {
                id_found = g_strdup_printf ("%s", str_value);
                printf ("Add card pour ID =%s \n", id_found);
                  rc = trello_set_card_label (api_key, user_token, id_found, priority);
               
            }
     }/* endif is object */
     else {
          printf ("*PlanLibre critical : Trello Json datas corrupted !*\n");
     }

     g_object_unref (reader);
     g_object_unref (parser);
  }


  curl_easy_cleanup(hnd);
  hnd = NULL;
  g_free (chunk.memory);
  g_free (url);
  g_free (tmpStr);

  return id_found;
}

/*******************************************************
  PRIVATE
  Trello : add a new card
  Please notice : when PlanLibre says "task"
  Trello means "card".
  Arguments : api_key, user_token, id_board, 
               rank of list where we insert card,
               title of card
               date in Glib format, 
               priority (2 = low, 1 = medium, 0 = high)
********************************************************/
gint trello_add_card (gchar *api_key, gchar *user_token, gchar *board_id, gint rank, gchar *card_title, GDate *date, gint priority)
{
  gint i, rc = 0, ret;
  gchar *url, *tmpStr, *tmp, *tmpDate, *tmpId;
  GList *l, *l2;

  if((rank<0) || (rank>3))
      return -1;

  l = trello_lists_request_for_board (api_key, user_token, board_id);
  if(!l)
    return -1;

  tmpDate = g_strdup_printf ("%.2d-%.2d-%.2dT20:00:00.000z", g_date_get_year(date), g_date_get_month(date), g_date_get_day(date));
  if(l) {
      if(g_list_length (l)>=4) {
          /* we change names for 4 first lists */
          for(i=0;i<4;i++) {
             l2 = g_list_nth (l, i);
             tmp = (gchar*) l2->data;
             if((i==0) && (rank==0)) {
               tmpId = trello_add_single_card (api_key, user_token, tmp, card_title, tmpDate, priority);
             }
             if((i==1) && (rank==1)) {
               tmpId = trello_add_single_card (api_key, user_token, tmp, card_title, tmpDate, priority);
             }
             if((i==2) && (rank==2)) {
               tmpId = trello_add_single_card (api_key, user_token, tmp, card_title, tmpDate, priority);
             }
             if((i==3) && (rank==3)) {
               tmpId = trello_add_single_card (api_key, user_token, tmp, card_title, tmpDate, priority);
             }

          }/* next i */
          /* if tmpId not NULL? you can set-up labels */
          if(tmpId) {

             g_free (tmpId);
          }
          /* we free datas */
	  l2 = g_list_first (l);
	  for(l2;l2!=NULL;l2=l2->next) {
	      tmp = (gchar*) l2->data;
	      g_free (tmp);
	  }
	  g_list_free (l);
      }
      else {
         printf ("*PlanLibre critical : Trello board's lists datas corrupted !*\n");
         rc = -1;
      }
  }
  else {
    printf ("*PlanLibre critical : Trello board's lists datas corrupted !*\n");
    rc = -1;/* error with lists */
  }

  g_free (tmpDate);
  return rc;
}

/*********************************************
  PRIVATE : export all tasks as "cards"
  arguments : app_data structure
     others vars are picked from secrets.h
  returns 0 if OK 
********************************************/
static gint trello_export_tasks (gchar *api_key, gchar *password, gchar *board_id, APP_data *data)
{
  gint i, rank, rc = 0, rc2 = 0;
  GDate date_start, date_end;
  GList *l;
  tasks_data *tmp_tasks_datas;


  for(i=0;i<g_list_length (data->tasksList);i++) {
     l = g_list_nth (data->tasksList, i);
     tmp_tasks_datas = (tasks_data *)l->data;
     /* now, we switch according to 'status' field */
     g_date_set_dmy (&date_start, 
                  tmp_tasks_datas->start_nthDay, tmp_tasks_datas->start_nthMonth, tmp_tasks_datas->start_nthYear);
     g_date_set_dmy (&date_end, 
                  tmp_tasks_datas->end_nthDay, tmp_tasks_datas->end_nthMonth, tmp_tasks_datas->end_nthYear);
     if(tmp_tasks_datas->type!=TASKS_TYPE_GROUP){
	     switch(tmp_tasks_datas->status) {
		case TASKS_STATUS_RUN:{
		   rank = 1;
		   break;
		}
		case TASKS_STATUS_DONE:{
		   rank = 3;
		   break;
		}
		case TASKS_STATUS_DELAY:{
		   rank = 2;
		   break;
		}
		case TASKS_STATUS_TO_GO:{
		   rank = 0;
		   break;
		}
	     }/* end switch */
             if(tmp_tasks_datas->type!=TASKS_TYPE_MILESTONE){
	        rc2 = trello_add_card (API_KEY, password, board_id, rank, 
                             tmp_tasks_datas->name, &date_end, tmp_tasks_datas->priority);
             }
             else {
	        rc2 = trello_add_card (API_KEY, password, board_id, rank, 
                             tmp_tasks_datas->name, &date_end, 3);/* for mielstone, priority == 3*/
             }
	     if(rc2!=0) {
		rc = -1;
	     }
    }/* endif true task or milestone */
    trello_pulse ();
  }/* next i */

  return rc;
}

/*********************************************
  PUBLIC : export current project
  arguments : app_data structure
     others vars are picked from secrets.h
  returns 0 if OK 
********************************************/
gint trello_export_project (APP_data *data)
{
  gint rc = 0;
  gchar *password, *name, *IdBoard = NULL;
  gchar *lbl_low, *lbl_med, *lbl_high, *lbl_milestone;
  GtkWidget *statusProgress;

  /* progress bar */
  statusProgress = GTK_WIDGET(gtk_builder_get_object (data->builder, "boxProgressMail"));
  gtk_widget_show (GTK_WIDGET(statusProgress));
  progressBar = GTK_WIDGET(gtk_builder_get_object (data->builder, "progressbarSending"));
  gtk_widget_show (GTK_WIDGET(progressBar));
  /* mandatory to get some advance time */
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progressBar), _("Sending to Trello"));
  gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR(progressBar), 0.1); // Sets the fraction of total progress bar length to move
  gtk_progress_bar_pulse (GTK_PROGRESS_BAR(progressBar));
  trello_pulse (); 
//while (gtk_events_pending ())
  //gtk_main_iteration ();
  /* we must test if password exists */
  password = find_my_trello_password ();
  trello_pulse ();
  if(password) {
      trello_pulse ();
     /* OK, we continue only if password has the required length - now, we check PlanLibre project's name - if not defined, we use a fallback */
     if(strlen(password)==64) {
        name = trello_get_project_name (data);
        trello_pulse ();
        if(name) {
            trello_pulse ();
	    IdBoard = trello_create_board (API_KEY, password, name);
	    if(IdBoard) {
               /* now we have an access, and we can really export datas */
               /* we change default labels */
      	       lbl_high = trello_add_label (API_KEY, password, IdBoard, _("Priority high"), "red");
               lbl_med = trello_add_label (API_KEY, password, IdBoard, _("Priority medium"), "orange");
               lbl_low = trello_add_label (API_KEY, password, IdBoard, _("Priority low"), "green");
               lbl_milestone = trello_add_label (API_KEY, password, IdBoard, _("Milestone"), "blue");
               trello_pulse ();
               /* we add the 4th column, like in PlanLibre */
               rc = trello_add_list (API_KEY, password, IdBoard, _("Column"));
               /* we add lists and rename them according to PLanLibre conventions */
               rc = trello_lists_initialize_for_board (API_KEY, password, IdBoard);
               /* now we add all tasks as cards */
               rc = trello_export_tasks (API_KEY, password, IdBoard, data);
               /* free memory */
	       g_free (lbl_med);
	       g_free (lbl_low);
	       g_free (lbl_high);
               g_free (lbl_milestone);
               g_free (IdBoard);
	    }
	    else {
		rc = -1;
	    }
          g_free (name);
        }
        else {
          rc = -1;
        }/* elseif name */
     }/* endif password length */
     else {
        rc = -1;
     }
     g_free (password);
  }
  else {
    rc = -1;
  }

  gtk_widget_hide (GTK_WIDGET(statusProgress));
  return rc;
}
