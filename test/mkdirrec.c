#include "hash.h"

int main(int argc, char *argv[])
{
	char buf[1024];
	cast_paths_ptr tmp;
	stage_path = argv[1];
	castHashInit();
	while(scanf("%s\n", buf) != EOF)
	{
		printf("Trying to mkdir -p %s\n", buf);
		tmp = castHashSetValueOf(buf);
		cast_mkdir_rec_staged(tmp);
	}
	castHashDestroy();
	return 0;
}
