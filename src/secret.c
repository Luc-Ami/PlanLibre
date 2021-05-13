/*************************************************
 * secret.c : tools for access to Gnome Keyring
 ************************************************/
#include <stdio.h>
#include <glib.h>
#include <gnome-keyring-1/gnome-keyring.h>
#include <gnome-keyring-1/gnome-keyring-memory.h>
#include "secret.h"

/******************************************

  Gnome_keyring : define a schema to store
  Trello user token

******************************************/
static GnomeKeyringPasswordSchema trello_schema = {
    GNOME_KEYRING_ITEM_GENERIC_SECRET,
    {
        { "user", GNOME_KEYRING_ATTRIBUTE_TYPE_STRING },
        { NULL, 0 }
    }
};

/*****************************************
  store Trello password in synchronous
  mode
  argument : password to store
  an existing password whith same arguments
  will be updated
******************************************/
void store_my_trello_password (gchar *password)
{
   GnomeKeyringResult res;
   if(password) {
      // attention : paramètres fonctions avec sync différents des autres, pas de CB
      res = gnome_keyring_store_password_sync (&trello_schema,      /* The password type */
                                       GNOME_KEYRING_DEFAULT,  /* Where to save it */
                                       "Trello user token",  /* Password description, displayed to user, for example with Seahorse */
                                       password,  /* The password itself */
                                       "user", g_get_user_name (),
                                       NULL); /* Always end with NULL */

      printf ("* PlanLibre - password storing : %s *\n", gnome_keyring_result_to_message (res));
   }
}

/*****************************************
  retrieve Trello password in synchronous
  mode
  returns a string, which must be freed
******************************************/

gchar *find_my_trello_password()
{
  GnomeKeyringResult res;
  gchar *buffer, *password = NULL;

  res = gnome_keyring_find_password_sync (&trello_schema,      /* The password type */
                                           &buffer,             /* A gchar to store password  */
                                           "user", g_get_user_name (), /* Attributes according to schema */
                                           NULL);                 /* Always end with NULL */

  printf ("* PlanLibre - password request : %s *\n", gnome_keyring_result_to_message (res));

  if(res == GNOME_KEYRING_RESULT_OK) {
     password = g_strdup_printf ("%s", buffer);
     gnome_keyring_free_password (buffer);
  }
  return password; /* from Remina : https://gitlab.com/Remmina/Remmina/commit/57ec85d8e9bf773b8a08c17a0218b6cd643c828b */
}

