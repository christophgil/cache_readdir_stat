#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <dlfcn.h>
#include <stdio.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/vfs.h>
#include <sys/statfs.h>
// /usr/include/x86_64-linux-gnu/sys/stat.h
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include "ht.c"
//#include <sys/xattr.h>
//  (global-set-key (kbd "<f2>") '(lambda() (interactive) (switch-to-buffer "cache_readdir_stat.c")))
// See /usr/include/dirent.h

// many __fxstatat(  / .ciopfs
/***********************************************/
/* Debug */
#include <execinfo.h>
#include <signal.h>

void my_backtrace(){ /*  compile with options -g -rdynamic */
  void *array[10];
  size_t size=backtrace(array,10);
  backtrace_symbols_fd(array,size,STDOUT_FILENO);
}
void handler(int sig) {
  printf( "ZIPsFS Error: signal %d:\n",sig);
  my_backtrace();
  abort();
}
/***********************************************/
/* Mutex */
static pthread_mutexattr_t _mutex_attr_recursive;
static pthread_mutex_t _mutex_dir,_mutex_fstatfs;
#define SYNCHRONIZE_DIR(code)  pthread_mutex_lock(&_mutex_dir);code;pthread_mutex_unlock(&_mutex_dir)
/***********************************************/
#define ANSI_FG_GREEN "\x1B[32m"
#define ANSI_FG_RED "\x1B[31m"
#define ANSI_FG_MAGENTA "\x1B[35m"
#define ANSI_FG_GRAY "\x1B[30;1m"
#define ANSI_FG_BLUE "\x1B[34;1m"
#define ANSI_FG_WHITE "\x1B[37m"
#define ANSI_INVERSE "\x1B[7m"
#define ANSI_RESET "\x1B[0m"
const char* snull(const char *s){ return s?s:"Null";}
static inline char *yes_no(int i){ return i?"Yes":"No";}
static bool endsWith(const char* s,const char* e){
  if (!s || !e) return false;
  const int sn=strlen(s),en=strlen(e);
  return en<=sn && 0==strcmp(s+sn-en,e);
}
#define log(...)  fprintf(stderr,__VA_ARGS__)


#define log_calling(...)  log(ANSI_FG_MAGENTA" CALLING "ANSI_RESET),log(__VA_ARGS__)
#define log_failed(...)  log(ANSI_FG_RED" Failed "ANSI_RESET),log(__VA_ARGS__)
#define log_warn(...)  log(ANSI_FG_RED" Warn "ANSI_RESET),log(__VA_ARGS__)
#define log_error(...)  log(ANSI_FG_RED" Error "ANSI_RESET),log(__VA_ARGS__)
#define log_succes(...)  log(ANSI_FG_GREEN" Success "ANSI_RESET),log(__VA_ARGS__)
#define log_debug_now(...)  log(ANSI_FG_MAGENTA" Debug "ANSI_RESET),log(__VA_ARGS__)
#define log_entered_function(...)   log(ANSI_INVERSE"> > > >"ANSI_RESET),printf(__VA_ARGS__)
#define log_exited_function(...)   log(ANSI_INVERSE"< < < <"ANSI_RESET),printf(__VA_ARGS__)
#define SHIFT_INODE_ROOT 40
#define SHIFT_INODE_ZIPENTRY 43

bool log_specific_path(const char *title,const char *p){
  if (p && endsWith(p,"20230126_PRO1_KTT_002_30-0046_LisaKahl_P01_VNATSerAuxpM1evoM2Glucose10mMGlycine3mM_dia_BC7_1_12094.d")){
    log(ANSI_FG_MAGENTA"log_specific_path %s %s\n",title,p);
    return true;
  }
  return false;
}


