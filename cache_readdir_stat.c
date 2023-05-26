#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <dlfcn.h>
#include <stdio.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/types.h>
// /usr/include/x86_64-linux-gnu/sys/stat.h
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/xattr.h>
#define WITH64 0
/* Without deactivation there is a problem: Files created in wine are not readable.  See test/Create_a_file_with_wine.sh */
#define HOOK_OPEN 0
#define HOOK_READ 0
#define HOOK_MALLOC 0
#define HOOK_REWINDDIR 0
#define HOOK_FSTAT 1
#define HOOK_READDIR 1
//#define HOOK_READDIR_PASS_THROUGH 1
#define IS_SILENT 0
// #define pthread_mutex_lock(...)
// diann_cli.sh  /export/win10/diann/queue/run/cgille@test_ZIPsFS_less.job5.active
//  sudo cp  /home/cgille/compiled/cache_readdir_stat.so /usr/local/bin/


//#include <sys/xattr.h>
//  (global-set-key (kbd "<f2>") '(lambda() (interactive) (switch-to-buffer "cache_readdir_stat.c")))
// See /usr/include/dirent.h
// many __fxstatat(  / .ciopfs
/***********************************************/
// (occur "\\borig_\\w+(")
/***********************************************/
/* Mutex */


static pthread_mutexattr_t _mutex_attr_recursive;
static pthread_mutex_t _mutex_dir,_mutex_ht;
#define DO_NOTHING true
#define SYNCHRONIZE_DIR(code)  pthread_mutex_lock(&_mutex_dir);code;pthread_mutex_unlock(&_mutex_dir)
/***********************************************/

#include "cg_log.c"
#include "cg_utils.c"
#include "cg_debug.c"
#include "ht4.c"
#define SHIFT_INODE_ROOT 40
#define SHIFT_INODE_ZIPENTRY 43
bool log_specific_path(const char *title,const char *p){
  if (DO_NOTHING) return false;
  if (p && endsWith(p,"20230126_PRO1_KTT_002_30-0046_LisaKahl_P01_VNATSerAuxpM1evoM2Glucose10mMGlycine3mM_dia_BC7_1_12094.d")){
    log_msg(ANSI_FG_MAGENTA"log_specific_path %s %s\n",title,p);
    return true;
  }
  return false;
}
#define log_mydir(d,...) log_msg(__VA_ARGS__),log_msg(ANSI_FG_MAGENTA" %d < %d <%d\n"ANSI_RESET,d->begin,d->i,d->end)
#define log_mthd_orig_p(s,path) static int __count_orig=0;_log_mthd(ANSI_FG_BLUE #s ANSI_RESET,++__count_orig),log_specific_path(#s,path)
/***********************************************/
/* *** Customize rules for caching *** */
bool path_contains_bruker(const char *p,int l){
  if (l<20) return false;
  for(int i=l+1;--i>=1;){
    if (i==l || p[i]=='/'){
      if (p[i-2]=='.' && p[i-1]=='d'){
        const char *b=memrchr(p,'/',i-1);
        if (b && b[1]=='2' && b[2]=='0' && isdigit(b[3]) && isdigit(b[4])) { /* folder name starts with year */
          return true;
        }
      }
    }
  }
  return false;
}
bool isPRO123(const char *p,int l){
  for(int i=l-4;--i>=1;){
    if (p[i]=='/'){
      if (i+4<l && !memcmp(p+i+1,"PRO",3)  && '0'<p[i+4] && p[i+4]<'4' && (i+5==l||p[i+5]=='/')) return true;
      if (i+5<l && !memcmp(p+i+1,"TIMS2",5) && (i+6==l||p[i+6]=='/')) return true;
    }
  }
  return false;
}
bool not_report_stat_error(const char *p){
#define I(s) endsWith(p,#s)||
  if (I(/analysis.tdf-wal) I(/analysis.tdf-journal) I(/.ciopfs) false) return true;
#undef I
  return false;
}
bool need_cache_dir(const char *p){
  //  if (DO_NOTHING) return false;
  //return true; // DEBUG_NOW
  const int l=my_strlen(p);
  if (!l) return false;
  if (path_contains_bruker(p,l)) return true;
  if (isPRO123(p,l)) return true;
  //log_debug_now(" NOT need_cache_dir %s \n",p);
  return false;
}
bool need_cache_fstatfs(const char *p){
  //  if (DO_NOTHING) return false;
  const int l=my_strlen(p);
  return path_contains_bruker(p,l) || isPRO123(p,l);
}
bool need_cache_xstat(const char *p){
  //if (DO_NOTHING) return false;
  const int l=my_strlen(p);
  if (path_contains_bruker(p,l)) return true;
  if (not_report_stat_error(p)) return true;
  if (isPRO123(p,l)) return true; /* This causes "Could not load ... " when the 14th Brukertimstof is loaded */
  return false;
}
bool need_cache_fxstatat(const char *p){  // This breaks diann
  //if (DO_NOTHING) return false;
  const int l=my_strlen(p);
  if (!l) return false;
  if (not_report_stat_error(p)) return true;
  return false;
}
/* End custom rules */
/***********************************************/

