
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

using GLib;
using Gtk;
using Gdk;
using Cairo;


public class Gmpc.Rating : Gtk.Frame
{
    private Gtk.Image[] rat;
    private Gtk.HBox    box;
    private Gtk.EventBox event;
    
    private bool button_press_event(Gtk.EventBox wid, Gdk.EventButton event)
    {
        if(event.type == Gdk.EventType.BUTTON_PRESS)
        {
            stdout.printf("button press\n");
            if(event.button == 1)
            {
                int width = this.allocation.width;
                int button = (int)((((event.x)/(double)width)+0.15)*5);
                this.set_rating(button);
                this.rating_changed(button);
                stdout.printf("set rating: %i\n", button);
            }
        }

        return false;
    }

    construct {
    
        this.box = new Gtk.HBox(true,6);
        this.event = new Gtk.EventBox();
        this.event.visible_window = false;
        this.rat = new Gtk.Image[5];
        this.rat[0] = new Gtk.Image.from_stock(Gtk.STOCK_ABOUT, Gtk.IconSize.MENU);
        this.box.pack_start(this.rat[0], false, false, 0);
        this.rat[1] = new Gtk.Image.from_stock(Gtk.STOCK_ABOUT, Gtk.IconSize.MENU);
        this.box.pack_start(this.rat[1], false, false, 0);
        this.rat[2] = new Gtk.Image.from_stock(Gtk.STOCK_ABOUT, Gtk.IconSize.MENU);
        this.box.pack_start(this.rat[2], false, false, 0);
        this.rat[3] = new Gtk.Image.from_stock(Gtk.STOCK_ABOUT, Gtk.IconSize.MENU);
        this.box.pack_start(this.rat[3], false, false, 0);
        this.rat[4] = new Gtk.Image.from_stock(Gtk.STOCK_ABOUT, Gtk.IconSize.MENU);
        this.box.pack_start(this.rat[4], false, false, 0);

        this.add(this.event);
        this.event.add(this.box);

//        this.add_events((int)Gdk.EventMask.BUTTON_PRESS_MASK);
        this.event.button_press_event += button_press_event;
        //GLib.Signal.connect_swapped(this.event, "button-press-event", (GLib.Callback)this.button_press_event, this);
       // this.button_press_event += this.button_press_event;
        this.show_all();
    }

    signal void rating_changed(int rating);

    public void set_rating(int rating)
    {
        int i=0;
        for(i=0;i<5;i++)
        {
            this.rat[i].sensitive = i<rating;
        }
    }



}