#define log_mydir(d,...) log(__VA_ARGS__),log(ANSI_FG_MAGENTA" %d < %d <%d\n"ANSI_RESET,d->begin,d->i,d->end)
void log_file_stat(const char * name,const struct stat *s){
  char *color= (s->st_ino>(1L<<SHIFT_INODE_ROOT)) ? ANSI_FG_MAGENTA:ANSI_FG_BLUE;
  printf("%s  size=%lu blksize=%lu blocks=%lu links=%lu inode=%s%lu"ANSI_RESET" dir=%s uid=%u gid=%u ",name,s->st_size,s->st_blksize,s->st_blocks,   s->st_nlink,color,s->st_ino,  yes_no(S_ISDIR(s->st_mode)), s->st_uid,s->st_gid);
  //st_blksize st_blocks f_bsize
  putchar(  S_ISDIR(s->st_mode)?'d':'-');
  putchar( (s->st_mode&S_IRUSR)?'r':'-');
  putchar( (s->st_mode&S_IWUSR)?'w':'-');
  putchar( (s->st_mode&S_IXUSR)?'x':'-');
  putchar( (s->st_mode&S_IRGRP)?'r':'-');
  putchar( (s->st_mode&S_IWGRP)?'w':'-');
  putchar( (s->st_mode&S_IXGRP)?'x':'-');
  putchar( (s->st_mode&S_IROTH)?'r':'-');
  putchar( (s->st_mode&S_IWOTH)?'w':'-');
  putchar( (s->st_mode&S_IXOTH)?'x':'-');
  putchar('\n');
}

int isPowerOfTwo(unsigned int n){ return n && (n&(n-1))==0;}
void _log_mthd(char *s,int count){ if (isPowerOfTwo(count)) log(" %s=%d ",s,count);}
#define log_mthd_invoke(s) static int __count=0;_log_mthd(ANSI_FG_GRAY #s ANSI_RESET,++__count);
#define log_mthd_orig(s) static int __count_orig=0;_log_mthd(ANSI_FG_BLUE #s ANSI_RESET,++__count_orig);
#define log_mthd_orig_p(s,path) static int __count_orig=0;_log_mthd(ANSI_FG_BLUE #s ANSI_RESET,++__count_orig),log_specific_path(#s,path)
/***********************************************/
/* *** Customize *** */
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
bool is_PRO123(const char *path){
  if (strstr(path,"/PRO1") || strstr(path,"/PRO2") || strstr(path,"/PRO3") || strstr(path,"/TIMS2")) return true;


  return false;
}
bool need_cache_dir(const char *path){
  const int l=path?strlen(path):0;
  if (!l) return false;
  if (path_contains_bruker(path,l)) return true;
  if (is_PRO123(path)) return true;
  //log_debug_now(" NOT need_cache_dir %s \n",path);
  return false;
}
bool need_cache_fstatfs(const char *path){
  const int l=path?strlen(path):0;
  return path_contains_bruker(path,l) || is_PRO123(path);
}
bool need_cache_xstat(const char *p){  // This breaks diann
  const int l=strlen(p);
  return path_contains_bruker(p,l) || is_PRO123(p);
}

/***********************************************/
#define DIM_STATFS 1024
#define DIM 1024
#define DIM_SHIFT 10
#define DIM_AND 1023
#define MAX_PATHLEN 1024
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
static int (*orig_stat)(const char *path,struct stat *s);
static int (*orig_fstat)(int fd,struct stat *s);
static int (*orig___xstat)(int ver, const char *filename,struct stat *b);
static ssize_t (*orig_readlink)(const char *path, char *buf, size_t bufsiz);
static int (*orig___fxstatat)(int ver, int fildes, const char *path,struct stat *buf, int flag);

#define F0(ret,m,...) static ret (*orig_ ## m)(__VA_ARGS__);ret m(__VA_ARGS__){log_mthd_orig(m);return orig_ ## m(AA);}
#define FP(ret,m,...) static ret (*orig_ ## m)(__VA_ARGS__);ret m(__VA_ARGS__){log_mthd_orig_p(m,path);return orig_ ## m(AA);}

#define AA path,mode
FP(int,access,const char *path,int mode)
#undef AA

#define AA path,buf
  FP(int,statfs,const char *path,struct statfs *buf);
#undef AA

#define AA path,buf
FP(int,stat,const char *path,struct stat *buf);
FP(int,lstat,const char *path,struct stat *buf);
FP(int,stat64,const char *path,struct stat64 *buf);
FP(int,lstat64,const char *path,struct stat64 *buf);

#undef AA

#define AA fd,buf
F0(int,fstat,int fd,struct stat *buf);
F0(int,fstat64,int fd,struct stat64 *buf);
#undef AA


#define AA fd,path,buf,flag
FP(int,fstatat,int fd, const char *path, struct stat *buf,int flag);
FP(int,fstatat64,int fd, const char *path, struct stat64 *buf,int flag);
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



