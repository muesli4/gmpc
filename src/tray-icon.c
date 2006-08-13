#include <config.h>

#include <gtk/gtk.h>
#include <string.h>
#include <glade/glade.h>
#include "playlist3.h"

#include "main.h"
#include "misc.h"
#include "config1.h"

GladeXML *tray_pref_xml = NULL;
#ifdef ENABLE_TRAYICON
#include "eggtrayicon.h"
#endif
#define BORDER_WIDTH 6
void destroy_tray_icon();
void tray_icon_song_change();
void tray_icon_state_change();
void tray_cover_art_fetched(mpd_Song *song,MetaDataResult ret, char *path, gpointer data);
void tray_leave_cb (GtkWidget *w, GdkEventCrossing *e, gpointer n);
void TrayStatusChanged(MpdObj *mi, ChangedStatusType what, void *userdata);


/* do tray */
int create_tray_icon();
void tray_init();


GtkWidget *tooltip_pb = NULL;
GtkWidget *tooltip_label = NULL;

extern GladeXML *pl3_xml;
#ifdef ENABLE_TRAYICON
EggTrayIcon *tray_icon = NULL;
#else
GtkWidget *tray_icon = NULL;
#endif
GladeXML *tray_xml = NULL;
GtkWidget *logo = NULL;
GtkTooltips *tps = NULL;
extern int pl3_hidden;
GtkWidget *tip = NULL;

guint popup_timeout = -1;



void tray_icon_pref_construct(GtkWidget *container);
void tray_icon_pref_destroy(GtkWidget *container);

/* plugin structure */

gmpcPrefPlugin tray_gpp = {
	tray_icon_pref_construct,
	tray_icon_pref_destroy
};

gmpcPlugin tray_icon_plug = {
	"Notification",
	{1,1,1},
	GMPC_INTERNALL,
	0,
	NULL,		/* path */
	&tray_init, 	/*initialize function */
	NULL,
	&TrayStatusChanged,
	NULL,
	&tray_gpp,	/* preferences */
	NULL,
	NULL,
	NULL
};


void tray_init()
{
	/* create a tray icon */
	if (cfg_get_single_value_as_int_with_default(config, "tray-icon", "enable",1))
	{
		create_tray_icon();
	}
}


/**/
gchar *tray_get_tooltip_text()
{
	GString *string = g_string_new("");

	gchar result[1024];
	gchar *retval;
	int id;
	if(mpd_check_connected(connection) && mpd_player_get_state(connection) != MPD_PLAYER_STOP)
	{
		mpd_Song *song = mpd_playlist_get_current_song(connection);
		mpd_song_markup(result, 1024, DEFAULT_TRAY_MARKUP, song);
		g_string_append(string, result);
	}
	else
	{
		g_string_append(string,"Gnome Music Player Client");
	}

	/* escape all & signs... needed for pango */
	for(id=0;id < string->len; id++)
	{
		if(string->str[id] == '&')
		{
			g_string_insert(string, id+1, "amp;");
			id++;
		}
	}
	/* return a string (that needs to be free'd */
	retval = string->str;
	g_string_free(string, FALSE);
	return retval;
}

/* fix it the ugly way */
int tooltip_queue_draw(GtkWidget *widget)
{
	gtk_widget_queue_draw(widget);
	return TRUE;
}

int popup_press_event(GtkWidget *wid, GdkEventKey *event)
{
	tray_leave_cb(wid,NULL,NULL);


	return FALSE;
}

gboolean tip_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	cairo_t *cr= gdk_cairo_create(GTK_WIDGET(widget)->window);

	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_rectangle(cr,0,0,widget->allocation.width,widget->allocation.height);
	cairo_fill_preserve(cr);
	cairo_set_source_rgb(cr, 0, 0,0);
	cairo_stroke(cr);

	cairo_destroy(cr);	
	return FALSE;
}

