#include "castfs.h"
#include "hash.h"

void usage()
{
	fprintf(stderr, 
    		"\ncastfs <mount-point> -o stage=<stage-dir>\n"
		"\n\texport CASTFS_LOGFILE=/tmp/castfs.log\t# Default\n"
		"\texport CASTFS_DBGLVL=15\t\t\t# Default\n"
		"\n\tcastfs is used to track changes in the filesystem.  All\n"
		"\tchanged files will be made in <stage-dir>.\n"
		"\tTo stage files, modify the version of the root filesystem\n"
		"\tin <mount-point>.  Any change to this copy will be tracked\n"
		"\tin <stage-dir>.\n"
	);
	return;
}

int cast_paths_log(cast_paths_ptr paths)
{
	char buf[1024];
	int size;
	if (CAST_DBG_PATHS & dbglvl && paths) {
		size = sprintf(buf, "Root Path: %s\n", paths->root_path);
		write(logfd, buf, size);
		/* I think this will work cross platform */
		size =
		    sprintf(buf, "\tAddress: 0x%lX\n",
			    (long unsigned int)paths);
		write(logfd, buf, size);
		size = sprintf(buf, "\tis staged: %d\n", paths->is_staged);
		write(logfd, buf, size);
		size = sprintf(buf, "\tis deleted: %d\n", paths->is_deleted);
		write(logfd, buf, size);
		size = sprintf(buf, "\tis ignored: %d\n", paths->is_ignored);
		write(logfd, buf, size);
		size = sprintf(buf, "\tstage path: %s\n", paths->stage_path);
		write(logfd, buf, size);
	}
	return 0;
}

int cast_log(unsigned int dbgmask, const char *format, ...)
{
	va_list list;
	int ret = 0;
	char buf[1024];
	if (dbgmask & dbglvl) {
		va_start(list, format);
		vsnprintf(buf, 1024, format, list);
		ret = write(logfd, buf, strlen(buf));
		va_end(list);
	}
	return ret;
}

int cast_copy_reg_file(const char *from, const char *to)
{
	int ffd, tfd;
	char buf[1024];
	ssize_t wsize, rsize = 1;
	ffd = open(from, O_RDONLY);
	if (ffd < 0)
		return -errno;
	tfd = open(to, O_WRONLY | O_TRUNC | O_CREAT);
	if (tfd < 0)
		return -errno;
	while (rsize > 0) {
		rsize = read(ffd, (void *)buf, 1024);
		if (rsize < 0)
			return -errno;
		wsize = write(tfd, (void *)buf, rsize);
		if (wsize < 0)
			return -errno;
	}
	close(ffd);
	close(tfd);
	return 1;
}

int cast_copy_file(const char *from, const char *to)
{
	char buf[1024];
	int ret = -1;
	struct stat tstats, fstats;
	lstat(to, &tstats);
	lstat(from, &fstats);
	if (tstats.st_dev == fstats.st_dev && tstats.st_ino == fstats.st_ino)
		return -1;
	if (lstat(to, &tstats) == 0)
		unlink(to);
	else if (errno != ENOENT)
		return -errno;
	if (S_ISREG(fstats.st_mode)) {
		ret = cast_copy_reg_file(from, to);
	} else if (S_ISDIR(fstats.st_mode)) {
		ret = mkdir(to, fstats.st_mode);
	} else if (S_ISCHR(fstats.st_mode) || S_ISBLK(fstats.st_mode)
		   || S_ISFIFO(fstats.st_mode)) {
		ret = mknod(to, fstats.st_mode, fstats.st_dev);
	} else if (S_ISLNK(fstats.st_mode)) {
		readlink(to, buf, 1024);
		ret = symlink(to, buf);
	} else {
		ret = -1;
	}
	chmod(to, fstats.st_mode);
	lchown(to, fstats.st_uid, fstats.st_gid);
	return ret;
}

int cast_mkdir_rec_staged(cast_paths_ptr dir)
{
	struct stat dir_stats;
	char *pdir = strdup(dir->root_path);
	pdir = dirname(pdir);
	cast_paths_ptr pdirPaths = castHashSetValueOf(pdir);
	if (lstat(pdirPaths->stage_path, &dir_stats) == -1) {
		if (!cast_mkdir_rec_staged(pdirPaths))
			return 0;
	}
	stat(dir->root_path, &dir_stats);
	mkdir(dir->stage_path, dir_stats.st_mode);
	chmod(dir->stage_path, dir_stats.st_mode);
	return 1;
}

void strip_out_arguments(char *arg)
{
	int i, j;
	int copying = 1;
	for (i = 0, j = 0; i < strlen(arg); i++) {
		if (!strncmp("stage=", arg + i, 6)
		    || !strncmp("ignore=", arg + i, 7))
			copying = 0;
		if (copying) {
			arg[j] = arg[i];
			++j;
		}
		if (arg[i] == ',')
			copying = 1;
	}
	if (arg[j - 1] == ',')
		arg[j - 1] = '\0';
	else
		arg[j] = '\0';
}

int parse_mount_options(char *str)
{
	char *strcp = strdup(str);
	char *token = strtok(strcp, ",");
	int found = 0;
	while (token) {
		if (!strncmp("stage=", token, 6)) {
			found++;
			stage_path = strdup(token + 6);
			clean_cast_path(stage_path);
		} else if (!strncmp("ignore=", token, 7)) {
			found++;
			ignored_dirs_len++;
			ignored_dirs =
			    realloc(ignored_dirs,
				    sizeof(char *) * ignored_dirs_len);
			ignored_dirs[ignored_dirs_len - 1] = strdup(token + 7);
			clean_cast_path(ignored_dirs[ignored_dirs_len - 1]);
		}
		token = strtok(NULL, ",");
	}
	free(strcp);
	return found;
}

int shouldIgnore(const char *path)
{
	int found = 0;
	int i;
	for (i = 0; i < ignored_dirs_len; i++) {
		if (!strncmp(ignored_dirs[i], path, strlen(ignored_dirs[i])))
			found = 1;
	}
	return found;
}

/*
 * stage_not_in_mnt():
 *	Simple test to ensure stage is not a subdir of mnt.
 *	I don't try to equate ~ to /home/user or any such reductions.
 *
 *	Status: Broken!  Be ye ware!
 */
int stage_not_in_mnt(const char *mnt, const char *stage)
{
	int n;
	char tmp[128];

	n = strlen(mnt); tmp[0]=0;

	if(strncmp(mnt, stage, n) == 0) {
fprintf(stderr, "castfs: Error, stage is a subdir to mnt\n");
		return 0;
	} else {
fprintf(stderr, "castfs: Cool staged dir seems okay\n");
		return 1;
	}
}
