/* gapcmon.c               serial-0088-0 *****************************************

  GKT+ GUI with Notification Area (System Tray) support.  Program  for 
  monitoring the apcupsd.sourceforge.net package.
  Copyright (C) 2006 James Scott, Jr. <skoona@users.sourceforge.net>

  Command Line Syntax: gapcmon [--help]

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* ************************************************************************** *
 * Program Structure
 * ------------------------ -------------------------------------------------
 * GtkWindow				Main Control Program Thread ----
 * EggTrayIcon              Notification system tray icon for main app.
 * * GtkNotebook            Main Interface
 *   * GtkTreeView          Monitors pg: list of active monitors and state
 *     - EggTrayIcon        Notification system tray icon for monitors.
 *     - GtkWindow          Monitor Information Window - 
 *                          TrayIcon and InfoWindow compose a monitor
 *       * PG-n GtkNotebook Active monitor notebook with four pages; 
 *                          summary chart, detailed, eventlog, statuslog pages.
 *         - nbPage HISTORY A lg_Graph histogram line chart of
 *                          LINEV, LOADPCT, BCHARGE, CUMONBATT, TIMELEFT
 *           g_timeout_add  Monitor g_timeout per monitor for data updates
 *           g_timeout_add  Monitor g_timeout per graph 1:30 collection updates
 *         - nbPage DETAILS Grouped results from the status output.
 *         - nbPage EVENTS  Current events log
 *         - nbPage STATUS  Current status log, same as APCACCESS
 * 	 * GtkTreeView	        Preferences pg: For Main app, and Monitors
 * 	 * GtkVBox              About pg: Program copyright info
 * * GtkHBox                Action info
 *   - GtkLabel             Program Title line
 *   - GtkButton            Program Close button
 * * GtkStatusbar           Status message line
 *
 * ------------------------ ---------------------------------------------------
 *   + GConfClient	    Tied to Preferences page
 * 			    Produces direct change on application state and
 * 			    operation by monitoring changes to config values
 * ------------------------ ---------------------------------------------------
 *     + GThread  	    Network Communication via socket io using GIOChannels
 * 			    Communication to interface gtkthread through
 *  			    a GAsyncQueue, with additional instance mutex
 *                          - protect hash table from multi-thread access
 *                          - protect GTK from multi-thread access
 *                            gdk_thread_[enter|leave] around gtk calls in timer
 *                            routines and threads - and gtk_main_loop.
 * ------------------------ ---------------------------------------------------
 *  GCONF2 Info
 *  CURRENT KEYS ARE:
 *  key /schemas/apps/gapcmon/controller/keys
 *              /apps/gapcmon/monitor/x/monitor-keys
 *  Where x is the internal monitor number.
 *  max monitors=unlimted or sizeof guint
 *  Where key is the actual keyname like enabled, host_name, port_name, or
 *  refresh_interval, etc.
 * ************************************************************************** *
*/

#include <unistd.h>             /* close() */
#include <sys/types.h>          /* socket() */
#include <sys/socket.h>         /* socket() */
#include <netinet/in.h>         /* sockaddr_in */
#include <arpa/inet.h>          /* ntohs() */
#include <netinet/in.h>         /* sockaddr_in */
#include <netdb.h>              /* gethostbyname() */
#include <errno.h>
#include <string.h>             /* memset() */
#include <time.h>
#include <stdlib.h>             /* malloc() */

#include <gconf/gconf-client.h>
#include <gtk/gtk.h>
#include "eggtrayicon.h"
#include "gapcmon.h"

static gboolean cb_monitor_dedicated_one_time_refresh(PGAPC_MONITOR pm);
static gboolean cb_monitor_automatic_refresh(PGAPC_MONITOR pm);
static gboolean cb_monitor_refresh_control(PGAPC_MONITOR pm);
static gboolean gapc_monitor_update_tooltip_msg(PGAPC_MONITOR pm);
static gint gapc_monitor_update(PGAPC_MONITOR pm);

static gdouble gapc_util_point_filter_set(PGAPC_SUMS sq, gdouble this_point);
static gdouble gapc_util_point_filter_reset(PGAPC_SUMS sq);
static void lg_graph_set_chart_title (PLGRAPH plg, gchar * pch_text);
static void lg_graph_set_y_label_text (PLGRAPH plg, gchar * pch_text);
static void lg_graph_set_x_label_text (PLGRAPH plg, gchar * pch_text);

static void lg_graph_set_chart_title_color (PLGRAPH plg, gchar * pch_color);
static void lg_graph_set_chart_scales_color (PLGRAPH plg, gchar * pch_color);
static void lg_graph_set_chart_window_fg_color (PLGRAPH plg, gchar * pch_color);
static void lg_graph_set_chart_window_bg_color (PLGRAPH plg, gchar * pch_color);

static PLGRAPH lg_graph_create (GtkWidget * box, gint width, gint height);
static void lg_graph_set_ranges (PLGRAPH plg,
                                 gint xminor_by,
                                 gint xmajor_by,
                                 gint x_min,
                                 gint x_max,
                                 gint yminor_by, gint ymajor_by, gint y_min, gint y_max);
static void lg_graph_redraw (PLGRAPH plg);

static gint lg_graph_data_series_add (PLGRAPH plg, gchar * pch_legend_text,
                                      gchar * pch_color_text);
static gboolean lg_graph_data_series_remove_all (PLGRAPH plg);
static gboolean lg_graph_data_series_add_value (PLGRAPH plg, gint i_series_number,
                                                gdouble y_value);
static gint lg_graph_data_series_draw (PLGRAPH plg, PLG_SERIES psd);

/* 
 * Private Interfaces */
static gint lg_graph_draw_tooltip (PLGRAPH plg);
static gint lg_graph_data_series_draw_all (PLGRAPH plg, gboolean redraw_control);
static void lg_graph_get_default_sizes (PLGRAPH plg, gint * width, gint * height);
static void lg_graph_draw_x_grid_labels (PLGRAPH plg);
static void lg_graph_draw_y_grid_labels (PLGRAPH plg);
static gint lg_graph_draw_grid_lines (PLGRAPH plg);
static gint lg_graph_draw_horizontal_text (PLGRAPH plg,
                                           gchar * pch_text,
                                           GdkRectangle * rect, gboolean redraw_control);
static gint lg_graph_draw_vertical_text (PLGRAPH plg,
                                         gchar * pch_text,
                                         GdkRectangle * rect, gboolean redraw_control);
static gint lg_graph_draw (PLGRAPH plg);
static gint lg_graph_configure_event_cb (GtkWidget * widget,
                                         GdkEventConfigure * event, PLGRAPH plg);
static gint lg_graph_expose_event_cb (GtkWidget * widget, GdkEventExpose * event,
                                      PLGRAPH plg);
static gboolean lg_graph_motion_notify_event_cb (GtkWidget * widget, GdkEventMotion * ev,
                                                 PLGRAPH plg);
static gboolean lg_graph_button_press_event_cb (GtkWidget * widget, GdkEventButton * ev,
                                                PLGRAPH plg);
static gboolean cb_util_barchart_handle_exposed(GtkWidget * widget,
   GdkEventExpose * event, gpointer data);
static gboolean cb_util_line_chart_refresh(PGAPC_HISTORY pg);

static gboolean cb_util_manage_iconify_event(GtkWidget *widget, 
                                             GdkEventWindowState *event,
                                             gpointer  gp);
static void gapc_util_text_view_append(GtkWidget * view, gchar * pch);
static void gapc_util_text_view_prepend(GtkWidget * view, gchar * pch);
static gboolean gapc_util_text_view_clear_buffer(GtkWidget * view);
static gboolean gapc_util_treeview_get_iter_from_monitor(GtkTreeModel * model,
   GtkTreeIter * iter, gint i_value);
static gint gapc_util_update_hashtable(PGAPC_MONITOR pm, gchar * pch_unparsed);
static void cb_panel_systray_icon_destroy(GtkObject * object, gpointer gp);
static void cb_main_interface_button_quit(GtkWidget * button, PGAPC_CONFIG pcfg);
static void gapc_monitor_interface_destroy(PGAPC_CONFIG pcfg, gint i_monitor);
static GtkWidget *gapc_monitor_interface_create(PGAPC_CONFIG pcfg, gint i_monitor,
   GtkTreeIter * iter);
static void cb_panel_monitor_list_activated(GtkTreeView * treeview,
   GtkTreePath * arg1, GtkTreeViewColumn * arg2, PGAPC_CONFIG pcfg);
static gint gapc_panel_glossary_page(PGAPC_CONFIG pcfg, GtkWidget * notebook);
static gint gapc_panel_graph_property_page(PGAPC_CONFIG pcfg, GtkWidget * notebook);

/*
 * Common interface to the various versions of gethostbyname_r().
 * Implemented in gethostname.c.
 */
struct hostent * gethostname_re
    (const char *host,struct hostent *hostbuf,char **tmphstbuf,size_t *hstbuflen);

/* 
 * Some small number of globals are required
*/
static gboolean lg_graph_debug = FALSE;


/* ************************************************************************* */

/*
 * Draws one data series points to chart
 * returns number of points processed
*/
static gint lg_graph_data_series_draw (PLGRAPH plg, PLG_SERIES psd)
{
    gint        v_index = 0;
    GdkPoint   *point_pos = NULL;

    g_return_val_if_fail (plg != NULL, -1);
    g_return_val_if_fail (psd != NULL, -1);
    g_return_val_if_fail (psd->point_pos != NULL, -1);

    gdk_gc_set_rgb_fg_color (plg->series_gc, &psd->legend_color);
    gdk_gc_set_line_attributes (plg->series_gc, 2,
                                GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);

    point_pos = psd->point_pos;

/* trap first and only point */
    if (psd->i_point_count == 0)
    {
        return 0;
    }
    if (psd->i_point_count == 1)
    {
        point_pos[0].x = plg->plot_box.x;
        point_pos[0].y =
            (plg->plot_box.y + plg->plot_box.height) -
            ((psd->lg_point_dvalue[0] *
              (gdouble) ((gdouble) plg->plot_box.height /
                         (gdouble) plg->y_range.i_max_scale)));

        gdk_draw_arc (plg->pixmap, plg->series_gc, TRUE,
                      point_pos[0].x - 1, point_pos[0].y - 2, 3, 3, 0, 360 * 64);
        return 1;
    }

    for (v_index = 0; v_index < psd->i_point_count; v_index++)
    {
        point_pos[v_index].x = plg->plot_box.x + (v_index * plg->x_range.i_minor_inc);
        point_pos[v_index].y = (plg->plot_box.y + plg->plot_box.height) -
            ((psd->lg_point_dvalue[v_index] *
              (gdouble) ((gdouble) plg->plot_box.height /
                         (gdouble) plg->y_range.i_max_scale)));

        if ((v_index != 0) && (v_index < psd->i_point_count - 1))
        {
            gdk_draw_arc (plg->pixmap, plg->series_gc, TRUE,
                          point_pos[v_index].x - 1,
                          point_pos[v_index].y - 2, 3, 3, 0, 360 * 64);
            gdk_draw_arc (plg->pixmap, plg->series_gc, FALSE,
                          point_pos[v_index].x - 1,
                          point_pos[v_index].y - 2, 3, 3, 0, 360 * 64);
        }
    }

    gdk_draw_lines (plg->pixmap, plg->series_gc, point_pos, psd->i_point_count);

    return v_index;
}

/*
 * Draws all data series points to chart
 * returns number of series processed, or -1 if not drawable
*/
static gint lg_graph_data_series_draw_all (PLGRAPH plg, gboolean redraw_control)
{
    PLG_SERIES  psd = NULL;
    GList      *data_sets = NULL;
    gint        v_index = 0;

    g_return_val_if_fail (plg != NULL, -1);

    if ( !(GTK_WIDGET_DRAWABLE (plg->drawing_area)) ) {
        return -1;
    }

    data_sets = g_list_first (plg->lg_series);
    while (data_sets)
    {
        psd = data_sets->data;
        if (psd != NULL)
        {                       /* found */
            lg_graph_data_series_draw (plg, psd);
            v_index++;
        }
        data_sets = g_list_next (data_sets);
    }

    if (lg_graph_debug)
    {
        g_print ("DrawAllDataSeries: series=%d\n", v_index);
    }

    return v_index;
}

/*
 * Add a single value to the requested data series
 * auto indexes the value is max is reach (appends to the end)
*/
static gboolean lg_graph_data_series_add_value (PLGRAPH plg, gint i_series_number,
                                                gdouble y_value)
{
    PLG_SERIES  psd = NULL;
    GList      *data_sets = NULL;
    gint        v_index = 0, time_count = 0;
    gboolean    b_found = FALSE;

    g_return_val_if_fail (plg != NULL, FALSE);

    data_sets = g_list_first (plg->lg_series);
    while (data_sets)
    {
        psd = data_sets->data;
        if (psd->i_series_id == i_series_number)
        {                       /* found */
            b_found = TRUE;
            break;
        }
        data_sets = g_list_next (data_sets);
    }

    if (!b_found)
    {
        g_message ("lg_graph_data_series_add_value(%d): Invalid data series number",
                   i_series_number);
        return FALSE;
    }

    if (y_value >= plg->y_range.i_max_scale)
    {
        y_value = (gdouble) plg->y_range.i_max_scale * 0.98;
    }

    if (psd->i_point_count == psd->i_max_points + 1)
    {
        for (v_index = 0; v_index < psd->i_max_points; v_index++)
        {
            psd->lg_point_dvalue[v_index] = psd->lg_point_dvalue[v_index + 1];
        }
        psd->lg_point_dvalue[psd->i_max_points] = y_value;
    }
    else
    {
        psd->lg_point_dvalue[psd->i_point_count++] = y_value;
    }

    psd->d_max_value = MAX (y_value, psd->d_max_value);
    psd->d_min_value = MIN (y_value, psd->d_min_value);

    plg->i_points_available = MAX (plg->i_points_available, psd->i_point_count);

    /* record current time with data points */
    if (psd->i_series_id == plg->i_num_series - 1)
    {
        GList      *gl_remove = NULL;

        gl_remove = g_list_first (plg->lg_series_time);
        
        time_count = g_list_length (plg->lg_series_time);    
        if (time_count == psd->i_max_points + 1)
        {
            plg->lg_series_time =
                g_list_remove_all (plg->lg_series_time, gl_remove->data);
        }
        plg->lg_series_time =
            g_list_append (plg->lg_series_time, GINT_TO_POINTER ((time_t) time (NULL)));
    }

    if (lg_graph_debug)
    {
        g_print
         ("DataSeriesAddValue: series=%d, value=%3.1f, index=%d, count=%d, time_count=%d, max_pts=%d\n",
          i_series_number, y_value, v_index, psd->i_point_count, time_count, psd->i_max_points);
    }

    return TRUE;
}

/*
 * A shutdown routine
 * destroys all the data series and any assocaited dynamic data
*/
static gboolean lg_graph_data_series_remove_all (PLGRAPH plg)
{
    PLG_SERIES  psd = NULL;
    GList      *data_sets = NULL;
    gint        i_count = 0;

    g_return_val_if_fail (plg != NULL, FALSE);

    data_sets = g_list_first (plg->lg_series);
    while (data_sets)
    {
        psd = data_sets->data;
        g_free (psd->lg_point_dvalue);
        g_free (psd->point_pos);
        g_free (psd);
        data_sets = g_list_next (data_sets);
        i_count++;
    }
    g_list_free (plg->lg_series);
    g_list_free (plg->lg_series_time);
    plg->lg_series = NULL;
    plg->lg_series_time = NULL;
    plg->i_num_series = 0;
    plg->i_points_available = 0;    

    if (lg_graph_debug)
    {
        g_print ("DataSeriesRemoveAll: series total=%d\n", i_count);
    }

    return TRUE;
}

/*
 * allocates space for another data series
 * returns the series number of this dataset
*/
static gint lg_graph_data_series_add (PLGRAPH plg, gchar * pch_legend_text,
                                      gchar * pch_color_text)
{
    PLG_SERIES  psd = NULL;

    g_return_val_if_fail (plg != NULL, -1);
    g_return_val_if_fail (pch_legend_text != NULL, -1);
    g_return_val_if_fail (pch_color_text != NULL, -1);

    psd = (PLG_SERIES) g_new0 (LG_SERIES, 1);
    g_return_val_if_fail (psd != NULL, -1);

    psd->lg_point_dvalue = (gdouble *) g_new0 (gdouble, (plg->x_range.i_max_scale + 4));
    g_return_val_if_fail (psd->lg_point_dvalue != NULL, -1);

    psd->point_pos = g_new0 (GdkPoint, (plg->x_range.i_max_scale + 4));
    g_return_val_if_fail (psd->point_pos != NULL, -1);

    g_snprintf (psd->ch_legend_text, sizeof (psd->ch_legend_text), "%s", pch_legend_text);
    psd->i_max_points = plg->x_range.i_max_scale;
    gdk_color_parse (pch_color_text, &psd->legend_color);
    g_snprintf (psd->ch_legend_color, sizeof (psd->ch_legend_color), "%s",
                pch_color_text);
    psd->cb_id = CB_SERIES_ID;

    plg->lg_series = g_list_append (plg->lg_series, psd);
    psd->i_series_id = plg->i_num_series++;

    if (lg_graph_debug)
    {
        g_print ("DataSeriesAdd: series=%d, max_pts=%d\n",
                 psd->i_series_id, psd->i_max_points);
    }

    return psd->i_series_id;
}

/*
 * Set the bottom x label text 
*/
static void lg_graph_set_x_label_text (PLGRAPH plg, gchar * pch_text)
{
    g_return_if_fail (plg != NULL);

    if (plg->x_label_text != NULL)
    {
        g_free (plg->x_label_text);
    }
    plg->x_label_text = g_strdup (pch_text);
}
static void lg_graph_set_y_label_text (PLGRAPH plg, gchar * pch_text)
{
    g_return_if_fail (plg != NULL);

    if (plg->y_label_text != NULL)
    {
        g_free (plg->y_label_text);
    }
    plg->y_label_text = g_strdup (pch_text);
}
static void lg_graph_set_chart_title (PLGRAPH plg, gchar * pch_text)
{
    g_return_if_fail (plg != NULL);

    if (plg->x_title_text != NULL)
    {
        g_free (plg->x_title_text);
    }
    plg->x_title_text = g_strdup (pch_text);
}
static void lg_graph_set_chart_window_bg_color (PLGRAPH plg, gchar * pch_color)
{
    g_return_if_fail (plg != NULL);
    g_snprintf (plg->ch_color_window_bg, sizeof (plg->ch_color_window_bg),
                "%s", pch_color);
}
static void lg_graph_set_chart_window_fg_color (PLGRAPH plg, gchar * pch_color)
{
    g_return_if_fail (plg != NULL);
    g_snprintf (plg->ch_color_chart_bg, sizeof (plg->ch_color_chart_bg), "%s", pch_color);
}
static void lg_graph_set_chart_scales_color (PLGRAPH plg, gchar * pch_color)
{
    g_return_if_fail (plg != NULL);
    g_snprintf (plg->ch_color_scale_fg, sizeof (plg->ch_color_scale_fg), "%s", pch_color);
}
static void lg_graph_set_chart_title_color (PLGRAPH plg, gchar * pch_color)
{
    g_return_if_fail (plg != NULL);
    g_snprintf (plg->ch_color_title_fg, sizeof (plg->ch_color_title_fg), "%s", pch_color);
}
static void lg_graph_redraw (PLGRAPH plg)
{
    GdkRectangle update_rect;
    GdkRegion  *region = NULL;

    g_return_if_fail (plg != NULL);

    update_rect.x = 0;
    update_rect.y = 0;
    update_rect.width = plg->drawing_area->allocation.width;
    update_rect.height = plg->drawing_area->allocation.height;

    /* --- And then draw it (calls expose event) --- */
    region = gdk_region_rectangle (&update_rect);
    gdk_window_invalidate_region (plg->drawing_area->window, region, FALSE);
    gdk_region_destroy (region);
}

/*
 * Toggle the legend function on off
 * "button-press-event"
*/
static gboolean lg_graph_button_press_event_cb (GtkWidget * widget,
                                                GdkEventButton * ev, PLGRAPH plg)
{
    g_return_val_if_fail (plg != NULL, FALSE);

    if ((ev->type & GDK_BUTTON_PRESS) && (ev->button == 1))
    {
        plg->b_tooltip_active = plg->b_tooltip_active ? FALSE : TRUE;
        lg_graph_redraw (plg);
        return TRUE;
    }
    if ((ev->type & GDK_BUTTON_PRESS) && (ev->button == 2) && plg->b_mouse_onoff)
    {
        lg_graph_debug = lg_graph_debug ? FALSE : TRUE; 
        return TRUE;        
    }

    if ((ev->type & GDK_BUTTON_PRESS) && (ev->button == 3))
    {
        plg->b_mouse_onoff = plg->b_mouse_onoff ? FALSE : TRUE; 
        return TRUE;        
    }

    return FALSE;
}

/*
 * Track the mouse pointer position
 * "motion-notify-event"
*/
static gboolean lg_graph_motion_notify_event_cb (GtkWidget * widget,
                                                 GdkEventMotion * ev, PLGRAPH plg)
{
    GdkModifierType state;
    gint        x = 0, y = 0;

    g_return_val_if_fail (plg != NULL, FALSE);

    if (ev->is_hint)
    {
        gdk_window_get_pointer (ev->window, &x, &y, &state);
    }
    else
    {
        x = ev->x;
        y = ev->y;
        state = ev->state;
    }

    plg->mouse_pos.x = x;
    plg->mouse_pos.y = y;
    plg->mouse_state = state;

    if ( lg_graph_draw_tooltip (plg) ) {
      	 lg_graph_redraw (plg);
    }

    if (lg_graph_debug)
    {
        g_print ("mouse is at x=%d, y=%d, with a state of %d\n", x, y, state);
    }

    return FALSE;
}

/*
 * Draw the chart x scale legend
*/
static void lg_graph_draw_x_grid_labels (PLGRAPH plg)
{
    gchar       ch_grid_label[GAPC_MAX_BUFFER];
    gchar       ch_work[GAPC_MAX_BUFFER];
    PangoLayout *layout = NULL;
    PangoTabArray *p_tabs = NULL;
    gint        x_adj = 0, x1_adj = 0, width = 0, height = 0, h_index = 0, x_scale = 0;

    g_return_if_fail (plg != NULL);

    g_snprintf (ch_grid_label, GAPC_MAX_BUFFER, "<small>%d</small>",
                plg->x_range.i_max_scale);
    layout = gtk_widget_create_pango_layout (plg->drawing_area, ch_grid_label);

    pango_layout_set_markup (layout, ch_grid_label, -1);
    pango_layout_get_pixel_size (layout, &width, &height);
    x_adj = width / 2;
    x1_adj = width / 4;

    g_snprintf (ch_grid_label, GAPC_MAX_BUFFER, "<small>%s", "0");
    for (h_index = plg->x_range.i_inc_major_scale_by;
         h_index <= plg->x_range.i_max_scale;
         h_index += plg->x_range.i_inc_major_scale_by)
    {
        g_strlcpy (ch_work, ch_grid_label, GAPC_MAX_BUFFER);
        g_snprintf (ch_grid_label, GAPC_MAX_BUFFER, "%s\t%d", ch_work, h_index);
        if (h_index < 10)
        {
            x_scale++;
        }
    }
    g_strlcpy (ch_work, ch_grid_label, GAPC_MAX_BUFFER);
    g_snprintf (ch_grid_label, GAPC_MAX_BUFFER, "%s</small>", ch_work);

    pango_layout_set_markup (layout, ch_grid_label, -1);

    if (lg_graph_debug)
    {
        g_print ("(%d:%d:%d)x_Labels=[%s]\n", x_adj, x1_adj, x_scale, ch_grid_label);
    }

    p_tabs = pango_tab_array_new (plg->x_range.i_num_major, TRUE);
    for (h_index = 0; h_index <= plg->x_range.i_num_major; h_index++)
    {
        gint        xbase = 0;

        if (h_index > x_scale)
        {
            xbase = (h_index * plg->x_range.i_major_inc);
        }
        else
        {
            xbase = (h_index * plg->x_range.i_major_inc) + x1_adj;
        }
        if (h_index == 0)
        {
            xbase = plg->x_range.i_major_inc + x1_adj;
        }
        pango_tab_array_set_tab (p_tabs, h_index, PANGO_TAB_LEFT, xbase);
    }
    pango_layout_set_tabs (layout, p_tabs);

    pango_layout_context_changed (layout);

    gdk_draw_layout (plg->pixmap,
                     plg->scale_gc,
                     plg->plot_box.x - x_adj,
                     plg->plot_box.y + plg->plot_box.height, layout);

    pango_tab_array_free (p_tabs);
    g_object_unref (layout);

    return;
}

/*
 * Draw the chart y scale legend
*/
static void lg_graph_draw_y_grid_labels (PLGRAPH plg)
{
    gchar       ch_grid_label[GAPC_MAX_BUFFER];
    gchar       ch_work[GAPC_MAX_BUFFER];
    PangoLayout *layout = NULL;
    gint        y_adj = 0, width = 0, height = 0, v_index = 0;

    g_return_if_fail (plg != NULL);

    g_snprintf (ch_grid_label, GAPC_MAX_BUFFER, "<small>%d</small>",
                plg->y_range.i_max_scale);
    layout = gtk_widget_create_pango_layout (plg->drawing_area, ch_grid_label);

    pango_layout_set_markup (layout, ch_grid_label, -1);
    pango_layout_get_pixel_size (layout, &width, &height);
    y_adj = height / 2;

    g_snprintf (ch_grid_label, GAPC_MAX_BUFFER, "<small>%d", plg->y_range.i_max_scale);
    for (v_index =
         plg->y_range.i_max_scale - plg->y_range.i_inc_major_scale_by;
         v_index > 0; v_index -= plg->y_range.i_inc_major_scale_by)
    {
        g_strlcpy (ch_work, ch_grid_label, GAPC_MAX_BUFFER);
        g_snprintf (ch_grid_label, GAPC_MAX_BUFFER, "%s\n%d", ch_work, v_index);
    }
    g_strlcpy (ch_work, ch_grid_label, GAPC_MAX_BUFFER);
    g_snprintf (ch_grid_label, GAPC_MAX_BUFFER, "%s</small>", ch_work);

    pango_layout_set_spacing (layout,
                              ((plg->y_range.i_major_inc - height) * PANGO_SCALE));
    pango_layout_set_alignment (layout, PANGO_ALIGN_RIGHT);
    pango_layout_set_markup (layout, ch_grid_label, -1);

    if (lg_graph_debug)
    {
        g_print ("(%d:%d)y_Labels=[%s]\n", y_adj, plg->y_range.i_major_inc,
                 ch_grid_label);
    }

    pango_layout_context_changed (layout);

    gdk_draw_layout (plg->pixmap,
                     plg->scale_gc,
                     plg->plot_box.x - (width * 1.2), plg->plot_box.y - y_adj, layout);

    g_object_unref (layout);

    return;
}

/*
 * Draws the minor and major grid lines inside the current plot_area
 * returns -1 on error, or TRUE;
*/
static gint lg_graph_draw_grid_lines (PLGRAPH plg)
{
    GtkWidget  *drawing_area = NULL;
    gint        y_minor_inc = 0, y_pos = 0, y_index = 0;
    gint        y_major_inc = 0;
    gint        x_minor_inc = 0, x_pos = 0, x_index = 0;
    gint        x_major_inc = 0;
    gint        count_major = 0, count_minor = 0;
    GdkSegment *seg_minor = NULL;
    GdkSegment *seg_major = NULL;

    g_return_val_if_fail (plg != NULL, -1);
    g_return_val_if_fail (GTK_WIDGET_DRAWABLE (plg->drawing_area), -1);

    drawing_area = plg->drawing_area;

    count_major = plg->y_range.i_num_major;
    count_minor = plg->y_range.i_num_minor;
    y_minor_inc = plg->y_range.i_minor_inc;
    y_major_inc = plg->y_range.i_major_inc;

    if (lg_graph_debug)
    {
        g_print
            ("count_major=%d, count_minor=%d, y_minor_inc=%d, y_major_inc=%d\n",
             count_major, count_minor, y_minor_inc, y_major_inc);
    }

    seg_minor = g_new0 (GdkSegment, count_minor + 8);
    seg_major = g_new0 (GdkSegment, count_major + 8);
    x_pos = plg->plot_box.width;
    y_pos = plg->plot_box.y;
    for (y_index = 0; y_index < count_minor; y_index++)
    {
        seg_minor[y_index].x1 = plg->plot_box.x;
        seg_minor[y_index].y1 = y_pos + (y_minor_inc * (y_index + 1));
        seg_minor[y_index].x2 = plg->plot_box.x + x_pos - 2;
        seg_minor[y_index].y2 = seg_minor[y_index].y1;
    }

    x_pos = plg->plot_box.width;
    y_pos = plg->plot_box.y;
    for (y_index = 0; y_index < count_major; y_index++)
    {
        seg_major[y_index].x1 = plg->plot_box.x;
        seg_major[y_index].y1 = y_pos + (y_major_inc * (y_index + 1));
        seg_major[y_index].x2 = plg->plot_box.x + x_pos - 2;
        seg_major[y_index].y2 = seg_major[y_index].y1;
    }

    gdk_gc_set_line_attributes (plg->window_gc,
                                1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_BEVEL);
    gdk_draw_segments (plg->pixmap, plg->window_gc, seg_minor, count_minor - 1);

    gdk_gc_set_line_attributes (plg->window_gc,
                                2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_BEVEL);
    gdk_draw_segments (plg->pixmap, plg->window_gc, seg_major, count_major - 1);

    g_free (seg_minor);
    g_free (seg_major);

    count_major = plg->x_range.i_num_major;
    count_minor = plg->x_range.i_num_minor;
    x_minor_inc = plg->x_range.i_minor_inc;
    x_major_inc = plg->x_range.i_major_inc;

    if (lg_graph_debug)
    {
        g_print
            ("count_major=%d, count_minor=%d, x_minor_inc=%d, x_major_inc=%d\n",
             count_major, count_minor, x_minor_inc, x_major_inc);
    }

    seg_minor = g_new0 (GdkSegment, count_minor + 8);
    seg_major = g_new0 (GdkSegment, count_major + 8);
    x_pos = plg->plot_box.x;
    y_pos = plg->plot_box.height;
    for (x_index = 0; x_index < count_minor; x_index++)
    {
        seg_minor[x_index].x1 = plg->plot_box.x + (x_minor_inc * (x_index + 1));
        seg_minor[x_index].y1 = plg->plot_box.y + 2;
        seg_minor[x_index].x2 = seg_minor[x_index].x1;
        seg_minor[x_index].y2 = plg->plot_box.y + y_pos;
    }

    x_pos = plg->plot_box.x;
    y_pos = plg->plot_box.height;
    for (x_index = 0; x_index < count_major; x_index++)
    {
        seg_major[x_index].x1 = plg->plot_box.x + (x_major_inc * (x_index + 1));
        seg_major[x_index].y1 = plg->plot_box.y + 2;
        seg_major[x_index].x2 = seg_major[x_index].x1;
        seg_major[x_index].y2 = plg->plot_box.y + y_pos;
    }

    gdk_gc_set_line_attributes (plg->window_gc,
                                1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_BEVEL);
    gdk_draw_segments (plg->pixmap, plg->window_gc, seg_minor, count_minor - 1);

    gdk_gc_set_line_attributes (plg->window_gc,
                                2, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_BEVEL);
    gdk_draw_segments (plg->pixmap, plg->window_gc, seg_major, count_major - 1);

    g_free (seg_minor);
    g_free (seg_major);

    return TRUE;
}

