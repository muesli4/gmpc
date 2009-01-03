/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpcwiki.sarine.nl/
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "gmpc-progress.h"
#include <pango/pango.h>
#include <cairo.h>
#include <float.h>
#include <math.h>
#include <gdk/gdk.h>
#include <stdlib.h>
#include <string.h>
#include <pango/pangocairo.h>




struct _GmpcProgressPrivate {
	guint total;
	guint current;
	gboolean _do_countdown;
	PangoLayout* _layout;
};

#define GMPC_PROGRESS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_TYPE_PROGRESS, GmpcProgressPrivate))
enum  {
	GMPC_PROGRESS_DUMMY_PROPERTY,
	GMPC_PROGRESS_HIDE_TEXT,
	GMPC_PROGRESS_DO_COUNTDOWN
};
static void gmpc_progress_real_size_request (GtkWidget* base, GtkRequisition* requisition);
static void gmpc_progress_draw_curved_rectangle (GmpcProgress* self, cairo_t* ctx, double rect_x0, double rect_y0, double rect_width, double rect_height);
static void gmpc_progress_redraw (GmpcProgress* self);
static gboolean gmpc_progress_on_expose2 (GmpcProgress* self, GmpcProgress* pb, GdkEventExpose* event);
static gboolean _gmpc_progress_on_expose2_gtk_widget_expose_event (GmpcProgress* _sender, GdkEventExpose* event, gpointer self);
static GObject * gmpc_progress_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_progress_parent_class = NULL;
static void gmpc_progress_finalize (GObject* obj);



/* The size_request method Gtk+ is calling on a widget to ask
 it the widget how large it wishes to be. It's not guaranteed
 that gtk+ will actually give this size to the widget*/
static void gmpc_progress_real_size_request (GtkWidget* base, GtkRequisition* requisition) {
	GmpcProgress * self;
	gint width;
	gint height;
	self = (GmpcProgress*) base;
	width = 0;
	height = 0;
	/* In this case, we say that we want to be as big as the
	 text is, plus a little border around it.*/
	if (gmpc_progress_get_hide_text (self)) {
		(*requisition).width = 40;
		(*requisition).height = 10;
	} else {
		pango_layout_get_size (self->priv->_layout, &width, &height);
		(*requisition).width = (width / PANGO_SCALE) + 6;
		(*requisition).height = (height / PANGO_SCALE) + 6;
	}
}


