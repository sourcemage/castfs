#include "hash.h"

int main(int argc, char *argv[])
{
	char buf[1024];
	cast_paths_ptr tmp;
	stage_path = strdup("/this/is/the/stage/path");
	castHashInit();
	while(scanf("%s\n", buf) != EOF)
	{
		printf("%s\n", buf);
		tmp = castHashGetValueOf(buf);
		if(tmp)
		{
			printf("Get Value Of %s, %s\n", tmp->root_path, tmp->stage_path);
		}
		tmp = castHashSetValueOf(buf);
		printf("Set Value Of %s, %s\n", tmp->root_path, tmp->stage_path); 
	}
	castHashDestroy();
	return 0;
}