/*
 * Draws the tooltip legend message at top or bottom of chart
 * returns the width of the text area, or -1 on error
 * requires plg->b_tooltip_active to be TRUE, (toggled by mouse)
*/
static gint lg_graph_draw_tooltip (PLGRAPH plg)
{
    PangoLayout *layout = NULL;
    gint        x_pos = 0, y_pos = 0, width = 0, height = 0;
    gint        v_index = 0, x_adj = 0;
    PLG_SERIES  psd = NULL;
    GList      *data_sets = NULL;
    GdkRegion  *region = NULL;
    gboolean    b_found = FALSE;

    g_return_val_if_fail (plg != NULL, -1);
    g_return_val_if_fail (GTK_WIDGET_DRAWABLE (plg->drawing_area), -1);

    if (!plg->b_tooltip_active)
    {
        return -1;
    }
    if (plg->i_points_available < 1) {
        return -1;    	
    }

    /*
     * Create tooltip if needed */
    region = gdk_region_rectangle (&plg->plot_box);
    x_adj = (plg->x_range.i_minor_inc / plg->x_range.i_inc_minor_scale_by);

    /* 
     * see if ptr is at a x-range point */
    if (!gdk_region_point_in (region, plg->mouse_pos.x, plg->mouse_pos.y))
    {
        gdk_region_destroy (region);
        return -1;
    }
    gdk_region_destroy (region);

    for (v_index = 0; v_index <= plg->x_range.i_max_scale; v_index++)
    {
        x_pos = plg->plot_box.x + (v_index * x_adj);
        if ((plg->mouse_pos.x > (x_pos - (x_adj / 3))) &&
            (plg->mouse_pos.x < (x_pos + (x_adj / 3))))
        {
            if (v_index < plg->i_points_available)
            {
                b_found = TRUE;
                break;
            }
        }
    }

    /* 
     * All we needed was x, so now post a tooltip */
    if (b_found)
    {
        gchar       ch_buffer[GAPC_MAX_BUFFER];
        gchar       ch_work[GAPC_MAX_BUFFER];
        gchar       ch_time_r[GAPC_MAX_TEXT];
        gchar      *pch_time = NULL;
        time_t      point_time;

        point_time = (time_t) g_list_nth_data (plg->lg_series_time, v_index);

        pch_time = ctime_r (&point_time, ch_time_r);

        g_strdelimit (pch_time, "\n", ' ');

        g_snprintf (ch_buffer, sizeof (ch_buffer),
                    "<small>{ <u>sample #%d @ %s</u>}\n", v_index, pch_time);
        data_sets = g_list_first (plg->lg_series);
        while (data_sets)
        {
            psd = data_sets->data;
            if (psd != NULL)
            {                   /* found */
                g_snprintf (ch_work, sizeof (ch_work), "%s", ch_buffer);
                g_snprintf (ch_buffer, sizeof (ch_buffer),
                            "%s{%3.0f%% <span foreground=\"%s\">%s</span>}",
                            ch_work,
                            psd->lg_point_dvalue[v_index],
                            psd->ch_legend_color, psd->ch_legend_text);
            }
            data_sets = g_list_next (data_sets);
        }

        g_snprintf (ch_work, sizeof (ch_work), "%s", ch_buffer);
        g_snprintf (ch_buffer, sizeof (ch_buffer), "%s</small>", ch_work);
        g_snprintf (plg->ch_tooltip_text, sizeof (plg->ch_tooltip_text), "%s",
                        ch_buffer);
    }

    if (!b_found)
    {
        return -1;
    }

    layout = gtk_widget_create_pango_layout (plg->drawing_area, plg->ch_tooltip_text);
    pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);

    pango_layout_set_markup (layout, plg->ch_tooltip_text, -1);

    pango_layout_get_pixel_size (layout, &width, &height);

    x_pos = plg->x_tooltip.x + ((plg->x_tooltip.width - width) / 2);
    y_pos = plg->x_tooltip.y + ((plg->x_tooltip.height - height) / 2);

    gdk_draw_rectangle (plg->pixmap, plg->window_gc, /* box_gc, */
                        TRUE,
                        plg->x_tooltip.x,
                        plg->x_tooltip.y, plg->x_tooltip.width, plg->x_tooltip.height);
    gdk_draw_rectangle (plg->pixmap, plg->box_gc,
                        FALSE,
                        plg->x_tooltip.x,
                        plg->x_tooltip.y, plg->x_tooltip.width, plg->x_tooltip.height);

    gdk_draw_layout (plg->pixmap, plg->scale_gc, x_pos, y_pos, layout);

    g_object_unref (layout);

    if (lg_graph_debug)
    {
        g_print ("DrawToolTip: x=%d, y=%d Width=%d, Height=%d, Text=%s\n",
                 x_pos, y_pos, width, height, plg->ch_tooltip_text);
    }

    return width;
}

#if GTK_CHECK_VERSION(2,6,0)
/*
 * Draws a label text on the Y axis
 * sets the width, height values of the input rectangle to the size of textbox
 * returns the height of the text area, or -1 on error
*/
static gint lg_graph_draw_vertical_text (PLGRAPH plg,
                                         gchar * pch_text,
                                         GdkRectangle * rect, gboolean redraw_control)
{
    PangoRenderer *renderer = NULL;
    PangoMatrix matrix = PANGO_MATRIX_INIT;
    PangoContext *context = NULL;
    PangoLayout *layout = NULL;
    gint        y_pos = 0;

    g_return_val_if_fail (plg != NULL, -1);
    g_return_val_if_fail (pch_text != NULL, -1);
    g_return_val_if_fail (rect != NULL, -1);
    g_return_val_if_fail (GTK_WIDGET_DRAWABLE (plg->drawing_area), -1);

    if (rect->width && redraw_control)
    {
        gdk_draw_rectangle (plg->pixmap, plg->window_gc,
                            TRUE, rect->x, rect->y, rect->width, rect->height);
    }

    /* Get the default renderer for the screen, and set it up for drawing  */
    renderer = gdk_pango_renderer_get_default (gtk_widget_get_screen (plg->drawing_area));
    gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (renderer), plg->pixmap);
    gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (renderer), plg->title_gc);

    context = gtk_widget_get_pango_context (plg->drawing_area);

    layout = pango_layout_new (context);
    pango_layout_set_markup (layout, pch_text, -1);

    pango_matrix_rotate (&matrix, 90.0);
    pango_context_set_matrix (context, &matrix);

    pango_layout_context_changed (layout);

                             /* xy switched due to rotate func */
    pango_layout_get_pixel_size (layout, &rect->height, &rect->width);  
    y_pos = rect->y + ((plg->plot_box.height - rect->height) / 2);

    gdk_draw_layout (plg->pixmap, plg->title_gc, rect->x, y_pos, layout);

    /* Clean up default renderer, since it is shared */
    gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (renderer), NULL);
    gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (renderer), NULL);
    pango_context_set_matrix (context, NULL);

    /* free the objects we created */
    g_object_unref (layout);

    if (lg_graph_debug)
    {
        g_print ("Vertical Label: x=%d, y=%d Width=%d, Height=%d Text:%s\n",
                 rect->x, rect->y, rect->width, rect->height, pch_text);
    }

    if (redraw_control)
    {
        GdkRegion  *region = NULL;

        region = gdk_region_rectangle (rect);
        gdk_window_invalidate_region (plg->drawing_area->window, region, FALSE);
        gdk_region_destroy (region);
    }

    return rect->height;
}
#else
static gint lg_graph_draw_vertical_text (PLGRAPH plg,
                                         gchar * pch_text,
                                         GdkRectangle * rect, gboolean redraw_control)
{
    PangoContext *context = NULL;
    PangoLayout *layout = NULL;
    gint        y_pos = 0;
	GdkPixmap      *norm_pixmap = NULL;
    gint            width, height;
    gint            rot_width, rot_height;
    GdkPixbuf      *norm_pixbuf = NULL, *rot_pixbuf = NULL;
    guint32        *norm_pix, *rot_pix;
    gint            i, j, k, l;

    g_return_val_if_fail (plg != NULL, -1);
    g_return_val_if_fail (pch_text != NULL, -1);
    g_return_val_if_fail (rect != NULL, -1);
  g_return_val_if_fail (GTK_WIDGET_DRAWABLE (plg->drawing_area), -1);


  context = gtk_widget_get_pango_context (plg->drawing_area);
  layout = pango_layout_new (context);
  pango_layout_set_markup (layout, pch_text, -1);
  pango_layout_get_pixel_size (layout, &width, &height);
  if (width <= 0 || height <= 0)
  {
    return 0;
  }

  /* Figure out the rotated width and height */
  rect->width  = rot_width = height;
  rect->height = rot_height = width;

  norm_pixmap = gdk_pixmap_new (plg->drawing_area->window, width, height, -1);
  gdk_draw_rectangle (norm_pixmap, plg->window_gc, TRUE, 0, 0, width, height);
  gdk_draw_layout (norm_pixmap, plg->title_gc, 0, 0, layout);

  norm_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height); 
  norm_pixbuf = gdk_pixbuf_get_from_drawable (norm_pixbuf, norm_pixmap, NULL, 
          									  0, 0, 0, 0, width, height);

  /* Get the raw pixel pointer of client buffer */
  norm_pix = (guint32 *) gdk_pixbuf_get_pixels (norm_pixbuf);
  
  /* Allocate a new client buffer with rotated memory */
  rot_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, rot_width, rot_height);
  rot_pix = (guint32 *) gdk_pixbuf_get_pixels (rot_pixbuf);

  /* Actually rotate */
  k = 0;
  for (j = width - 1; j >= 0; j--)
  {
       l = j;
       for (i = 0; i < height; i++, k++, l += width)
       {
          rot_pix[k] = norm_pix[l];
       }
  }

  /* compute a centered position on chart */
  y_pos = rect->y + ((plg->plot_box.height - rect->height) / 2);

  /* Draw it to the chart */
  gdk_pixbuf_render_to_drawable ( rot_pixbuf,
                        plg->pixmap,
                        plg->title_gc,                        
                        0, 0,
                        rect->x -1, y_pos,
                        rect->width, rect->height,
                        GDK_RGB_DITHER_NONE, 0, 0);

  /* Free everything */
  g_object_unref (layout);
  g_object_unref (G_OBJECT (norm_pixmap));
  g_object_unref (G_OBJECT (norm_pixbuf));
  g_object_unref (G_OBJECT (rot_pixbuf));   


  return rect->height;
}
#endif

/*
 * Draws a label text on the X axis
 * sets the width, height values of the input rectangle to the size of textbox
 * returns the width of the text area, or -1 on error
 * redraw_control = 1 causes an expose_event, 0 or != 1 does not
*/
static gint lg_graph_draw_horizontal_text (PLGRAPH plg,
                                           gchar * pch_text,
                                           GdkRectangle * rect, gboolean redraw_control)
{
    PangoLayout *layout = NULL;
    gint        x_pos = 0;

    g_return_val_if_fail (plg != NULL, -1);
    g_return_val_if_fail (pch_text != NULL, -1);
    g_return_val_if_fail (rect != NULL, -1);
    g_return_val_if_fail (GTK_WIDGET_DRAWABLE (plg->drawing_area), -1);

    if (rect->width && redraw_control)
    {
        gdk_draw_rectangle (plg->pixmap, plg->window_gc,
                            TRUE, rect->x, rect->y, rect->width, rect->height);
    }

    layout = gtk_widget_create_pango_layout (plg->drawing_area, pch_text);
    pango_layout_set_markup (layout, pch_text, -1);
    pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);

    pango_layout_get_pixel_size (layout, &rect->width, &rect->height);
    x_pos = rect->x + ((plg->plot_box.width - rect->width) / 2);

    gdk_draw_layout (plg->pixmap, plg->title_gc, x_pos, rect->y, layout);

    g_object_unref (layout);

    if (lg_graph_debug)
    {
        g_print ("Horizontal Label: x=%d, y=%d Width=%d, Height=%d Text:%s\n",
                 x_pos, rect->y, rect->width, rect->height, pch_text);

    }

    if (redraw_control)
    {
        GdkRegion  *region = NULL;

        region = gdk_region_rectangle (rect);
        gdk_window_invalidate_region (plg->drawing_area->window, region, FALSE);
        gdk_region_destroy (region);
    }

    return rect->width;
}

/*
 * Computes the size of 3 proportional charactor using default font
*/
static void lg_graph_get_default_sizes (PLGRAPH plg, gint * width, gint * height)
{
    PangoLayout *layout = NULL;

    g_return_if_fail (plg != NULL);

    layout = gtk_widget_create_pango_layout (plg->drawing_area, "1M5");

    pango_layout_set_markup (layout, "<big><b>M5</b></big>", -1);

    pango_layout_get_pixel_size (layout, width, height);

    g_object_unref (layout);

    if (lg_graph_debug)
    {
        g_print ("Default Sizing(1M5): Width=%d, Height=%d\n", *width, *height);
    }

    return;
}

/*
 * Compute and set x-y ranges 
*/
static void lg_graph_set_ranges (PLGRAPH plg,
                                 gint xminor_by,
                                 gint xmajor_by,
                                 gint x_min,
                                 gint x_max,
                                 gint yminor_by, gint ymajor_by, gint y_min, gint y_max)
{
    g_return_if_fail (plg != NULL);

    plg->x_range.i_inc_minor_scale_by = xminor_by;  /* minimum scale value - ex:   0 */
    plg->x_range.i_inc_major_scale_by = xmajor_by;  /* minimum scale value - ex:   0 */

    plg->x_range.i_min_scale = x_min;   /* minimum scale value - ex:   0 */
    plg->x_range.i_max_scale = x_max;   /* maximum scale value - ex: 100 */
    plg->x_range.i_num_minor = x_max / xminor_by;   /* number of minor points */
    plg->x_range.i_num_major = x_max / xmajor_by;   /* number of major points */

    plg->y_range.i_inc_minor_scale_by = yminor_by;  /* minimum scale value - ex:   0 */
    plg->y_range.i_inc_major_scale_by = ymajor_by;  /* minimum scale value - ex:   0 */

    plg->y_range.i_min_scale = y_min;   /* minimum scale value - ex:   0 */
    plg->y_range.i_max_scale = y_max;   /* maximum scale value - ex: 100 */
    plg->y_range.i_num_minor = y_max / yminor_by;   /* number of minor points */
    plg->y_range.i_num_major = y_max / ymajor_by;   /* number of major points */

    return;
}

/*
 * Repaint
 *
 * data - widget to repaint
 */
static gint lg_graph_draw (PLGRAPH plg)
{
    GtkWidget  *drawing_area = NULL;

    g_return_val_if_fail (plg != NULL, TRUE);

    drawing_area = plg->drawing_area;

    if ( !(GTK_WIDGET_DRAWABLE (drawing_area)) ) {
        return TRUE;
    }

    /* 
     * Clear the whole area
     */
    gdk_draw_rectangle (plg->pixmap, plg->window_gc,
                        TRUE, 0, 0,
                        plg->drawing_area->allocation.width,
                        plg->drawing_area->allocation.height);

    /* 
     * draw plot area
     */
    gdk_draw_rectangle (plg->pixmap,
                        plg->box_gc,
                        TRUE,
                        plg->plot_box.x,
                        plg->plot_box.y, plg->plot_box.width, plg->plot_box.height);

    gdk_gc_set_line_attributes (plg->drawing_area->style->black_gc,
                                2, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_BEVEL);
    gdk_draw_rectangle (plg->pixmap,
                        plg->drawing_area->style->black_gc,
                        FALSE,
                        plg->plot_box.x,
                        plg->plot_box.y, plg->plot_box.width, plg->plot_box.height);

    if (lg_graph_debug)
    {
        g_print
            ("Window: Width=%d, Height=%d, Plot Area x=%d y=%d width=%d, height=%d\n",
             drawing_area->allocation.width, drawing_area->allocation.height,
             plg->plot_box.x, plg->plot_box.y, plg->plot_box.width, plg->plot_box.height);
    }

    /*
     * draw titles 
     */
    lg_graph_draw_horizontal_text (plg, plg->x_title_text, &plg->x_title, FALSE);

    lg_graph_draw_horizontal_text (plg, plg->x_label_text, &plg->x_label, FALSE);

    lg_graph_draw_vertical_text (plg, plg->y_label_text, &plg->y_label, FALSE);

    lg_graph_draw_grid_lines (plg);

    lg_graph_draw_x_grid_labels (plg);

    lg_graph_draw_y_grid_labels (plg);

    lg_graph_data_series_draw_all (plg, FALSE);

    lg_graph_draw_tooltip (plg);

    /* The entire pixmap is going to be copied
     *     onto the window so the rect is configured 
     *     as the size of the window.
     */
    lg_graph_redraw (plg);

    return (FALSE);
}

/* 
 * configure_event
 *
 * Create a new backing pixmap of the appropriate size 
 * Of course, this is called whenever the window is 
 * resized.  We have to free up things we allocated.
 */
static gint lg_graph_configure_event_cb (GtkWidget * widget,
                                         GdkEventConfigure * event, PLGRAPH plg)
{
    GdkRectangle clip_area;
    gint        xfactor = 0, yfactor = 0;

    /* --- Free background if we created it --- */
    if (plg->pixmap)
    {
        gdk_pixmap_unref (plg->pixmap);
    }

    /* --- Create a new pixmap with new size --- */
    plg->pixmap = gdk_pixmap_new (widget->window,
                                  widget->allocation.width,
                                  widget->allocation.height, -1);

    gdk_draw_rectangle (plg->pixmap, plg->window_gc,
                        TRUE, 0, 0, widget->allocation.width, widget->allocation.height);

    plg->width = widget->allocation.width;
    plg->height = widget->allocation.height;

    clip_area.x = 0;
    clip_area.y = 0;
    clip_area.width = widget->allocation.width;
    clip_area.height = widget->allocation.height;

    xfactor = MAX (plg->x_range.i_num_minor, plg->x_range.i_num_major);
    yfactor = MAX (plg->y_range.i_num_minor, plg->y_range.i_num_major);

    lg_graph_get_default_sizes (plg, &plg->x_border, &plg->y_border);
    plg->x_border /= 4;
    plg->y_border /= 4;

    plg->x_label.x = plg->x_border * 6; /* define top-left corner of textbox */
    plg->x_label.y = plg->height - (plg->y_border * 4) + 2;

    plg->y_label.x = plg->x_border;
    plg->y_label.y = plg->y_border * 6;

    plg->x_title.x = plg->x_border * 6;
    plg->x_title.y = 1;         /* /plg->y_border ; */

    plg->x_tooltip.x = plg->x_border;
    plg->x_tooltip.y = plg->y_border;
    plg->x_tooltip.width = plg->width - (plg->x_border * 2);
    plg->x_tooltip.height = plg->y_border * 7;

    plg->plot_box.x = plg->x_border * 6;
    plg->plot_box.y = plg->y_border * 6;
    plg->plot_box.width =
        ((gint) (plg->width - (plg->x_border * 10)) / xfactor) * xfactor;
    plg->plot_box.height =
        ((gint) (plg->height - (plg->y_border * 14)) / yfactor) * yfactor;

    /* reposition the box according to scale-able increments */
    plg->plot_box.x = (((gfloat) (plg->width - plg->plot_box.width) / 10.0) * 7) + 4;
    plg->plot_box.y = (((gfloat) (plg->height - plg->plot_box.height) / 10.0) * 5) + 4;
    plg->x_label.x = plg->x_title.x = plg->plot_box.x;
    plg->y_label.y = plg->plot_box.y;

    plg->y_range.i_minor_inc = plg->plot_box.height / plg->y_range.i_num_minor;
    plg->y_range.i_major_inc = plg->plot_box.height / plg->y_range.i_num_major;

    plg->x_range.i_minor_inc = plg->plot_box.width / plg->x_range.i_num_minor;
    plg->x_range.i_major_inc = plg->plot_box.width / plg->x_range.i_num_major;

    g_timeout_add (250, (GSourceFunc) lg_graph_draw, plg);

    return TRUE;
}

/*
 * expose_event
 *
 * When the window is exposed to the viewer or 
 * the gdk_widget_draw routine is called, this 
 * routine is called.  Copies the background pixmap
 * to the window.
 */
static gint lg_graph_expose_event_cb (GtkWidget * widget, GdkEventExpose * event,
                                      PLGRAPH plg)
{

    g_return_val_if_fail (GDK_IS_DRAWABLE (widget->window), FALSE);

    /* --- Copy pixmap to the window --- */
    gdk_draw_pixmap (widget->window,
                     widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                     plg->pixmap,
                     event->area.x, event->area.y,
                     event->area.x, event->area.y, event->area.width, event->area.height);

    return FALSE;
}

static void cb_util_popup_menu_response_exit(GtkWidget * widget, gpointer gp)
{
   PGAPC_CONFIG pcfg = NULL;
   PGAPC_MONITOR pm = NULL;
   gchar *penabled = NULL;

   g_return_if_fail(gp != NULL);

   if (((PGAPC_MONITOR) gp)->cb_id == CB_MONITOR_ID) {
      /* this is a monitor struct (2) */
      pm = (PGAPC_MONITOR) gp;
      pm->b_run = FALSE;
      penabled = g_strdup_printf(GAPC_ENABLE_KEY, pm->cb_monitor_num);
      gconf_client_set_bool(pm->client, penabled, FALSE, NULL);
      g_free(penabled);
   } else {
      /* this is a config struct (1) */
      pcfg = (PGAPC_CONFIG) gp;
      gtk_widget_destroy(GTK_WIDGET(pcfg->window));
   }

   return;
}
static void cb_util_popup_menu_response_jumpto(GtkWidget * widget, gpointer gp)
{
   PGAPC_CONFIG pcfg = NULL;
   PGAPC_MONITOR pm = NULL;
   GtkWindow *window = NULL;

   g_return_if_fail(gp != NULL);

   if (((PGAPC_MONITOR) gp)->cb_id == CB_MONITOR_ID) {
      /* this is a monitor struct (2) */
      pm = (PGAPC_MONITOR) gp;
      window = GTK_WINDOW(pm->window);
   } else {
      /* this is a config struct (1) */
      pcfg = (PGAPC_CONFIG) gp;
      window = GTK_WINDOW(pcfg->window);
   }

   if (window != NULL) {
      gtk_window_present(window);
   }

   return;
}

/*
 * Change the color values back to their original defaults
 */
static void cb_panel_property_color_reset (GtkButton *button, PGAPC_CONFIG pcfg)
{
  gchar *pstring = NULL;

  g_return_if_fail (pcfg != NULL);

  gdk_color_parse ("green", &pcfg->color_linev);   	
  pstring = gtk_color_selection_palette_to_string ( &pcfg->color_linev, 1);
  if (pstring != NULL) {
	  gconf_client_set_string(pcfg->client, GAPC_COLOR_LINEV_KEY, pstring, NULL);
	  g_free (pstring);
  }

  gdk_color_parse ("blue", &pcfg->color_loadpct);   	
  pstring = gtk_color_selection_palette_to_string ( &pcfg->color_loadpct, 1);
  if (pstring != NULL) {
	  gconf_client_set_string(pcfg->client, GAPC_COLOR_LOADPCT_KEY, pstring, NULL);
	  g_free (pstring);
  }

  gdk_color_parse ("red", &pcfg->color_timeleft);   	
  pstring = gtk_color_selection_palette_to_string ( &pcfg->color_timeleft, 1);
  if (pstring != NULL) {
	  gconf_client_set_string(pcfg->client, GAPC_COLOR_TIMELEFT_KEY, pstring, NULL);
	  g_free (pstring);
  }

  gdk_color_parse ("yellow", &pcfg->color_bcharge);   	
  pstring = gtk_color_selection_palette_to_string ( &pcfg->color_bcharge, 1);
  if (pstring != NULL) {
	  gconf_client_set_string(pcfg->client, GAPC_COLOR_BCHARGE_KEY, pstring, NULL);
	  g_free (pstring);
  }

  gdk_color_parse ("black", &pcfg->color_battv);   	
  pstring = gtk_color_selection_palette_to_string ( &pcfg->color_battv, 1);
  if (pstring != NULL) {
	  gconf_client_set_string(pcfg->client, GAPC_COLOR_BATTV_KEY, pstring, NULL);
	  g_free (pstring);
  }

  gdk_color_parse ("white", &pcfg->color_window);   	
  pstring = gtk_color_selection_palette_to_string ( &pcfg->color_window, 1);
  if (pstring != NULL) {
	  gconf_client_set_string(pcfg->client, GAPC_COLOR_WINDOW_KEY, pstring, NULL);
	  g_free (pstring);
  }

  gdk_color_parse ("light blue", &pcfg->color_chart);   	
  pstring = gtk_color_selection_palette_to_string ( &pcfg->color_chart, 1);
  if (pstring != NULL) {
	  gconf_client_set_string(pcfg->client, GAPC_COLOR_CHART_KEY, pstring, NULL);
	  g_free (pstring);
  }

  gdk_color_parse ("blue", &pcfg->color_title);   	
  pstring = gtk_color_selection_palette_to_string ( &pcfg->color_title, 1);
  if (pstring != NULL) {
	  gconf_client_set_string(pcfg->client, GAPC_COLOR_TITLE_KEY, pstring, NULL);
	  g_free (pstring);
  }
  
  return;
}
/*
 * catch the color change signal and save its value into gconf
 */
static void cb_panel_property_color_change (GtkColorButton *widget, gchar *color_key)
{
  GConfClient *client = NULL;
  GdkColor  color;
  gchar     *pstring = NULL;
  
  g_return_if_fail(GTK_IS_COLOR_BUTTON(widget));
  g_return_if_fail(color_key != NULL);
  
  gtk_color_button_get_color (GTK_COLOR_BUTTON(widget), &color); 
  pstring = gtk_color_selection_palette_to_string ( &color, 1);
  if (pstring == NULL) {
  	  return;
  }
  
  client = (GConfClient *)g_object_get_data (G_OBJECT(widget), "gconf-client");
  if (client != NULL) {
   	  gconf_client_set_string (client, color_key, pstring, NULL);
  }
  
  g_free (pstring);
                                               
  return;
}
/*
 * Changes the Applets icon if needed
 * returns FALSE if OK
 * return TRUE is any error
*/
static gint gapc_util_change_icons(PGAPC_MONITOR pm)
{
   GdkPixbuf *pixbuf = NULL;
   GdkPixbuf *scaled = NULL;
   GtkOrientation orientation;
   gint size = 0;

   g_return_val_if_fail(pm != NULL, TRUE);

   if (pm->i_icon_index >= GAPC_N_ICONS) {
      pm->i_icon_index = GAPC_ICON_ONLINE;
   }

   pixbuf = pm->my_icons[pm->i_icon_index];
   if (pixbuf) {
      if (pm->tray_image != NULL) {
         orientation = egg_tray_icon_get_orientation(EGG_TRAY_ICON(pm->tray_icon));
         if (orientation == GTK_ORIENTATION_HORIZONTAL) {
            size = pm->i_icon_height;
         } else {
            size = pm->i_icon_width;
         }
         if (size < 5) {
            size = 22;
         }
         scaled = gdk_pixbuf_scale_simple(pixbuf, size, size, GDK_INTERP_BILINEAR);
         gtk_image_set_from_pixbuf(GTK_IMAGE(pm->tray_image), scaled);
         gtk_widget_show(pm->tray_image);
         gdk_pixbuf_unref(scaled);
      }

      if (pm->window != NULL)
         gtk_window_set_icon(GTK_WINDOW(pm->window), pixbuf);
   }

   return FALSE;
}

/*
 * used to switch timers when d_refresh changes
 * b_timer_control True trigger the target timer to stop and have this one
 * restart it; so it can pickup the new interval;
*/
static gboolean cb_util_line_chart_refresh_control(PGAPC_MONITOR pm)
{
   GtkWidget *w = NULL;
   gchar *pch1 = NULL, *pch = NULL;

   g_return_val_if_fail(pm != NULL, FALSE);

   if ((!pm->b_run) || !(pm->cb_enabled))
      return FALSE;                /* stop timers */

   pm->b_graph_control = FALSE;
   pm->phs.d_xinc = pm->d_graph * pm->d_refresh;

   pm->tid_graph_refresh =
      g_timeout_add((guint) (pm->phs.d_xinc * GAPC_REFRESH_FACTOR_1K ),
      (GSourceFunc) cb_util_line_chart_refresh, &pm->phs);

   pch = g_strdup_printf(
                 "<i>sampled every %3.1f seconds</i>", 
                 pm->phs.d_xinc);
   lg_graph_set_x_label_text (pm->phs.plg, pch);

   g_free(pch);

   w = g_hash_table_lookup(pm->pht_Widgets, "StatusBar");

   if ((pm->tid_graph_refresh != 0) && (w != NULL)) {
      gtk_statusbar_pop(GTK_STATUSBAR(w), pm->i_info_context);
      pch1 = g_strdup_printf
         ("Graphing refresh cycle changed for host %s completed!...", pm->pch_host);
      gtk_statusbar_push(GTK_STATUSBAR(w), pm->i_info_context, pch1);
      g_free(pch1);

   }

   return FALSE;
}

/*
 * used to switch timers when d_refresh changes
 * b_timer_control True trigger the target timer to stop and have this one
 * restart it; so it can pickup the new interval;
*/
static gboolean cb_monitor_refresh_control(PGAPC_MONITOR pm)
{
   GtkWidget *w = NULL;
   gchar *pch1 = NULL;

   g_return_val_if_fail(pm != NULL, FALSE);

   if ((!pm->b_run) || !(pm->cb_enabled))
      return FALSE;                /* stop timers */

   pm->b_timer_control = FALSE;

   pm->tid_automatic_refresh =
      g_timeout_add((guint) (pm->d_refresh * GAPC_REFRESH_FACTOR_1K),
      (GSourceFunc) cb_monitor_automatic_refresh, pm);

   w = g_hash_table_lookup(pm->pht_Widgets, "StatusBar");

   if ((pm->tid_automatic_refresh != 0) && (w != NULL)) {
      gtk_statusbar_pop(GTK_STATUSBAR(w), pm->i_info_context);
      pch1 = g_strdup_printf("Refresh Cycle Change for host %s Completed!...",
         pm->pch_host);
      gtk_statusbar_push(GTK_STATUSBAR(w), pm->i_info_context, pch1);
      g_free(pch1);
   }

   return FALSE;
}

/*
 * timer service routine for IPL refresh and refresh_button.
 * used to overcome the multi-threaded startup delay.  very short
 */