gboolean tray_motion_cb (GtkWidget *event, GdkEventCrossing *event1, gpointer n)
{
	//int from_tray = GPOINTER_TO_INT(n);
	char *tooltiptext = NULL;
	//GtkWidget *eventb;
	if(tip != NULL)
	{
		tray_leave_cb(NULL, NULL, 0);
	}
	tooltiptext = tray_get_tooltip_text();
	tip = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_container_set_border_width(GTK_CONTAINER(tip), 1);
	gtk_widget_set_size_request(tip, 300,90);
	gtk_window_set_title(GTK_WINDOW(tip), "gmpc tray tooltip");
	{
		GtkWidget *alimg, *hbox, *vbox;
		
		hbox = gtk_hbox_new(FALSE, 0);
		gtk_widget_set_app_paintable(GTK_WIDGET(tip), TRUE);
		g_signal_connect(G_OBJECT(tip), "expose-event", G_CALLBACK(tip_expose_event), NULL);
		
		alimg = gmpc_metaimage_new(META_ALBUM_ART);
		gmpc_metaimage_set_connection(GMPC_METAIMAGE(alimg), connection);
		gmpc_metaimage_set_size(GMPC_METAIMAGE(alimg), 80);
		gtk_widget_set_size_request(GTK_WIDGET(alimg), 86,86);
		gmpc_metaimage_update_cover(GMPC_METAIMAGE(alimg), connection, MPD_CST_SONGID,NULL);

		gtk_widget_modify_bg(GTK_WIDGET(alimg),GTK_STATE_NORMAL, &(tip->style->bg[GTK_STATE_SELECTED]));

		gtk_container_add(GTK_CONTAINER(tip), hbox);
		gtk_box_pack_start(GTK_BOX(hbox), alimg,FALSE, TRUE,0);

		vbox = gtk_vbox_new(FALSE,0);
		gtk_container_set_border_width(GTK_CONTAINER(vbox), 3);
		
		tooltip_label = gtk_label_new("");
		gtk_widget_modify_text(GTK_WIDGET(tooltip_label),GTK_STATE_NORMAL, &(tip->style->black));
		gtk_label_set_markup(GTK_LABEL(tooltip_label), tooltiptext);
		gtk_misc_set_alignment(GTK_MISC(tooltip_label), 0,0);
		gtk_label_set_ellipsize(GTK_LABEL(tooltip_label), PANGO_ELLIPSIZE_END);
		gtk_box_pack_start(GTK_BOX(vbox), tooltip_label,TRUE, TRUE,0);



		tooltip_pb = gtk_progress_bar_new();
		{
			int totalTime = mpd_status_get_total_song_time(connection);
			int elapsedTime = mpd_status_get_elapsed_song_time(connection);	
			char*label = g_strdup_printf("%02i:%02i/%02i:%02i", elapsedTime/60, elapsedTime%60,
					totalTime/60,totalTime%60);
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(tooltip_pb), 
					(elapsedTime/(gdouble)totalTime<0)?0:(elapsedTime/(gdouble)totalTime));
			gtk_progress_bar_set_text(GTK_PROGRESS_BAR(tooltip_pb), label);
			g_free(label);
		}

		gtk_box_pack_start(GTK_BOX(vbox), tooltip_pb,FALSE, TRUE,0);	
		gtk_box_pack_start(GTK_BOX(hbox), vbox,TRUE, TRUE,0);
	
	
		
		gtk_widget_show_all(hbox);
	}

	g_free(tooltiptext);
	/** Position popup 
	 * FIXME: Do this propperly following the users settings.
	 */
	{
		int x_tv, y_tv;
		int x, y;
		GtkRequisition req;
		gdk_window_get_origin(GTK_WIDGET(tray_icon)->window, &x_tv, &y_tv);
		y = y_tv+GTK_WIDGET(tray_icon)->allocation.height;
		x = x_tv;
		gtk_widget_size_request(tip, &req);	
		x -= req.width/2;
		
		gtk_window_move(GTK_WINDOW(tip), x, y);
		gtk_widget_show_all(tip);





	}
	return TRUE;
}

void tray_leave_cb (GtkWidget *w, GdkEventCrossing *e, gpointer n)
{
	if(popup_timeout != -1) g_source_remove(popup_timeout);
	popup_timeout = -1;

	if(tip != NULL)
	{
		gtk_widget_destroy(tip);
		tooltip_pb =NULL;
		tooltip_label = NULL;
	}

	tip = NULL;
}

/* this function updates the trayicon on changes */
void tray_icon_song_change()
{
	if(cfg_get_single_value_as_int_with_default(config, "tray-icon", "do-popup", 1) &&
			mpd_player_get_state(connection) != MPD_PLAYER_STOP)
	{
		tray_leave_cb(NULL, NULL, NULL);	
		tray_motion_cb((GtkWidget*)tray_icon,NULL,GINT_TO_POINTER(0));

		if(cfg_get_single_value_as_int_with_default(config, "tray-icon","popup-timeout",5))
		{
			if(popup_timeout != -1)
			{
				g_source_remove(popup_timeout);
			}
			popup_timeout = g_timeout_add(cfg_get_single_value_as_int_with_default(config, "tray-icon","popup-timeout",5)*1000,
					(GSourceFunc)(tray_leave_cb),
					NULL);
		}

	}
}

