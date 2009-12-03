/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpc.wikia.com/
 
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

public Gmpc.PixbufCache pbc = null;
public class Gmpc.PixbufCacheData : GLib.Object{
    public weak Gdk.Pixbuf pb = null;
    public uint timeout = 0;
    public bool unused = false;
}
public class Gmpc.PixbufCache : GLib.Object
{
    public GLib.HashTable<string, Gmpc.PixbufCacheData> hash = new GLib.HashTable<string, Gmpc.PixbufCacheData>.full(string.hash, GLib.str_equal, g_free, GLib.Object.unref);



    public Gdk.Pixbuf? get_pixbuf(string uri, int size)
    {
        var key = "%i:%s".printf(size, uri); 
        var pb = hash.lookup(key);
        if(pb == null) return null;
        stdout.printf("Cache hit!\n");
        if(pb.timeout > 0) GLib.Source.remove(pb.timeout);
        pb.timeout = 0;
        pb.unused = false;
        return pb.pb;
    }
/*
    static bool unreffed_pixbuf_timeout(Gdk.Pixbuf pb)
    {
        string url = (string)pb.get_data("url");
        stdout.printf("removing: %s \n", (string)url);
        pbc.hash.remove((string)url);
        pb.remove_toggle_ref(unreffed_pixbuf);
        stdout.printf("Cache size: %u\n", pbc.hash.size());
    }*/
    static void unreffed_pixbuf(GLib.Object data, bool last_ref)
    {
        weak Gdk.Pixbuf pb = (Gdk.Pixbuf)data;
        stdout.printf("Pixbuf is now unused %i\n", (int)last_ref);
        if(last_ref)
        {
            string url = (string)pb.get_data("url");
            stdout.printf("marking: %s\n", url);
            var a = pbc.hash.lookup(url);
            if(a != null)
            {
                a.unused =true;
                pbc.set_cleanup_timeout();
            }
        }
    }

    private uint timeout = 0;
    public void set_cleanup_timeout()
    {
        if(this.timeout > 0 ) {
            GLib.Source.remove(this.timeout);
        }
        this.timeout = GLib.Timeout.add_seconds(15, remove_old_items);
        stdout.printf("set timeout\n");
    }
    private bool remove_old_items()
    {
        stdout.printf("Remove old items\n");
        List<string> items = null;
        HashTableIter<string, Gmpc.PixbufCacheData> iter = HashTableIter<string, Gmpc.PixbufCacheData>(this.hash);
        {
            string url = null;
            Gmpc.PixbufCacheData data;
            while(iter.next(out url, out data)){
                if(data.unused) {
                    data.pb.remove_toggle_ref(unreffed_pixbuf);
                    items.prepend(url);
                }
            }
        }
        foreach(string url in items){
            stdout.printf("removing:%s\n", url);
            hash.remove(url);
            stdout.printf("Cache size: %u\n", hash.size());
        }

        this.timeout = 0;
        return false;
    }
    public void append(string uri, Gdk.Pixbuf buf, int size)
    {
        string url = "%i:%s".printf(size, uri); 
        buf.set_data_full("url", (void *)url.dup(), g_free); 
        buf.add_toggle_ref(unreffed_pixbuf);
        var a = new Gmpc.PixbufCacheData();
        a.pb = buf;
        hash.insert(url, a);
        stdout.printf("Cache size: %u\n", hash.size());
    }

}

public class Gmpc.PixbufLoaderAsync : GLib.Object
{
    private weak GLib.Cancellable? pcancel = null; 
    public string uri = null;
    public Gdk.Pixbuf pixbuf {set;get;default=null;}
    private Gtk.TreeRowReference rref = null;


    signal void pixbuf_update(Gdk.Pixbuf? pixbuf);

    public void set_rref(Gtk.TreeRowReference rreference)
    {
        this.rref = rreference;
    }

    private void call_row_changed()
    {
        if(rref != null) {
            var model = rref.get_model();
            var path = rref.get_path();
            Gtk.TreeIter iter;
            if(model.get_iter(out iter, path))
            {
                model.row_changed(path, iter);
            }
        }
    }

    construct {
    if(pbc == null) pbc = new Gmpc.PixbufCache();
	stdout.printf("Create the image loading\n" );
    }

    ~PixbufLoaderAsync() {
        warning("Free the image loading");
        if(this.pcancel != null) pcancel.cancel();
    }