static gboolean cb_monitor_dedicated_one_time_refresh(PGAPC_MONITOR pm)
{
   GtkWidget *w = NULL;
   gchar *pch1 = NULL;

   g_return_val_if_fail(pm != NULL, FALSE);

   if ((!pm->b_run) || !(pm->cb_enabled)) {
      return FALSE;
   }

   if (!g_mutex_trylock(pm->gm_update)) {
      w = g_hash_table_lookup(pm->pht_Widgets, "StatusBar");
      if (w != NULL) {
         gtk_statusbar_pop(GTK_STATUSBAR(w), pm->i_info_context);
         pch1 = g_strdup_printf("Quick refresh for %s failed!"
            " Network thread is busy...", pm->pch_host);
         gtk_statusbar_push(GTK_STATUSBAR(w), pm->i_info_context, pch1);
         g_free(pch1);
      }
      return TRUE;                 /* thread must be busy */
   }

   gdk_threads_enter();

   w = g_hash_table_lookup(pm->pht_Widgets, "StatusBar");
   if (w != NULL) {
      gtk_statusbar_pop(GTK_STATUSBAR(w), pm->i_info_context);
   }

   gapc_monitor_update_tooltip_msg(pm);

   if (!gapc_monitor_update(pm)) {
      if (w != NULL) {
         pch1 = g_strdup_printf("Refresh for %s failed! "
            "(retry enabled)... network busy!", pm->pch_host);
         gtk_statusbar_push(GTK_STATUSBAR(w), pm->i_info_context, pch1);
         g_free(pch1);
      }
      if (pm->i_netbusy_counter++ % 10) {       /* Fall thru and quit after ten trys */
         g_mutex_unlock(pm->gm_update);
         gdk_flush();
         gdk_threads_leave();
         return TRUE;              /* try again */
      }
   }

   if (w != NULL) {
      pch1 = g_strdup_printf("One-Time Refresh for %s Completed...", pm->pch_host);
      gtk_statusbar_push(GTK_STATUSBAR(w), pm->i_info_context, pch1);
      g_free(pch1);
   }

   g_mutex_unlock(pm->gm_update);
   gdk_flush();
   gdk_threads_leave();

   return FALSE;                   /* this will terminate the timer */
}

/*
 * used to make a work request to network queue
*/
static gboolean cb_monitor_automatic_refresh(PGAPC_MONITOR pm)
{
   gchar *pch1 = NULL;
   GtkWidget *w = NULL;

   g_return_val_if_fail(pm != NULL, FALSE);

   if ((!pm->b_run) || !(pm->cb_enabled))
      return FALSE;                /* stop timers */

   if (pm->b_timer_control) {
      g_timeout_add(100, (GSourceFunc) cb_monitor_refresh_control, pm);
      return FALSE;
   }

   if (!g_mutex_trylock(pm->gm_update)) {
      w = g_hash_table_lookup(pm->pht_Widgets, "StatusBar");
      if (w != NULL) {
         gtk_statusbar_pop(GTK_STATUSBAR(w), pm->i_info_context);
         pch1 = g_strdup_printf("Automatic refresh for %s failed!"
            " Network thread is busy...", pm->pch_host);
         gtk_statusbar_push(GTK_STATUSBAR(w), pm->i_info_context, pch1);
         g_free(pch1);
      }
      return TRUE;                 /* thread must be busy */
   }

   gdk_threads_enter();

   w = g_hash_table_lookup(pm->pht_Widgets, "StatusBar");
   if (w != NULL) {
      gtk_statusbar_pop(GTK_STATUSBAR(w), pm->i_info_context);
   }

   gapc_monitor_update_tooltip_msg(pm); /* false = OK */
   if (gapc_monitor_update(pm)) {
      if (w != NULL) {
         pch1 =
            g_strdup_printf("Automatic refresh for %s complete...", pm->pch_host);
         gtk_statusbar_push(GTK_STATUSBAR(w), pm->i_info_context, pch1);
         g_free(pch1);
      }
   } else {
      if (w != NULL) {
         pch1 = g_strdup_printf("Automatic refresh for %s failed!"
            " Network thread is busy...", pm->pch_host);
         gtk_statusbar_push(GTK_STATUSBAR(w), pm->i_info_context, pch1);
         g_free(pch1);
      }
      if (pm->i_netbusy_counter++ % 10) {       /* fall thru every tenth time to queue a message */
         g_mutex_unlock(pm->gm_update);
         gdk_flush();
         gdk_threads_leave();

         return TRUE;
      }
   }

   /*
    * This is the work request to network queue */
   g_async_queue_push(pm->q_network, (gpointer) pm);

   g_mutex_unlock(pm->gm_update);
   gdk_flush();
   gdk_threads_leave();

   return TRUE;
}

/*
 * Manage the state icon in the panel and the associated tooltip
 * Composes the expanded tooltip message
 */
static gboolean gapc_monitor_update_tooltip_msg(PGAPC_MONITOR pm)
{
   gchar *pchx = NULL, *pmsg = NULL, *ptitle = NULL, *pch5 = NULL, *pch5a =
      NULL, *pch5b = NULL;
   gchar *pch1 = NULL, *pch2 = NULL, *pch3 = NULL, *pch4 = NULL;
   gchar *pch6 = NULL, *pch7 = NULL, *pch8 = NULL, *pch9 = NULL;
   gchar *pchb = NULL, *pchc = NULL, *pchd = NULL, *pche = NULL;
   gchar *pcha = NULL, *pmview = NULL, *pch_watt = NULL;
   GtkWidget *w = NULL;
   gdouble d_value = 0.0, d_watt =0.0, d_loadpct = 0.0;
   gboolean b_flag = FALSE, b_valid = FALSE;
   gint i_series = 0;
   GtkTreeIter miter;
   GdkPixbuf *pixbuf;

   g_return_val_if_fail(pm != NULL, TRUE);

   if (pm->b_run != TRUE)
      return TRUE;

   w = g_hash_table_lookup(pm->pht_Widgets, "StatusBar");

   pm->i_icon_index = GAPC_ICON_ONLINE;

   pch1 = g_hash_table_lookup(pm->pht_Status, "UPSNAME");
   pch2 = g_hash_table_lookup(pm->pht_Status, "HOSTNAME");
   if (pch2 == NULL) {
      pch2 = pm->pch_host;
   }
   if (pch2 == NULL) {
      pch2 = "unknown";
   }
   pch3 = g_hash_table_lookup(pm->pht_Status, "STATUS");
   if (pch3 == NULL) {
      pch3 = "NISERR";
   }
   pch4 = g_hash_table_lookup(pm->pht_Status, "NUMXFERS");
   pch5a = g_hash_table_lookup(pm->pht_Status, "TONBATT");
   if (pch5a == NULL) {
      pch5a = " ";
   }
   pch5b = g_hash_table_lookup(pm->pht_Status, "CUMONBATT");
   if (pch5b == NULL) {
      pch5b = " ";
   }
   pch5 = g_hash_table_lookup(pm->pht_Status, "XONBATT");
   if (pch5 == NULL) {
      pch5 = " ";
   }
   pch6 = g_hash_table_lookup(pm->pht_Status, "LINEV");
   pch7 = g_hash_table_lookup(pm->pht_Status, "BCHARGE");
   if (pch7 == NULL) {
      pch7 = "n/a";
   }
   pch8 = g_hash_table_lookup(pm->pht_Status, "LOADPCT");
   if ( pch8 != NULL ) {
     d_loadpct = g_strtod (pch8, NULL);
     d_loadpct /= 100.0;
   }
   pch_watt = g_hash_table_lookup(pm->pht_Status, "NOMPOWER");
   if ( pch_watt != NULL ) {
     pm->i_watt = g_strtod (pch_watt, NULL);
     d_watt = d_loadpct * pm->i_watt;
   } else {
     d_watt = d_loadpct * pm->i_watt;
   }

   pch9 = g_hash_table_lookup(pm->pht_Status, "TIMELEFT");
   pcha = g_hash_table_lookup(pm->pht_Status, "VERSION");
   pchb = g_hash_table_lookup(pm->pht_Status, "STARTTIME");
   pchc = g_hash_table_lookup(pm->pht_Status, "MODEL");
   pchd = g_hash_table_lookup(pm->pht_Status, "UPSMODE");
   pche = g_hash_table_lookup(pm->pht_Status, "CABLE");

   if (pm->b_data_available) {
      d_value = g_strtod(pch7, NULL);
      pchx = NULL;
      if (g_strrstr(pch3, "COMMLOST") != NULL) {
         pchx = " cable un-plugged...";
         pm->i_icon_index = GAPC_ICON_UNPLUGGED;
         b_flag = TRUE;
      } else if ((d_value < 99.0) && (g_strrstr(pch3, "LINE") != NULL)) {
         pchx = " and charging...";
         pch3 = "CHARGING";
         pm->i_icon_index = GAPC_ICON_CHARGING;
      } else if (g_strrstr(pch3, "BATT") != NULL) {
         pchx = " on battery...";
         pm->i_icon_index = GAPC_ICON_ONBATT;
      }
   } else {
      b_flag = TRUE;
      pchx = " NIS network error...";
      pch3 = "NISERR";
      g_hash_table_replace(pm->pht_Status, g_strdup("STATUS"), g_strdup(pch3));
      pm->i_icon_index = GAPC_ICON_NETWORKERROR;
      for (i_series = 0; i_series < pm->phs.plg->i_num_series; i_series++) {
         gapc_util_point_filter_set(&(pm->phs.sq[i_series]), 0.0);
      }
   }

   if (b_flag) {
      ptitle = g_strdup_printf("<span foreground=\"red\" size=\"large\">"
         "%s@%s\nis %s%s" "</span>",
         (pch1 != NULL) ? pch1 : "unknown",
         (pch2 != NULL) ? pch2 : "unknown",
         (pch3 != NULL) ? pch3 : "n/a", (pchx != NULL) ? pchx : " ");
   } else {
      ptitle = g_strdup_printf("<span foreground=\"blue\" size=\"large\">"
         "%s@%s\nis %s%s" "</span>",
         (pch1 != NULL) ? pch1 : "unknown",
         (pch2 != NULL) ? pch2 : "unknown",
         (pch3 != NULL) ? pch3 : "n/a", (pchx != NULL) ? pchx : " ");
   }

   pmsg = g_strdup_printf("%s@%s\nStatus: %s%s\n"
      "Refresh occurs every %3.1f seconds\n"
      "----------------------------------------------------------\n"
      "%s Outage[s]\n" "Last on %s\n" "%s Utility VAC\n"
      "%s Battery Charge\n" 
      "%s UPS Load\n" 
      "%3.0f of %d watts\n"
      "%s Remaining\n"
      "----------------------------------------------------------\n"
      "Build: %s\n" "Started: %s\n"
      "----------------------------------------------------------\n"
      "Model: %s\n" " Mode: %s\n" "Cable: %s",
      (pch1 != NULL) ? pch1 : "unknown",
      (pch2 != NULL) ? pch2 : "unknown",
      (pch3 != NULL) ? pch3 : "n/a",
      (pchx != NULL) ? pchx : " ",
      pm->d_refresh,
      (pch4 != NULL) ? pch4 : "n/a",
      (pch5 != NULL) ? pch5 : "n/a",
      (pch6 != NULL) ? pch6 : "n/a",
      (pch7 != NULL) ? pch7 : "n/a",
      (pch8 != NULL) ? pch8 : "n/a",
      d_watt, pm->i_watt,
      (pch9 != NULL) ? pch9 : "n/a",
      (pcha != NULL) ? pcha : "n/a",
      (pchb != NULL) ? pchb : "n/a",
      (pchc != NULL) ? pchc : "n/a",
      (pchd != NULL) ? pchd : "n/a", (pche != NULL) ? pche : "n/a");


   switch (pm->i_icon_index) {
   case GAPC_ICON_NETWORKERROR:
      pmview = g_strdup_printf("<span foreground=\"red\" size=\"large\">"
         "<b><i>%s@%s</i></b></span>\n"
         "NIS network connection not Responding!",
         (pch1 != NULL) ? pch1 : "unknown", (pch2 != NULL) ? pch2 : "unknown");
      break;
   case GAPC_ICON_UNPLUGGED:
      pmview = g_strdup_printf("<span foreground=\"red\" size=\"large\">"
         "<b><i>%s@%s</i></b></span>\n"
         "%s",
         (pch1 != NULL) ? pch1 : "unknown",
         (pch2 != NULL) ? pch2 : "unknown", (pchx != NULL) ? pchx : " un-plugged");
      break;
   case GAPC_ICON_CHARGING:
      pmview = g_strdup_printf("<span foreground=\"blue\">"
         "<b><i>%s@%s</i></b></span>\n"
         "%s Outage, Last on %s\n"
         "%s VAC, %s Charge\n"
         "%s Remaining, %s total on battery, %3.0f of %d watts",
         (pch1 != NULL) ? pch1 : "unknown",
         (pch2 != NULL) ? pch2 : "unknown",
         (pch4 != NULL) ? pch4 : "n/a",
         (pch5 != NULL) ? pch5 : "n/a",
         (pch6 != NULL) ? pch6 : "n/a",
         (pch7 != NULL) ? pch7 : "n/a",
         (pch9 != NULL) ? pch9 : "n/a", 
         (pch5b != NULL) ? pch5b : "n/a",
         d_watt, pm->i_watt);
      break;
   case GAPC_ICON_ONBATT:
      pmview = g_strdup_printf("<span foreground=\"yellow\">"
         "<b><i>%s@%s</i></b></span>\n"
         "%s Outage, Last on %s\n"
         "%s Charge, %s total on battery\n"
         "%s Remaining, %s on battery, %3.0f of %d watts",
         (pch1 != NULL) ? pch1 : "unknown",
         (pch2 != NULL) ? pch2 : "unknown",
         (pch4 != NULL) ? pch4 : "n/a",
         (pch5 != NULL) ? pch5 : "n/a",
         (pch7 != NULL) ? pch7 : "n/a",
         (pch5b != NULL) ? pch5b : "n/a",
         (pch9 != NULL) ? pch9 : "n/a", 
         (pch5a != NULL) ? pch5a : "n/a ",
         d_watt, pm->i_watt);
      break;
   case GAPC_ICON_ONLINE:
   case GAPC_ICON_DEFAULT:
   default:
      pmview = g_strdup_printf("<b><i>%s@%s</i></b>\n"
         "%s Outage, Last on %s\n"
         "%s VAC, %s Charge %s %3.0f of %d watts",
         (pch1 != NULL) ? pch1 : "unknown",
         (pch2 != NULL) ? pch2 : "unknown",
         (pch4 != NULL) ? pch4 : "n/a",
         (pch5 != NULL) ? pch5 : "n/a",
         (pch6 != NULL) ? pch6 : "n/a",
         (pch7 != NULL) ? pch7 : "n/a", 
         (pchx != NULL) ? pchx : " ",
         d_watt, pm->i_watt);
      break;
   }

   pixbuf = pm->my_icons[pm->i_icon_index];
   if (pm->i_old_icon_index != pm->i_icon_index) {
      b_flag = TRUE;
   } else {
      b_flag = FALSE;
   }

   if ((pm->tooltips != NULL) && (pm->tray_icon != NULL)) {
      gtk_tooltips_set_tip(pm->tooltips, GTK_WIDGET(pm->tray_icon), pmsg, NULL);
      if (b_flag) {
         gapc_util_change_icons(pm);
      }
   }

   b_valid =
      gapc_util_treeview_get_iter_from_monitor(pm->monitor_model, &miter,
      pm->cb_monitor_num);
   if (b_valid) {
      if (b_flag) {
         gtk_list_store_set(GTK_LIST_STORE(pm->monitor_model), &miter,
            GAPC_MON_STATUS, pmview, GAPC_MON_UPSSTATE, pch3,
            GAPC_MON_ICON, pixbuf, -1);
      } else {
         gtk_list_store_set(GTK_LIST_STORE(pm->monitor_model), &miter,
            GAPC_MON_STATUS, pmview, GAPC_MON_UPSSTATE, pch3, -1);
      }
   }

   if ((w = g_hash_table_lookup(pm->pht_Widgets, "TitleStatus"))) {
      gtk_label_set_markup(GTK_LABEL(w), ptitle);
      lg_graph_set_chart_title (pm->phs.plg, ptitle);
      g_snprintf(pm->ch_title_info, GAPC_MAX_TEXT, "%s", ptitle);
      
      lg_graph_draw ( pm->phs.plg );
   }

   g_free(pmsg);
   g_free(ptitle);

/*  g_free (pmview); */

   return b_flag;
}

/*
 * main data updating routine.
 * -- collects and pushes data to all ui
 */
static gint gapc_monitor_update(PGAPC_MONITOR pm)
{
   gint i_x = 0;
   GtkWidget *win = NULL, *w = NULL;
   gchar *pch = NULL, *pch1 = NULL, *pch2 = NULL, *pch3 = NULL, *pch4 = NULL;
   gchar *pch_watt = NULL, *pch5 = NULL, *pch6 = NULL;
   gdouble dValue = 0.00, dScale = 0.0, dtmp = 0.0, dCharge = 0.0, d_loadpct = 0.0, d_watt = 0.0;
   gchar ch_buffer[GAPC_MAX_TEXT];
   PGAPC_BAR_H pbar = NULL;

   g_return_val_if_fail(pm != NULL, FALSE);

   if (pm->window == NULL)         /* not created yet */
      return TRUE;

   if (pm->b_run == FALSE)
      return FALSE;

   if (pm->b_data_available == FALSE)
      return FALSE;

   w = g_hash_table_lookup(pm->pht_Widgets, "StatusPage");
   if (gapc_util_text_view_clear_buffer(GTK_WIDGET(w))) {
      return FALSE;
   }
   for (i_x = 1; pm->pach_status[i_x] != NULL; i_x++) {
      gapc_util_text_view_append(GTK_WIDGET(w), pm->pach_status[i_x]);
   }

   w = g_hash_table_lookup(pm->pht_Widgets, "EventsPage");
   gapc_util_text_view_clear_buffer(GTK_WIDGET(w));
   for (i_x = 0; pm->pach_events[i_x] != NULL; i_x++) {
      gapc_util_text_view_prepend(GTK_WIDGET(w), pm->pach_events[i_x]);
   }

   /*
    *  compute graphic points */
   pch = g_hash_table_lookup(pm->pht_Status, "LINEV");
   if (pch == NULL) {
      pch = "n/a";
   }
   dValue = g_strtod(pch, NULL);
   dScale = (( dValue - 200 ) > 1) ? 230.0 : 120.0;
   dValue /= dScale;
   gapc_util_point_filter_set(&(pm->phs.sq[0]), dValue);
   pbar = g_hash_table_lookup(pm->pht_Status, "HBar1");
   pbar->d_value = dValue;
   g_snprintf(pbar->c_text, sizeof(pbar->c_text), "%s from Utility", pch);
   w = g_hash_table_lookup(pm->pht_Widgets, "HBar1-Widget");
   if (GTK_WIDGET_DRAWABLE(w))
      gdk_window_invalidate_rect(w->window, &pbar->rect, FALSE);

   pch = g_hash_table_lookup(pm->pht_Status, "BATTV");
   if (pch == NULL) {
      pch = "n/a";
   }
   pch1 = g_hash_table_lookup(pm->pht_Status, "NOMBATTV");
   if (pch1 == NULL) {
      pch1 = "n/a";
   }
   dValue = g_strtod(pch, NULL);
   dScale = g_strtod(pch1, NULL);
   if (dScale == 0.0)
      dScale = ((gint) (dValue - 20)) ? 24 : 12;
   dValue /= dScale;
   gapc_util_point_filter_set(&(pm->phs.sq[4]), dValue);
   pbar = g_hash_table_lookup(pm->pht_Status, "HBar2");
   pbar->d_value = (dValue > 1.0) ? 1.0 : dValue;
   g_snprintf(pbar->c_text, sizeof(pbar->c_text), "%s DC on Battery", pch);

   w = g_hash_table_lookup(pm->pht_Widgets, "HBar2-Widget");
   if (GTK_WIDGET_DRAWABLE(w))
      gdk_window_invalidate_rect(w->window, &pbar->rect, FALSE);

   pch = g_hash_table_lookup(pm->pht_Status, "BCHARGE");
   if (pch == NULL) {
      pch = "n/a";
   }
   dCharge = dValue = g_strtod(pch, NULL);
   dValue /= 100.0;
   gapc_util_point_filter_set(&(pm->phs.sq[3]), dValue);
   pbar = g_hash_table_lookup(pm->pht_Status, "HBar3");
   pbar->d_value = dValue;
   g_snprintf(pbar->c_text, sizeof(pbar->c_text), "%s Battery Charge", pch);
   w = g_hash_table_lookup(pm->pht_Widgets, "HBar3-Widget");
   if (GTK_WIDGET_DRAWABLE(w))
      gdk_window_invalidate_rect(w->window, &pbar->rect, FALSE);

   pch = g_hash_table_lookup(pm->pht_Status, "LOADPCT");
   if (pch == NULL) {
      pch = "n/a";
   }
   dValue = g_strtod(pch, NULL);
   d_loadpct = dtmp = dValue /= 100.0;
   gapc_util_point_filter_set(&(pm->phs.sq[1]), dValue);
   pbar = g_hash_table_lookup(pm->pht_Status, "HBar4");
   pbar->d_value = (dValue > 1.0) ? 1.0 : dValue;
   g_snprintf(pbar->c_text, sizeof(pbar->c_text), "%s", pch);

   w = g_hash_table_lookup(pm->pht_Widgets, "HBar4-Widget");
   if (GTK_WIDGET_DRAWABLE(w))
      gdk_window_invalidate_rect(w->window, &pbar->rect, FALSE);

   pch = g_hash_table_lookup(pm->pht_Status, "TIMELEFT");
   if (pch == NULL) {
      pch = "n/a";
   }
   dValue = g_strtod(pch, NULL);
   dScale = dValue / (1 - dtmp);
   dValue /= dScale;
   gapc_util_point_filter_set(&(pm->phs.sq[2]), dValue);
   pbar = g_hash_table_lookup(pm->pht_Status, "HBar5");
   pbar->d_value = dValue;
   g_snprintf(pbar->c_text, sizeof(pbar->c_text), "%s Remaining", pch);
   w = g_hash_table_lookup(pm->pht_Widgets, "HBar5-Widget");
   if (GTK_WIDGET_DRAWABLE(w))
      gdk_window_invalidate_rect(w->window, &pbar->rect, FALSE);

   /*
    * information window update */
   win = g_hash_table_lookup(pm->pht_Widgets, "SoftwareInformation");
   pch = g_hash_table_lookup(pm->pht_Status, "VERSION");
   pch1 = g_hash_table_lookup(pm->pht_Status, "UPSNAME");
   pch2 = g_hash_table_lookup(pm->pht_Status, "CABLE");
   pch3 = g_hash_table_lookup(pm->pht_Status, "UPSMODE");
   pch4 = g_hash_table_lookup(pm->pht_Status, "STARTTIME");
   pch5 = g_hash_table_lookup(pm->pht_Status, "STATUS");
   g_snprintf(ch_buffer, sizeof(ch_buffer),
      "<span foreground=\"blue\">" "%s\n%s\n%s\n%s\n%s\n%s" "</span>",
      (pch != NULL) ? pch : "N/A", (pch1 != NULL) ? pch1 : "N/A",
      (pch2 != NULL) ? pch2 : "N/A", (pch3 != NULL) ? pch3 : "N/A",
      (pch4 != NULL) ? pch4 : "N/A", (pch5 != NULL) ? pch5 : "N/A");
   gtk_label_set_markup(GTK_LABEL(win), ch_buffer);

   win = g_hash_table_lookup(pm->pht_Widgets, "PerformanceSummary");
   pch = g_hash_table_lookup(pm->pht_Status, "SELFTEST");
   pch1 = g_hash_table_lookup(pm->pht_Status, "NUMXFERS");
   pch2 = g_hash_table_lookup(pm->pht_Status, "LASTXFER");
   pch3 = g_hash_table_lookup(pm->pht_Status, "XONBATT");
   pch4 = g_hash_table_lookup(pm->pht_Status, "XOFFBATT");
   pch5 = g_hash_table_lookup(pm->pht_Status, "TONBATT");
   pch6 = g_hash_table_lookup(pm->pht_Status, "CUMONBATT");
   g_snprintf(ch_buffer, sizeof(ch_buffer),
      "<span foreground=\"blue\">" "%s\n%s\n%s\n%s\n%s\n%s\n%s" "</span>",
      (pch != NULL) ? pch : "N/A", (pch1 != NULL) ? pch1 : "N/A",
      (pch2 != NULL) ? pch2 : "N/A", (pch3 != NULL) ? pch3 : "N/A",
      (pch4 != NULL) ? pch4 : "N/A", (pch5 != NULL) ? pch5 : "N/A",
      (pch6 != NULL) ? pch6 : "N/A");
   gtk_label_set_markup(GTK_LABEL(win), ch_buffer);

   win = g_hash_table_lookup(pm->pht_Widgets, "ProductInformation");
   pch = g_hash_table_lookup(pm->pht_Status, "MODEL");
   pch1 = g_hash_table_lookup(pm->pht_Status, "SERIALNO");
   pch2 = g_hash_table_lookup(pm->pht_Status, "MANDATE");
   pch3 = g_hash_table_lookup(pm->pht_Status, "FIRMWARE");
   pch4 = g_hash_table_lookup(pm->pht_Status, "BATTDATE");
   pch_watt = g_hash_table_lookup(pm->pht_Status, "NOMPOWER");
   if (pch_watt != NULL) {
    pm->i_watt = g_strtod (pch_watt, NULL);
    d_watt = d_loadpct * pm->i_watt;
   } else {
    d_watt = d_loadpct * pm->i_watt;
   }
   g_snprintf(ch_buffer, sizeof(ch_buffer),
      "<span foreground=\"blue\">" "%s\n%s\n%s\n%s\n%s\n%3.0f of %d" "</span>",
      (pch != NULL) ? pch : "N/A", (pch1 != NULL) ? pch1 : "N/A",
      (pch2 != NULL) ? pch2 : "N/A", (pch3 != NULL) ? pch3 : "N/A",
      (pch4 != NULL) ? pch4 : "N/A", d_watt, pm->i_watt);
   gtk_label_set_markup(GTK_LABEL(win), ch_buffer);

   return TRUE;
}
/*  sknet_util_log_msg()
 *  capture the current application related error values
 *  output the composed str only if debug_flag is on
 */
static void sknet_util_log_msg (gchar * pch_func, gchar * pch_topic, gchar * pch_emsg)
{
  if (lg_graph_debug)
  {
    g_print ("%s(%s) msg=%s\n", pch_func, pch_topic, pch_emsg);
  }

  return;
}

/*
 * Write nbytes to the network.
 * It may require several writes.
 */
static gint sknet_net_write_nbytes (GIOChannel * ioc, gchar * ptr, gsize nbytes)
{
  gssize nleft = 0;
  gsize nwritten = 0;
  GError *gerror = NULL;
  GIOStatus ios = 0;
  gboolean b_eof = TRUE;

  nleft = nbytes;

  do
  {
    b_eof = FALSE;
    ios = g_io_channel_write_chars (ioc, ptr, nleft, &nwritten, &gerror);
    switch (ios)
    {
    case G_IO_STATUS_ERROR:
      sknet_util_log_msg ("sknet_net_write_nbytes", "G_IO_STATUS_ERROR", gerror->message);
      g_error_free (gerror);
      return -1;
      break;
    case G_IO_STATUS_AGAIN:
      sknet_util_log_msg ("sknet_net_write_nbytes", "G_IO_STATUS_AGAIN", "retry enabled");
      g_usleep (500000);
      break;
    case G_IO_STATUS_EOF:
      sknet_util_log_msg ("sknet_net_write_nbytes", "G_IO_STATUS_EOF", "ok");
      return 0;
      break;
    case G_IO_STATUS_NORMAL:
      break;
    default:
      sknet_util_log_msg ("sknet_net_write_nbytes", "unknown state", "aborted");
      b_eof = TRUE;
      return 0;
      break;
    }
    nleft -= nwritten;
    ptr += nwritten;
  }
  while ((nleft > 0) && (b_eof != TRUE));

  return (nbytes - nleft);
}

/*
 * Send a message over the network. The send consists of
 * two network packets. The first is sends a short containing
 * the length of the data packet which follows.
 * Returns number of bytes sent
 * Returns -1 on error
 */
static gint sknet_net_send (GIOChannel * ioc, gchar * buff, gsize len)
{
  gint rc = 0;
  gshort pktsiz = 0;

  /* send short containing size of data packet */
  pktsiz = g_htons ((gshort) len);
  rc = sknet_net_write_nbytes (ioc, (gchar *) &pktsiz, sizeof (gshort));
  if (rc != sizeof (gshort))
  {
    sknet_util_log_msg ("sknet_net_send", "send message size", "failed");
    return -1;
  }

  /* send data packet */
  rc = sknet_net_write_nbytes (ioc, buff, len);
  if (rc != len)
  {
    sknet_util_log_msg ("sknet_net_send", "send message buffer", "failed");
    return -1;
  }

  return rc;
}

/*
 * Read nbytes from the network.
 * It is possible that the total bytes requires several 
 * read requests, which will happen automatically.
 * Returns: count of bytes read, or -1 for error
 */
static gint sknet_net_read_nbytes (GIOChannel * ioc, gchar * ptr, gsize nbytes)
{
  gsize nleft = 0;
  gsize nread = 0;
  GError *gerror = NULL;
  GIOStatus ios = 0;
  gboolean b_eof = FALSE;

  nleft = nbytes;

  do
  {
    b_eof = FALSE;
    ios = g_io_channel_read_chars (ioc, ptr, nleft, &nread, &gerror);
    switch (ios)
    {
    case G_IO_STATUS_ERROR:
      sknet_util_log_msg ("sknet_net_read_nbytes", "G_IO_STATUS_ERROR", gerror->message);
      g_error_free (gerror);
      return -1;
      break;
    case G_IO_STATUS_AGAIN:
      sknet_util_log_msg ("sknet_net_read_nbytes", "G_IO_STATUS_AGAIN", "aborted");
      g_usleep (500000);
      break;
    case G_IO_STATUS_EOF:
      sknet_util_log_msg ("sknet_net_read_nbytes", "G_IO_STATUS_EOF", "ok");
      return 0;
      break;
    case G_IO_STATUS_NORMAL:
      break;
    default:
      sknet_util_log_msg ("sknet_net_read_nbytes", "unknown state", "aborted");
      b_eof = TRUE;
      return 0;
      break;
    }
    nleft -= nread;
    ptr += nread;
  }
  while ((nleft > 0) && (b_eof != TRUE));

  return (nbytes - nleft);      /* return >= 0 */
}

/* 
 * Receive a message from the other end. Each message consists of
 * two packets. The first is a header that contains the size
 * of the data that follows in the second packet.
 * Returns number of bytes read
 * Returns 0 on end of file
 * Returns -1 on hard end of file (i.e. network connection close)
 * Returns -2 on error
 */
static gint sknet_net_recv (GIOChannel * ioc, gchar * buff, gsize maxlen)
{
  gint nbytes = 0;
  gshort pktsiz = 0;

  /* get data size -- in short */
  nbytes = sknet_net_read_nbytes (ioc, (gchar *) &pktsiz, sizeof (gshort));
  if (nbytes <= 0)
  {
    sknet_util_log_msg ("sknet_net_recv", "read msg_len", "failed");
    return -1;                  /* assume hard EOF received */
  }
  if (nbytes != sizeof (gshort))
  {
    sknet_util_log_msg ("sknet_net_recv", "read short_len", "failed");
    return -2;
  }

  pktsiz = g_ntohs (pktsiz);    /* decode no. of bytes that follow */
  if (pktsiz > maxlen)
  {
    sknet_util_log_msg ("sknet_net_recv", "msg_len gt buffer", "overflow");
    return -2;
  }

  if (pktsiz == 0)
  {
    sknet_util_log_msg ("sknet_net_recv", "Soft error", "End-of-File");
    return 0;                   /* soft EOF */
  }

  /* now read the actual data */
  nbytes = sknet_net_read_nbytes (ioc, buff, pktsiz);
  if (nbytes <= 0)
  {
    sknet_util_log_msg ("sknet_net_recv", "read message", "failed");
    return -2;
  }
  if (nbytes != pktsiz)
  {
    sknet_util_log_msg ("sknet_net_recv", "read incomplete", "length error");
    return -2;
  }

  return (nbytes);              /* return actual length of message */
}

