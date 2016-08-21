/**
 * =====================================================================================
 *
 *   @file failure_agent.c
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
#include "utils.h"
#include "config.h"
#include "failure_agent.h"
#include "superfasthash.h"

#include <glob.h>

#define DEFAULT_FAILURE_SAVE_PATH "/tmp/failure"

file_record* s_file_record_new(char *filename, size_t len, uint64_t retry_time, bool in_memory)
{
  assert(filename);

  file_record *record = malloc(sizeof(file_record));
  record->filename = strdup(filename);
  record->n_retries = 0;
  record->rawsize = len;
  record->next_retry = now() + retry_time;
  record->in_memory = in_memory;
  return record;
}

void s_file_record_destroy(file_record* record)
{
  if (record != NULL) {
    mem_free(record->filename);
    record->n_retries = 0;
    record->next_retry = 0;
    record->rawsize = 0;
    mem_free(record);
  }
}

char *s_generate_id(time_t seconds, const char *data) 
{
  char id[MAX_STRING_LEN];
  memset(id, 0, MAX_STRING_LEN);
  char *hash = StrSuperFastHash((const uint8_t*)data, strlen(data), 0);
  snprintf(id, MAX_STRING_LEN, "%lld-%s", (long long)seconds, hash);
  free(hash);
  return strdup(id);
}

char *s_generate_filename(const char *basepath, const char *id) 
{
  char filename[MAX_STRING_LEN];
  memset(filename, 0, MAX_STRING_LEN);
  if (basepath) {
    utils_recursive_mkdir(basepath);
    snprintf(filename, MAX_STRING_LEN, "%s/%s.dat.gz", basepath, id);
  } else { 
    snprintf(filename, MAX_STRING_LEN, "%s.dat.gz", id);
  }
  return strdup(filename);
}

#ifndef E2E_TESTING
void failure_agent (void *user_args, zctx_t *ctx, void *pipe)
{

  /*  Read config */
  config_context *cfg_ctx = config_new();
  char *persistpath = config_get_str(cfg_ctx, "hermes.failure.persistpath");
  char *memorypath = config_get_str(cfg_ctx, "hermes.failure.memorypath");
  if (memorypath == NULL) memorypath = DEFAULT_FAILURE_SAVE_PATH;
  int max_retry = config_get_int(cfg_ctx, "hermes.failure.max_retry");
  uint64_t retry_time = (uint64_t)(config_get_int(cfg_ctx, "hermes.failure.retry_time") * 1000);
  uint64_t nonvolatile_retry = 60 * 1000;
  config_destroy(cfg_ctx);

  zhash_t *file_table = zhash_new();
  uint64_t next_nonvolatile_retry = now() + nonvolatile_retry;

  /*  Fill up the hash table if previously existing chunks */
  {
    glob_t myglob;
    char pattern[MAX_STRING_LEN];
    char *vfat_path = utils_get_vfat_mountpoint();
    if (vfat_path != NULL) {
      int i;
      bzero(pattern, MAX_STRING_LEN);
      snprintf(pattern, MAX_STRING_LEN, "%s/*.dat.gz", vfat_path);
      mem_free(vfat_path);
      if (glob((const char*)pattern, 0, NULL, &myglob) == 0) {
        for(i=0; i<myglob.gl_pathc; i++) {
          char *filename = myglob.gl_pathv[i];
          char *base_name = basename(filename);
          char *data = malloc(16*1024);
          bzero(data, 16*1024);
          int len = utils_gzip_read(filename, data, 16*1024);
          mem_free(data);
          file_record *record = s_file_record_new(filename, len, retry_time, false);
          zhash_insert(file_table, base_name, record);
        }
      }
      if (myglob.gl_pathc > 0) globfree(&myglob);
    }
  }

  while (!zctx_interrupted) {
    if (zsocket_poll(pipe, 0)) {
      zmsg_t* message = zmsg_recv (pipe);
      if (!message) continue;

#else
void failure_agent (zmsg_t* message, char* persistpath, char* memorypath, zhash_t *file_table)
{
  if (message) {
#endif
      
      assert (zmsg_size(message) == 2);

      char *reason = zmsg_popstr(message);
      if (streq(reason, MSG_POST)) {
        char *data = zmsg_popstr(message);
        char *msg_id = s_generate_id(time(NULL), data);
        void *exists = zhash_lookup(file_table, msg_id);
        if (exists == NULL) {
          debugLog("Inserting msgid %s in failure table", msg_id);
          char *file_name = NULL;
          char *vfat_path = NULL;
          bool written_in_memory = false;

          // If persistpath exists then use it (override)
          // If not, look for a vfat partition and try to write. 
          // If write failed, write to mem and send the memfile to persistency agent.
          if (persistpath != NULL) {
            file_name = s_generate_filename(persistpath, msg_id);
          } else {
            vfat_path = utils_get_vfat_mountpoint();
            if (vfat_path == NULL) {
              file_name = s_generate_filename(memorypath, msg_id);
              written_in_memory = true;
            } else {
              file_name = s_generate_filename(vfat_path, msg_id);
              mem_free(vfat_path);
            }
          }

          int ret = utils_gzip_write(file_name, data, strlen(data));
          if (ret == STATUS_ERROR && !written_in_memory) {
            file_name = s_generate_filename(memorypath, msg_id);
            written_in_memory = true;
            utils_gzip_write(file_name, data, strlen(data));
          }

          file_record *record = s_file_record_new(file_name, strlen(data), retry_time, written_in_memory);
          zhash_insert(file_table, msg_id, record);
          mem_free(file_name);
        } else {
          debugLog("Msg of id %s was already in failure table", msg_id);
        }
        mem_free(msg_id);
        mem_free(data);
      } else if streq(reason, MSG_SUCCESS) {
        char *msg_id = zmsg_popstr(message);
        debugLog("Retry(http, ftp, ...) of msgid %s succeeded,", msg_id);
        file_record *record = zhash_lookup(file_table, msg_id);
        if (record != NULL) {
          utils_delete_file(record->filename);
          s_file_record_destroy(record);
        }
        zhash_delete(file_table, msg_id);
        mem_free(msg_id);
      } else if streq(reason, MSG_ERROR) {
        char *msg_id = zmsg_popstr(message);
        file_record *record = zhash_lookup(file_table, msg_id);
        if (record != NULL) {
          record->n_retries++;
          debugLog("Retry(http, ftp, ...) of msgid %s failed [%d retries],", msg_id, record->n_retries);
        }
        mem_free(msg_id);
      }

      mem_free(reason);
      agent_msg_destroy(&message);

#ifndef E2E_TESTING
    } // zsocket_poll
    usleep(HERMES_THREAD_SLEEP*1000);
#else
    } // if message
#endif

  // Try resending over HTTP / FTP / satan ...
  zlist_t *keys = zhash_keys(file_table);
  char *key = zlist_pop(keys);
  while (key != NULL) {
    file_record *record = zhash_lookup(file_table, key);
    assert (record != NULL);
    if (now() > record->next_retry) {
      debugLog("Age exceeded for record '%s': retrying.", key);
      if (record->n_retries > max_retry) {
        debugLog("Too many retries: FTP");
        // FTP or Satan fallbacks
        if (ftp_agent_pipe != NULL) {
          zmsg_t *ftpmsg = zmsg_new();
          zmsg_addstr(ftpmsg, MSG_POST_FILE);
          zmsg_addstr(ftpmsg, key);
          zmsg_addstr(ftpmsg, record->filename);
          agent_msg_send(&ftpmsg, ftp_agent_pipe);
        }
        record->n_retries = 0;
      } else {
        debugLog("Sending again to HTTP agent.");
        char *data = malloc(record->rawsize + 2); // eventual EOF and NULL terminator
        memset(data, 0, record->rawsize + 2);
        utils_gzip_read(record->filename, data, record->rawsize+2);
        zmsg_t *repost = zmsg_new();
        zmsg_addstr(repost, MSG_POST_ID);
        zmsg_addstr(repost, key);
        zmsg_addstr(repost, data);
        agent_msg_send(&repost, http_agent_pipe);
        mem_free(data);
      }
      record->next_retry = now() + retry_time;
    }
    mem_free(key);
    key = zlist_pop(keys);
  }
  zlist_destroy(&keys);


  // Try to write to nonvolatile partition
  if (now() > next_nonvolatile_retry) {
    /*  Get the vfat mountpoint again */
    char *vfat_path = utils_get_vfat_mountpoint();
    if (vfat_path != NULL) {
      zlist_t *keys = zhash_keys(file_table);
      char *key = zlist_pop(keys);
      while (key != NULL) {
        file_record *record = zhash_lookup(file_table, key);
        if (record->in_memory) {
          char new_filename[MAX_STRING_LEN];
          char *base_name = basename(record->filename);
          bzero(new_filename, sizeof(new_filename));
          snprintf(new_filename, MAX_STRING_LEN, "%s/%s", vfat_path, base_name);
          int ret = utils_copy_file(record->filename, new_filename);
          if (ret == STATUS_OK) {
            utils_delete_file(record->filename);
            mem_free(record->filename);
            record->filename = strdup(new_filename);
          }
        }
        mem_free(key);
        key = zlist_next(keys);
      }
      mem_free(vfat_path);
    }
    next_nonvolatile_retry = now() + nonvolatile_retry;
  }
  

#ifndef E2E_TESTING
  } // while(!zctx_interrupted)

  zhash_destroy(&file_table);
  mem_free(memorypath);
  mem_free(persistpath);
#endif
}




