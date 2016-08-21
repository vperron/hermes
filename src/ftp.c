/**
 * =====================================================================================
 *
 *   @file ftp.c
 *
 *   
 *        Version:  1.0
 *        Created:  01/08/2013 05:26:52 PM
 *
 *
 *   @section DESCRIPTION
 *
 *       SOPS
 *       
 *   @section LICENSE
 *
 *       
 *
 * =====================================================================================
 */

#include <string.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <libgen.h>

#include "main.h"
#include "ftp.h"

#define FTP_DEFAULT_TIMEOUT 30 // secs

struct ftp_handle_t_st {
  char* base_uri;
  char* datastream_id;
  int timeout;
}; 

ftp_handle_t* ftp_new(const char* uri, const char *datastream_id, int timeout) 
{
  assert (uri);
  assert (datastream_id);

  char full_uri[MAX_STRING_LEN];
  snprintf(full_uri, MAX_STRING_LEN, "%s/%s", uri, datastream_id);

  ftp_handle_t* ftp = malloc(sizeof(ftp_handle_t));
  memset(ftp,0,sizeof(ftp_handle_t));
  
  ftp->base_uri = strdup(full_uri);
  ftp->datastream_id = strdup(datastream_id);
  if (timeout == -1)
    ftp->timeout = FTP_DEFAULT_TIMEOUT;
  else 
    ftp->timeout = timeout;
  
  return ftp;
}

void ftp_destroy(ftp_handle_t* ftp) 
{
  assert (ftp);
  free(ftp->datastream_id);
  free(ftp->base_uri);
  free(ftp);
}

static int s_upload_file(ftp_handle_t* ftp, const char* uri, const char* filename)
{
  int ret = STATUS_ERROR;
  CURL *curl = NULL;
  FILE *hd_src = NULL;
  CURLcode res;
  struct stat file_info;
  curl_off_t fsize;

  if(stat(filename, &file_info)) {
    errorLog("Couldnt open '%s': %s", filename, strerror(errno));
    return STATUS_ERROR;
  }

  fsize = (curl_off_t)file_info.st_size;
  hd_src = fopen(filename, "rb");

  curl = curl_easy_init();
  if(curl) {
    // Create the remote directory
    char mkdoption[MAX_STRING_LEN];
    struct curl_slist *commands=NULL;
    snprintf(mkdoption, MAX_STRING_LEN, "MKD %s", ftp->datastream_id);
    commands = curl_slist_append(commands, mkdoption);
    curl_easy_setopt(curl, CURLOPT_QUOTE, commands);
    curl_easy_setopt(curl, CURLOPT_URL, uri);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.28.1");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, ftp->timeout);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_slist_free_all(commands);
  }

  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, uri);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.28.1");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, ftp->timeout);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fsize);
    res = curl_easy_perform(curl);
    debugLog("ftpurl %s ", uri);
    if(res != CURLE_OK) {
      errorLog("curl_easy_perform() failed: %d %s",res, curl_easy_strerror(res));
      ret = STATUS_ERROR;
    } else {
      ret = STATUS_OK;
    }
    curl_easy_cleanup(curl);
  }
  fclose(hd_src);
  return ret;
}

int ftp_file_upload(ftp_handle_t* ftp, const char* filename)
{
  assert (ftp);
  char full_uri[MAX_STRING_LEN];
  char* base_name;
  char filename_copy[MAX_STRING_LEN];
  memset(full_uri,0,MAX_STRING_LEN);
  memset(filename_copy,0,MAX_STRING_LEN);
  strncpy(filename_copy, filename, MAX_STRING_LEN);
  base_name = basename(filename_copy);
  snprintf(full_uri, MAX_STRING_LEN, "%s/%s", ftp->base_uri, base_name);
  return s_upload_file(ftp, full_uri, filename);
}

/* Test */
int ftp_selftest(bool verbose)
{
    printf (" * ftp: ");

    ftp_handle_t* ftp  = NULL;
    char *uri = "ftp://stupid";
    char *datastream_id = "datastream";
    char *full_uri = "ftp://stupid/datastream";

    /*  Test ftp timeout setting  */
    ftp = ftp_new(uri, datastream_id, 42);
    assert (ftp->timeout == 42);
    ftp_destroy(ftp);

    /*  Test ftp creation  */
    ftp = ftp_new(uri, datastream_id, -1);
    assert (streq(ftp->base_uri, full_uri));
    assert (ftp->timeout == FTP_DEFAULT_TIMEOUT);

    /*  Test error reporting  */
    int rc = ftp_file_upload(ftp, "nosuchfile");
    assert (rc == STATUS_ERROR);

    /*  Destroy safely */
    ftp_destroy(ftp);

    printf ("OK\n");
    return 0;
}