/* sknet_net_close()
 * Close the active or error'ed socket
*/
static void sknet_net_close (GIOChannel *ioc, gboolean b_flush)
{
    int sockfd =0;
    GError *gerror = NULL;
    GIOStatus  ios = G_IO_STATUS_NORMAL;
    
    sockfd = g_io_channel_unix_get_fd (ioc);
    ios = g_io_channel_shutdown (ioc, b_flush, &gerror);
    if (gerror != NULL)
    {
      sknet_util_log_msg ("sknet_channel_close", "error", gerror->message);
      g_error_free (gerror);
    }
    if (ios != G_IO_STATUS_NORMAL) {
        g_message ("net_close: g_io_channel_shutdown(%d) failed with %d", sockfd, ios);      
    }

    g_io_channel_unref (ioc);
    

  return;
}

/*     
 * Open a TCP connection using GIOChannels to a host
 * Returns NULL on error, with err text in ch_error_message
 * Returns GIOChannel ptr otherwise
 * Affects: -psk->gip, which is a sockaddr_in address of partner host
 *          this value is allocated and retained for the life of this program
 *          -psk->b_network_control, if true causes the addr to be resolved again
 *          or if false, it uses the current value - saving a dns hit/query
 */
static GIOChannel *sknet_net_open (PSKCOMM psk)
{
  GIOChannel *ioc = NULL;
  int sockfd;
  struct sockaddr_in *tcp_serv_addr = NULL;
  gint   nrc = 0; 

  g_return_val_if_fail (psk != NULL, NULL);

  /*
   * Allocate a new address struct if it does not exist 
  */
  if (psk->gip == NULL) {
      psk->b_network_control=TRUE;      
      psk->gip = g_new0( struct sockaddr_in , 1);
      g_return_val_if_fail (psk->gip != NULL, NULL);
      tcp_serv_addr = (struct sockaddr_in *)psk->gip;
  } else {
      tcp_serv_addr = (struct sockaddr_in *)psk->gip;
  }

  /* 
   * Fill in the structure serv_addr with the address of
   * the server that we want to connect with.
   */
  if ( psk->b_network_control ) {   
    memset ((char *)tcp_serv_addr, 0, sizeof (struct sockaddr_in));
    tcp_serv_addr->sin_family = AF_INET;
    tcp_serv_addr->sin_port = g_htons (psk->i_port);

    nrc = inet_aton (psk->ch_ip_string, (struct in_addr *)&tcp_serv_addr->sin_addr.s_addr);
    if ( nrc == 0) /* inet_aton failed */
    {
        struct hostent he, *phe;
        char *buff;
        size_t bufflen = 0;

        phe = gethostname_re(psk->ch_ip_string, &he, &buff, &bufflen);
        if (phe == NULL)
        {
            free(buff);
            sknet_util_log_msg ("sknet_net_open", "gethostbyname() failed", "");
            g_snprintf(psk->ch_error_msg, sizeof(psk->ch_error_msg), "gethostbyname() failed");
            psk->ioc = NULL;
            return NULL;
        }
        if (he.h_length != sizeof (struct in_addr) || he.h_addrtype != AF_INET)
        {
            free(buff);
            sknet_util_log_msg ("sknet_net_open", "struct hostent", "argument error");
            g_snprintf(psk->ch_error_msg, sizeof(psk->ch_error_msg),"%s","argument error");
            psk->ioc = NULL;
            return NULL;
        }

        tcp_serv_addr->sin_addr.s_addr = *(unsigned int *) he.h_addr;
        free(buff);
    } /* end if inet_addr */
    
  } /* end if b_network */ 


  /* Open a TCP socket */
  if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
  {
    sknet_util_log_msg ("sknet_net_open", "socket() api failed",
                      (gchar *) g_strerror (errno));
    g_snprintf(psk->ch_error_msg, sizeof(psk->ch_error_msg),"%s",
              (gchar *) g_strerror (errno));
    psk->ioc = NULL;
    return NULL;
  }

  {
  struct timeval tv;
  int rcx = 0;
  tv.tv_sec = 0;
  tv.tv_usec = 200000;
  
  rcx = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO | SO_SNDTIMEO, 
                   &tv, (socklen_t) sizeof(struct timeval));
   if (rcx == -1) {
       sknet_util_log_msg ("sknet_net_open", "setsockopt(200ms) failed!",
                       (gchar *) g_strerror (errno));
   }
  
  }
  
  /* connect to server */
  if ((connect (sockfd, (struct sockaddr *) tcp_serv_addr, sizeof (struct sockaddr_in))) == -1)
  {
    sknet_util_log_msg ("sknet_net_open", "connect() api failed",
                      (gchar *) g_strerror (errno));
    g_snprintf(psk->ch_error_msg, sizeof(psk->ch_error_msg),"%s",
              (gchar *) g_strerror (errno));

    close (sockfd);
    psk->b_network_control = TRUE;
    psk->ioc = NULL;
    return NULL;
  }
  
  psk->b_network_control = FALSE;
  
  ioc = g_io_channel_unix_new (sockfd);
  g_io_channel_set_encoding (ioc, NULL, NULL);
  g_io_channel_set_buffered (ioc, FALSE);
  
  psk->ioc = ioc;
  return ioc;
}

/* sknet_net_client_init()
 * Create a control structure and set ip values
 * for an open call.
 * return NULL on error.
*/
static PSKCOMM sknet_net_client_init (gchar *pch_remote_ip, gint i_remote_port)
{
   PSKCOMM psk = NULL;

   psk = g_new0(SKNET_COMMS, 1);
         g_return_val_if_fail(psk != NULL, NULL);
         
   psk->gp_reserved = NULL;
   psk->cb_id = 0;        /* initialize count of instances created */       
   g_snprintf( psk->ch_ip_string, sizeof(psk->ch_ip_string), "%s", pch_remote_ip);
   psk->i_port = i_remote_port;
   psk->b_network_control = TRUE;

   g_snprintf (psk->ch_error_msg, sizeof (psk->ch_error_msg), 
               "client init(client ready to connect to %s:%6d", 
               psk->ch_ip_string, psk->i_port);
   sknet_util_log_msg ("sknet_net_client_init", psk->ch_error_msg, "Ready");
    
   return psk;
}

/*
 * Close server socket if open and release internal storage
*/
static void sknet_net_shutdown (PSKCOMM psk)
{

    g_return_if_fail (psk != NULL);

    if (psk->gp_reserved != NULL) {
        ((PSKCOMM)psk->gp_reserved)->cb_id--;    /* decrement count of instances created */
    } else {                                     /* or close main socket */
            if (psk->fd_server) 
            {
                close (psk->fd_server);
            }
    }

    if (psk->gip != NULL) {
        g_free(psk->gip);
    }
    
    g_free(psk);
    
    return;
}

/*
 * performs a complete NIS transaction by sending cmd and
 * loading each result line into the pch array.
 * also, refreshes status key/value pairs in hastable.
 * return error = 0,  or number of lines read from network
 */
static gint gapc_net_transaction_service(PGAPC_MONITOR pm, gchar * cp_cmd, gchar ** pch)
{
   gint n = 0, iflag = 0;
   GIOChannel   *ioc = NULL;
   
   g_return_val_if_fail(pm, -1);
   g_return_val_if_fail(pm->psk, -1);   
   g_return_val_if_fail(pm->pch_host, -1);


   ioc = sknet_net_open(pm->psk);
   if (ioc == NULL) {
      return 0;
   }

   n = sknet_net_send( ioc, cp_cmd, g_utf8_strlen(cp_cmd, -1));
   if (n <= 0) {
      sknet_net_close( ioc, TRUE);
      return 0;
   }

   /* clear current data */
   for (iflag = 0; iflag < GAPC_MAX_ARRAY; iflag++) {
      if (pch[iflag] != NULL) {
         g_free(pch[iflag]);
      }
      pch[iflag] = NULL;
   }

   iflag = 0;
   while (iflag < GAPC_MAX_ARRAY) {
      n = sknet_net_recv(ioc, pm->psk->ch_session_message, sizeof(pm->psk->ch_session_message));
      if (n < 1)
         break;

      pm->psk->ch_session_message[n] = 0;
      pch[iflag++] = g_strdup(pm->psk->ch_session_message);

      if (g_str_equal(cp_cmd, "status") && iflag > 1)
         gapc_util_update_hashtable(pm, pm->psk->ch_session_message);
   }

   sknet_net_close(ioc, TRUE);

   return iflag;                   /* count of records received */
}

/*
 * Worker thread for network communications.
 */
static gpointer *gapc_net_thread_qwork(PGAPC_MONITOR pm)
{
   gint rc = 0;
   GAsyncQueue *thread_queue = NULL;

   g_return_val_if_fail(pm != NULL, NULL);
   g_return_val_if_fail(pm->q_network != NULL, NULL);

   g_async_queue_ref(pm->q_network);
   thread_queue = pm->q_network;

   if (pm->psk == NULL) {
      pm->psk = sknet_net_client_init (pm->pch_host, pm->i_port);
      if (pm->psk == NULL) {
         g_async_queue_unref(thread_queue);
         g_thread_exit(GINT_TO_POINTER(0));
      }
   }

   while ((pm = (PGAPC_MONITOR) g_async_queue_pop(thread_queue))) {
      if (pm->b_thread_stop) {
         break;
      }

      if (pm->b_run) {
         g_mutex_lock(pm->gm_update);
         if (!pm->b_run) {         /* may have waited a while for lock */
            g_mutex_unlock(pm->gm_update);
            continue;
         }

         if ((rc = gapc_net_transaction_service(pm, "status", pm->pach_status))) {
            gapc_net_transaction_service(pm, "events", pm->pach_events);
         }
         g_mutex_unlock(pm->gm_update);

         if (rc > 0) {
            pm->b_data_available = TRUE;
         } else {
            pm->b_data_available = FALSE;
         }
      } else {
         pm->b_data_available = FALSE;
      }
   }                               /* end-while */

   if (pm->psk != NULL) {
       sknet_net_shutdown (pm->psk);
       pm->psk = NULL;
   }
   
   g_async_queue_unref(thread_queue);

   g_thread_exit(GINT_TO_POINTER(1));

   return NULL;
}

/*
 * return the answer and reset the internal controls to zero
*/
static gdouble gapc_util_point_filter_reset(PGAPC_SUMS sq)
{
   gdouble d_the_final_answer = 0.0;

   g_mutex_lock(sq->gm_graph);

   d_the_final_answer = sq->last_answer;

   sq->point_count = 0;

   sq->this_point = 0.0;
   sq->last_point = 0.0;

   sq->this_answer = 0.0;
   sq->last_answer = 0.0;

   sq->answer_summ = 0.0;
   sq->point_min = 0.0;
   sq->point_max = 0.0;

   g_mutex_unlock(sq->gm_graph);

   return (d_the_final_answer);
}

/*
 * Compute the average of the given data point
*/
static gdouble gapc_util_point_filter_set(PGAPC_SUMS sq, gdouble this_point)
{
   g_mutex_lock(sq->gm_graph);

   sq->this_point = this_point;

   sq->this_point *= 100;
   sq->point_count++;

   /* some calc here */
   sq->answer_summ += sq->this_point ;
   sq->this_answer = sq->answer_summ / sq->point_count;

   sq->last_point = sq->this_point;
   sq->last_answer = sq->this_answer;

   if (sq->point_min > sq->this_point)
      sq->point_min = sq->this_point;
   if (sq->point_max < sq->this_point)
      sq->point_max = sq->this_point;

   g_mutex_unlock(sq->gm_graph);

   return (sq->this_answer);
}

/*
 * Get a iter to the list_store record containing this monitor key.
 * Will search either list_store
 * - monitor col is the same in both models
 * Returns: TRUE and iter is found, FALSE and invalid iter if not found
*/
static gboolean gapc_util_treeview_get_iter_from_monitor(GtkTreeModel * model,
   GtkTreeIter * iter, gint i_value)
{
   gboolean valid = FALSE, b_result = FALSE;
   gint i_monitor;

   g_return_val_if_fail(model != NULL, FALSE);
   g_return_val_if_fail(iter != NULL, FALSE);

   valid = gtk_tree_model_get_iter_first(model, iter);
   while (valid) {
      gtk_tree_model_get(model, iter, GAPC_PREFS_MONITOR, &i_monitor, -1);

      if (i_monitor == i_value) {
         /* set sucess flag */
         b_result = TRUE;
         break;
      }
      valid = gtk_tree_model_iter_next(model, iter);
   }

   return b_result;
}

/*
 * Add a preferences record to the gconf instance and prefs_model
 * returns FALSE on error
 * returns TRUE on sucess
*/
static gboolean gapc_panel_preferences_gconf_add_rec(PGAPC_CONFIG pcfg,
   gint i_monitor)
{
   GAPC_PKEYS pk;
   gchar *pkey = NULL;

   g_return_val_if_fail(pcfg != NULL, FALSE);
   g_return_val_if_fail(pcfg->client != NULL, FALSE);
   g_return_val_if_fail(pcfg->prefs_model != NULL, FALSE);

   pkey = GAPC_MID_GROUP_KEY;
   g_snprintf(pk.k_enabled, GAPC_MAX_TEXT, "%s/%d/%s", pkey, i_monitor, "enabled");
   g_snprintf(pk.k_use_systray, GAPC_MAX_TEXT, "%s/%d/%s", pkey, i_monitor,
      "use_systray");
   g_snprintf(pk.k_port_number, GAPC_MAX_TEXT, "%s/%d/%s", pkey, i_monitor,
      "port_number");
   g_snprintf(pk.k_network_interval, GAPC_MAX_TEXT, "%s/%d/%s", pkey, i_monitor,
      "network_interval");
   g_snprintf(pk.k_graph_interval, GAPC_MAX_TEXT, "%s/%d/%s", pkey, i_monitor,
      "graph_interval");
   g_snprintf(pk.k_host_name, GAPC_MAX_TEXT, "%s/%d/%s", pkey, i_monitor,
      "host_name");
   g_snprintf(pk.v_host_name, GAPC_MAX_TEXT, "%s", GAPC_HOST_DEFAULT);

   gconf_client_set_bool(pcfg->client, pk.k_enabled, FALSE, NULL);
   gconf_client_set_bool(pcfg->client, pk.k_use_systray, FALSE, NULL);
   gconf_client_set_int(pcfg->client, pk.k_port_number, GAPC_PORT_DEFAULT, NULL);
   gconf_client_set_float(pcfg->client, pk.k_network_interval, GAPC_REFRESH_DEFAULT,
      NULL);
   gconf_client_set_float(pcfg->client, pk.k_graph_interval,
      GAPC_LINEGRAPH_REFRESH_FACTOR, NULL);
   gconf_client_set_string(pcfg->client, pk.k_host_name, pk.v_host_name, NULL);

   return TRUE;
}

/*
 *  capture the current application related error values centrally
*/
static void gapc_util_log_app_msg(gchar * pch_func, gchar * pch_topic,
   gchar * pch_emsg)
{
   gchar *pch = NULL;

   g_return_if_fail(pch_func != NULL);

   pch = g_strdup_printf("%s(%s) emsg=%s", pch_func, pch_topic, pch_emsg);

   g_message(pch);

   g_free(pch);

   return;
}

/*
 * parses received line of text into key/value pairs to be inserted
 * into the status hashtable.
 */
static gint gapc_util_update_hashtable(PGAPC_MONITOR pm, gchar * pch_unparsed)
{
   gchar *pch_in = NULL;
   gchar *pch = NULL;
   gchar *pch_end = NULL;
   gint ilen = 0;

   g_return_val_if_fail(pm != NULL, FALSE);
   g_return_val_if_fail(pch_unparsed != NULL, -1);

   /* unparsed contains - keystring : keyvalue nl */
   pch_in = g_strdup(pch_unparsed);
   pch_end = g_strrstr(pch_in, "\n");
   if (pch_end != NULL)
      *pch_end = 0;

   ilen = g_utf8_strlen(pch_in, -1);

   pch = g_strstr_len(pch_in, ilen, ":");
   *pch = 0;
   pch_in = g_strchomp(pch_in);
   pch++;
   pch = g_strstrip(pch);

   g_hash_table_replace(pm->pht_Status, g_strdup(pch_in), g_strdup(pch));

   g_free(pch_in);

   return ilen;
}

/*
 *  Implements a Horizontal Bar Chart...
 *  - data value has a range of 0.0 to 1.0 for 0-100% display
 *  - in chart text is limited to about 30 chars
 */
static gboolean cb_util_barchart_handle_exposed(GtkWidget * widget,
   GdkEventExpose * event, gpointer data)
{
   PGAPC_BAR_H pbar = data;
   gint i_percent = 0;
   PangoLayout *playout = NULL;

   g_return_val_if_fail(data, FALSE);   /* error exit */

   pbar->rect.x = 0;
   pbar->rect.y = 0;
   pbar->rect.width = widget->allocation.width;
   pbar->rect.height = widget->allocation.height;

   /* scale up the less than zero data value */
   i_percent =
      (gint) ((gdouble) (widget->allocation.width / 100.0) *
      (gdouble) (pbar->d_value * 100.0));

   /* the frame of the chart */
   gtk_paint_box(widget->style, widget->window, GTK_WIDGET_STATE(widget),
      GTK_SHADOW_ETCHED_IN, &pbar->rect, widget, "gapc_hbar_frame", 0, 0,
      widget->allocation.width - 1, widget->allocation.height - 1);

   /* the scaled value */
   gtk_paint_box(widget->style, widget->window, GTK_STATE_ACTIVE, GTK_SHADOW_OUT,
      &pbar->rect, widget, "gapc_hbar_value", 1, 1, i_percent,
      widget->allocation.height - 4);

   if (pbar->c_text[0]) {
      gint x = 0, y = 0;

      playout = gtk_widget_create_pango_layout(widget, pbar->c_text);
      pango_layout_set_markup(playout, pbar->c_text, -1);

      pango_layout_get_pixel_size(playout, &x, &y);
      x = (widget->allocation.width - x) / 2;
      y = (widget->allocation.height - y) / 2;

      gtk_paint_layout(widget->style, widget->window, GTK_STATE_NORMAL, TRUE,
         &pbar->rect, widget, "gapc_hbar_text",
         (pbar->b_center_text) ? x : 6, y, playout);

      g_object_unref(playout);
   }

   return TRUE;
}

/*
 * creates horizontal bar chart and allocates control data
 * requires cb_h_bar_chart_exposed() routine
 * return drawing area widget
 */
static GtkWidget *gapc_util_barchart_create(PGAPC_MONITOR pm, GtkWidget * vbox,
   gchar * pch_hbar_name, gdouble d_percent, gchar * pch_text)
{
   PGAPC_BAR_H pbar = NULL;
   GtkWidget *drawing_area = NULL;
   gchar *pch = NULL;

   g_return_val_if_fail(pm != NULL, NULL);

   pbar = g_new0(GAPC_BAR_H, 1);
   pbar->d_value = d_percent;
   pbar->b_center_text = FALSE;
   g_strlcpy(pbar->c_text, pch_text, sizeof(pbar->c_text));

   drawing_area = gtk_drawing_area_new();       /* manual bargraph */
   gtk_widget_set_size_request(drawing_area, 100, 20);
   g_signal_connect(G_OBJECT(drawing_area), "expose_event",
      G_CALLBACK(cb_util_barchart_handle_exposed), (gpointer) pbar);

   gtk_box_pack_start(GTK_BOX(vbox), drawing_area, TRUE, TRUE, 0);
   gtk_widget_show(drawing_area);
   g_hash_table_insert(pm->pht_Status, g_strdup(pch_hbar_name), pbar);
   pch = g_strdup_printf("%s-Widget", pch_hbar_name);
   g_hash_table_insert(pm->pht_Widgets, pch, drawing_area);

   return drawing_area;
}

/*
 * Utility Routines for text views
 */
static gboolean gapc_util_text_view_clear_buffer(GtkWidget * view)
{
   GtkTextIter start, end;
   GtkTextBuffer *buffer = NULL;

   g_return_val_if_fail(view != NULL, TRUE);

   buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
   gtk_text_buffer_get_bounds(buffer, &start, &end);
   gtk_text_buffer_delete(buffer, &start, &end);

   return FALSE;
}

/*
 * Utility Routines for text views
 */
static void gapc_util_text_view_prepend(GtkWidget * view, gchar * pch)
{
   GtkTextIter iter;
   GtkTextBuffer *buffer;

   g_return_if_fail(view != NULL);
   g_return_if_fail(pch != NULL);

   buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
   gtk_text_buffer_get_start_iter(buffer, &iter);
   gtk_text_buffer_insert(buffer, &iter, pch, -1);
}

/*
 * Utility Routines for text views
 */
static void gapc_util_text_view_append(GtkWidget * view, gchar * pch)
{
   GtkTextIter iter;
   GtkTextBuffer *buffer;

   g_return_if_fail(view != NULL);
   g_return_if_fail(pch != NULL);

   buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
   gtk_text_buffer_get_end_iter(buffer, &iter);
   gtk_text_buffer_insert(buffer, &iter, pch, -1);
}

static gint gapc_panel_monitor_model_rec_add(PGAPC_CONFIG pcfg, PGAPC_MONITOR pm)
{
   GtkTreeIter iter;
   gchar *pch = NULL;

   g_return_val_if_fail(pcfg != NULL, -1);
   g_return_val_if_fail(pm != NULL, -1);
   g_return_val_if_fail(pcfg->monitor_model != NULL, -1);
   g_return_val_if_fail(pcfg->monitor_select != NULL, -1);

   gtk_list_store_append(GTK_LIST_STORE(pcfg->monitor_model), &iter);

   pch = g_hash_table_lookup(pm->pht_Status, "STATUS");
   if (pch == NULL) {
      pch = "Starting";
   }

   gtk_list_store_set(GTK_LIST_STORE(pcfg->monitor_model), &iter, GAPC_MON_ICON,
      pm->my_icons[pm->i_icon_index], GAPC_MON_STATUS,
      pm->ch_title_info, GAPC_MON_MONITOR, pm->cb_monitor_num,
      GAPC_MON_POINTER, (gpointer) pm, GAPC_MON_UPSSTATE, g_strdup(pch), -1);

   gtk_tree_selection_select_iter(pcfg->monitor_select, &iter);

   return TRUE;
}

static gint gapc_panel_preferences_model_rec_remove(PGAPC_CONFIG pcfg)
{
   GtkTreeIter iter, *piter = NULL;
   gboolean result = FALSE;
   gint i_monitor = 0;
   gchar ch[GAPC_MAX_TEXT];
   gchar chk[GAPC_MAX_TEXT];
   GError *gerror = NULL;

   g_return_val_if_fail(pcfg != NULL, -1);
   g_return_val_if_fail(pcfg->prefs_model != NULL, -1);

   if (gtk_tree_selection_get_selected(pcfg->prefs_select, NULL, &iter)) {
      piter = gtk_tree_iter_copy(&iter);
      if (gtk_tree_model_iter_next(GTK_TREE_MODEL(pcfg->prefs_model), piter)) {
         gtk_tree_selection_select_iter(pcfg->prefs_select, piter);
      }
      gtk_tree_iter_free(piter);
      gtk_tree_model_get(GTK_TREE_MODEL(pcfg->prefs_model), &iter,
         GAPC_PREFS_MONITOR, &i_monitor, -1);

      /* now remove the record from gconf */
      g_snprintf(ch, GAPC_MAX_TEXT, "%s/%d", GAPC_MID_GROUP_KEY, i_monitor);

      gconf_client_unset(pcfg->client, ch, &gerror);
      if (gerror != NULL) {
         gapc_util_log_app_msg("gapc_panel_preferences_model_rec_remove()",
            "gconf_client_unset(DIR) Failed", gerror->message);
         g_error_free(gerror);
         gerror = NULL;
      }

      gconf_client_suggest_sync(pcfg->client, &gerror);
      if (gerror != NULL) {
         gapc_util_log_app_msg("gapc_panel_preferences_model_rec_remove()",
            "gconf_client_suggest_sync() Failed", gerror->message);
         g_error_free(gerror);
         gerror = NULL;
      }

      g_snprintf(chk, GAPC_MAX_TEXT, "%s/%s", ch, "enabled");
      gconf_client_unset(pcfg->client, chk, NULL);

      g_snprintf(chk, GAPC_MAX_TEXT, "%s/%s", ch, "use_systray");
      gconf_client_unset(pcfg->client, chk, NULL);

      g_snprintf(chk, GAPC_MAX_TEXT, "%s/%s", ch, "skip_pagers");
      gconf_client_unset(pcfg->client, chk, NULL);

      g_snprintf(chk, GAPC_MAX_TEXT, "%s/%s", ch, "port_number");
      gconf_client_unset(pcfg->client, chk, NULL);

      g_snprintf(chk, GAPC_MAX_TEXT, "%s/%s", ch, "network_interval");
      gconf_client_unset(pcfg->client, chk, NULL);

      g_snprintf(chk, GAPC_MAX_TEXT, "%s/%s", ch, "graph_interval");
      gconf_client_unset(pcfg->client, chk, NULL);

      g_snprintf(chk, GAPC_MAX_TEXT, "%s/%s", ch, "host_name");
      gconf_client_unset(pcfg->client, chk, NULL);

      gconf_client_unset(pcfg->client, ch, &gerror);    /* again to drop it in gconf */
      if (gerror != NULL) {
         gapc_util_log_app_msg("gapc_panel_preferences_model_rec_remove()",
            "gconf_client_unset(DIR) Failed", gerror->message);
         g_error_free(gerror);
         gerror = NULL;
      }

      gconf_client_suggest_sync(pcfg->client, &gerror);
      if (gerror != NULL) {
         gapc_util_log_app_msg("gapc_panel_preferences_model_rec_remove()",
            "gconf_client_suggest_sync() Failed", gerror->message);
         g_error_free(gerror);
         gerror = NULL;
      }

   }

   return result;
}

static gint gapc_panel_preferences_model_rec_add(PGAPC_CONFIG pcfg)
{
   g_return_val_if_fail(pcfg != NULL, -1);
   g_return_val_if_fail(pcfg->prefs_model != NULL, -1);
   g_return_val_if_fail(pcfg->prefs_select != NULL, -1);

   gapc_panel_preferences_gconf_add_rec(pcfg, ++pcfg->prefs_last_monitor);

   return TRUE;
}

/*
 * EggTrayIcon Callbacks 
*/
static void cb_panel_systray_icon_activated(GtkPlug * plug, gpointer gp)
{
   PGAPC_CONFIG pcfg = NULL;
   PGAPC_MONITOR pm = NULL;

   g_return_if_fail(plug != NULL);
   g_return_if_fail(gp != NULL);

   if (((PGAPC_MONITOR) gp)->cb_id == CB_MONITOR_ID) {
      /* this is a monitor struct (2) */
      pm = (PGAPC_MONITOR) gp;
      if (pm->window != NULL) {
         g_object_set(pm->window, "skip-pager-hint", TRUE, "skip-taskbar-hint",
            TRUE, NULL);
      }

   } else {
      /* this is a config struct (1) */
      pcfg = (PGAPC_CONFIG) gp;
      if (pcfg->window != NULL) {
         g_object_set(pcfg->window, "skip-pager-hint", TRUE, "skip-taskbar-hint",
            TRUE, NULL);
      }

   }

   return;
}

static gboolean cb_panel_systray_icon_configure(GtkWidget * widget,
   GdkEventConfigure * event, gpointer gp)
{
   PGAPC_CONFIG pcfg = NULL;
   PGAPC_MONITOR pm = NULL;

   gint *pi_icon_size = NULL;

   g_return_val_if_fail(gp != NULL, FALSE);
   g_return_val_if_fail(event != NULL, FALSE);

   if (((PGAPC_MONITOR) gp)->cb_id == CB_MONITOR_ID) {
      /* this is a monitor struct (2) */
      pm = (PGAPC_MONITOR) gp;
      pi_icon_size = &pm->i_icon_size;
      pm->i_icon_height = event->height;
      pm->i_icon_width = event->width;
   } else {
      /* this is a config struct (1) */
      pcfg = (PGAPC_CONFIG) gp;
      pi_icon_size = &pcfg->i_icon_size;
      pcfg->i_icon_height = event->height;
      pcfg->i_icon_width = event->width;
   }

   *pi_icon_size = MIN(event->width, event->height);
   return FALSE;
}

static gboolean cb_panel_systray_icon_handle_clicked(GtkWidget * widget,
   GdkEventButton * event, gpointer gp)
{
   PGAPC_CONFIG pcfg = NULL;
   PGAPC_MONITOR pm = NULL;
   GtkWidget *window = NULL;
   gboolean b_visible = FALSE;
   GtkWidget *menu = NULL;

   g_return_val_if_fail(gp != NULL, FALSE);
   g_return_val_if_fail(widget != NULL, FALSE);

   if (((PGAPC_MONITOR) gp)->cb_id == CB_MONITOR_ID) {
      /* this is a monitor struct (2) */
      pm = (PGAPC_MONITOR) gp;
      window = pm->window;
      b_visible = pm->b_visible;
      menu = pm->menu;
   } else {
      /* this is a config struct (1) */
      pcfg = (PGAPC_CONFIG) gp;
      window = pcfg->window;
      b_visible = pcfg->b_visible;
      menu = pcfg->menu;
   }

   if (window == NULL) {
      return FALSE;
   }

   if (event->type == GDK_BUTTON_PRESS) {
      switch (event->button) {
      case 1:
         if (b_visible) {
            gtk_widget_hide(GTK_WIDGET(window));
         } else {
            gtk_window_present(GTK_WINDOW(window));
         }
         break;
      case 2:
      case 3:
         if (menu != NULL) {
            gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button,
               event->time);
         }
         break;
      default:
         return FALSE;
      }

      return TRUE;
   }

   return FALSE;
}

static void cb_panel_systray_icon_destroy(GtkObject * object, gpointer gp)
{
   PGAPC_CONFIG pcfg = NULL;
   PGAPC_MONITOR pm = NULL;

   g_return_if_fail(gp != NULL);

   if (((PGAPC_MONITOR) gp)->cb_id == CB_MONITOR_ID) {
      /* this is a monitor struct (2) */
      pm = (PGAPC_MONITOR) gp;
      pm->tray_icon = NULL;
      pm->tray_image = NULL;

      if (pm->b_run && (pm->window != NULL)) {
         g_object_set(pm->window, "skip-pager-hint", FALSE, "skip-taskbar-hint",
            FALSE, NULL);
         gtk_window_present(GTK_WINDOW(pm->window));
      }

   } else {
      /* this is a config struct (1) */
      pcfg = (PGAPC_CONFIG) gp;
      pcfg->tray_icon = NULL;
      pcfg->tray_image = NULL;

      if (pcfg->b_run && (pcfg->window != NULL)) {
         g_object_set(pcfg->window, "skip-pager-hint", FALSE, "skip-taskbar-hint",
            FALSE, NULL);
         gtk_window_present(GTK_WINDOW(pcfg->window));
      }
   }

   return;
}

