#include "prom6309.h"
#include "../common/logic-common.h"
int prom6309_init(prom6309_device_t *dev, char *filename, byte_t *A, byte_t *D, bit_t *CE1_, bit_t *CE2_) {
	I_(A)=A;
	I_(D)=D;
	I_(CE1_)=CE1_;
	I_(CE2_)=CE2_;
	return( rom_read_file(filename,256,I_(data)) );
}

/* Tristate outputs, data output from addressed location enabled when CE1_=0, CE2_=0 */
void prom6309_update(prom6309_device_t *dev) {
	TRI_OUTPUT( S_(D), (!S_(CE1_)&&!S_(CE2_)), I_(data[S_(A)]) );
}
