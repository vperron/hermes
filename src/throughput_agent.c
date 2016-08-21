/**
 * =====================================================================================
 *
 *   @file throughput_agent.c
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
#include "throughput_agent.h"

typedef struct msg_record_t {
  int dBm_total;
  int n_dBm;
  char *timestamp;
  char *mac_hash;
  char *prefix;
  char *bssid;
  char *ssid;
} msg_record;


void s_free_record(msg_record* record) {
  mem_free(record->timestamp);
  mem_free(record->mac_hash);
  mem_free(record->prefix);
  mem_free(record->bssid);
  if (record->ssid != NULL) {
    mem_free(record->ssid);
  }
  free(record);
}

zmsg_t *s_record_to_hash_message(msg_record* record)
{
  char dbm[MAX_STRING_LEN];
  memset(dbm, 0, MAX_STRING_LEN);
  char *timestamp = record->timestamp;
  char *mac_hash = record->mac_hash;
  char *prefix = record->prefix;
  snprintf(dbm, MAX_STRING_LEN, "%d", record->dBm_total / record->n_dBm);
  char *bssid = record->bssid;
  char *ssid = record->ssid;

  char *json_msg = utils_generate_json(timestamp, mac_hash, prefix, dbm, bssid, ssid);

  zmsg_t* output = zmsg_new();
  zmsg_addstr(output, MSG_POST_HASH);
  zmsg_addstr(output, json_msg);

  mem_free(json_msg);
  return output;
}

int s_send_and_free_record(const char *key, void *item, void *argument) 
{
  assert(key != NULL);
  assert(item != NULL);

  msg_record* record = (msg_record*) item;
  zmsg_t* output = s_record_to_hash_message(record);
  agent_msg_send(&output, hashtable_agent_pipe);

  s_free_record(record);
  return 0;
}

char *s_generate_key(const char* mac, const char* prefix, const char* ssid)
{
  char hashkey[MAX_STRING_LEN];
  memset(hashkey, 0, MAX_STRING_LEN);
  if (ssid != NULL) {
    snprintf(hashkey, MAX_STRING_LEN, "%s%s%s", mac, prefix, ssid);
  } else {
    snprintf(hashkey, MAX_STRING_LEN, "%s%s", mac, prefix);
  }
  return strdup(hashkey);
}


#ifndef E2E_TESTING
void throughput_agent (void *user_args, zctx_t *ctx, void *pipe) 
{

  /*  Read config */
  config_context *cfg_ctx = config_new();
  int maxsize = config_get_int(cfg_ctx, "hermes.throughput.maxsize");
  uint64_t maxtime = (uint64_t)config_get_int(cfg_ctx, "hermes.throughput.maxtime") * 1000;
  config_destroy(cfg_ctx);

  zhash_t *throughput = zhash_new();
  uint64_t creation_time = now();

  while (!zctx_interrupted) {
    if (zsocket_poll(pipe, 0)) {
      zmsg_t* message = zmsg_recv (pipe);
      if (!message) continue;

#else
/**
 * Keeps all the incoming messages for one second and
 * outputs a list of the beacons, averaged by RSSI.
 *
 * 
 *
 */
void throughput_agent (zmsg_t* message, zhash_t* throughput, int maxsize, uint64_t maxtime) 
{
#endif

      char *reason = zmsg_popstr(message);
      if (streq(reason, MSG_POST_THROUGHPUT)) {
        assert (zmsg_size(message) >= 4);
        char *timestamp = zmsg_popstr(message);
        char *mac_hash = zmsg_popstr(message);
        char *prefix = zmsg_popstr(message);
        char *dbm = zmsg_popstr(message);
        char *bssid = zmsg_popstr(message);
        char *ssid = NULL;
        if(zmsg_size(message) != 0) {
          ssid = zmsg_popstr(message);
          assert (ssid != 0);
        }

        assert (timestamp != 0);
        assert (mac_hash != 0);
        assert (prefix != 0);
        assert (dbm != 0);
        assert (bssid != 0);
        int u_dBm = strtol(dbm, NULL, 10);

        char *key = s_generate_key(mac_hash, prefix, ssid);
        msg_record* record = (msg_record*) zhash_lookup(throughput, key);
        if (record != NULL) {
          record->dBm_total = record->dBm_total + u_dBm;
          record->n_dBm = record->n_dBm + 1;
        } else {
          record = malloc(sizeof(msg_record));
          bzero(record, sizeof(msg_record));
          record->dBm_total = u_dBm;
          record->n_dBm = 1;
          record->timestamp = strdup(timestamp);
          record->mac_hash = strdup(mac_hash);
          record->prefix = strdup(prefix);
          record->bssid = strdup(bssid);
          if(ssid != NULL) {
            record->ssid = strdup(ssid);
          }
          zhash_insert (throughput, key, (void*) record);
        }
        
        mem_free(timestamp);
        mem_free(mac_hash);
        mem_free(bssid);
        mem_free(prefix);
        mem_free(dbm);
        if(ssid != NULL) {
          mem_free(ssid);
        }
        mem_free(key); // Key is always duplicated anyway
      }
      mem_free(reason);
      agent_msg_destroy(&message);

#ifndef E2E_TESTING
    } // zsocket_poll
    usleep(HERMES_THREAD_SLEEP*1000);
#endif

    if (hashtable_agent_pipe) {
      if (zhash_size(throughput) >= maxsize || now() > creation_time + maxtime) {
        debugLog("Flushing throughput to hashtable after %llu seconds [%d records]", 
            (long long unsigned)(now() - creation_time)/1000, (int)zhash_size(throughput));

        zhash_foreach (throughput, s_send_and_free_record, NULL);

        zhash_destroy(&throughput);
        throughput = zhash_new();
        creation_time = now();
      }
    }

#ifndef E2E_TESTING
  } // while(!zctx_interrupted)

  zhash_destroy(&throughput);
#endif
}

