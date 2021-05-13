/*
 * File: undoredo.c header
 * Description: Contains config undo/redo operations
 *
 *
 *
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef UNDOREDO_H
#define UNDOREDO_H

#include <gtk/gtk.h>
#include <glib.h>
#include "support.h"
#include "misc.h"

/* Local macros */

#define MAX_UNDO_OPERATIONS 64

#define OP_NONE -1


/* types */
enum {OP_INSERT_TASK, OP_REMOVE_TASK, OP_APPEND_TASK, OP_MODIFY_TASK, 
      OP_TIMESHIFT_TASKS, OP_TIMESHIFT_GROUP, OP_REPEAT_TASK, OP_ADVANCE_TASK, OP_DELAY_TASK,
      OP_REDUCE_TASK, OP_INCREASE_TASK, OP_MOVE_TASK, OP_MOVE_GROUP_TASK, 
      OP_MODIFY_TASK_GROUP, OP_INSERT_GROUP, OP_MODIFY_GROUP, OP_REMOVE_GROUP,
      OP_INSERT_MILESTONE, OP_MODIFY_MILESTONE, OP_REMOVE_MILESTONE,
      
      LAST_TASKS_OP,
      OP_INSERT_RSC, OP_REMOVE_RSC, OP_APPEND_RSC, OP_MODIFY_RSC, OP_CLONE_RSC, OP_IMPORT_LIB_RSC,
      LAST_RSC_OP,
      OP_MODIFY_PROP, OP_MODIFY_CAL,
      LAST_MISC_OP,
      OP_MOVE_DASH,
      LAST_DASH_OP
};

/* functions */
void undo_push (gint current_stack, undo_datas *value, APP_data *data);
void undo_pop (APP_data *data);
void undo_free_all (APP_data *data);

#endif /* UNDOREDO_H */
