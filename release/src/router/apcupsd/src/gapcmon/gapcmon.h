/* gapcmon.h               serial-0087-0 ************************************

  GKT+ GUI with Notification Area (System Tray) support.  Program  for 
  monitoring the apcupsd.sourceforge.net package.
  Copyright (C) 2006 James Scott, Jr. <skoona@users.sourceforge.net>

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




#ifndef GAPC_H_
#define GAPC_H_

G_BEGIN_DECLS

#ifndef VERSION
#define GAPC_VERSION "0.8.7-0"
#else
#define GAPC_VERSION VERSION
#endif

#ifndef ICON_DIR
#define ICON_DIR "/usr/share"
#endif

#define GAPC_PROG_NAME    "gapcmon"
#define GAPC_GROUP_TITLE "<i>  Uninterruptible Power Supply Monitor...</i>\n  for APCUPSD"
#define GAPC_WINDOW_TITLE  "gapcmon: UPS Information Panels"
#define GAPC_CP_GROUP_KEY    "/apps/gapcmon/controller"
#define GAPC_CP_SYSTRAY_KEY  "/apps/gapcmon/controller/use_systray"
#define GAPC_CP_PAGERS_KEY   "/apps/gapcmon/controller/skip_pagers"

#define GAPC_COLOR_LINEV_KEY "/apps/gapcmon/controller/color_linev"
#define GAPC_COLOR_LOADPCT_KEY "/apps/gapcmon/controller/color_loadpct"
#define GAPC_COLOR_TIMELEFT_KEY "/apps/gapcmon/controller/color_timeleft"
#define GAPC_COLOR_BCHARGE_KEY "/apps/gapcmon/controller/color_bcharge"
#define GAPC_COLOR_BATTV_KEY "/apps/gapcmon/controller/color_battv"
#define GAPC_COLOR_WINDOW_KEY "/apps/gapcmon/controller/color_window"
#define GAPC_COLOR_CHART_KEY "/apps/gapcmon/controller/color_chart"
#define GAPC_COLOR_TITLE_KEY "/apps/gapcmon/controller/color_title"

#define GAPC_MID_GROUP_KEY   "/apps/gapcmon/monitor"
#define GAPC_ENABLE_KEY      "/apps/gapcmon/monitor/%d/enabled"
#define GAPC_SYSTRAY_KEY     "/apps/gapcmon/monitor/%d/use_systray"
#define GAPC_PAGER_KEY       "/apps/gapcmon/monitor/%d/skip_pagers"     /* not used */
#define GAPC_PORT_KEY        "/apps/gapcmon/monitor/%d/port_number"
#define GAPC_REFRESH_KEY     "/apps/gapcmon/monitor/%d/network_interval"
#define GAPC_GRAPH_KEY       "/apps/gapcmon/monitor/%d/graph_interval"
#define GAPC_HOST_KEY        "/apps/gapcmon/monitor/%d/host_name"
#define GAPC_WATT_KEY        "/apps/gapcmon/monitor/%d/ups_wattage"
#define GAPC_MAX_ARRAY 256         /* for arrays or lists */
#define GAPC_MAX_TEXT 256          /* for strings */
#define GAPC_ICON_SIZE 24          /* Ideal size of icons */
#define GAPC_MAX_BUFFER  512      /* Size of a text buffer or local string */

#define GAPC_REFRESH_FACTOR_1K 1000     /* micro.secs for visual refresh    */
#define GAPC_REFRESH_FACTOR_ONE_TIME 500
#define GAPC_HOST_DEFAULT "localhost"
#define GAPC_PORT_DEFAULT 3551
#define GAPC_WATT_DEFAULT 600
#define GAPC_REFRESH_DEFAULT 8.0
#define GAPC_REFRESH_MIN_INCREMENT 1.0  /* Minimum refresh cycle seconds */
#define GAPC_LINEGRAPH_XMAX 40
#define GAPC_LINEGRAPH_YMAX 110
#define GAPC_LINEGRAPH_MAX_SERIES 5
#define GAPC_LINEGRAPH_REFRESH_FACTOR 30.0      /* Num refreshes per collection  */

#define SKNET_HUGE_ARRAY 4096
#define SKNET_REG_ARRAY  1024
#define SKNET_STR_ARRAY   256