#ifdef TESTING

/* Test */
int throughput_agent_selftest(bool verbose)
{
    printf (" * throughput_agent (s_generate_key) : ");
    {
      char* key_0 = s_generate_key(Fix_v1, Fix_v2, Fix_v3, Fix_v4);
      assert (streq(key_0, Fix_json_hash_0));
      char* key_1 = s_generate_key(Fix_v1, Fix_v2, Fix_v3, NULL);
      assert (streq(key_1, Fix_json_hash_1));
      printf ("OK\n");
    }

    printf (" * throughput_agent (record to hash message) : ");
    {
      msg_record record;
      record.timestamp = Fix_ts;
      record.mac_hash = Fix_v1;
      record.prefix = Fix_v2;
      record.dBm_total = Fix_v3 * 5;
      record.n_dBm = 5;
      record.bssid = Fix_v4;
      record.ssid = Fix_v5;

      zmsg_t *out = s_record_to_hash_message(&record);
      assert (zmsg_size(out) == 3);
      str = zmsg_popstr(out);
      assert (streq(str, MSG_POST_HASH));
      free(str);
      str = zmsg_popstr(out);
      assert (streq(str, Fix_json_built_0));
      free(str);
      zmsg_destroy(&out);
      printf ("OK\n");
    }

    printf (" * throughput_agent (s_append_to_msg) : ");
    { 
      char *str = NULL;
      zhash_t *throughput = zhash_new();
      zhash_insert(throughput, Fix_json_hash_0, Fix_json_built_0);
      zhash_insert(throughput, Fix_json_hash_0, Fix_json_built_0);
      zhash_insert(throughput, Fix_json_hash_1, Fix_json_built_1);

      assert (zhash_size(throughput) == 2);

      zmsg_t *output = zmsg_new();
      zhash_foreach (throughput, s_append_to_msg, output);
      assert (zmsg_size(output) == 2);

      str = zmsg_popstr(output);
      assert (streq(str, Fix_json_built_0));
      free(str);

      str = zmsg_popstr(output);
      assert (streq(str, Fix_json_built_1));
      free(str);

      zmsg_destroy(&output);
      zhash_destroy(&throughput);
      printf ("OK\n");
    }
    
    return 0;
}

#endif