static void gmpc_progress_draw_curved_rectangle (GmpcProgress* self, cairo_t* ctx, double rect_x0, double rect_y0, double rect_width, double rect_height) {
	double rect_x1;
	double rect_y1;
	double radius;
	gboolean _tmp0;
	g_return_if_fail (self != NULL);
	g_return_if_fail (ctx != NULL);
	rect_x1 = 0.0;
	rect_y1 = 0.0;
	radius = (double) 10;
	rect_x1 = rect_x0 + rect_width;
	rect_y1 = rect_y0 + rect_height;
	_tmp0 = FALSE;
	if (rect_width == 0) {
		_tmp0 = TRUE;
	} else {
		_tmp0 = rect_height == 0;
	}
	if (_tmp0) {
		return;
	}
	if ((rect_width / 2) < radius) {
		if ((rect_height / 2) < radius) {
			cairo_move_to (ctx, rect_x0, (rect_y0 + rect_y1) / 2);
			cairo_curve_to (ctx, rect_x0, rect_y0, rect_x0, rect_y0, (rect_x0 + rect_x1) / 2, rect_y0);
			cairo_curve_to (ctx, rect_x1, rect_y0, rect_x1, rect_y0, rect_x1, (rect_y0 + rect_y1) / 2);
			cairo_curve_to (ctx, rect_x1, rect_y1, rect_x1, rect_y1, (rect_x1 + rect_x0) / 2, rect_y1);
			cairo_curve_to (ctx, rect_x0, rect_y1, rect_x0, rect_y1, rect_x0, (rect_y0 + rect_y1) / 2);
		} else {
			cairo_move_to (ctx, rect_x0, rect_y0 + radius);
			cairo_curve_to (ctx, rect_x0, rect_y0, rect_x0, rect_y0, (rect_x0 + rect_x1) / 2, rect_y0);
			cairo_curve_to (ctx, rect_x1, rect_y0, rect_x1, rect_y0, rect_x1, rect_y0 + radius);
			cairo_line_to (ctx, rect_x1, rect_y1 - radius);
			cairo_curve_to (ctx, rect_x1, rect_y1, rect_x1, rect_y1, (rect_x1 + rect_x0) / 2, rect_y1);
			cairo_curve_to (ctx, rect_x0, rect_y1, rect_x0, rect_y1, rect_x0, rect_y1 - radius);
		}
	} else {
		if ((rect_height / 2) < radius) {
			cairo_move_to (ctx, rect_x0, (rect_y0 + rect_y1) / 2);
			cairo_curve_to (ctx, rect_x0, rect_y0, rect_x0, rect_y0, rect_x0 + radius, rect_y0);
			cairo_line_to (ctx, rect_x1 - radius, rect_y0);
			cairo_curve_to (ctx, rect_x1, rect_y0, rect_x1, rect_y0, rect_x1, (rect_y0 + rect_y1) / 2);
			cairo_curve_to (ctx, rect_x1, rect_y1, rect_x1, rect_y1, rect_x1 - radius, rect_y1);
			cairo_line_to (ctx, rect_x0 + radius, rect_y1);
			cairo_curve_to (ctx, rect_x0, rect_y1, rect_x0, rect_y1, rect_x0, (rect_y0 + rect_y1) / 2);
		} else {
			cairo_move_to (ctx, rect_x0, rect_y0 + radius);
			cairo_curve_to (ctx, rect_x0, rect_y0, rect_x0, rect_y0, rect_x0 + radius, rect_y0);
			cairo_line_to (ctx, rect_x1 - radius, rect_y0);
			cairo_curve_to (ctx, rect_x1, rect_y0, rect_x1, rect_y0, rect_x1, rect_y0 + radius);
			cairo_line_to (ctx, rect_x1, rect_y1 - radius);
			cairo_curve_to (ctx, rect_x1, rect_y1, rect_x1, rect_y1, rect_x1 - radius, rect_y1);
			cairo_line_to (ctx, rect_x0 + radius, rect_y1);
			cairo_curve_to (ctx, rect_x0, rect_y1, rect_x0, rect_y1, rect_x0, rect_y1 - radius);
		}
	}
	cairo_close_path (ctx);
}


static void gmpc_progress_redraw (GmpcProgress* self) {
	g_return_if_fail (self != NULL);
	if (((GtkWidget*) self)->window != NULL) {
		gdk_window_process_updates (((GtkWidget*) self)->window, FALSE);
	}
}


