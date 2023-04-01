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
    log(ANSI_FG_MAGENTA"log_specific_path %s %s\n",title,p);
    return true;
  }
  return false;
}
#define log_mydir(d,...) log(__VA_ARGS__),log(ANSI_FG_MAGENTA" %d < %d <%d\n"ANSI_RESET,d->begin,d->i,d->end)
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
  //if (DO_NOTHING) return false;
  const int l=my_strlen(p);
  if (!l) return false;
  if (path_contains_bruker(p,l)) return true;
  if (isPRO123(p,l)) return true;
  //log_debug_now(" NOT need_cache_dir %s \n",p);
  return false;
}
bool need_cache_fstatfs(const char *p){
    if (DO_NOTHING) return false;
  const int l=my_strlen(p);

  return path_contains_bruker(p,l) || isPRO123(p,l);
}
bool need_cache_xstat(const char *p){  // This breaks diann
  //    if (DO_NOTHING) return false;
  const int l=my_strlen(p);
  if (path_contains_bruker(p,l)) return true;
  //  if (isPRO123(p,l)) return true;
  return false;
}
bool need_cache_fxstatat(const char *p){  // This breaks diann
    if (DO_NOTHING) return false;
  const int l=my_strlen(p);
  if (!l) return false;
  if (not_report_stat_error(p)) return true;
  return false;
}

/* End custom rules */
/***********************************************/

#define DIM 1024
#define DIM_SHIFT 10
#define DIM_AND 1023
#define d1_d2(i) const int d1=i>>DIM_SHIFT, d2=i&DIM_AND
/***********************************************/
static struct dirent *(*orig_readdir)(DIR *d);
static struct dirent64 *(*orig_readdir64)(DIR *d);
static DIR *(*orig_opendir)(const char *name);
static DIR *(*orig_fdopendir)(int fd);
static int (*orig_closedir)(DIR *d);
static void (*orig_rewinddir)(DIR *d);
static int (*orig_dirfd)(DIR *d);
static void (*orig_seekdir)(DIR *d,long pos);
static long (*orig_telldir)(DIR *d);
static int (*orig_fstatfs)(int fd,struct statfs *b);
//static int (*orig_stat)(const char *path,struct stat *s);
//static int (*orig_fstat)(int fd,struct stat *s);
static int (*orig___xstat)(int ver, const char *filename,struct stat *b);
//static ssize_t (*orig_readlink)(const char *path, char *buf, size_t bufsiz);
static int (*orig___fxstatat)(int ver, int fildes, const char *path,struct stat *buf, int flag);
static bool _inititialized=false;
#define INIT(orig) if (!_inititialized) _init_c(),assert(orig_ ## orig!=NULL)
static void _init_c();
#define F0(ret,m,...) static ret (*orig_ ## m)(__VA_ARGS__);ret m(__VA_ARGS__){log_mthd_orig(m);INIT(m);return orig_ ## m(AA);}
#define FP(ret,m,...) static ret (*orig_ ## m)(__VA_ARGS__);ret m(__VA_ARGS__){log_mthd_orig_p(m,path);INIT(m);return orig_ ## m(AA);}

#define AA path,mode
FP(int,access,const char *path,int mode)
#undef AA

#define AA path,buf
  FP(int,statfs,const char *path,struct statfs *buf);
#undef AA

#define AA path,buf
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


static void *my_dlsym(const char *symbol){
  void *s=dlsym(RTLD_NEXT,symbol);
  //log_debug_now("dlsym %s %p\n",symbol,s);
  if (!s) log_error("dlsym  %s\n",dlerror());
  return s;
}
struct ht *_ht_dir,*_ht_stat, *_ht_fstatat;