typedef struct _SKNET_Control_Data {
  gint        cb_id;           
  GIOChannel *ioc;                                 /* socket io channel */
  gint        fd_server;                           /* our local server-socket */
  gint        i_port;                              /* dest host port */   
  gboolean    b_network_control;                   /* TRUE signals resolve address needed */
  gchar       ch_ip_string[SKNET_STR_ARRAY];       /* dest host ip addr or dns name */
  gchar       ch_ip_client[SKNET_STR_ARRAY];       /* incoming host ip addr or dns name */  
  gchar       ch_ip_client_port[SKNET_STR_ARRAY];  /* incoming host ip port */  
  gchar       ch_session_message[SKNET_HUGE_ARRAY];
  gchar       ch_error_msg[SKNET_REG_ARRAY];
  gpointer    gip;                                 /* struct sockaddr_in -- resolved tcp-ip address */
  gpointer    gp_reserved;                         /*  reserved private pointer for me */  
  gpointer    gp_user_data;                        /*  private pointer for YOU or user */
  gint        i_byte_counter;                      /* public byte counter */  
} SKNET_COMMS, *PSKCOMM;

typedef enum _Control_Block_id {
    CB_SERIES_ID,
    CB_RANGE_ID,
    CB_GRAPH_ID,
    CB_HISTORY_ID,    
    CB_MONITOR_ID,    
    CB_CONTROL_ID,    
    CB_COLUMN_ID,   
    CB_SUMM_ID,
    CB_PSKCOMM_ID,      
    CB_N_ID
} GAPCDataID;

   typedef enum _State_Icons_IDs {
   GAPC_ICON_ONLINE,
   GAPC_ICON_ONBATT,
   GAPC_ICON_CHARGING,
   GAPC_ICON_DEFAULT,
   GAPC_ICON_UNPLUGGED,
   GAPC_ICON_NETWORKERROR,
   GAPC_N_ICONS
} GAPC_IconType;

typedef enum _Timer_IDs {
   GAPC_TIMER_AUTO,
   GAPC_TIMER_DEDICATED,
   GAPC_TIMER_CONTROL,
   GAPC_N_TIMERS
} GAPC_TimerType;

typedef enum _Prefs_Store_IDs {
   GAPC_PREFS_MONITOR,
   GAPC_PREFS_ENABLED,
   GAPC_PREFS_SYSTRAY,
   GAPC_PREFS_PORT,
   GAPC_PREFS_REFRESH,
   GAPC_PREFS_GRAPH,
   GAPC_PREFS_HOST,
   GAPC_PREFS_WATT,
   GAPC_N_PREFS_COLUMNS
} GAPC_PrefsType;

typedef enum _Monitor_Store_IDs {
   GAPC_MON_MONITOR,
   GAPC_MON_ICON,
   GAPC_MON_STATUS,
   GAPC_MON_POINTER,
   GAPC_MON_UPSSTATE,
   GAPC_N_MON_COLUMNS
} GAPC_MonitorType;

typedef struct _Preferences_Key_Records {
   gchar k_enabled[GAPC_MAX_TEXT];
   gchar k_use_systray[GAPC_MAX_TEXT];
   gchar k_port_number[GAPC_MAX_TEXT];
   gchar k_network_interval[GAPC_MAX_TEXT];
   gchar k_graph_interval[GAPC_MAX_TEXT];
   gchar k_host_name[GAPC_MAX_TEXT];
   gchar v_host_name[GAPC_MAX_TEXT];
} GAPC_PKEYS, *PGAPC_PKEYS;

/* Control structure for TreeView columns and callbacks */
typedef struct _Prefs_Column_Data {
   GAPCDataID cb_id;               /* This is REQUIRED TO BE 1ST in struct */
   guint cb_monitor_num;           /* monitor number 1-based */
   guint i_col_num;
   GConfClient *client;
   GtkTreeModel *prefs_model;      /* GtkListStore */

} GAPC_PREFS_COLUMN, *PGAPC_PREFS_COLUMN;

typedef struct _Monitor_Column_Data {
   GAPCDataID cb_id;                  /* This is REQUIRED TO BE 1ST in struct */
   guint cb_monitor_num;           /* monitor number 1-based */
   guint i_col_num;
   GConfClient *client;
   GtkTreeModel *monitor_model;    /* GtkListStore */

} GAPC_MON_COLUMN, *PGAPC_MON_COLUMN;

typedef struct _GAPC_H_CHART {
   GAPCDataID cb_id;
   gdouble d_value;
   gboolean b_center_text;
   gchar c_text[GAPC_MAX_TEXT];
   GdkRectangle rect;
} GAPC_BAR_H, *PGAPC_BAR_H;

