/*
 * icons.[ch] written by Paolo Bacchilega, who writes:
 * "There is no problem for me, you can license my code under whatever licence you wish :)"
 *
 * $Id: icons.h 11284 2010-10-01 20:22:51Z charles $
 */

#ifndef ICONS_H
#define ICONS_H

#define DIRECTORY_MIME_TYPE "folder"
#define UNKNOWN_MIME_TYPE "unknown"

const char * get_mime_type_from_filename( const char *file );

GdkPixbuf * get_mime_type_icon( const char   * mime_type,
                                GtkIconSize    icon_size,
                                GtkWidget    * for_widget );

#endif
