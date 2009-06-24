
#ifndef __GMPC_CONNECTION_H__
#define __GMPC_CONNECTION_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS


#define GMPC_TYPE_CONNECTION (gmpc_connection_get_type ())
#define GMPC_CONNECTION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_TYPE_CONNECTION, GmpcConnection))
#define GMPC_CONNECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_TYPE_CONNECTION, GmpcConnectionClass))
#define GMPC_IS_CONNECTION(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_TYPE_CONNECTION))
#define GMPC_IS_CONNECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_TYPE_CONNECTION))
#define GMPC_CONNECTION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_TYPE_CONNECTION, GmpcConnectionClass))

typedef struct _GmpcConnection GmpcConnection;
typedef struct _GmpcConnectionClass GmpcConnectionClass;
typedef struct _GmpcConnectionPrivate GmpcConnectionPrivate;

struct _GmpcConnection {
	GObject parent_instance;
	GmpcConnectionPrivate * priv;
};

struct _GmpcConnectionClass {
	GObjectClass parent_class;
};


GType gmpc_connection_get_type (void);
GmpcConnection* gmpc_connection_new (void);
GmpcConnection* gmpc_connection_construct (GType object_type);


G_END_DECLS

#endif