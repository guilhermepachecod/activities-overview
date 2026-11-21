#pragma once

#include <gtk/gtk.h>

/* Paints window's background with the current MATE wallpaper (org.mate.background),
 * scaled to width x height and dimmed. Must be called once, right after the
 * window is created; the window must stay app-paintable. */
void ao_background_attach (GtkWidget *window,
                            int        width,
                            int        height);
