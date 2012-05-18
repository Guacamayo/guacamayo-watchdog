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
   * After our own options have been parsed, the next option should be '--'
   * followed by the program to spawn + its options.
   */
  if (argc < 3 || !argv[2] || !*argv[2] ||
      !(argv[1][0] == '-' && argv[1][1] == '-'))
    {
      g_error ("No child commandline supplied, quitting!");
    }

  while (1)
    {
      gint status = 0;

      if (GMAYOW_DBG_WANT (STATUS))
        g_message ("Spawning %s.", argv[2]);

      if (!g_spawn_sync (NULL,
                         argv + 2,
                         NULL,
                         G_SPAWN_SEARCH_PATH        |
                         G_SPAWN_STDOUT_TO_DEV_NULL |
                         G_SPAWN_STDERR_TO_DEV_NULL,
                         NULL, NULL, NULL, NULL, &status, &error))
        {
          g_warning ("Failed to spawn %s: %s", argv[2], error->message);
          exit (-1);
        }

      if (WIFEXITED (status))
        {
          gint c;

          if (!(c = WEXITSTATUS (status)) && !keep_alive)
            {
              if (GMAYOW_DBG_WANT (STATUS))
                g_message ("%s exited normally, quitting.", argv[2]);
              break;
            }
          else
            {
              if (GMAYOW_DBG_WANT (STATUS))
                g_message ("%s exited with status %d, re-spawning.",
                         argv[2]);
              continue;
            }
        }
      else if (WIFSIGNALED (status) && GMAYOW_DBG_WANT (SIGNALS))
        {
          gint c = WTERMSIG (status);

          g_message ("%s exited due to signal %d, re-spawning.", argv[2]);
          continue;
        }
    }
}
