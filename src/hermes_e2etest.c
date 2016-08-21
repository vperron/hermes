/**
 * =====================================================================================
 *
 *   @file hermes_e2etest.c
 *
 *   
 *        Version:  1.0
 *        Created:  04/09/2013 11:07:44 AM
 *
 *
 *   @section DESCRIPTION
 *
 *       E2E tests to test agents.
 *       
 *   @section LICENSE
 *
 *       
 *
 * =====================================================================================
 */

#include "main.h"
#include "fixtures.h"
#include "czmq.h"
#include "sops.h"
#include "config.h"
#include "utils.h"

#include "http_agent.h"
#include "hashtable_agent.h"
#include "failure_agent.h"

#include "fff.h"
DEFINE_FFF_GLOBALS;

/*  global values  */
void *ftp_agent_pipe = (void*)1;
void *http_agent_pipe = (void*)2;
void *failure_agent_pipe = (void*)3;
void *hashtable_agent_pipe = (void*)4;
void *persist_agent_pipe = (void*)5;

uint64_t retry_time;
uint64_t nonvolatile_retry;
uint64_t next_nonvolatile_retry;
uint64_t creation_time;
int max_retry;

/*  Base functions redefinition  */
FAKE_VOID_FUNC(agent_msg_send, zmsg_t**, void*);
FAKE_VOID_FUNC(agent_msg_destroy, zmsg_t**);
FAKE_VOID_FUNC(mem_free, void*);

/*  Utils functions redefinition  */
FAKE_VOID_FUNC(utils_send_success, const char *);
FAKE_VOID_FUNC(utils_send_failure, const char *);
FAKE_VOID_FUNC(utils_recursive_mkdir, const char *);
FAKE_VALUE_FUNC(int, utils_gzip_write, const char*, char*, size_t);
FAKE_VALUE_FUNC(int, utils_gzip_read, const char*, char*, size_t);
FAKE_VALUE_FUNC(int, utils_delete_file, const char *);
FAKE_VALUE_FUNC(int, utils_copy_file, const char *, const char *);
FAKE_VALUE_FUNC(char*, utils_get_vfat_mountpoint);
FAKE_VALUE_FUNC(char*, utils_generate_json, const char*, const char*, const char*, const char*, const char*, const char*);

/*  config.h fake functions  */
FAKE_VALUE_FUNC(config_context*, config_new);
FAKE_VOID_FUNC(config_destroy, config_context*);
FAKE_VALUE_FUNC(char*, config_get_str, config_context*, char*);
FAKE_VALUE_FUNC(int, config_get_int, config_context*, char*);
FAKE_VALUE_FUNC(bool, config_get_bool, config_context*, char*);
FAKE_VALUE_FUNC(double, config_get_double, config_context*, char*);
FAKE_VALUE_FUNC(int, config_set, config_context*, char*);
FAKE_VALUE_FUNC(int, config_commit, config_context*, char*);

/*  sops.h fake functions */
FAKE_VALUE_FUNC(sops_handle_t*, sops_new_user_password, const char*, const char*, const char*, const char*, int);
FAKE_VALUE_FUNC(sops_handle_t*, sops_new_token, const char*, const char*, const char*, int);
FAKE_VOID_FUNC(sops_destroy, sops_handle_t*);
FAKE_VALUE_FUNC(int, sops_post_datastream, sops_handle_t*, const char*, const char*, size_t);
FAKE_VOID_FUNC(sops_compress_requests, sops_handle_t*, bool);

