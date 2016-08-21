/* =====================================================================================
 *
 *   @file main.c
 *
 *
 *   Version:  1.0
 *   Created:  10/21/2012 07:49:49 PM
 *
 *   @section DESCRIPTION
 *
 *       Main forwarder
 *
 *   @section LICENSE
 *
 *
 *
 * =====================================================================================
 */

#include <uci.h>

#include "main.h"
#include "config.h"
#include "zeromq.h"
#include "utils.h"

#include "http_agent.h"
#include "persist_agent.h"
#include "ftp_agent.h"
#include "failure_agent.h"
#include "hashtable_agent.h"
#include "throughput_agent.h"

#include <assert.h>

#ifndef TESTING
void *ftp_agent_pipe = NULL;
void *http_agent_pipe = NULL;
void *failure_agent_pipe = NULL;
void *hashtable_agent_pipe = NULL;
void *persist_agent_pipe = NULL;
void *throughput_agent_pipe = NULL;
#endif

typedef struct _hermes_params_t {
  char *endpoint;
  bool realtime;
  bool backup;
  bool standalone;
  bool has_ftp;

  int maxruns;
} hermes_params_t;

static void s_help(void)
{
  printf("Usage: ./hermes [OPTIONS]\n" \
      " * config file: /etc/config/hermes\n" \
      " -r to enable realtime output\n" \
      " -b to enable local backup output\n" \
      " -f to enable ftp failover mode\n" \
      " -n NUMBER to run for NUMBER messages and quit\n");
  exit(1);
}

static void s_handle_cmdline(hermes_params_t* params, int argc, char** argv)
{

  int flags = 0;

  while (1+flags < argc && argv[1+flags][0] == '-') {
    switch (argv[1+flags][1]) {
      case 'n':
        if (flags+2<argc) {
          flags++;
          params->maxruns = atoi(argv[1+flags]);
        } else {
          errorLog("Error: Please specify a number of loops after -n option !");
        }
        break;
      case 'r':
        params->realtime = true;
        break;
      case 'b':
        params->backup = true;
        break;
      case 'f':
        params->has_ftp = true;
        break;
      case 'e':
        if (flags+2<argc) {
          flags++;
          params->endpoint = strndup(argv[1+flags],MAX_STRING_LEN);
        } else {
          errorLog("Error: Please specify a valid endpoint !");
          exit(0);
        }
        break;
      case 'h':
        s_help();
        break;
      default:
        printf("Error: Unsupported option '%s' !\n", argv[1+flags]);
        s_help();
        exit(1);
    }
    flags++;
  }

  if (argc < flags + 1)
    s_help();

}

zmsg_t *s_generate_simple_message(zmsg_t* message)
{
  char *timestamp = zmsg_popstr(message);
  char *mac_hash = zmsg_popstr(message);
  char *prefix = zmsg_popstr(message);
  char *dbm = zmsg_popstr(message);
  char *bssid = zmsg_popstr(message);
  char *ssid = NULL;
  if(zmsg_size(message) != 0) {
    ssid = zmsg_popstr(message);
  }

  char *json_msg = utils_generate_json(timestamp, mac_hash, prefix, dbm, bssid, ssid);

  mem_free(timestamp);
  mem_free(mac_hash);
  mem_free(prefix);
  mem_free(dbm);
  mem_free(bssid);
  if (ssid != NULL) {
    mem_free(ssid);
  }

  zmsg_t* output = zmsg_new();
  zmsg_addstr(output, MSG_POST);
  zmsg_addstr(output, json_msg);
  mem_free(json_msg);
  return output;
}

