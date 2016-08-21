/**
 * =====================================================================================
 *
 *   @file hermes_unittest.c
 *
 *   
 *        Version:  1.0
 *        Created:  04/09/2013 10:06:32 AM
 *
 *
 *   @section DESCRIPTION
 *
 *       Routines to test hermes
 *       
 *   @section LICENSE
 *
 *       
 *
 * =====================================================================================
 */


#include "main.h"
#include "fixtures.h"

void *ftp_agent_pipe = NULL;
void *http_agent_pipe = NULL;
void *failure_agent_pipe = NULL;
void *hashtable_agent_pipe = NULL;
void *persist_agent_pipe = NULL;

#include "superfasthash.c"
#include "utils.c"
#include "config.c"
#include "ftp.c"
#include "sops.c"
#include "http_agent.c"
#include "hashtable_agent.c"
#include "failure_agent.c"
#include "main.c"

int main (int argc, char *argv [])
{
  bool verbose;
  if (argc == 2 && streq (argv [1], "-v"))
    verbose = true;
  else
    verbose = false;

  printf ("Running hermes unit tests...\n");

  ftp_selftest(verbose);
  http_agent_selftest(verbose);
  hashtable_agent_selftest(verbose);
  failure_agent_selftest(verbose);
  receiver_agent_selftest(verbose);

  printf ("Unit tests passed OK\n");
  return 0;
}


