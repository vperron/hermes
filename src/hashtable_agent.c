/**
 * =====================================================================================
 *
 *   @file hashtable_agent.c
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
#include "hashtable_agent.h"

int s_append_to_msg(const char *key, void *item, void *argument) 
{
  zmsg_t* msg = (zmsg_t*) argument;
  assert(key != NULL);
  assert(item != NULL);
  zmsg_addstr(msg, item);
  return 0;
}

#ifndef E2E_TESTING
void hashtable_agent (void *user_args, zctx_t *ctx, void *pipe) 
{

  /*  Read config */
  config_context *cfg_ctx = config_new();
  int first_maxsize = 10;
  int maxsize = config_get_int(cfg_ctx, "hermes.hashtable.maxsize");
  uint64_t maxtime = (uint64_t)config_get_int(cfg_ctx, "hermes.hashtable.maxtime") * 1000;
  config_destroy(cfg_ctx);

  zlist_t *beacon_list = zlist_new();
  zlist_autofree(beacon_list);
  uint64_t creation_time = now();

  while (!zctx_interrupted) {
    if (zsocket_poll(pipe, 0)) {
      zmsg_t* message = zmsg_recv (pipe);
      if (!message) continue;

#else
void hashtable_agent (zmsg_t* message, zlist_t* beacon_list, int maxsize, uint64_t maxtime, int first_maxsize) 
{
#endif

      char *reason = zmsg_popstr(message);
      if (streq(reason, MSG_POST_HASH)) {
        assert (zmsg_size(message) == 1);
        char *json_str = zmsg_popstr(message);
        assert (json_str != 0);
        /*  zlist_t implementation leaves key unchanged if already there */
        zlist_append (beacon_list, json_str);
        // debugLog("Received %s : added to list as item n°%d",json_str, (int)zlist_size(beacon_list));
        mem_free(json_str);
      }
      mem_free(reason);
      agent_msg_destroy(&message);

#ifndef E2E_TESTING
    } // zsocket_poll
    usleep(HERMES_THREAD_SLEEP*1000);
#endif

    if (http_agent_pipe) {
      if ( (first_maxsize != 0 && zlist_size(beacon_list) > first_maxsize) || 
          zlist_size(beacon_list) >= maxsize || 
          (zlist_size(beacon_list) > 1 && (now() > creation_time + maxtime))
       ) {

        first_maxsize = 0;
        debugLog("Flushing beacon_list to HTTP after %llu seconds [%d records]", 
            (long long unsigned)(now() - creation_time)/1000, (int)zlist_size(beacon_list));

        zmsg_t* output = zmsg_new();
        zmsg_addstr(output, MSG_POST);
        char* json_str = zlist_first(beacon_list);
        debugLog("HASHTABLE %s", json_str);
        while(json_str != NULL) {
          zmsg_addstr(output, json_str);
          json_str = zlist_next(beacon_list);
        }
        agent_msg_send(&output, http_agent_pipe);
        zlist_destroy(&beacon_list);
        beacon_list = zlist_new();
        zlist_autofree(beacon_list);
        creation_time = now();
      }
    }

#ifndef E2E_TESTING
  } // while(!zctx_interrupted)

  zlist_destroy(&beacon_list);
#endif
}

#ifdef TESTING

/* Test */
int hashtable_agent_selftest(bool verbose)
{
    printf (" * hashtable_agent (s_append_to_msg) : ");
    { 
      char *str = NULL;
      zlist_t *beacon_list = zlist_new();
      zlist_append(beacon_list, Fix_json_built_0);
      zlist_append(beacon_list, Fix_json_built_0);
      zlist_append(beacon_list, Fix_json_built_1);

      assert (zlist_size(beacon_list) == 3);

      zmsg_t *output = zmsg_new();
      char* json_str = zlist_first(beacon_list);
      while(json_str != NULL) {
        zmsg_addstr(output, json_str);
        json_str = zlist_next(beacon_list);
      }
      assert (zmsg_size(output) == 3);

      str = zmsg_popstr(output);
      assert (streq(str, Fix_json_built_0));
      free(str);

      str = zmsg_popstr(output);
      assert (streq(str, Fix_json_built_0));
      free(str);

      str = zmsg_popstr(output);
      assert (streq(str, Fix_json_built_1));
      free(str);

      zmsg_destroy(&output);
      zlist_destroy(&beacon_list);
      printf ("OK\n");
    }
    
    return 0;
}

#endif
