/**
 * =====================================================================================
 *
 *   @file persist_agent.c
 *
 *   
 *        Version:  1.0
 *        Created:  04/16/2013 08:23:56 PM
 *
 *
 *   @section DESCRIPTION
 *
 *       
 *       
 *   @section LICENSE
 *
 *       
 *
 * =====================================================================================
 */

#include "main.h"
#include "config.h"
#include "utils.h"
#include "persist_agent.h"

#include <libgen.h>

void persist_agent (void *user_args, zctx_t *ctx, void *pipe)
{
  FILE *persist_file = NULL;
  char new_filename[MAX_STRING_LEN];

  /*  Read config */
  config_context *cfg_ctx = config_new();
  int buffer_lines = config_get_int(cfg_ctx, "hermes.persist.buffer_lines");
  debugLog("Max buffer lines: %d", buffer_lines);
  config_destroy(cfg_ctx);

  /*  List of stored strings */
  zlist_t *csv_lines = zlist_new();
  zlist_autofree(csv_lines);

  while (!zctx_interrupted) {
    if (zsocket_poll(pipe, 0)) {
      zmsg_t* message = zmsg_recv (pipe);
      if (!message) continue;

      char *reason = zmsg_popstr(message);
      if (streq(reason, MSG_POST)) {
        char *csv_line = zmsg_popstr(message);
        debugLog("> [msg #%06d] '%s'", (int)zlist_size(csv_lines), csv_line);
        zlist_append(csv_lines, csv_line);
        mem_free(csv_line);
      }
      free(reason);
      agent_msg_destroy(&message); 
    }

    usleep(HERMES_THREAD_SLEEP*1000);

    // TODO: if vfat is not there, we should wait a little before retrying
    if (zlist_size(csv_lines) > buffer_lines) {
      /*  open file for appending, with direct sync */
      char *vfat_path = utils_get_vfat_mountpoint();
      if (vfat_path != NULL) {
        bzero(new_filename, sizeof(new_filename));
        snprintf(new_filename, MAX_STRING_LEN, "%s/%s", vfat_path, "persisted.csv");
        persist_file = fopen(new_filename, "a");
        if (persist_file != NULL) {
          setvbuf(persist_file, NULL, _IONBF, 0);
          fcntl(fileno(persist_file), F_SETFL, fcntl(fileno(persist_file), F_GETFL) | O_DSYNC | O_RSYNC);
          char *csv_line = zlist_pop(csv_lines);
          while (csv_line != NULL) {
            fprintf(persist_file, "%s\n", csv_line);
            debugLog("echo '%s' to %s", csv_line, new_filename);
            mem_free(csv_line);
            csv_line = zlist_pop(csv_lines);
          }
          fflush(persist_file);
          fclose(persist_file);
        }
        mem_free(vfat_path);
      }
    }
  }

  zlist_destroy(&csv_lines);
}
