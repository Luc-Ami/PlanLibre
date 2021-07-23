/************************************************************
  functions to manage ms project xml files



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
#include "misc.h"

#include "files.h"

#include "calendars.h" 
#include "msproject.h" 


/*********************************
  Local function
  save assignments part to project
  * xml file
*********************************/
static gint save_assign_to_project (xmlNodePtr assign_node, APP_data *data)
{
  gint i = 0, j, ret = 0, rscId, link, highId = 1;
  gint  hour1, min1, hour2, min2;
  gchar buffer[100];/* for gdouble conversions */
  GDate current_date;
  GDateTime *my_time;  
  GList *l, *l2;
  GError *error = NULL;
  rsc_datas *tmp_rsc_datas;
  tasks_data *tmp_tasks_datas;
  xmlNodePtr assign_object = NULL, task_UID_node = NULL, rsc_UID_node = NULL, assign_UID_node = NULL, assign_over_node = NULL;
  xmlNodePtr assign_start_node = NULL, assign_finish_node = NULL, assign_units_node = NULL, assign_work_node = NULL;
  xmlNodePtr time_phase_node = NULL, type_node = NULL, UID_phase_node = NULL, phase_start_node = NULL, phase_finish_node = NULL;
  xmlNodePtr phase_value_node = NULL, phase_unit_node = NULL;
  /* under PlanLibre, assignments are managed as links  */
  /* and for MS Project, links/assignments requires UID */

  // highId = misc_get_highest_id (data);/* useless assign UIDS are traten separatly by ms project */
  for(i = 0; i<g_list_length (data->rscList); i++) {
     l = g_list_nth (data->rscList, i);
     tmp_rsc_datas = (rsc_datas *)l->data;
     rscId = tmp_rsc_datas->id;
     /* RscId will become resourceUID under ms project */
     /* now we do loop inside tasks list, and, in case of current resource is used, we build an element */
     for(j = 0; j < g_list_length (data->tasksList); j++) {
        l2 = g_list_nth (data->tasksList, j);
        tmp_tasks_datas = (tasks_data *)l2->data;		 
        /* now, we check if ressource 'id' is used by current task */
        link = links_check_element (tmp_tasks_datas->id, rscId, data);
        /* if link >= 0 thus we can set up a new Assignment element */
        if(link >= 0) {
			/* we have to set an unique UID for the assignment itself */
			highId++;			
			assign_object = xmlNewChild (assign_node, NULL, BAD_CAST "Assignment", NULL);
		    assign_UID_node = xmlNewChild (assign_object, NULL, BAD_CAST "UID", NULL);
		    xmlNodeSetContent (assign_UID_node, BAD_CAST g_strdup_printf ("%d", highId));
		    task_UID_node = xmlNewChild (assign_object, NULL, BAD_CAST "TaskUID", NULL); 
		    xmlNodeSetContent (task_UID_node, BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->id)); 
			rsc_UID_node = xmlNewChild (assign_object, NULL, BAD_CAST "ResourceUID", NULL);
		    xmlNodeSetContent (rsc_UID_node, BAD_CAST g_strdup_printf ("%d", rscId)); 			
			assign_over_node = xmlNewChild (assign_object, NULL, BAD_CAST "OverAllocated", NULL);
		    xmlNodeSetContent (assign_over_node, BAD_CAST "0");
		    /* start - TODO correct starting hour for current resource */
			assign_start_node = xmlNewChild (assign_object, NULL, BAD_CAST "Start", NULL);
		    hour1 = data->properties.scheduleStart/60;
		    min1 =  data->properties.scheduleStart - 60*hour1;		    
		    
		    xmlNodeSetContent (assign_start_node, BAD_CAST  g_strdup_printf ("%02d-%02d-%02dT%02d:%02d:00",
								  tmp_tasks_datas->start_nthYear,
								  tmp_tasks_datas->start_nthMonth,
								  tmp_tasks_datas->start_nthDay,
								  hour1, min1)
		 
						   ); 
		    /* end - TODO correct starting hour for current resource */
		    hour2 = data->properties.scheduleEnd/60;
		    min2 =  data->properties.scheduleEnd - 60*hour2;
		 		    
		    assign_finish_node = xmlNewChild (assign_object, NULL, BAD_CAST "Finish", NULL);
		    xmlNodeSetContent (assign_finish_node, BAD_CAST  g_strdup_printf ("%02d-%02d-%02dT%02d:%02d:00",
								  tmp_tasks_datas->end_nthYear,
								  tmp_tasks_datas->end_nthMonth,
								  tmp_tasks_datas->end_nthDay,
								  hour2, min2)
		 
						   ); 
			/* units */		    
		    assign_units_node = xmlNewChild (assign_object, NULL, BAD_CAST "Units", NULL);	
		    xmlNodeSetContent (assign_units_node, BAD_CAST "1");/* hours */
		    /* work */
		    assign_work_node = xmlNewChild (assign_object, NULL, BAD_CAST "Work", NULL);
		    min1 = tmp_tasks_datas->days*(data->properties.scheduleEnd-data->properties.scheduleStart); /* here we have a time in minutes */
		    min1 = min1 + 60*tmp_tasks_datas->minutes;
		    hour2 = min1/60;
		    min2 = min1-60*hour2;
		    if(tmp_tasks_datas->type == TASKS_TYPE_MILESTONE) {
			   hour2 = 0;
			   min2 = 0; 
	        }
		    xmlNodeSetContent (assign_work_node, BAD_CAST  g_strdup_printf ("PT%dH%dM0S", hour2, min2));			    			    					   	    
		    /* and now we must compute periods ! */
		    /*
		                 <TimephasedData>
                <Type>1</Type>
                <UID>2</UID>
                <Start>2021-07-23T10:00:00</Start>
                <Finish>2021-07-24T10:00:00</Finish>
                <Unit>3</Unit>
                <Value>PT8H0M0S</Value>
            </TimephasedData>
		  */
		    /* for now, we use only ONE time phase */
		    time_phase_node = xmlNewChild (assign_object, NULL, BAD_CAST "TimephasedData", NULL);
		    type_node = xmlNewChild (time_phase_node, NULL, BAD_CAST "Type", NULL);
		    xmlNodeSetContent (type_node, BAD_CAST "1");
		    UID_phase_node = xmlNewChild (time_phase_node, NULL, BAD_CAST "UID", NULL);
		    xmlNodeSetContent (UID_phase_node, BAD_CAST g_strdup_printf ("%d", highId));/* strictly same as assignment UID */		   
		    
		    /* start - TODO correct starting hour for current resource */
		    hour1 = data->properties.scheduleStart/60;
		    min1 =  data->properties.scheduleStart - 60*hour1;		    
		    
		    phase_start_node = xmlNewChild (time_phase_node, NULL, BAD_CAST "Start", NULL);
		    xmlNodeSetContent (phase_start_node, BAD_CAST  g_strdup_printf ("%02d-%02d-%02dT%02d:%02d:00",
								  tmp_tasks_datas->start_nthYear,
								  tmp_tasks_datas->start_nthMonth,
								  tmp_tasks_datas->start_nthDay,
								  hour1, min1)
		 
						   ); 

		    /* finish - don't used for now */ 
		    hour2 = data->properties.scheduleEnd/60;
		    min2 =  data->properties.scheduleEnd - 60*hour2;
		 		    
		    phase_finish_node = xmlNewChild (time_phase_node, NULL, BAD_CAST "Finish", NULL);
		    xmlNodeSetContent (phase_finish_node, BAD_CAST  g_strdup_printf ("%02d-%02d-%02dT%02d:%02d:00",
								  tmp_tasks_datas->end_nthYear,
								  tmp_tasks_datas->end_nthMonth,
								  tmp_tasks_datas->end_nthDay,
								  hour1, min1)
		 
						   ); 
		    
		    phase_unit_node = xmlNewChild (time_phase_node, NULL, BAD_CAST "Unit", NULL);
		    xmlNodeSetContent (phase_unit_node, BAD_CAST "1");/* hours */
		    /* duration */		    		    
		    phase_value_node = xmlNewChild (time_phase_node, NULL, BAD_CAST "Value", NULL);
		    min1 = tmp_tasks_datas->days*(data->properties.scheduleEnd-data->properties.scheduleStart); /* here we have a time in minutes */
		    min1 = min1 + 60*tmp_tasks_datas->minutes;
		    hour2 = min1/60;
		    min2 = min1-60*hour2;
		    if(tmp_tasks_datas->type == TASKS_TYPE_MILESTONE) {
			   hour2 = 0;
			   min2 = 0; 
	        }
		    xmlNodeSetContent (phase_value_node, BAD_CAST  g_strdup_printf ("PT%dH%dM0S", hour2, min2));		    

	    }/* endif link */
	 }/* next j */
    
  }/* next i */  
  
  return ret;	
}	