// Reset the counters
void s_global_reset() 
{
  ftp_agent_pipe = (void*)1;
  http_agent_pipe = (void*)2;
  failure_agent_pipe = (void*)3;
  hashtable_agent_pipe = (void*)4;
  persist_agent_pipe = (void*)5;

  creation_time = now();
  max_retry = 10;
  retry_time = 60 * 1000;
  nonvolatile_retry = 60 * 1000;
  nonvolatile_retry = now() + 60 * 1000;

  RESET_FAKE(agent_msg_send);
  RESET_FAKE(agent_msg_destroy);
  RESET_FAKE(utils_send_success);
  RESET_FAKE(utils_send_failure);
  RESET_FAKE(utils_recursive_mkdir);
  RESET_FAKE(utils_gzip_write);
  RESET_FAKE(utils_gzip_read);
  RESET_FAKE(utils_delete_file);
  RESET_FAKE(utils_copy_file);
  RESET_FAKE(utils_get_vfat_mountpoint);
  RESET_FAKE(mem_free);
  RESET_FAKE(config_new);
  RESET_FAKE(config_destroy);
  RESET_FAKE(config_get_int);
  RESET_FAKE(config_get_str);
  RESET_FAKE(config_get_bool);
  RESET_FAKE(config_get_double);
  RESET_FAKE(config_set);
  RESET_FAKE(config_commit);
  RESET_FAKE(sops_new_user_password);
  RESET_FAKE(sops_new_token);
  RESET_FAKE(sops_destroy);
  RESET_FAKE(sops_post_datastream);
}