zmsg_t *s_generate_csv_message(zmsg_t* message)
{
  char csvline[MAX_STRING_LEN];

  char *timestamp = zmsg_popstr(message);
  char *mac_hash = zmsg_popstr(message);
  char *prefix = zmsg_popstr(message);
  char *dbm = zmsg_popstr(message);
  char *bssid = zmsg_popstr(message);
  char *ssid = NULL;
  if(zmsg_size(message) != 0) {
    ssid = zmsg_popstr(message);
  }

  memset(csvline, 0, sizeof(csvline));
  snprintf(csvline, MAX_STRING_LEN, "%s,%s,%s,%s,%s,%s",
      timestamp, mac_hash, prefix, dbm, bssid, ssid == NULL ? "" : ssid);

  mem_free(timestamp);
  mem_free(mac_hash);
  mem_free(prefix);
  mem_free(bssid);
  mem_free(dbm);
  if (ssid != NULL) {
    mem_free(ssid);
  }

  zmsg_t* output = zmsg_new();
  zmsg_addstr(output, MSG_POST);
  zmsg_addstr(output, csvline);
  return output;
}

zmsg_t *s_generate_throughput_message(zmsg_t* message)
{
  zmsg_t* output = zmsg_dup(message);
  zmsg_pushstr(output, MSG_POST_THROUGHPUT);
  return output;
}


#ifndef TESTING

int main(int argc, char *argv[])
{
  void *socket = NULL;
  char* endpoint = NULL;
  char* token = NULL;

  openlog("hermes", LOG_CONS | LOG_PID, LOG_USER);

  /*  Read config */
  hermes_params_t params;
  config_context *cfg_ctx = config_new();

  token = config_get_str(cfg_ctx, "hermes.http.token");
  if ((token == NULL) && (cfg_ctx->err != UCI_OK)) {
    errorLog("token is currently not provisioned");
    config_destroy(cfg_ctx);
    exit(1);
  }

  params.has_ftp = config_get_bool(cfg_ctx, "hermes.receiver.has_ftp");
  params.realtime = config_get_bool(cfg_ctx, "hermes.receiver.realtime");
  params.backup = config_get_bool(cfg_ctx, "hermes.receiver.backup");
  params.standalone = config_get_bool(cfg_ctx, "hermes.receiver.standalone");
  params.endpoint = config_get_str(cfg_ctx, "hermes.receiver.endpoint");
  config_destroy(cfg_ctx);

  params.maxruns = -1;

  /* Eventually override */
  s_handle_cmdline(&params, argc, argv);

  /*  ZMQ init */
  zctx_t *zmq_ctx = zctx_new ();
  socket = zeromq_create_socket(zmq_ctx, params.endpoint, ZMQ_PULL, NULL, false, -1, -1);
  assert (socket != NULL);

  debugLog("zeromq PULL socket @ %s opened", params.endpoint);

  /*  Create HTTP & FTP agents  */
  if (!params.standalone) {
    http_agent_pipe = zthread_fork(zmq_ctx, http_agent, NULL);
    assert(http_agent_pipe);
    debugLog("HTTP agent created");
    if (params.has_ftp) {
      ftp_agent_pipe = zthread_fork(zmq_ctx, ftp_agent, NULL);
      assert(ftp_agent_pipe);
      debugLog("FTP agent created");
    }
  }

  /*  Create Failure agent */
  if (!params.standalone) {
    failure_agent_pipe = zthread_fork(zmq_ctx, failure_agent, NULL);
    assert(failure_agent_pipe);
    debugLog("failure agent created");
  }

  /*  Create Hashtable & Throughput agent */
  if (!params.standalone && !params.realtime) {
    hashtable_agent_pipe = zthread_fork(zmq_ctx, hashtable_agent, http_agent_pipe);
    assert(hashtable_agent_pipe);
    debugLog("hashtable agent created");
    throughput_agent_pipe = zthread_fork(zmq_ctx, throughput_agent, hashtable_agent_pipe);
    assert(throughput_agent_pipe);
    debugLog("throughput agent created");
  }

  /*  Create Persist agent */
  if (params.standalone || params.backup) {
    persist_agent_pipe = zthread_fork(zmq_ctx, persist_agent, NULL);
    assert(persist_agent_pipe);
    debugLog("persist agent created");
  }
  
  /*  Main listener loop */
  while (!zctx_interrupted) {
    if (zsocket_poll(socket, 0)) {
      zmsg_t *message = zmsg_recv (socket);
      if (message) {
        if (params.standalone) {
          zmsg_t *backup_msg = s_generate_csv_message(message);
          agent_msg_send(&backup_msg, persist_agent_pipe);
        } else {
          if (params.backup) {
            zmsg_t* copy = zmsg_dup(message);
            zmsg_t *backup_msg = s_generate_csv_message(copy);
            agent_msg_send(&backup_msg, persist_agent_pipe);
            zmsg_destroy(&copy);
          }
          if (params.realtime) {
            zmsg_t *backup_msg = s_generate_simple_message(message);
            agent_msg_send(&backup_msg, http_agent_pipe);
          } else {
            zmsg_t *throughput_msg = s_generate_throughput_message(message);
            agent_msg_send(&throughput_msg, throughput_agent_pipe);
          }
        }
        zmsg_destroy(&message);
      }
    }

    if (params.maxruns != -1) {
      params.maxruns--;
      if (params.maxruns == 0)
        break;
    }

    usleep(HERMES_THREAD_SLEEP*1000);
  }

  debugLog("Exiting software gracefully after 2 seconds.");
  zctx_interrupted = true;
  sleep(2);

  zsocket_destroy (zmq_ctx, socket);
  zctx_destroy (&zmq_ctx);
  mem_free(params.endpoint);
  closelog();

  return 0;
}


