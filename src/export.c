/***************************
  functions to export files
****************************/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* translations */
#include <libintl.h>
#include <locale.h>
#include <string.h>
#include <uuid/uuid.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h> /* g_fopen, etc */
#include <glib/gi18n.h>
#include "interface.h" /* glade requirement */
#include "support.h" /* glade requirement */
#include "export.h"
#include "ressources.h"
#include "calendars.h"
#include "links.h"
#include "files.h"

/*********************************
  compute twips for an image
  Twips = Pixels / 96 * 1440 
  96 = DPI for the image, a
  "twip" is 1/1440  of an inch 
*********************************/
gint get_twips (gint val, gint dpi)
{
   return val*1440/dpi;
}

/******************************
  take a jpeg memory buffer
  and output an RTF image string
  i.e. in unsigned Hex
*********************************/
void output_RTF_image_string (gchar *buffer, gsize lg, FILE *outputFile, gint width, gint height)
{
  glong i, k;
  gchar *tmpstr;
 
  tmpstr = g_strdup_printf ("{\\pict\\pngblip\\picw%d\\pich%d\\picwgoal%d\\picscalex%d\\pichgoal%d\\picscaley%d \n", 
              width, height,
              get_twips(width, 144), 100, get_twips (height, 144), 100);
  /* we put the image section header */
  fwrite (tmpstr, sizeof(gchar), strlen (tmpstr), outputFile);
  g_free (tmpstr);
  /* image datas in Hexadecimal \bin is useless */
  k=0;
  for(i=0;i<lg;i++) {
     tmpstr = g_strdup_printf ("%02x",(unsigned char)buffer[i]); 
     fwrite (tmpstr, sizeof(gchar), strlen (tmpstr), outputFile);
     g_free (tmpstr);
     k++;
     /* 32 hex values by line */
     if(k>31) {
       k=0;
       fwrite ("\n", sizeof(gchar), 1, outputFile);
     }
  }
  /* and the image trailer */
  fwrite ("}\n", sizeof(gchar), 2, outputFile);
}

/******************************
  output a gchar valid pseudo
  string to an opened file
*****************************/
void output_RTF_pure_text (gchar *text, FILE *outputFile)
{
  guint16 code;
  gchar *tmpstr;
  glong i, bytes_written, bytes_read;
  GError *error = NULL;

  guint16 *uniText = g_utf8_to_utf16 (text, -1, &bytes_read, &bytes_written, &error);

  for(i=0;i<bytes_written;i++) {
      code = uniText[i];
      if(code>255) {/* pure UCS16 */
         tmpstr = g_strdup_printf ("\\u%d?",code); 
         fwrite (tmpstr, sizeof(gchar), strlen (tmpstr), outputFile);
         g_free (tmpstr);
      }
      else {
         if(code>127) {/* extended ASCII */
            tmpstr = g_strdup_printf ("%s", "\\\'"); 
            fwrite (tmpstr, sizeof(gchar), strlen (tmpstr), outputFile);
            g_free (tmpstr);
            tmpstr = g_strdup_printf ("%x",(unsigned char)code); 
            fwrite (tmpstr, sizeof(gchar), strlen (tmpstr), outputFile);
            g_free (tmpstr);
         }
         else {/* ASCII 7 bits */
           if(code>31) {
               if (code == '\\' || code == '{' || code == '}') {
                  tmpstr = g_strdup_printf ("\\%c",(char)code); 
                  fwrite (tmpstr, sizeof(gchar), 2, outputFile);
                  g_free (tmpstr);
               }
               else {
                  tmpstr = g_strdup_printf ("%c",(char)code); 
                  fwrite (tmpstr, sizeof(gchar), 1, outputFile);
                  g_free (tmpstr);
               }
           }/* code >31*/
           else {/* control codes */
             switch((char)code) {
               case '\n': {
                  tmpstr = g_strdup_printf ("%s","\\par\\pard "); /* new paragraph = reset paragraph formatting with \pard */
                  fwrite (tmpstr, sizeof(gchar), strlen (tmpstr), outputFile);
                  g_free (tmpstr);
                  fwrite ("\\s20 ", sizeof(gchar), 5, outputFile);
                  fwrite ("\\f1 ", sizeof(gchar), 3, outputFile);
                  fwrite ("\\ql ", sizeof(gchar), 3, outputFile);
                  fwrite ("\\fi0 " , sizeof(gchar), 4, outputFile);/* first line indent = 1/2"*/
                  fwrite ("\\li0 " , sizeof(gchar), 4, outputFile);/* block paargraph left indent = 1/4 " */
                  fwrite ("\\ri0 " , sizeof(gchar), 5, outputFile);/* block paragrph. right indent = 1/4 "*/
                  break;
               }
               case '\t': {
                  tmpstr = g_strdup_printf ("%s","\\tab "); 
                  fwrite (tmpstr, sizeof(gchar), strlen (tmpstr), outputFile);
                  g_free (tmpstr);
                  break;
               }
               default:;
             }/* end switch */
           }/* if control codes */
         }/* if ascii 7 bits */
      }/* extended ASCII */
  }/* next */
 g_free (uniText);
}