/*********************************
  Local function
  save resources part to project
  * xml file
*********************************/
static gint save_resources_to_project (xmlNodePtr rsc_node, APP_data *data)
{
  gint i = 0, ret = 0;
  gchar buffer[100];/* for gdouble conversions */
  GDate current_date;
  GDateTime *my_time;  
  GList *l;
  GError *error = NULL;
  xmlNodePtr rsc_object = NULL, rsc_object_sub = NULL, name_node = NULL, UID_node = NULL, id_node = NULL;
  xmlNodePtr isnull_node = NULL, type_node = NULL, initials_node = NULL, email_node = NULL, notes_node = NULL;
  xmlNodePtr overalloc_node = NULL, creation_node = NULL, std_rate_node = NULL, std_rate_fmt_node = NULL, overtime_rate_node = NULL;
  xmlNodePtr max_units_node = NULL, calendar_UID_node = NULL;
  
  rsc_datas *tmp_rsc_datas;

  g_date_set_time_t (&current_date, time (NULL)); /* date of today */
  /* get current time, should be unrefed once used */
  my_time = g_date_time_new_now_local ();  
  /* main loop */

  for(i = 0; i < g_list_length (data->rscList); i++) {
     l = g_list_nth (data->rscList, i);
     tmp_rsc_datas = (rsc_datas *)l->data;
     rsc_object = xmlNewChild (rsc_node, NULL, BAD_CAST "Resource", NULL);
     /* UID and Id for current resource */
     UID_node = xmlNewChild (rsc_object, NULL, BAD_CAST "UID", NULL);
     id_node = xmlNewChild (rsc_object, NULL, BAD_CAST "ID", NULL);
		 
     xmlNodeSetContent (UID_node, BAD_CAST g_strdup_printf ("%d", tmp_rsc_datas->id));
	 xmlNodeSetContent (id_node, BAD_CAST g_strdup_printf ("%d", tmp_rsc_datas->id));      
     
     /* name */
     name_node = xmlNewChild (rsc_object, NULL, BAD_CAST "Name", NULL);
     /* initials */
     initials_node = xmlNewChild (rsc_object, NULL, BAD_CAST "Initials", NULL);
     if(tmp_rsc_datas->name) {
        xmlNodeSetContent (name_node,  BAD_CAST g_strdup_printf ("%s",tmp_rsc_datas->name));
        xmlNodeSetContent (initials_node,  BAD_CAST g_strdup_printf ("%.2s",tmp_rsc_datas->name));
     }
     else {
        xmlNodeSetContent (name_node,  BAD_CAST _("Noname"));
        xmlNodeSetContent (initials_node,  BAD_CAST _("??"));
     }
     
     /* flags */
     isnull_node = xmlNewChild (rsc_object, NULL, BAD_CAST "IsNull", NULL);
     xmlNodeSetContent (isnull_node, BAD_CAST "0");
     /* type - 0 material, 1 work force 2 cost - PlanLibre 0 human, 1 animal >=2 materials */
     type_node = xmlNewChild (rsc_object, NULL, BAD_CAST "Type", NULL);
     if(tmp_rsc_datas->type<2) {
         xmlNodeSetContent (type_node, BAD_CAST "1");
     }
     else {
         xmlNodeSetContent (type_node, BAD_CAST "0");		 
	 }
     /* email */
     email_node = xmlNewChild (rsc_object, NULL, BAD_CAST "EmailAddress", NULL);
     if(tmp_rsc_datas->mail) {
        xmlNodeSetContent (email_node,  BAD_CAST g_strdup_printf ("%s",tmp_rsc_datas->mail));
     }
     else {
        xmlNodeSetContent (email_node,  BAD_CAST "");
     }

     /* max units */
     max_units_node = xmlNewChild (rsc_object, NULL, BAD_CAST "MaxUnits", NULL);     
     g_ascii_dtostr (buffer, 50, tmp_rsc_datas->quantity);
     xmlNodeSetContent (max_units_node, BAD_CAST buffer);
  
     /* notes - reminder */
     notes_node = xmlNewChild (rsc_object, NULL, BAD_CAST "Notes", NULL);     
     if(tmp_rsc_datas->reminder) {
        xmlNodeSetContent (notes_node, BAD_CAST g_strdup_printf ("%s",tmp_rsc_datas->reminder));
     }
     else {
        xmlNodeSetContent (notes_node, BAD_CAST "");
     }
     
     /* OverAllocated == concurrent usage for PlanLibre */
     overalloc_node = xmlNewChild (rsc_object, NULL, BAD_CAST "OverAllocated", NULL);     
     if(tmp_rsc_datas->fAllowConcurrent) {    
		xmlNodeSetContent (overalloc_node, BAD_CAST "1"); 
	 }
	 else {
		xmlNodeSetContent (overalloc_node, BAD_CAST "0"); 
     }
     
     /* creation date, not critical */
	 creation_node = xmlNewChild (rsc_object, NULL, BAD_CAST "CreationDate", NULL);
	 xmlNodeSetContent (creation_node, BAD_CAST 
						   g_strdup_printf ("%d-%02d-%02dT%02d:%02d:00", g_date_get_year (&current_date), g_date_get_month (&current_date), 
											g_date_get_day (&current_date),
											g_date_time_get_hour (my_time),
											g_date_time_get_minute (my_time)
						   ));
     
     
     /* integer & float */
     /*  affiliate */
  //   xmlNewProp (rsc_object, BAD_CAST "Affiliate", BAD_CAST g_strdup_printf ("%d",tmp_rsc_datas->affiliate));


     /*  cost value : standard rate for Ms project */
     std_rate_node = xmlNewChild (rsc_object, NULL, BAD_CAST "StandardRate", NULL);        
     g_ascii_dtostr (buffer, 50, tmp_rsc_datas->cost);
     xmlNodeSetContent (std_rate_node, BAD_CAST buffer);
     /*  cost_type 
		#define RSC_COST_HOUR 0
		#define RSC_COST_DAY 1
		#define RSC_COST_WEEK 2
		#define RSC_COST_MONTH 3
		#define RSC_COST_FIXED 4 */     
     /* rate format : 2 for hours, 8 for material resources */
     std_rate_fmt_node = xmlNewChild (rsc_object, NULL, BAD_CAST "StandardRateFormat", NULL);
     switch(tmp_rsc_datas->cost_type) { 
		case 0: {        
           xmlNodeSetContent (std_rate_fmt_node, BAD_CAST "2");     
           break;
        }
		case 1: {        
           xmlNodeSetContent (std_rate_fmt_node, BAD_CAST "3");     
           break;
        }
        case 2: {        
           xmlNodeSetContent (std_rate_fmt_node, BAD_CAST "4");     
           break;
        }
		case 3: {        
           xmlNodeSetContent (std_rate_fmt_node, BAD_CAST "5");     
           break;
        }
		case 4: {        
           xmlNodeSetContent (std_rate_fmt_node, BAD_CAST "8");     
           break;
        }                        
		default: {        
           xmlNodeSetContent (std_rate_fmt_node, BAD_CAST "2");     
        }        
     }/* end switch */
     /* overtime rate - for convenience 20% more for all resources */
     overtime_rate_node = xmlNewChild (rsc_object, NULL, BAD_CAST "OvertimeRate", NULL);
     g_ascii_dtostr (buffer, 50, tmp_rsc_datas->cost *1.2);
     xmlNodeSetContent (overtime_rate_node, BAD_CAST buffer);     

     /* calendar, fow now only standard, default */    
  //   calendar_UID_node = xmlNewChild (rsc_object, NULL, BAD_CAST "CalendarUID", NULL);
  //   xmlNodeSetContent (calendar_UID_node, BAD_CAST "1");  
 	 
 

  //   xmlNewProp (rsc_object, BAD_CAST "calendar", BAD_CAST g_strdup_printf ("%d", tmp_rsc_datas->calendar));
 //    xmlNewProp (rsc_object, BAD_CAST "hours", BAD_CAST g_strdup_printf ("%d", tmp_rsc_datas->day_hours));
 //    xmlNewProp (rsc_object, BAD_CAST "minutes", BAD_CAST g_strdup_printf ("%d", tmp_rsc_datas->day_mins));

 
  }/* next i */
  return ret;
}


