/*
 * castfs.h
 */

#include "castfs.h"
#include "hash.h"

/* getattr(): see man fstat or man lstat */
static int cast_getattr(const char *path, struct stat *stbuf)
{
//fprintf(stderr, "cast_getattr: %s\n", path);
	int res;
	char *cpath = strdup(path);
	cast_paths_ptr paths = castHashGetValueOf(cpath);

	cast_log(CAST_DBG_SYS, "getattr %s\n", cpath);
	cast_paths_log(paths);
	if (paths && paths->is_staged)
		res = lstat(paths->stage_path, stbuf);
	else
		res = lstat(cpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

/* shouldn't this be nearly identical to above?  This won't stage, will it? */
static int cast_fgetattr(const char *path, struct stat *stbuf,
			 struct fuse_file_info *fi)
{
//fprintf(stderr, "cast_fgetattr: %s\n", path);
	int res;
	cast_log(CAST_DBG_SYS, "fgetattr %s\n", path);
#if 1	// 0=old behavior; 1=new behavior
	char *cpath = strdup(path);
	cast_paths_ptr paths = castHashGetValueOf(cpath);

	cast_log(CAST_DBG_SYS, "\tfgetattr %s\n", cpath);
	cast_paths_log(paths);
	if (paths && paths->is_staged)
		res = lstat(paths->stage_path, stbuf);
		// Not sure this is correct, but I think it is! --D.E.
	else
		//res = fstat(cpath, stbuf);
#endif
		res = fstat(fi->fh, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int cast_access(const char *path, int mask)
{
//fprintf(stderr, "cast_access: %s\n", path);
	int res;
	char *cpath = strdup(path);
	cast_paths_ptr paths = castHashGetValueOf(cpath);
	cast_log(CAST_DBG_SYS, "access %s\n", cpath);
	cast_paths_log(paths);

	if (paths && paths->is_staged)
		res = access(paths->stage_path, mask);
	else
		res = access(cpath, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int cast_readlink(const char *path, char *buf, size_t size)
{
//fprintf(stderr, "cast_readlink: %s\n", path);
	int res;
	char *cpath = strdup(path);
	cast_paths_ptr paths = castHashGetValueOf(cpath);
	cast_log(CAST_DBG_SYS, "readlink %s\n", cpath);
	cast_paths_log(paths);

	if (paths && paths->is_staged)
		res = readlink(paths->stage_path, buf, size - 1);
	else
		res = readlink(cpath, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}

static int cast_opendir(const char *path, struct fuse_file_info *fi)
{
//fprintf(stderr, "cast_opendir: %s\n", path);
	DIR *dp;
	char *cpath = strdup(path);
	cast_paths_ptr paths = castHashGetValueOf(cpath);
	cast_log(CAST_DBG_SYS, "opendir %s\n", cpath);
	cast_paths_log(paths);

	if (paths && paths->is_staged)
		dp = opendir(paths->stage_path);
	else
		dp = opendir(cpath);
	if (dp == NULL)
		return -errno;
	fi->fh = (unsigned long)dp;
	return 0;
}

/* FIXME: This appears to be old-style fuse.  Please update to current. */
static int cast_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			off_t offset, struct fuse_file_info *fi)
{
//fprintf(stderr, "cast_readdir: %s\n", path);
	DIR *dp = opendir(path);
	char *cpath = strdup(path);
	cast_paths_ptr paths = castHashGetValueOf(cpath);
	cast_paths_ptr delcheck;
	char pathbuf[1024];
	struct dirent *de;
	cast_log(CAST_DBG_SYS, "readdir %s\n", cpath);
	cast_paths_log(paths);

	/* still does some funky stuff with reading the same dirent twice
	 * which the outputs in an ls call twice */
	if (dp) {
		seekdir(dp, offset);
		while ((de = readdir(dp)) != NULL) {
			struct stat st;
			memset(&st, 0, sizeof(st));
			st.st_ino = de->d_ino;
			st.st_mode = de->d_type << 12;
			pathbuf[0] = '\0';
			strcat(pathbuf, cpath);
			strcat(pathbuf, "/");
			strcat(pathbuf, de->d_name);
			delcheck = NULL;
			delcheck = castHashGetValueOf(pathbuf);
			cast_log(CAST_DBG_SYS, "reading %s\n", pathbuf);
			if (!(delcheck && delcheck->is_deleted)) {
				//if (filler(buf, de->d_name, &st, telldir(dp)))
				if (filler(buf, de->d_name, &st, 0)) {
					break;
				}
			}
		}
		closedir(dp);
	}
	if (paths && strcmp(paths->stage_path, paths->root_path)) {
		dp = opendir(paths->stage_path);
		if (!dp)
			return 0;
		seekdir(dp, offset);
		while ((de = readdir(dp)) != NULL) {
			struct stat st;
			memset(&st, 0, sizeof(st));
			st.st_ino = de->d_ino;
			st.st_mode = de->d_type << 12;
			pathbuf[0] = '\0';
			strcat(pathbuf, cpath);
			strcat(pathbuf, "/");
			strcat(pathbuf, de->d_name);
			delcheck = NULL;
			delcheck = castHashGetValueOf(pathbuf);
			cast_log(CAST_DBG_SYS, "stage reading %s\n", pathbuf);
			if (delcheck && delcheck->is_staged) {
				// this is the old conditional not sure about this
				// and why it doesn't work I'll have to email fuse
				//if (filler(buf, de->d_name, &st, telldir(dp)))
				if (filler(buf, de->d_name, &st, 0)) {
					break;
				}
			}
		}
		closedir(dp);
	}
	return 0;
}

static int cast_mknod(const char *path, mode_t mode, dev_t rdev)
{
//fprintf(stderr, "cast_mknod: %s\n", path);
	int res;
	char *cpath = strdup(path);
	cast_paths_ptr paths = castHashSetValueOf(cpath);
	cast_log(CAST_DBG_SYS, "mknod %s\n", cpath);
	cast_paths_log(paths);

	if (S_ISFIFO(mode))
		res = mkfifo(paths->stage_path, mode);
	else
		res = mknod(paths->stage_path, mode, rdev);
	paths->is_staged = 1;
	paths->is_deleted = 0;
	return 0;
}

static int cast_mkdir(const char *path, mode_t mode)
{
//fprintf(stderr, "cast_mkdir: %s\n", path);
	int res;
	char *cpath = strdup(path);
	cast_paths_ptr paths = castHashSetValueOf(cpath);
	char *dir = strdup(cpath);
	dir = dirname(dir);
	cast_paths_ptr dpaths = castHashSetValueOf(dir);
	cast_log(CAST_DBG_SYS, "mkdir %s\n", cpath);
	cast_paths_log(paths);

	cast_mkdir_rec_staged(dpaths);
	res = mkdir(paths->stage_path, mode);
	if (res == -1)
		return -errno;
	paths->is_staged = 1;
	paths->is_deleted = 0;
	return 0;
}

static int cast_unlink(const char *path)
{
//fprintf(stderr, "cast_unlink: %s\n", path);
	int res = 0;
	char *cpath = strdup(path);
	cast_paths_ptr paths = castHashSetValueOf(cpath);
	cast_log(CAST_DBG_SYS, "unlink %s\n", cpath);
	cast_paths_log(paths);

	if (paths->is_staged || paths->is_ignored) {
		cast_log(CAST_DBG_SYS, "actually removing file %s\n",
			 paths->stage_path);
		res = unlink(paths->stage_path);
	}
	if (res == -1)
		return -errno;
	paths->is_staged = 1;
	paths->is_deleted = 1;
	return 0;
}

static int cast_rmdir(const char *path)
{
//fprintf(stderr, "cast_rmdir: %s\n", path);
	int res = 0;
	char *cpath = strdup(path);
	cast_paths_ptr paths = castHashSetValueOf(cpath);
	cast_log(CAST_DBG_SYS, "rmdir %s\n", cpath);
	cast_paths_log(paths);

	if (paths->is_staged || paths->is_ignored) {
		res = rmdir(paths->stage_path);
	}
	if (res == -1)
		return -errno;
	paths->is_staged = 1;
	paths->is_deleted = 1;
	return 0;
}

static int cast_symlink(const char *from, const char *to)
{
//fprintf(stderr, "cast_symlink: %s\n", from);
	int res;
	char *cto = strdup(to);
	cast_paths_ptr paths = castHashSetValueOf(cto);
	char *dir = strdup(cto);
	dir = dirname(dir);
	cast_paths_ptr dpaths = castHashSetValueOf(dir);
	cast_log(CAST_DBG_SYS, "symlink %s, %s\n", from, cto);
	cast_paths_log(paths);

	cast_mkdir_rec_staged(dpaths);

	res = symlink(from, paths->stage_path);
	if (res == -1)
		return -errno;
	paths->is_staged = 1;
	paths->is_deleted = 0;
	return 0;
}

static int cast_rename(const char *from, const char *to)
{
//fprintf(stderr, "cast_rename: %s\n", from);
	int res;
	char *cto = strdup(to);
	char *cfrom = strdup(from);
	cast_paths_ptr fpaths = castHashSetValueOf(cfrom);
	cast_paths_ptr tpaths = castHashSetValueOf(cto);
	char *dir = strdup(cto);
	dir = dirname(dir);
	cast_paths_ptr dpaths = castHashSetValueOf(dir);

	cast_log(CAST_DBG_SYS, "rename %s, %s\n", cfrom, cto);
	cast_paths_log(fpaths);
	cast_paths_log(tpaths);

	cast_mkdir_rec_staged(dpaths);

	if (fpaths->is_staged || fpaths->is_ignored) {
		res = rename(fpaths->stage_path, tpaths->stage_path);
	} else {
		res = cast_copy_file(cfrom, tpaths->stage_path);
	}
	if (res == -1)
		return -errno;
	fpaths->is_staged = 1;
	fpaths->is_deleted = 1;
	tpaths->is_staged = 1;
	tpaths->is_deleted = 0;
	return 0;
}

static int cast_link(const char *from, const char *to)
{
//fprintf(stderr, "cast_link: %s\n", from);
	int res;
	char *cto = strdup(to);
	char *cfrom = strdup(from);
	cast_paths_ptr fpaths = castHashGetValueOf(cfrom);
	cast_paths_ptr tpaths = castHashSetValueOf(cto);
	char *dir = strdup(cto);
	dir = dirname(dir);
	cast_paths_ptr dpaths = castHashSetValueOf(dir);
	cast_log(CAST_DBG_SYS, "link %s, %s\n", cfrom, cto);
	cast_paths_log(tpaths);

	cast_mkdir_rec_staged(dpaths);

	if (fpaths && fpaths->is_staged) {
		res = link(fpaths->stage_path, tpaths->stage_path);
	} else {
		res = link(cfrom, tpaths->stage_path);
	}
	if (res == -1)
		return -errno;
	tpaths->is_staged = 1;
	tpaths->is_deleted = 0;
	return 0;
}

static int cast_chmod(const char *path, mode_t mode)
{
//fprintf(stderr, "cast_chmod: %s\n", path);
	int res;
	char *cpath = strdup(path);
	cast_paths_ptr paths = castHashSetValueOf(cpath);
	char *dir = strdup(cpath);
	dir = dirname(dir);
	cast_paths_ptr dpaths = castHashSetValueOf(dir);
	cast_log(CAST_DBG_SYS, "chmod %s\n", cpath);
	cast_paths_log(paths);

	if (!(paths->is_staged)) {
		cast_mkdir_rec_staged(dpaths);
		cast_copy_file(cpath, paths->stage_path);
	}

	res = chmod(paths->stage_path, mode);
	if (res == -1)
		return -errno;
	paths->is_staged = 1;
	paths->is_deleted = 0;
	return 0;
}

static int cast_chown(const char *path, uid_t uid, gid_t gid)
{
//fprintf(stderr, "cast_chown: %s\n", path);
	int res;
	char *cpath = strdup(path);
	cast_paths_ptr paths = castHashSetValueOf(cpath);
	char *dir = strdup(cpath);
	dir = dirname(dir);
	cast_paths_ptr dpaths = castHashSetValueOf(dir);
	cast_log(CAST_DBG_SYS, "chown %s\n", cpath);
	cast_paths_log(paths);

	if (!(paths->is_staged)) {
		cast_mkdir_rec_staged(dpaths);
		cast_copy_file(cpath, paths->stage_path);
	}

	res = lchown(paths->stage_path, uid, gid);
	if (res == -1)
		return -errno;

	paths->is_staged = 1;
	paths->is_deleted = 0;
	return 0;
}

static int cast_truncate(const char *path, off_t size)
{
//fprintf(stderr, "cast_truncate: %s\n", path);
	int res;
	char *cpath = strdup(path);
	cast_paths_ptr paths = castHashSetValueOf(cpath);
	char *dir = strdup(cpath);
	dir = dirname(dir);
	cast_paths_ptr dpaths = castHashSetValueOf(dir);
	cast_log(CAST_DBG_SYS, "truncate %s\n", cpath);
	cast_paths_log(paths);

	if (!(paths->is_staged)) {
		cast_mkdir_rec_staged(dpaths);
		cast_copy_file(cpath, paths->stage_path);
	}

	res = truncate(paths->stage_path, size);
	if (res == -1)
		return -errno;

	paths->is_staged = 1;
	paths->is_deleted = 0;
	return 0;
}

static int cast_ftruncate(const char *path, off_t size,
			  struct fuse_file_info *fi)
{
//fprintf(stderr, "cast_ftruncate: %s\n", path);
	int res;
	cast_log(CAST_DBG_SYS, "ftruncate %s\n", path);

	(void)path;

	res = ftruncate(fi->fh, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int cast_utime(const char *path, struct utimbuf *buf)
{
//fprintf(stderr, "cast_utime: %s\n", path);
	int res;
	char *cpath = strdup(path);
	cast_paths_ptr paths = castHashSetValueOf(cpath);
	char *dir = strdup(cpath);
	dir = dirname(dir);
	cast_paths_ptr dpaths = castHashSetValueOf(dir);
	cast_log(CAST_DBG_SYS, "utime %s\n", cpath);
	cast_paths_log(paths);

	if (!(paths->is_staged)) {
		cast_mkdir_rec_staged(dpaths);
		cast_log(CAST_DBG_SYS, "going to call copy file\n");
		cast_copy_file(cpath, paths->stage_path);
	}

	res = utime(paths->stage_path, buf);
	if (res == -1)
		return -errno;

	paths->is_staged = 1;
	paths->is_deleted = 0;
	return 0;
}

static int cast_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
//fprintf(stderr, "cast_create: %s\n", path);
	int fd;
	char *cpath = strdup(path);
	cast_paths_ptr paths = castHashSetValueOf(cpath);
	char *dir = strdup(cpath);
	dir = dirname(dir);
	cast_paths_ptr dpaths = castHashSetValueOf(dir);
	cast_log(CAST_DBG_SYS, "create %s, %o\n", cpath, mode);
	cast_paths_log(paths);

	cast_mkdir_rec_staged(dpaths);

	fd = open(paths->stage_path, fi->flags, mode);
	if (fd == -1)
		return -errno;
	paths->is_staged = 1;
	paths->is_deleted = 0;
	fi->fh = fd;
	return 0;
}

static int cast_open(const char *path, struct fuse_file_info *fi)
{
//fprintf(stderr, "cast_open: %s\n", path);
	int fd;
	char *cpath = strdup(path);
	cast_paths_ptr paths = castHashGetValueOf(cpath);
	char *dir = strdup(cpath);
	dir = dirname(dir);
	cast_paths_ptr dpaths;
	cast_log(CAST_DBG_SYS, "open %s, 0x%X\n", cpath, fi->flags);
	cast_paths_log(paths);

	if (paths && paths->is_staged) {
		cast_log(CAST_DBG_SYS, "Okay it's staged\n");
		fd = open(paths->stage_path, fi->flags);
	} else {
		if (((fi->flags) & O_WRONLY) || ((fi->flags) & O_RDWR)
		    || ((fi->flags) & O_TRUNC)) {
			cast_log(CAST_DBG_SYS,
				 "Okay it's not staged and we need to stage it\n");
			paths = castHashSetValueOf(cpath);
			dpaths = castHashSetValueOf(dir);
			cast_mkdir_rec_staged(dpaths);
			cast_copy_file(cpath, paths->stage_path);
			paths->is_staged = 1;
			fd = open(paths->stage_path, fi->flags);
		} else {
			cast_log(CAST_DBG_SYS,
				 "Okay it's not staged and we don't need to copy it\n");
			fd = open(cpath, fi->flags);
		}
	}

	if (fd == -1)
		return -errno;

	fi->fh = fd;
	return 0;
}

static int cast_read(const char *path, char *buf, size_t size, off_t offset,
		     struct fuse_file_info *fi)
{
//fprintf(stderr, "cast_read: %s\n", path);
	int res;
	cast_log(CAST_DBG_SYS, "read %s\n", path);

	(void)path;
	res = pread(fi->fh, buf, size, offset);
	if (res == -1)
		res = -errno;

	return res;
}

static int cast_write(const char *path, const char *buf, size_t size,
		      off_t offset, struct fuse_file_info *fi)
{
//fprintf(stderr, "cast_write: %s\n", path);
	int res;
	cast_log(CAST_DBG_SYS, "write %s\n", path);

	(void)path;
	res = pwrite(fi->fh, buf, size, offset);
	if (res == -1)
		res = -errno;

	return res;
}

static int cast_statfs(const char *path, struct statvfs *stbuf)
{
//fprintf(stderr, "cast_statfs: %s\n", path);
	int res;
	char *cpath = strdup(path);
	cast_paths_ptr paths = castHashGetValueOf(cpath);
	/* since fuse tends to stat the root of the mount
	 * all the time you can comment this out to get rid
	 * of some of the irrelevent debugging output */
/*    cast_log(CAST_DBG_SYS, "statfs %s\n", path);
    cast_paths_log(paths); */
	if (paths && paths->is_staged)
		res = statvfs(paths->stage_path, stbuf);
	else
		res = statvfs(cpath, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int cast_release(const char *path, struct fuse_file_info *fi)
{
//fprintf(stderr, "cast_release: %s\n", path);
	cast_log(CAST_DBG_SYS, "release %s\n", path);
	(void)path;
	close(fi->fh);

	return 0;
}

static int cast_fsync(const char *path, int isdatasync,
		      struct fuse_file_info *fi)
{
//fprintf(stderr, "cast_write: %s\n", path);
	int res;
	cast_log(CAST_DBG_SYS, "fsync %s\n", path);
	(void)path;

#ifndef HAVE_FDATASYNC
	(void)isdatasync;
#else
	if (isdatasync)
		res = fdatasync(fi->fh);
	else
#endif
		res = fsync(fi->fh);
	if (res == -1)
		return -errno;

	return 0;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int cast_setxattr(const char *path, const char *name, const char *value,
			 size_t size, int flags)
{
//fprintf(stderr, "cast_setxattr: %s\n", path);
	cast_log(CAST_DBG_SYS, "setxattr %s, %s, %s, %d, %d\n", path, name,
		 value, size, flags);
	int res = lsetxattr(path, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int cast_getxattr(const char *path, const char *name, char *value,
			 size_t size)
{
//fprintf(stderr, "cast_getxattr: %s\n", path);
	cast_log(CAST_DBG_SYS, "getxattr %s, %s, %d\n", path, name, size);
	int res = lgetxattr(path, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int cast_listxattr(const char *path, char *list, size_t size)
{
//fprintf(stderr, "cast_listxattr: %s\n", path);
	cast_log(CAST_DBG_SYS, "listxattr %s, %d\n", path, size);
	int res = llistxattr(path, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int cast_removexattr(const char *path, const char *name)
{
//fprintf(stderr, "cast_removexattr: %s\n", path);
	cast_log(CAST_DBG_SYS, "removexattr %s, %s\n", path, name);
	int res = lremovexattr(path, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static void cast_destroy(void *tmp)
{
//fprintf(stderr, "cast_destroy:\n");
	int i;
	castHashDestroy();
	for (i = 0; i < ignored_dirs_len; i++)
		free(ignored_dirs[i]);
	free(ignored_dirs);
	free(stage_path);
//	free(mount_path);
	close(logfd);
}

static inline DIR *get_dirp(struct fuse_file_info *fi)
{
//fprintf(stderr, "cast_dirp: \n");
	cast_log(CAST_DBG_SYS, "get_dirp\n");
	return (DIR *) (uintptr_t) fi->fh;
}

static int cast_releasedir(const char *path, struct fuse_file_info *fi)
{
//fprintf(stderr, "cast_releasedir: %s\n", path);
	DIR *dp = get_dirp(fi);
	cast_log(CAST_DBG_SYS, "releasedir %s\n", path);
	(void)path;
	closedir(dp);
	return 0;
}

static struct fuse_operations cast_oper = {
	.getattr = cast_getattr,
	.fgetattr = cast_fgetattr,
	.access = cast_access,
	.readlink = cast_readlink,
	.opendir = cast_opendir,
	.readdir = cast_readdir,
	.releasedir = cast_releasedir,
	.mknod = cast_mknod,
	.mkdir = cast_mkdir,
	.symlink = cast_symlink,
	.unlink = cast_unlink,
	.rmdir = cast_rmdir,
	.rename = cast_rename,
	.link = cast_link,
	.chmod = cast_chmod,
	.chown = cast_chown,
	.truncate = cast_truncate,
	.ftruncate = cast_ftruncate,
	.utime = cast_utime,
	.create = cast_create,
	.open = cast_open,
	.read = cast_read,
	.write = cast_write,
	.statfs = cast_statfs,
	.release = cast_release,
	.fsync = cast_fsync,
#ifdef HAVE_SETXATTR
	.setxattr = cast_setxattr,
	.getxattr = cast_getxattr,
	.listxattr = cast_listxattr,
	.removexattr = cast_removexattr,
#endif
	.destroy = cast_destroy
};

int main(int argc, char *argv[], char *env[])
{
	int i, dbgdef;
	char *new_argv[argc], *logptr, *dbgptr, deflog[]="/tmp/castfs.log";
	int new_argc = 0;

	/* get environmental vars and open fd's for log file */
	umask(0);

	if (argc == 1) {
		fprintf(stderr, "castfs: no arguments given\n");
		usage();
		exit(-1);
	}

	if(strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
		fprintf(stdout, "castfs: version 0.6-git\n"); /* FIXME: s/b dynamic */
		exit(0);
	} else if(strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
		usage();
		exit(0);
	}
	/* If user specifies logfile, use that, else use default */
	logptr=getenv("CASTFS_LOGFILE");
	if(logptr == NULL) {
		logptr=deflog;
	}
	logfd = open(logptr, O_CREAT|O_APPEND|O_WRONLY, 0666);
	if (logfd < 0) {
		fprintf(stderr, "castfs: couldn't open logfile %s\n", logptr);
		usage();
		exit(-1);
	}

	/* If user specifies dbglvl, use that, else use default */
	dbgptr = getenv("CASTFS_DBGLVL");
	if(dbgptr != NULL) {
		dbglvl = atoi(dbgptr);
	} else {
		dbgdef = 0;
		/* comment out the following not used in default */
		dbgdef |= CAST_DBG_MAIN; 
		/* dbgdef |= CAST_DBG_SYS; */
		/* dbgdef |= CAST_DBG_UTIL; */
		/* dbgdef |= CAST_DBG_PATHS; */
		dbglvl = dbgdef;	// Toggle the needed bits.
	}

	/* Currently, need: `castfs mnt-dir -o stage=stage-dir` */
	if (argc < 4) {
		fprintf(stderr, "castfs: argc wrong\n");
		usage();
		exit(-1);
	}

	/* dump some nice header information in the log file */
	cast_log(CAST_DBG_MAIN, "main\n");
	cast_log(CAST_DBG_MAIN, "logfile %s fd %d\n", getenv("CASTFS_LOGFILE"),
		 logfd);
	cast_log(CAST_DBG_MAIN, "debug level %d\n", dbglvl);

	/* time to parse the arguments and strip out the stuff we use */
	mount_path = argv[1];
	/* check the stage path here */
	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-o")==0) {
			if (parse_mount_options(argv[i + 1])) {
				strip_out_arguments(argv[i + 1]);
				if (strlen(argv[i + 1]) == 0) {
					i++;
				} else {
					new_argv[new_argc] = argv[i];
					new_argc++;
				}
			}
		} else {
			new_argv[new_argc] = argv[i];
			new_argc++;
		}
	}
	if (!stage_path) {
		fprintf(stderr, "castfs: stage_path is null\n");
		usage();
		exit(-1);
	}

	cast_log(CAST_DBG_MAIN, "mount path %s\n", mount_path);
	cast_log(CAST_DBG_MAIN, "stage path %s\n", stage_path);
	for (i = 0; i < ignored_dirs_len; i++)
		cast_log(CAST_DBG_MAIN, "ignored dir %s\n", ignored_dirs[i]);
	for (i = 0; i < new_argc; i++)
		cast_log(CAST_DBG_MAIN, "argv[%d] = \"%s\"\n", i, new_argv[i]);

fprintf(stderr, "castfs: checking sanity of <mnt-dir> and <stage-dir>\n");
	if(stage_not_in_mnt(mount_path, stage_path)) {
		fprintf(stderr, "castfs: stagedir is okay!\n");
	} else {
		fprintf(stderr, "castfs: <stage-dir> cannot be a subdir of <mnt>\n");
		exit(1);
	}

	castHashInit();

	return fuse_main(new_argc, new_argv, &cast_oper);
}