static gboolean gmpc_progress_on_expose2 (GmpcProgress* self, GmpcProgress* pb, GdkEventExpose* event) {
	cairo_t* ctx;
	gint width;
	gint height;
	gint pw;
	gint pwidth;
	GdkColor _tmp0 = {0};
	GdkColor _tmp1 = {0};
	cairo_pattern_t* pattern;
	GdkColor start;
	GdkColor stop;
	gboolean _tmp18;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (pb != NULL, FALSE);
	ctx = gdk_cairo_create ((GdkDrawable*) ((GtkWidget*) self)->window);
	width = ((GtkWidget*) self)->allocation.width - 3;
	height = ((GtkWidget*) self)->allocation.height - 3;
	pw = width - 3;
	pwidth = (gint) ((self->priv->current * pw) / ((double) self->priv->total));
	if (pwidth > pw) {
		pwidth = pw;
	}
	/* Draw border */
	cairo_set_line_width (ctx, 1.0);
	cairo_set_tolerance (ctx, 0.2);
	cairo_set_line_join (ctx, CAIRO_LINE_JOIN_ROUND);
	/*paint background*/
	gdk_cairo_set_source_color (ctx, (_tmp0 = gtk_widget_get_style ((GtkWidget*) pb)->bg[(gint) GTK_STATE_NORMAL], &_tmp0));
	cairo_paint (ctx);
	cairo_new_path (ctx);
	/* Stroke a white line, and clip on that */
	gdk_cairo_set_source_color (ctx, (_tmp1 = gtk_widget_get_style ((GtkWidget*) pb)->dark[(gint) GTK_STATE_NORMAL], &_tmp1));
	gmpc_progress_draw_curved_rectangle (self, ctx, 1.5, 1.5, (double) width, (double) height);
	cairo_stroke_preserve (ctx);
	/* Make a clip */
	cairo_clip (ctx);
	if (self->priv->total > 0) {
		GdkColor _tmp2 = {0};
		/* don't allow more then 100% */
		if (pwidth > width) {
			pwidth = width;
		}
		cairo_new_path (ctx);
		gdk_cairo_set_source_color (ctx, (_tmp2 = gtk_widget_get_style ((GtkWidget*) pb)->bg[(gint) GTK_STATE_SELECTED], &_tmp2));
		gmpc_progress_draw_curved_rectangle (self, ctx, 1.5 + 2, 1.5 + 2, (double) pwidth, (double) (height - 4));
		cairo_fill (ctx);
	}
	/* Paint nice reflection layer on top */
	cairo_new_path (ctx);
	pattern = cairo_pattern_create_linear (0.0, 0.0, 0.0, (double) height);
	start = gtk_widget_get_style ((GtkWidget*) pb)->dark[(gint) GTK_STATE_NORMAL];
	stop = gtk_widget_get_style ((GtkWidget*) pb)->light[(gint) GTK_STATE_NORMAL];
	cairo_pattern_add_color_stop_rgba (pattern, 0.0, start.red / (65536.0), start.green / (65536.0), start.blue / (65536.0), 0.6);
	cairo_pattern_add_color_stop_rgba (pattern, 0.40, stop.red / (65536.0), stop.green / (65536.0), stop.blue / (65536.0), 0.2);
	cairo_pattern_add_color_stop_rgba (pattern, 0.551, stop.red / (65536.0), stop.green / (65536.0), stop.blue / (65536.0), 0.0);
	cairo_set_source (ctx, pattern);
	cairo_rectangle (ctx, 1.5, 1.5, (double) width, (double) height);
	cairo_fill (ctx);
	cairo_reset_clip (ctx);
	/**
	         * Draw text
	         */
	if (gmpc_progress_get_hide_text (self) == FALSE) {
		gint fontw;
		gint fonth;
		gint e_hour;
		gint e_minutes;
		gint e_seconds;
		gint t_hour;
		gint t_minutes;
		gint t_seconds;
		char* a;
		guint p;
		char* _tmp8;
		char* _tmp7;
		fontw = 0;
		fonth = 0;
		e_hour = 0;
		e_minutes = 0;
		e_seconds = 0;
		t_hour = ((gint) self->priv->total) / 3600;
		t_minutes = (((gint) self->priv->total) % 3600) / 60;
		t_seconds = ((gint) self->priv->total) % 60;
		a = g_strdup ("");
		p = self->priv->current;
		if (gmpc_progress_get_do_countdown (self)) {
			char* _tmp3;
			p = self->priv->total - self->priv->current;
			_tmp3 = NULL;
			a = (_tmp3 = g_strconcat (a, ("-"), NULL), a = (g_free (a), NULL), _tmp3);
		}
		e_hour = ((gint) p) / 3600;
		e_minutes = ((gint) (p % 3600)) / 60;
		e_seconds = (gint) (p % 60);
		if (e_hour > 0) {
			char* _tmp5;
			char* _tmp4;
			_tmp5 = NULL;
			_tmp4 = NULL;
			a = (_tmp5 = g_strconcat (a, _tmp4 = (g_strdup_printf ("%02i", e_hour)), NULL), a = (g_free (a), NULL), _tmp5);
			_tmp4 = (g_free (_tmp4), NULL);
			if (e_minutes > 0) {
				char* _tmp6;
				_tmp6 = NULL;
				a = (_tmp6 = g_strconcat (a, (":"), NULL), a = (g_free (a), NULL), _tmp6);
			}
		}
		_tmp8 = NULL;
		_tmp7 = NULL;
		a = (_tmp8 = g_strconcat (a, _tmp7 = (g_strdup_printf ("%02i:%02i", e_minutes, e_seconds)), NULL), a = (g_free (a), NULL), _tmp8);
		_tmp7 = (g_free (_tmp7), NULL);
		if (self->priv->total > 0) {
			char* _tmp9;
			char* _tmp14;
			char* _tmp13;
			_tmp9 = NULL;
			a = (_tmp9 = g_strconcat (a, (" -  "), NULL), a = (g_free (a), NULL), _tmp9);
			if (t_hour > 0) {
				char* _tmp11;
				char* _tmp10;
				_tmp11 = NULL;
				_tmp10 = NULL;
				a = (_tmp11 = g_strconcat (a, _tmp10 = (g_strdup_printf ("%02i", t_hour)), NULL), a = (g_free (a), NULL), _tmp11);
				_tmp10 = (g_free (_tmp10), NULL);
				if (t_minutes > 0) {
					char* _tmp12;
					_tmp12 = NULL;
					a = (_tmp12 = g_strconcat (a, (":"), NULL), a = (g_free (a), NULL), _tmp12);
				}
			}
			_tmp14 = NULL;
			_tmp13 = NULL;
			a = (_tmp14 = g_strconcat (a, _tmp13 = (g_strdup_printf ("%02i:%02i", t_minutes, t_seconds)), NULL), a = (g_free (a), NULL), _tmp14);
			_tmp13 = (g_free (_tmp13), NULL);
		}
		pango_layout_set_text (self->priv->_layout, a, -1);
		pango_cairo_update_layout (ctx, self->priv->_layout);
		pango_layout_get_pixel_size (self->priv->_layout, &fontw, &fonth);
		if (self->priv->total > 0) {
			if (pwidth >= (((width - fontw) / 2) + 1)) {
				GdkColor _tmp15 = {0};
				cairo_new_path (ctx);
				gdk_cairo_set_source_color (ctx, (_tmp15 = gtk_widget_get_style ((GtkWidget*) pb)->fg[(gint) GTK_STATE_SELECTED], &_tmp15));
				cairo_rectangle (ctx, 3.5, 1.5, (double) pwidth, (double) height);
				cairo_clip (ctx);
				cairo_move_to (ctx, ((width - fontw) / 2) + 1.5, ((height - fonth) / 2) + 1.5);
				pango_cairo_show_layout (ctx, self->priv->_layout);
				cairo_reset_clip (ctx);
			}
			if (pwidth < ((((width - fontw) / 2) + 1) + fontw)) {
				GdkColor _tmp16 = {0};
				cairo_new_path (ctx);
				gdk_cairo_set_source_color (ctx, (_tmp16 = gtk_widget_get_style ((GtkWidget*) pb)->fg[(gint) GTK_STATE_NORMAL], &_tmp16));
				cairo_rectangle (ctx, pwidth + 3.5, 1.5, (double) width, (double) height);
				cairo_clip (ctx);
				cairo_move_to (ctx, ((width - fontw) / 2) + 1.5, ((height - fonth) / 2) + 1.5);
				pango_cairo_show_layout (ctx, self->priv->_layout);
			}
		} else {
			GdkColor _tmp17 = {0};
			cairo_new_path (ctx);
			gdk_cairo_set_source_color (ctx, (_tmp17 = gtk_widget_get_style ((GtkWidget*) pb)->fg[(gint) GTK_STATE_NORMAL], &_tmp17));
			cairo_move_to (ctx, ((width - fontw) / 2) + 1.5, ((height - fonth) / 2) + 1.5);
			pango_cairo_show_layout (ctx, self->priv->_layout);
		}
		a = (g_free (a), NULL);
	}
	return (_tmp18 = TRUE, (ctx == NULL) ? NULL : (ctx = (cairo_destroy (ctx), NULL)), (pattern == NULL) ? NULL : (pattern = (cairo_pattern_destroy (pattern), NULL)), _tmp18);
}