#define DIM_D2 1024
#define DIM_SHIFT 10
#define DIM_AND 1023
#define d1_d2(i) const int d1=i>>DIM_SHIFT, d2=i&DIM_AND
/***********************************************/
#if HOOK_OPEN
FILE* (*orig_fopen)(const char *name,const char *mode);
int (*orig_open)(const char *name,int flags);
int (*orig_open64)(const char *name,int flags);
int (*orig_openat)(int dirfd, const char *name, int flags);
#endif
#if HOOK_READ
ssize_t (*orig_read)(int fd, void *buf, size_t count);
#endif
#if HOOK_READDIR
static struct dirent *(*orig_readdir)(DIR *d);
DIR *(*orig_opendir)(const char *name);
DIR *(*orig_fdopendir)(int fd);
int (*orig_closedir)(DIR *d);
#endif
#if HOOK_REWINDDIR
void (*orig_rewinddir)(DIR *d);
int (*orig_dirfd)(DIR *d);
void (*orig_seekdir)(DIR *d,long pos);
long (*orig_telldir)(DIR *d);
#endif
#if HOOK_FSTAT
int (*orig_fstatfs)(int fd,struct statfs *b);
int (*orig___xstat)(int ver, const char *filename,struct stat *b);
int (*orig___fxstatat)(int ver, int fildes, const char *path,struct stat *buf, int flag);
#endif
static bool _inititialized=false;
#define INIT(orig) if (!_inititialized) _init_c(),assert(orig_ ## orig!=NULL)
void _init_c();
#define F0(ret,m,...) static ret (*orig_ ## m)(__VA_ARGS__);ret m(__VA_ARGS__){log_mthd_orig(m);INIT(m);return orig_ ## m(AA);}
#define FP(ret,m,...) static ret (*orig_ ## m)(__VA_ARGS__);ret m(__VA_ARGS__){log_mthd_orig_p(m,path);INIT(m);return orig_ ## m(AA);}

#if HOOK_FSTAT
#define AA path,mode
FP(int,access,const char *path,int mode)
  FP(int,creat,const char *path, mode_t mode);
#undef AA
#define AA path,buf
FP(int,statfs,const char *path,struct statfs *buf);
FP(int,stat,const char *path,struct stat *buf);
FP(int,lstat,const char *path,struct stat *buf);
#if WITH64
FP(int,stat64,const char *path,struct stat64 *buf);
FP(int,lstat64,const char *path,struct stat64 *buf);
#endif
#undef AA

#define AA fd,buf
F0(int,fstat,int fd,struct stat *buf);
#if WITH64
F0(int,fstat64,int fd,struct stat64 *buf);
#endif
#undef AA

#define AA fd,path,buf,flag
FP(int,fstatat,int fd, const char *path, struct stat *buf,int flag);
#if WITH64
FP(int,fstatat64,int fd, const char *path, struct stat64 *buf,int flag);
#endif
#undef AA

#define AA ver,fildes,buf
F0(int,__fxstat,int ver, int fildes, struct stat *buf)
#undef AA

/* #define AA stream */
/*   F0(int,fileno,FILE* stream); */
/* #undef AA */

#define AA ver,path,buf
  FP(int,__lxstat,int ver,const char *path,struct stat *buf);
#undef AA

#define AA dirfd,path,flags,mask,buf
FP(int,statx,int dirfd,const char *path,int flags,unsigned int mask,struct statx *buf);
#undef AA