/**********************************
  decode tags 
  it's the core function
**********************************/
void decode_tags (GtkTextBuffer *buffer, FILE *outputFile)
{
  GtkTextIter start, iter, infIter; 
  GtkTextTagTable *tagTable1;
  GtkTextTag *tag;
  gchar *tmpStr =NULL, *pixBuffer;
  gboolean ok;
  gboolean fBold=FALSE, fItalic=FALSE, fUnderline =FALSE, fStrike = FALSE, fHighlight=FALSE;
  gboolean fSuper=FALSE, fSub =FALSE, fQuote =FALSE, fTitle =FALSE;
  gboolean fLeft=FALSE, fCenter=FALSE, fRight=FALSE, fFill =FALSE;
  GdkPixbuf *pixbuf;
  gsize pixSize;
  GError *pixError = NULL;
  gint k, width, height;

  /* we get the absolute bounds */
  gtk_text_buffer_get_start_iter (buffer, &start);

  /* we start decoding in memory */
  tagTable1 = gtk_text_buffer_get_tag_table (buffer);
  iter = start;
  infIter = start;/* infIter & supIter are the bounds of text zone */

  ok=TRUE;
  k=0;
  while(ok) {
     /* is there is a tag ? */
     /* left alignment */
     if(gtk_text_iter_has_tag (&iter,gtk_text_tag_table_lookup (tagTable1, "left"))) {
       if(!fLeft) {
         fLeft = TRUE;
         fwrite ("{\\ql ", sizeof(gchar), 5, outputFile);
       }
     }
     else {
       if(fLeft) {
          fLeft = FALSE;
          fwrite ("}", sizeof(gchar), 1, outputFile);/* for others WP */
       }
     }
     /* right alignment */
     if(gtk_text_iter_has_tag (&iter,gtk_text_tag_table_lookup (tagTable1, "right"))) {
       if(!fRight) {
         fRight = TRUE;
         fwrite ("{\\qr ", sizeof(gchar), 5, outputFile);
       }
     }
     else {
       if(fRight) {
          fRight = FALSE;
          fwrite ("}", sizeof(gchar), 1, outputFile);/* for others WP */
       }
     }
    /* center alignment */
     if(gtk_text_iter_has_tag (&iter,gtk_text_tag_table_lookup (tagTable1, "center"))) {
       if(!fCenter) {
         fCenter = TRUE;
         fwrite ("{\\qc ", sizeof(gchar), 5, outputFile);
       }
     }
     else {
       if(fCenter) {
          fCenter = FALSE; 
          fwrite ("}", sizeof(gchar), 1, outputFile);/* for others WP */
       }
     }
    /* fill alignment */
     if(gtk_text_iter_has_tag (&iter,gtk_text_tag_table_lookup (tagTable1, "fill"))) {
       if(!fFill) {
         fFill = TRUE;
         fwrite ("{\\qj ", sizeof(gchar), 5, outputFile);
       }
     }
     else {
       if(fFill) {
          fFill = FALSE;  
          fwrite ("}", sizeof(gchar), 1, outputFile);/* for others WP */
       }
     }

     if(gtk_text_iter_has_tag (&iter,gtk_text_tag_table_lookup (tagTable1, "title"))) {
       if(!fTitle) {
         fTitle = TRUE;
         /* we add the RTF bold tag on file : \b  */
         fwrite ("{\\b\\ul\\fs36 ", sizeof(gchar), 12, outputFile);
       }
     }
     else {/* if the Bold tag is already armed, then we change format ! so we must also output text */
       if(fTitle) {
          fTitle = FALSE;
          fwrite ("}", sizeof(gchar), 1, outputFile);
       }
     } 

     if(gtk_text_iter_has_tag (&iter,gtk_text_tag_table_lookup (tagTable1, "bold"))) {
       if(!fBold) {
         fBold = TRUE;
         /* we add the RTF bold tag on file : \b  */
         fwrite ("{\\b ", sizeof(gchar), 4, outputFile);
       }
     }
     else {/* if the Bold tag is already armed, then we change format ! so we must also output text */
       if(fBold) {
          fBold = FALSE;
          fwrite ("}", sizeof(gchar), 1, outputFile);
       }
     }     
     if(gtk_text_iter_has_tag (&iter,gtk_text_tag_table_lookup (tagTable1, "italic"))) {
       if(!fItalic) {
         fItalic = TRUE;
         fwrite ("{\\i ", sizeof(gchar), 4, outputFile);
       }
     }
     else {/* if the Bold tag is already armed, then we change format ! so we must also output text */
       if(fItalic) {
          fItalic = FALSE;
          fwrite ("}", sizeof(gchar), 1, outputFile);
       }
     }  
     if(gtk_text_iter_has_tag (&iter,gtk_text_tag_table_lookup (tagTable1, "underline"))) {
       if(!fUnderline) {
         fUnderline = TRUE;
         fwrite ("{\\ul ", sizeof(gchar), 5, outputFile);
       }
     }
     else {
       if(fUnderline) {
          fUnderline = FALSE;
          fwrite ("}", sizeof(gchar), 1, outputFile);
       }
     } 
     /* strikethrough */
     if(gtk_text_iter_has_tag (&iter,gtk_text_tag_table_lookup (tagTable1, "strikethrough"))) {
       if(!fStrike) {
         fStrike = TRUE;
         fwrite ("{\\strike ", sizeof(gchar), 9, outputFile);
       }
     }
     else {/* if the Bold tag is already armed, then we change format ! so we must also output text */
       if(fStrike) {
          fStrike = FALSE;
          fwrite ("}", sizeof(gchar), 1, outputFile);
       }
     }
     /* superscript */
     if(gtk_text_iter_has_tag (&iter,gtk_text_tag_table_lookup (tagTable1, "superscript"))) {
       if(!fSuper) {
         fSuper = TRUE;
         fwrite ("{\\super ", sizeof(gchar), 8, outputFile);
       }
     }
     else {
       if(fSuper) {
          fSuper = FALSE;
          fwrite ("}", sizeof(gchar), 1, outputFile);/* for others WP */
       }
     }
     /* subscript */
     if(gtk_text_iter_has_tag (&iter,gtk_text_tag_table_lookup (tagTable1, "subscript"))) {
       if(!fSub) {
         fSub = TRUE;
         fwrite ("{\\sub ", sizeof(gchar), 6, outputFile);
       }
     }
     else {
       if(fSub) {
          fSub = FALSE;
          fwrite ("}", sizeof(gchar), 1, outputFile);/* for others WP */
       }
     }
     /* highlighting - don't work with Abiword */
     if(gtk_text_iter_has_tag (&iter,gtk_text_tag_table_lookup (tagTable1, "highlight"))) {
       if(!fHighlight) {
         fHighlight = TRUE;
         //fwrite ("\\cb3 ", sizeof(gchar), 5, outputFile);
         fwrite ("{\\highlight3 ", sizeof(gchar), 13,outputFile);
         /* and the trick ! we must add all others formatings in order to keep them */
       }
     }
     else {/* if the Bold tag is already armed, then we change format ! so we must also output text */
       if(fHighlight) {
          fHighlight = FALSE;
          //fwrite ("\\cb2 ", sizeof(gchar), 5, outputFile);
         fwrite ("\\highlight0 ", sizeof(gchar), 12,outputFile);
         fwrite ("}\n", sizeof(gchar), 2, outputFile);
       }
     }
     /* quotation */
     if(gtk_text_iter_has_tag (&iter,gtk_text_tag_table_lookup (tagTable1, "quotation"))) {
       if(!fQuote) {
         fQuote = TRUE;
         /* we add the RTF bold tag on file : \b  */
       //  fwrite ("\\par ", sizeof(gchar), 5, outputFile);
         fwrite ("{", sizeof(gchar), 1, outputFile);
         fwrite ("\\pard ", sizeof(gchar), 6, outputFile);/* mandatory */
         fwrite ("\\s21", sizeof(gchar), 4, outputFile);
         fwrite ("\\fi720" , sizeof(gchar), 6, outputFile);/* first line indent = 1/2"*/
         fwrite ("\\li360" , sizeof(gchar), 6, outputFile);/* block paargraph left indent = 1/4 " */
         fwrite ("\\ri360" , sizeof(gchar), 6, outputFile);/* block paragrph. right indent = 1/4 "*/
         fwrite ("\\highlight4 ", sizeof(gchar), 12,outputFile);/* for LibreOffice */
         fwrite ("\\qj ", sizeof(gchar), 3, outputFile);/* justify */
         fwrite ("\\cb4 ", sizeof(gchar), 4, outputFile);/* background color 4 = light grey - for Abiword*/
         fwrite ("\\f0 ", sizeof(gchar), 4, outputFile);/* font 0 */
       }
     }
     else {/* if the Bold tag is already armed, then we change format ! so we must also output text */
       if(fQuote) {
          fQuote = FALSE;
          //fwrite ("\\par}\n", sizeof(gchar), 6, outputFile);
//fwrite ("\\cb0", sizeof(gchar), 4, outputFile);/* background color 4 = light grey - for Abiword*/
//fwrite ("\\highlight0 ", sizeof(gchar), 12,outputFile);
       //   fwrite ("\\par ", sizeof(gchar), 5, outputFile);/* mandatory */
          fwrite ("}", sizeof(gchar), 1, outputFile);
      //    fwrite ("\\s20", sizeof(gchar), 4, outputFile);
       //   fwrite ("\\f1 ", sizeof(gchar), 4, outputFile);
        //  fwrite ("\\ql ", sizeof(gchar), 4, outputFile);
        //  fwrite ("\\fi0 " , sizeof(gchar), 5, outputFile);/* first line indent = 1/2"*/
        //  fwrite ("\\li0 " , sizeof(gchar), 5, outputFile);/* block paargraph left indent = 1/4 " */
         // fwrite ("\\ri0 " , sizeof(gchar), 5, outputFile);/* block paragrph. right indent = 1/4 "*/
       }
     }
     /* next char - keep in mind that a picture is a char ! */
     ok = gtk_text_iter_forward_char (&iter);
     /* we must check if we are an image at the current infIter position */
     pixbuf = NULL;
     pixbuf = gtk_text_iter_get_pixbuf (&infIter);

     if(pixbuf) {
          width = gdk_pixbuf_get_width (pixbuf);
          height = gdk_pixbuf_get_height (pixbuf);
          gdk_pixbuf_save_to_buffer (pixbuf, &pixBuffer, &pixSize, "png", &pixError, NULL);
          /* we output to file the jpeg */
          output_RTF_image_string (pixBuffer, pixSize, outputFile, width, height);
          /* don't use g_object_unref !!! */
          g_free (pixBuffer);
     }

     /* if not, we output to file the current char */
     output_RTF_pure_text (gtk_text_buffer_get_text (buffer, &infIter, &iter, FALSE), outputFile);
     infIter = iter;
     k++;
  }/* wend */
}

