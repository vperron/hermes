/**
 * =====================================================================================
 *
 *   @file ftp_agent.c
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
#include "ftp.h"
#include "ftp_agent.h"

void ftp_agent (void *user_args, zctx_t *ctx, void *pipe)
{
  char* endpoint = NULL;
  char* folder = NULL;

  /*  Read config */
  config_context *cfg_ctx = config_new();
  endpoint = config_get_str(cfg_ctx, "hermes.ftp.endpoint");
  folder = config_get_str(cfg_ctx, "hermes.ftp.folder");
  int timeout = config_get_int(cfg_ctx, "hermes.ftp.timeout");
  config_destroy(cfg_ctx);

  /*  Init FTP */
  ftp_handle_t *ftp = ftp_new(endpoint, folder, timeout);

  while (!zctx_interrupted) {
    if (zsocket_poll(pipe, 0)) {
      zmsg_t* message = zmsg_recv (pipe);
      if (!message) continue;

      char *reason = zmsg_popstr(message);
      if (streq(reason, MSG_POST_FILE)) {
        char *msg_id = zmsg_popstr(message);
        char *filename = zmsg_popstr(message);
        debugLog("FTP sending file '%s' of msgid '%s'", filename, msg_id);
        if (ftp_file_upload(ftp, filename) == STATUS_OK) {
          utils_send_success(msg_id);
        } else {
          utils_send_failure(msg_id);
        }
        mem_free(filename);
        mem_free(msg_id);
      }
      free(reason);
      agent_msg_destroy(&message); 
    }
    usleep(HERMES_THREAD_SLEEP*1000);
  }

  ftp_destroy(ftp);
  mem_free(endpoint);
  mem_free(folder);
}