#define AA path,name,value,size
FP(ssize_t,getxattr,const char *path,const char *name,void *value,size_t size);
#undef AA

#define AA fd,path,value,size
FP(ssize_t,fgetxattr,int fd,const char *path,void *value,size_t size);
#undef AA

#undef F0
#undef FP

#endif

void *__dlsym(void *handle, const char *symbol);
void *my_dlsym(const char *symbol){
  void *s=dlsym(RTLD_NEXT,symbol);
  //log_debug_now("dlsym %s %p\n",symbol,s);
  if (!s) log_error("dlsym  %s\n",dlerror());
  return s;
}
struct ht _ht_dir,_ht_stat,_ht_fstatat;



void _init_c(void){
  if (_inititialized) return;
  log_entered_function("\n\ncache_readdir_stat.so _init_c\n");
#define F(s) (orig_ ## s=my_dlsym(#s))
  /* --- Reading Directory    deprecated:readdir_r --- */
#if HOOK_OPEN
  F(open);F(open64);F(fopen);F(creat);F(openat);
#endif
#if HOOK_READ
  F(read);
#endif
#if HOOK_READDIR
  F(opendir);  F(fdopendir);  F(readdir);  F(closedir);
#endif
#if HOOK_REWINDDIR
  F(rewinddir);  F(dirfd);  F(seekdir);  F(telldir);
#endif
  /* --- stat etc. --- */
#if HOOK_FSTAT
  F(statfs);  F(access);   F(fstatfs);
  log_msg("Note: the following  dl-symbols may be aliased.\n"
          "   stat -> __xstat    lstat -> __lxstat    fstat -> __fxstat\n"
          "   See https://stackoverflow.com/questions/48994135/unable-to-get-stat-with-dlsym\n");

  F(stat);  F(fstat);  F(lstat);  F(fstatat);
#endif
#if WITH64
  F(stat64);  F(fstat64);  F(lstat64);  F(fstatat64);
#endif
#if HOOK_FSTAT
  F(__fxstat);  F(__xstat);  F(__lxstat);  F(__fxstatat);
  F(statx);  F(getxattr);  F(fgetxattr);
#endif
#undef F
  pthread_mutexattr_init(&_mutex_attr_recursive);
  pthread_mutexattr_settype(&_mutex_attr_recursive,PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&_mutex_ht,&_mutex_attr_recursive);
  pthread_mutex_init(&_mutex_dir,&_mutex_attr_recursive);
  ht_init(&_ht_dir,11);
  ht_init(&_ht_stat,11);
  ht_init(&_ht_fstatat,11);
  _inititialized=true;
  log_exited_function("\ncache_readdir_stat.so _init_c\n\n");
#if IS_SILENT
  _logIsSilent=true;
#endif
  //_logIsSilentFailed=false,_logIsSilentWarn=false,_logIsSilentError=false;
}



