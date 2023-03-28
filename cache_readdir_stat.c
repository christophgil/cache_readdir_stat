#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <sys/vfs.h>    /* or <sys/statfs.h> */
#include "ht.c"
//  (global-set-key (kbd "<f2>") '(lambda() (interactive) (switch-to-buffer "cache_readdir_stat.c")))
// See /usr/include/dirent.h
/***********************************************/
#define ANSI_FG_GREEN "\x1B[32m"
#define ANSI_FG_RED "\x1B[31m"
#define ANSI_FG_MAGENTA "\x1B[35m"
#define ANSI_FG_GRAY "\x1B[30;1m"
#define ANSI_FG_BLUE "\x1B[34;1m"
#define ANSI_FG_WHITE "\x1B[37m"
#define ANSI_INVERSE "\x1B[7m"
#define ANSI_RESET "\x1B[0m"
#define log(...)  fprintf(stderr,__VA_ARGS__)
#define log_calling(...)  log(ANSI_FG_MAGENTA" CALLING "ANSI_RESET),log(__VA_ARGS__)
#define log_failed(...)  log(ANSI_FG_RED" Failed "ANSI_RESET),log(__VA_ARGS__)
#define log_warn(...)  log(ANSI_FG_RED" Warn "ANSI_RESET),log(__VA_ARGS__)
#define log_error(...)  log(ANSI_FG_RED" Error "ANSI_RESET),log(__VA_ARGS__)
#define log_succes(...)  log(ANSI_FG_GREEN" Success "ANSI_RESET),log(__VA_ARGS__)
#define log_debug_now(...)  log(ANSI_FG_MAGENTA" Debug "ANSI_RESET),log(__VA_ARGS__)
#define log_entered_function(...)   log(ANSI_INVERSE"> > > >"ANSI_RESET),printf(__VA_ARGS__)
#define log_exited_function(...)   log(ANSI_INVERSE"< < < <"ANSI_RESET),printf(__VA_ARGS__)
#define log_mydir(d,...) log(__VA_ARGS__),log(ANSI_FG_MAGENTA" %d < %d <%d\n"ANSI_RESET,d->begin,d->i,d->end)
#define log_run10(s) static int count=0; if (count++%10==0) fputs(" "#s" ",stderr)
/***********************************************/
/* *** Customize *** */
      bool need_cache_dir(const char *path){
        return true;
        int len=path?strlen(path):0;
        if (len>20 && strcmp(path+len-2,".d")) return true;
        return false;
      }
