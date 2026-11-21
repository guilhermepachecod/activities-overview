#include <gtk/gtk.h>
#include <stdlib.h>

#include "overview.h"

#define AO_APP_ID "io.github.guilhermepachecod.ActivitiesOverview"

/* G_APPLICATION_DEFAULT_FLAGS only exists since GLib 2.74; older GLib
 * (e.g. AlmaLinux 9) only has the now-deprecated G_APPLICATION_FLAGS_NONE,
 * which is the same value. */
#ifndef G_APPLICATION_DEFAULT_FLAGS
#define G_APPLICATION_DEFAULT_FLAGS G_APPLICATION_FLAGS_NONE
#endif

static void
action_toggle (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  (void) action;
  (void) parameter;
  ao_overview_toggle (user_data);
}

static void
action_show (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  (void) action;
  (void) parameter;
  ao_overview_show (user_data);
}

static void
action_hide (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  (void) action;
  (void) parameter;
  ao_overview_hide (user_data);
}

int
main (int argc, char **argv)
{
  gboolean opt_daemon = FALSE;
  gboolean opt_toggle = FALSE;
  gboolean opt_show = FALSE;
  gboolean opt_hide = FALSE;
  gboolean opt_version = FALSE;
  GOptionEntry entries[] = {
    { "daemon", 0, 0, G_OPTION_ARG_NONE, &opt_daemon, "Run as the background daemon", NULL },
    { "toggle", 0, 0, G_OPTION_ARG_NONE, &opt_toggle, "Toggle the overview", NULL },
    { "show", 0, 0, G_OPTION_ARG_NONE, &opt_show, "Show the overview", NULL },
    { "hide", 0, 0, G_OPTION_ARG_NONE, &opt_hide, "Hide the overview", NULL },
    { "version", 0, 0, G_OPTION_ARG_NONE, &opt_version, "Print version and exit", NULL },
    { NULL, 0, 0, 0, NULL, NULL, NULL }
  };
  GOptionContext *context;
  GError *error = NULL;
  GtkApplication *app;
  AoOverview *overview;
  int status;

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
      g_option_context_free (context);
      return EXIT_FAILURE;
    }
  g_option_context_free (context);

  if (opt_version)
    {
      g_print ("activities-overview %s\n", PACKAGE_VERSION);
      return EXIT_SUCCESS;
    }

  /* No flag at all (e.g. a bare keybinding command) behaves like --toggle. */
  if (!opt_daemon && !opt_toggle && !opt_show && !opt_hide)
    opt_toggle = TRUE;

  gtk_init (&argc, &argv);

  app = gtk_application_new (AO_APP_ID, G_APPLICATION_DEFAULT_FLAGS);

  if (!g_application_register (G_APPLICATION (app), NULL, &error))
    {
      g_printerr ("Failed to register application: %s\n", error->message);
      g_error_free (error);
      g_object_unref (app);
      return EXIT_FAILURE;
    }

  if (g_application_get_is_remote (G_APPLICATION (app)))
    {
      /* A daemon is already running: ask it to act and get out of the way. */
      if (opt_toggle)
        g_action_group_activate_action (G_ACTION_GROUP (app), "toggle", NULL);
      else if (opt_show)
        g_action_group_activate_action (G_ACTION_GROUP (app), "show", NULL);
      else if (opt_hide)
        g_action_group_activate_action (G_ACTION_GROUP (app), "hide", NULL);
      /* --daemon while one is already running: nothing to do. */

      g_object_unref (app);
      return EXIT_SUCCESS;
    }

  /* We are the primary instance: become the daemon. */
  overview = ao_overview_new ();

  GActionEntry action_entries[] = {
    { "toggle", action_toggle, NULL, NULL, NULL, { 0 } },
    { "show", action_show, NULL, NULL, NULL, { 0 } },
    { "hide", action_hide, NULL, NULL, NULL, { 0 } },
  };
  g_action_map_add_action_entries (G_ACTION_MAP (app), action_entries,
                                    G_N_ELEMENTS (action_entries), overview);

  g_application_hold (G_APPLICATION (app)); /* stay alive with no window open */

  if (opt_toggle)
    ao_overview_toggle (overview);
  else if (opt_show)
    ao_overview_show (overview);
  /* --hide or a cold --daemon start: nothing to show yet. */

  status = g_application_run (G_APPLICATION (app), 0, NULL);

  ao_overview_free (overview);
  g_object_unref (app);

  return status;
}