typedef struct _GAPC_SUM_SQUARES {
   GAPCDataID cb_id;
   gint point_count;

   gdouble this_point;
   gdouble this_answer;

   gdouble last_point;
   gdouble last_answer;

   gdouble answer_summ;
   gdouble point_min;
   gdouble point_max;

   GMutex *gm_graph;               /* Control mutex  for graphics filter */
} GAPC_SUMS, *PGAPC_SUMS;

typedef struct _LGRAPH_SERIES {
    GAPCDataID  cb_id;
    gint        i_series_id;    /* is this series number 1 2 or 3, ZERO based */
    gint        i_point_count;  /* 1 based */
    gint        i_max_points;   /* 1 based */
    gchar       ch_legend_text[GAPC_MAX_TEXT];
    gchar       ch_legend_color[GAPC_MAX_TEXT];
    GdkColor    legend_color;
    gdouble     d_max_value;
    gdouble     d_min_value;
    gdouble    *lg_point_dvalue;    /* array of doubles y values zero based, x = index */
    GdkPoint   *point_pos;      /* last gdk position each point - recalc on evey draw */
} LG_SERIES, *PLG_SERIES;

typedef struct _LGRAPH_RANGES {
    GAPCDataID  cb_id;
    gint        i_inc_minor_scale_by;   /* minor increments */
    gint        i_inc_major_scale_by;   /* major increments */
    gint        i_min_scale;    /* minimum scale value - ex:   0 */
    gint        i_max_scale;    /* maximum scale value - ex: 100 */
    gint        i_num_minor;    /* number of minor points */
    gint        i_num_major;    /* number of major points */
    gint        i_minor_inc;    /* pixels per minor increment */
    gint        i_major_inc;    /* pixels per major increment */
} LG_RANGE , *PLG_RANGE;

typedef struct _LG_GRAPH {
    GAPCDataID  cb_id;
    GtkWidget  *drawing_area;
    GdkPixmap  *pixmap;         /* --- Backing pixmap for drawing area  --- */
    GdkGC      *window_gc;
    GdkGC      *box_gc;
    GdkGC      *scale_gc;
    GdkGC      *title_gc;
    GdkGC      *series_gc;
    /* data points and tooltip info */
    gint        i_num_series;   /* 1 based */
    GList      *lg_series;      /* double-linked list of data series PLG_SERIES */
    GList      *lg_series_time; /* time_t of each sample */
    gint        i_points_available;
    gboolean    b_tooltip_active;
    /* actual size of graph area */
    gint        width;
    gint        height;
    /* buffer around all sides */
    gint        x_border;
    gint        y_border;
    /* current mouse position */
    gboolean    b_mouse_onoff;
    GdkPoint    mouse_pos;
    GdkModifierType mouse_state;
    /* top/left or baseline of labels and titles */
    gchar       ch_color_window_bg[GAPC_MAX_TEXT];
    gchar       ch_color_chart_bg[GAPC_MAX_TEXT];
    gchar       ch_color_title_fg[GAPC_MAX_BUFFER];
    GdkRectangle x_label;
    GdkRectangle y_label;
    GdkRectangle x_title;
    GdkRectangle x_tooltip;
    gchar       ch_tooltip_text[GAPC_MAX_BUFFER];
    gchar      *x_label_text;
    gchar      *y_label_text;
    gchar      *x_title_text;
    /* position and area of main graph plot area */
    GdkRectangle plot_box;
    gchar       ch_color_scale_fg[GAPC_MAX_TEXT];
    LG_RANGE    x_range;
    LG_RANGE    y_range;
} LGRAPH   , *PLGRAPH;

/* * Control structure for GtkExtra Charts in Information Window */
typedef struct _History_Page_Data {
   GAPCDataID cb_id;                  /* This is REQUIRED TO BE 1ST in struct   */
   guint cb_monitor_num;           /* monitor number 1-based */
   gpointer *gp;                   /* ptr back to the monitor */
   LGRAPH   *plg;                   /* Line Graph pointer */
   GAPC_SUMS sq[GAPC_LINEGRAPH_MAX_SERIES + 4]; /* data point collector */
   gdouble d_xinc;                 /* base refresh increment for graph */
   gboolean b_startup;
} GAPC_HISTORY, *PGAPC_HISTORY;