void tray_icon_connection_changed(MpdObj *mi, int connect)
{
	if(connect){
		if(tray_icon)gtk_image_set_from_stock(GTK_IMAGE(logo), "gmpc-tray", -1);
		tray_icon_state_change();
	}
	else{
		if(tray_icon)gtk_image_set_from_stock(GTK_IMAGE(logo), "gmpc-tray-disconnected", -1);
	}
}

void tray_icon_state_change()
{
	int state = mpd_player_get_state(connection);
	if(state == MPD_PLAYER_STOP || state == MPD_PLAYER_UNKNOWN)
	{
		if(tray_icon)gtk_image_set_from_stock(GTK_IMAGE(logo), "gmpc-tray", -1);
		/*if(cover_pb)
		  {
		  g_object_unref(cover_pb);
		  cover_pb = NULL;
		  }*/
	}
	else if(state == MPD_PLAYER_PLAY){

		if(tray_icon)	gtk_image_set_from_stock(GTK_IMAGE(logo), "gmpc-tray-play", -1);
		tray_icon_song_change();
	}
	else if(state == MPD_PLAYER_PAUSE){
		if(tray_icon)gtk_image_set_from_stock(GTK_IMAGE(logo), "gmpc-tray-pause", -1);
	}
	if(tray_icon)
	{
		gtk_widget_queue_draw(GTK_WIDGET(tray_icon));
	}
}

/* if the item was destroyed and the user still wants an icon recreate it */
/* if the main window was hidden and the icon was destroyed, popup the main window because otherwise you wouldnt be able to 
 * get it back
 */

void tray_icon_destroyed()
{
	tray_icon = NULL;
	if(cfg_get_single_value_as_int_with_default(config, "tray-icon", "enable", 1))
	{
		g_idle_add((GSourceFunc)create_tray_icon, NULL);
	}
	if(pl3_hidden)
	{
		create_playlist3();
	}
}

/* destroy the tray icon */
void destroy_tray_icon()
{
	gtk_widget_destroy(GTK_WIDGET(tray_icon));
}

/* wrong name: this handles clickes on the tray icon
 * button1: present/hide window
 * button2: play/pause
 * button3: menu
 */

void tray_icon_info()
{
	mpd_Song *song = mpd_playlist_get_current_song(connection);
	if(song)
	{
		call_id3_window_song(mpd_songDup(song));
	}
}

int  tray_mouse_menu(GtkWidget *wid, GdkEventButton *event)
{
	if(event->button == 1 && event->state != (GDK_CONTROL_MASK|GDK_BUTTON1_MASK))
	{

		gtk_widget_queue_draw(GTK_WIDGET(tray_icon));
		if(pl3_hidden)
		{
			create_playlist3();
		}
		else{
			pl3_hide();
		}
		gtk_widget_queue_draw(GTK_WIDGET(tray_icon));
	}
	else if (event->button == 2 || (event->button == 1 && event->state == (GDK_CONTROL_MASK|GDK_BUTTON1_MASK)))
	{
		gchar *string = cfg_get_single_value_as_string_with_default(config, "tray-icon","middle-mouse-action","pause");
		if(!strcmp(string,"pause") || !strcmp(string,"play")){
			play_song();
		}
		else if (!strcmp(string,"next")) {
			next_song();
		}
		else if (!strcmp(string,"prev")) {
			prev_song();
		}
		else if (!strcmp(string,"stop")) {
			stop_song();
		}
		cfg_free_string(string);
	}
	else if(event->button == 3)
	{
		GtkWidget *item;
		GtkWidget *menu = gtk_menu_new();

		if(mpd_check_connected(connection))
		{
			if(mpd_server_check_command_allowed(connection, "play"))
			{
				item = gtk_image_menu_item_new_with_mnemonic(_("Pl_ay/Pause"));
				gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
						gtk_image_new_from_stock("gtk-media-play", GTK_ICON_SIZE_MENU));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(play_song), NULL);


				item = gtk_image_menu_item_new_with_mnemonic(_("_Stop"));
				gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
						gtk_image_new_from_stock("gtk-media-stop", GTK_ICON_SIZE_MENU));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(stop_song), NULL);

				item = gtk_image_menu_item_new_with_mnemonic(_("_Next"));
				gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
						gtk_image_new_from_stock("gtk-media-next", GTK_ICON_SIZE_MENU));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(next_song), NULL);


				item = gtk_image_menu_item_new_with_mnemonic(_("_Previous"));
				gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
						gtk_image_new_from_stock("gtk-media-previous", GTK_ICON_SIZE_MENU));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(prev_song), NULL);
				item = gtk_separator_menu_item_new();
				gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);
			}
		}
		else
		{
			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CONNECT, NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(connect_to_mpd), NULL);
			item = gtk_separator_menu_item_new();
			gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);			
		}
		item = gtk_image_menu_item_new_with_mnemonic(_("Pla_ylist"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock("gtk-justify-fill", GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(create_playlist3), NULL);

		if(mpd_player_get_state(connection) == MPD_PLAYER_PLAY ||
				mpd_player_get_state(connection) == MPD_PLAYER_PAUSE)
		{
			item = gtk_image_menu_item_new_with_mnemonic(_("Song _Information"));
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
					gtk_image_new_from_stock("gtk-info", GTK_ICON_SIZE_MENU));
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(tray_icon_info), NULL);

		}
		item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);

		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(main_quit), NULL);
		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 3, event->time);
	}
	return FALSE;

}

