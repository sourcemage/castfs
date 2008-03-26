#ifndef CAST_FILESYS_H
#define CAST_FILESYS_H

#define _GNU_SOURCE

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#define CAST_DBG_MAIN (1<<0)
#define CAST_DBG_SYS  (1<<1)
#define CAST_DBG_UTIL (1<<2)
#define CAST_DBG_PATHS (1<<3)

struct st_cast_path
{
	char *root_path;
	char *stage_path;
	unsigned int is_deleted;
	unsigned int is_staged;
	unsigned int is_ignored;
	struct st_cast_path *next;
};

typedef struct st_cast_path cast_paths;
typedef cast_paths* cast_paths_ptr;

int logfd;
int dbglvl;

char *mount_path;
char *stage_path;
char **ignored_dirs;
int ignored_dirs_len;

void usage(void);
int cast_log(unsigned int dbgmask, const char* format,...);
int cast_copy_file(const char *from, const char *to);
int cast_paths_log(cast_paths_ptr paths);
int cast_mkdir_rec_staged(cast_paths_ptr dir);
int parse_mount_options(char *str);
void strip_out_arguments(char *arg);
int shouldIgnore(const char *path);

#endif