/*******************************
   LOCAL : check if a task
    use any task, emit xml 
    * datas if OK 
*******************************/
static gint project_check_task_linked (xmlNodePtr tasks_object, gint tsk, APP_data *data)
{
  gint ret = -1, i = 0, sender;
  GList *l;
  gint links_list_len = g_list_length (data->linksList);
  link_datas *tmp_links_datas;
  xmlNodePtr pred_node = NULL, pred_UID_node = NULL, type_node = NULL, cross_project_node = NULL;

  /* we test if task is sender anywhere  */
  while( i<links_list_len ) {
     l = g_list_nth (data->linksList, i);
     tmp_links_datas = (link_datas *)l->data;
     if((tmp_links_datas->receiver == tsk) && (tmp_links_datas->type == LINK_TYPE_TSK_TSK)) {
        ret = i;
        sender = tmp_links_datas->sender;
        pred_node = xmlNewChild (tasks_object, NULL, BAD_CAST "PredecessorLink", NULL); 
        pred_UID_node = xmlNewChild (pred_node, NULL, BAD_CAST "PredecessorUID", NULL); 
        xmlNodeSetContent (pred_UID_node, BAD_CAST g_strdup_printf ("%d", sender));
        type_node = xmlNewChild (pred_node, NULL, BAD_CAST "Type", NULL); 
        xmlNodeSetContent (type_node, BAD_CAST "1");/* link finish to start */
        cross_project_node = xmlNewChild (pred_node, NULL, BAD_CAST "CrossProject", NULL);
        xmlNodeSetContent (cross_project_node, BAD_CAST "0");/* flag, not member of another project */
        
                
     }
     i++;
  }/* wend */ 

  return ret;
}


