#ifndef CAST_HASH_H
#define CAST_HASH_H

#include <string.h>

#include "castfs.h"
#include "rbtree.h"

cast_paths_ptr malloc_cast_paths(const char *path);
void free_cast_paths(cast_paths_ptr paths);
void castHashInit(void);
cast_paths_ptr castHashGetValueOf(char *value);
cast_paths_ptr castHashSetValueOf(char *value);
void castHashDestroy(void);
void clean_cast_path(char *path);

#endif