int scroll_event(GtkWidget *eventb, GdkEventScroll *event)
{
	if(event->type == GDK_SCROLL)
	{
		if(event->direction == GDK_SCROLL_UP)
		{
			if(mpd_server_check_command_allowed(connection, "volume") == MPD_SERVER_COMMAND_ALLOWED)
				mpd_status_set_volume(connection,mpd_status_get_volume(connection)+5);
		}
		else if (event->direction == GDK_SCROLL_DOWN)
		{
			if(mpd_server_check_command_allowed(connection, "volume") == MPD_SERVER_COMMAND_ALLOWED)
				mpd_status_set_volume(connection,mpd_status_get_volume(connection)-5);
		}
		else if(event->direction == GDK_SCROLL_LEFT)
		{
			prev_song();
		}
		else if (event->direction == GDK_SCROLL_RIGHT)
		{
			next_song();
		}
	}

	return FALSE;
}


int create_tray_icon()
{
#ifdef ENABLE_TRAYICON		
	GtkWidget *event;
	if(tray_icon != NULL)
	{
		return FALSE;
	}
	/* set up tray icon */
	tray_icon = egg_tray_icon_new(_("Gnome Music Player Client"));
	event = gtk_event_box_new();
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(event), FALSE);
	logo = 	gtk_image_new_from_stock("gmpc-tray-disconnected",-1);//gtk_event_box_new();



	gtk_widget_show(event);
	gtk_widget_show(logo);
	gtk_container_add(GTK_CONTAINER(event), logo);
	gtk_container_add(GTK_CONTAINER(tray_icon), event);
	g_signal_connect(G_OBJECT(event), "button-release-event", G_CALLBACK(tray_mouse_menu), NULL);
	g_signal_connect(G_OBJECT(tray_icon), "destroy", G_CALLBACK(tray_icon_destroyed), NULL);

	gtk_widget_add_events (GTK_WIDGET (tray_icon),
			GDK_BUTTON_PRESS_MASK);


	g_signal_connect(G_OBJECT(event), "enter-notify-event", 
			G_CALLBACK(tray_motion_cb), GINT_TO_POINTER(1));
	g_signal_connect(G_OBJECT(event), "leave-notify-event",
			G_CALLBACK(tray_leave_cb), NULL);
	g_signal_connect(G_OBJECT(event), "scroll-event", G_CALLBACK(scroll_event), NULL);
	/* show all */
	gtk_widget_show(GTK_WIDGET(tray_icon));
	if(tps == NULL)	tps = gtk_tooltips_new();

	/* make sure the icon gets updated propperly */
	tray_icon_connection_changed(connection, mpd_check_connected(connection));
	tray_icon_state_change();
#endif
	return FALSE;
}

