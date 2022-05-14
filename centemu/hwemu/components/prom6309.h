#include "../common/rom-common.h"
#include "../common/logic-common.h"

typedef struct prom6309_device_t {
	char *filename;
	byte_t data[256];
	byte_t *A;
	byte_t *D;
	bit_t *CE1_;
	bit_t *CE2_;
} prom6309_device_t;

void prom6309_update(prom6309_device_t *dev);

int prom6309_init(prom6309_device_t *dev, char *filename, byte_t *A, byte_t *D, bit_t *CE_1, bit_t *CE2_);