/****************************
  save a file in standard rich
  text format (Ms RTF)
*******************************/
gint save_RTF_rich_text (gchar *filename, APP_data *data_app)
{
  gchar *tmpFileName, *fntFamily = NULL;
  const gchar *page_size_a4_portrait="\\paperh16834 \\paperw11909 \\margl1440 \\margr1440 \\margt1800 \\margb1800 \\portrait\n";
  gchar *fonts_header;
  const gchar *color_header="{\\colortbl;\\red255\\green0\\blue0;\\red0\\green0\\blue255;\\red243\\green242\\blue25;\\red241\\green241\\blue241;}\\widowctrl\\s0\\f1\\ql\\fs24\n";
  const gchar *styles_header="{\\stylesheet{\\s20\\f1\\ql redac-Normal;}{\\s21\\fi720\\li360\\ri360\\highlight4\\qj\\cb4\\f0 redac-Quotation;}}\n";
  const gchar *init_paragr_header="\\pard\\s20\\f1\\ql ";
  const gchar *rtf_header = "{\\rtf1\\ansi\\ansicpg1252\\deff0\n";
  const gchar *rtf_trailer = "}}";
  gint i, ret, fntSize = 12;
  FILE *outputFile;
  GKeyFile *keyString;
  GtkTextBuffer *buffer = data_app->buffer;
  PangoFontDescription *desc;
  /* get main font description */

  keyString = g_object_get_data (G_OBJECT(data_app->appWindow), "config"); 
  desc = pango_font_description_from_string (g_key_file_get_string(keyString, "editor", "font", NULL));
  if(desc) {
      fntFamily = pango_font_description_get_family (desc);
      fntSize = pango_font_description_get_size (desc)/1000;
  }
  fonts_header = g_strdup_printf ("{\\fonttbl{\\f0 Courier;}{\\f1 %s;}{\\f2\\fswiss\\fprq2\\fcharset222 Arial;}}\n", fntFamily);
  /* we check if filename has the .rtf extension */
  if(!g_str_has_suffix (filename, ".rtf")) {
    /* we correct the filename */
    tmpFileName = g_strconcat (filename, ".rtf", NULL);
  }
  else {
    tmpFileName = g_strdup_printf ("%s", filename);/* force to allocate memory */
  }
  ret = 0;

  /* we save as internal Gtk riche text format not the standard RTF format */
  outputFile = fopen (tmpFileName, "w");
  fwrite (rtf_header, sizeof(gchar), strlen (rtf_header), outputFile);
  fwrite (fonts_header, sizeof(gchar), strlen (fonts_header), outputFile);
  fwrite (color_header, sizeof(gchar), strlen (color_header), outputFile);
  fwrite (styles_header, sizeof(gchar), strlen (styles_header), outputFile);
  fwrite (page_size_a4_portrait, sizeof(gchar), strlen (page_size_a4_portrait), outputFile);
  fwrite (init_paragr_header, sizeof(gchar), strlen (init_paragr_header), outputFile);

  decode_tags (buffer, outputFile);

  fwrite (rtf_trailer, sizeof(gchar), strlen (rtf_trailer), outputFile);

  fclose (outputFile);
  g_free (fonts_header);
  g_free (tmpFileName);
  return ret;
}