/*********************************
  Local function
  save tasks part to project
  * xml file
*********************************/
static gint save_tasks_to_project (xmlNodePtr tasks_node, APP_data *data)
{
  gint i = 0, j, ret = 0, wbs_count = 0, priority, hour1, min1, hour2, min2, found;
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
         
     /* if the task is a group, we don't manage it for now */
     if(tmp_tasks_datas->type != TASKS_TYPE_GROUP) {
		 /* task start of declaration */
		 tasks_object = xmlNewChild (tasks_node, NULL, BAD_CAST "Task", NULL);
		 /* task identifier */
		 UID_node = xmlNewChild (tasks_object, NULL, BAD_CAST "UID", NULL);
		 id_node = xmlNewChild (tasks_object, NULL, BAD_CAST "ID", NULL);
		 /* we can't really use the true ID, because something seems wrong with various applications (Planner, ProjectLibre, but not MOOS) when the UID if later than the POSITION */
		 xmlNodeSetContent (UID_node, BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->id));
		 xmlNodeSetContent (id_node, BAD_CAST g_strdup_printf ("%d", tmp_tasks_datas->id));      
		 
		 /* name */
		 name_node = xmlNewChild (tasks_object, NULL, BAD_CAST "Name", NULL);
		 if(tmp_tasks_datas->name) {
			xmlNodeSetContent (name_node,  BAD_CAST g_strdup_printf ("%s", tmp_tasks_datas->name));
		 }
		 else {
			xmlNodeSetContent (name_node,  BAD_CAST g_strdup_printf ("T%d", tmp_tasks_datas->id));
		 }
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
		//	xmlNodeSetContent (wbs_node, BAD_CAST  g_strdup_printf ("%d", wbs_count));
						xmlNodeSetContent (wbs_node, BAD_CAST  "");
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
		 xmlNodeSetContent (start_node, BAD_CAST  g_strdup_printf ("%02d-%02d-%02dT%02d:%02d:00",
								  tmp_tasks_datas->start_nthYear,
								  tmp_tasks_datas->start_nthMonth,
								  tmp_tasks_datas->start_nthDay,
								  hour1, min1)
		 
						   );      
		 /* finish - don't used for now */ 
		 hour2 = data->properties.scheduleEnd/60;
		 min2 =  data->properties.scheduleEnd - 60*hour2;
		 
		 finish_node = xmlNewChild (tasks_object, NULL, BAD_CAST "Finish", NULL);
		 xmlNodeSetContent (finish_node, BAD_CAST  g_strdup_printf ("%02d-%02d-%02dT%02d:%02d:00",
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
		 if(tmp_tasks_datas->type == TASKS_TYPE_MILESTONE) {
			hour2 = 0;
			min2 = 0; 
	     }
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
		 xmlNodeSetContent (over_node, BAD_CAST "0");     
		 /* set flag duration as estimated */
		 estimated_node = xmlNewChild (tasks_object, NULL, BAD_CAST "Estimated", NULL);      
		 xmlNodeSetContent (estimated_node, BAD_CAST "0");     
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
	   
	     /* predecessors ? If we found predecessor(s) we add links  
	      * we do loop on index j in order to detect links with current task as a receiver 
	      * TODO : for now, PlanLibre uses only FS links, i.e. type 1 for MS project */
         found = project_check_task_linked (tasks_object, tmp_tasks_datas->id, data);
    }/* endif not group */
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
                 
    //     exception_name_node = xmlNewChild (exception_node, NULL, BAD_CAST "Name", NULL);            
         
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
  * 
  * data types are defined here :
  * 
**********************************/
gint save_to_project (gchar *filename, APP_data *data)
{
	gint rc = 0;
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
    xmlNodePtr properties_node = NULL, rsc_node = NULL, tasks_node = NULL, assign_node = NULL;
    xmlNodePtr links_node = NULL, calendars_node = NULL;
    gint i, j, ret;

    /* get current date */
    g_date_set_time_t (&current_date, time (NULL)); /* date of today */
    /* get current time, should be unrefed once used */
    my_time = g_date_time_new_now_local ();
    /* get currency */
    locali = localeconv ();

 

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
  //  uuid_node = xmlNewChild (root_node, NULL, BAD_CAST "UID", NULL);
 //   xmlNodeSetContent (uuid_node, BAD_CAST "1");/* ???? */
   
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
    /* finish date - the finish date is computed with latest task */
    
    finish_date_node = xmlNewChild (root_node, NULL, BAD_CAST "FinishDate", NULL);// TODO mettre parv défaut ide date début
    xmlNodeSetContent (finish_date_node, BAD_CAST     
                       g_strdup_printf ("%d-%02d-%02dT00:00:00", data->properties.end_nthYear, data->properties.end_nthMonth, data->properties.end_nthDay)
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
    /* calendar UID - it seems to be the number of calendars defined, not and UID */
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
    rsc_node = xmlNewChild (root_node, NULL, BAD_CAST "Resources", NULL);
    ret = save_resources_to_project (rsc_node, data);
    /* assignments for resources */
    assign_node = xmlNewChild (root_node, NULL, BAD_CAST "Assignments", NULL);
    ret = save_assign_to_project (assign_node, data);
 

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

   return rc;
}


/****************************
  save tasks in standard 
  CSV file, separator is
  semicolon ";"
*******************************/
gint export_project_XML (APP_data *data)
{
  gint ret = 0;
  GKeyFile *keyString;
  gchar *path_to_file, *filename, *tmpFileName;
  GtkFileFilter *filter = gtk_file_filter_new ();

  gtk_file_filter_add_pattern (filter, "*.xml");
  gtk_file_filter_set_name (filter, _("MS Project XML files"));

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");

  GtkWidget *dialog = create_saveFileDialog ( _("Export project to MS Project XML file ..."), data);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir());
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  /* run dialog for file selection */
  if(gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    if(!g_str_has_suffix (filename, ".xml" ) && !g_str_has_suffix (filename, ".XML" )) {
         /* we correct the filename */
         tmpFileName = g_strdup_printf ("%s.xml", filename);
         g_free (filename);
         filename = tmpFileName;
    }/* endif extension CSV*/
    /* TODO : check overwrite */

    gtk_widget_destroy (GTK_WIDGET(dialog)); 
    ret = save_to_project (filename, data);
    if(ret!=0) {
        gchar *msg = g_strdup_printf ("%s", _("Sorry !\nAn error happened during file exportation.\nOperation finished with errors. You can try again, or\ncheck your system."));
        misc_ErrorDialog (data->appWindow, msg);
        g_free (msg);
    }
    g_free (filename);
  }
  else
     gtk_widget_destroy (GTK_WIDGET(dialog)); 
  return ret;
}
