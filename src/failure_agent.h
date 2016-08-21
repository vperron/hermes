/**
 * =====================================================================================
 *
 *   @file failure_agent.h
 *
 *   
 *        Version:  1.0
 *        Created:  04/16/2013 08:24:08 PM
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


#ifndef _HERMES_FAILURE_AGENT_H_
#define _HERMES_FAILURE_AGENT_H_ 

#ifdef __cplusplus
extern "C" {
#endif

typedef struct file_record_t {
  char *filename;
  bool in_memory;
  size_t rawsize;
  int n_retries;
  uint64_t next_retry;
} file_record;

#ifndef E2E_TESTING
void failure_agent (void *user_args, zctx_t *ctx, void *pipe);
#else
void failure_agent (zmsg_t* msg, char* persistpath, char *memorypath, zhash_t *file_table);
extern int max_retry;
extern uint64_t retry_time;
extern uint64_t nonvolatile_retry;
extern uint64_t next_nonvolatile_retry;
#endif

#ifdef __cplusplus
}
#endif

#endif // _HERMES_FAILURE_AGENT_H_