/****************************
  save a file in standard 
  Icalendar (ICS) file format
*******************************/
gint save_project_to_icalendar (gchar *filename, APP_data *data)
{
  gint ret = 0, i, j, day, month, year, t_mark, total, ok, k, t_mark_rsc, wd, hour;
  GDate date;
  GList *l, *l1;
  gchar *tmpStr;
  uuid_t newUUID;
  gchar *uuidStr = g_malloc (38);
  tasks_data *tmpTsk;
  rsc_datas *tmp_rsc_datas;
  FILE *outputFile;
  const gchar *ics_header = "BEGIN:VCALENDAR\nVERSION:2.0\nPRODID:-//Laboratoire PlanLibre//PlanLibre 0.1//EN\nCALSCALE:GREGORIAN\nMETHOD:PUBLISH\n";
  const gchar *ics_trailer = "END:VCALENDAR\n";
  const gchar *ics_begin_event = "BEGIN:VEVENT\n";
  const gchar *ics_end_event = "END:VEVENT\n";
  const gchar *ics_transparent = "TRANSP:TRANSPARENT\n";

  /* we open file and output header */
  outputFile = fopen (filename, "w");
  fwrite (ics_header, sizeof(gchar), strlen (ics_header), outputFile);/* simple example here : https://icalendar.org/ */

  total = g_list_length (data->rscList);

  /* content */
  for(i=0; i<g_list_length (data->tasksList);i++) {
     l = g_list_nth (data->tasksList, i);
     tmpTsk = (tasks_data *) l->data;
     g_date_set_dmy (&date, tmpTsk->start_nthDay, tmpTsk->start_nthMonth, tmpTsk->start_nthYear);
     if(tmpTsk->type == 0) {/* we export only TRUE tasks, not grpups */
     	     /* do loop for all days where this event is present */
	     for(j=0;j<tmpTsk->days;j++) {
                /* we test if current day is worked or not */
                /* current date - */
                wd = g_date_get_weekday (&date);
	        day = g_date_get_day (&date);
	        month = g_date_get_month (&date);
	        year = g_date_get_year (&date);         
                t_mark = calendars_date_is_marked (day, month, year, calendars_get_nth_for_id (0, data), data);/* calendar = 0 == ID for default calendar  */
                if(t_mark<0) {/* <0 if day isn't marked */
		     fwrite (ics_begin_event, sizeof(gchar), strlen (ics_begin_event), outputFile);
		     /* UID - Glib gchar *g_uuid_string_random (void); not compatible with 14.04 
                    - thanks to https://stackoverflow.com/questions/51053568/generating-a-random-uuid-in-c */

		     uuid_generate (newUUID);
		     uuid_unparse (newUUID, uuidStr);
		     tmpStr = g_strdup_printf ("UID:%s\n", uuidStr);
		     fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
		     g_free (tmpStr);

		     /* starting date - trailing 0 mandatory for month and day */
                     hour = calendars_get_starting_hour (0, misc_get_calendar_day_from_glib (wd), data);
                     if(hour<0)
                         hour = data->properties.scheduleStart;
		     tmpStr = g_strdup_printf ("DTSTART:%d%.2d%.2dT%.2d%.2d00\n", year, month, day, hour/60, hour % 60);
		     fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
		     g_free (tmpStr);
		     /* ending date - please note - requires that ending date is always >starting date  
                       - trailing 0 mandatory for month and day  */
                     hour = calendars_get_ending_hour (0, misc_get_calendar_day_from_glib (wd), data);
                     if(hour<0)
                         hour = data->properties.scheduleEnd;
		     tmpStr = g_strdup_printf ("DTEND:%d%.2d%.2dT%.2d%.2d00\n", year, month, day, hour/60, hour % 60);
		     fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
		     g_free (tmpStr);
		     /* summary */
		     tmpStr = g_strdup_printf ("SUMMARY:%s\n", tmpTsk->name);
		     fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
		     g_free (tmpStr);
		     /* transparent ? */
		     fwrite (ics_transparent, sizeof(gchar), strlen (ics_transparent), outputFile);
// participants : voir ici https://icalendar.org/CalDAV-Scheduling-RFC-6638/b-1-example-organizer-inviting-multiple-attendees.html
                     /* we check for each ressource if they belong to current task */
                     for (k=0; k<total; k++) {
                        l1 = g_list_nth (data->rscList, k);
                        tmp_rsc_datas = (rsc_datas *)l1->data;
                        ok = links_check_element (tmpTsk->id, tmp_rsc_datas->id , data);
                        if(ok>=0) {/* now, we ought to test if ressource is really available : calendar and days test */
                             t_mark_rsc = calendars_date_is_marked (day, month, year, 
                                                        calendars_get_nth_for_id (tmp_rsc_datas->calendar, data), data);
                             if((t_mark_rsc<0) && (rsc_is_available_for_day (tmp_rsc_datas, wd))) {
                                tmpStr = g_strdup_printf ("ATTENDEE;ROLE=REQ-PARTICIPANT;CN=%s\n", tmp_rsc_datas->name);
                                fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
		                g_free (tmpStr);
                             }
                        }
                     }/* next k */

		     /* close event */
		     fwrite (ics_end_event, sizeof(gchar), strlen (ics_end_event), outputFile);
                }/* endif t_mark */
		g_date_add_days (&date, 1);
	     }/* next j */
     }/* zndif task type */
  }/* next i */

/* exemple intéressant venant d'ici : https://icalendar.org/iCalendar-RFC-5545/3-6-1-event-component.html
BEGIN:VEVENT
 UID:20070423T123432Z-541111@example.com
 DTSTAMP:20070423T123432Z
 DTSTART;VALUE=DATE:20070628
 DTEND;VALUE=DATE:20070709
 SUMMARY:Festival International de Jazz de Montreal
 TRANSP:TRANSPARENT
 END:VEVENT


*/
  /* trailer and close file */
  fwrite (ics_trailer, sizeof(gchar), strlen (ics_trailer), outputFile);
  fclose (outputFile);

  g_free (uuidStr);
  return ret;
}