static gboolean gapc_panel_systray_icon_create(gpointer gp)
{
   PGAPC_CONFIG pcfg = NULL;
   PGAPC_MONITOR pm = NULL;

   EggTrayIcon **tray_icon = NULL;
   GtkWidget **tray_image = NULL;
   GdkPixbuf *pixbuf = NULL;
   GtkTooltips *tooltips = NULL;
   gchar *pch_title = NULL;

   g_return_val_if_fail(gp != NULL, FALSE);

   if (((PGAPC_MONITOR) gp)->cb_id == CB_MONITOR_ID) {
      /* this is a monitor struct (2) */
      pm = (PGAPC_MONITOR) gp;
      tray_icon = &pm->tray_icon;
      tray_image = &pm->tray_image;
      tooltips = pm->tooltips;
      pixbuf = pm->my_icons[GAPC_ICON_DEFAULT];

      if (!pm->cb_use_systray) {
         return FALSE;
      }

      pch_title = pm->ch_title_info;
   } else {
      /* this is a config struct (1) */
      pcfg = (PGAPC_CONFIG) gp;
      tray_icon = &pcfg->tray_icon;
      tray_image = &pcfg->tray_image;
      tooltips = pcfg->tooltips;
      pixbuf = pcfg->my_icons[GAPC_ICON_DEFAULT];
      pch_title = GAPC_WINDOW_TITLE;

      if (!pcfg->b_use_systray) {
         return FALSE;
      }
   }

   g_return_val_if_fail(*tray_icon == NULL, FALSE);
   g_return_val_if_fail(*tray_image == NULL, FALSE);

   *tray_icon = egg_tray_icon_new(pch_title);
   g_return_val_if_fail(*tray_icon != NULL, FALSE);

   g_signal_connect(*tray_icon, "embedded",
      G_CALLBACK(cb_panel_systray_icon_activated), gp);
   g_signal_connect(*tray_icon, "destroy",
      G_CALLBACK(cb_panel_systray_icon_destroy), gp);
   g_signal_connect(*tray_icon, "configure-event",
      G_CALLBACK(cb_panel_systray_icon_configure), gp);
   g_signal_connect(*tray_icon, "button-press-event",
      G_CALLBACK(cb_panel_systray_icon_handle_clicked), gp);

   *tray_image = gtk_image_new_from_pixbuf(pixbuf);
   gtk_container_add(GTK_CONTAINER(*tray_icon), *tray_image);
   gtk_widget_show(*tray_image);

   gtk_widget_show_all(GTK_WIDGET(*tray_icon));

   if (tooltips != NULL) {
      gtk_tooltips_set_tip(tooltips, GTK_WIDGET(*tray_icon), pch_title, NULL);
   }

   return TRUE;
}

static gboolean gapc_panel_systray_icon_remove(gpointer gp)
{
   PGAPC_CONFIG pcfg = NULL;
   PGAPC_MONITOR pm = NULL;

   EggTrayIcon **tray_icon = NULL;
   GtkWidget **tray_image = NULL;

   g_return_val_if_fail(gp != NULL, FALSE);

   if (((PGAPC_MONITOR) gp)->cb_id == CB_MONITOR_ID) {
      /* this is a monitor struct (2) */
      pm = (PGAPC_MONITOR) gp;
      tray_icon = &pm->tray_icon;
      tray_image = &pm->tray_image;
   } else {
      /* this is a config struct (1) */
      pcfg = (PGAPC_CONFIG) gp;
      tray_icon = &pcfg->tray_icon;
      tray_image = &pcfg->tray_image;
   }

   g_return_val_if_fail(*tray_icon != NULL, FALSE);
   g_return_val_if_fail(*tray_image != NULL, FALSE);

   gtk_widget_destroy(*tray_image);
   gtk_widget_destroy(GTK_WIDGET(*tray_icon));

   return TRUE;
}

/*
 * Handle the prefs add record button action
*/
static void cb_panel_prefs_button_add_rec(GtkWidget * button, PGAPC_CONFIG pcfg)
{
   g_return_if_fail(pcfg != NULL);

   gapc_panel_preferences_model_rec_add(pcfg);

   return;
}

/*
 * Handle the prefs remove record button action
*/
static void cb_panel_prefs_button_remove_rec(GtkWidget * button, PGAPC_CONFIG pcfg)
{
   g_return_if_fail(pcfg != NULL);

   gapc_panel_preferences_model_rec_remove(pcfg);

   return;
}

/*
 * Handle the prefs use-systray checkbutton action
*/
static void cb_panel_prefs_button_use_systray(GtkWidget * button, PGAPC_CONFIG pcfg)
{
   gboolean b_value = FALSE;
   gchar *pch = NULL;

   g_return_if_fail(pcfg != NULL);

   if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) {
      b_value = TRUE;
   } else {
      b_value = FALSE;
   }

   pch = pcfg->pch_gkeys[GAPC_PREFS_SYSTRAY];
   gconf_client_set_bool(pcfg->client, pch, b_value, NULL);

   return;
}

/*
 * Handled the toggle of a checkbox in the preferences dialog
 * Could be: 
 * enabled - enabled this monitor to run 
 * systray - include the notification tray icon
 * pagers  - remove from task_list and pager_list
*/
static void cb_panel_prefs_handle_cell_toggled(GtkCellRendererToggle * cell,
   gchar * path_str, PGAPC_PREFS_COLUMN pcolumn)
{
   GtkTreeModel *model;
   GtkTreeIter iter;
   GtkTreePath *path = NULL;
   gboolean b_value;
   gint col_number = 0, i_monitor = 0;
   gchar *penabled = NULL;

   g_return_if_fail(pcolumn != NULL);
   g_return_if_fail(path_str != NULL);

   model = pcolumn->prefs_model;
   col_number = pcolumn->i_col_num;

   /* get toggled iter */
   path = gtk_tree_path_new_from_string(path_str);
   gtk_tree_model_get_iter(model, &iter, path);
   gtk_tree_path_free(path);

   /* get the column id they asked for -- hope its boolean */
   gtk_tree_model_get(model, &iter, col_number, &b_value, GAPC_PREFS_MONITOR,
      &i_monitor, -1);

   /* do something with the value */
   b_value ^= 1;

   switch (col_number) {
   case GAPC_PREFS_ENABLED:
      penabled = g_strdup_printf(GAPC_ENABLE_KEY, i_monitor);
      gconf_client_set_bool(pcolumn->client, penabled, b_value, NULL);
      g_free(penabled);
      break;
   case GAPC_PREFS_SYSTRAY:
      penabled = g_strdup_printf(GAPC_SYSTRAY_KEY, i_monitor);
      gconf_client_set_bool(pcolumn->client, penabled, b_value, NULL);
      g_free(penabled);
      break;
   default:
      g_message("Cell_Toggled:Unknown key for Value(%s)\n",
         b_value ? "True" : "False");
      break;
   }

   return;
}

/*
 * A callback routine that collect changes to the path row and save it directly
 * to the desired location, also passes it back to the treeview for display.
 * These data point use this routine.
 * - Host
 * - Port
 * - Graph refresh
 * - Network refresh
 * - Monitor - not a visible field
 *  We update gconf here and gconf updates the
 *  list_store in cb_panel_preference_gconf_changed()
*/
static void cb_panel_prefs_handle_cell_edited(GtkCellRendererText * cell,
   gchar * path_string, gchar * pch_new, PGAPC_PREFS_COLUMN pcolumn)
{
   GtkTreeModel *model;
   GtkTreeIter iter;
   GtkTreePath *path;
   gint col_number = 0, i_port = 0, i_monitor = 0, i_len = 0;
   gfloat f_refresh = 0.0, f_graph = 0.0;
   gchar ch[GAPC_MAX_TEXT], *pch = NULL;
   gboolean b_dupped = FALSE;

   g_return_if_fail(pcolumn != NULL);
   g_return_if_fail(pch_new != NULL);
   g_return_if_fail(path_string != NULL);

   model = pcolumn->prefs_model;
   col_number = pcolumn->i_col_num;

   i_len = g_snprintf(ch, GAPC_MAX_TEXT, "%s", pch_new);
   /*
    * get iter to record
    */
   path = gtk_tree_path_new_from_string(path_string);
   gtk_tree_model_get_iter(model, &iter, path);
   gtk_tree_path_free(path);

   /*
    * get data from that row
    */
   gtk_tree_model_get(model, &iter, GAPC_PREFS_MONITOR, &i_monitor, -1);

   switch (col_number) {
   case GAPC_PREFS_HOST:
      {
         gchar *phost = g_strdup_printf(GAPC_HOST_KEY, i_monitor);

         if ((pch_new == NULL) || (i_len < 2)) {
            pch = g_strdup(GAPC_HOST_DEFAULT);
         } else {
            pch = pch_new;
            b_dupped = TRUE;
         }
         gconf_client_set_string(pcolumn->client, phost, pch, NULL);
         g_free(phost);
      }
      break;
   case GAPC_PREFS_PORT:
      {
         gchar *pport = g_strdup_printf(GAPC_PORT_KEY, i_monitor);

         i_port = (gint) g_strtod(pch_new, NULL);

         if (i_port == 0) {
            i_port = GAPC_PORT_DEFAULT;
            pch = g_strdup_printf("%d", i_port);
         } else {
            pch = pch_new;
            b_dupped = TRUE;
         }
         gconf_client_set_int(pcolumn->client, pport, i_port, NULL);
         g_free(pport);
      }
      break;
   case GAPC_PREFS_WATT:
      {
         gchar *pport = g_strdup_printf(GAPC_WATT_KEY, i_monitor);

         i_port = (gint) g_strtod(pch_new, NULL);

         if (i_port == 0) {
            i_port = GAPC_WATT_DEFAULT;
            pch = g_strdup_printf("%d", i_port);
         } else {
            pch = pch_new;
            b_dupped = TRUE;
         }
         gconf_client_set_int(pcolumn->client, pport, i_port, NULL);
         g_free(pport);
      }
      break;
   case GAPC_PREFS_REFRESH:
      {
         gchar *prefresh = g_strdup_printf(GAPC_REFRESH_KEY, i_monitor);

         f_refresh = (gfloat) g_strtod(pch_new, NULL);

         if (f_refresh < GAPC_REFRESH_MIN_INCREMENT) {
            f_refresh = GAPC_REFRESH_DEFAULT;
            pch = g_strdup_printf("%3.1f", f_refresh);
         } else {
            pch = pch_new;
            b_dupped = TRUE;
         }
         gconf_client_set_float(pcolumn->client, prefresh, f_refresh, NULL);
         g_free(prefresh);
      }
      break;
   case GAPC_PREFS_GRAPH:
      {
         gchar *prefresh = g_strdup_printf(GAPC_GRAPH_KEY, i_monitor);

         f_graph = (gfloat) g_strtod(pch_new, NULL);

         if (f_graph < GAPC_REFRESH_MIN_INCREMENT) {
            f_graph = GAPC_LINEGRAPH_REFRESH_FACTOR;
            pch = g_strdup_printf("%3.1f", f_graph);
         } else {
            pch = pch_new;
            b_dupped = TRUE;
         }
         gconf_client_set_float(pcolumn->client, prefresh, f_graph, NULL);
         g_free(prefresh);
      }
      break;
   default:
      g_message("Cell_Edited:Unknown key for Value(%s)\n", pch_new);
      g_object_set(cell, "text", pch_new, NULL);
      break;
   }

   if ((pch != NULL) && !b_dupped) {
      g_free(pch);
   }

   return;
}

/*
 * Cell data function used to format floating point numbers
 * This gets called a thousand times...
*/
static void cb_panel_prefs_handle_float_format(GtkTreeViewColumn * col,
   GtkCellRenderer * renderer, GtkTreeModel * model, GtkTreeIter * iter, gpointer gp)
{
   gfloat d_value;
   gchar buf[32];
   guint colnum = 0;
   gchar *pch_format = NULL;

   g_return_if_fail(gp != NULL);

   colnum = GPOINTER_TO_UINT(gp);

   pch_format = (gchar *) g_object_get_data(G_OBJECT(col), "float_format");

   gtk_tree_model_get(model, iter, colnum, &d_value, -1);

   if (pch_format) {
      g_snprintf(buf, sizeof(buf), pch_format, d_value);
   } else {
      g_snprintf(buf, sizeof(buf), "%3.0f", d_value);
   }

   g_object_set(renderer, "text", buf, NULL);

   return;
}

/* This routine initializes a user-data structure for use by the renderers of
 * the preference treeview
*/
static PGAPC_PREFS_COLUMN gapc_panel_prefs_col_data_init(PGAPC_CONFIG pcfg,
   GAPC_PrefsType col_num)
{
   PGAPC_PREFS_COLUMN pcol = NULL;

   pcol = g_new0(GAPC_PREFS_COLUMN, 1);
   g_return_val_if_fail(pcol != NULL, NULL);

   pcol->cb_id = CB_COLUMN_ID;
   pcol->prefs_model = pcfg->prefs_model;
   pcol->i_col_num = col_num;
   pcol->client = pcfg->client;

   return pcol;
}

/*
 * Gets the gconf instance preferences for all monitors
 * and loads the prefs_model.
 * returns FALSE on error
 * returns TRUE on sucess
*/
static gboolean gapc_panel_preferences_data_model_load(PGAPC_CONFIG pcfg)
{
   GError *gerror = NULL;
   GSList *monitors = NULL;
   GtkTreeIter iter;
   gboolean b_valid = FALSE;
   gboolean v_enabled;
   gboolean v_use_systray;
   gint v_port_number;
   gint v_watt_number;   
   gfloat v_network_interval;
   gfloat v_graph_interval;
   gchar *v_host_name;

   gchar k_enabled[GAPC_MAX_TEXT];
   gchar k_use_systray[GAPC_MAX_TEXT];
   gchar k_port_number[GAPC_MAX_TEXT];
   gchar k_network_interval[GAPC_MAX_TEXT];
   gchar k_graph_interval[GAPC_MAX_TEXT];
   gchar k_host_name[GAPC_MAX_TEXT];
   gchar k_watt_number[GAPC_MAX_TEXT];   

   g_return_val_if_fail(pcfg != NULL, FALSE);
   g_return_val_if_fail(pcfg->client != NULL, FALSE);
   g_return_val_if_fail(pcfg->prefs_model != NULL, FALSE);

   b_valid = gconf_client_dir_exists(pcfg->client, GAPC_MID_GROUP_KEY, &gerror);
   if (gerror != NULL) {
      gapc_util_log_app_msg("gapc_panel_preferences_data_model_load",
         "gconf_dir_exists() Failed", gerror->message);
      g_error_free(gerror);
      gerror = NULL;
      return FALSE;
   }

   
   if (b_valid == FALSE) {
      gapc_util_log_app_msg("gapc_panel_preferences_data_model_load",
         "No monitors predefined.", "very first startup");
      return FALSE;
   }

   monitors = gconf_client_all_dirs(pcfg->client, GAPC_MID_GROUP_KEY, &gerror);
   if (gerror != NULL) {
      gapc_util_log_app_msg("gapc_panel_preferences_data_model_load",
         "gconf_client_all_dirs() Failed", gerror->message);
      g_error_free(gerror);
      gerror = NULL;
      return FALSE;
   }

   while (monitors) {              /* should be the regular text key */
      gchar *pmon = NULL;
      gint i_monitor = 0;
      GtkWidget *widget = NULL;

      pmon = g_strrstr((gchar *) monitors->data, "/");
      if (pmon) {
         i_monitor = (gint) g_strtod(pmon + 1, NULL);
         pcfg->prefs_last_monitor = MAX(pcfg->prefs_last_monitor, i_monitor);
      } else {
         g_free(monitors->data);
         monitors = g_slist_next(monitors);
         continue;
      }

      g_snprintf(k_enabled, GAPC_MAX_TEXT, "%s/%s", (gchar *) monitors->data,
         "enabled");
      g_snprintf(k_use_systray, GAPC_MAX_TEXT, "%s/%s", (gchar *) monitors->data,
         "use_systray");
      g_snprintf(k_port_number, GAPC_MAX_TEXT, "%s/%s", (gchar *) monitors->data,
         "port_number");
      g_snprintf(k_watt_number, GAPC_MAX_TEXT, "%s/%s", (gchar *) monitors->data,
         "ups_wattage");
      g_snprintf(k_network_interval, GAPC_MAX_TEXT, "%s/%s",
         (gchar *) monitors->data, "network_interval");
      g_snprintf(k_graph_interval, GAPC_MAX_TEXT, "%s/%s", (gchar *) monitors->data,
         "graph_interval");
      g_snprintf(k_host_name, GAPC_MAX_TEXT, "%s/%s", (gchar *) monitors->data,
         "host_name");

      v_enabled = gconf_client_get_bool(pcfg->client, k_enabled, NULL);
      v_use_systray = gconf_client_get_bool(pcfg->client, k_use_systray, NULL);
      v_port_number = gconf_client_get_int(pcfg->client, k_port_number, NULL);
      if (v_port_number == 0) {
         v_port_number = GAPC_PORT_DEFAULT;
      }
      v_watt_number = gconf_client_get_int(pcfg->client, k_watt_number, NULL);
      if (v_watt_number == 0) {
         v_watt_number = GAPC_WATT_DEFAULT;
      }
      v_network_interval =
         gconf_client_get_float(pcfg->client, k_network_interval, NULL);
      if (v_network_interval == 0.0) {
         v_network_interval = GAPC_REFRESH_DEFAULT;
      }
      v_graph_interval =
         gconf_client_get_float(pcfg->client, k_graph_interval, NULL);
      if (v_graph_interval == 0.0) {
         v_graph_interval = GAPC_LINEGRAPH_REFRESH_FACTOR;
      }
      v_host_name = gconf_client_get_string(pcfg->client, k_host_name, NULL);
      if (v_host_name == NULL) {
         v_host_name = g_strdup(GAPC_HOST_DEFAULT);
      }

      gtk_list_store_append(GTK_LIST_STORE(pcfg->prefs_model), &iter);
      gtk_list_store_set(GTK_LIST_STORE(pcfg->prefs_model), &iter,
         GAPC_PREFS_MONITOR, i_monitor, 
         GAPC_PREFS_SYSTRAY, v_use_systray, 
         GAPC_PREFS_ENABLED, v_enabled,
         GAPC_PREFS_PORT, v_port_number, 
         GAPC_PREFS_REFRESH, v_network_interval,
         GAPC_PREFS_GRAPH, v_graph_interval, 
         GAPC_PREFS_HOST, v_host_name, 
         GAPC_PREFS_WATT, v_watt_number, 
         -1);

      /* Startup Processing */
      if (v_enabled) {
         widget = gapc_monitor_interface_create(pcfg, i_monitor, &iter);
         if ((widget != NULL) && !v_use_systray ) {
              gtk_window_present( GTK_WINDOW(widget));
         }
      }
      g_free(monitors->data);
      monitors = g_slist_next(monitors);
   }
   g_slist_free(monitors);

   return TRUE;
}

/*
 * Create a data model to hold the preferences for this program.  We are 
 * using the list store model to hold the all data. Then we create the
 * complete GtkTreeView and initialize the columns.
 * returns GtkTreeView or NULL
*/
static GtkWidget *gapc_panel_preferences_model_init(PGAPC_CONFIG pcfg)
{
   GtkWidget *treeview = NULL;
   GtkTreeModel *model = NULL;
   GtkTreeViewColumn *column = NULL;
   GtkCellRenderer *renderer_enabled = NULL, *renderer_systray = NULL,
      *renderer_port = NULL, *renderer_watt = NULL, *renderer_refresh = NULL,
      *renderer_graph = NULL, *renderer_host = NULL, *anyrndr = NULL;
   PGAPC_PREFS_COLUMN col_enabled = NULL, col_systray = NULL, col_port = NULL,
      col_refresh = NULL, col_graph = NULL, col_host = NULL, col_watt = NULL;

   g_return_val_if_fail(pcfg != NULL, NULL);

   /* Don't create it twice */
   if (pcfg->prefs_treeview != NULL)
      return GTK_WIDGET(pcfg->prefs_treeview);

   /* Create the model -- this column order and that of the enum must match */
   model = GTK_TREE_MODEL(gtk_list_store_new(GAPC_N_PREFS_COLUMNS, 
         G_TYPE_INT,  /* Monitor base-1 */
         G_TYPE_BOOLEAN,           /* enabled */
         G_TYPE_BOOLEAN,           /* systray icon */
         G_TYPE_INT,               /* Port  */
         G_TYPE_FLOAT,             /* network Refresh */
         G_TYPE_FLOAT,             /* graph Refresh */
         G_TYPE_STRING,            /* Host  */
         G_TYPE_INT                /* Wattage  */
      ));
   /* store it for later */
   pcfg->prefs_model = model;

   /* load the data model */
   gapc_panel_preferences_data_model_load(pcfg);

   /* create the display columns and treeview */
   treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
   gtk_tree_view_columns_autosize(GTK_TREE_VIEW(treeview));
   gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), TRUE);
   gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), FALSE);

   /* allocate control struct for viewable columns */
   col_enabled = gapc_panel_prefs_col_data_init(pcfg, GAPC_PREFS_ENABLED);
   col_systray = gapc_panel_prefs_col_data_init(pcfg, GAPC_PREFS_SYSTRAY);
   col_port = gapc_panel_prefs_col_data_init(pcfg, GAPC_PREFS_PORT);
   col_watt = gapc_panel_prefs_col_data_init(pcfg, GAPC_PREFS_WATT);
   col_refresh = gapc_panel_prefs_col_data_init(pcfg, GAPC_PREFS_REFRESH);
   col_graph = gapc_panel_prefs_col_data_init(pcfg, GAPC_PREFS_GRAPH);
   col_host = gapc_panel_prefs_col_data_init(pcfg, GAPC_PREFS_HOST);

   /* create display formatters where needed */
   renderer_enabled = gtk_cell_renderer_toggle_new();
   renderer_systray = gtk_cell_renderer_toggle_new();
   renderer_port = gtk_cell_renderer_text_new();
   renderer_watt = gtk_cell_renderer_text_new();
   renderer_refresh = gtk_cell_renderer_text_new();
   renderer_graph = gtk_cell_renderer_text_new();
   renderer_host = gtk_cell_renderer_text_new();
   anyrndr = gtk_cell_renderer_text_new();

   /* set renderers attributes */
   g_object_set(G_OBJECT(renderer_port), "xalign", 0.5, "editable", TRUE, NULL);
   g_object_set(G_OBJECT(renderer_watt), "xalign", 0.5, "editable", TRUE, NULL);
   g_object_set(G_OBJECT(renderer_graph), "xalign", 0.5, "editable", TRUE, NULL);
   g_object_set(G_OBJECT(renderer_refresh), "xalign", 0.5, "editable", TRUE, NULL);
   g_object_set(G_OBJECT(renderer_host), "editable", TRUE, NULL);


   /* prepare callbacks to correctly handle the columns
    * careful to use these only once per column --
    * using an auto g_free to release allocated storage
    */
   gtk_signal_connect_full(GTK_OBJECT(renderer_enabled), "toggled",
      G_CALLBACK(cb_panel_prefs_handle_cell_toggled), NULL,
      col_enabled, g_free, FALSE, TRUE);
   gtk_signal_connect_full(GTK_OBJECT(renderer_systray), "toggled",
      G_CALLBACK(cb_panel_prefs_handle_cell_toggled), NULL,
      col_systray, g_free, FALSE, TRUE);
   gtk_signal_connect_full(GTK_OBJECT(renderer_port), "edited",
      G_CALLBACK(cb_panel_prefs_handle_cell_edited), NULL,
      col_port, g_free, FALSE, TRUE);
   gtk_signal_connect_full(GTK_OBJECT(renderer_watt), "edited",
      G_CALLBACK(cb_panel_prefs_handle_cell_edited), NULL,
      col_watt, g_free, FALSE, TRUE);
   gtk_signal_connect_full(GTK_OBJECT(renderer_refresh), "edited",
      G_CALLBACK(cb_panel_prefs_handle_cell_edited), NULL,
      col_refresh, g_free, FALSE, TRUE);
   gtk_signal_connect_full(GTK_OBJECT(renderer_graph), "edited",
      G_CALLBACK(cb_panel_prefs_handle_cell_edited), NULL,
      col_graph, g_free, FALSE, TRUE);
   gtk_signal_connect_full(GTK_OBJECT(renderer_host), "edited",
      G_CALLBACK(cb_panel_prefs_handle_cell_edited), NULL,
      col_host, g_free, FALSE, TRUE);

   /* Define the column order and attributes */
   column = gtk_tree_view_column_new_with_attributes("Enabled", renderer_enabled,
      "active", GAPC_PREFS_ENABLED, NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

   column = gtk_tree_view_column_new_with_attributes("use\nTrayIcon",
      renderer_systray, "active", GAPC_PREFS_SYSTRAY, NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

   column = gtk_tree_view_column_new_with_attributes("network\nRefresh",
      renderer_refresh, "text", GAPC_PREFS_REFRESH, NULL);
   g_object_set_data(G_OBJECT(column), "float_format", "%3.1f");
   gtk_tree_view_column_set_cell_data_func(column, renderer_refresh,
      cb_panel_prefs_handle_float_format,
      GUINT_TO_POINTER(GAPC_PREFS_REFRESH), NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

   column = gtk_tree_view_column_new_with_attributes("Port", renderer_port, "text",
      GAPC_PREFS_PORT, NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

   column = gtk_tree_view_column_new_with_attributes("graph\nRefresh",
      renderer_graph, "text", GAPC_PREFS_GRAPH, NULL);
   g_object_set_data(G_OBJECT(column), "float_format", "%3.0f");
   gtk_tree_view_column_set_cell_data_func(column, renderer_graph,
      cb_panel_prefs_handle_float_format, GUINT_TO_POINTER(GAPC_PREFS_GRAPH), NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

   column = gtk_tree_view_column_new_with_attributes("Rated\nWattage", renderer_watt, "text",
      GAPC_PREFS_WATT, NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

   column = gtk_tree_view_column_new_with_attributes("Host Name or IP Address",
      renderer_host, "text", GAPC_PREFS_HOST, NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

   column = gtk_tree_view_column_new_with_attributes("Monitor", anyrndr,
      "text", GAPC_PREFS_MONITOR, NULL);
   gtk_tree_view_column_set_sort_column_id(column, GAPC_PREFS_MONITOR);
   gtk_tree_view_column_clicked(column);
   g_object_set(G_OBJECT(column), "visible", FALSE, NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

   return treeview;
}

/*
 * Create a data model to hold the current list of active monitors.  We are 
 * using the list store model to hold the active data. Then we create the
 * complete GtkTreeView and initialize the columns.
 * returns GtkTreeView or NULL
*/
static GtkWidget *gapc_panel_monitors_model_init(PGAPC_CONFIG pcfg)
{
   GtkWidget *treeview = NULL;
   GtkTreeModel *model = NULL;
   GtkCellRenderer *renderer_icon = NULL, *renderer_text = NULL,
      *renderer_state = NULL, *renderer_any = NULL;
   GtkTreeViewColumn *column = NULL;

   g_return_val_if_fail(pcfg != NULL, NULL);

   /* Don't create it twice */
   if (pcfg->monitor_treeview != NULL)
      return GTK_WIDGET(pcfg->monitor_treeview);

   /* Create the model -- this column order and that of the enum must match */
   model = GTK_TREE_MODEL(gtk_list_store_new(GAPC_N_MON_COLUMNS, G_TYPE_INT,    /* Monitor Num */
         GDK_TYPE_PIXBUF,          /* ICON */
         G_TYPE_STRING,            /* Status Text */
         G_TYPE_POINTER,           /* monitor ptr */
         G_TYPE_STRING             /* State  Text */
      ));
   /* store it for later */
   pcfg->monitor_model = model;

   /* create the display columns and treeview */
   treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
   gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), TRUE);
   gtk_tree_view_columns_autosize(GTK_TREE_VIEW(treeview));
   gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), FALSE);
   g_signal_connect(treeview, "row-activated",
      G_CALLBACK(cb_panel_monitor_list_activated), pcfg);

   renderer_icon = gtk_cell_renderer_pixbuf_new();
   renderer_text = gtk_cell_renderer_text_new();
   renderer_state = gtk_cell_renderer_text_new();
   renderer_any = gtk_cell_renderer_text_new();
   g_object_set(G_OBJECT(renderer_state), "xalign", 0.5, NULL);
   g_object_set(G_OBJECT(renderer_text), "xalign", 0.0, NULL);
   g_object_set(G_OBJECT(renderer_text), "yalign", 0.5, NULL);


   column = gtk_tree_view_column_new();

   gtk_tree_view_column_set_title(column, "Status");

   gtk_tree_view_column_pack_start(column, renderer_icon, TRUE);

   gtk_tree_view_column_add_attribute(column, renderer_icon, "pixbuf",
      GAPC_MON_ICON);

   gtk_tree_view_column_pack_end(column, renderer_state, FALSE);

   gtk_tree_view_column_add_attribute(column, renderer_state, "markup",
      GAPC_MON_UPSSTATE);

   gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

   column = gtk_tree_view_column_new_with_attributes("Current summary ups info",
      renderer_text, "markup", GAPC_MON_STATUS, NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

   column = gtk_tree_view_column_new_with_attributes("Monitor", renderer_any,
      "text", GAPC_MON_MONITOR, NULL);
   gtk_tree_view_column_set_sort_column_id(column, GAPC_MON_MONITOR);
   g_object_set(G_OBJECT(column), "visible", FALSE, NULL);
   gtk_tree_view_column_clicked(column);
   gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);


   return treeview;
}

/*
 * Load ICONs and set default icon
 * return TRUE if ok, FALSE otherwise
*/
static gboolean gapc_util_load_icons(PGAPC_CONFIG pcfg)
{
    guint i_x = 0, x = 0;
    GError *gerror = NULL;
    GdkPixbuf *pixbuf = NULL;
    gboolean b_rc = TRUE;
    gchar pch_file[GAPC_MAX_ARRAY];
    gchar *pch_2 = "./";
    gchar *pch_3 = "../pixmaps/";
    gchar *pch_4 = NULL;
    gchar *pch_image_names[] = {
       "online.png",
       "onbatt.png",
       "charging.png",
       "apcupsd.png",
       "unplugged.png",
       "gapc_prefs.png",
       NULL
    };

    g_return_val_if_fail(pcfg != NULL, FALSE);

    /* build system path for icons */
    pch_4 = g_strconcat (ICON_DIR, "/pixmaps/", NULL);

    i_x = 0;
    while (i_x == 0) {
       if (g_file_test(pch_image_names[0], G_FILE_TEST_EXISTS)) {
          i_x = 1;
          break;
       }

       g_snprintf(pch_file, GAPC_MAX_ARRAY, "%s%s", pch_2, pch_image_names[0]);
       if (g_file_test(pch_file, G_FILE_TEST_EXISTS)) {
          i_x = 2;
          break;
       }

       g_snprintf(pch_file, GAPC_MAX_ARRAY, "%s%s", pch_3, pch_image_names[0]);
       if (g_file_test(pch_file, G_FILE_TEST_EXISTS)) {
          i_x = 3;
          break;
       }

       g_snprintf(pch_file, GAPC_MAX_ARRAY, "%s%s", pch_4, pch_image_names[0]);
       if (g_file_test(pch_file, G_FILE_TEST_EXISTS)) {
          i_x = 4;
          break;
       }

       break;
    }

    if (i_x == 0) {
       gapc_util_log_app_msg("gapc_util_load_icons", "Unable to find icons",
          "--load failed!");
      g_free (pch_4);
      return FALSE;
    }

    for (x = 0; (pch_image_names[x] != NULL) && (x < GAPC_N_ICONS); x++) {
       switch (i_x) {
       case 1:
          g_snprintf(pch_file, GAPC_MAX_ARRAY, "%s", pch_image_names[x]);
          break;
       case 2:
          g_snprintf(pch_file, GAPC_MAX_ARRAY, "%s%s", pch_2, pch_image_names[x]);
          break;
       case 3:
          g_snprintf(pch_file, GAPC_MAX_ARRAY, "%s%s", pch_3, pch_image_names[x]);
          break;
       case 4:
          g_snprintf(pch_file, GAPC_MAX_ARRAY, "%s%s", pch_4, pch_image_names[x]);
          break;
       default:
          g_return_val_if_reached(FALSE);
          break;
       }

       pixbuf = gdk_pixbuf_new_from_file(pch_file, &gerror);
       if (gerror != NULL) {
          gchar *pch = NULL;

          pch = g_strdup_printf("Get Icon=%s Failed", pch_file);
          gapc_util_log_app_msg("gapc_util_load_icons", pch, gerror->message);
          g_error_free(gerror);
          g_free(pch);
          gerror = NULL;
          b_rc = FALSE;
          pcfg->my_icons[x] = NULL;
       } else {
          pcfg->my_icons[x] =
             gdk_pixbuf_scale_simple(pixbuf, GAPC_ICON_SIZE, GAPC_ICON_SIZE,
             GDK_INTERP_BILINEAR);
          g_object_unref(pixbuf);
       }
    }

    g_free (pch_4);
    return b_rc;
}

/* 
 * Monitor List "row-activated"
*/
static void cb_panel_monitor_list_activated(GtkTreeView * treeview,
   GtkTreePath * arg1, GtkTreeViewColumn * arg2, PGAPC_CONFIG pcfg)
{
   PGAPC_MONITOR pm = NULL;
   GtkTreeIter iter;

   g_return_if_fail(pcfg != NULL);

   if (gtk_tree_model_get_iter(pcfg->monitor_model, &iter, arg1)) {
      gtk_tree_model_get(pcfg->monitor_model, &iter, GAPC_MON_POINTER, &pm, -1);

      if ((pm != NULL) && (pm->window != NULL)) {
         if (pm->b_visible) {
            gtk_widget_hide(GTK_WIDGET(pm->window));
         } else {
            gtk_window_present(GTK_WINDOW(pm->window));
         }
      }
   }

   return;
}

/* 
 * Active monitor selection 
 */
static void cb_panel_monitor_list_selection(GtkTreeSelection * selection,
   PGAPC_CONFIG pcfg)
{
   GtkTreeIter iter;
   GtkTreeModel *model;
   gint i_monitor = 0;

   g_return_if_fail(pcfg != NULL);

   if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
      gtk_tree_model_get(model, &iter, GAPC_MON_MONITOR, &i_monitor, -1);

      pcfg->cb_last_monitor = i_monitor;
   }

   return;
}

/*
 * The Active Monitor Icon List for the information window
 * returns created notebook page number.
 */
static gint gapc_panel_monitor_list_page(PGAPC_CONFIG pcfg, GtkNotebook * notebook)
{
   GtkWidget *label = NULL, *frame = NULL, *vbox = NULL, *sw = NULL;
   GtkWidget *treeview = NULL;
   GtkTreeSelection *select = NULL;
   GtkTreeIter iter;

   gint i_page = 0;

   g_return_val_if_fail(pcfg != NULL, -1);
   g_return_val_if_fail(notebook != NULL, -1);

   /* Create notebook page */
   frame = gtk_frame_new(NULL);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
   label = gtk_label_new("Active Monitors");
   i_page = gtk_notebook_append_page(notebook, frame, label);
   gtk_widget_show(frame);

   label = gtk_label_new("<span foreground=\"blue\">"
      "<i>double-click a row to popup information window.</i>" "</span>");
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);

   vbox = gtk_event_box_new();
   gtk_container_add(GTK_CONTAINER(frame), vbox);
   gtk_widget_show(vbox);
   frame = gtk_frame_new("");
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
   gtk_frame_set_label_widget(GTK_FRAME(frame), label);
   gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.8);
   gtk_container_add(GTK_CONTAINER(vbox), frame);
   gtk_widget_show(frame);

   /* Create the container for the icon view */
   sw = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
      GTK_SHADOW_ETCHED_IN);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER,
      GTK_POLICY_AUTOMATIC);
   gtk_container_add(GTK_CONTAINER(frame), sw);
   gtk_widget_show(sw);

   /*  create the active monitor list in a treeview */
   treeview = gapc_panel_monitors_model_init(pcfg);
   pcfg->monitor_treeview = GTK_TREE_VIEW(treeview);
   gtk_container_add(GTK_CONTAINER(sw), treeview);
   gtk_widget_show(GTK_WIDGET(treeview));

   /* Setup the selection handler */
   select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
   pcfg->monitor_select = select;
   gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
   g_signal_connect(G_OBJECT(select), "changed",
      G_CALLBACK(cb_panel_monitor_list_selection), pcfg);

   /* Selection the first record */
   if (gtk_tree_model_get_iter_first(pcfg->monitor_model, &iter)) {
      gtk_tree_selection_select_iter(select, &iter);
   }

   return i_page;
}