#include <sys/xattr.h>

/***********************************************/
/* *** Utils *** */

// #pragma GCC diagnostic ignored "-Wunused-variable" //__attribute__((unused));
unsigned int my_strlen(const char *s){ return !s?0:strnlen(s,MAX_PATHLEN);}

static bool is_dir(const char *f){
  struct stat st={0};
  return !lstat(f,&st) && S_ISDIR(st.st_mode);
}
static int path_for_fd(char *path, int fd,char *buf){
  *path=0;
  sprintf(buf,"/proc/%d/fd/%d",getpid(),fd);
  const ssize_t n=readlink(buf,path, MAX_PATHLEN-1);
  if (n<0){
    log_error("path_for_fd %s\n",buf);
    perror(" ");
    return -1;
  }
  //log_succes("path_for_fd %d %s \n",fd,path);
  return path[n]=0;
}
/* **********************************************/
#define DIM_DIRENT 1024
#define DIM_DIR 1000
#define d1_d2(i) const int d1=i>>DIM_SHIFT, d2=i&DIM_AND
#define MAX_PATHLEN_MYDIR 255
struct mydir{int begin,end,i;  /*  char path[MAX_PATHLEN_MYDIR+1];*/};
struct mydir _store_dir[DIM_DIR];
#define MYDIR() struct mydir *mydir=(struct mydir*)dir
struct ht *_ht_dir, *_ht_statfs, *_ht_stat, *_ht_fstatat;
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
  //log_debug_now("push_dirent %p %s\n",e,e->d_name);
  if (!_store_dirent[d1]) _store_dirent[d2]=malloc(DIM*sizeof(struct mydirent));
  _store_dirent[d1][d2].e=*e;
  _store_dirent[d1][d2].e64=*e64;
  return _dirent_l++;
}
static void *my_dlsym(const char *symbol){
  void *s=dlsym(RTLD_NEXT,symbol);
  log_debug_now("dlsym %s %p\n",symbol,s);
  if (!s) log_error("dlsym  %s\n",dlerror());
  return s;
}
void my_init(void) __attribute__((constructor));
void my_init(void){
  pthread_mutexattr_init(&_mutex_attr_recursive);
  pthread_mutexattr_settype(&_mutex_attr_recursive,PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&_mutex_fstatfs,&_mutex_attr_recursive);
  pthread_mutex_init(&_mutex_dir,&_mutex_attr_recursive);
  _ht_statfs=ht_create(333);
  _ht_dir=ht_create(333);
  _ht_stat=ht_create(333);
  _ht_fstatat=ht_create(333);
  log_debug_now("my_init\n");
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
  F(stat64);
  F(fstat);
  F(fstat64);
  F(lstat);
  F(lstat64);
  F(fstatat);
  F(fstatat64);

  F(__fxstat);
  F(__xstat);
  F(__lxstat);
  F(__fxstatat);

  F(readlink);
  F(statx);
  F(getxattr);
  F(fgetxattr);
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
  log_mthd_invoke(readdir64);
  if (is_mydir(dir)){
    struct mydirent *e=next_dirent(dir);
    return e?&e->e64:NULL;
  }
  log_mthd_orig(readdir64);
  return orig_readdir64(dir);
}
struct dirent *readdir(DIR *dir){
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
  struct dirent *dirent;
  assert(need_cache_dir(path));
  SYNCHRONIZE_DIR(struct mydir *mydir=getmydir(GET,path));
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
  log_mthd_invoke(opendir);
  if (need_cache_dir(path)) return  opendir_common(path);
  log_mthd_orig_p(opendir,path);
  return orig_opendir(path);
}
DIR *fdopendir(int fd){
  log_mthd_invoke(fdopendir);
  char path[MAX_PATHLEN],buf[99];
  *path=0;
  if (fd>2 && !path_for_fd(path,fd,buf) && need_cache_dir(path)) return opendir_common(path);
  log_mthd_orig_p(fdopendir,path);
  return orig_fdopendir(fd);
}
int closedir(DIR *dir){
  log_mthd_invoke(closedir);
  if (is_mydir(dir)) return 0;
  log_mthd_orig(closedir);
  return orig_closedir(dir);
}
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
    log_error("dirfd not implemented \n");
    exit(-1);
  }
  return orig_dirfd(dir);
}
/********************************************************************************/
/* *** fstatfs *** */
struct statfs *_store_statfs[DIM_STATFS]={0};
int _statfs_l=0;
int fstatfs(int fd,struct statfs *buf){
  log_mthd_invoke(fstatfs);
  //      log_mthd_orig(fstatfs);      return orig_fstatfs(fd,buf);
  char path[MAX_PATHLEN],sbuf[99]; *path=0;
  //path_for_fd(path,fd,sbuf);log_entered_function("fstatfs %d  %s \n",fd,path);
  if (fd>2 && !path_for_fd(path,fd,sbuf) && need_cache_fstatfs(path)){
    int res=-1;
    struct statfs *s=ht_get(_ht_statfs,path);
    if (s){
      *buf=*s;
      res=s->f_type==-1?-1:0;
    }else{
      d1_d2(_statfs_l);
      if (!_store_statfs[d1]) _store_statfs[d1]=malloc(DIM*sizeof(void*));
      ht_set(_ht_statfs,path,s=_store_statfs[d1]+d2);
      s->f_type=-1;
      assert(s->f_type==-1);
      log_mthd_orig_p(fstatfs,path);
      if ((res=orig_fstatfs(fd,s))){
        log_error("__xstat %d %s pid=%d\n",fd,path,getpid());
        perror(" ");
      }else{
        *buf=*s;
      }
      _statfs_l++;
    }
    return res;
  }
  log_mthd_orig_p(fstatfs,path);
  return orig_fstatfs(fd,buf);
}

