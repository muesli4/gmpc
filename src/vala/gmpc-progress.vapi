/* gmpc-progress.vapi generated by valac, do not modify. */

[CCode (cprefix = "Gmpc", lower_case_cprefix = "gmpc_")]
namespace Gmpc {
	[CCode (cheader_filename = "gmpc-progress.h")]
	public class Progress : Gtk.HBox {
		public bool _hide_text;
		public Progress ();
		public void set_time (uint total, uint current);
		public bool hide_text { get; set; }
	}
}