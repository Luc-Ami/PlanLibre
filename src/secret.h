/*************************************************
 * secret.h : tools for access to Gnome Keyring
 ************************************************/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#ifndef SECRET_H
#define SECRET_H

#include <stdlib.h>
#include <glib.h>
#include <gnome-keyring-1/gnome-keyring.h>
#include <gnome-keyring-1/gnome-keyring-memory.h>
#include "support.h"

/* functions */
void store_my_trello_password (gchar *password);
gchar *find_my_trello_password();

#endif /* SECRET_H */