/*
 * The Preferences List for the information window
 * returns created notebook page number.
 */
static gint gapc_panel_preferences_page(PGAPC_CONFIG pcfg, GtkNotebook * notebook)
{
   GtkWidget *label = NULL, *frame = NULL, *vbox = NULL, *sw = NULL;
   GtkWidget *pbox = NULL, *box = NULL, *cbox = NULL;
   GtkWidget *treeview = NULL;
   GtkTreeSelection *select = NULL;
   GtkTreeIter iter;
   gint i_page = 0;

   g_return_val_if_fail(pcfg != NULL, -1);
   g_return_val_if_fail(notebook != NULL, -1);

   /* Create notebook page */
   frame = gtk_frame_new(NULL);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
   label = gtk_label_new("Preferences");
   i_page = gtk_notebook_append_page(notebook, frame, label);
   gtk_widget_show(frame);

   label = gtk_label_new("<span foreground=\"blue\">"
      "<i>double-click a columns value to change it.</i>" "</span>");
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);

   box = gtk_event_box_new();
   gtk_container_add(GTK_CONTAINER(frame), box);
   gtk_widget_show(box);
   vbox = gtk_vbox_new(FALSE, 0);
   gtk_container_add(GTK_CONTAINER(box), vbox);
   gtk_widget_show(vbox);
   frame = gtk_frame_new("");
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
   gtk_frame_set_label_widget(GTK_FRAME(frame), label);
   gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.8);
   gtk_container_add(GTK_CONTAINER(vbox), frame);
   gtk_widget_show(frame);
   pbox = gtk_vbox_new(FALSE, 0);
   gtk_container_add(GTK_CONTAINER(frame), pbox);
   gtk_widget_show(pbox);

   /* Create the container for the icon view */
   sw = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
      GTK_SHADOW_ETCHED_IN);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER,
      GTK_POLICY_AUTOMATIC);
   gtk_container_add(GTK_CONTAINER(pbox), sw);
   gtk_widget_show(sw);

   /*  create the preferences in a treeview */
   treeview = gapc_panel_preferences_model_init(pcfg);
   pcfg->prefs_treeview = GTK_TREE_VIEW(treeview);
   gtk_container_add(GTK_CONTAINER(sw), treeview);
   gtk_widget_show(GTK_WIDGET(treeview));

   /* Setup the selection handler */
   select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
   pcfg->prefs_select = select;
   gtk_tree_selection_set_mode(select, GTK_SELECTION_BROWSE);

   /* Select the first record */
   if (gtk_tree_model_get_iter_first(pcfg->prefs_model, &iter)) {
      gtk_tree_selection_select_iter(select, &iter);
   }

   /* add options for adding monitors */
   box = gtk_hbox_new(FALSE, 4);
   gtk_box_pack_start(GTK_BOX(pbox), box, FALSE, FALSE, 0);
   gtk_widget_show(box);

   cbox = gtk_button_new_from_stock(GTK_STOCK_ADD);
   g_signal_connect(cbox, "clicked", G_CALLBACK(cb_panel_prefs_button_add_rec),
      pcfg);
   gtk_tooltips_set_tip(pcfg->tooltips, GTK_WIDGET(cbox),
      "Adds a new monitor\ndefinition to the system.", NULL);
   gtk_box_pack_start(GTK_BOX(box), cbox, FALSE, FALSE, 2);
   gtk_widget_show(cbox);

   cbox = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
   g_signal_connect(cbox, "clicked", G_CALLBACK(cb_panel_prefs_button_remove_rec),
      pcfg);
   gtk_tooltips_set_tip(pcfg->tooltips, GTK_WIDGET(cbox),
      "Removes selected monitor\ndefinition from the system.", NULL);
   gtk_box_pack_start(GTK_BOX(box), cbox, FALSE, FALSE, 2);
   gtk_widget_show(cbox);

   /* add options for control panel */
   frame = gtk_frame_new("Control panel options");
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
   gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
   gtk_widget_show(frame);

   box = gtk_hbox_new(FALSE, 4);
   gtk_container_add(GTK_CONTAINER(frame), box);
   gtk_widget_show(box);

   cbox = gtk_check_button_new_with_mnemonic("Use _tray Icon");
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbox), pcfg->b_use_systray);
   g_signal_connect(cbox, "toggled", G_CALLBACK(cb_panel_prefs_button_use_systray),
      pcfg);
   gtk_tooltips_set_tip(pcfg->tooltips, GTK_WIDGET(cbox),
      "Creates a notification area icon\nfor this control panel.", NULL);
   gtk_box_pack_start(GTK_BOX(box), cbox, FALSE, FALSE, 2);
   gtk_widget_show(cbox);
   g_hash_table_insert(pcfg->pht_Widgets, g_strdup("UseTrayIcon"), cbox);

   return i_page;
}

/*
 * The about page in the information window
 * returns created notebook page number.
 */
static gint gapc_panel_about_page(GtkNotebook * notebook, gchar * pch_pname,
   gchar * pch_pversion, GdkPixbuf * icon)
{
   GtkWidget *label = NULL, *frame = NULL, *vbox = NULL;
   GtkWidget *hbox = NULL, *image = NULL;
   gchar *about_text = NULL;
   gchar *about_msg = NULL;
   GdkPixbuf *scaled = NULL;
   gint i_page = 0;

   g_return_val_if_fail(notebook != NULL, -1);


   about_text =
      g_strdup_printf("<b><big>%s</big>\nVersion %s</b>\n", pch_pname, pch_pversion);

   about_msg =
      g_strdup_printf("<b>gui monitor for UPSs under the management"
      " of the APCUPSD.sourceforge.net package</b>\n"
      "<i>http://gapcmon.sourceforge.net/</i>\n\n"
      "Copyright \xC2\xA9 2006 James Scott, Jr.\n"
      "skoona@users.sourceforge.net\n\n"
      "Released under the GNU Public License\n"
      "%s comes with\nABSOLUTELY NO WARRANTY", pch_pname);

   /* Create About page */
   frame = gtk_frame_new(NULL);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
   label = gtk_label_new("About");
   i_page = gtk_notebook_append_page(notebook, frame, label);
   gtk_widget_show(frame);

   vbox = gtk_vbox_new(FALSE, 8);
   gtk_container_add(GTK_CONTAINER(frame), vbox);
   gtk_widget_show(vbox);

   hbox = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
   gtk_widget_show(hbox);

   image = gtk_image_new();
   gtk_misc_set_alignment((GtkMisc *) image, 1.0, 0.5);
   scaled = gdk_pixbuf_scale_simple(icon, 48, 48, GDK_INTERP_BILINEAR);
   gtk_image_set_from_pixbuf(GTK_IMAGE(image), scaled);
   gtk_box_pack_start(GTK_BOX(hbox), image, TRUE, TRUE, 0);
   gtk_widget_show(image);
   gdk_pixbuf_unref(scaled);

   label = gtk_label_new(about_text);
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
   gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
   gtk_misc_set_alignment((GtkMisc *) label, 0.0, 0.7);
   gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
   gtk_widget_show(label);

   label = gtk_label_new(about_msg);
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
   gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
   gtk_misc_set_alignment((GtkMisc *) label, 0.5, 0.5);
   gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
   gtk_widget_show(label);

   g_free(about_text);
   g_free(about_msg);

   return i_page;
}

static void cb_monitor_interface_show(GtkWidget * widget, PGAPC_MONITOR pm)
{
   g_return_if_fail(pm != NULL);
   pm->b_visible = TRUE;
   lg_graph_draw ( pm->phs.plg );
}

static void cb_monitor_interface_hide(GtkWidget * widget, PGAPC_MONITOR pm)
{
   g_return_if_fail(pm != NULL);
   pm->b_visible = FALSE;
}

static gboolean cb_monitor_interface_delete_event(GtkWidget * widget,
   GdkEvent * event, PGAPC_MONITOR pm)
{
   g_return_val_if_fail(pm != NULL, FALSE);

   return gtk_widget_hide_on_delete(widget);
}

/*
 * Handle the close button action from the information window
*/
/*
static void cb_monitor_interface_button_close(GtkWidget * button, PGAPC_MONITOR pm)
{
   g_return_if_fail(pm != NULL);
   gtk_widget_hide(GTK_WIDGET(pm->window));
   return;
}
*/

/*
 * Handle the refresh button action from the information window
*/
static void cb_monitor_interface_button_refresh(GtkWidget * button, PGAPC_MONITOR pm)
{
   g_return_if_fail(pm != NULL);

   if ((!pm->b_run) || !(pm->cb_enabled) || (pm->window == NULL)) {
      return;
   }

   g_async_queue_push(pm->q_network, pm);
   g_timeout_add(GAPC_REFRESH_FACTOR_ONE_TIME,
      (GSourceFunc) cb_monitor_dedicated_one_time_refresh, pm);

   return;
}

static void cb_main_interface_show(GtkWidget * widget, PGAPC_CONFIG pcfg)
{
   g_return_if_fail(pcfg != NULL);
   pcfg->b_visible = TRUE;
}

static void cb_main_interface_hide(GtkWidget * widget, PGAPC_CONFIG pcfg)
{
   g_return_if_fail(pcfg != NULL);
   pcfg->b_visible = FALSE;
}

/* "window-state-event"
 * iconify/minimize verus hide needs this routine to manage visibility 
*/          
static gboolean cb_util_manage_iconify_event(GtkWidget *widget, GdkEventWindowState *event,
                                                gpointer  gp)
{
   g_return_val_if_fail(gp != NULL, FALSE);
   
   /* iconified */
   if ((event->type == GDK_WINDOW_STATE) && (
      ((event->changed_mask & GDK_WINDOW_STATE_ICONIFIED) && (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED)) ||
      ((event->changed_mask & GDK_WINDOW_STATE_WITHDRAWN) && (event->new_window_state & GDK_WINDOW_STATE_WITHDRAWN)) 
                                           )){
        if ( ((PGAPC_MONITOR)gp)->cb_id == CB_MONITOR_ID) {
              if ( event->window == GTK_WIDGET(((PGAPC_MONITOR)gp)->window)->window ) {   
                   ((PGAPC_MONITOR)gp)->b_visible = FALSE;
              }
        } else {
              if ( event->window == GTK_WIDGET(((PGAPC_CONFIG)gp)->window)->window ) {   
                 ((PGAPC_CONFIG)gp)->b_visible = FALSE;
              }
        }

    return TRUE;
   }

   /* un - iconified */
   if ((event->type == GDK_WINDOW_STATE) &&   ( 
      ((event->changed_mask & GDK_WINDOW_STATE_ICONIFIED) && !(event->new_window_state & GDK_WINDOW_STATE_ICONIFIED)) || 
      ((event->changed_mask & GDK_WINDOW_STATE_WITHDRAWN) && !(event->new_window_state & GDK_WINDOW_STATE_WITHDRAWN)))
                                              ) {
        if ( ((PGAPC_MONITOR)gp)->cb_id == CB_MONITOR_ID) {
              if ( event->window == GTK_WIDGET(((PGAPC_MONITOR)gp)->window)->window ) {   
                   ((PGAPC_MONITOR)gp)->b_visible = TRUE;
              }
        } else {
              if ( event->window == GTK_WIDGET(((PGAPC_CONFIG)gp)->window)->window ) {   
                 ((PGAPC_CONFIG)gp)->b_visible = TRUE;
              }
        }

    return TRUE;
   }

   return FALSE;
}
                                                                                  
static gboolean cb_main_interface_delete_event(GtkWidget * widget, GdkEvent * event,
   PGAPC_CONFIG pcfg)
{
   g_return_val_if_fail(pcfg != NULL, FALSE);
   cb_main_interface_button_quit(widget, pcfg);
   return FALSE;
}

/*
 * Handle the quit button action from the information window
*/
static void cb_main_interface_button_quit(GtkWidget * button, PGAPC_CONFIG pcfg)
{
   GtkTreeIter iter;
   PGAPC_MONITOR pm = NULL;
   gboolean valid = FALSE;

   g_return_if_fail(pcfg != NULL);

   valid = gtk_tree_model_get_iter_first(pcfg->monitor_model, &iter);
   while (valid) {
      gtk_tree_model_get(pcfg->monitor_model, &iter, GAPC_MON_POINTER, &pm, -1);
      if ((pm != NULL) && (pm->window != NULL)) {
         pm->b_run = FALSE;
         gtk_widget_destroy(GTK_WIDGET(pm->window));
         pm->window = NULL;
         pm = NULL;
      }
      valid = gtk_tree_model_iter_next(pcfg->monitor_model, &iter);
   }

   gtk_widget_destroy(GTK_WIDGET(pcfg->window));
   return;
}

/*
 * GConf2 routine
 * Handles changes to use_systray and skip_pager for the control panel.
 * Triggers for this routine should not be installed until after the 
 * control panel has been created. 
*/
static void cb_panel_controller_gconf_changed(GConfClient * client, guint cnxn_id,
   GConfEntry * entry, PGAPC_CONFIG pcfg)
{
   gboolean b_new_value = FALSE;
   GtkWidget *cbox = NULL;
   gchar  const *pstring = NULL;
   GdkColor *pcolor = NULL;

   g_return_if_fail(pcfg != NULL);
   g_return_if_fail(entry != NULL);
   g_return_if_fail(entry->value != NULL);
   g_return_if_fail(pcfg->window != NULL);

   switch (entry->value->type) {
 	case GCONF_VALUE_STRING:
		/* take action to propagate the value to all monitors */
	    pstring = gconf_value_get_string(entry->value);
	    if (pstring == NULL) {
		    break;
		}
		pcolor = NULL;
	    if (g_str_equal(entry->key, GAPC_COLOR_LINEV_KEY)) {
	    	pcolor = &pcfg->color_linev;
	    	cbox = g_hash_table_lookup(pcfg->pht_Widgets, "color-linev");
	    }
	    if (g_str_equal(entry->key, GAPC_COLOR_LOADPCT_KEY)) {
	    	pcolor = &pcfg->color_loadpct;
	    	cbox = g_hash_table_lookup(pcfg->pht_Widgets, "color-loadpct");
	    }
	    if (g_str_equal(entry->key, GAPC_COLOR_TIMELEFT_KEY)) {
	    	pcolor = &pcfg->color_timeleft;
	    	cbox = g_hash_table_lookup(pcfg->pht_Widgets, "color-timeleft");
	    }
	    if (g_str_equal(entry->key, GAPC_COLOR_BCHARGE_KEY)) {
	    	pcolor = &pcfg->color_bcharge;
	    	cbox = g_hash_table_lookup(pcfg->pht_Widgets, "color-bcharge");
	    }
	    if (g_str_equal(entry->key, GAPC_COLOR_BATTV_KEY)) {
	    	pcolor = &pcfg->color_battv;
	    	cbox = g_hash_table_lookup(pcfg->pht_Widgets, "color-battv");
	    }
	    if (g_str_equal(entry->key, GAPC_COLOR_WINDOW_KEY)) {
	    	pcolor = &pcfg->color_window;
	    	cbox = g_hash_table_lookup(pcfg->pht_Widgets, "color-window");
	    }
	    if (g_str_equal(entry->key, GAPC_COLOR_CHART_KEY)) {
	    	pcolor = &pcfg->color_chart;
	    	cbox = g_hash_table_lookup(pcfg->pht_Widgets, "color-chart");
	    }
	    if (g_str_equal(entry->key, GAPC_COLOR_TITLE_KEY)) {
	    	pcolor = &pcfg->color_title;
	    	cbox = g_hash_table_lookup(pcfg->pht_Widgets, "color-title");
	    }
	    if ( pcolor ) {
			gdk_color_parse (pstring, pcolor);
	    }
	    if ( cbox ) {
	    	gtk_color_button_set_color (GTK_COLOR_BUTTON(cbox), pcolor);
	    }
	  	break;
 	case GCONF_VALUE_BOOL:
	    b_new_value = gconf_value_get_bool(entry->value);
	    if (g_str_equal(entry->key, pcfg->pch_gkeys[GAPC_PREFS_SYSTRAY])) {
	       if (pcfg->b_use_systray == b_new_value) {
	           break;
	       }

		   pcfg->b_use_systray = b_new_value;
	       if (b_new_value) {
     	       if (pcfg->tray_icon == NULL) {
           		   gapc_panel_systray_icon_create(pcfg);
		       }
		   } else {
	           if (pcfg->tray_icon != NULL) {
		           gapc_panel_systray_icon_remove(pcfg);
		       }
	       }
	       cbox = g_hash_table_lookup(pcfg->pht_Widgets, "UseTrayIcon");
	       gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbox), pcfg->b_use_systray);
	       break;
	    }
	    break;
     default:
	   gapc_util_log_app_msg("cb_panel_controller_gconf_changed",
						     "(UnKnown Data Type for key)", entry->key);
   }
   
   return;
}

/*
 * GConf2 routine
 * Handles changes to prefs_model, or the master list of monitors in the preferences page.
 * Triggers for this routine should not be installed until after the 
 * control panel has been created. 
*/
static void cb_panel_preferences_gconf_changed(GConfClient * client, guint cnxn_id,
   GConfEntry * entry, PGAPC_CONFIG pcfg)
{
   gchar *pch_value = NULL;
   gchar *pkey = NULL, **pkey_l = NULL;

   gboolean ov_b_tray = FALSE;
   gint ov_i_port = 0;
   gint ov_i_watt = 0;
   gfloat ov_f_graph = 0.0;
   gfloat ov_f_refresh = 0.0;
   gchar *ov_s_host = NULL;


   gint i_monitor = 0;
   gint i_value = 0, i_len = 0;
   gfloat f_value = 0.0;
   gchar *s_value = NULL, ch[GAPC_MAX_TEXT];
   gboolean b_value = FALSE, b_flag_dupped = FALSE;

   gboolean b_ls_valid = FALSE;
   gboolean b_v_valid = FALSE;
   gboolean b_k_valid = FALSE;
   gboolean b_k_is_dir = FALSE;
   gboolean b_m_valid = FALSE;
   gboolean b_m_enabled = FALSE, b_add = TRUE, b_active_valid = FALSE;
   GtkTreeIter iter;
   GtkTreeIter miter;
   PGAPC_MONITOR pm = NULL;

   g_return_if_fail(pcfg != NULL);
   g_return_if_fail(entry != NULL);
   g_return_if_fail(pcfg->window != NULL);

   /* Parse out monitor number and item.key */
   pkey_l = g_strsplit(entry->key, "/", -1);
   if (pkey_l[5] != NULL) {
      pkey = g_strdup(pkey_l[5]);
      b_k_valid = TRUE;
   } else {
      b_k_is_dir = TRUE;
   }
   if (pkey_l[4] != NULL) {
      i_monitor = (gint) g_strtod(pkey_l[4], NULL);
      b_m_valid = TRUE;
   }
   g_strfreev(pkey_l);

   gdk_threads_enter();

   /* Determine control bools */
   b_ls_valid =
      gapc_util_treeview_get_iter_from_monitor(pcfg->prefs_model, &iter, i_monitor);
   if (b_ls_valid) {
      gtk_tree_model_get(pcfg->prefs_model, &iter,
         GAPC_PREFS_ENABLED, &b_m_enabled,
         GAPC_PREFS_SYSTRAY, &ov_b_tray,
         GAPC_PREFS_PORT, &ov_i_port,
         GAPC_PREFS_WATT, &ov_i_watt,
         GAPC_PREFS_GRAPH, &ov_f_graph,
         GAPC_PREFS_REFRESH, &ov_f_refresh, 
         GAPC_PREFS_HOST, &ov_s_host, -1);
   }
   if (entry->value != NULL) {
      pch_value = (gchar *) gconf_value_to_string(entry->value);
      if (pch_value != NULL) {
         b_v_valid = TRUE;
      }
   }

   /* perform record.level operations */
   if (b_ls_valid && !b_k_valid && !b_v_valid) {        /* delete null dir - no key val */
      if (b_m_enabled) {
         gapc_monitor_interface_destroy(pcfg, i_monitor);
         b_m_enabled = FALSE;
      }
      gtk_list_store_remove(GTK_LIST_STORE(pcfg->prefs_model), &iter);
      b_ls_valid = FALSE;
      pcfg->cb_last_monitor_deleted = i_monitor;
   }
   if (i_monitor == pcfg->cb_last_monitor_deleted) {    /* override gconf_unset-kde issue */
      b_k_valid = FALSE;
      b_v_valid = FALSE;
      b_m_enabled = FALSE;
   }
   if (!b_ls_valid && b_v_valid && b_k_valid) { /* add new rec if keys valid -nfound */
      gtk_list_store_append(GTK_LIST_STORE(pcfg->prefs_model), &iter);
      gtk_list_store_set(GTK_LIST_STORE(pcfg->prefs_model), &iter,
         GAPC_PREFS_MONITOR, i_monitor, 
         GAPC_PREFS_SYSTRAY, FALSE,
         GAPC_PREFS_ENABLED, FALSE,
         GAPC_PREFS_PORT, GAPC_PORT_DEFAULT, 
         GAPC_PREFS_GRAPH, GAPC_LINEGRAPH_REFRESH_FACTOR, 
         GAPC_PREFS_HOST, GAPC_HOST_DEFAULT, 
         GAPC_PREFS_REFRESH, GAPC_REFRESH_DEFAULT, 
         GAPC_PREFS_WATT, GAPC_WATT_DEFAULT, 
         -1);

      b_ls_valid = TRUE;
      b_add = TRUE;
      b_m_enabled = FALSE;
   }

   /* perform cell.level operations */
   if (b_ls_valid && b_v_valid && b_k_valid) {

      b_active_valid =
         gapc_util_treeview_get_iter_from_monitor(pcfg->monitor_model, &miter,
         i_monitor);
      if (b_active_valid) {
         gtk_tree_model_get(pcfg->monitor_model, &miter, GAPC_MON_POINTER, &pm, -1);
      } else {
         b_active_valid = FALSE;
      }

      if ((pm == NULL) || (pm->window == NULL)) {
         b_active_valid = FALSE;
      }

      if (g_str_equal(pkey, "enabled")) {
         b_value = gconf_value_get_bool(entry->value);
         if (b_value != b_m_enabled) {
            gtk_list_store_set(GTK_LIST_STORE(pcfg->prefs_model), &iter,
               GAPC_PREFS_ENABLED, b_value, -1);
         }
         if (b_value && !b_m_enabled && !b_active_valid) {
            gtk_widget_show_all(gapc_monitor_interface_create(pcfg, i_monitor,
                  &iter));
         }
         if (!b_value && b_m_enabled && b_active_valid) {
            gapc_monitor_interface_destroy(pcfg, i_monitor);
         }
      }
      if (g_str_equal(pkey, "use_systray")) {
         b_value = gconf_value_get_bool(entry->value);
         if (b_value != ov_b_tray) {
            gtk_list_store_set(GTK_LIST_STORE(pcfg->prefs_model), &iter,
               GAPC_PREFS_SYSTRAY, b_value, -1);
         }
         if ((b_m_enabled) && (b_active_valid)) {
            pm->cb_use_systray = b_value;
            if (b_value) {
               gapc_panel_systray_icon_create(pm);
            } else {
               gapc_panel_systray_icon_remove(pm);
            }
         }
      }
      if (g_str_equal(pkey, "port_number")) {
         i_value = gconf_value_get_int(entry->value);
         if (i_value != ov_i_port) {
            gtk_list_store_set(GTK_LIST_STORE(pcfg->prefs_model), &iter,
               GAPC_PREFS_PORT, i_value, -1);
         }
         if ((b_m_enabled) && (b_active_valid)) {
            pm->i_port = i_value;
            pm->b_network_control = TRUE;
            if (pm->psk != NULL) {
                pm->psk->i_port = i_value;
                pm->psk->b_network_control = TRUE;
            }
         }
      }
      if (g_str_equal(pkey, "ups_wattage")) {
         i_value = gconf_value_get_int(entry->value);
         if (i_value != ov_i_watt) {
            gtk_list_store_set(GTK_LIST_STORE(pcfg->prefs_model), &iter,
               GAPC_PREFS_WATT, i_value, -1);
         }
         if ( pm != NULL ) {
              pm->i_watt = i_value;
         }
      }
      if (g_str_equal(pkey, "network_interval")) {
         f_value = gconf_value_get_float(entry->value);
         if (f_value != ov_f_refresh) {
            gtk_list_store_set(GTK_LIST_STORE(pcfg->prefs_model), &iter,
               GAPC_PREFS_REFRESH, f_value, -1);
         }
         if ((b_m_enabled) && (b_active_valid)) {
            pm->d_refresh = f_value;
            pm->b_timer_control = TRUE;
         }
      }
      if (g_str_equal(pkey, "graph_interval")) {
         f_value = gconf_value_get_float(entry->value);
         if (f_value != ov_f_graph) {
            gtk_list_store_set(GTK_LIST_STORE(pcfg->prefs_model), &iter,
               GAPC_PREFS_GRAPH, f_value, -1);
         }
         if ((b_m_enabled) && (b_active_valid)) {
            pm->d_graph = f_value;
            pm->b_graph_control = TRUE;
         }
      }
      if (g_str_equal(pkey, "host_name")) {
         s_value = (gchar *) gconf_value_get_string(entry->value);
         i_len = g_snprintf(ch, GAPC_MAX_TEXT, "%s", s_value);
         if (i_len < 2) {
            s_value = g_strdup(GAPC_HOST_DEFAULT);
            b_flag_dupped = TRUE;
         }
         if (!g_str_equal(s_value, ov_s_host)) {
            gtk_list_store_set(GTK_LIST_STORE(pcfg->prefs_model), &iter,
               GAPC_PREFS_HOST, s_value, -1);
         }
         if ((b_m_enabled) && (b_active_valid)) {
            if (pm->pch_host != NULL) {
               g_free(pm->pch_host);
            }
            pm->pch_host = g_strdup(s_value);
            pm->b_network_control = TRUE;
            if (pm->psk != NULL) {
                g_snprintf(pm->psk->ch_ip_string, sizeof(pm->psk->ch_ip_string), "%s", s_value);
                pm->psk->b_network_control = TRUE;
            }
         }
         if (b_flag_dupped) {
            g_free(s_value);
            b_flag_dupped = FALSE;
         }
         if (ov_s_host) {
            g_free(ov_s_host);
         }
      }
   }

   if (pkey != NULL) {
      g_free(pkey);
   }
   if (pch_value != NULL) {
      g_free(pch_value);
   }

   gdk_flush();
   gdk_threads_leave();

   return;
}