void gmpc_progress_set_time (GmpcProgress* self, guint total, guint current) {
	gboolean _tmp0;
	g_return_if_fail (self != NULL);
	_tmp0 = FALSE;
	if (self->priv->total != total) {
		_tmp0 = TRUE;
	} else {
		_tmp0 = self->priv->current != current;
	}
	if (_tmp0) {
		self->priv->total = total;
		self->priv->current = current;
		gtk_widget_queue_draw ((GtkWidget*) self);
	}
}


GmpcProgress* gmpc_progress_construct (GType object_type) {
	GmpcProgress * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GmpcProgress* gmpc_progress_new (void) {
	return gmpc_progress_construct (GMPC_TYPE_PROGRESS);
}


gboolean gmpc_progress_get_hide_text (GmpcProgress* self) {
	g_return_val_if_fail (self != NULL, FALSE);
	return self->_hide_text;
}


void gmpc_progress_set_hide_text (GmpcProgress* self, gboolean value) {
	g_return_if_fail (self != NULL);
	self->_hide_text = value;
	gtk_widget_queue_resize ((GtkWidget*) self);
	g_object_notify ((GObject *) self, "hide-text");
}


gboolean gmpc_progress_get_do_countdown (GmpcProgress* self) {
	g_return_val_if_fail (self != NULL, FALSE);
	return self->priv->_do_countdown;
}


void gmpc_progress_set_do_countdown (GmpcProgress* self, gboolean value) {
	g_return_if_fail (self != NULL);
	self->priv->_do_countdown = value;
	gmpc_progress_redraw (self);
	g_object_notify ((GObject *) self, "do-countdown");
}


static gboolean _gmpc_progress_on_expose2_gtk_widget_expose_event (GmpcProgress* _sender, GdkEventExpose* event, gpointer self) {
	return gmpc_progress_on_expose2 (self, _sender, event);
}


/* Construct function */
static GObject * gmpc_progress_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GmpcProgressClass * klass;
	GObjectClass * parent_class;
	GmpcProgress * self;
	klass = GMPC_PROGRESS_CLASS (g_type_class_peek (GMPC_TYPE_PROGRESS));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = GMPC_PROGRESS (obj);
	{
		PangoLayout* _tmp1;
		PangoLayout* _tmp0;
		g_object_set ((GtkWidget*) self, "app-paintable", TRUE, NULL);
		g_signal_connect_object ((GtkWidget*) self, "expose-event", (GCallback) _gmpc_progress_on_expose2_gtk_widget_expose_event, self, 0);
		/* Set a string so we can get height */
		_tmp1 = NULL;
		_tmp0 = NULL;
		self->priv->_layout = (_tmp1 = (_tmp0 = gtk_widget_create_pango_layout ((GtkWidget*) self, " "), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0)), (self->priv->_layout == NULL) ? NULL : (self->priv->_layout = (g_object_unref (self->priv->_layout), NULL)), _tmp1);
	}
	return obj;
}