/* * Control structure per active monitor icon in panel  */
typedef struct _Monitor_Instance_Data {
   GAPCDataID cb_id;                  /* This is REQUIRED TO BE 1ST in struct   */

   guint cb_monitor_num;           /* Begin Preference values 1-based */
   gboolean cb_enabled;
   gboolean cb_use_systray;
   gchar *pch_host;
   gint i_port;
   gint i_watt;                    /* rated wattage of UPS*/   
   gfloat d_refresh;
   gfloat d_graph;                 /* End Preference values */

   gchar ch_title_info[GAPC_MAX_TEXT];

   GtkWidget *window;              /* information window   */
   GtkWidget *menu;                /* Popup Menu */
   GtkWidget *notebook;            /* information Notebook */
   gboolean b_visible;             /* is the info window visible */
   guint i_info_context;           /* StatusBar message Context */

   gboolean b_run;                 /* controller for all monitor resources -- except thread */
   gboolean b_thread_stop;         /* single flag to stop thread */
   GThread *tid_thread_qwork;      /* Background Thread */
   GMutex *gm_update;              /* Control mutex for hashtables and thread */
   GAsyncQueue *q_network;
   guint i_netbusy_counter;
   guint tid_automatic_refresh;    /* monitor refresh timer id */
   guint tid_graph_refresh;
   gboolean b_data_available;      /* Flag from thread indicating data ready */
   gboolean b_network_control;     /* TRUE signals resolve address needed */

   gboolean b_timer_control;       /* TRUE signals change in refresh interval */
   gboolean b_graph_control;       /* TRUE signals change in refresh interval */
   gboolean b_refresh_button;      /* Flag to thread to immediately update */

   GHashTable *pht_Status;         /* Private hashtable status key=values */
   GHashTable *pht_Widgets;        /* Private hashtable holding widget ptrs  */

   GtkTooltips *tooltips;
   guint i_icon_index;
   gint i_old_icon_index;
   gint i_icon_size;
   gint i_icon_height;
   gint i_icon_width;

   GdkPixbuf **my_icons;

   EggTrayIcon *tray_icon;
   GtkWidget *tray_image;

   GList *data_status;             /* Holds line of status text */
   GList *data_events;             /* Holds line of event text */
   gchar *pach_status[GAPC_MAX_ARRAY];  /* Holds line of status text */
   gchar *pach_events[GAPC_MAX_ARRAY];  /* Holds line of event text */

   GConfClient *client;            /* GCONF id */
   gpointer *gp;                   /* assumed to point to pcfg */
   GtkTreeModel *monitor_model;    /* GtkListStore */
   GAPC_HISTORY phs;               /* structure for history notebook page */
   PSKCOMM      psk;               /* communication structure */
} GAPC_MONITOR, *PGAPC_MONITOR;

/* * Control structure for root panel object -- this is the anchor */
typedef struct _System_Control_Data {
   GAPCDataID cb_id;                  /* This is REQUIRED TO BE 1ST in struct  */
   GList *cb_glist_monitors;       /* assumed to point to  PGAPC_MONITOR */
   guint cb_last_monitor;          /* last selected from icon list - 1-based */
   gboolean b_use_systray;         /* gconf parms */
   gboolean b_tooltips;
   gboolean b_run;                 /* operational flag */
   gchar *pch_gkeys[GAPC_N_PREFS_COLUMNS];
   GConfClient *client;            /* GCONF id */
   guint i_group_id;               /* GCONF dir notify ids - controller */
   guint i_prefs_id;               /* GCONF dir notify ids - prefs-view */

   GtkWidget *window;
   GtkWidget *menu;                /* Popup Menu */
   gboolean b_visible;             /* is the info window visible */

   GtkTreeModel *prefs_model;      /* GtkListStore */
   GtkTreeView *prefs_treeview;
   GtkTreeSelection *prefs_select;
   guint prefs_last_monitor;       /* assigning monitor numbers */
   gint cb_last_monitor_deleted;   /* overide gconf inconsistency on kde */

   GtkTreeModel *monitor_model;    /* GtkListStore */
   GtkTreeView *monitor_treeview;
   GtkTreeSelection *monitor_select;

   GtkWidget *image;
   GtkTooltips *tooltips;

   EggTrayIcon *tray_icon;
   GtkWidget *tray_image;
   GtkTooltips *tray_tooltips;
   gint i_icon_size;
   gint i_icon_height;
   gint i_icon_width;

   GHashTable *pht_Widgets;        /* hashtable holding widget ptrs  */
   GHashTable *pht_Status;         /* hashtable holding status text  */
   guint i_info_context;           /* StatusBar Context */
   GdkPixbuf *my_icons[GAPC_N_ICONS + 8];

   /* Global graph properties */
   GdkColor    color_linev;
   GdkColor    color_loadpct;   
   GdkColor    color_timeleft;   
   GdkColor    color_bcharge;
   GdkColor    color_battv;
   GdkColor    color_window;
   GdkColor    color_chart;   
   GdkColor    color_title;   

} GAPC_CONFIG, *PGAPC_CONFIG;

