#include <stdio.h>
#include <inttypes.h>

int main(int argc, char *argv[]) {
	int r;
	uint16_t i=0;
	FILE *fp;
	char *filename, *varname;
	int opt;

	if(argc!=3) {
		printf("Requires a filename and a variable name as an argument");
		return(0);
	}
	filename=argv[1];
	varname=argv[2];

	fp=fopen(filename,"rb");

	printf("uint8_t %s[] = {",varname);
	do{
		r=fgetc(fp);
		if(feof(fp)) { break; }

		printf("%s%s0x%02x",i?",":"",i%8?"":"\n\t",(uint8_t)r);
		i++;
	} while(1);

	printf("\n};\n#define ROMSIZE_%s 0x%04x",varname,i);

	if(ferror(fp)){
		fprintf(stderr,"Failed while reading %s",filename);
		perror("read_rom_file");
		fclose(fp);
		return(-1);

	}
	fclose(fp);

	return(0);
}