/*
 * Clears the gconf directory watchers for the control panel
 * returns FALSE on error
 * returns TRUE on sucess
*/
static gboolean gapc_panel_gconf_destroy(PGAPC_CONFIG pcfg)
{
   g_return_val_if_fail(pcfg != NULL, FALSE);

   if (pcfg->i_group_id > 0) {
      gconf_client_remove_dir(pcfg->client, GAPC_MID_GROUP_KEY, NULL);
      gconf_client_remove_dir(pcfg->client, GAPC_CP_GROUP_KEY, NULL);
      gconf_client_notify_remove(pcfg->client, pcfg->i_group_id);
      gconf_client_notify_remove(pcfg->client, pcfg->i_prefs_id);
   }
   g_object_unref(pcfg->client);

   g_free(pcfg->pch_gkeys[GAPC_PREFS_SYSTRAY]);

   pcfg->i_group_id = 0;
   pcfg->i_prefs_id = 0;
   pcfg->client = NULL;
   pcfg->pch_gkeys[GAPC_PREFS_SYSTRAY] = NULL;

   return TRUE;
}

/*
 * Set the gconf directory watchers for the control panel
 * returns FALSE on error
 * returns TRUE on sucess
*/
static gboolean gapc_panel_gconf_watch(PGAPC_CONFIG pcfg)
{
   GError *gerror = NULL;

   g_return_val_if_fail(pcfg != NULL, FALSE);

   /* Have gconf call us if something does change */
   pcfg->i_group_id =
      gconf_client_notify_add(pcfg->client, GAPC_CP_GROUP_KEY,
      (GConfClientNotifyFunc)
      cb_panel_controller_gconf_changed, pcfg, NULL, &gerror);
   if (gerror != NULL) {
      gapc_util_log_app_msg("gapc_panel_gconf_watch",
         "gconf_client_notify_add(controller.group) Failed", gerror->message);
      g_error_free(gerror);
      pcfg->i_group_id = 0;
      gerror = NULL;
      return FALSE;
   }

   pcfg->i_prefs_id =
      gconf_client_notify_add(pcfg->client, GAPC_MID_GROUP_KEY,
      (GConfClientNotifyFunc)
      cb_panel_preferences_gconf_changed, pcfg, NULL, &gerror);
   if (gerror != NULL) {
      gapc_util_log_app_msg("gapc_panel_gconf_watch",
         "gconf_client_notify_add(prefs.group) Failed", gerror->message);
      g_error_free(gerror);
      pcfg->i_group_id = 0;
      gerror = NULL;
      return FALSE;
   }

   return TRUE;
}

/*
 * Gets the gconf instance preferences for this program
 * and init the control panel values.
 * returns FALSE on error
 * returns TRUE on sucess
*/
static gboolean gapc_panel_gconf_init(PGAPC_CONFIG pcfg)
{
   GError *gerror = NULL;
   gchar *pstring = NULL;

   g_return_val_if_fail(pcfg != NULL, FALSE);

   /* prepare control panel keys */
   pcfg->pch_gkeys[GAPC_PREFS_SYSTRAY] = g_strdup(GAPC_CP_SYSTRAY_KEY);

   /* contact gconf2 */
   pcfg->client = gconf_client_get_default();
   g_return_val_if_fail(pcfg->client != NULL, FALSE);

   /* Have gconf watch for changes in this controller directory */
   gconf_client_add_dir(pcfg->client, GAPC_CP_GROUP_KEY,
      GCONF_CLIENT_PRELOAD_ONELEVEL, &gerror);
   if (gerror != NULL) {
      gapc_util_log_app_msg("gapc_panel_gconf_init", "gconf_client_add_dir() Failed",
         gerror->message);
      g_error_free(gerror);
      gerror = NULL;
      return FALSE;
   }

   /* Have gconf watch for changes in this monitor directory */
   gconf_client_add_dir(pcfg->client, GAPC_MID_GROUP_KEY,
      GCONF_CLIENT_PRELOAD_ONELEVEL, &gerror);
   if (gerror != NULL) {
      gapc_util_log_app_msg("gapc_panel_gconf_init", "gconf_client_add_dir() Failed",
         gerror->message);
      g_error_free(gerror);
      gerror = NULL;
      return FALSE;
   }

   /* Defaults are FALSE */
   pcfg->b_use_systray =
      gconf_client_get_bool(pcfg->client, pcfg->pch_gkeys[GAPC_PREFS_SYSTRAY], NULL);

   /* Load graph colors or set defaults */
   pstring = gconf_client_get_string(pcfg->client, GAPC_COLOR_LINEV_KEY, NULL);
   if (pstring) {
	   gdk_color_parse (pstring, &pcfg->color_linev);
   } else {
	   gdk_color_parse ("green", &pcfg->color_linev);   	
   }
   pstring = gconf_client_get_string(pcfg->client, GAPC_COLOR_LOADPCT_KEY, NULL);
   if (pstring) {
	   gdk_color_parse (pstring, &pcfg->color_loadpct);
   } else {
	   gdk_color_parse ("blue", &pcfg->color_loadpct);   	
   }
   pstring = gconf_client_get_string(pcfg->client, GAPC_COLOR_TIMELEFT_KEY, NULL);
   if (pstring) {
	   gdk_color_parse (pstring, &pcfg->color_timeleft);
   } else {
	   gdk_color_parse ("red", &pcfg->color_timeleft);   	
   }
   pstring = gconf_client_get_string(pcfg->client, GAPC_COLOR_BCHARGE_KEY, NULL);
   if (pstring) {
	   gdk_color_parse (pstring, &pcfg->color_bcharge);
   } else {
	   gdk_color_parse ("yellow", &pcfg->color_bcharge);   	
   }
   pstring = gconf_client_get_string(pcfg->client, GAPC_COLOR_BATTV_KEY, NULL);
   if (pstring) {
	   gdk_color_parse (pstring, &pcfg->color_battv);
   } else {
	   gdk_color_parse ("black", &pcfg->color_battv);   	
   }
   pstring = gconf_client_get_string(pcfg->client, GAPC_COLOR_WINDOW_KEY, NULL);
   if (pstring) {
	   gdk_color_parse (pstring, &pcfg->color_window);
   } else {
	   gdk_color_parse ("white", &pcfg->color_window);   	
   }
   pstring = gconf_client_get_string(pcfg->client, GAPC_COLOR_CHART_KEY, NULL);
   if (pstring) {
	   gdk_color_parse (pstring, &pcfg->color_chart);
   } else {
	   gdk_color_parse ("light blue", &pcfg->color_chart);   	
   }
   pstring = gconf_client_get_string(pcfg->client, GAPC_COLOR_TITLE_KEY, NULL);
   if (pstring) {
	   gdk_color_parse (pstring, &pcfg->color_title);
   } else {
	   gdk_color_parse ("blue", &pcfg->color_title);   	
   }

   return TRUE;
}

/* 
 * Handle Object.Destroy Signal
 * have no choice be go away when destroyed 
*/
static void cb_monitor_interface_destroy(GtkWidget * widget, PGAPC_MONITOR pm)
{
   PGAPC_CONFIG pcfg = (PGAPC_CONFIG)pm->gp;
   GtkWidget *sbar = NULL;
   gint h_index = 0;

   g_return_if_fail(pm != NULL);
   g_return_if_fail(pcfg != NULL);   

   pm->b_run = FALSE;

   if (pm->tid_graph_refresh) {
      g_source_remove(pm->tid_graph_refresh);
   }
   if (pm->tid_automatic_refresh) {
      g_source_remove(pm->tid_automatic_refresh);
   }

   if (pm->tid_thread_qwork != NULL) {
      pm->b_thread_stop = TRUE;
      g_async_queue_push(pm->q_network, pm);
      g_thread_join(pm->tid_thread_qwork);
   }

   g_mutex_free(pm->gm_update);
   g_async_queue_unref(pm->q_network);


   for (h_index = 0; h_index < GAPC_LINEGRAPH_MAX_SERIES; h_index++) {
      if (pm->phs.sq[h_index].gm_graph != NULL) {
         g_mutex_free(pm->phs.sq[h_index].gm_graph);
      }
   }

   if (pm->pht_Widgets != NULL) {
      g_hash_table_destroy(pm->pht_Widgets);
      g_hash_table_destroy(pm->pht_Status);
      pm->pht_Widgets = NULL;
      pm->pht_Status = NULL;
   }

   for (h_index = 0; h_index < GAPC_MAX_ARRAY; h_index++) {
      if (pm->pach_events[h_index] != NULL) {
         g_free(pm->pach_events[h_index]);
      }
      pm->pach_events[h_index] = NULL;
      if (pm->pach_status[h_index] != NULL) {
         g_free(pm->pach_status[h_index]);
      }
      pm->pach_status[h_index] = NULL;
   }

   if (pm->tray_icon != NULL) {
      gtk_widget_destroy(GTK_WIDGET(pm->tray_icon));
      pm->tray_icon = NULL;
      pm->tray_image = NULL;
   }
   if (pm->menu != NULL) {
      gtk_widget_destroy(GTK_WIDGET(pm->menu));
      pm->menu = NULL;
   }

   g_object_unref (pm->tooltips);
   
   lg_graph_data_series_remove_all ( pm->phs.plg );

   sbar = g_hash_table_lookup (pcfg->pht_Widgets, "StatusBar");
   if (sbar) {
      gchar *pch = NULL;
      
      gtk_statusbar_pop(GTK_STATUSBAR(sbar), pcfg->i_info_context);
      pch = g_strdup_printf
         ("Monitor for %s Destroyed!...", pm->pch_host );
      gtk_statusbar_push(GTK_STATUSBAR(sbar), pcfg->i_info_context, pch);
      g_free(pch);
   }
   
   g_free(pm);
   return;
}

static void cb_main_interface_destroy(GtkWidget * widget, PGAPC_CONFIG pcfg)
{
   gint x = 0;

   g_return_if_fail(pcfg != NULL);

   pcfg->b_run = FALSE;

   gapc_panel_gconf_destroy(pcfg);

   if (GTK_IS_TREE_VIEW(pcfg->prefs_treeview)) {
      gtk_widget_destroy(GTK_WIDGET(pcfg->prefs_treeview));
      pcfg->prefs_treeview = NULL;
   }
   if (GTK_IS_TREE_VIEW(pcfg->monitor_treeview)) {
      gtk_widget_destroy(GTK_WIDGET(pcfg->monitor_treeview));
      pcfg->monitor_treeview = NULL;
   }
   if (pcfg->prefs_model != NULL) {
      gtk_list_store_clear(GTK_LIST_STORE(pcfg->prefs_model));
      g_object_unref(G_OBJECT(pcfg->prefs_model));
      pcfg->prefs_model = NULL;
   }
   if (pcfg->monitor_model != NULL) {
      gtk_list_store_clear(GTK_LIST_STORE(pcfg->monitor_model));
      g_object_unref(G_OBJECT(pcfg->monitor_model));
      pcfg->monitor_model = NULL;
   }
   if (pcfg->pht_Widgets != NULL) {
      g_hash_table_destroy(pcfg->pht_Widgets);
      g_hash_table_destroy(pcfg->pht_Status);
   }
   for (x = 0; x < GAPC_N_ICONS; x++) {
      g_object_unref(pcfg->my_icons[x]);
   }

   g_object_unref (pcfg->tooltips);
   
   gtk_main_quit();
   return;
}

/*
 * returns TRUE if helps was offered, else FALSE if input was all ok.
 */
static gint gapc_main_interface_parse_args(gint argc, gchar ** argv,
   PGAPC_CONFIG pcfg)
{
   gchar *pch = NULL;
   GString *gs_parm1 = NULL, *gs_parm2 = NULL;

   gs_parm1 = g_string_new(GAPC_CP_GROUP_KEY);
   gs_parm2 = g_string_new(GAPC_CP_GROUP_KEY);

   /* *********************************************************** *
    *  Get user input
    *   - default to known values
    *   - check config file for saved values -- careful not to override cmdline
    */
   while (--argc > 0) {            /* ADJUST COUNTER HERE */
      g_string_assign(gs_parm1, argv[argc]);
      if (argv[argc + 1] != NULL) {
         g_string_assign(gs_parm2, argv[argc + 1]);
      }

      pch = g_strstr_len(gs_parm1->str, 6, "-help");
      if (pch != NULL) {
         g_print("\nsyntax: gapcmon [--help]\n"
            "where:  --help, this message, no command line options are available\n"
            "Skoona@Users.SourceForge.Net (GPL) 2006 \n\n");

         g_string_free(gs_parm1, TRUE);
         g_string_free(gs_parm2, TRUE);
         return TRUE;              /* trigger exit */
      }
   }

   g_string_free(gs_parm1, TRUE);
   g_string_free(gs_parm2, TRUE);

   /* *********************************************************
    * Apply instance value
    */

   return FALSE;
}

/*
 * Create main interface with the following panels
 * - icon list
 * - preferences panel
 * - about panel
*/
static GtkWidget *gapc_main_interface_create(PGAPC_CONFIG pcfg)
{
   GtkWindow *window = NULL;
   GdkPixbuf *pixbuf = NULL;
   GtkWidget *sbar = NULL, *notebook = NULL, *menu = NULL, *menu_item = NULL;
   GtkWidget *label = NULL, *button = NULL;
   GtkWidget *bbox = NULL, *lbox = NULL, *nbox = NULL, *box = NULL;
   gint i_page = 0;

   g_return_val_if_fail(pcfg != NULL, NULL);

   /* Create hash table for easy access to system widgets */
   pcfg->pht_Status = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
   pcfg->pht_Widgets = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
   pixbuf = pcfg->my_icons[GAPC_ICON_DEFAULT];
   pcfg->b_visible = FALSE;
   pcfg->tooltips = gtk_tooltips_new();
   g_object_ref (pcfg->tooltips);
   gtk_object_sink (GTK_OBJECT(pcfg->tooltips));   
   pcfg->b_run = TRUE;
   pcfg->cb_last_monitor_deleted = -1;

   /*
    * Create the top level window for the notebook to be packed into.*/
   window = g_object_new(GTK_TYPE_WINDOW, "border-width", 0, "destroy-with-parent",
      TRUE, "icon", pixbuf, "resizable", TRUE, "title",
      GAPC_WINDOW_TITLE, "type", GTK_WINDOW_TOPLEVEL, "type-hint",
      GDK_WINDOW_TYPE_HINT_NORMAL, "window-position", GTK_WIN_POS_NONE, NULL);

   pcfg->window = GTK_WIDGET(window);
   g_signal_connect(window, "destroy", G_CALLBACK(cb_main_interface_destroy), pcfg);
   g_signal_connect(window, "delete-event",
      G_CALLBACK(cb_main_interface_delete_event), pcfg);
   g_signal_connect(window, "show", G_CALLBACK(cb_main_interface_show), pcfg);
   g_signal_connect(window, "hide", G_CALLBACK(cb_main_interface_hide), pcfg);
   g_signal_connect(window, "window-state-event", 
                    G_CALLBACK(cb_util_manage_iconify_event), pcfg); 

   gapc_panel_systray_icon_create(pcfg);

   lbox = gtk_vbox_new(FALSE, 2);
   gtk_container_add(GTK_CONTAINER(window), lbox);
   gtk_widget_show(lbox);

/* */

   /* Notebook Box  */
   bbox = gtk_vbox_new(FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(bbox), 6);
   gtk_box_pack_start(GTK_BOX(lbox), bbox, TRUE, TRUE, 2);
   gtk_widget_show(bbox);
   nbox = gtk_hbox_new(TRUE, 0);
   gtk_box_pack_start(GTK_BOX(bbox), nbox, TRUE, TRUE, 0);
   gtk_widget_show(nbox);

   /* create the status bar */
   sbar = gtk_statusbar_new();
   gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(sbar), FALSE);
   g_hash_table_insert(pcfg->pht_Widgets, g_strdup("StatusBar"), sbar);
   gtk_box_pack_end(GTK_BOX(lbox), sbar, FALSE, TRUE, 0);
   gtk_widget_show(sbar);
   pcfg->i_info_context =
      gtk_statusbar_get_context_id(GTK_STATUSBAR(sbar), "Informational");

   /* buttons Box  */
   bbox = gtk_hbox_new(FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(bbox), 0);
   gtk_box_pack_end(GTK_BOX(lbox), bbox, FALSE, FALSE, 0);
   gtk_widget_show(bbox);
   box = gtk_hbutton_box_new();
   gtk_container_set_border_width(GTK_CONTAINER(box), 6);
   gtk_box_pack_end(GTK_BOX(bbox), box, FALSE, FALSE, 2);
   gtk_widget_show(box);

   /* Create the top-level notebook */
   notebook = gtk_notebook_new();
   gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
   gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
   gtk_notebook_set_homogeneous_tabs(GTK_NOTEBOOK(notebook), TRUE);
   gtk_box_pack_start(GTK_BOX(nbox), notebook, TRUE, TRUE, 0);
   gtk_widget_show(notebook);

   /* Create the main pages */
   gapc_panel_monitor_list_page(pcfg, GTK_NOTEBOOK(notebook));
   i_page = gapc_panel_preferences_page(pcfg, GTK_NOTEBOOK(notebook));
   gapc_panel_graph_property_page(pcfg, notebook);
   gapc_panel_glossary_page(pcfg, notebook);
   gapc_panel_about_page(GTK_NOTEBOOK(notebook), GAPC_WINDOW_TITLE, GAPC_VERSION,
      pixbuf);

   label = gtk_label_new("<big>" GAPC_GROUP_TITLE "</big>");
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   gtk_label_set_line_wrap(GTK_LABEL(label), FALSE);
   gtk_box_pack_start(GTK_BOX(bbox), label, TRUE, TRUE, 0);
   gtk_widget_show(label);

   /* quit Control button */
/*   
   button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
   g_signal_connect_swapped(button, "clicked", G_CALLBACK(gtk_widget_hide), GTK_WIDGET(window));  
   gtk_box_pack_end(GTK_BOX(box), button, TRUE, TRUE, 0);
   gtk_widget_show(button);

   GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
   gtk_widget_grab_default(button);
*/

   button = gtk_button_new_from_stock(GTK_STOCK_QUIT);
   g_signal_connect(button, "clicked", G_CALLBACK(cb_main_interface_button_quit),
      pcfg);
   gtk_box_pack_end(GTK_BOX(box), button, TRUE, TRUE, 0);
   gtk_widget_show(button);


   gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), i_page);

   pcfg->menu = menu = gtk_menu_new();
   menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_JUMP_TO, NULL);
   g_signal_connect(G_OBJECT(menu_item), "activate",
      G_CALLBACK(cb_util_popup_menu_response_jumpto), pcfg);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
   gtk_widget_show(menu_item);
   menu_item = gtk_separator_menu_item_new();
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
   gtk_widget_show(menu_item);
   menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
   g_signal_connect(G_OBJECT(menu_item), "activate",
      G_CALLBACK(cb_util_popup_menu_response_exit), pcfg);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
   gtk_widget_show(menu_item);
   gtk_widget_show(menu);

   gtk_widget_show_all(lbox);

   return GTK_WIDGET(window);
}

/*
 * Creates a GtkGLGraph histogram linechart page
*/
static gint gapc_monitor_history_page(PGAPC_MONITOR pm, GtkWidget * notebook)
{
   PGAPC_HISTORY pphs = (PGAPC_HISTORY) & pm->phs;
   PGAPC_CONFIG pcfg = (PGAPC_CONFIG) pm->gp;
   PLGRAPH       plg = NULL;
   gint i_page = 0, i_series = 0, h_index = 0;
   GtkWidget *label = NULL, *box = NULL;
   gchar *pch = NULL;
   gchar *pch_colors[6];
   gchar *pch_legend[] = { "LINEV", "LOADPCT", "TIMELEFT", "BCHARGE", "BATTV" };

   g_return_val_if_fail(pm != NULL, -1);

   /*
    * Prepare the environment */
   pphs->gp = (gpointer) pm;
   pphs->d_xinc = pm->d_refresh * pm->d_graph;
   lg_graph_debug = FALSE;

   for (h_index = 0; h_index < GAPC_LINEGRAPH_MAX_SERIES; h_index++) {
      if (pm->phs.sq[h_index].gm_graph != NULL) {
         g_mutex_free(pm->phs.sq[h_index].gm_graph);
      }
      pm->phs.sq[h_index].gm_graph = g_mutex_new();
   }

   /* get values from graph properties */
   pch_colors[0] = gtk_color_selection_palette_to_string ( &pcfg->color_linev, 1);
   pch_colors[1] = gtk_color_selection_palette_to_string ( &pcfg->color_loadpct, 1);    
   pch_colors[2] = gtk_color_selection_palette_to_string ( &pcfg->color_timeleft, 1);    
   pch_colors[3] = gtk_color_selection_palette_to_string ( &pcfg->color_bcharge, 1);    
   pch_colors[4] = gtk_color_selection_palette_to_string ( &pcfg->color_battv, 1);    
   pch_colors[5] = NULL;

   /*
    * Create notebook page page */
   box = gtk_vbox_new(FALSE, 0);
   g_object_set_data ( G_OBJECT(box), "pcfg-pointer", pm->gp);
   label = gtk_label_new("Historical Summary");
   i_page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, label);
   gtk_widget_show(GTK_WIDGET(box));
   gtk_widget_show(label);

   /*
    * Create Chart surface */
   plg = pphs->plg = lg_graph_create ( box, 300, 200 );
   if (plg != NULL) {
       for (i_series = 0; i_series < GAPC_LINEGRAPH_MAX_SERIES; i_series++) {
            lg_graph_data_series_add (plg, pch_legend[i_series], pch_colors[i_series]);
            g_free (pch_colors[i_series]);
       }

       pch = g_strdup_printf(
                 "<i>sampled every %3.1f seconds</i>", 
                 pm->phs.d_xinc);
       lg_graph_set_x_label_text (plg, pch);
       g_free(pch);

       lg_graph_set_chart_title (plg, pm->ch_title_info);
   }

   pm->tid_graph_refresh =
      gtk_timeout_add(((guint) (pphs->d_xinc * GAPC_REFRESH_FACTOR_1K )),
      (GSourceFunc) cb_util_line_chart_refresh, pphs);

   /* collect one right away */
   pphs->b_startup = TRUE;
   g_timeout_add((guint) (pm->d_refresh * GAPC_REFRESH_FACTOR_1K + 75),
      (GSourceFunc) cb_util_line_chart_refresh, pphs);

   return i_page;
}

/*
 * Add a new data point until xrange is reached
 * then rotate back the y points and add new point to end
 * return TRUE is successful, FALSE other wise
*/
static gboolean cb_util_line_chart_refresh(PGAPC_HISTORY pg)
{
   gint h_index = 0;
   PLGRAPH plg = NULL;
   PGAPC_MONITOR pm = NULL;

   g_return_val_if_fail(pg != NULL, FALSE);

   pm = (PGAPC_MONITOR) pg->gp;
   plg = pg->plg;
   g_return_val_if_fail(plg != NULL, FALSE);   
   g_return_val_if_fail(pm != NULL, FALSE);
   
   if (!pm->b_run)                 /* stop this timer */
      return FALSE;

   if (pm->b_graph_control) {
      g_timeout_add(100, (GSourceFunc) cb_util_line_chart_refresh_control, pm);
      return FALSE;
   }


   gdk_threads_enter();
   
   for (h_index = 0; h_index < plg->i_num_series; h_index++) {
        lg_graph_data_series_add_value (plg, h_index, 
                                gapc_util_point_filter_reset(&(pg->sq[h_index])) );
   }

   if (plg->i_points_available >= plg->x_range.i_max_scale ) {
       lg_graph_draw ( plg );    
   } else {
       lg_graph_data_series_draw_all (plg, TRUE);
       lg_graph_redraw ( plg );
   }

   gdk_flush();
   gdk_threads_leave();

   if (pg->b_startup) {
     pg->b_startup = FALSE;
     return FALSE;
   }

   /* first data point collected */
   return TRUE;
}


/*
 * Create and Initialize a Line Chart
*/
static PLGRAPH lg_graph_create (GtkWidget * box, gint width, gint height)
{
    PLGRAPH     plg = NULL;
    PGAPC_CONFIG pcfg = NULL;
    GtkWidget  *drawing_area = NULL;
    GdkColor    color;
    PangoFontDescription *font_desc = NULL;
    gchar *pstring = NULL;

    pcfg = (PGAPC_CONFIG)g_object_get_data (G_OBJECT(box), "pcfg-pointer");
    g_return_val_if_fail (pcfg != NULL, NULL);    
  
    plg = g_new0 (LGRAPH, 1);
    g_return_val_if_fail (plg != NULL, NULL);

    plg->cb_id = CB_GRAPH_ID;
    plg->x_range.cb_id = CB_RANGE_ID;
    plg->y_range.cb_id = CB_RANGE_ID;
    plg->b_tooltip_active = TRUE;
    /* 
     * These must be set before the first drawing_area configure event 
     */
    lg_graph_set_chart_title  (plg, "Waiting for Update");
    lg_graph_set_y_label_text (plg, "Precentage of 100% normal");
    lg_graph_set_x_label_text (plg, "Waiting for Update");

    g_snprintf (plg->ch_tooltip_text, sizeof (plg->ch_tooltip_text), "%s",
                "Waiting for Graphable Data...");

    pstring = gtk_color_selection_palette_to_string ( &pcfg->color_title, 1);
    lg_graph_set_chart_title_color (plg, pstring);
    g_free (pstring);   

    lg_graph_set_chart_scales_color (plg, "black");

    pstring = gtk_color_selection_palette_to_string ( &pcfg->color_chart, 1);
    lg_graph_set_chart_window_fg_color (plg, pstring);
    g_free (pstring);
    
    pstring = gtk_color_selection_palette_to_string ( &pcfg->color_window, 1);
    lg_graph_set_chart_window_bg_color (plg, pstring);
    g_free (pstring);

    /* Xminor divisions, Xmajor divisions, Xbotton scale, Xtop scale, ...y */
    lg_graph_set_ranges (plg, 1, 2, 0, 40, 2, 10, 0, 110);

    drawing_area = plg->drawing_area = gtk_drawing_area_new ();
    gtk_widget_set_events (drawing_area,
                           GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
                           GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);
    gtk_drawing_area_size (GTK_DRAWING_AREA (drawing_area), width, height); 
    gtk_box_pack_start (GTK_BOX (box), drawing_area, TRUE, TRUE, 0);

    font_desc = pango_font_description_from_string ("Monospace 10");
    gtk_widget_modify_font (drawing_area, font_desc);
    pango_font_description_free (font_desc);

    gtk_widget_realize (drawing_area);
    gtk_widget_show (drawing_area);

    plg->series_gc = gdk_gc_new (drawing_area->window);
    plg->window_gc = gdk_gc_new (drawing_area->window);
    plg->box_gc = gdk_gc_new (drawing_area->window);
    plg->scale_gc = gdk_gc_new (drawing_area->window);
    plg->title_gc = gdk_gc_new (drawing_area->window);

    gdk_gc_copy (plg->series_gc,
                 drawing_area->style->fg_gc[GTK_WIDGET_STATE (drawing_area)]);
    gdk_gc_copy (plg->window_gc,
                 drawing_area->style->bg_gc[GTK_WIDGET_STATE (drawing_area)]);
    gdk_gc_copy (plg->box_gc,
                 drawing_area->style->fg_gc[GTK_WIDGET_STATE (drawing_area)]);
    gdk_gc_copy (plg->scale_gc,
                 drawing_area->style->fg_gc[GTK_WIDGET_STATE (drawing_area)]);
    gdk_gc_copy (plg->title_gc,
                 drawing_area->style->text_aa_gc[GTK_WIDGET_STATE (drawing_area)]);

    gdk_color_parse (plg->ch_color_window_bg, &color);
    gdk_gc_set_rgb_fg_color (plg->window_gc, &color);

    gdk_color_parse (plg->ch_color_chart_bg, &color);
    gdk_gc_set_rgb_fg_color (plg->box_gc, &color);

    gdk_color_parse (plg->ch_color_scale_fg, &color);
    gdk_gc_set_rgb_fg_color (plg->scale_gc, &color);

    gdk_color_parse (plg->ch_color_title_fg, &color);
    gdk_gc_set_rgb_fg_color (plg->title_gc, &color);

    /* --- Signals used to handle backing pixmap --- */
    gtk_signal_connect (GTK_OBJECT (drawing_area), "configure_event",
                        (GtkSignalFunc) lg_graph_configure_event_cb, plg);
    gtk_signal_connect (GTK_OBJECT (drawing_area), "expose_event",
                        (GtkSignalFunc) lg_graph_expose_event_cb, plg);
    gtk_signal_connect (GTK_OBJECT (drawing_area), "motion_notify_event",
                        (GtkSignalFunc) lg_graph_motion_notify_event_cb, plg);
    gtk_signal_connect (GTK_OBJECT (drawing_area), "button_press_event",
                        (GtkSignalFunc) lg_graph_button_press_event_cb, plg);

    return plg;
}


