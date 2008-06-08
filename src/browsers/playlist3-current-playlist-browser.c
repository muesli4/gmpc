/*
 *Copyright (C) 2004-2007 Qball Cow <qball@sarine.nl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include <glade/glade.h>
#include <config.h>
#include <regex.h>

#include "plugin.h"

#include "main.h"
#include "misc.h"
#include "playlist3.h"
#include "playlist3-current-playlist-browser.h"
#include "config1.h"
#include "TreeSearchWidget.h"
#include "gmpc-mpddata-model.h"
#include "gmpc-mpddata-model-playlist.h"
#include "gmpc-mpddata-treeview.h"
#include "eggcolumnchooserdialog.h"
#include "sexy-icon-entry.h"

static int pl3_current_playlist_browser_button_press_event(GtkTreeView *tree, GdkEventButton *event);
static void pl3_current_playlist_browser_scroll_to_current_song(void);
static void pl3_current_playlist_browser_add(GtkWidget *cat_tree);

static void pl3_current_playlist_browser_selected(GtkWidget *container);
static void pl3_current_playlist_browser_unselected(GtkWidget *container);

static int pl3_current_playlist_browser_cat_menu_popup(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event);


static void pl3_current_playlist_status_changed(MpdObj *mi, ChangedStatusType what, void *userdata);
static int pl3_current_playlist_browser_add_go_menu(GtkWidget *menu);

/* just for here */
static void pl3_current_playlist_browser_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col);
static int  pl3_current_playlist_browser_button_release_event(GtkTreeView *tree, GdkEventButton *event);
static int  pl3_current_playlist_browser_button_press_event(GtkTreeView *tree, GdkEventButton *event);
static int  pl3_current_playlist_browser_key_release_event(GtkTreeView *tree, GdkEventKey *event,GtkWidget *entry);
static void pl3_current_playlist_browser_show_info(void);
static void pl3_current_playlist_save_playlist(void);
static void pl3_current_playlist_browser_shuffle_playlist(void);
static void pl3_current_playlist_browser_clear_playlist(void);
static int pl3_current_playlist_key_press_event(GtkWidget *mw, GdkEventKey *event, int type);
static void pl3_current_playlist_connection_changed(MpdObj *mi, int connect, gpointer data);
static void pl3_current_playlist_save_myself(void);
static void pl3_current_playlist_browser_init(void);



GtkTreeModel *playlist = NULL;
GtkWidget *pl3_cp_tree = NULL;
static gboolean search_keep_open = FALSE;


/* Tooltip */
static void self_enable_tooltip(GtkWidget *treeview);


static void pl3_cp_current_song_changed(GmpcMpdDataModelPlaylist *model2,GtkTreePath *path, GtkTreeIter *iter,gpointer data)
{
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_cp_tree));
    if(GMPC_IS_MPDDATA_MODEL_PLAYLIST(model))
    {
        if(cfg_get_single_value_as_int_with_default(config, "playlist", "st_cur_song", 0))
        {
            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(pl3_cp_tree),
                    path,
                    NULL,
                    TRUE,0.5,0);
        }
    }
}
static void __real_pl3_total_playtime_changed(GmpcMpdDataModelPlaylist *model, unsigned long loaded_songs, unsigned long total_playtime, gpointer user_data)
{
     if(mpd_playlist_get_playlist_length(connection)&&loaded_songs > 0)

     {
         unsigned long total_songs = GMPC_MPDDATA_MODEL(model)->num_rows;
         guint playtime = total_playtime*((gdouble)(total_songs/(gdouble)loaded_songs));
         gchar *string = format_time(playtime);
         gchar *mesg = NULL;

         mesg = g_strdup_printf("%lu %s%c %s %s (%lu%% counted)", total_songs, 
                 ngettext("item", "items", total_songs ), (string[0])?',':' ', string,     
                 (string[0] == 0 || total_songs == loaded_songs)? "":_("(Estimation)"),(loaded_songs*100)/total_songs);

         pl3_push_rsb_message(mesg);

         q_free(string);

         q_free(mesg);

     } else {

         pl3_push_rsb_message("");

     }
}

static void pl3_total_playtime_changed(GmpcMpdDataModelPlaylist *model, unsigned long loaded_songs, unsigned long total_playtime, gpointer user_data)
{
    if(pl3_cat_get_selected_browser() == current_playlist_plug.id)
    {
        __real_pl3_total_playtime_changed(model, loaded_songs, total_playtime, user_data);
    }
}

static void pl3_cp_init()
{
    playlist = (GtkTreeModel *)gmpc_mpddata_model_playlist_new(gmpcconn,connection);
    pl3_current_playlist_browser_init();
    g_signal_connect(G_OBJECT(playlist), "current_song_changed", G_CALLBACK(pl3_cp_current_song_changed), NULL);
    g_signal_connect(G_OBJECT(playlist), "total_playtime_changed", G_CALLBACK(pl3_total_playtime_changed), NULL);

}

