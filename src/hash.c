#include "hash.h"

struct rb_root root;

/*
 * clean_cast_path():
 *	Remove multiple '/' chars from path string
 */
void clean_cast_path(char *path)
{
	int i = 0;
	int j;
	for (i = 1; i < strlen(path); i++)
		if (path[i] == '/' && path[i - 1] == '/')
			while (path[i] == '/')
				for (j = i; j < strlen(path); j++)
					path[j] = path[j + 1];
}

/*
 * malloc_cast_paths():
 *	Allocate a cast_paths_ptr and initialize it to path.
 */
cast_paths_ptr malloc_cast_paths(const char *path)
{
	cast_paths_ptr ret = malloc(sizeof(cast_paths));
	ret->root_path = strdup(path);
	ret->stage_path =
		malloc(sizeof(char)*(strlen(path)+3+strlen(stage_path)));
	(ret->stage_path)[0] = '\0';
	if (!shouldIgnore(path)) {
		ret->is_ignored = 0;
		strcat(ret->stage_path, stage_path);
	} else {
		ret->is_ignored = 1;
	}
	if (path[0] != '/')
		strcat(ret->stage_path, "/");
	strcat(ret->stage_path, path);
	ret->is_staged = 0;
	ret->is_deleted = 0;
	ret->next = NULL;
	return ret;
}

/*
 * free_cast_paths(): free() a cast_paths_ptr item.
 */
void free_cast_paths(cast_paths_ptr paths)
{
	if (paths) {
		free(paths->root_path);
		free(paths->stage_path);
		free(paths);
	}
}

/*
 * castHashInit():
 *	(I need more info.)
 */
void castHashInit(void)
{
	if (root.rb_node)
		castHashDestroy();
	root.rb_node = malloc(sizeof(struct rb_node));
	root.rb_node->rb_left = NULL;
	root.rb_node->rb_right = NULL;
	root.rb_node->rb_parent_color = RB_BLACK;
	root.rb_node->path = malloc_cast_paths("/");
}

/*
 * castHashGetValueOf():
 *	Perform a rb_search() of value starting at tree's root (midpoint).
 */
cast_paths_ptr castHashGetValueOf(char *value)
{
	clean_cast_path(value);
	struct rb_node *ret = rb_search(&root, value);
	return (ret) ? ret->path : NULL;
}

/*
 * castHashSetValueOf():
 *	Set a new cast_paths_ptr into the rbtree.  Return
 *	the new or existing pointer.
 */
cast_paths_ptr castHashSetValueOf(char *value)
{
//	clean_cast_path(value);		// called in castHashGetValueOf()
	cast_paths_ptr tmp = castHashGetValueOf(value);
	if (tmp) {
		printf("castfs: castHashSetValueOf: %s exists\n", value);
		return tmp;
	}
	struct rb_node *rbtmp = malloc(sizeof(struct rb_node));
	rbtmp->rb_left = NULL;
	rbtmp->rb_right = NULL;
	rbtmp->rb_parent_color = 0;
	rbtmp->path = malloc_cast_paths(value);
	rb_insert(&root, rbtmp);
	return rbtmp->path;
}

void __rb_rec_destroy(struct rb_node *n)
{
	if (n) {
		__rb_rec_destroy(n->rb_left);
		__rb_rec_destroy(n->rb_right);
		free(n);
	}
}

void castHashDestroy(void)
{
	__rb_rec_destroy(root.rb_node);
	root.rb_node = NULL;
}
