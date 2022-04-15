
/* Octal 3-STATE Buffer/Line Driver/Line Receiver */
/* Enable inverting outputs: G1=0->Y1 (Active LO), G2=0->Y2 (Active LO) */
int 74ls240(74ls240_device_t *dev){
	if(!*(dev->G1)) {*(dev->Y1)=(~*(dev->A1))&0xf;}
	if(!*(dev->G2)) {*(dev->Y2)=(~*(dev->A2))&0xf;}
}
/* Octal 3-STATE Buffer/Line Driver/Line Receiver */
/* Enable non-inverting outputs: G1=0->Y1 (Active LO), G2=1->Y2 (Active HI) */
int 74ls240(74ls241_device_t *dev){
	if(!*(dev->G1)) {*(dev->Y1)=(*(dev->A1))&0xf;}
	if(*(dev->G2)) {*(dev->Y2)=(*(dev->A2))&0xf;}
}