static void _init_c(void){
  log_entered_function("_init_c\n");
#define F(s) (orig_ ## s=my_dlsym(#s))
  /* --- Reading Directory    deprecated:readdir64_r,readdir_r --- */
  F(readdir);
  F(readdir64);
  F(opendir);
  F(fdopendir);
  F(closedir);
  F(rewinddir);
  F(dirfd);
  F(seekdir);
  F(telldir);
  /* --- stat etc. --- */
  F(statfs);
  F(access);
  F(fstatfs);
  log("Note: the following  dl-symbols may be aliased.\n"
      "   stat -> __xstat    lstat -> __lxstat    fstat -> __fxstat\n"
      "   See https://stackoverflow.com/questions/48994135/unable-to-get-stat-with-dlsym\n");

  F(stat);
  F(fstat);
  F(lstat);
  F(fstatat);
  #if WITH64
  F(stat64);
  F(fstat64);
  F(lstat64);
  F(fstatat64);
  #endif





  F(__fxstat);
  F(__xstat);
  F(__lxstat);
  F(__fxstatat);

  //  F(readlink);
  F(statx);
  F(getxattr);
  F(fgetxattr);
#undef F
  pthread_mutexattr_init(&_mutex_attr_recursive);
  pthread_mutexattr_settype(&_mutex_attr_recursive,PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&_mutex_ht,&_mutex_attr_recursive);
  pthread_mutex_init(&_mutex_dir,&_mutex_attr_recursive);
  _ht_dir=ht_create(11);
  _ht_stat=ht_create(11);
  _ht_fstatat=ht_create(11);
  _inititialized=true;
  log_exited_function("_init_c\n");
}




/* *** End dlsym *** */
/***********************************************/

/***********************************************/
/* *** Fake file directory *** */
#define DIM_DIRENT 1024
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
/***********************************************/
/* *** Directory entry *** */

struct mydirent { struct dirent e;struct dirent64 e64;};
struct mydirent *_store_dirent[DIM_DIRENT]={0};
int _dirent_l=0;
static int push_dirent(struct dirent *e,struct dirent64 *e64){
  d1_d2(_dirent_l);
  if (d2>DIM_DIRENT){
    log_error("Too many mydirent\n");
    exit(-1);
  }
  //log_debug_now("push_dirent %p %s\n",e,e->d_name);
  if (!_store_dirent[d1]) _store_dirent[d2]=malloc(DIM*sizeof(struct mydirent));
  _store_dirent[d1][d2].e=*e;
  _store_dirent[d1][d2].e64=*e64;
  return _dirent_l++;
}
struct mydirent *next_dirent(void *dir){
  struct mydir *mydir=dir;
  const int i=mydir->i++;
  //log_mydir(mydir,"next_dirent");
  if (i>=mydir->end) return NULL;
  d1_d2(i);
  struct mydirent *d=_store_dirent[d1]?_store_dirent[d1]+d2:NULL;

  //  char *n= d ? d->e.d_name:NULL;     if (endsWith(n,".tdf")) mydir->tdf=true;     if (endsWith(n,".tdf_bin")) mydir->tdf_bin=true;

