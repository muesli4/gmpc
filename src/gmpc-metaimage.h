/* Generated by GOB (v2.0.14)   (do not edit directly) */

#include <glib.h>
#include <glib-object.h>


#include <gtk/gtk.h>
#include <libmpd/libmpd.h>
#include "main.h"

#ifndef __GMPC_METAIMAGE_H__
#define __GMPC_METAIMAGE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*
 * Type checking and casting macros
 */
#define GMPC_TYPE_METAIMAGE	(gmpc_metaimage_get_type())
#define GMPC_METAIMAGE(obj)	G_TYPE_CHECK_INSTANCE_CAST((obj), gmpc_metaimage_get_type(), GmpcMetaImage)
#define GMPC_METAIMAGE_CONST(obj)	G_TYPE_CHECK_INSTANCE_CAST((obj), gmpc_metaimage_get_type(), GmpcMetaImage const)
#define GMPC_METAIMAGE_CLASS(klass)	G_TYPE_CHECK_CLASS_CAST((klass), gmpc_metaimage_get_type(), GmpcMetaImageClass)
#define GMPC_IS_METAIMAGE(obj)	G_TYPE_CHECK_INSTANCE_TYPE((obj), gmpc_metaimage_get_type ())

#define GMPC_METAIMAGE_GET_CLASS(obj)	G_TYPE_INSTANCE_GET_CLASS((obj), gmpc_metaimage_get_type(), GmpcMetaImageClass)

/* Private structure type */
typedef struct _GmpcMetaImagePrivate GmpcMetaImagePrivate;

/*
 * Main object structure
 */
#ifndef __TYPEDEF_GMPC_METAIMAGE__
#define __TYPEDEF_GMPC_METAIMAGE__
typedef struct _GmpcMetaImage GmpcMetaImage;
#endif
struct _GmpcMetaImage {
	GtkEventBox __parent__;
	/*< public >*/
	int size;
	int image_type;
	gboolean hide_on_na;
	/*< private >*/
	GmpcMetaImagePrivate *_priv;
};

/*
 * Class definition
 */
typedef struct _GmpcMetaImageClass GmpcMetaImageClass;
struct _GmpcMetaImageClass {
	GtkEventBoxClass __parent__;
};


/*
 * Public methods
 */
GType	gmpc_metaimage_get_type	(void);
gint 	gmpc_metaimage_get_image_type	(GmpcMetaImage * self);
void 	gmpc_metaimage_set_image_type	(GmpcMetaImage * self,
					gint val);
gint 	gmpc_metaimage_get_size	(GmpcMetaImage * self);
void 	gmpc_metaimage_set_size	(GmpcMetaImage * self,
					gint val);
gboolean 	gmpc_metaimage_get_hide_on_na	(GmpcMetaImage * self);
void 	gmpc_metaimage_set_hide_on_na	(GmpcMetaImage * self,
					gboolean val);
GtkWidget * 	gmpc_metaimage_new	(int type);
void 	gmpc_metaimage_update_cover	(GmpcMetaImage * self,
					MpdObj * mi,
					ChangedStatusType what,
					GmpcConnection * gmpcconn);
void 	gmpc_metaimage_set_cover_na	(GmpcMetaImage * self);
void 	gmpc_metaimage_set_cover_fetching	(GmpcMetaImage * self);
void 	gmpc_metaimage_set_cover_from_path	(GmpcMetaImage * self,
					gchar * path);

/*
 * Argument wrapping macros
 */
#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
#define GMPC_METAIMAGE_PROP_IMAGE_TYPE(arg)    	"image_type", __extension__ ({gint z = (arg); z;})
#define GMPC_METAIMAGE_GET_PROP_IMAGE_TYPE(arg)	"image_type", __extension__ ({gint *z = (arg); z;})
#define GMPC_METAIMAGE_PROP_SIZE(arg)    	"size", __extension__ ({gint z = (arg); z;})
#define GMPC_METAIMAGE_GET_PROP_SIZE(arg)	"size", __extension__ ({gint *z = (arg); z;})
#define GMPC_METAIMAGE_PROP_HIDE_ON_NA(arg)    	"hide_on_na", __extension__ ({gboolean z = (arg); z;})
#define GMPC_METAIMAGE_GET_PROP_HIDE_ON_NA(arg)	"hide_on_na", __extension__ ({gboolean *z = (arg); z;})
#else /* __GNUC__ && !__STRICT_ANSI__ */
#define GMPC_METAIMAGE_PROP_IMAGE_TYPE(arg)    	"image_type",(gint )(arg)
#define GMPC_METAIMAGE_GET_PROP_IMAGE_TYPE(arg)	"image_type",(gint *)(arg)
#define GMPC_METAIMAGE_PROP_SIZE(arg)    	"size",(gint )(arg)
#define GMPC_METAIMAGE_GET_PROP_SIZE(arg)	"size",(gint *)(arg)
#define GMPC_METAIMAGE_PROP_HIDE_ON_NA(arg)    	"hide_on_na",(gboolean )(arg)
#define GMPC_METAIMAGE_GET_PROP_HIDE_ON_NA(arg)	"hide_on_na",(gboolean *)(arg)
#endif /* __GNUC__ && !__STRICT_ANSI__ */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
