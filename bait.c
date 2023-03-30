#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <dlfcn.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/vfs.h>
#include <sys/statfs.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <sys/xattr.h>



int main(int argc,char *argv[]){

  for(int i=1;i<argc;i++){
  char *p=argv[i];
  struct stat *st;
  stat(p,st);  printf("S_S_ISDIR=%d\n",S_ISDIR(st->st_mode));
  lstat(p,st);  printf("S_S_ISDIR=%d\n",S_ISDIR(st->st_mode));
  uid_t uid;
  getxattr(p, "user.virtfs.uid", &uid,          sizeof(uid_t));
  printf("uid=%d\n",uid);
  }

}
