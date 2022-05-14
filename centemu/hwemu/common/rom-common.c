#include <errno.h>
#include <stdio.h>
#include "rom-common.h"
int rom_read_file(char *filename, uint16_t size, uint8_t *data ) {
	FILE *fp;
	size_t ret_code;

	fp=fopen(filename,"rb");
	ret_code=fread(data,1,size,fp);
	if(ret_code != size) {
		if(feof(fp)) {
			fprintf(stderr,"Unexpected EOF while reading %s: Only got %li byte of an expected %i.\n",
				filename, ret_code, size);
		} else if(ferror(fp)){
			fprintf(stderr,"Failed while reading %s",filename);
			perror("read_rom_file");

		}
		fclose(fp);
		return(-1);
	}
	fclose(fp);

	return(0);
}