gmpcPlBrowserPlugin current_playlist_gbp = {
	.add = pl3_current_playlist_browser_add,
	.selected = pl3_current_playlist_browser_selected,
	.unselected = pl3_current_playlist_browser_unselected,
	.cat_right_mouse_menu = pl3_current_playlist_browser_cat_menu_popup,
	.add_go_menu = pl3_current_playlist_browser_add_go_menu,
	.key_press_event = pl3_current_playlist_key_press_event
};


gmpcPlugin current_playlist_plug = {
	.name = 							"Play Queue Browser",
	.version = 							{1,1,1},
	.plugin_type =						GMPC_PLUGIN_PL_BROWSER,
    .init   =                           pl3_cp_init,
	.browser = 							&current_playlist_gbp,
	.mpd_status_changed = 				pl3_current_playlist_status_changed,
	.mpd_connection_changed = 			pl3_current_playlist_connection_changed,
	.destroy = 							pl3_current_playlist_destroy,
	.save_yourself = 					pl3_current_playlist_save_myself
};


/* external objects */
extern GladeXML *pl3_xml;

/* internal */

static GtkWidget *pl3_cp_sw = NULL;
static GtkWidget *pl3_cp_vbox = NULL;
static TreeSearch *tree_search = NULL;
static GtkTreeRowReference *pl3_curb_tree_ref = NULL;

static void pl3_current_playlist_search_activate(TreeSearch *search,GtkTreeView *tree)
{
	GtkTreeModel *model = gtk_tree_view_get_model(tree); 
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	if (gtk_tree_selection_count_selected_rows (selection) == 1)            
	{
		GList *list = gtk_tree_selection_get_selected_rows (selection, &model);
		pl3_current_playlist_browser_row_activated(GTK_TREE_VIEW(tree),(GtkTreePath *)list->data, NULL);	
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);                        	
		g_list_free (list);
	}
}



static void pl3_current_playlist_column_changed(GtkTreeView *tree)
{
	int position = 0;
	GList *iter,*cols = gtk_tree_view_get_columns(tree);
	for(iter = cols; iter; iter = g_list_next(iter))
	{
		gpointer data = g_object_get_data(G_OBJECT(iter->data), "colid");
		int colid = GPOINTER_TO_INT(data);
		char *string = g_strdup_printf("%i", position);
		cfg_set_single_value_as_int(config, "current-playlist-column-pos", string, colid);
		position++;
		q_free(string);
	}
	g_list_free(cols);
}

void pl3_current_playlist_destroy()
{
	if(pl3_cp_tree)
	{
		GList *iter,*cols = gtk_tree_view_get_columns(GTK_TREE_VIEW(pl3_cp_tree));
		for(iter = cols; iter; iter = g_list_next(iter))
		{
			gpointer data = g_object_get_data(G_OBJECT(iter->data), "colid");
			int colid = GPOINTER_TO_INT(data);
			gchar *string = g_strdup_printf("%i", colid);
			int width = gtk_tree_view_column_get_width(GTK_TREE_VIEW_COLUMN(iter->data));
			cfg_set_single_value_as_int(config, "current-playlist-column-width", string,width);
			q_free(string);
		}
		g_list_free(cols);

		/* destroy the entry */ 
		if(pl3_curb_tree_ref)
		{
			GtkTreeIter iter;
			GtkTreePath *path;
			path = gtk_tree_row_reference_get_path(pl3_curb_tree_ref);
			if(path)
			{
				if(gtk_tree_model_get_iter(GTK_TREE_MODEL(gtk_tree_row_reference_get_model(pl3_curb_tree_ref)), &iter,path))
				{
					gtk_list_store_remove(GTK_LIST_STORE(gtk_tree_row_reference_get_model(pl3_curb_tree_ref)), &iter);
				}
				gtk_tree_path_free(path);
			}
			gtk_tree_row_reference_free(pl3_curb_tree_ref);
			pl3_curb_tree_ref = NULL;
		}
		/* destroy the browser */
		if(pl3_cp_vbox)
		{
			/* remove the signal handler so when the widget is destroyed, the numbering of the labels are not changed again */
			g_signal_handlers_disconnect_by_func(G_OBJECT(pl3_cp_tree), G_CALLBACK(pl3_current_playlist_column_changed), NULL);
			g_object_unref(pl3_cp_vbox);
			pl3_cp_vbox = NULL;
		}
		pl3_cp_tree =  NULL;
	}
}




static GtkTreeModel *mod_fill = NULL;
static GtkWidget *filter_entry = NULL;
static guint timeout=0;

