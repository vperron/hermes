/**
 * =====================================================================================
 *
 *   @file http_agent.c
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
#include "sops.h"
#include "http_agent.h"

static void s_dump_to_file(const char* filename, const char* data, int len) {
  FILE* file = fopen(filename, "w");
  if (file != NULL) {
    setvbuf(file, NULL, _IONBF, 0);
    fcntl(fileno(file), F_SETFL, fcntl(fileno(file), F_GETFL) | O_DSYNC | O_RSYNC);
    fprintf(file, "%s\n", data);
    fflush(file);
    fclose(file);
  }
}

char *s_generate_jsonarray(zmsg_t *msg)
{
  char *ptr = NULL;
  int i = 0;
  int parts_num = zmsg_size(msg);
  int parts_size = zmsg_content_size(msg);
  // Total size = json parts concatenation, commas, terminators, and null space
  int total_size = parts_size + parts_num * sizeof(char) + 4;

  char *output = malloc(total_size);
  memset(output, 0, total_size);
  ptr = output; *ptr = '['; ptr++;
  i = 0;
  while (1 && parts_num != 0) {
    char* json = zmsg_popstr(msg);
    /*  FIXME: This is currently a bug in czmq or libzmq. The quickfix
     *  below should be removed with the next stable version of either
     *  of those libraries. */
    if (streq(json, MSG_POST_ID)) {
      mem_free(json);
      ptr--;
      break;
    }
    int len = strlen(json);
    memcpy(ptr, json, len);
    mem_free(json);
    ptr += len;
    if (i == parts_num - 1)
      break;
    *ptr = ','; ptr++;
    i++;
  }
  *ptr = ']';

  return output;
}

#ifndef E2E_TESTING
void http_agent (void *user_args, zctx_t *ctx, void *pipe)
#else
void http_agent (zmsg_t* message)
#endif
{

  char* server = NULL;
  char* auth_endpoint = NULL;
  char* data_endpoint = NULL;
  char* token = NULL;
  char* version = NULL;
  char* username = NULL;
  char* password = NULL;
  char* status_file_name = NULL;

  sops_handle_t* sops = NULL;

  /*  Read config */
  config_context *cfg_ctx = config_new();
  server = config_get_str(cfg_ctx, "hermes.http.server");
  auth_endpoint = config_get_str(cfg_ctx, "hermes.http.auth_endpoint");
  data_endpoint = config_get_str(cfg_ctx, "hermes.http.data_endpoint");
  token =  config_get_str(cfg_ctx, "hermes.http.token");
  version =  config_get_str(cfg_ctx, "hermes.http.version");
  username = config_get_str(cfg_ctx, "hermes.http.username");
  password = config_get_str(cfg_ctx, "hermes.http.password");
  int timeout = config_get_int(cfg_ctx, "hermes.http.timeout");
  bool compress_requests = config_get_bool(cfg_ctx, "hermes.http.compress_requests");
  status_file_name = config_get_str(cfg_ctx, "hermes.meta.status_file_name");
  if (status_file_name == NULL) {
    status_file_name = strdup("/tmp/hermes_status");
  }
  config_destroy(cfg_ctx);

#ifndef E2E_TESTING
  if ((token == NULL) && (auth_endpoint == NULL || username == NULL || password == NULL)) {
    errorLog("Error in HTTP authentication parameters. Check your hermes configuration file." );
    exit(1);
  }
#endif

  /*  Init SOPS */
  if (token != NULL) {
    sops = sops_new_token(server, token, version, timeout);
  } else {
    sops = sops_new_user_password(server, auth_endpoint, username, password, timeout);
  }

  if (sops) sops_compress_requests(sops, compress_requests);

#ifndef E2E_TESTING
  while (!zctx_interrupted) {
    if (zsocket_poll(pipe, 0)) {
      zmsg_t* message = zmsg_recv (pipe);
      if (!message) continue;
#endif

      /*  If token has not been received */
      if (sops == NULL) {
        if ((token == NULL) &&  (auth_endpoint != NULL) && (username != NULL) && (password != NULL)) {
          debugLog("Renegociating SOPS token.");
          sops = sops_new_user_password(server, auth_endpoint, username, password, timeout);
        }
        if (sops) sops_compress_requests(sops, compress_requests);
      }

      char *reason = zmsg_popstr(message);
      if (streq(reason, MSG_POST_ID)) {
        char *msg_id = zmsg_popstr(message);
        char *jsonarray = zmsg_popstr(message);
        if (sops != NULL &&
            sops_post_datastream(sops, data_endpoint, (const char*) jsonarray, 
              strlen(jsonarray)) == STATUS_OK) {
          utils_send_success(msg_id);
          debugLog("Re-Sending '%s' to '%s' succeeded.", msg_id, data_endpoint);
        } else {
          utils_send_failure(msg_id);
          debugLog("Re-Sending '%s' to '%s' failed.", msg_id, data_endpoint);
        }
        mem_free(jsonarray);
        mem_free(msg_id);
      } else if (streq(reason, MSG_POST)) {
        char *jsonarray = s_generate_jsonarray(message);
        if (sops == NULL ||
            sops_post_datastream(sops, data_endpoint, (const char*) jsonarray, 
              strlen(jsonarray)) == STATUS_ERROR) {
          debugLog("Upload to '%s' failed : sending to Failure agent", data_endpoint);
          zmsg_t *status = zmsg_new();
          zmsg_addstr(status, MSG_POST);
          zmsg_addstr(status, jsonarray);
          if (failure_agent_pipe != NULL) {
            agent_msg_send(&status, failure_agent_pipe);
          } else {
            agent_msg_destroy(&status);
          }
        } else {
          debugLog("Upload to '%s' succeeded.", data_endpoint);
          s_dump_to_file(status_file_name, (const char*)jsonarray, strlen(jsonarray));
        }
        mem_free(jsonarray);
      }
      mem_free(reason);
      agent_msg_destroy(&message);

#ifndef E2E_TESTING
    }
    usleep(HERMES_THREAD_SLEEP*1000);
  }
#endif

  sops_destroy(sops);
  mem_free(server);
  mem_free(auth_endpoint);
  mem_free(data_endpoint);
  mem_free(username);
  mem_free(password);
}

#ifdef TESTING

/* Test */
int http_agent_selftest(bool verbose)
{
    printf (" * http_agent: ");

    /*  Test that we generate a right JSON array */
    {
      zmsg_t* msg = zmsg_new();
      zmsg_addstr(msg, Fix_json1);
      zmsg_addstr(msg, Fix_json2);
      zmsg_addstr(msg, Fix_json3);
      char *array = s_generate_jsonarray(msg);
      assert (streq(array, Fix_json_aggreg));
      agent_msg_destroy(&msg);
    }


    /*  Test that the nasty ZMQ bug is fixed : we ignore what happens after POSTID */
    {
      zmsg_t* msg = zmsg_new();
      zmsg_addstr(msg, Fix_json1);
      zmsg_addstr(msg, Fix_json2);
      zmsg_addstr(msg, Fix_json3);
      zmsg_addstr(msg, MSG_POST_ID);
      zmsg_addstr(msg, "some-message-hash");
      zmsg_addstr(msg, Fix_json_aggreg_3);

      char *array = s_generate_jsonarray(msg);
      assert (streq(array, Fix_json_aggreg));
      agent_msg_destroy(&msg);
    }


    printf ("OK\n");
    return 0;
}

#endif