/***********************************************/
#define DIM_STATFS 1024
#define DIM 1024
#define DIM_SHIFT 10
#define DIM_AND 1023
#define MAX_PATHLEN 1024
/***********************************************/
/* *** Utils *** */
static char *yes_no(int i){ return i?"Yes":"No";}
static bool is_dir(const char *f){ struct stat st={0}; return !lstat(f,&st) && S_ISDIR(st.st_mode); }
static int path_for_fd(char *path, int fd,char *buf){
  *path=0;
  sprintf(buf,"/proc/%d/fd/%d",getpid(),fd);
  const int n=readlink(buf,path, MAX_PATHLEN-1);
  if (n<0){
    log_error("path_for_fd %s\n",buf);
    perror(" ");
    return -1;
  }
  log_succes("path_for_fd %d %s \n",fd,path);
  return path[n]=0;
}
/* **********************************************/
#define DIM_DIRENT 1024
#define DIM_DIR 100
#define d1_d2(i) const int d1=i>>DIM_SHIFT, d2=i&DIM_AND
struct mydir{int id,begin,end,i;};
struct mydir _store_dir[DIM_DIR];
#define MYDIR() struct mydir *mydir=(struct mydir*)dir
struct ht *_ht_dir;
int _dir_l=0;
enum data_op{GET,CREATE,RELEASE};
bool is_mydir(void *dir){ /* Can be struct mydir or DIR */
  MYDIR();
  return _store_dir<=mydir &&  mydir<_store_dir+_dir_l;
}
struct mydir *getmydir(enum data_op op,const char *path){
  if ((!path || !*path) ) return NULL;
  struct mydir *dir=ht_get(_ht_dir,path);
  if (!dir && op==CREATE){
    if (_dir_l<DIM_DIR){
      ht_set(_ht_dir,path,dir=_store_dir+_dir_l++);
    }else{
      log_error("Too many directories\n");
    }
  }
  return dir;
}
struct mydirent { struct dirent e;struct dirent64 e64;};
struct mydirent *_store_dirent[DIM_DIRENT]={0};
int _dirent_l=0;
static int push_dirent(struct dirent *e,struct dirent64 *e64){
  d1_d2(_dirent_l);
  if (d2>DIM_DIRENT){
    log_error("Too many mydirent\n");
    exit(-1);
  }
  log_debug_now("push_dirent %p %s\n",e,e->d_name);
  if (!_store_dirent[d1]) _store_dirent[d2]=malloc(DIM*sizeof(struct mydirent));
  _store_dirent[d1][d2].e=*e;
  _store_dirent[d1][d2].e64=*e64;
  return _dirent_l++;
}
struct ht *_ht_statfs=NULL;
static struct dirent *(*orig_readdir)(DIR *d);
static struct dirent64 *(*orig_readdir64)(DIR *d);
static DIR *(*orig_opendir)(const char *name);
static DIR *(*orig_fdopendir)(int fd);
static int (*orig_closedir)(DIR *d);
static void (*orig_rewinddir)(DIR *d);
static int (*orig_dirfd)(DIR *d);
static void (*orig_seekdir)(DIR *d,long pos);
static long (*orig_telldir)(DIR *d);
static int (*orig_statfs)(const char *path,struct statfs *buf);
static int (*orig_access)(const char *pathname,int mode);
static int (*orig_fstatfs)(int fd,struct statfs *buf);
static int (*orig_lstat)(const char *path,struct stat *statbuf);
static int (*orig_stat)(const char *path,struct stat *statbuf);
static int (*orig_fstat)(int fd,struct stat *statbuf);
void my_cache_init(void) __attribute__((constructor));
void my_cache_init(void){
  _ht_statfs=ht_create(333);
  _ht_dir=ht_create(333);
  log_debug_now("my_cache_init \n");
#define F(s) orig_ ## s=dlsym(RTLD_NEXT,#s)
  /* --- directory --- */
  F(readdir);
  F(readdir64);
  F(opendir);
  F(fdopendir);
  F(closedir);
  F(rewinddir);
  F(dirfd);
  F(seekdir);
  F(telldir);
  /* deprecated:  readdir64_r readdir_r */
  /* --- stat etc. --- */
  F(statfs);
  F(access);
  F(fstatfs);
  F(lstat);
  F(stat);
  F(fstat);
#undef F
}
struct mydirent *next_dirent(void *dir){
  struct mydir *mydir=dir;
  const int i=mydir->i++;
  //log_mydir(mydir,"next_dirent");
  if (i>=mydir->end) return NULL;
  d1_d2(i);
  return _store_dirent[d1]?_store_dirent[d1]+d2:NULL;
}
struct dirent64 *readdir64(DIR *dir){
  if (is_mydir(dir)){
    struct mydirent *e=next_dirent(dir);
    return e?&e->e64:NULL;
  }else{
    return orig_readdir64(dir);
  }
}
struct dirent *readdir(DIR *dir){
  log_entered_function("readdir fd=%d is_mydir=%s\n",dirfd(dir),yes_no(is_mydir(dir)));
  if (is_mydir(dir)){
    struct mydirent *e=next_dirent(dir);
    return e?&e->e:NULL;
  }else{
    return orig_readdir(dir);
  }
}
static void *opendir_common(const char *path){
  struct dirent *dirent;
  struct mydir *mydir=getmydir(GET,path);
  log_entered_function("opendir_common mydir=%s\n",yes_no(!!mydir));
  if (!mydir){
    DIR *dir=orig_opendir(path);
    mydir=getmydir(CREATE,path);
    assert(is_mydir(mydir));
    log_debug_now("is_mydir=%d\n",is_mydir(mydir));
    struct dirent64 dirent64={0};
    for(int k=0;(dirent=orig_readdir(dir));k++){
#define to64(v) dirent64.v=dirent->v
      to64(d_ino);
      to64(d_off);
      to64(d_reclen);
      to64(d_type);
#undef to64
      strcpy(dirent64.d_name, dirent->d_name);
      mydir->end=push_dirent(dirent,&dirent64);
      if (!k) mydir->begin=mydir->end;
    }
    orig_closedir(dir);
  }
  mydir->i=mydir->begin;
  return mydir;
}
DIR *opendir(const char *path){  return need_cache_dir(path)?opendir_common(path):orig_opendir(path);}
DIR *fdopendir(int fd){
  char path[MAX_PATHLEN],buf[99];
  path_for_fd(path,fd,buf);
  return need_cache_dir(path)?opendir_common(path):orig_fdopendir(fd);
}
int closedir(DIR *dir){ return is_mydir(dir)?0:orig_closedir(dir);}
void rewinddir(DIR *dir){
  if (is_mydir(dir)){
    MYDIR();
    mydir->i=mydir->begin;
  }else{
    orig_rewinddir(dir);
  }
}
void seekdir(DIR *dir,long pos){
  if (is_mydir(dir)){
    MYDIR();
    mydir->i=mydir->begin+pos;
  }else{
    return orig_seekdir(dir,pos);
  }
}
long telldir(DIR *dir){
  if (is_mydir(dir)){
    MYDIR();
    return mydir->i-mydir->begin;
  }else{
    return orig_telldir(dir);
  }
}
int dirfd(DIR *dir){
  if (is_mydir(dir)){
    log_error("dirfd not implemented\n");
    exit(-1);
  }
  return orig_dirfd(dir);
}
int access(const char *path,int mode){
  log_run10(access);
  return orig_access(path,mode);
}
int statfs(const char *path,struct statfs *buf){
  log_run10(statfs);
  return orig_statfs(path,buf);
}
int fstatfs(int fd,struct statfs *buf){
  log_run10(fstatfs);
  return orig_fstatfs(fd,buf);
}
int stat(const char *path,struct stat *statbuf){
  log_run10(stat);
  return orig_stat(path,statbuf);
}
int fstat(int fd,struct stat *statbuf){
  log_run10(fstat);
  return orig_fstat(fd,statbuf);
}
int lstat(const char *path,struct stat *statbuf){
  log_run10(lstat);
  return orig_lstat(path,statbuf);
}
/********************************************************************************/
/* *** Testing *** */
int print_dir_listing(const char *path){
  struct dirent *entry;
  DIR *dp=opendir(path);
  log(ANSI_INVERSE"listdir %s"ANSI_RESET"\n",path);
  if (dp==NULL){perror("opendir");return -1;}
  while((entry=readdir(dp))) log("%s  ",entry->d_name);
  closedir(dp);
  log(" ");
  return 0;
}
int main(int argc,char *argv[]){
  for(int i=1;i<argc;i++){
    char *path=argv[i];
    int fd=open(path,O_RDONLY);
    //struct statfs statfs;
    bool is_d=is_dir(path);
    log_debug_now(" main %s %d \n",path,is_d);
    for(int repeat=10;--repeat>=0;){
      if(is_d){
        print_dir_listing(path);
      }else{
        //  if (!fstatfs(fd,&statfs)) log_succes("fstatfs\n");
      }
    }
    close(fd);
  }
}