  return d;
}
struct dirent64 *readdir64(DIR *dir){
  INIT(readdir64);
  log_error("readdir64 \n");exit(9);
  log_mthd_invoke(readdir64);
  if (is_mydir(dir)){
    struct mydirent *e=next_dirent(dir);
    return e?&e->e64:NULL;
  }
  log_mthd_orig(readdir64);
  return orig_readdir64(dir);
}
struct dirent *readdir(DIR *dir){
    INIT(readdir);
  log_mthd_invoke(readdir);
  //log_entered_function("readdir fd=%d is_mydir=%s\n",dirfd(dir),yes_no(is_mydir(dir)));
  if (is_mydir(dir)){
    struct mydirent *e=next_dirent(dir);
    return e?&e->e:NULL;
  }
  log_mthd_orig(readdir);
  return orig_readdir(dir);
}
static void *opendir_common(const char *path){
  INIT(opendir);
  struct dirent *dirent;
  assert(need_cache_dir(path));
  SYNCHRONIZE_DIR(struct mydir *mydir=getmydir(GET,path));
  //strcpy(mydir->path,path); // DEBUG_NOW
  //log_entered_function("opendir_common mydir=%s\n",yes_no(!!mydir));
  if (!mydir){
    DIR *dir=orig_opendir(path);
    pthread_mutex_lock(&_mutex_dir);
    mydir=getmydir(CREATE,path);
    assert(is_mydir(mydir));
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
  char path[MAX_PATHLEN],buf[99];
  *path=0;
  if (fd>2 && !path_for_fd(path,fd,buf) && need_cache_dir(path)) return opendir_common(path);
  log_mthd_orig_p(fdopendir,path);
  return orig_fdopendir(fd);
}
int closedir(DIR *dir){
  INIT(closedir);
  log_mthd_invoke(closedir);
  if (is_mydir(dir)){

    //MYDIR();if (file_ends_tdf_bin(mydir->path)){ // DEBUG_NOW
    //if (!mydir->tdf_bin || !mydir->tdf){log_error("%s tdf_bin=%d tdf=%d \n",mydir->path,mydir->tdf_bin,mydir->tdf);exit(9);}

    return 0;
  }
  log_mthd_orig(closedir);
  return orig_closedir(dir);
}
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
/********************************************************************************/
/* *** fstatfs *** */
/********************************************************************************/
/*
  ssize_t readlink(const char *path, char *buf, size_t bufsiz){
  const ssize_t n=orig_readlink(path,buf,bufsiz);
  log_mthd_orig(readlink);
  log("path='");
  if (n>0) write(2,path,MIN(bufsiz,n));
  log("'\n");
  return n;
  }
*/
/********************************************************************************/
#define X_FOR_PATH(STRUCT)                                              \
  int _ ## STRUCT ## _l=0;                                              \
  struct STRUCT *_store_ ## STRUCT[1024]={0};                           \
  struct STRUCT *STRUCT ## _for_path(const char *path,struct ht *ht){   \
    pthread_mutex_lock(&_mutex_ht);                                     \
    struct STRUCT *s=ht_get(ht,path);                                   \
    if (!s){                                                            \
      d1_d2(_ ## STRUCT ## _l++);                                       \
      if (!_store_ ## STRUCT[d1]) _store_ ## STRUCT[d1]=malloc(DIM*sizeof(struct STRUCT)); \
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
int fstatfs(int fd,struct statfs *buf){
  log_mthd_invoke(fstatfs);
  //      log_mthd_orig(fstatfs);      return orig_fstatfs(fd,buf);
  char path[MAX_PATHLEN],sbuf[99]; *path=0;
  //path_for_fd(path,fd,sbuf);log_entered_function("fstatfs %d  %s \n",fd,path);
  if (fd>2 && !path_for_fd(path,fd,sbuf) && need_cache_fstatfs(path)){
    struct statfs *s=statfs_for_path(path,_ht_fstatat);
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
  char path[MAX_PATHLEN], sbuf[99];
  path_for_fd(path,fildes,sbuf);
  int len=my_strlen(path);
  path[len]='/';
  strcpy(path+len+1,name);
  //log_entered_function("\n__fxstatat %d %d %s  path=%s\n",ver,fildes,name,path);
  if (need_cache_fxstatat(path)){
    struct stat *s=stat_for_path(path,_ht_fstatat);
    if (!s->st_ino){
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
  if (!DO_NOTHING) log_debug_now("need_cache_fxstatat:false  Calling __fxstatat %d %s\n",fildes,path);
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
    struct stat *s=stat_for_path(path,_ht_stat);
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
    my_file_checks(path,s);
    return 0;
  }
  log_mthd_orig_p(__xstat,path);

  const int res=orig___xstat(ver,path,statbuf);
  if (!res)     my_file_checks(path,statbuf);
  return res;
}

/********************************************************************************/
/* *** Testing *** */
int list_files(const char *path){
  struct dirent *entry;
  DIR *dp=opendir(path);
  log(ANSI_INVERSE"listdir %s"ANSI_RESET"\n",path);
  if (dp==NULL){perror("opendir");return -1;}
  while((entry=readdir(dp))) log("%s  ",entry->d_name);
  closedir(dp);
  log(" ");
  return 0;
}
int xxxmain(int argc,char *argv[]){
  {
    //free("");
  }
  log_debug_now("opendir  %p %p\n",opendir,orig_opendir);
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