static gboolean mod_fill_do_entry_changed(GtkWidget *entry, GtkWidget *tree)
{
    const gchar *text2 = gtk_entry_get_text(GTK_ENTRY(entry));
    if(strlen(text2) > 0)
    {
        MpdData *data = NULL;
        gchar **text = tokenize_string(text2);
        int i;
        int searched = 0;
         
         for(i=0;text && text[i];i++)
         {
            if(!searched)
                mpd_playlist_search_start(connection, FALSE);
            mpd_playlist_search_add_constraint(connection, MPD_TAG_ITEM_ANY,text[i]);
            searched = 1;
         }
        if(searched)
            data = mpd_playlist_search_commit(connection);
        gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(mod_fill), data);
        g_strfreev(text);
        gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_cp_tree), mod_fill);
    }
    else
    {
        gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_cp_tree), playlist);
        gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(mod_fill), NULL);
        if(!search_keep_open)
        {
            gtk_widget_hide(entry);
            gtk_widget_grab_focus(pl3_cp_tree);
        }
    }
    timeout = 0;
    return FALSE;
}

static gboolean mod_fill_entry_key_press_event(GtkWidget *entry, GdkEventKey *event, gpointer data)
{
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));

    if(strlen(text) == 0)
    {
        if(event->keyval == GDK_BackSpace || event->keyval == GDK_Escape)
        {
            search_keep_open = FALSE;
            gtk_widget_hide(entry);
            gtk_widget_grab_focus(pl3_cp_tree);
            return TRUE;
        }
    }else if(event->keyval == GDK_Escape)
    {
        search_keep_open = FALSE;
        gtk_entry_set_text(GTK_ENTRY(entry), "");
    }
    return FALSE;
}

static void  mod_fill_entry_changed(GtkWidget *entry, GtkWidget *tree)
{
    if(timeout != 0)
        g_source_remove(timeout);
    timeout = g_timeout_add(1000, (GSourceFunc)mod_fill_do_entry_changed, entry);
    gtk_widget_show(entry);
}


static void pl3_current_playlist_browser_init(void)
{
	pl3_cp_vbox = gtk_vbox_new(FALSE,6);
    GtkWidget *tree = gmpc_mpddata_treeview_new("current-pl", FALSE, GTK_TREE_MODEL(playlist));

    /** 
     * Popup
     */
    self_enable_tooltip(tree);

    /* filter */
    mod_fill = (GtkTreeModel *)gmpc_mpddata_model_new();
    GtkWidget *entry = sexy_icon_entry_new(); 
    sexy_icon_entry_add_clear_button(SEXY_ICON_ENTRY(entry));
    gtk_box_pack_start(GTK_BOX(pl3_cp_vbox), entry, FALSE, TRUE,0);
    filter_entry= entry;
    g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(mod_fill_entry_changed), tree);
    g_signal_connect(G_OBJECT(entry), "key-press-event", G_CALLBACK(mod_fill_entry_key_press_event), NULL);

    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(tree), TRUE);
	GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
    gtk_container_add(GTK_CONTAINER(sw), tree);
	gtk_box_pack_start(GTK_BOX(pl3_cp_vbox), GTK_WIDGET(sw), TRUE, TRUE,0);
    gtk_widget_show_all(sw);
	/* set up the tree */
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree), FALSE);

	/* setup signals */
	g_signal_connect(G_OBJECT(tree), "row-activated",G_CALLBACK(pl3_current_playlist_browser_row_activated), NULL); 
	g_signal_connect(G_OBJECT(tree), "button-press-event", G_CALLBACK(pl3_current_playlist_browser_button_press_event), NULL);
	g_signal_connect(G_OBJECT(tree), "button-release-event", G_CALLBACK(pl3_current_playlist_browser_button_release_event), NULL);
	g_signal_connect(G_OBJECT(tree), "key-press-event", G_CALLBACK(pl3_current_playlist_browser_key_release_event), entry);

	/* set up the scrolled window */
	pl3_cp_sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pl3_cp_sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pl3_cp_sw), GTK_SHADOW_ETCHED_IN);


	tree_search = (TreeSearch *)treesearch_new(GTK_TREE_VIEW(tree), MPDDATA_MODEL_COL_MARKUP);
	g_signal_connect(G_OBJECT(tree_search), "result-activate", G_CALLBACK(pl3_current_playlist_search_activate),tree);


	gtk_box_pack_end(GTK_BOX(pl3_cp_vbox), GTK_WIDGET(tree_search), FALSE, TRUE, 0);	


	/* set initial state */
    pl3_cp_tree = tree;
	g_object_ref(G_OBJECT(pl3_cp_vbox));
}

