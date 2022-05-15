#include <stdio.h>
#include <inttypes.h>
#include <getopt.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	int r;
	uint16_t i=0;
	FILE *fp;
	char *filename, *varname="";
	int opt;
	uint8_t ws=1, array=0;
	const struct option long_opts[] = {
		{"name",	required_argument,0,'n'},
		{"word-size",	required_argument,0,'w'},
		{"width",	required_argument,0,'w'},
		/*{"array",	0,0,'a'},*/
		{0,0,0,0}
	};
	char *optstr= "n:w:";
	while(1) {
		opt = getopt_long(argc,argv,optstr,long_opts,0);
		if(opt==-1){ break; }
		switch(opt) {
			case 'n': varname=optarg; break;
			case 'w': ws=atoi(optarg); break;
			default:
				printf("Unknown option\n");
				return(-1);
			case 'a': array=1; break;
		}
	}

	if( ws > 8 ) {
		printf("Maximum word-size is (currently) 8 bytes (long long int)\n");
		return(-1);
	}
	
	if( !(*varname) ) {
		printf("Please specify required option --name=<varname> to specify the output variable name\n");
		return(-1);
	}

	if( (optind+1) != argc ) {
		printf("Requires a single filename after options as an argument. Use '-' for stdin\n");
		return(-1);
	}
	filename=argv[optind];
	if(*filename=='-') {
		fp=stdin;
	} else {
		fp=fopen(filename,"rb");
	}
	
	switch(ws) {
		case 1: printf("uint8_t"); break;
		case 2: printf("uint16_t"); break;
		case 3: case 4: printf("uint32_t"); break;
		case 5: case 6: case 7: case 8: printf("uint64_t"); break;
	}
	printf(" %s[] = {",varname);
	do{
		
		for(int b=0; b<ws;b++) {
			r=fgetc(fp);
			if(feof(fp)) { goto done; }
			if(!b) { printf("%s%s", (i? ",":""), ( (i%(ws>4?4:8))? "0x":"\n\t0x") ); }
			printf("%02x",(uint8_t)r);
		}
		printf("%s", (ws>4?"LL":"") );
		i++;

	} while(1);
done:
	printf("\n};\n");
	printf("#define ROMSIZE_%s 0x%04x\n",varname,i);
	printf("#define ROMWIDTH_%s 0x%02u\n",varname,ws);

	if(ferror(fp)){
		fprintf(stderr,"Failed while reading %s",filename);
		perror("read_rom_file");
		fclose(fp);
		return(-1);

	}
	fclose(fp);

	return(0);
}