/****************************
  save a file in standard 
  Icalendar (ICS) file format
  for a "ressource" with
  identifiant ID
*******************************/
gint save_project_to_icalendar_for_rsc (gchar *filename, gint id, APP_data *data)
{
  gint ret = 0, i, j, day, month, year, t_mark, total, ok, k, t_mark_rsc, wd, hourStart, hourEnd, avg_day;
  GDate date;
  GList *l, *l1;
  gchar *tmpStr;
  uuid_t newUUID;
  gchar *uuidStr = g_malloc (38);
  tasks_data *tmpTsk;
  rsc_datas *tmp_rsc_datas;
  FILE *outputFile;
  const gchar *ics_header = "BEGIN:VCALENDAR\nVERSION:2.0\nPRODID:-//Laboratoire PlanLibre//PlanLibre 0.1//EN\nCALSCALE:GREGORIAN\nMETHOD:PUBLISH\n";
  const gchar *ics_trailer = "END:VCALENDAR\n";
  const gchar *ics_begin_event = "BEGIN:VEVENT\n";
  const gchar *ics_end_event = "END:VEVENT\n";
  const gchar *ics_transparent = "TRANSP:TRANSPARENT\n";

  /* we open file and output header */
  outputFile = fopen (filename, "w");
  fwrite (ics_header, sizeof(gchar), strlen (ics_header), outputFile);/* simple example here : https://icalendar.org/ */

  total = g_list_length (data->rscList);

  /* content */
  for(i=0; i<g_list_length (data->tasksList);i++) {
     l = g_list_nth (data->tasksList, i);
     tmpTsk = (tasks_data *) l->data;
     g_date_set_dmy (&date, tmpTsk->start_nthDay, tmpTsk->start_nthMonth, tmpTsk->start_nthYear);
     if(tmpTsk->type == 0) {/* we export only TRUE tasks, not groups */
     	  /* do loop for all days where this event is present */
	  for(j=0;j<tmpTsk->days;j++) {
             /* we test if current day is worked or not */
             /* current date - */
             wd = g_date_get_weekday (&date);
	     day = g_date_get_day (&date);
	     month = g_date_get_month (&date);
	     year = g_date_get_year (&date);         
             t_mark = calendars_date_is_marked (day, month, year, calendars_get_nth_for_id (0, data), data);/* calendar = 0 == ID for default calendar for tasks  */
             if(t_mark<0) {/* <0 if day isn't marked */
		for(k=0; k<total; k++) {
		   l1 = g_list_nth (data->rscList, k);
          	   tmp_rsc_datas = (rsc_datas *)l1->data;
                   if(tmp_rsc_datas->id==id) {
		      ok = links_check_element (tmpTsk->id, tmp_rsc_datas->id , data);
		      if(ok>=0) {/* now, we ought to test if ressource is really available : calendar and days test */
			  t_mark_rsc = calendars_date_is_marked (day, month, year, 
			                              calendars_get_nth_for_id (tmp_rsc_datas->calendar, data), data);
			  if((t_mark_rsc<0) && (rsc_is_available_for_day (tmp_rsc_datas, wd))) {
                             fwrite (ics_begin_event, sizeof(gchar), strlen (ics_begin_event), outputFile);
			     uuid_generate (newUUID);
			     uuid_unparse (newUUID, uuidStr);
			     tmpStr = g_strdup_printf ("UID:%s\n", uuidStr);
			     fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
			     g_free (tmpStr);
                             /* starting date - trailing 0 mandatory for month and day */
                             hourStart = calendars_get_starting_hour (calendars_get_nth_for_id (tmp_rsc_datas->calendar, data),
                                                              misc_get_calendar_day_from_glib (wd), data);
                             if(hourStart<0)
                                    hourStart = data->properties.scheduleStart;
		             tmpStr = g_strdup_printf ("DTSTART:%d%.2d%.2dT%.2d%.2d00\n", year, month, day, 
                                                           hourStart/60, hourStart % 60); 
			     fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
			     g_free (tmpStr);
			     /* ending date - please note - requires that ending date is always >starting date  
				   - trailing 0 mandatory for month and day  */
                             hourEnd = calendars_get_ending_hour (calendars_get_nth_for_id (tmp_rsc_datas->calendar, data),
                                                              misc_get_calendar_day_from_glib (wd), data);
                             if(hourEnd<0)
                                    hourEnd = data->properties.scheduleEnd;
                             /* we must scheck if ending hour don't exceed maximum working time for current ressource */
                             avg_day = rsc_get_max_daily_working_time (tmp_rsc_datas->id, data);
                             if(hourEnd-hourStart>avg_day) {
                                      hourEnd = hourStart+avg_day;
                             }
                             /* test limits */
                             if(hourEnd>1439)
                                hourEnd = 1439; /* 23h59 exprimed in minutes */
		             tmpStr = g_strdup_printf ("DTEND:%d%.2d%.2dT%.2d%.2d00\n", year, month, day, 
                                                           hourEnd/60, hourEnd % 60);
			     fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
			     g_free (tmpStr);
                             /* summary */
			     tmpStr = g_strdup_printf ("SUMMARY:%s\n", tmpTsk->name);
			     fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
			     g_free (tmpStr);
                             /* transparent ? */
			     fwrite (ics_transparent, sizeof(gchar), strlen (ics_transparent), outputFile);
                             /* close event */
			     fwrite (ics_end_event, sizeof(gchar), strlen (ics_end_event), outputFile);
			  }/* endif t_mark */
		      }/* endif OK */
                   }/* endif ID */
		}/* next k */		     
	     }/* endif t_mark */
	     g_date_add_days (&date, 1);
	  }/* next j */
     }/* zndif task type */
  }/* next i */

  /* trailer and close file */
  fwrite (ics_trailer, sizeof(gchar), strlen (ics_trailer), outputFile);
  fclose (outputFile);

  g_free (uuidStr);
  return ret;
}