static void pl3_current_playlist_browser_select_current_song()
{
	if(pl3_cp_tree == NULL|| !GTK_WIDGET_REALIZED(pl3_cp_tree)) return;
	/* scroll to the playing song */
	if(mpd_player_get_current_song_pos(connection) >= 0 && mpd_playlist_get_playlist_length(connection)  > 0)
	{
		GtkTreePath *path = gtk_tree_path_new_from_indices(mpd_player_get_current_song_pos(connection),-1);
		if(path != NULL && GMPC_IS_MPDDATA_MODEL_PLAYLIST(gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_cp_tree))))
		{
        	gtk_tree_view_set_cursor(GTK_TREE_VIEW(pl3_cp_tree), path, NULL, FALSE);
		}
		gtk_tree_path_free(path);
	}      
}

static void pl3_current_playlist_browser_scroll_to_current_song()
{
	if(pl3_cp_tree == NULL || !GTK_WIDGET_REALIZED(pl3_cp_tree)) return;
	/* scroll to the playing song */
	if(mpd_player_get_current_song_pos(connection) >= 0 && mpd_playlist_get_playlist_length(connection)  > 0)
	{
		GtkTreePath *path = gtk_tree_path_new_from_indices(mpd_player_get_current_song_pos(connection),-1);
		if(path != NULL)
		{
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(pl3_cp_tree),
					path,
					NULL,
					TRUE,0.5,0);
			gtk_tree_path_free(path);
		}
	
	}      
}

/* add's the toplevel entry for the current playlist view */
static void pl3_current_playlist_browser_add(GtkWidget *cat_tree)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	gint pos = cfg_get_single_value_as_int_with_default(config, "current-playlist","position",0);
	playlist3_insert_browser(&iter, pos);
	gtk_list_store_set(GTK_LIST_STORE(pl3_tree), &iter, 
			PL3_CAT_TYPE, current_playlist_plug.id,/*PL3_CURRENT_PLAYLIST,*/
			PL3_CAT_TITLE, _("Play Queue"),
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "playlist-browser",
			PL3_CAT_PROC, TRUE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,
			-1);
	if(pl3_curb_tree_ref)
	{
		gtk_tree_row_reference_free(pl3_curb_tree_ref);
		pl3_curb_tree_ref = NULL;
	}
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), &iter);
	if(path)
	{
		pl3_curb_tree_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(pl3_tree),path);
		gtk_tree_path_free(path);
	}
}


/* delete all selected songs,
 * if no songs select ask the user if he want's to clear the list 
 */
static void pl3_current_playlist_browser_delete_selected_songs ()
{
	/* grab the selection from the tree */
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_cp_tree));
	/* check if where connected */
	/* see if there is a row selected */
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GList *list = NULL, *llist = NULL;
        GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_cp_tree));
		/* start a command list */
		/* grab the selected songs */
		list = gtk_tree_selection_get_selected_rows (selection, &model);
		/* grab the last song that is selected */
		llist = g_list_first (list);
		/* remove every selected song one by one */
		do{
			GtkTreeIter iter;
			int value;
			gtk_tree_model_get_iter (model, &iter,(GtkTreePath *) llist->data);
			gtk_tree_model_get (model, &iter, MPDDATA_MODEL_COL_SONG_ID, &value, -1);
			mpd_playlist_queue_delete_id(connection, value);			
		} while ((llist = g_list_next (llist)));

		/* close the list, so it will be executed */
		mpd_playlist_queue_commit(connection);
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
	else
	{
		/* create a warning message dialog */
		GtkWidget *dialog =
			gtk_message_dialog_new (GTK_WINDOW
					(glade_xml_get_widget
					 (pl3_xml, "pl3_win")),
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_WARNING,
					GTK_BUTTONS_NONE,
					_("Are you sure you want to clear the playlist?"));
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL,
				GTK_RESPONSE_CANCEL, GTK_STOCK_OK,
				GTK_RESPONSE_OK, NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog),
				GTK_RESPONSE_CANCEL);

		switch (gtk_dialog_run (GTK_DIALOG (dialog)))
		{
			case GTK_RESPONSE_OK:
				/* check if where still connected */
				mpd_playlist_clear(connection);
		}
		gtk_widget_destroy (GTK_WIDGET (dialog));
	}
	/* update everything if where still connected */
	gtk_tree_selection_unselect_all(selection);

	mpd_status_queue_update(connection);
}

static void pl3_current_playlist_browser_crop_selected_songs()
{
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_cp_tree));
	/* grab the selection from the tree */
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_cp_tree));

	/* see if there is a row selected */	
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GtkTreeIter iter;


		/* start a command list */
		/* remove every selected song one by one */
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter))
		{
			do{
				int value=0;
				if(!gtk_tree_selection_iter_is_selected(selection, &iter))
				{
					gtk_tree_model_get (GTK_TREE_MODEL(model), &iter, MPDDATA_MODEL_COL_SONG_ID, &value, -1);
					mpd_playlist_queue_delete_id(connection, value);				
				}
			} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(model),&iter));
			mpd_playlist_queue_commit(connection);
		}

	}
	/* update everything if where still connected */
	gtk_tree_selection_unselect_all(selection);

	mpd_status_queue_update(connection);
}

