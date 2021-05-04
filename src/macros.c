#include "macros.h"

void* safe_malloc(const char func_name[], size_t size)
{
	void *ptr = malloc(size);
	if(ptr == NULL) {
		eprintf("%s(): Failed to allocate memory\n", func_name);
		if(f_send != NULL) fprintf(f_send, "ERR SRV\n");
		exit(EXIT_FAILURE);
	}
	return ptr;
}