/* *** End dlsym *** */
/********************************************************************************/
/* *** Maybe this prevents some seqfaults in Winde diann.exe *** */
#if HOOK_MALLOC
void *malloc(size_t size){return calloc(size,1);}
#endif
#if HOOK_OPEN
ssize_t read(int fd, void *buf, size_t count){
  INIT(read);
  const ssize_t n=orig_read(fd,buf,count);
  char p[MAX_PATHLEN]; path_for_fd("read", p,fd);
  if(tdf_or_tdf_bin(p)){
    static struct ht _ht_read={0};
    sprintf(p+strlen(p),"  %d",fd);
    pthread_mutex_lock(&_mutex_ht);
    bool do_log=!ht_set(&_ht_read,p,"");
    pthread_mutex_unlock(&_mutex_ht);
    if (do_log) log_debug_now("read %ld %s\n",n,p);
  }
  return n;
}
#endif
#if HOOK_OPEN
int openat(int dirfd, const char *path, int flags,...){
  INIT(openat);
  log_mthd_invoke(openat);
  const int fd=orig_openat(dirfd,path,flags);
  if (tdf_or_tdf_bin(path)) { log_debug_now("openat %d  ",fd); print_path_for_fd(fd);}
  if (fd<0) log_warn("failed openat %s\n",path);
  return fd;
}
int open(const char *path,int flags, ...){
  if (endsWith(path,".d")) return -1;
  INIT(open);
  //log_entered_function("open %s\n",path);
  log_mthd_invoke(open);
  const int fd=orig_open(path,flags);
  if (tdf_or_tdf_bin(path)){ log_debug_now("open %s  %d ",path,fd);   print_path_for_fd(fd);  }
  if (fd<0)  log_warn("failed open %s\n",path);
  return fd;
}
int open64(const char *path,int flags,...){
  INIT(open64);
  return orig_open64(path,flags);
}
FILE *fopen(const char *path,const char *mode){
  INIT(fopen);
  FILE *f=orig_fopen(path,mode);
  if (tdf_or_tdf_bin(path))    log_debug_now("fopen %s %p \n",path,f);
  return f;
}
#endif
/***********************************************/
/* *** Fake file directory *** */
#define DIM_D1 1024*256
#define DIM_DIR 1000
#define MAX_PATHLEN_MYDIR 255
struct mydir{int begin,end,i;
  //  char path[MAX_PATHLEN_MYDIR+1]; bool tdf,tdf_bin;// DEBUG_NOW
};
struct mydir _store_dir[DIM_DIR];
#define MYDIR() struct mydir *mydir=(struct mydir*)dir
int _dir_l=0;
enum data_op{GET,CREATE,RELEASE};
bool is_mydir(void *dir){ /* Can be struct mydir or DIR */
  MYDIR();
  return _store_dir<=mydir &&  mydir<_store_dir+_dir_l;
}
struct mydir *getmydir(enum data_op op,const char *path){
  if ((!path || !*path)) return NULL;
  struct mydir *dir=ht_get(&_ht_dir,path);
  if (!dir && op==CREATE){
    if (_dir_l<DIM_DIR){
      ht_set(&_ht_dir,path,dir=_store_dir+_dir_l++);
    }else{
      log_error("Too many directories\n");
    }
  }
  return dir;
}
/***********************************************/
/* *** Directory entry *** */