/* ************************************************************************* */

#define GAPC_GLOSSARY  "<span size=\"xx-large\"><b>GAPCMON</b></span>\n \
A monitor for UPS's under the management of APCUPSD.\n\n \
When active, gapcmon provides three visual objects to interact with. \
First is the main control.panel where monitors are defined, enabled, and listed when \
active. Second are notification area icons that manage the visibility of each window or \
panel. The third is an information window showing historical and current details of \
a UPS being monitored. \n\
\n\n\
<big><b>CONTROL PANEL WINDOW PAGES</b></big>\n\
<b>ACTIVE MONITORS PAGE</b>\n\
\n\
A short list of the monitors that are currently enabled.  The list shows \
each monitor's current icon, its status, and a brief summary of its key metrics.  \
Double-clicking a row causes the information window of that monitor to be presented.\n\
\n\
<b>PREFERENCES PAGE</b>\n\
<i>-for the monitors\n</i>\
\n\
<b><i>Enable:</i></b>\nCauses the monitor to immediately run, create an info-window, \
and add an entry in the icon list window.\n\
\n\
<b><i>Use Trayicon:</i></b>\nAdds a notification area icon which toggles \
the visibility of the monitor info-window when clicked.\n\
\n\
<b><i>Network refresh:</i></b>\nThe number of seconds between collections of status and event \
data from the network.\n\
\n\
<b><i>Graph refresh:</i></b>\nMultiplied by the network refresh value to determine \
the total number of seconds between graph data collections.\n\
\n\
<b><i>Hostname or IP Address:</i></b>\nThe hostname or address where an apcupsd NIS interface \
is running.\n\
\n\
<b><i>Port:</i></b>\nThe NIS access port on the APCUPSD host; defaults to 3551.\n\
\n\
<b><i>Add | Remove Buttons:</i></b>\nButtons to add or remove a monitor entry from the \
list of monitors above.  Add, adds with defaulted values at end of list.  Remove, removes \
the currently selected row.\n\
\n\n\
<i>-for the control.panel\n</i>\
\n\
<b><i>Use Trayicon:</i></b>\nAdds a notification area icon which toggles \
the visibility of the control panel when clicked.  <i><b>Note:</b> all tray icons \
contain a popup menu with the choices of 'JumpTo' interactive window, and 'Quit' \
which either hides the window or destroys it in the case of the control panel. \
Additionally, when use_trayicon is selected, the title of the window is removed from \
the desktop's windowlist or taskbar.</i>\n\
\n\n\
<b>GRAPH PROPERTIES PAGE</b>\n\
\n\
Allows you to specify the colors to be used for each of the five data series on the \
Historical Summary graph page of each monitor.   General window background colors can \
also be specified.\n\
\n\n\
<b>GLOSSARY PAGE</b>\n\
\n\
This page of introductory text.\n\
\n\
<b>ABOUT PAGE</b>\n\
More standard vanity, and my e-mail ID in case something breaks.\n\
\n\n\
<big><b>MONITOR INFORMATION WINDOW PAGES</b></big>\n\
<b>HISTORICAL SUMMARY PAGE</b>\n\
A graph showing the last 40 samples of five key data points, scaled to represent all \
points as a percentage of that value's normal range.  A data point's value can be \
viewed by moving the mouse over any desired point, a tooltip will appear \
showing the color and value of all points at that interval.  Data points are collected \
periodically, based on the product of graph_refresh times network_refresh in seconds. \
 These tooltips can be enabled or disabled by clicking anywhere on the graph once.\n\
\n\
<b>DETAILED INFORMATION PAGE</b>\n\
A more in-depth view of the monitored UPS's environmental values.  Software, product, \
and operational values are available and updated every 'network_refresh' seconds.\n\
\n\
<b>POWER EVENTS PAGE</b>\n\
A log of all power events recorded by APCUPSD on the server.\n\
\n\
<b>FULL UPS STATUS PAGE</b>\n\
A listing of the output from apcaccess showing the actual state as reported \
by the UPS.\n\
\n\
 "

/* ************************************************************************* */

G_END_DECLS
#endif                             /*GAPC_H_ */
