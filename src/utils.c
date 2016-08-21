/**
 * =====================================================================================
 *
 *   @file utils.c
 *
 *   
 *        Version:  1.0
 *        Created:  04/23/2013 11:24:56 AM
 *
 *
 *   @section DESCRIPTION
 *
 *       Utility functions, mostly for I/O
 *       
 *   @section LICENSE
 *
 *       
 *
 * =====================================================================================
 */


#include "main.h"
#include "utils.h"

#include <zlib.h>

int utils_copy_file(const char *src, const char *dst)
{
  char buf[BUFSIZ];
  size_t size;
  int ret = STATUS_OK;

  FILE* source = fopen(src, "rb");
  FILE* dest = fopen(dst, "wb");

  if (source != NULL && dest != NULL) {
    while ((size = fread(buf, 1, BUFSIZ, source))) {
      fwrite(buf, 1, size, dest);
    }
    if (ferror(source) != 0 || ferror(dest) != 0) {
      ret = STATUS_ERROR;
    }
  }

  if (source)
    fclose(source);
  if (dest)
    fclose(dest);

  return ret;
}

char *utils_get_vfat_mountpoint() 
{
  char mount_point[MAX_STRING_LEN];
  char mount_type[MAX_STRING_LEN];
  char *found_mount = NULL;

  FILE *f = fopen("/proc/mounts", "r");

  bzero(mount_point, sizeof(mount_point));
  bzero(mount_type, sizeof(mount_type));
  while( fscanf(f, "%*s %s %s %*s %*d %*d\n", mount_point, mount_type) != EOF ) {
    if (streq(mount_type, "vfat")) {
      found_mount = strdup(mount_point);
    }
    bzero(mount_point, sizeof(mount_point));
    bzero(mount_type, sizeof(mount_type));
  }

  fclose(f);

  return found_mount;
}


void utils_send_success(const char* msg_id)
{
  zmsg_t *status = zmsg_new();
  zmsg_addstr(status, MSG_SUCCESS);
  zmsg_addstr(status, msg_id);
  if (failure_agent_pipe != NULL) {
    agent_msg_send(&status, failure_agent_pipe);
  } else {
    agent_msg_destroy(&status);
  }
}

void utils_send_failure(const char* msg_id)
{
  zmsg_t *status = zmsg_new();
  zmsg_addstr(status, MSG_ERROR);
  zmsg_addstr(status, msg_id);
  if (failure_agent_pipe != NULL) {
    agent_msg_send(&status, failure_agent_pipe);
  } else {
    agent_msg_destroy(&status);
  }
}


/*  
 *  Credit for this useful function goes to Nico Golde at
 *  http://nion.modprobe.de/blog/archives/357-Recursive-directory-creation.html 
 */
void utils_recursive_mkdir(const char *path)
{
  char opath[MAX_STRING_LEN];
  char *p;
  size_t len;

  strncpy(opath, path, sizeof(opath));
  len = strlen(opath);
  if (opath[len - 1] == '/')
    opath[len - 1] = '\0';
  for(p = opath; *p; p++)
    if (*p == '/') {
      *p = '\0';
      if (access(opath, F_OK))
        mkdir(opath, S_IRWXU);
      *p = '/';
    }
  if (access(opath, F_OK))  /* if path is not terminated with / */
    mkdir(opath, S_IRWXU);
}

int utils_gzip_write(const char* file_name, char* data, size_t len)
{
  int ret = 0;
  gzFile fi = gzopen(file_name,"wb");
  if (!fi)
    return STATUS_ERROR;
  ret = gzwrite(fi,data,len);
  gzclose(fi);
  return ret <= 0 ? STATUS_ERROR : STATUS_OK;
}

int utils_gzip_read(const char* file_name, char* buffer, size_t maxlen)
{
  int len = 0;
  gzFile fi = gzopen(file_name,"rb");
  if (!fi)
    return STATUS_ERROR;

  gzrewind(fi);
  len = gzread(fi,buffer,maxlen);
  gzclose(fi);
  return len;
}

int utils_delete_file(const char *file_name)
{
  return unlink(file_name);
}

char *utils_generate_json(const char *timestamp, const char* mac, const char* prefix,
                          const char* dbm, const char* bssid, const char* ssid)
{
  char output[MAX_STRING_LEN];

  snprintf(output, MAX_STRING_LEN, "{\"ts\":\"%s\",\"mac\":\"%s\",\"manuf\":\"%s\",\"rssi\":%s, \"bssid\":\"%s\",\"ssid\":\"",
      timestamp, mac, prefix, dbm, bssid);

  if (ssid != NULL) {
    snprintf(output, MAX_STRING_LEN, "{\"ts\":\"%s\",\"mac\":\"%s\",\"manuf\":\"%s\",\"rssi\":%s,\"bssid\":\"%s\",\"ssid\":\"%s\"}",
      timestamp, mac, prefix, dbm, bssid, ssid);
  } else {
    snprintf(output, MAX_STRING_LEN, "{\"ts\":\"%s\",\"mac\":\"%s\",\"manuf\":\"%s\",\"rssi\":%s,\"bssid\":\"%s\",\"ssid\":\"\"}",
      timestamp, mac, prefix, dbm, bssid);
  }

  return strdup(output);
}