static void gmpc_progress_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec) {
	GmpcProgress * self;
	self = GMPC_PROGRESS (object);
	switch (property_id) {
		case GMPC_PROGRESS_HIDE_TEXT:
		g_value_set_boolean (value, gmpc_progress_get_hide_text (self));
		break;
		case GMPC_PROGRESS_DO_COUNTDOWN:
		g_value_set_boolean (value, gmpc_progress_get_do_countdown (self));
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void gmpc_progress_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec) {
	GmpcProgress * self;
	self = GMPC_PROGRESS (object);
	switch (property_id) {
		case GMPC_PROGRESS_HIDE_TEXT:
		gmpc_progress_set_hide_text (self, g_value_get_boolean (value));
		break;
		case GMPC_PROGRESS_DO_COUNTDOWN:
		gmpc_progress_set_do_countdown (self, g_value_get_boolean (value));
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void gmpc_progress_class_init (GmpcProgressClass * klass) {
	gmpc_progress_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcProgressPrivate));
	G_OBJECT_CLASS (klass)->get_property = gmpc_progress_get_property;
	G_OBJECT_CLASS (klass)->set_property = gmpc_progress_set_property;
	G_OBJECT_CLASS (klass)->constructor = gmpc_progress_constructor;
	G_OBJECT_CLASS (klass)->finalize = gmpc_progress_finalize;
	GTK_WIDGET_CLASS (klass)->size_request = gmpc_progress_real_size_request;
	g_object_class_install_property (G_OBJECT_CLASS (klass), GMPC_PROGRESS_HIDE_TEXT, g_param_spec_boolean ("hide-text", "hide-text", "hide-text", FALSE, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), GMPC_PROGRESS_DO_COUNTDOWN, g_param_spec_boolean ("do-countdown", "do-countdown", "do-countdown", FALSE, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
}


static void gmpc_progress_instance_init (GmpcProgress * self) {
	self->priv = GMPC_PROGRESS_GET_PRIVATE (self);
	self->priv->total = (guint) 0;
	self->priv->current = (guint) 0;
	self->priv->_do_countdown = FALSE;
	self->priv->_layout = NULL;
	self->_hide_text = FALSE;
}


static void gmpc_progress_finalize (GObject* obj) {
	GmpcProgress * self;
	self = GMPC_PROGRESS (obj);
	{
		g_object_unref ((GObject*) self->priv->_layout);
	}
	(self->priv->_layout == NULL) ? NULL : (self->priv->_layout = (g_object_unref (self->priv->_layout), NULL));
	G_OBJECT_CLASS (gmpc_progress_parent_class)->finalize (obj);
}


GType gmpc_progress_get_type (void) {
	static GType gmpc_progress_type_id = 0;
	if (gmpc_progress_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcProgressClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_progress_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcProgress), 0, (GInstanceInitFunc) gmpc_progress_instance_init, NULL };
		gmpc_progress_type_id = g_type_register_static (GTK_TYPE_EVENT_BOX, "GmpcProgress", &g_define_type_info, 0);
	}
	return gmpc_progress_type_id;
}