void   TrayStatusChanged(MpdObj *mi, ChangedStatusType what, void *userdata)
{
	if(what&MPD_CST_STATE)
	{
		tray_icon_state_change();
	}
	else if(what&MPD_CST_SONGID)
	{
		if(!tip)
		{
			tray_icon_song_change();	
		}
		else
		{
			char *tooltiptext = tray_get_tooltip_text();
			gtk_label_set_markup(GTK_LABEL(tooltip_label), tooltiptext);
			g_free(tooltiptext);
			if(popup_timeout != -1)
			{
				g_source_remove(popup_timeout);
			}
			popup_timeout = g_timeout_add(cfg_get_single_value_as_int_with_default(config, "tray-icon","popup-timeout",5)*1000,
					(GSourceFunc)(tray_leave_cb),
					NULL);
		}
	}
	if(what&MPD_CST_ELAPSED_TIME)
	{
		if(tooltip_pb)
		{
			int totalTime = mpd_status_get_total_song_time(connection);                                 		
			int elapsedTime = mpd_status_get_elapsed_song_time(connection);	
			char*label = g_strdup_printf("%02i:%02i/%02i:%02i", elapsedTime/60, elapsedTime%60,
					totalTime/60,totalTime%60);
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(tooltip_pb), 
					(elapsedTime/(gdouble)totalTime<0)?0:(elapsedTime/(gdouble)totalTime));
			gtk_progress_bar_set_text(GTK_PROGRESS_BAR(tooltip_pb), label);
			g_free(label);
		}

	}
}
//#endif

/* PREFERENCES */


void tray_enable_toggled(GtkToggleButton *but)
{
	debug_printf(DEBUG_INFO,"tray-icon.c: changing tray icon %i\n", gtk_toggle_button_get_active(but));
	cfg_set_single_value_as_int(config, "tray-icon", "enable", (int)gtk_toggle_button_get_active(but));
	if(cfg_get_single_value_as_int_with_default(config, "tray-icon", "enable", 1))
	{
		create_tray_icon();
	}
	else
	{
		destroy_tray_icon();
	}
}

/* this sets all the settings in the notification area preferences correct */
void tray_update_settings()
{
	gtk_toggle_button_set_active((GtkToggleButton *)
			glade_xml_get_widget(tray_pref_xml, "ck_tray_enable"), 
			cfg_get_single_value_as_int_with_default(config, "tray-icon", "enable", DEFAULT_TRAY_ICON_ENABLE));
}

void popup_enable_toggled(GtkToggleButton *but)
{
	cfg_set_single_value_as_int(config, "tray-icon", "do-popup", gtk_toggle_button_get_active(but));
}


void popup_position_changed(GtkComboBox *om)
{
	cfg_set_single_value_as_int(config, "tray-icon", "popup-location", gtk_combo_box_get_active(om));
}

void popup_timeout_changed()
{
	cfg_set_single_value_as_int(config, "tray-icon", "popup-timeout",
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(tray_pref_xml, "popup_timeout"))));
}

void update_popup_settings()
{
	gtk_toggle_button_set_active((GtkToggleButton *)
			glade_xml_get_widget(tray_pref_xml, "ck_popup_enable"),
			cfg_get_single_value_as_int_with_default(config, "tray-icon", "do-popup", 1));
	gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(tray_pref_xml, "om_popup_position")),
			cfg_get_single_value_as_int_with_default(config, "tray-icon", "popup-location", 0));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(tray_pref_xml, "popup_timeout")),
			cfg_get_single_value_as_int_with_default(config, "tray-icon", "popup-timeout", 5));
}

void tray_icon_pref_destroy(GtkWidget *container)
{
	if(tray_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(tray_pref_xml, "tray-pref-vbox");
		gtk_container_remove(GTK_CONTAINER(container),vbox);
		g_object_unref(tray_pref_xml);
		tray_pref_xml = NULL;
	}
}
void tray_icon_pref_construct(GtkWidget *container)
{
	gchar *path = gmpc_get_full_glade_path("gmpc.glade");
	tray_pref_xml = glade_xml_new(path, "tray-pref-vbox",NULL);

	if(tray_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(tray_pref_xml, "tray-pref-vbox");
		gtk_container_add(GTK_CONTAINER(container),vbox);
		tray_update_settings();
		update_popup_settings();
#ifndef ENABLE_TRAYICON
		gtk_widget_hide(glade_xml_get_widget(tray_pref_xml, "frame16"));
#endif				
		glade_xml_signal_autoconnect(tray_pref_xml);
	}
}