struct dirent *_store_dirent[DIM_D1]={0};
int _dirent_l=0;
int push_dirent(struct dirent *e){
  d1_d2(_dirent_l);
  if (d1>DIM_D1){
    log_error("Too many dirent\n");
    exit(-1);
  }
  //log_debug_now("push_dirent %p %s\n",e,e->d_name);
  if (!_store_dirent[d1]) _store_dirent[d1]=malloc(DIM_D2*sizeof(struct dirent));
  _store_dirent[d1][d2]=*e;
  return _dirent_l++;
}
#if HOOK_READDIR
struct dirent *next_dirent(void *dir){
  struct mydir *mydir=dir;
  const int i=mydir->i;
  //log_mydir(mydir,"next_dirent");
  if (i>=mydir->end) return NULL;
  mydir->i++;
  d1_d2(i);
  struct dirent *d=_store_dirent[d1]?_store_dirent[d1]+d2:NULL;
  //  char *n= d ? d->e.d_name:NULL;     if (endsWith(n,".tdf")) mydir->tdf=true;     if (endsWith(n,".tdf_bin")) mydir->tdf_bin=true;
  return d;
}
struct dirent *readdir(DIR *dir){
  INIT(readdir);
  log_mthd_invoke(readdir);
  //log_entered_function("readdir fd=%d is_mydir=%s\n",dirfd(dir),yes_no(is_mydir(dir)));
  if (is_mydir(dir)){
    struct dirent *e=next_dirent(dir);
    //log_msg(ANSI_YELLOW""ANSI_FG_BLACK" %s "ANSI_RESET,e->d_name);
    return e;
  }
  log_mthd_orig(readdir);
  return orig_readdir(dir);
}
void *opendir_common(const char *path){
  INIT(opendir);
  SYNCHRONIZE_DIR(struct mydir *mydir=getmydir(GET,path));

  //strcpy(mydir->path,path); // DEBUG_NOW
  //log_entered_function("oooooooooooooooooooo opendir_common mydir=%s\n",yes_no(!!mydir));
  if (!mydir){
    DIR *dir=orig_opendir(path);
    if (!dir) log_warn("opendir failed %s\n",path);
    pthread_mutex_lock(&_mutex_dir);
    SYNCHRONIZE_DIR(mydir=getmydir(CREATE,path));
    assert(is_mydir(mydir));
      struct dirent *dirent;
    for(int k=0;(dirent=orig_readdir(dir)) && k<10000;k++){
      mydir->end=push_dirent(dirent);
      if (!k) mydir->begin=mydir->end;
    }
    pthread_mutex_unlock(&_mutex_dir);
    orig_closedir(dir);
  }
  SYNCHRONIZE_DIR(mydir->i=mydir->begin);
  return mydir;
}
DIR *opendir(const char *path){
  INIT(opendir);
  log_mthd_invoke(opendir);
  if (need_cache_dir(path)) return  opendir_common(path);
  log_mthd_orig_p(opendir,path);
  return orig_opendir(path);
}
DIR *fdopendir(int fd){
  INIT(fdopendir);
  log_mthd_invoke(fdopendir);
  char path[MAX_PATHLEN];
  *path=0;
  if (fd>2 && !path_for_fd("fdopendir",path,fd) && need_cache_dir(path)) return opendir_common(path);
  log_mthd_orig_p(fdopendir,path);
  return orig_fdopendir(fd);
}
int closedir(DIR *dir){
  INIT(closedir);
  log_mthd_invoke(closedir);
  if (is_mydir(dir)){
    return 0;
  }
  log_mthd_orig(closedir);
  return orig_closedir(dir);
}
#if HOOK_REWINDDIR
void rewinddir(DIR *dir){
  INIT(rewinddir);
  if (is_mydir(dir)){
    MYDIR();
    mydir->i=mydir->begin;
  }else{
    orig_rewinddir(dir);
  }
}
void seekdir(DIR *dir,long pos){
  INIT(seekdir);
  if (is_mydir(dir)){
    MYDIR();
    mydir->i=mydir->begin+pos;
  }else{
    return orig_seekdir(dir,pos);
  }
}
long telldir(DIR *dir){
  INIT(telldir);
  if (is_mydir(dir)){
    MYDIR();
    return mydir->i-mydir->begin;
  }else{
    return orig_telldir(dir);
  }
}
int dirfd(DIR *dir){
  INIT(dirfd);
  if (is_mydir(dir)){
    log_error("dirfd not implemented \n");
    exit(-1);
  }
  return orig_dirfd(dir);
}
#endif
#endif
/********************************************************************************/
/* *** fstatfs *** */
/********************************************************************************/
/*
  ssize_t readlink(const char *path, char *buf, size_t bufsiz){
  const ssize_t n=orig_readlink(path,buf,bufsiz);
  log_mthd_orig(readlink);
  log_msg("path='");
  if (n>0) write(2,path,MIN(bufsiz,n));
  log_msg("'\n");
  return n;
  }
*/
/********************************************************************************/
#define X_FOR_PATH(STRUCT)                                              \
  int _ ## STRUCT ## _l=0;                                              \
  struct STRUCT *_store_ ## STRUCT[DIM_D1]={0};                           \
  struct STRUCT *STRUCT ## _for_path(const char *path,struct ht *ht){   \
    pthread_mutex_lock(&_mutex_ht);                                     \
    struct STRUCT *s=ht_get(ht,path);                                   \
    if (!s){                                                            \
      d1_d2(_ ## STRUCT ## _l++);                                       \
      if (!_store_ ## STRUCT[d1]) _store_ ## STRUCT[d1]=malloc(DIM_D2*sizeof(struct STRUCT)); \
      s=_store_ ## STRUCT[d1]+d2;                                       \
      memset(s,0,sizeof(struct STRUCT));                                \
      ht_set(ht,path,s=_store_ ## STRUCT[d1]+d2);                       \
    }                                                                   \
    pthread_mutex_unlock(&_mutex_ht);                                   \
    return s;                                                           \
  }
X_FOR_PATH(stat);
X_FOR_PATH(statfs);
/********************************************************************************/
#if HOOK_FSTAT
int fstatfs(int fd,struct statfs *buf){
  log_mthd_invoke(fstatfs);
  char path[MAX_PATHLEN]; *path=0;
  if (fd>2 && !path_for_fd("fstatfs",path,fd) && need_cache_fstatfs(path)){
    struct statfs *s=statfs_for_path(path,&_ht_fstatat);
    if (!s->f_type){
      log_mthd_orig_p(fstatfs,path);
      if (orig_fstatfs(fd,s)){
        log_warn("fstatfs %d\n",fd);
        perror(" ");
        s->f_type=-1;
      }
    }
    if (s->f_type==-1) return -1;
    *buf=*s;
    return 0;
  }
  log_mthd_orig_p(fstatfs,path);
  return orig_fstatfs(fd,buf);
}
/********************************************************************************/
int __fxstatat(int ver, int fildes, const char *name,struct stat *statbuf, int flag){
  //return orig___fxstatat(ver,fildes,name,statbuf,flag);
  log_mthd_invoke(__xstat);
  char path[MAX_PATHLEN];
  path_for_fd("__fxstatat",path,fildes);
  int len=my_strlen(path);
  path[len]='/';
  strcpy(path+len+1,name);
  if (need_cache_fxstatat(path)){
    struct stat *s=stat_for_path(path,&_ht_fstatat);
    if (!s->st_ino){
      //log_entered_function("\n__fxstatat %d %d %s  path=%s ",ver,fildes,name,path);
      log_mthd_orig_p(__fxstatat,path);
      if (orig___fxstatat(ver,fildes,name,statbuf,flag)){
        if (!not_report_stat_error(path)){ log_warn("__fxstatat %d %s\n",fildes,path); perror(" ");}
        s->st_ino=-1;
      }
    }
    if (s->st_ino==-1) return -1;
    *statbuf=*s;
    return 0;
  }
  log_mthd_orig_p(__fxstatat,path);
  return orig___fxstatat(ver,fildes,name,statbuf,flag);
}
/********************************************************************************/
/* *** __xstat *** */
int __xstat(int ver, const char *path,struct stat *statbuf){
  //    return orig___xstat(ver,path,statbuf);
  //log_debug_now("__xstat ver=%d %s ",ver,path);
  log_mthd_invoke(__xstat);
  if (need_cache_xstat(path)){
    struct stat *s=stat_for_path(path,&_ht_stat);
    if (!s->st_ino){
      log_mthd_orig_p(__xstat,path);
      if (orig___xstat(ver,path,s)){
        if (!not_report_stat_error(path)){ log_warn("__xstat %d %s\n",ver,path);perror(" ");}
        s->st_ino=-1;
        assert(s->st_ino==-1);
      }
    }
    if (s->st_ino==-1) return -1;
    *statbuf=*s;
    debug_my_file_checks(path,s);
    return 0;
  }
  log_mthd_orig_p(__xstat,path);
  const int res=orig___xstat(ver,path,statbuf);
  if (!res) debug_my_file_checks(path,statbuf);
  return res;
}
#endif
/********************************************************************************/

/********************************************************************************/
/* *** Testing *** */

static int list_files(const char *path){
  struct dirent *entry;
  DIR *dp=opendir(path);
  log_msg(ANSI_INVERSE"listdir %s"ANSI_RESET"\n",path);
  if (dp==NULL){perror("opendir");return -1;}
  while((entry=readdir(dp))) log_msg("%s  ",entry->d_name);
  closedir(dp);
  log_msg(" ");
  return 0;
}
static int xxxxmain(int argc,char *argv[]){
  {
    puts (yes_no(not_report_stat_error(argv[1])));
  }
  return 0;
  for(int i=1;i<argc;i++){
    char *path=argv[i];
    const int fd=open(path,O_RDONLY);
    struct statfs bufstatfs;
    log_debug_now(" main %s \n",path);
    bool is_d=is_dir(path);
    for(int repeat=10;--repeat>=0;){
      if(is_d){
        list_files(path);
      }else{
        if (!fstatfs(fd,&bufstatfs)) log_succes("fstatfs\n");
      }
    }
    close(fd);
  }
  return 0;
}
/*
  Creating file descriptors
  open()
  creat()[5]
  socket()
  accept()
  socketpair()
  pipe()
  epoll_create() (Linux)
  signalfd() (Linux)
  eventfd() (Linux)
  timerfd_create() (Linux)
  memfd_create() (Linux)
  userfaultfd() (Linux)
  fanotify_init() (Linux)
  inotify_init() (Linux)
  clone() (with flag CLONE_PIDFD, Linux)
  pidfd_open() (Linux)
  o

  pen_by_handle_at() (Linux)
*/
// Saves 20 %
