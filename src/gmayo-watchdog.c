/*
 *  Copyright (C) 2012, sleep(5) ltd <http://sleepfive.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  Author: Tomas Frydrych <tomas@sleepfive.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <time.h>

#define GMAYOW_DBG_WANT(x) (gmayow_dbg & GMAYOW_DBG_##x)

typedef enum
{
  GMAYOW_DBG_STATUS  = (1 << 0),
  GMAYOW_DBG_SIGNALS = (1 << 1),

  GMAYOW_DBG_ALL     = 0xffffffff
} GmayowDbgFlags;

guint32 gmayow_dbg = 0;

static const GDebugKey gmayow_dbg_opts[] =
{
  {"status",  GMAYOW_DBG_STATUS},
  {"signals", GMAYOW_DBG_SIGNALS},

  {"all",     GMAYOW_DBG_ALL}
};

static gboolean
gmayow_dbg_opts_cb (const char *key, const char *val, gpointer data)
{
  gmayow_dbg |= g_parse_debug_string (val,
                                      gmayow_dbg_opts,
                                      G_N_ELEMENTS (gmayow_dbg_opts));
  return TRUE;
}

int
main (int argc, char **argv)
{
  GError          *error      = NULL;
  gboolean         keep_alive = FALSE;
  char           **child_argv = NULL;
  gint             i;
  gint             child_off  = 1; /* offset where the child argv starts */
  gint             retries = 5;
  time_t           launch_time;
  GOptionContext  *opt_ctx;
  GOptionEntry     opts[] = {
    {
      "debug", 'd', 0, G_OPTION_ARG_CALLBACK, gmayow_dbg_opts_cb,
      "Debugging flags to set", "DEBUG FLAGS"
    },
    {
      "keep-alive", 'a', 0, G_OPTION_ARG_NONE, &keep_alive,
      "Keep process alive even if it exits with status of 0", NULL
    },
    {NULL}
  };

  opt_ctx = g_option_context_new (" -- program [program arguments]");
  g_option_context_add_main_entries (opt_ctx, opts, NULL);

  if (!g_option_context_parse (opt_ctx, &argc, &argv, &error))
    g_error ("Failed to parse options: %s", error->message);

  /*
   * At least two arguments have to be left (our own name being the first)
   */
  if (argc < 2)
    g_error ("No child commandline supplied, quitting!");

  /*
   * After our own options have been parsed, the next option might be '--',
   * depending on what options were specified for the child program, skip this.
   */
  if (argv[1][0] == '-' && argv[1][1] == '-')
    child_off++;

  /*
   * Need a NULL-terminated copy for spawn, removing 1 or 2 elements
   */
  child_argv = g_malloc0 (sizeof (char*) * (argc - (child_off - 1)));

  for (i = child_off; i < argc; ++i)
    child_argv[i-child_off] = argv[i];

  while (1)
    {
      gint     status = 0;
      time_t   exit_time;
      gboolean quit = FALSE;

      if (GMAYOW_DBG_WANT (STATUS))
        g_message ("Spawning %s.", child_argv[0]);

      launch_time = time (NULL);

      if (!g_spawn_sync (NULL,
                         child_argv,
                         NULL,
                         G_SPAWN_SEARCH_PATH,
                         NULL, NULL, NULL, NULL, &status, &error))
        {
          g_warning ("Failed to spawn %s: %s", child_argv[0], error->message);
          exit (-1);
        }

      exit_time = time (NULL);

      /*
       * If we have been up for more than, say, 3 minutes, reset the retry
       * counter.
       */
      if ((exit_time - launch_time) > 3*60)
        retries = 5;
      else
        retries--;

      if (WIFEXITED (status))
        {
          gint c;

          if (!(c = WEXITSTATUS (status)) && !keep_alive)
            {
              if (GMAYOW_DBG_WANT (STATUS))
                g_message ("%s exited normally.", child_argv[0]);

              quit = TRUE;
            }
          else
            {
              if (GMAYOW_DBG_WANT (STATUS))
                g_message ("%s exited with status %d.", child_argv[0], c);
            }
        }
      else if (WIFSIGNALED (status))
        {
          gint c;

          switch ((c = WTERMSIG (status)))
            {
            case SIGTERM:
            case SIGKILL:
              if (GMAYOW_DBG_WANT (SIGNALS))
                g_message ("%s exited due to signal %d, quitting.",
                           child_argv[0], c);
              quit = TRUE;
              break;
            default:
              if (GMAYOW_DBG_WANT (SIGNALS))
                g_message ("%s exited due to signal %d.", child_argv[0], c);
            }
        }

      if (!quit)
        {
          if (retries > 0)
            {
              if (GMAYOW_DBG_WANT (SIGNALS))
                g_message ("Re-spawning %s, remaining attempts: %d",
                           child_argv[0], retries);
            }
          else
            {
              if (GMAYOW_DBG_WANT (SIGNALS))
                g_message ("max retry count reached, quitting");

              quit = TRUE;
            }
        }

      if (quit)
        break;
      else
        g_usleep (500 * 1000);
    }

  g_free (child_argv);
}
