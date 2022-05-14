#include "../common/rom-common.h"
#include "../common/logic-common.h"

typedef struct prom76161_device_t {
	char *filename;
	uint8_t data[2048];
	uint16_t *A;
	uint8_t *D;
	bit_t *CE1_;
	bit_t *CE2;
	bit_t *CE3;
} prom76161_device_t;

void prom76161_update(prom76161_device_t *dev);

int prom76161_init(prom76161_device_t *dev, char *filename, uint16_t *A, uint8_t *D, bit_t *CE_1, bit_t *CE2, bit_t *CE3);