static void pl3_current_playlist_editor_add_to_playlist(GtkWidget *menu)
{
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_cp_tree));
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_cp_tree));
	gchar *data = g_object_get_data(G_OBJECT(menu), "playlist");
	GList *iter, *list = gtk_tree_selection_get_selected_rows (selection, &model);
	if(list)
	{
		iter = g_list_first(list);
		do{
			GtkTreeIter giter;
			if(gtk_tree_model_get_iter(model, &giter, (GtkTreePath *)iter->data))
			{
				gchar *file = NULL;
				gtk_tree_model_get(model, &giter, MPDDATA_MODEL_COL_PATH, &file, -1);
				mpd_database_playlist_list_add(connection, data,file); 
				g_free(file);
			}
		}while((iter = g_list_next(iter)));

		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);                        	
		g_list_free (list);
	}

	playlist_editor_fill_list();
}
static int pl3_current_playlist_browser_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	GtkTreePath *path = NULL;
	if(event->button == 3 &&gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree), event->x, event->y,&path,NULL,NULL,NULL))
	{	
		GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
		if(gtk_tree_selection_path_is_selected(sel, path))
		{
			gtk_tree_path_free(path);
			return TRUE;
		}
	}
	if(path) {
		gtk_tree_path_free(path);
	}
	return FALSE;                                                                                                     
}
static void pl3_current_playlist_browser_edit_columns(void)
{
  gmpc_mpddata_treeview_edit_columns(GMPC_MPDDATA_TREEVIEW(pl3_cp_tree));
}

static int pl3_current_playlist_browser_button_release_event(GtkTreeView *tree, GdkEventButton *event)
{
	if(event->button == 3)
	{
		/* del, crop */
		GtkWidget *item;
		GtkWidget *menu = gtk_menu_new();	
    
        /* */
        if(gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(tree)) == 1)	
        {
            mpd_Song *song;
            GtkTreeIter iter;
            GtkTreePath *path;
            GtkTreeModel *model = gtk_tree_view_get_model(tree);
            GList *list = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), &model);
            path = list->data;
            /* free result */
            g_list_free(list);
            if(path && gtk_tree_model_get_iter(model, &iter, path)) {
                gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_MPDSONG, &song, -1);
                if(song)
                {
                    submenu_for_song(menu, song);
                }
                if(path)                                                                
                    gtk_tree_path_free(path);                                               
            }
        }

        /* add the delete widget */
        item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE,NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_delete_selected_songs), NULL);

        /* add the delete widget */
        item = gtk_image_menu_item_new_with_label(_("Crop"));
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                gtk_image_new_from_stock(GTK_STOCK_CUT, GTK_ICON_SIZE_MENU));
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_crop_selected_songs), NULL);		
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

        gtk_menu_shell_append(GTK_MENU_SHELL(menu),gtk_separator_menu_item_new());
        /* add the clear widget */
        item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR,NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_clear_playlist), NULL);		


        /* add the shuffle widget */
        item = gtk_image_menu_item_new_with_label(_("Shuffle"));
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_shuffle_playlist), NULL);		
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

        gtk_menu_shell_append(GTK_MENU_SHELL(menu),gtk_separator_menu_item_new());

        item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DIALOG_INFO,NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_show_info), NULL);		

        /* add the shuffle widget */
 

        item = gtk_image_menu_item_new_with_label(_("Edit Columns"));
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(pl3_current_playlist_browser_edit_columns), NULL);





        playlist_editor_right_mouse(menu,pl3_current_playlist_editor_add_to_playlist);

        gtk_widget_show_all(menu);
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL,0, event->time);	
        return TRUE;
    }
    return FALSE;
}

static void pl3_current_playlist_browser_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col)
{
    GtkTreeIter iter;
    gint song_id;
    gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, path);
    gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, MPDDATA_MODEL_COL_SONG_ID,&song_id, -1);
    mpd_player_play_id(connection, song_id);
}

static void pl3_current_playlist_browser_show_info()
{

    GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW(pl3_cp_tree));
    GtkTreeSelection *selection =gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_cp_tree));
    if (gtk_tree_selection_count_selected_rows (selection) > 0)
    {
        GList *list = NULL;
        list = gtk_tree_selection_get_selected_rows (selection, &model);

        list = g_list_last (list);

        {
            GtkTreeIter iter;
            mpd_Song *song = NULL;
            gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
            gtk_tree_model_get (model, &iter, MPDDATA_MODEL_COL_MPDSONG, &song, -1);

            info2_activate();
            info2_fill_song_view(song);	
        }


        g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (list);
    }
}