int main (int argc, char *argv [])
{
  printf ("\nRunning hermes e2e tests...\n");

  zctx_t *zmq_ctx = zctx_new();

  printf (" * HTTP Agent: POST_ID, successful: ");
  {
    s_global_reset();
    sops_new_token_fake.return_val = (sops_handle_t*)FAKE_PTR_VAL_0;
    sops_new_user_password_fake.return_val = (sops_handle_t*)FAKE_PTR_VAL_0;
    sops_post_datastream_fake.return_val = STATUS_OK;

    zmsg_t* msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST_ID);
    zmsg_addstr(msg, Fix_hash1);
    zmsg_addstr(msg, Fix_json_aggreg);

    http_agent(msg);

    assert (sops_post_datastream_fake.call_count == 1);
    assert (sops_post_datastream_fake.arg0_history[0] == (sops_handle_t*)FAKE_PTR_VAL_0);
    assert (streq(sops_post_datastream_fake.arg2_history[0], Fix_json_aggreg));

    /*  Check that a SUCCESS message has been sent with right ID to failure agent */
    assert (utils_send_success_fake.call_count == 1);
    assert (streq(utils_send_success_fake.arg0_history[0], Fix_hash1));
    assert (agent_msg_destroy_fake.call_count == 1);

    printf ("OK\n");
  }

  printf (" * HTTP Agent: POST_ID, successful (NULL failure agent): ");
  {
    s_global_reset();
    failure_agent_pipe = NULL;
    sops_new_token_fake.return_val = (sops_handle_t*)FAKE_PTR_VAL_0;
    sops_new_user_password_fake.return_val = (sops_handle_t*)FAKE_PTR_VAL_0;
    sops_post_datastream_fake.return_val = STATUS_OK;

    zmsg_t* msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST_ID);
    zmsg_addstr(msg, Fix_hash1);
    zmsg_addstr(msg, Fix_json_aggreg);

    http_agent(msg);

    assert (sops_post_datastream_fake.call_count == 1);
    assert (sops_post_datastream_fake.arg0_history[0] == (sops_handle_t*)FAKE_PTR_VAL_0);
    assert (streq(sops_post_datastream_fake.arg2_history[0], Fix_json_aggreg));

    assert (utils_send_success_fake.call_count == 1);
    assert (streq(utils_send_success_fake.arg0_history[0], Fix_hash1));
    assert (agent_msg_send_fake.call_count == 0);
    assert (agent_msg_destroy_fake.call_count == 1);
    printf ("OK\n");
  }

  printf (" * HTTP Agent: POST_ID, unsuccessful: ");
  {
    s_global_reset();
    sops_new_token_fake.return_val = (sops_handle_t*)FAKE_PTR_VAL_0;
    sops_new_user_password_fake.return_val = (sops_handle_t*)FAKE_PTR_VAL_0;
    sops_post_datastream_fake.return_val = STATUS_ERROR;

    zmsg_t* msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST_ID);
    zmsg_addstr(msg, Fix_hash1);
    zmsg_addstr(msg, Fix_json_aggreg);

    http_agent(msg);

    assert (sops_post_datastream_fake.call_count == 1);
    assert (sops_post_datastream_fake.arg0_history[0] == (sops_handle_t*)FAKE_PTR_VAL_0);
    assert (streq(sops_post_datastream_fake.arg2_history[0], Fix_json_aggreg));

    /*  Check that an ERROR message has been sent with right ID to failure agent */
    assert (utils_send_failure_fake.call_count == 1);
    assert (streq(utils_send_failure_fake.arg0_history[0], Fix_hash1));
    assert (agent_msg_destroy_fake.call_count == 1);

    printf ("OK\n");
  }

  printf (" * HTTP Agent: POST_ID, unsuccessful (NULL failure agent): ");
  {
    s_global_reset();
    failure_agent_pipe = NULL;
    sops_new_token_fake.return_val = (sops_handle_t*)FAKE_PTR_VAL_0;
    sops_new_user_password_fake.return_val = (sops_handle_t*)FAKE_PTR_VAL_0;
    sops_post_datastream_fake.return_val = STATUS_ERROR;

    zmsg_t* msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST_ID);
    zmsg_addstr(msg, Fix_hash1);
    zmsg_addstr(msg, Fix_json_aggreg);

    http_agent(msg);

    assert (sops_post_datastream_fake.call_count == 1);
    assert (sops_post_datastream_fake.arg0_history[0] == (sops_handle_t*)FAKE_PTR_VAL_0);
    assert (streq(sops_post_datastream_fake.arg2_history[0], Fix_json_aggreg));

    assert (agent_msg_send_fake.call_count == 0);
    assert (agent_msg_destroy_fake.call_count == 1);
    printf ("OK\n");
  }

  printf (" * HTTP Agent: POST, successful: ");
  {
    s_global_reset();
    sops_new_token_fake.return_val = (sops_handle_t*)FAKE_PTR_VAL_0;
    sops_new_user_password_fake.return_val = (sops_handle_t*)FAKE_PTR_VAL_0;
    sops_post_datastream_fake.return_val = STATUS_OK;

    zmsg_t* msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST);
    zmsg_addstr(msg, Fix_json1);
    zmsg_addstr(msg, Fix_json2);
    zmsg_addstr(msg, Fix_json3);

    http_agent(msg);

    assert (sops_post_datastream_fake.call_count == 1);
    assert (sops_post_datastream_fake.arg0_history[0] == (sops_handle_t*)FAKE_PTR_VAL_0);
    /*  Check that JSON values have been correctly aggregated */
    assert (streq(sops_post_datastream_fake.arg2_history[0], Fix_json_aggreg));

    /*  Check that no POST message has been sent to failure agent */
    assert (agent_msg_send_fake.call_count == 0);
    assert (agent_msg_destroy_fake.call_count == 1);
    printf ("OK\n");
  }

  printf (" * HTTP Agent: POST, unsuccessful: ");
  {
    s_global_reset();
    sops_new_token_fake.return_val = (sops_handle_t*)FAKE_PTR_VAL_0;
    sops_new_user_password_fake.return_val = (sops_handle_t*)FAKE_PTR_VAL_0;
    sops_post_datastream_fake.return_val = STATUS_ERROR;

    zmsg_t* msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST);
    zmsg_addstr(msg, Fix_json1);
    zmsg_addstr(msg, Fix_json2);
    zmsg_addstr(msg, Fix_json3);

    http_agent(msg);

    assert (sops_post_datastream_fake.call_count == 1);
    assert (sops_post_datastream_fake.arg0_history[0] == (sops_handle_t*)FAKE_PTR_VAL_0);
    /*  Check that JSON values have been correctly aggregated */
    assert (streq(sops_post_datastream_fake.arg2_history[0], Fix_json_aggreg));

    /*  Check that a POST message has been sent to failure agent */
    assert (agent_msg_send_fake.call_count == 1);
    assert (agent_msg_send_fake.arg1_history[0] == (void*)failure_agent_pipe);
    zmsg_t* status = *agent_msg_send_fake.arg0_history[0];
    char* type = zmsg_popstr(status);
    char* str = zmsg_popstr(status);
    assert (streq(type, MSG_POST));
    assert (streq(str, Fix_json_aggreg));
    assert (agent_msg_destroy_fake.call_count == 1);

    printf ("OK\n");
  }

  printf (" * HTTP Agent: POST, unsuccessful (NULL failure agent): ");
  {
    s_global_reset();
    failure_agent_pipe = NULL;
    sops_new_token_fake.return_val = (sops_handle_t*)FAKE_PTR_VAL_0;
    sops_new_user_password_fake.return_val = (sops_handle_t*)FAKE_PTR_VAL_0;
    sops_post_datastream_fake.return_val = STATUS_ERROR;

    zmsg_t* msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST);
    zmsg_addstr(msg, Fix_json1);
    zmsg_addstr(msg, Fix_json2);
    zmsg_addstr(msg, Fix_json3);

    http_agent(msg);

    assert (sops_post_datastream_fake.call_count == 1);
    assert (sops_post_datastream_fake.arg0_history[0] == (sops_handle_t*)FAKE_PTR_VAL_0);
    /*  Check that JSON values have been correctly aggregated */
    assert (streq(sops_post_datastream_fake.arg2_history[0], Fix_json_aggreg));

    assert (agent_msg_send_fake.call_count == 0);
    assert (agent_msg_destroy_fake.call_count == 2);
    printf ("OK\n");
  }

  printf (" * Hashtable Agent: POST_HASH is throttled: ");
  {
    s_global_reset();
    zmsg_t* msg = NULL;
    zlist_t *beacon_list = zlist_new();

    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST_HASH);
    zmsg_addstr(msg, Fix_json_built_0);
    hashtable_agent(msg, beacon_list, 5, 60*1000, 0);
    zmsg_destroy(&msg);
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST_HASH);
    zmsg_addstr(msg, Fix_json_built_0);
    hashtable_agent(msg, beacon_list, 5, 60*1000, 0);
    zmsg_destroy(&msg);
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST_HASH);
    zmsg_addstr(msg, Fix_json_built_0);
    hashtable_agent(msg, beacon_list, 5, 60*1000, 0);

    assert (zlist_size(beacon_list) == 3);

    zmsg_destroy(&msg);
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST_HASH);
    zmsg_addstr(msg, Fix_json_built_1);
    hashtable_agent(msg, beacon_list, 5, 60*1000, 0);

    assert (zlist_size(beacon_list) == 4);

    printf ("OK\n");
  }

  printf (" * Hashtable Agent: flush when maxsize reached: ");
  {
    s_global_reset();
    zmsg_t* msg = NULL;
    zlist_t *beacon_list = zlist_new();

    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST_HASH);
    zmsg_addstr(msg, Fix_json_built_0);
    hashtable_agent(msg, beacon_list, 3, 60*1000, 0);

    assert (agent_msg_send_fake.call_count == 0);

    zmsg_destroy(&msg);
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST_HASH);
    zmsg_addstr(msg, Fix_json_built_1);
    hashtable_agent(msg, beacon_list, 3, 60*1000, 0);

    assert (agent_msg_send_fake.call_count == 0);

    zmsg_destroy(&msg);
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST_HASH);
    zmsg_addstr(msg, Fix_json_built_2);
    hashtable_agent(msg, beacon_list, 3, 60*1000, 0);

    assert (agent_msg_send_fake.call_count == 1);

    assert (agent_msg_send_fake.arg1_history[0] == (void*)http_agent_pipe);
    zmsg_t* status = *agent_msg_send_fake.arg0_history[0];
    char* type = zmsg_popstr(status);
    char* msg1 = zmsg_popstr(status);
    char* msg2 = zmsg_popstr(status);
    char* msg3 = zmsg_popstr(status);
    assert (zmsg_size(status) == 0);
    assert (streq(type, MSG_POST));
    /*  WARNING: this part DEPENDS on the internal order inside a zlist_t structure. */
    assert (streq(msg1, Fix_json_built_0));
    assert (streq(msg2, Fix_json_built_1));
    assert (streq(msg3, Fix_json_built_2));

    printf ("OK\n");
  }

  printf (" * Hashtable Agent: flush when maxsize reached (HTTP Agent null): ");
  {
    s_global_reset();
    http_agent_pipe = NULL;
    zmsg_t* msg = NULL;
    zlist_t *beacon_list = zlist_new();

    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST_HASH);
    zmsg_addstr(msg, Fix_json_built_0);
    hashtable_agent(msg, beacon_list, 3, 60*1000, 0);

    assert (agent_msg_send_fake.call_count == 0);

    zmsg_destroy(&msg);
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST_HASH);
    zmsg_addstr(msg, Fix_json_built_0);
    hashtable_agent(msg, beacon_list, 3, 60*1000, 0);

    assert (agent_msg_send_fake.call_count == 0);

    zmsg_destroy(&msg);
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST_HASH);
    zmsg_addstr(msg, Fix_json_built_1);
    hashtable_agent(msg, beacon_list, 3, 60*1000, 0);

    assert (agent_msg_send_fake.call_count == 0);

    zmsg_destroy(&msg);
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST_HASH);
    zmsg_addstr(msg, Fix_json_built_2);
    hashtable_agent(msg, beacon_list, 3, 60*1000, 0);

    assert (agent_msg_send_fake.call_count == 0);
    printf ("OK\n");
  }

  printf (" * Hashtable Agent: flush when maxtime reached: ");
  {
    s_global_reset();
    zmsg_t* msg = NULL;
    zlist_t *beacon_list = zlist_new();

    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST_HASH);
    zmsg_addstr(msg, Fix_json_built_0);
    creation_time -= 25 * 1000; // minus 25 seconds

    hashtable_agent(msg, beacon_list, 10, 60*1000, 0);
    assert (agent_msg_send_fake.call_count == 0);

    zmsg_destroy(&msg);
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST_HASH);
    zmsg_addstr(msg, Fix_json_built_0);
    creation_time -= 25 * 1000; // minus 25 seconds

    hashtable_agent(msg, beacon_list, 10, 60*1000, 0);
    assert (agent_msg_send_fake.call_count == 0);

    zmsg_destroy(&msg);
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST_HASH);
    zmsg_addstr(msg, Fix_json_built_1);
    creation_time -= 25 * 1000; // minus 25 seconds

    hashtable_agent(msg, beacon_list, 10, 60*1000, 0);
    assert (agent_msg_send_fake.call_count == 1);

    /*  Hashtable has been flushed */
    assert (zlist_size(beacon_list) == 0);

    assert (agent_msg_send_fake.arg1_history[0] == (void*)http_agent_pipe);
    zmsg_t* status = *agent_msg_send_fake.arg0_history[0];
    char* type = zmsg_popstr(status);
    char* msg1 = zmsg_popstr(status);
    char* msg2 = zmsg_popstr(status);
    char* msg3 = zmsg_popstr(status);
    assert (zmsg_size(status) == 0);
    assert (streq(type, MSG_POST));
    /*  WARNING: this part DEPENDS on the internal order inside a zlist_t structure. */
    assert (streq(msg1, Fix_json_built_0));
    assert (streq(msg2, Fix_json_built_0));
    assert (streq(msg3, Fix_json_built_1));

    printf ("OK\n");
  }

  printf (" * Failure Agent (hashtable filling and deletion): ");
  {
    s_global_reset();
    zmsg_t* msg = NULL;
    zhash_t *file_table = zhash_new();

    // Send a block id as a failure, added to the table
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_ERROR);
    zmsg_addstr(msg, Fix_file_id_0);
    failure_agent(msg, NULL, Fix_memorypath, file_table);
    zmsg_destroy(&msg);

    // Send a success message for this id, verify that delete file is called
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_SUCCESS);
    zmsg_addstr(msg, Fix_file_id_0);
    failure_agent(msg, NULL, Fix_memorypath, file_table);
    assert (utils_delete_file_fake.call_count == 0);
    zmsg_destroy(&msg);

    // Post a chunk of data to failure agent, verify it is written somewhere
    // and added to the hash table
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST);
    zmsg_addstr(msg, Fix_json_aggreg);
    failure_agent(msg, NULL, Fix_memorypath, file_table);
    assert (utils_gzip_write_fake.call_count == 1);
    assert (zhash_size(file_table) == 1);
    zmsg_destroy(&msg);

    // Send the same chunk again, the agent should not rewrite nor create a new 
    // entry in the hash table
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST);
    zmsg_addstr(msg, Fix_json_aggreg);
    failure_agent(msg, NULL, Fix_memorypath, file_table);
    assert (utils_gzip_write_fake.call_count == 1);
    assert (zhash_size(file_table) == 1);
    zmsg_destroy(&msg);

    // Check that the entry in the hash table is correct
    zlist_t* keys = zhash_keys(file_table);
    char *key = zlist_first(keys);
    file_record *record = zhash_lookup(file_table, key);
    assert (record != NULL);
    assert (streq(record->filename,utils_gzip_write_fake.arg0_history[0]));
    assert (record->n_retries == 0);
    char *filename = strdup(record->filename);

    // Send a few retry errors about this same file, check number of retries
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_ERROR);
    zmsg_addstr(msg, key);
    failure_agent(msg, NULL, Fix_memorypath, file_table);
    zmsg_destroy(&msg);
    
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_ERROR);
    zmsg_addstr(msg, key);
    failure_agent(msg, NULL, Fix_memorypath, file_table);
    zmsg_destroy(&msg);

    assert (record->n_retries == 2);

    // Then finally send a success status, check file destroyed properly
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_SUCCESS);
    zmsg_addstr(msg, key);
    failure_agent(msg, NULL, Fix_memorypath, file_table);
    assert (utils_delete_file_fake.call_count == 1);
    assert (streq(utils_delete_file_fake.arg0_history[0], filename));
    zmsg_destroy(&msg);
    printf ("OK\n");
  }

  printf (" * Failure Agent (retry / fallback logic, TAKES TIME): ");
  {
    s_global_reset();

    // Set retry timeout after 1 second and maximum 3 retries
    retry_time = 1000;
    max_retry = 3;

    zmsg_t* msg = NULL;
    zhash_t *file_table = zhash_new();

    // Send a few json chunks of data quickly, should not be a retry inbetween
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST);
    zmsg_addstr(msg, Fix_json_aggreg);
    failure_agent(msg, NULL, Fix_memorypath, file_table);

    assert (agent_msg_send_fake.call_count == 0);
    usleep(400 * 1000); // Sleep 400 ms approx

    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST);
    zmsg_addstr(msg, Fix_json_aggreg_1);
    failure_agent(msg, NULL, Fix_memorypath, file_table);

    assert (agent_msg_send_fake.call_count == 0);
    usleep(400 * 1000); // Sleep 400 ms approx

    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST);
    zmsg_addstr(msg, Fix_json_aggreg_2);
    failure_agent(msg, NULL, Fix_memorypath, file_table);

    assert (agent_msg_send_fake.call_count == 0);
    usleep(400 * 1000); // Sleep 400 ms approx

    // Now after 1200 ms, we should have witnessed a retry
    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST);
    zmsg_addstr(msg, Fix_json_aggreg_2);
    failure_agent(msg, NULL, Fix_memorypath, file_table);
    assert (agent_msg_send_fake.call_count == 1);
    assert (utils_gzip_read_fake.call_count == 1);
    assert (agent_msg_send_fake.arg1_history[0] == (void*)http_agent_pipe);
    zmsg_t* status = *agent_msg_send_fake.arg0_history[0];
    char* type = zmsg_popstr(status);
    assert (zmsg_size(status) == 2);
    assert (streq(type, MSG_POST_ID));

    printf ("OK\n");
  }

  printf (" * Failure Agent (fallback only, TAKES TIME): ");
  {
    s_global_reset();

    retry_time = 200; // 200ms
    max_retry = 3;

    // Check that after too many retries, the ftp agent is called as fallback
    zhash_t *file_table = zhash_new();
    zmsg_t *msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST);
    zmsg_addstr(msg, Fix_json_aggreg_2);
    failure_agent(msg, NULL, Fix_memorypath, file_table);

    usleep(100 * 1000);
    failure_agent(NULL, NULL, Fix_memorypath, file_table);
    assert (agent_msg_send_fake.call_count == 0);
    assert (utils_gzip_read_fake.call_count == 0);
    
    usleep(200 * 1000);
    failure_agent(NULL, NULL, Fix_memorypath, file_table);
    assert (agent_msg_send_fake.call_count == 1);
    assert (agent_msg_send_fake.arg1_history[0] == (void*)http_agent_pipe);
    assert (utils_gzip_read_fake.call_count == 1);

    zlist_t* keys = zhash_keys(file_table);
    char *key = zlist_first(keys);
    file_record *record = zhash_lookup(file_table, key);

    msg = zmsg_new();
    zmsg_addstr(msg, MSG_ERROR);
    zmsg_addstr(msg, key);
    failure_agent(msg, NULL, Fix_memorypath, file_table);
    assert (agent_msg_send_fake.call_count == 1);

    msg = zmsg_new();
    zmsg_addstr(msg, MSG_ERROR);
    zmsg_addstr(msg, key);
    failure_agent(msg, NULL, Fix_memorypath, file_table);
    assert (agent_msg_send_fake.call_count == 1);

    msg = zmsg_new();
    zmsg_addstr(msg, MSG_ERROR);
    zmsg_addstr(msg, key);
    failure_agent(msg, NULL, Fix_memorypath, file_table);
    assert (agent_msg_send_fake.call_count == 1);

    msg = zmsg_new();
    zmsg_addstr(msg, MSG_ERROR);
    zmsg_addstr(msg, key);
    failure_agent(msg, NULL, Fix_memorypath, file_table);
    assert (agent_msg_send_fake.call_count == 1);

    usleep(300 * 1000);
    failure_agent(NULL, NULL, Fix_memorypath, file_table);
    assert (agent_msg_send_fake.call_count == 2);
    assert (agent_msg_send_fake.arg1_history[1] == (void*)ftp_agent_pipe);
    assert (utils_gzip_read_fake.call_count == 1);


    printf ("OK\n");
  }

  printf (" * Failure Agent (memory/permanent persistency logic): ");
  {
    s_global_reset();
    zmsg_t* msg = NULL;
    zhash_t *file_table = zhash_new();

    // Post a chunk of data with no persistent file path, should find the vfat entry
    utils_get_vfat_mountpoint_fake.return_val = "/mnt/vfat";

    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST);
    zmsg_addstr(msg, Fix_json_aggreg);
    failure_agent(msg, NULL, Fix_memorypath, file_table);
    assert (utils_gzip_write_fake.call_count == 1);
    char *filename = (char*)utils_gzip_write_fake.arg0_history[0];
    assert (strncmp(filename, "/mnt/vfat", 9) == 0);
    assert (zhash_size(file_table) == 1);
    zlist_t* keys = zhash_keys(file_table);
    char *key = zlist_first(keys);
    file_record *record = zhash_lookup(file_table, key);
    assert (strncmp(record->filename, "/mnt/vfat", 9) == 0);
    
    // Post a chunk of data with no persistent file path, should find the vfat entry
    // This time the vfat is there, but the file could not be written
    s_global_reset();
    utils_get_vfat_mountpoint_fake.return_val = "/mnt/vfat";
    utils_gzip_write_fake.return_val = STATUS_ERROR;

    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST);
    zmsg_addstr(msg, Fix_json_aggreg_1);
    failure_agent(msg, NULL, Fix_memorypath, file_table);
    // 1 write to the vfat path, then 1 write to the memory path, then send to persist agent
    assert (utils_gzip_write_fake.call_count == 2);
    filename = (char*)utils_gzip_write_fake.arg0_history[0];
    assert (strncmp(filename, "/mnt/vfat", 9) == 0);
    filename = (char*)utils_gzip_write_fake.arg0_history[1];
    assert (strncmp(filename, Fix_memorypath, strlen(Fix_memorypath) - 1) == 0);

    // Post a chunk of data with no persistent file path, no vfat
    s_global_reset();

    nonvolatile_retry = 300; // 300ms
    next_nonvolatile_retry = now() + 300;
    zhash_destroy(&file_table);
    file_table = zhash_new();

    utils_get_vfat_mountpoint_fake.return_val = NULL;
    utils_gzip_write_fake.return_val = STATUS_ERROR;

    msg = zmsg_new();
    zmsg_addstr(msg, MSG_POST);
    zmsg_addstr(msg, Fix_json_aggreg_2);
    failure_agent(msg, NULL, Fix_memorypath, file_table);
    // 1 write to the vfat path, then 1 write to the memory path, then send to persist agent
    assert (utils_gzip_write_fake.call_count == 1);
    filename = (char*)utils_gzip_write_fake.arg0_history[0];
    assert (strncmp(filename, Fix_memorypath, strlen(Fix_memorypath) - 1) == 0);
    usleep(100 * 1000);
    assert (utils_get_vfat_mountpoint_fake.call_count == 1);
    assert (utils_delete_file_fake.call_count == 0);
    assert (utils_copy_file_fake.call_count == 0);

    usleep(300 * 1000);
    failure_agent(NULL, NULL, Fix_memorypath, file_table);
    assert (utils_get_vfat_mountpoint_fake.call_count == 2);
    assert (utils_delete_file_fake.call_count == 0);
    assert (utils_copy_file_fake.call_count == 0);
    keys = zhash_keys(file_table);
    key = zlist_first(keys);
    record = zhash_lookup(file_table, key);
    assert (strncmp(record->filename, Fix_memorypath, strlen(Fix_memorypath) - 1) == 0);

    usleep(500 * 1000);
    utils_get_vfat_mountpoint_fake.return_val = "/mnt/vfat";
    utils_gzip_write_fake.return_val = STATUS_OK;
    failure_agent(NULL, NULL, Fix_memorypath, file_table);
    assert (utils_get_vfat_mountpoint_fake.call_count == 3);
    assert (utils_delete_file_fake.call_count == 1);
    assert (utils_copy_file_fake.call_count == 1);
    keys = zhash_keys(file_table);
    key = zlist_first(keys);
    record = zhash_lookup(file_table, key);
    assert (strncmp(record->filename, "/mnt/vfat", 9) == 0);

    zmsg_destroy(&msg);
    printf ("OK\n");
  }

  printf ("E2E tests passed OK\n");
  return 0;
}