    private Gdk.Pixbuf? modify_pixbuf(Gdk.Pixbuf? pix, int size,bool casing) 
    {
        if(pix == null) return null;
        if(casing)
        {
            int width = pix.width;
            int height = pix.height;
            double spineRatio = 5.0/65.0;

            var ii = Gtk.IconTheme.get_default().lookup_icon("stylized-cover", size, 0);
            if(ii != null) {
                var path = ii.get_filename();
                try {
                    var case_image = new Gdk.Pixbuf.from_file_at_scale(path, size, size, true);

                    var tempw = (int)(case_image.width*(1.0-spineRatio));
                    Gdk.Pixbuf pix2;
                    if((case_image.height/(double)height)*width < tempw) {
                        pix2 = pix.scale_simple(tempw, (int)((height*tempw)/width), Gdk.InterpType.BILINEAR);
                    }else{
                        pix2 = pix.scale_simple((int)(width*(case_image.height/(double)height)), case_image.height, Gdk.InterpType.BILINEAR);
                    }
                    var blank = new Gdk.Pixbuf(Gdk.Colorspace.RGB, true, 8, case_image.width, case_image.height);
                    blank.fill(0x000000FF);
                    tempw = (tempw >= pix2.width)? pix2.width:tempw;
                    var temph = (case_image.height > pix2.height)?pix2.height:case_image.height;
                    pix2.copy_area(0,0, tempw-1, temph-2, blank, case_image.width-tempw, 1);
                    case_image.composite(blank, 0,0,case_image.width, case_image.height, 0,0,1,1,Gdk.InterpType.BILINEAR, 250);
                    return blank;
                }catch (Error e) {

                }

            }
        }
        
        Gmpc.Fix.add_border(pix);
        return pix;
    }


    public new void set_from_file(string uri, int size, bool border)
    {
        if(this.pcancel != null)
            this.pcancel.cancel();
        this.pcancel = null;
        this.uri = uri;

        Gdk.Pixbuf pb = pbc.get_pixbuf(this.uri, size);
        if(pb != null) {
            pixbuf = pb;
            pixbuf_update(pixbuf);
            call_row_changed();
        }else{
            GLib.Cancellable cancel= new GLib.Cancellable();
            this.pcancel = cancel;
            this.load_from_file_async(uri, size, cancel, border);
        }
    }

    private async void load_from_file_async(string uri, int size, GLib.Cancellable cancel, bool border)
    {
        GLib.File file = GLib.File.new_for_path(uri);
        size_t result = 0;
        Gdk.PixbufLoader loader = new Gdk.PixbufLoader();
        loader.size_prepared.connect((source, width, height) => {
            double dsize = (double)size;
            int nwidth = (height > width)? (int)((dsize/height)*width): size;
            int nheight= (width > height )? (int)((dsize/width)*height): size;
            loader.set_size(nwidth, nheight);

        });
        loader.area_prepared.connect((source) => {
                var apix = loader.get_pixbuf();
                var afinal = this.modify_pixbuf(apix, size,border);
                
                pixbuf = afinal;
                pixbuf_update(pixbuf);
                call_row_changed();
                });
        try{
            var stream = yield file.read_async(0, cancel);
            if(!cancel.is_cancelled() && stream != null )
            {
                do{
                    try {
                          uchar data[1024]; 
                          result = yield stream.read_async(data,1024, 0, cancel);
                          Gmpc.Fix.write_loader(loader,(string)data, result);
                      }catch ( Error erro) {
                          warning("Error trying to fetch image: %s", erro.message);
                      }
                  }while(!cancel.is_cancelled() && result > 0);
            }      
        }catch ( Error e) {
            warning("Error trying to fetch image: %s", e.message);
        }
        try {
            loader.close();
        }catch (Error err) {
            warning("Error trying to parse image: %s", err.message);
        }

        if(cancel.is_cancelled())
        {
            warning("Cancelled loading of image");
            cancel.reset();
            return;
        }

        Gdk.Pixbuf pix = loader.get_pixbuf();
        var final = this.modify_pixbuf(pix, size,border);
        pixbuf = final;
        pbc.append(this.uri, this.pixbuf, size);
        pixbuf_update(pixbuf);
        call_row_changed();
        this.pcancel = null;
        loader = null;
    }
}

public class Gmpc.MetaImageAsync : Gtk.Image
{
    private Gmpc.PixbufLoaderAsync? loader = null;
    public string uri = null;

    construct {
    }

    ~MetaImageAsync() {
	stdout.printf("Freeing metaimageasync\n");
    }

    public new void set_from_file(string uri, int size, bool border)
    {
        loader = new PixbufLoaderAsync(); 
        loader.pixbuf_update.connect((source, pixbuf)=>{
                this.set_from_pixbuf(pixbuf);
                });
        loader.set_from_file(uri, size, border);
    }
    public void clear_now()
    {
        this.loader = null;
        this.uri = null;
        this.clear();
    }

    public void set_pixbuf(Gdk.Pixbuf pb)
    {
        this.loader = null;
        this.uri = null;
        this.set_from_pixbuf(pb);
    }
}
