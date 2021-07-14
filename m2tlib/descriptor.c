#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m2tlib.h"

#define DEBUG 0

DESC_COMPONENT *st_desc_component( unsigned char *buf ){

	DESC_COMPONENT *ptr = (DESC_COMPONENT *)calloc(1,sizeof(DESC_COMPONENT));

        uint64_t gomi;
        uint64_t boff = 0;
	gomi			= getBit( buf, &boff, 4);
	ptr->stream_content	= getBit( buf, &boff, 4);
	ptr->component_type	= getBit( buf, &boff, 8);
	ptr->component_tag	= getBit( buf, &boff, 8);

#if DEBUG
	printf("descriptor.c: stream_content=%02X component_type=%02X component_tag=%02X\n", 
			ptr->stream_content, ptr->component_type, ptr->component_tag);
#endif

	return(ptr);

}

void free_desc_component( DESC_COMPONENT *ptr ){
	free(ptr);
}
