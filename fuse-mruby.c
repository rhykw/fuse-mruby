
#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <mruby.h>
#include <mruby/compile.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/proc.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/variable.h>
#include <mruby/hash.h>
#include <mruby/error.h>
#include <unistd.h>

#include <stdio.h>
#include <syslog.h>

/* mrb_state *mrb;*/
mrbc_context *mrbc;

#define MRB_FUSE_ERR_CHECK \
  if( mrb->exc ){\
    char *p;\
    mrb_value exc = mrb_obj_value(mrb->exc);\
    mrb_value inspect = mrb_inspect(mrb, exc);\
    p = mrb_str_to_cstr(mrb, inspect);\
    syslog(LOG_NOTICE, "%s: %s", __FUNCTION__ , p);\
    mrb->exc = 0;\
    return -EACCES;\
  }

#define MRB_FUSE_PRIVATE_DATA ((struct mrb_fuse_private_data *) fuse_get_context()->private_data)


struct mrb_fuse_private_data {
    mrb_state *mrb;
};


static int mrb_fuse_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;
    mrb_state *mrb = MRB_FUSE_PRIVATE_DATA->mrb;

    memset(stbuf, 0, sizeof(struct stat));

    syslog(LOG_NOTICE, "getattr path is %s", path);

    if(strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return res;
    }

    mrb_value vh = mrb_funcall(mrb, mrb_top_self(mrb), "readdir", 1, mrb_str_new_cstr(mrb,path) );
    if ( ! mrb_hash_p( vh ) ){
        res = -ENOENT;
        return res;
    }

    mrb_sym kmode = mrb_intern_cstr(mrb,"mode");
    mrb_value vmode = mrb_hash_get( mrb, vh, mrb_symbol_value(kmode) );

    if( !mrb_fixnum_p(vmode) ){
      mrb_value vstrinspect = mrb_funcall(mrb, vmode, "inspect", 0);
      syslog(LOG_NOTICE, "path %s has invalid mode. %s", path,mrb_string_value_ptr(mrb, vstrinspect));
      res = -ENOENT;
      return res;
    }

    stbuf->st_mode = mrb_fixnum(vmode);
    stbuf->st_nlink = 1;

    mrb_value vsize = mrb_hash_get( mrb, vh, mrb_symbol_value(mrb_intern_cstr(mrb,"size")) );
    if( mrb_fixnum_p(vsize) ){
      stbuf->st_size  = mrb_fixnum(vsize);
    }


    mrb_value vmtime = mrb_hash_get( mrb, vh, mrb_symbol_value(mrb_intern_cstr(mrb,"mtime")) );
MRB_FUSE_ERR_CHECK;
    mrb_value vvv = mrb_funcall(mrb,vmtime,"to_i",0);
MRB_FUSE_ERR_CHECK;
    if( mrb_fixnum_p(vvv) ){
      stbuf->st_mtime  = mrb_fixnum(vvv);
    }

/*
    else if(strcmp(path, "/dir") == 0 ){
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else if(strcmp(path, "/file") == 0) {
        mrb_value v = mrb_funcall(mrb, mrb_top_self(mrb), "rand", 1, mrb_fixnum_value(1024) );
        

        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size  = mrb_fixnum(v);
    }
    else
        res = -ENOENT;
*/
    return res;
}


static int mrb_fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;
    mrb_state *mrb = MRB_FUSE_PRIVATE_DATA->mrb;
/*
    if(strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, "file", NULL, 0);
    filler(buf, "dir", NULL, 0);
*/

    mrb_value vh = mrb_funcall(mrb, mrb_top_self(mrb), "readdir", 1, mrb_str_new_cstr(mrb,path) );

    mrb_value keys = mrb_hash_keys( mrb, vh );
    int len = RARRAY_LEN( keys );
    int i =0;
    printf( "\nhash_keys_len=%d\n", len );
    for ( i = 0; i < len; ++i ){
      mrb_value key = mrb_ary_ref( mrb, keys, i );
      if ( mrb_string_p( key ) ){
        filler(buf, RSTRING_PTR(key), NULL, 0);
      }
    }
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    return 0;
}

static int mrb_fuse_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    mrb_state *mrb = MRB_FUSE_PRIVATE_DATA->mrb;

    /* syslog(LOG_NOTICE, "%s:%d:private_data=%d", __FUNCTION__,__LINE__ , private_data ); */

    mrb_value vh = mrb_funcall(mrb, mrb_top_self(mrb), "create", 1, mrb_str_new_cstr(mrb,path) );
    MRB_FUSE_ERR_CHECK;
    int fd = open("/dev/null", fi->flags, mode);
    fi->fh = fd;
    return 0;
}

static int mrb_fuse_open(const char *path, struct fuse_file_info *fi)
{
/*
    if(strcmp(path, "/file") != 0)
        return -ENOENT;

    if((fi->flags & 3) != O_RDONLY)
        return -EACCES;
*/
    int fd = open("/dev/null", fi->flags);
    fi->fh = fd;

    return 0;
}