#else

/* Test */
int receiver_agent_selftest(bool verbose)
{

    printf (" * receiver_agent (s_generate_json) : ");
    {
      char* json_0 = utils_generate_json(Fix_ts, Fix_v1, Fix_v2, Fix_v3, Fix_v4, Fix_v5);
      errorLog("COMPARE '%s' '%s'", json_0, Fix_json_built_0);
      assert (streq(json_0, Fix_json_built_0));
      char* json_1 = utils_generate_json(Fix_ts, Fix_v1, Fix_v2, Fix_v3, Fix_v4, NULL);
      assert (streq(json_1, Fix_json_built_1));
      char* json_2 = utils_generate_json(Fix_ts, Fix_v1, Fix_v2, Fix_v3, Fix_v4, Fix_v5_escapes);
      assert (streq(json_2, Fix_json_built_0_escapes));
      char* json_3 = utils_generate_json(Fix_ts, Fix_v1, Fix_v2, Fix_v3, Fix_v4, Fix_v5_other_escapes);
      assert (streq(json_3, Fix_json_built_0_other_escapes));
      printf ("OK\n");
    }

    /*  Test that we generate the right messages  */
    printf (" * receiver_agent (simple POST with SSID) : ");
    {
      char *str = NULL;
      zmsg_t *in = zmsg_new();
      zmsg_addstr(in, Fix_ts);
      zmsg_addstr(in, Fix_v1);
      zmsg_addstr(in, Fix_v2);
      zmsg_addstr(in, Fix_v3);
      zmsg_addstr(in, Fix_v4);
      zmsg_addstr(in, Fix_v5);

      zmsg_t *out = s_generate_simple_message(in);
      assert (zmsg_size(out) == 2);
      str = zmsg_popstr(out);
      assert (streq(str, MSG_POST));
      free(str);
      str = zmsg_popstr(out);
      assert (streq(str, Fix_json_built_0));
      free(str);
      zmsg_destroy(&in);
      zmsg_destroy(&out);
      printf ("OK\n");
    }

    printf (" * receiver_agent (simple POST without SSID) : ");
    {
      char *str = NULL;
      zmsg_t *in = zmsg_new();
      zmsg_addstr(in, Fix_ts);
      zmsg_addstr(in, Fix_v1);
      zmsg_addstr(in, Fix_v2);
      zmsg_addstr(in, Fix_v3);
      zmsg_addstr(in, Fix_v4);

      zmsg_t *out = s_generate_simple_message(in);
      assert (zmsg_size(out) == 2);
      str = zmsg_popstr(out);
      assert (streq(str, MSG_POST));
      free(str);
      str = zmsg_popstr(out);
      assert (streq(str, Fix_json_built_1));
      free(str);
      zmsg_destroy(&in);
      zmsg_destroy(&out);
      printf ("OK\n");
    }


    return 0;
}

#endif
