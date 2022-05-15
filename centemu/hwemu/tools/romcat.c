#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <getopt.h>


int main(int argc, char *argv[]) {
	int r;
	uint16_t i=0;
	int opt, optidx;
	uint8_t fnum, files, chunks, stride=1;
	char format= 'b';
	char *Fdelim="";
	char *Rdelim="\n";
	FILE *fp[8];

	const struct option long_opts[] = {
		{"stride",	required_argument,0,'s'},
		{"format",	required_argument,0,'f'},
		{"FS",	required_argument,0,'d'},
		{"field-separator",	required_argument,0,'d'},
		{"RS",	required_argument,0,'r'},
		{"record-separator",	required_argument,0,'r'},
		{0,0,0,0}
	};
	char *optstr= "s:f:d:r:";
	while(1) {
		opt = getopt_long(argc,argv,optstr,long_opts,&optidx);
		if(opt==-1){ break; }
		switch(opt) {
			case 's': stride=atoi(optarg); break;
			case 'f': format=*optarg; break;
			case 'd': Fdelim=optarg; break;
			case 'r': Rdelim=optarg; break;
		}
	}
	if( stride > 8 ) {
		printf("Maximum stride is (currently) 8 bytes (long long int)\n");
		return(-1);
	}

	files=argc-optind;
	if(!files ) {
		printf("Requires one or more filenames name as an arguments\n");
		return(-1);
	}
	
	if(files%stride) {
		printf("Number of files must be an even multiple of stride (%0u)\n",stride);
		return(-1);
	}
	
	chunks=(files/stride);
	i=0;
	for(int c=0; c<chunks;c++) {

		for(int f=0; f<stride;f++){
			fnum=(stride*c)+f;
			if( !(fp[f]=fopen(argv[optind+fnum],"rb")) ) {
				fprintf(stderr,"Failed while reading %s",argv[optind+fnum]);
				perror("fopen:");
				for(int q=0; q<stride;q++) { if(fp[q]){ fclose(fp[q]); } };
				return(-1);
			}
		}

		do {
			r=fgetc(fp[i%stride]);
			if(feof(fp[i%stride])) { break; }
			
			switch(format) {
				case 'x':
					if(!(i%stride) && i) { printf("%s",Rdelim); }
					printf("%02x",r);
					break;
				default:
				case 'b': printf("%c",r); break;

				//	printf("%s%s0x%02x",i?",":"",i%8?"":"\n\t",(uint8_t)r);

			}
			i++;
		} while(1);


		if(ferror(fp[i%stride])){
			fprintf(stderr,"Failed while reading %s",argv[optind+(stride*c)+(i%stride)]);
			perror("read_rom_file");
			for(int q=0; q<stride;q++) { if(fp[q]){ fclose(fp[q]); } };
			return(-1);

		}
		for(int q=0; q<stride;q++) { if(fp[q]){ fclose(fp[q]); } };
	}
	return(0);
}
