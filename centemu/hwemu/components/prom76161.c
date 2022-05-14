#include <stdio.h>
#include "prom76161.h"
#include "../common/logic-common.h"
int prom76161_init(prom76161_device_t *dev, char *filename, uint16_t *A, uint8_t *D, bit_t *CE1_, bit_t *CE2, bit_t *CE3) {
	int res;

	I_(A)=A;
	I_(D)=D;
	I_(CE1_)=CE1_;
	I_(CE2)=CE2;
	I_(CE3)=CE3;
	res=rom_read_file(filename,2048,I_(data));
	return(res);

}

/* Tristate outputs, data output from addressed location enabled when CE1_=0, CE2=1, CE3=1 */
void prom76161_update(prom76161_device_t *dev) {
	TRI_OUTPUT( S_(D), (!S_(CE1_)&&S_(CE2)&&S_(CE3)), I_(data[S_(A)]) );
}