static void pl3_current_playlist_browser_selected(GtkWidget *container)
{
    unsigned long a = 0,b = 0;
    if(pl3_cp_vbox == NULL)
    {
        pl3_current_playlist_browser_init();
    }
    gtk_container_add(GTK_CONTAINER(container), pl3_cp_vbox);
    gtk_widget_show(pl3_cp_vbox);

    gtk_widget_grab_focus(pl3_cp_tree);
    gmpc_mpddata_model_playlist_get_total_playtime(GMPC_MPDDATA_MODEL_PLAYLIST(playlist), &a, &b);
    __real_pl3_total_playtime_changed(GMPC_MPDDATA_MODEL_PLAYLIST(playlist),a,b,NULL);

    gtk_widget_grab_focus(pl3_cp_tree);
}
static void pl3_current_playlist_browser_unselected(GtkWidget *container)
{
    gtk_container_remove(GTK_CONTAINER(container), pl3_cp_vbox);
}


static int pl3_current_playlist_browser_cat_menu_popup(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event)
{
    /* here we have:  Save, Clear*/
    GtkWidget *item;
    if(type != current_playlist_plug.id) return 0;
    /* add the save widget */
    item = gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE,NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_save_playlist), NULL);

    /* add the clear widget */
    item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR,NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_clear_playlist), NULL);

    item = gtk_image_menu_item_new_with_label(_("Add URL"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
		    gtk_image_new_from_icon_name("gmpc-add-url", GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(url_start), NULL);

    return 1;
}

static int  pl3_current_playlist_browser_key_release_event(GtkTreeView *tree, GdkEventKey *event,GtkWidget *entry)
{
    if(event->keyval == GDK_Delete)
    {
        pl3_current_playlist_browser_delete_selected_songs ();
        return TRUE;                                          		
    }
    else if(event->keyval == GDK_i && event->state&GDK_MOD1_MASK)
    {
        pl3_current_playlist_browser_show_info();
        return TRUE;
    }
    else if (event->keyval == GDK_space)
    {
        pl3_current_playlist_browser_scroll_to_current_song();
        pl3_current_playlist_browser_select_current_song();
        return TRUE;			
    }
    else if (event->keyval == GDK_f && event->state&GDK_CONTROL_MASK)
    {
        mod_fill_entry_changed(entry, NULL);
        gtk_widget_grab_focus(entry);        
        search_keep_open = TRUE;
        return TRUE;
    }
    else if((event->state&(GDK_CONTROL_MASK|GDK_MOD1_MASK)) == 0 && 
            ((event->keyval >= GDK_space && event->keyval <= GDK_z)))
    {
        char data[2];
        data[0] = (char)gdk_keyval_to_unicode(event->keyval);
        data[1] = '\0';
        gtk_widget_grab_focus(entry);      
        gtk_entry_set_text(GTK_ENTRY(entry),data);
        gtk_editable_set_position(GTK_EDITABLE(entry),1);

        return TRUE;
    }
    return pl3_window_key_press_event(GTK_WIDGET(tree),event);
}

/* create a dialog that allows the user to save the current playlist */
static void pl3_current_playlist_save_playlist ()
{
    gchar *str;
    GladeXML *xml = NULL;
    int run = TRUE;
    /* check if the connection is up */
    if (!mpd_check_connected(connection))
    {
        return;
    }
    /* create the interface */
    str = gmpc_get_full_glade_path("playlist3.glade");
    xml = glade_xml_new (str, "save_pl", NULL);
    q_free(str);

    /* run the interface */
    do
    {
        switch (gtk_dialog_run (GTK_DIALOG (glade_xml_get_widget (xml, "save_pl"))))
        {
            case GTK_RESPONSE_OK:
                run = FALSE;
                /* if the users agrees do the following: */
                /* get the song-name */
                str = (gchar *)	gtk_entry_get_text (GTK_ENTRY
                        (glade_xml_get_widget (xml, "pl-entry")));
                /* check if the user entered a name, we can't do withouth */
                /* TODO: disable ok button when nothing is entered */
                /* also check if there is a connection */
                if (strlen (str) != 0 && mpd_check_connected(connection))
                {
                    int retv = mpd_database_save_playlist(connection, str);
                    if(retv == MPD_DATABASE_PLAYLIST_EXIST )
                    {
                        gchar *errormsg = g_strdup_printf(_("<i>Playlist <b>\"%s\"</b> already exists\nOverwrite?</i>"), str);
                        gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(xml, "label_error")), errormsg);
                        gtk_widget_show(glade_xml_get_widget(xml, "hbox5"));
                        /* ask to replace */
                        gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "pl-entry")), FALSE);
                        switch (gtk_dialog_run (GTK_DIALOG (glade_xml_get_widget (xml, "save_pl"))))
                        {
                            case GTK_RESPONSE_OK:
                                run = FALSE;
                                mpd_database_delete_playlist(connection, str);
                                mpd_database_save_playlist(connection,str);
                                GmpcStatusChangedCallback(connection, MPD_CST_DATABASE, NULL);
                                break;
                            default:
                                run = TRUE;
                        }
                        /* return to stare */
                        gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "pl-entry")), TRUE);
                        gtk_widget_hide(glade_xml_get_widget(xml, "hbox5"));

                        q_free(errormsg);
                    }
                    else if (retv != MPD_OK)
                    {
                        playlist3_show_error_message(_("Failed to save the playlist file."), ERROR_WARNING);
                    }
                    else 
                    {
                        GmpcStatusChangedCallback(connection, MPD_CST_DATABASE, NULL);
                    }
                }
                break;
            default:
                run = FALSE;
        }
    }while(run);
    /* destroy the window */
    gtk_widget_destroy (glade_xml_get_widget (xml, "save_pl"));

    /* unref the gui description */
    g_object_unref (xml);
}

