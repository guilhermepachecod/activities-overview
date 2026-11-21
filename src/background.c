#include "background.h"

#include <cairo.h>
#include <gio/gio.h>

#define AO_BACKGROUND_DIM_ALPHA 0.55

typedef struct
{
  GdkPixbuf *wallpaper; /* owned, may be NULL if it could not be loaded */
} AoBackgroundData;

static void
ao_background_data_free (gpointer  data,
                          GClosure *closure)
{
  AoBackgroundData *bg = data;

  (void) closure;

  g_clear_object (&bg->wallpaper);
  g_free (bg);
}

static gboolean
on_draw (GtkWidget *widget,
         cairo_t   *cr,
         gpointer   user_data)
{
  AoBackgroundData *bg = user_data;
  GtkAllocation allocation;

  gtk_widget_get_allocation (widget, &allocation);

  if (bg->wallpaper != NULL)
    {
      gdk_cairo_set_source_pixbuf (cr, bg->wallpaper, 0, 0);
      cairo_paint (cr);
    }
  else
    {
      cairo_set_source_rgb (cr, 0.1, 0.1, 0.1);
      cairo_paint (cr);
    }

  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, AO_BACKGROUND_DIM_ALPHA);
  cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
  cairo_fill (cr);

  return FALSE;
}

void
ao_background_attach (GtkWidget *window,
                       int        width,
                       int        height)
{
  AoBackgroundData *bg = g_new0 (AoBackgroundData, 1);
  GSettings *settings = g_settings_new ("org.mate.background");
  char *path = g_settings_get_string (settings, "picture-filename");

  if (path != NULL && *path != '\0')
    {
      GError *error = NULL;

      bg->wallpaper = gdk_pixbuf_new_from_file_at_scale (path, width, height,
                                                          FALSE, &error);
      if (bg->wallpaper == NULL)
        {
          g_warning ("Could not load wallpaper '%s': %s", path, error->message);
          g_clear_error (&error);
        }
    }

  g_free (path);
  g_object_unref (settings);

  g_signal_connect_data (window, "draw", G_CALLBACK (on_draw), bg,
                          ao_background_data_free, 0);
}
