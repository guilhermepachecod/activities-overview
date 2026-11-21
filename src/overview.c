#include "overview.h"

#include <gtk/gtk.h>

#include "background.h"

struct _AoOverview
{
  GPtrArray *windows; /* GtkWidget *, one per monitor, owned */
  gboolean   visible;
};

static gboolean
on_key_press (GtkWidget   *widget,
              GdkEventKey *event,
              gpointer     user_data)
{
  AoOverview *overview = user_data;

  (void) widget;

  if (event->keyval == GDK_KEY_Escape)
    {
      ao_overview_hide (overview);
      return TRUE;
    }

  return FALSE;
}

static gboolean
on_button_press (GtkWidget      *widget,
                  GdkEventButton *event,
                  gpointer        user_data)
{
  AoOverview *overview = user_data;

  (void) widget;
  (void) event;

  /* M1: the whole surface is still empty space. Once the window grid
   * (M2) lands, its own handler will stop this from firing on a click
   * that landed on a window thumbnail. */
  ao_overview_hide (overview);
  return TRUE;
}

static GtkWidget *
create_monitor_window (AoOverview *overview,
                        GdkMonitor *monitor)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_POPUP);
  GdkScreen *screen = gdk_screen_get_default ();
  GdkVisual *rgba_visual = gdk_screen_get_rgba_visual (screen);
  GdkRectangle geometry;

  if (rgba_visual != NULL)
    gtk_widget_set_visual (window, rgba_visual);

  gtk_widget_set_app_paintable (window, TRUE);
  gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
  gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);
  gtk_window_stick (GTK_WINDOW (window));

  gdk_monitor_get_geometry (monitor, &geometry);
  gtk_window_move (GTK_WINDOW (window), geometry.x, geometry.y);
  gtk_window_resize (GTK_WINDOW (window), geometry.width, geometry.height);

  ao_background_attach (window, geometry.width, geometry.height);

  g_signal_connect (window, "key-press-event", G_CALLBACK (on_key_press), overview);
  g_signal_connect (window, "button-press-event", G_CALLBACK (on_button_press), overview);

  return window;
}

AoOverview *
ao_overview_new (void)
{
  AoOverview *overview = g_new0 (AoOverview, 1);
  GdkDisplay *display = gdk_display_get_default ();
  int n_monitors = gdk_display_get_n_monitors (display);

  overview->windows = g_ptr_array_new ();
  overview->visible = FALSE;

  for (int i = 0; i < n_monitors; i++)
    {
      GdkMonitor *monitor = gdk_display_get_monitor (display, i);
      GtkWidget *window = create_monitor_window (overview, monitor);

      g_ptr_array_add (overview->windows, window);
    }

  return overview;
}

void
ao_overview_free (AoOverview *overview)
{
  if (overview == NULL)
    return;

  for (guint i = 0; i < overview->windows->len; i++)
    gtk_widget_destroy (g_ptr_array_index (overview->windows, i));

  g_ptr_array_free (overview->windows, TRUE);
  g_free (overview);
}

void
ao_overview_show (AoOverview *overview)
{
  if (overview->visible)
    return;

  for (guint i = 0; i < overview->windows->len; i++)
    {
      GtkWidget *window = g_ptr_array_index (overview->windows, i);

      gtk_widget_show (window);
      gtk_window_present (GTK_WINDOW (window));
    }

  overview->visible = TRUE;
}

void
ao_overview_hide (AoOverview *overview)
{
  if (!overview->visible)
    return;

  for (guint i = 0; i < overview->windows->len; i++)
    gtk_widget_hide (g_ptr_array_index (overview->windows, i));

  overview->visible = FALSE;
}

void
ao_overview_toggle (AoOverview *overview)
{
  if (overview->visible)
    ao_overview_hide (overview);
  else
    ao_overview_show (overview);
}

gboolean
ao_overview_is_visible (AoOverview *overview)
{
  return overview->visible;
}
