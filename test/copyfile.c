#include "castfs.h"
#include "hash.h"

int main(int argc, char **argv)
{
	if(argc < 3)
		return -1;
	printf("Copying %s to %s\n", argv[1], argv[2]);
	return cast_copy_file(argv[1], argv[2]);
}