/********************************************************************************/
ssize_t readlink(const char *path, char *buf, size_t bufsiz){
  // const ssize_t n=orig_readlink(path,buf,bufsiz);
  ssize_t n=0;
  //if (log_mthd_orig(readlink)){
  //log("path=");
  //    if (n>0) write(path,2,MIN(bufsiz,n));
  //}
  return n;
}

struct stat *_store_stat[DIM_STATFS]={0};
int _stat_l=0;

struct stat *stat_for_path(const char *path,struct ht *ht){
  struct stat *s=ht_get(ht,path);
  if (!s){
    d1_d2(_stat_l++);
    if (!_store_stat[d1]) _store_stat[d1]=malloc(DIM*sizeof(void*));
    s=_store_stat[d1]+d2;
    log_mthd_orig_p(__xstat,path);
    memset(s,0,sizeof(struct stat));
    ht_set(ht,path,s=_store_stat[d1]+d2);
  }
  return s;
}



int __fxstatat(int ver, int fildes, const char *name,struct stat *statbuf, int flag){
  //return orig___fxstatat(ver,fildes,name,statbuf,flag);
  char path[MAX_PATHLEN], sbuf[99];
  path_for_fd(path,fildes,sbuf);
  int len=my_strlen(path);
  path[len]='/';
  strcpy(path+len+1,name);
  //log_entered_function("\n__fxstatat %d %d %s  path=%s\n",ver,fildes,name,path);
  struct stat *s=stat_for_path(path,_ht_fstatat);;
  if (!s->st_ino){
    log_mthd_orig_p(__fxstatat,path);
    if (orig___fxstatat(ver,fildes,name,statbuf,flag)){
      log_error("__fxstatat %d %s\n",fildes,path);
      perror(" ");
      s->st_ino=-1;
    }
  }
  if (s->st_ino==-1) return -1;
  *statbuf=*s;
  return 0;
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
        log_error("__xstat %d %s\n",ver,path);
        perror(" ");
        s->st_ino=-1;
        assert(s->st_ino==-1);
      }
    }
    if (s->st_ino==-1) return -1;
    *statbuf=*s;
    return 0;
  }
  log_mthd_orig_p(__xstat,path);
  return orig___xstat(ver,path,statbuf);
}
/********************************************************************************/


//      !strcmp(symbol,"stat")?"__xstat":
//      !strcmp(symbol,"lstat")?"__lxstat":
//      !strcmp(symbol,"fstat")?"__fxstat":NULL;
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
int xxxmain(int argc,char *argv[]){
  signal(SIGSEGV,handler);
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
        print_dir_listing(path);
      }else{
        if (!fstatfs(fd,&bufstatfs)) log_succes("fstatfs\n");
      }
    }
    close(fd);
  }
  return 0;
}