/*****************************
  PROTECTED : export tasks
  to csv file
*****************************/
static gint export_tasks_to_csv_file (gchar *filename, APP_data *data)
{
  gint ret = 0, i;
  FILE *outputFile;
  const gchar *task_header = _("Name;Starting date;Duration(days);Duration(hours);Due;Due date;Done;Priority;Status\n");
  const gchar *task_def_name = _("Noname tasks");
  gchar *tmpStr;
  tasks_data *tmpTsk;
  GList *l;
  GDate date;

  /* we open file and output header */
  outputFile = fopen (filename, "w");
  fwrite (task_header, sizeof(gchar), strlen (task_header), outputFile);
  for(i=0;i<g_list_length (data->tasksList); i++) {
     l = g_list_nth (data->tasksList, i);
     tmpTsk = (tasks_data *) l->data;
     g_date_set_dmy (&date, tmpTsk->start_nthDay, tmpTsk->start_nthMonth, tmpTsk->start_nthYear);
     if(tmpTsk->type == TASKS_TYPE_TASK) {
        if(tmpTsk->name) {
           if(strlen(tmpTsk->name)>0) {
              tmpStr = g_strdup_printf ("\"%s\"", tmpTsk->name);
              misc_erase_c (tmpStr, ';');/* remove unwanted separators */
              fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
              g_free (tmpStr);
           }
           else {
              fwrite (task_def_name, sizeof(gchar), strlen (task_def_name), outputFile);
           }
           fwrite (";", sizeof(gchar), 1, outputFile);
        }
        /* start date */
        tmpStr = misc_convert_date_to_friendly_str (&date);
        fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
        g_free (tmpStr);
        fwrite (";", sizeof(gchar), 1, outputFile);
        /* durations */
        tmpStr = g_strdup_printf ("%d;%d;", tmpTsk->days, tmpTsk->hours);
        fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
        g_free (tmpStr);
        /* due mode + due date */
        switch(tmpTsk->durationMode) {
           case TASKS_AS_SOON: {
             tmpStr = g_strdup_printf ("%s;", _("as soon as possible"));
             fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
             g_free (tmpStr);
             g_date_set_dmy (&date, tmpTsk->start_nthDay, tmpTsk->start_nthMonth, tmpTsk->start_nthYear);
             break;
           }
           case TASKS_DEADLINE: {
             tmpStr = g_strdup_printf ("%s;", _("don't end after"));
             fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
             g_free (tmpStr);
             g_date_set_dmy (&date, tmpTsk->lim_nthDay, tmpTsk->lim_nthMonth, tmpTsk->lim_nthYear);
             break;
           }
           case TASKS_AFTER: {
             tmpStr = g_strdup_printf ("%s;", _("start no earlier than"));
             fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
             g_free (tmpStr);
             g_date_set_dmy (&date, tmpTsk->lim_nthDay, tmpTsk->lim_nthMonth, tmpTsk->lim_nthYear);
             break;
           }
           case TASKS_FIXED: {
             tmpStr = g_strdup_printf ("%s;", _("must start on"));
             fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
             g_free (tmpStr);
             g_date_set_dmy (&date, tmpTsk->start_nthDay, tmpTsk->start_nthMonth, tmpTsk->start_nthYear);
             break;
           }
           default: {
             tmpStr = g_strdup_printf ("%s;", _("N/A"));
             fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
             g_free (tmpStr);
             g_date_set_dmy (&date, tmpTsk->start_nthDay, tmpTsk->start_nthMonth, tmpTsk->start_nthYear);
           }
        }
        tmpStr = misc_convert_date_to_friendly_str (&date);
        fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
        g_free (tmpStr);
        fwrite (";", sizeof(gchar), 1, outputFile);
        /* % done */
        tmpStr = g_strdup_printf ("%.2f;", tmpTsk->progress);
        fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
        g_free (tmpStr);
        /* priority */
        switch(tmpTsk->priority) {
           case 0: {
             tmpStr = g_strdup_printf ("%s;", _("High"));
             fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
             g_free (tmpStr);
             break;
           }
           case 1: {
             tmpStr = g_strdup_printf ("%s;", _("Medium"));
             fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
             g_free (tmpStr);
             break;
           }
           case 2: {
             tmpStr = g_strdup_printf ("%s;", _("Low"));
             fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
             g_free (tmpStr);
             break;
           }
           default: {
             tmpStr = g_strdup_printf ("%s;", _("N/A"));
             fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
             g_free (tmpStr);
           }
        }
        /* status */
        switch(tmpTsk->status) {
           case 0: {
             tmpStr = g_strdup_printf ("%s;", _("Run"));
             fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
             g_free (tmpStr);
             break;
           }
           case 1: {
             tmpStr = g_strdup_printf ("%s;", _("Done"));
             fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
             g_free (tmpStr);
             break;
           }
           case 2: {
             tmpStr = g_strdup_printf ("%s;", _("Overdue"));
             fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
             g_free (tmpStr);
             break;
           }
           case 3: {
             tmpStr = g_strdup_printf ("%s;", _("To Go"));
             fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
             g_free (tmpStr);
             break;
           }
           default: {
             tmpStr = g_strdup_printf ("%s;", _("N/A"));
             fwrite (tmpStr, sizeof(gchar), strlen (tmpStr), outputFile);
             g_free (tmpStr);
           }
        }
        /* new line */
        fwrite ("\n", sizeof(gchar), 1, outputFile);
     }/* endif type task*/

  }/* next i */
  fclose (outputFile);
  return ret;
}

/****************************
  save tasks in standard 
  CSV file, separator is
  semicolon ";"
*******************************/
gint save_tasks_CSV (APP_data *data)
{
  gint ret = 0;
  GKeyFile *keyString;
  gchar *path_to_file, *filename, *tmpFileName;
  GtkFileFilter *filter = gtk_file_filter_new ();

  gtk_file_filter_add_pattern (filter, "*.csv");
  gtk_file_filter_set_name (filter, _("Spreadsheets CSV files"));

  keyString = g_object_get_data (G_OBJECT(data->appWindow), "config");

  GtkWidget *dialog = create_saveFileDialog ( _("Export tasks to spreadsheet file ..."), data);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir());
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  /* run dialog for file selection */
  if(gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    if(!g_str_has_suffix (filename, ".csv" ) && !g_str_has_suffix (filename, ".CSV" )) {
         /* we correct the filename */
         tmpFileName = g_strdup_printf ("%s.csv", filename);
         g_free (filename);
         filename = tmpFileName;
    }/* endif extension CSV*/
    /* TODO : check overwrite */

    gtk_widget_destroy (GTK_WIDGET(dialog)); 
    ret = export_tasks_to_csv_file (filename, data);
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