#ifdef TESTING

/* Test */
int failure_agent_selftest(bool verbose)
{
    printf (" * failure_agent (s_generate_id) : ");
    { 
      char expected[MAX_STRING_LEN];
      time_t seconds = time(NULL);
      char *msg_id = s_generate_id(seconds, Fix_json_aggreg);
      snprintf(expected, MAX_STRING_LEN, "%lld-%s", (long long)seconds, "b462b253"); // hardcoded id of the hashed fixture
      assert (streq(expected, msg_id));
      free(msg_id);
      printf("OK\n");
    }

    printf (" * failure_agent (s_generate_filename) : ");
    { 
      char expected[MAX_STRING_LEN];
      char *filename = s_generate_filename(Fix_memorypath, Fix_file_id_0);
      snprintf(expected, MAX_STRING_LEN, "%s/%s.dat.gz", Fix_memorypath, Fix_file_id_0);
      assert (streq(expected, filename));
      free(filename);

      filename = s_generate_filename(NULL, Fix_file_id_0);
      snprintf(expected, MAX_STRING_LEN, "%s.dat.gz", Fix_file_id_0);
      assert (streq(expected, filename));
      free(filename);
      printf("OK\n");
    }
    
    printf (" * failure_agent (s_record_new/s_record_destroy) : ");
    { 
      zhash_t *file_table = zhash_new();
      uint64_t tstart = now();
      file_record *record = s_file_record_new(Fix_file_name_0, strlen(Fix_json_hash_2), 1000, true);
      assert (streq(record->filename, Fix_file_name_0));
      /* Make sure the string has been duplicated */
      assert ((char*)record->filename != (char*)Fix_file_name_0);
      assert (record->n_retries == 0);
      assert (record->next_retry >= tstart + 1000);

      zhash_insert(file_table, Fix_file_id_0, record);
      zhash_insert(file_table, Fix_file_id_0, record);
      zhash_insert(file_table, Fix_file_id_0, record);

      file_record *other = zhash_lookup(file_table, Fix_file_id_0);
      assert (other != NULL);
      assert (other == record);
      
      s_file_record_destroy(other);
      printf("OK\n");
    }
    return 0;
}

#endif