/*
 * Detailed Information Notebook Page
*/
static gint gapc_monitor_information_page(PGAPC_MONITOR pm, GtkWidget * notebook)
{
   GtkWidget *frame, *label, *pbox, *lbox, *rbox, *gbox;
   GtkWidget *tbox, *tlbox, *trbox;
   gint i_page = 0;

   /* Create a Notebook Page */
   gbox = gtk_frame_new(NULL);
   gtk_container_set_border_width(GTK_CONTAINER(gbox), 4);
   gtk_frame_set_shadow_type(GTK_FRAME(gbox), GTK_SHADOW_NONE);
   label = gtk_label_new("Detailed Information");
   i_page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), gbox, label);
   gtk_widget_show(gbox);

   tbox = gtk_hbox_new(FALSE, 4);
   gtk_container_add(GTK_CONTAINER(gbox), tbox);
   gtk_widget_show(tbox);

   /*
    * create basic frame */
   tlbox = gtk_vbox_new(TRUE, 2);
   gtk_box_pack_start(GTK_BOX(tbox), tlbox, TRUE, TRUE, 0);
   gtk_widget_show(tlbox);
   trbox = gtk_vbox_new(TRUE, 2);
   gtk_box_pack_end(GTK_BOX(tbox), trbox, TRUE, TRUE, 0);
   gtk_widget_show(trbox);

   frame = gtk_frame_new("<b><i>Performance Summary</i></b>");
   gtk_frame_set_label_align(GTK_FRAME(frame), 0.1, 0.8);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
   label = gtk_frame_get_label_widget(GTK_FRAME(frame));
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   gtk_box_pack_start(GTK_BOX(tlbox), frame, TRUE, TRUE, 0);
   gtk_widget_show(frame);

   pbox = gtk_hbox_new(FALSE, 4);
   gtk_container_add(GTK_CONTAINER(frame), pbox);
   gtk_widget_show(pbox);
   lbox = gtk_vbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(pbox), lbox, FALSE, TRUE, 0);
   gtk_widget_show(lbox);
   rbox = gtk_vbox_new(FALSE, 0);
   gtk_box_pack_end(GTK_BOX(pbox), rbox, TRUE, TRUE, 0);
   gtk_widget_show(rbox);

   label = gtk_label_new("Selftest running\n" "Number of transfers\n"
      "Reason last transfer\n" "Last transfer to battery\n"
      "Last transfer off battery\n" "Time on battery\n" "Cummulative on battery");
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
   gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
   gtk_misc_set_alignment((GtkMisc *) label, 1.0, 0.5);
   gtk_box_pack_start(GTK_BOX(lbox), label, FALSE, FALSE, 0);
   gtk_widget_show(label);

   label = gtk_label_new("Waiting for refresh\n");
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
   gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
   gtk_misc_set_alignment((GtkMisc *) label, 0.0, 0.0);
   gtk_box_pack_start(GTK_BOX(rbox), label, TRUE, TRUE, 0);
   g_hash_table_insert(pm->pht_Widgets, g_strdup("PerformanceSummary"), label);
   gtk_widget_show(label);

   frame = gtk_frame_new("<b><i>Software Information</i></b>");
   gtk_frame_set_label_align(GTK_FRAME(frame), 0.1, 0.8);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
   label = gtk_frame_get_label_widget(GTK_FRAME(frame));
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   gtk_box_pack_end(GTK_BOX(tlbox), frame, TRUE, TRUE, 0);
   gtk_widget_show(frame);

   pbox = gtk_hbox_new(FALSE, 4);
   gtk_container_add(GTK_CONTAINER(frame), pbox);
   gtk_widget_show(pbox);
   lbox = gtk_vbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(pbox), lbox, FALSE, FALSE, 0);
   gtk_widget_show(lbox);
   rbox = gtk_vbox_new(FALSE, 0);
   gtk_box_pack_end(GTK_BOX(pbox), rbox, TRUE, TRUE, 0);
   gtk_widget_show(rbox);

   label = gtk_label_new("APCUPSD version\n" "Monitored UPS name\n"
      "Cable Driver type\n" "Configuration mode\n" "Last started\n" "UPS State");
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
   gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
   gtk_misc_set_alignment((GtkMisc *) label, 1.0, 0.5);
   gtk_box_pack_start(GTK_BOX(lbox), label, FALSE, FALSE, 0);
   gtk_widget_show(label);

   label = gtk_label_new("Waiting for refresh\n");
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
   gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
   gtk_misc_set_alignment((GtkMisc *) label, 0.0, 0.0);
   gtk_box_pack_start(GTK_BOX(rbox), label, TRUE, TRUE, 0);
   g_hash_table_insert(pm->pht_Widgets, g_strdup("SoftwareInformation"), label);
   gtk_widget_show(label);

   frame = gtk_frame_new("<b><i>UPS Metrics</i></b>");
   gtk_frame_set_label_align(GTK_FRAME(frame), 0.1, 0.8);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
   label = gtk_frame_get_label_widget(GTK_FRAME(frame));
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   gtk_box_pack_start(GTK_BOX(trbox), frame, TRUE, TRUE, 0);
   gtk_widget_show(frame);

   gbox = gtk_vbox_new(TRUE, 2);
   gtk_container_add(GTK_CONTAINER(frame), gbox);
   gtk_widget_show(gbox);

   gapc_util_barchart_create(pm, gbox, "HBar1", 10.8, "Waiting for refresh");
   gapc_util_barchart_create(pm, gbox, "HBar2", 40.8, "Waiting for refresh");
   gapc_util_barchart_create(pm, gbox, "HBar3", 0.8, "Waiting for refresh");
   gapc_util_barchart_create(pm, gbox, "HBar4", 40.8, "Waiting for refresh");
   gapc_util_barchart_create(pm, gbox, "HBar5", 10.8, "Waiting for refresh");

   frame = gtk_frame_new("<b><i>Product Information</i></b>");
   gtk_frame_set_label_align(GTK_FRAME(frame), 0.1, 0.8);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
   label = gtk_frame_get_label_widget(GTK_FRAME(frame));
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   gtk_box_pack_end(GTK_BOX(trbox), frame, TRUE, TRUE, 0);
   gtk_widget_show(frame);

   pbox = gtk_hbox_new(FALSE, 4);
   gtk_container_add(GTK_CONTAINER(frame), pbox);
   gtk_widget_show(pbox);
   lbox = gtk_vbox_new(FALSE, 0);
   gtk_box_pack_start(GTK_BOX(pbox), lbox, FALSE, FALSE, 0);
   gtk_widget_show(lbox);
   rbox = gtk_vbox_new(FALSE, 0);
   gtk_box_pack_end(GTK_BOX(pbox), rbox, TRUE, TRUE, 0);
   gtk_widget_show(rbox);

   label = gtk_label_new("Device\n" "Serial\n" "Manf date\n" "Firmware\n"
      "Batt date\n" "Wattage");
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
   gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
   gtk_misc_set_alignment((GtkMisc *) label, 1.0, 0.5);
   gtk_box_pack_start(GTK_BOX(lbox), label, FALSE, FALSE, 0);
   gtk_widget_show(label);

   label = gtk_label_new("Waiting for refresh\n");
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
   gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
   gtk_misc_set_alignment((GtkMisc *) label, 0.0, 0.0);
   gtk_box_pack_start(GTK_BOX(rbox), label, TRUE, TRUE, 0);
   g_hash_table_insert(pm->pht_Widgets, g_strdup("ProductInformation"), label);
   gtk_widget_show(label);

   return i_page;
}

/*
 * Events and Status Report Pages
*/
static gint gapc_monitor_text_report_page(PGAPC_MONITOR pm, GtkWidget * notebook,
   gchar * pchTitle, gchar * pchKey)
{
   PangoFontDescription *font_desc;
   GtkWidget *scrolled, *view, *label;
   gint i_page = 0;

   scrolled = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled),
      GTK_SHADOW_ETCHED_IN);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   label = gtk_label_new(pchTitle);
   i_page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolled, label);
   gtk_widget_show(scrolled);

   view = gtk_text_view_new();
   gtk_container_add(GTK_CONTAINER(scrolled), view);
   gtk_widget_show(view);

   /* Change default font throughout the widget */
   font_desc = pango_font_description_from_string("Monospace 9");
   gtk_widget_modify_font(view, font_desc);
   pango_font_description_free(font_desc);

   gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
   gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 5);

   g_hash_table_insert(pm->pht_Widgets, g_strdup(pchKey), view);

   return i_page;
}

static gint gapc_panel_glossary_page(PGAPC_CONFIG pcfg, GtkWidget * notebook)
{
   GtkWidget *scrolled, *label, *vbox;
   gint i_page = 0;
   gchar *ptext = GAPC_GLOSSARY;
   GdkColor color;

   gdk_color_parse("white", &color);

   vbox = gtk_vbox_new(FALSE, 0);
   label = gtk_label_new("Glossary");
   i_page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);
   gtk_widget_show(vbox);

   scrolled = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled),
      GTK_SHADOW_ETCHED_IN);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
   gtk_container_add(GTK_CONTAINER(vbox), scrolled);
   gtk_widget_show(GTK_WIDGET(scrolled));

   label = gtk_label_new(ptext);
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_FILL);
   gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);

   gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), label);
   gtk_widget_show(label);

   gtk_widget_modify_bg(gtk_widget_get_parent(label), GTK_STATE_NORMAL, &color);

   return i_page;
}

static gint gapc_panel_graph_property_page(PGAPC_CONFIG pcfg, GtkWidget * notebook)
{
   GtkWidget *s_frame, *w_frame, *label, *s_box, *w_box, *frame, *hbox, *pbox;
   GtkWidget *cb_linev, *cb_loadpct, *cb_timeleft, *cb_bcharge, *cb_battv;
   GtkWidget *cb_window, *cb_chart, *cb_text;
   GtkWidget *bbox, *b_undo;
   
   gint i_page = 0;

   frame = gtk_vbox_new(FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(frame), 4);
   label = gtk_label_new("Graph Properties");
   i_page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, label);
   gtk_widget_show(frame);

  /* 
   * Prepare the top color choice area */
   hbox = gtk_hbox_new(FALSE, 2);
   gtk_container_add ( GTK_CONTAINER (frame), hbox );
   gtk_widget_show(hbox);

  /* 
   * Prepare the bottom button/message area */
   bbox = gtk_hbox_new(FALSE, 0);
   gtk_box_pack_end ( GTK_BOX (frame), bbox, TRUE, TRUE, 2);
   gtk_widget_show(bbox);

   b_undo = gtk_button_new_from_stock (GTK_STOCK_UNDO);
   gtk_box_pack_start ( GTK_BOX (bbox), b_undo, FALSE, TRUE, 2);
   gtk_widget_show(b_undo);
   g_signal_connect (GTK_OBJECT(b_undo), "clicked", 
                     G_CALLBACK(cb_panel_property_color_reset), pcfg);
   
   label = gtk_label_new("These values will be used during the"
                         " creation of a monitor. Disable and "
                         "enable existing monitors to push values now.");
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
   gtk_box_pack_end ( GTK_BOX (bbox), label, TRUE, TRUE, 2);
   gtk_widget_show(label);
   
  /* 
   * Prepare the top color choice area */
   s_frame = gtk_frame_new("Series Color");
   gtk_container_set_border_width(GTK_CONTAINER(s_frame), 4);
   gtk_frame_set_shadow_type(GTK_FRAME(s_frame), GTK_SHADOW_IN);
   gtk_box_pack_start(GTK_BOX(hbox), s_frame, TRUE, TRUE, 0);
   gtk_widget_show(s_frame);

   s_box = gtk_vbox_new(FALSE, 0);   
   gtk_container_add ( GTK_CONTAINER (s_frame), s_box );
   gtk_widget_show(s_box);

   pbox = gtk_hbox_new(TRUE, 4);   
   gtk_box_pack_start(GTK_BOX(s_box), pbox, FALSE, FALSE, 0);   
	   label = gtk_label_new("LINEV");   
	   gtk_box_pack_start(GTK_BOX(pbox), label, FALSE, FALSE, 0);
       gtk_widget_show(label); 

      
	   cb_linev = gtk_color_button_new_with_color( &pcfg->color_linev);
	   gtk_color_button_set_title (GTK_COLOR_BUTTON(cb_linev), "LINEV");
	   gtk_box_pack_start(GTK_BOX(pbox), cb_linev, FALSE, FALSE, 0);
	   gtk_widget_show(cb_linev);
	   g_object_set_data (G_OBJECT(cb_linev), "gconf-client", pcfg->client );
	   g_signal_connect ( GTK_OBJECT(cb_linev), "color-set", 
	   					  G_CALLBACK(cb_panel_property_color_change), 
	   					  GAPC_COLOR_LINEV_KEY);
       g_hash_table_insert(pcfg->pht_Widgets, g_strdup("color-linev"), cb_linev);  

   pbox = gtk_hbox_new(TRUE, 4);   
   gtk_box_pack_start(GTK_BOX(s_box), pbox, FALSE, FALSE, 0);   
	   label = gtk_label_new("LOADPCT");   
	   gtk_box_pack_start(GTK_BOX(pbox), label, FALSE, FALSE, 0);
       gtk_widget_show(label);         

	   cb_loadpct = gtk_color_button_new_with_color( &pcfg->color_loadpct);
	   gtk_color_button_set_title (GTK_COLOR_BUTTON(cb_loadpct), "LOADPCT");
	   gtk_box_pack_start(GTK_BOX(pbox), cb_loadpct, FALSE, FALSE, 0);
	   gtk_widget_show(cb_loadpct);
	   g_object_set_data (G_OBJECT(cb_loadpct), "gconf-client", pcfg->client );
	   g_signal_connect ( GTK_OBJECT(cb_loadpct), "color-set", 
	   					  G_CALLBACK(cb_panel_property_color_change), 
	   					  GAPC_COLOR_LOADPCT_KEY);
       g_hash_table_insert(pcfg->pht_Widgets, g_strdup("color-loadpct"), cb_loadpct);  
       
   pbox = gtk_hbox_new(TRUE, 4);   
   gtk_box_pack_start(GTK_BOX(s_box), pbox, FALSE, FALSE, 0);   
	   label = gtk_label_new("TIMELEFT");   
	   gtk_box_pack_start(GTK_BOX(pbox), label, FALSE, FALSE, 0);
       gtk_widget_show(label);         

	   cb_timeleft = gtk_color_button_new_with_color( &pcfg->color_timeleft);
	   gtk_color_button_set_title (GTK_COLOR_BUTTON(cb_timeleft), "TIMELEFT");
	   gtk_box_pack_start(GTK_BOX(pbox), cb_timeleft, FALSE, FALSE, 0);
	   gtk_widget_show(cb_timeleft);
	   g_object_set_data (G_OBJECT(cb_timeleft), "gconf-client", pcfg->client );
	   g_signal_connect ( GTK_OBJECT(cb_timeleft), "color-set", 
	   					  G_CALLBACK(cb_panel_property_color_change), 
	   					  GAPC_COLOR_TIMELEFT_KEY);
       g_hash_table_insert(pcfg->pht_Widgets, g_strdup("color-timeleft"), cb_timeleft);  

   pbox = gtk_hbox_new(TRUE, 4);   
   gtk_box_pack_start(GTK_BOX(s_box), pbox, FALSE, FALSE, 0);   
	   label = gtk_label_new("BCHARGE");   
	   gtk_box_pack_start(GTK_BOX(pbox), label, FALSE, FALSE, 0);
       gtk_widget_show(label);         

	   cb_bcharge = gtk_color_button_new_with_color( &pcfg->color_bcharge);
	   gtk_color_button_set_title (GTK_COLOR_BUTTON(cb_bcharge), "BCHARGE");
	   gtk_box_pack_start(GTK_BOX(pbox), cb_bcharge, FALSE, FALSE, 0);
	   gtk_widget_show(cb_bcharge);
	   g_object_set_data (G_OBJECT(cb_bcharge), "gconf-client", pcfg->client );
	   g_signal_connect ( GTK_OBJECT(cb_bcharge), "color-set", 
	   					  G_CALLBACK(cb_panel_property_color_change), 
	   					  GAPC_COLOR_BCHARGE_KEY);
       g_hash_table_insert(pcfg->pht_Widgets, g_strdup("color-bcharge"), cb_bcharge);  

   pbox = gtk_hbox_new(TRUE, 4);   
   gtk_box_pack_start(GTK_BOX(s_box), pbox, FALSE, FALSE, 0);   
	   label = gtk_label_new("BATTV");   
	   gtk_box_pack_start(GTK_BOX(pbox), label, FALSE, FALSE, 0);
       gtk_widget_show(label);         

	   cb_battv = gtk_color_button_new_with_color( &pcfg->color_battv);
	   gtk_color_button_set_title (GTK_COLOR_BUTTON(cb_battv), "BATTV");
	   gtk_box_pack_start(GTK_BOX(pbox), cb_battv, FALSE, FALSE, 0);
	   gtk_widget_show(cb_battv);
	   g_object_set_data (G_OBJECT(cb_battv), "gconf-client", pcfg->client );
	   g_signal_connect ( GTK_OBJECT(cb_battv), "color-set", 
	   					  G_CALLBACK(cb_panel_property_color_change), 
	   					  GAPC_COLOR_BATTV_KEY);
       g_hash_table_insert(pcfg->pht_Widgets, g_strdup("color-battv"), cb_battv);  
   
   w_frame = gtk_frame_new("Window Colors");
   gtk_container_set_border_width(GTK_CONTAINER(w_frame), 4);
   gtk_frame_set_shadow_type(GTK_FRAME(w_frame), GTK_SHADOW_IN);
   gtk_box_pack_start(GTK_BOX(hbox), w_frame, TRUE, TRUE, 0);
   gtk_widget_show(w_frame);

   w_box = gtk_vbox_new(FALSE, 0);   
   gtk_container_add ( GTK_CONTAINER (w_frame), w_box );
   gtk_widget_show(w_box);

   pbox = gtk_hbox_new(TRUE, 4);   
   gtk_box_pack_start(GTK_BOX(w_box), pbox, FALSE, FALSE, 0);   
	   label = gtk_label_new("Window Background");   
	   gtk_box_pack_start(GTK_BOX(pbox), label, FALSE, FALSE, 0);
       gtk_widget_show(label);         

	   cb_window = gtk_color_button_new_with_color( &pcfg->color_window);
	   gtk_color_button_set_title (GTK_COLOR_BUTTON(cb_window), "Window Background");
	   gtk_box_pack_start(GTK_BOX(pbox), cb_window, FALSE, FALSE, 0);
	   gtk_widget_show(cb_window);	   
	   g_object_set_data (G_OBJECT(cb_window), "gconf-client", pcfg->client );
	   g_signal_connect ( GTK_OBJECT(cb_window), "color-set", 
	   					  G_CALLBACK(cb_panel_property_color_change), 
	   					  GAPC_COLOR_WINDOW_KEY);
       g_hash_table_insert(pcfg->pht_Widgets, g_strdup("color-window"), cb_window);  

   pbox = gtk_hbox_new(TRUE, 4);   
   gtk_box_pack_start(GTK_BOX(w_box), pbox, FALSE, FALSE, 0);   
	   label = gtk_label_new("Chart Background");   
	   gtk_box_pack_start(GTK_BOX(pbox), label, FALSE, FALSE, 0);
       gtk_widget_show(label);         

	   cb_chart = gtk_color_button_new_with_color( &pcfg->color_chart);
	   gtk_color_button_set_title (GTK_COLOR_BUTTON(cb_chart), "Chart Background");
	   gtk_box_pack_start(GTK_BOX(pbox), cb_chart, FALSE, FALSE, 0);
	   gtk_widget_show(cb_chart);
	   g_object_set_data (G_OBJECT(cb_chart), "gconf-client", pcfg->client );
	   g_signal_connect ( GTK_OBJECT(cb_chart), "color-set", 
	   					  G_CALLBACK(cb_panel_property_color_change), 
	   					  GAPC_COLOR_CHART_KEY);
       g_hash_table_insert(pcfg->pht_Widgets, g_strdup("color-chart"), cb_chart);  	   					  

   pbox = gtk_hbox_new(TRUE, 4);   
   gtk_box_pack_start(GTK_BOX(w_box), pbox, FALSE, FALSE, 0);   
	   label = gtk_label_new("Title Texts");   
	   gtk_box_pack_start(GTK_BOX(pbox), label, FALSE, FALSE, 0);
       gtk_widget_show(label);         

	   cb_text = gtk_color_button_new_with_color( &pcfg->color_title);
	   gtk_color_button_set_title (GTK_COLOR_BUTTON(cb_text), "Title Texts");
	   gtk_box_pack_start(GTK_BOX(pbox), cb_text, FALSE, FALSE, 0);
	   gtk_widget_show(cb_text);
	   g_object_set_data (G_OBJECT(cb_text), "gconf-client", pcfg->client );
	   g_signal_connect ( GTK_OBJECT(cb_text), "color-set", 
	   					  G_CALLBACK(cb_panel_property_color_change), 
	   					  GAPC_COLOR_TITLE_KEY);
       g_hash_table_insert(pcfg->pht_Widgets, g_strdup("color-title"), cb_text);     

   return i_page;
}


/*
 * Creates monitor interface with the normal panels
*/
static GtkWidget *gapc_monitor_interface_create(PGAPC_CONFIG pcfg, gint i_monitor,
   GtkTreeIter * iter)
{
   GtkWindow *window = NULL;
   GdkPixbuf *pixbuf = NULL;
   GtkWidget *sbar = NULL, *notebook = NULL, *menu = NULL, *menu_item = NULL;
   GtkWidget *label = NULL, *button = NULL;
   GtkWidget *bbox = NULL, *lbox = NULL, *nbox = NULL, *box = NULL;
   PGAPC_MONITOR pm = NULL;
   gint z_monitor = 0;

   g_return_val_if_fail(pcfg != NULL, NULL);

   pm = g_new0(GAPC_MONITOR, 1);
   g_return_val_if_fail(pm != NULL, NULL);
   pm->cb_id = CB_MONITOR_ID;
   pm->cb_monitor_num = i_monitor;
   pm->gp = (gpointer) pcfg;
   pm->phs.gp = (gpointer) pm;
   pm->phs.cb_id = CB_HISTORY_ID;
   pm->phs.sq[0].cb_id = CB_SUMM_ID;
   pm->phs.sq[1].cb_id = CB_SUMM_ID;
   pm->phs.sq[2].cb_id = CB_SUMM_ID;
   pm->phs.sq[3].cb_id = CB_SUMM_ID;
   pm->phs.sq[4].cb_id = CB_SUMM_ID;

   pm->my_icons = pcfg->my_icons;
   pm->pht_Status = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
   pm->pht_Widgets = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
   pm->phs.b_startup = TRUE;

   pm->cb_enabled = TRUE;
   pm->b_visible = FALSE;
   pm->b_run = TRUE;
   pm->i_icon_index = GAPC_ICON_DEFAULT;
   pm->i_old_icon_index = GAPC_ICON_ONLINE;
   pm->tray_icon = NULL;
   pm->tray_image = NULL;
   if (pm->pch_host != NULL) {
      g_free(pm->pch_host);
   }
   gtk_tree_model_get(GTK_TREE_MODEL(pcfg->prefs_model), iter,
      GAPC_PREFS_SYSTRAY, &(pm->cb_use_systray),
      GAPC_PREFS_PORT, &(pm->i_port),
      GAPC_PREFS_WATT, &(pm->i_watt),
      GAPC_PREFS_GRAPH, &(pm->d_graph),
      GAPC_PREFS_HOST, &(pm->pch_host),
      GAPC_PREFS_REFRESH, &(pm->d_refresh), 
      GAPC_PREFS_MONITOR, &z_monitor, -1);

   pixbuf = pm->my_icons[GAPC_ICON_DEFAULT];
   pm->tooltips = gtk_tooltips_new();
   g_object_ref (pm->tooltips);
   gtk_object_sink (GTK_OBJECT(pm->tooltips));   
   pm->gm_update = g_mutex_new();
   pm->client = pcfg->client;
   pm->i_old_icon_index = GAPC_ICON_DEFAULT;
   pm->monitor_model = pcfg->monitor_model;
   g_snprintf(pm->ch_title_info, GAPC_MAX_TEXT, "%s {%s}", GAPC_WINDOW_TITLE,
      pm->pch_host);

   if (pm->d_refresh < GAPC_REFRESH_MIN_INCREMENT) {
      pm->d_refresh = GAPC_REFRESH_DEFAULT;
   }
   if (pm->d_graph < GAPC_REFRESH_MIN_INCREMENT) {
      pm->d_graph = GAPC_LINEGRAPH_REFRESH_FACTOR;
   }

   /*
    * Start the central network thread */
   pm->q_network = g_async_queue_new();
   g_return_val_if_fail(pm->q_network != NULL, NULL);

   pm->b_thread_stop = FALSE;
   pm->tid_thread_qwork =
      g_thread_create((GThreadFunc) gapc_net_thread_qwork, pm, TRUE, NULL);

   /*
    * Create the top level window for the notebook to be packed into.*/
   window = g_object_new(GTK_TYPE_WINDOW, "border-width", 0, "destroy-with-parent",
      TRUE, "icon", pixbuf, "resizable", TRUE, "title",
      pm->ch_title_info, "type", GTK_WINDOW_TOPLEVEL, "type-hint",
      GDK_WINDOW_TYPE_HINT_NORMAL, "window-position", GTK_WIN_POS_NONE, NULL);

   pm->window = GTK_WIDGET(window);
   g_signal_connect(window, "destroy", G_CALLBACK(cb_monitor_interface_destroy), pm);
   g_signal_connect(window, "delete-event",
      G_CALLBACK(cb_monitor_interface_delete_event), pm);
   g_signal_connect(window, "show", G_CALLBACK(cb_monitor_interface_show), pm);
   g_signal_connect(window, "hide", G_CALLBACK(cb_monitor_interface_hide), pm);
   g_signal_connect(window, "window-state-event", 
                    G_CALLBACK(cb_util_manage_iconify_event), pm); 

   g_snprintf(pm->ch_title_info, GAPC_MAX_TEXT, "%s\n<b><i>{%s}</i></b>",
      GAPC_WINDOW_TITLE, pm->pch_host);

   lbox = gtk_vbox_new(FALSE, 2);
   gtk_container_add(GTK_CONTAINER(window), lbox);
   gtk_widget_show(lbox);

/* */

   /* Notebook Box  */
   bbox = gtk_vbox_new(FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(bbox), 6);
   gtk_box_pack_start(GTK_BOX(lbox), bbox, TRUE, TRUE, 2);
   gtk_widget_show(bbox);
   nbox = gtk_hbox_new(TRUE, 0);
   gtk_box_pack_start(GTK_BOX(bbox), nbox, TRUE, TRUE, 0);
   gtk_widget_show(nbox);

   /* create the status bar */
   sbar = gtk_statusbar_new();
   gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(sbar), FALSE);
   g_hash_table_insert(pm->pht_Widgets, g_strdup("StatusBar"), sbar);
   gtk_box_pack_end(GTK_BOX(lbox), sbar, FALSE, TRUE, 0);
   gtk_widget_show(sbar);
   pm->i_info_context =
      gtk_statusbar_get_context_id(GTK_STATUSBAR(sbar), "Informational");

   /* buttons Box  */
   bbox = gtk_hbox_new(FALSE, 0);
   gtk_container_set_border_width(GTK_CONTAINER(bbox), 0);
   gtk_box_pack_end(GTK_BOX(lbox), bbox, FALSE, FALSE, 0);
   gtk_widget_show(bbox);
   box = gtk_hbutton_box_new();
   gtk_hbutton_box_set_layout_default(GTK_BUTTONBOX_SPREAD);
   gtk_container_set_border_width(GTK_CONTAINER(box), 4);
   gtk_box_pack_end(GTK_BOX(bbox), box, TRUE, TRUE, 0);
   gtk_widget_show(box);

   /* Create the top-level notebook */
   pm->notebook = notebook = gtk_notebook_new();
   gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
   gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
   gtk_notebook_set_homogeneous_tabs(GTK_NOTEBOOK(notebook), FALSE);
   gtk_box_pack_start(GTK_BOX(nbox), notebook, TRUE, TRUE, 0);
   gtk_widget_show(notebook);

   /* Create the main pages */
   gapc_monitor_history_page(pm, notebook);
   gapc_monitor_information_page(pm, notebook);
   gapc_monitor_text_report_page(pm, notebook, "Power Events", "EventsPage");
   gapc_monitor_text_report_page(pm, notebook, "Full UPS Status", "StatusPage");

   /* Create the main pages */

   label = gtk_label_new(pm->ch_title_info);
   gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
   gtk_label_set_line_wrap(GTK_LABEL(label), FALSE);
   gtk_box_pack_start(GTK_BOX(bbox), label, TRUE, TRUE, 0);
   gtk_widget_show(label);
   g_hash_table_insert(pm->pht_Widgets, g_strdup("TitleStatus"), label);

   /* refresh Control button */
   button = gtk_button_new_from_stock(GTK_STOCK_REFRESH);
   GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
   g_signal_connect(button, "clicked",
      G_CALLBACK(cb_monitor_interface_button_refresh), pm);
   gtk_box_pack_end(GTK_BOX(box), button, TRUE, TRUE, 0);
   gtk_widget_show(button);
   gtk_widget_grab_default(button);

/*
   button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
   g_signal_connect(button, "clicked",
      G_CALLBACK(cb_monitor_interface_button_close), pm);
   gtk_box_pack_end(GTK_BOX(box), button, TRUE, TRUE, 0);
   gtk_widget_show(button);
*/
   gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);

   gapc_panel_monitor_model_rec_add(pcfg, pm);

   gapc_panel_systray_icon_create(pm);

   g_async_queue_push(pm->q_network, (gpointer) pm);

   pm->tid_automatic_refresh =
      g_timeout_add((guint) (pm->d_refresh * GAPC_REFRESH_FACTOR_1K),
      (GSourceFunc) cb_monitor_automatic_refresh, pm);

   pm->menu = menu = gtk_menu_new();
   menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_JUMP_TO, NULL);
   g_signal_connect(G_OBJECT(menu_item), "activate",
      G_CALLBACK(cb_util_popup_menu_response_jumpto), pm);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
   gtk_widget_show(menu_item);
   menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
   g_signal_connect(G_OBJECT(menu_item), "activate",
      G_CALLBACK(cb_util_popup_menu_response_exit), pm);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
   gtk_widget_show(menu_item);
   gtk_widget_show(menu);
   
   sbar = g_hash_table_lookup (pcfg->pht_Widgets, "StatusBar");
   if (sbar) {
      gchar *pch = NULL;
      
      gtk_statusbar_pop(GTK_STATUSBAR(sbar), pcfg->i_info_context);
      pch = g_strdup_printf
         ("Monitor for %s Created!...", pm->pch_host);
      gtk_statusbar_push(GTK_STATUSBAR(sbar), pcfg->i_info_context, pch);
      g_free(pch);
   }

   return GTK_WIDGET(window);
}

/*
 * Creates monitor interface with the normal panels
*/
static void gapc_monitor_interface_destroy(PGAPC_CONFIG pcfg, gint i_monitor)
{
   GtkTreeIter iter;
   PGAPC_MONITOR pm = NULL;

   g_return_if_fail(pcfg != NULL);

   if (gapc_util_treeview_get_iter_from_monitor(pcfg->monitor_model, &iter,
         i_monitor)) {
      gtk_tree_model_get(pcfg->monitor_model, &iter, GAPC_MON_POINTER, &pm, -1);
   }

   if ((pm == NULL) || (pm->window == NULL)) {
      return;
   }

   if (!pm->cb_enabled) {
      return;
   }

   pm->b_run = FALSE;

   gtk_list_store_remove(GTK_LIST_STORE(pcfg->monitor_model), &iter);

   if (pm->cb_use_systray) {
      gapc_panel_systray_icon_remove(pm);
   }

   if (pm->menu != NULL) {
      gtk_widget_destroy(pm->menu);
   }

   if (pm->window != NULL) {
      gtk_widget_destroy(pm->window);
   }

   return;
}

/*
 * Main entry point
*/
extern int main(int argc, char *argv[])
{
   PGAPC_CONFIG pcfg = NULL;
   GtkWidget *window = NULL;

   /*
    * Initialize GLib thread support, and GTK
    */
   g_type_init();
   g_thread_init(NULL);

   gdk_threads_init();

   gtk_init(&argc, &argv);

   pcfg = g_new0(GAPC_CONFIG, 1);
   pcfg->cb_id = CB_CONTROL_ID;

   /*
    * Get the instance number for this execution */
   if (gapc_main_interface_parse_args(argc, argv, pcfg)) {
      return 1;                    /* exit if user only wanted help */
   }

   gapc_util_load_icons(pcfg);

   gapc_panel_gconf_init(pcfg);

   window = gapc_main_interface_create(pcfg);
   if (!pcfg->b_use_systray) {
       pcfg->b_visible = TRUE;
       g_object_set(pcfg->window, 
                    "skip-pager-hint",   FALSE, 
                    "skip-taskbar-hint", FALSE, 
                    NULL);
       gtk_window_present(GTK_WINDOW(pcfg->window));       
   }
   
   gapc_panel_gconf_watch(pcfg);

   /*
    * enter the GTK main loop
    */
   gdk_threads_enter();
   gtk_main();
   gdk_flush();
   gdk_threads_leave();

   return (0);
}