static int mrb_fuse_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    size_t len;
    char *str = "aaaaaaaa";
    (void) fi;

    if(strcmp(path, "/entries.rb") == 0){
        mrb_state *mrb = MRB_FUSE_PRIVATE_DATA->mrb;
        mrb_value v = mrb_funcall(mrb, mrb_top_self(mrb), "entries_json", 0);
        str = mrb_str_to_cstr(mrb, v);
    }

    len = strlen(str);

syslog(LOG_NOTICE, "%s:%d:path=%s,size=%d,offset=%d,len=%d", __FUNCTION__,__LINE__ , path,size,offset,len );

    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, str + offset, size);
    } else
        size = 0;

    return size;
}

static int mrb_fuse_write(const char *path,const char *buf,size_t size,
  off_t offset,struct fuse_file_info *fi)
{
    (void) buf;
    (void) offset;
    (void) fi;
    return size;

}

static int mrb_fuse_truncate(const char *path, off_t size)
{
    (void) size;

/*
    if(strcmp(path, "/") != 0)
        return -ENOENT;
*/
    return 0;
}

static int mrb_fuse_utimens(const char *path, const struct timespec ts[2])
{
    int res;
    mrb_state *mrb = MRB_FUSE_PRIVATE_DATA->mrb;
/*
    struct timeval tv[2];

    tv[0].tv_sec = ts[0].tv_sec;
    tv[0].tv_usec = ts[0].tv_nsec / 1000;
    tv[1].tv_sec = ts[1].tv_sec;
    tv[1].tv_usec = ts[1].tv_nsec / 1000;
*/

    double s1 = (double) ts[0].tv_nsec / 1000 / 1000 / 1000;
    double tta = (double) ts[0].tv_sec + s1;

syslog(LOG_NOTICE, "%s:%d:tv_sec = %d , tta = %.4f", __FUNCTION__,__LINE__ , ts[0].tv_sec , tta);

/*
 *
    float ttu = (float) ts[1].tv_sec + ts[1].tv_nsec / 1000 / 1000 / 1000;
*/
#if 0

    mrb_value v1 = mrb_funcall(mrb, mrb_top_self(mrb), "Time.at", 1, mrb_float_value(mrb,tta) );
    MRB_FUSE_ERR_CHECK;

#endif

/*
    mrb_value v2 = mrb_funcall(mrb, mrb_top_self(mrb), "Time.at", 1, mrb_float_value(mrb,ttu) );
*/
    mrb_value vh = mrb_funcall(mrb, mrb_top_self(mrb), "utimesf", 2, mrb_str_new_cstr(mrb,path), mrb_float_value(mrb,tta));
MRB_FUSE_ERR_CHECK;
/*
    res = utimes(path, tv);
    if (res == -1)
        return -errno;
*/
    return 0;
}

static int mrb_fuse_release(const char *path, struct fuse_file_info *fi)
{
    (void) path;

    if( fi->fh )
        close(fi->fh);

    return 0;
}

static int mrb_fuse_unlink(const char *path)
{
    mrb_state *mrb = MRB_FUSE_PRIVATE_DATA->mrb;
    mrb_value vh = mrb_funcall(mrb, mrb_top_self(mrb), "unlink", 1, mrb_str_new_cstr(mrb,path) );
    MRB_FUSE_ERR_CHECK;
    return 0;
}

static struct fuse_operations mrb_fuse_op = {
    .getattr	= mrb_fuse_getattr,
    .readdir	= mrb_fuse_readdir,
    .open	= mrb_fuse_open,
    .read	= mrb_fuse_read,
    .write      = mrb_fuse_write,
    .create     = mrb_fuse_create,
    .truncate   = mrb_fuse_truncate,
    .utimens    = mrb_fuse_utimens,
    .release    = mrb_fuse_release,
    .unlink	= mrb_fuse_unlink,
};

int main(int argc, char *argv[])
{
    mrb_state *mrb;
    mrb = mrb_open();
    /* install_mrb_class(mrb); */
    mrb_gc_arena_restore(mrb, 0);
    mrbc = mrbc_context_new(mrb);

    struct mrb_fuse_private_data *private_data;
    private_data = malloc(sizeof(struct mrb_fuse_private_data));
    private_data->mrb = mrb;

    FILE *fp = fopen("fuse-mruby.rb", "r");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open %s\n", "file");
    }else {
        mrb_load_file_cxt(mrb, fp, mrbc);
        fclose(fp);
        fp = NULL;
        if (mrb->exc) {
          mrb_print_error(mrb);
          mrb->exc = 0;
          return 1;
        }
    }
    fp = NULL; 

    openlog("ABCD128Z", LOG_CONS | LOG_PID, LOG_USER);
    syslog(LOG_NOTICE, "%s", "I am fuse-mruby.");

    return fuse_main(argc, argv, &mrb_fuse_op, private_data );
}