static void pl3_current_playlist_browser_clear_playlist()
{
    mpd_playlist_clear(connection);
}

static void pl3_current_playlist_browser_shuffle_playlist()
{
    mpd_playlist_shuffle(connection);
}

static void pl3_current_playlist_status_changed(MpdObj *mi, ChangedStatusType what, void *userdata)
{
    if(pl3_cp_vbox == NULL)
        return;
    if(what&MPD_CST_PLAYLIST)
    {
        mod_fill_do_entry_changed(filter_entry, NULL);

    }
}

static void pl3_current_playlist_browser_activate()
{
    GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
            glade_xml_get_widget (pl3_xml, "cat_tree"));


    GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_curb_tree_ref); 
    if(path)
    {
        gtk_tree_selection_select_path(selec, path);
        gtk_tree_path_free(path);
    }
}


static int pl3_current_playlist_browser_add_go_menu(GtkWidget *menu)
{
    GtkWidget *item = NULL;

    item = gtk_image_menu_item_new_with_label(_("Play Queue"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
            gtk_image_new_from_icon_name("playlist-browser", GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_add_accelerator(GTK_WIDGET(item), "activate", gtk_menu_get_accel_group(GTK_MENU(menu)), GDK_F1, 0, GTK_ACCEL_VISIBLE);
    g_signal_connect(G_OBJECT(item), "activate", 
            G_CALLBACK(pl3_current_playlist_browser_activate), NULL);
    return 1;
}

/**
 * 
 */
static int pl3_current_playlist_key_press_event(GtkWidget *mw, GdkEventKey *event, int type)
{

    return FALSE;
}

static void pl3_current_playlist_connection_changed(MpdObj *mi, int connect,gpointer data)
{
    if(!connect && filter_entry)
    {
        gtk_entry_set_text(GTK_ENTRY(filter_entry), "");
    }
}
/* function that saves the settings */
static void pl3_current_playlist_save_myself(void)
{
    if(pl3_curb_tree_ref)
    {
        GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_curb_tree_ref);
        if(path)
        {
            gint *indices = gtk_tree_path_get_indices(path);
            debug_printf(DEBUG_INFO,"Saving myself to position: %i\n", indices[0]);
            cfg_set_single_value_as_int(config, "current-playlist","position",indices[0]);
            gtk_tree_path_free(path);
        }
    }
}

/****
 * TREEVIEW
 */
static GtkWidget *tooltip_window = NULL;
static GtkWidget *tw_image = NULL;

static GtkWidget *tw_title = NULL;
static GtkWidget *tw_artist = NULL;
static GtkWidget *tw_album = NULL;
static GtkWidget *tw_genre = NULL;
static GtkWidget *tw_date = NULL;

static gchar *file = NULL;

#define MOUSE_OFFSET 10

static gboolean self_query_tooltip(GtkWidget *treeview, 
        gint x, gint y, 
        gboolean keyboard_tip,
        GtkTooltip *tooltip, 
        gpointer data)
{
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
    GtkTreeIter iter;
    GtkTreePath *path = NULL;
    int show = FALSE;
    if (!gtk_tree_view_get_tooltip_context (GTK_TREE_VIEW(treeview), &x, &y,
                keyboard_tip,
                &model, &path, &iter))
        return FALSE;
    
    mpd_Song *song = NULL;
    gtk_tree_model_get(model, &iter,MPDDATA_MODEL_COL_MPDSONG, &song, -1);
    if(song)
    {
        show = TRUE;
        /* makes it look a bit nicer */
        if(file && song->file && strcmp(file, song->file))
        {
            show = FALSE;
            g_free(file);file = NULL; 
        }
        else if(!(file && song->file && strcmp(file, song->file) == 0))
        {
            gchar *temp;

            gmpc_metaimage_update_cover_from_song(GMPC_METAIMAGE(tw_image), song);
            if(file)
                g_free(file);
            file = NULL;
            if(song->file)
                file = g_strdup(song->file);

            gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_SONG_TITLE, &temp, -1);
            gtk_label_set_text(GTK_LABEL(tw_title), temp);
            g_free(temp);

            gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_SONG_ARTIST,&temp, -1);
            gtk_label_set_text(GTK_LABEL(tw_artist), temp);
            g_free(temp);

            gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_SONG_ALBUM, &temp, -1);
            gtk_label_set_text(GTK_LABEL(tw_album), temp);
            g_free(temp);

            gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_SONG_GENRE, &temp, -1);
            gtk_label_set_text(GTK_LABEL(tw_genre), temp);
            g_free(temp);

            gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_SONG_DATE, &temp, -1);
            gtk_label_set_text(GTK_LABEL(tw_date), temp);
            g_free(temp);
        }
    }
    gtk_tree_path_free(path);
    return show;
}
static void self_enable_tooltip(GtkWidget *treeview)
{
    GtkWidget *table = gtk_table_new(5,2, FALSE);
    GtkWidget *vbox = gtk_hbox_new(FALSE, 6);
    GtkWidget *label;
    GtkSizeGroup *group  = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
    tooltip_window = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_default_size(GTK_WINDOW(tooltip_window), 3*128,128);
    
    gtk_container_add(GTK_CONTAINER(tooltip_window), vbox);
    tw_image = gmpc_metaimage_new(META_ALBUM_ART);
    gmpc_metaimage_set_size(GMPC_METAIMAGE(tw_image), 128);
    gtk_box_pack_start(GTK_BOX(vbox), tw_image, FALSE, FALSE, 0);

    gtk_table_set_row_spacings(GTK_TABLE(table), 6);
    gtk_table_set_col_spacings(GTK_TABLE(table), 6);

    label = gtk_label_new("");
    gtk_size_group_add_widget(group, label);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Title:</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0,0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0,1,0,1,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
    tw_title = gtk_label_new("n/a");
    gtk_misc_set_alignment(GTK_MISC(tw_title), 0.0,0.5);
    gtk_label_set_ellipsize(GTK_LABEL(tw_title), PANGO_ELLIPSIZE_END);
    gtk_table_attach(GTK_TABLE(table), tw_title, 1,2,0,1,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);


    label = gtk_label_new("");
    gtk_size_group_add_widget(group, label);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Artist:</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0,0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0,1,1,2,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
    tw_artist = gtk_label_new("n/a");
    gtk_misc_set_alignment(GTK_MISC(tw_artist), 0.0,0.5);
    gtk_label_set_ellipsize(GTK_LABEL(tw_artist),PANGO_ELLIPSIZE_END);
    gtk_table_attach(GTK_TABLE(table), tw_artist, 1,2,1,2,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);



    label = gtk_label_new("");
    gtk_size_group_add_widget(group, label);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Album:</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0,0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0,1,2,3,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
    tw_album = gtk_label_new("n/a");
    gtk_misc_set_alignment(GTK_MISC(tw_album), 0.0,0.5);
    gtk_label_set_ellipsize(GTK_LABEL(tw_album),PANGO_ELLIPSIZE_END);
    gtk_table_attach(GTK_TABLE(table), tw_album, 1,2,2,3,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);

    label = gtk_label_new("");
    gtk_size_group_add_widget(group, label);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Genre:</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0,0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0,1,3,4,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
    tw_genre = gtk_label_new("n/a"); 
    gtk_misc_set_alignment(GTK_MISC(tw_genre), 0.0,0.5);
    gtk_label_set_ellipsize(GTK_LABEL(tw_genre),PANGO_ELLIPSIZE_END);
    gtk_table_attach(GTK_TABLE(table), tw_genre, 1,2,3,4,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);

    label = gtk_label_new("");
    gtk_size_group_add_widget(group, label);
    gtk_label_set_markup(GTK_LABEL(label), _("<b>Date:</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 1.0,0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0,1,4,5,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
    tw_date = gtk_label_new("n/a"); 
    gtk_misc_set_alignment(GTK_MISC(tw_date), 0.0,0.5);
    gtk_label_set_ellipsize(GTK_LABEL(tw_date),PANGO_ELLIPSIZE_END);
    gtk_table_attach(GTK_TABLE(table), tw_date, 1,2,4,5,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);





    gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);



    g_signal_connect(G_OBJECT(treeview), 
            "query-tooltip",
            G_CALLBACK(self_query_tooltip),
            NULL); 
    gtk_widget_set_tooltip_window(treeview, GTK_WINDOW(tooltip_window));
    gtk_widget_show_all(vbox);
}

