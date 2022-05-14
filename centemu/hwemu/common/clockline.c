#include "clockline.h"

clock_state_t clock_advance(clockline_t *cl) {
	cl->clk=CLOCK_STATE_NEXT(cl->clk);
	cl->clk_=CLOCK_STATE_INV(cl->clk);
	return(cl->clk);
};

clock_state_t clock_set(clockline_t *cl, clock_state_t state) {
	cl->clk=state;
	cl->clk_=CLOCK_STATE_INV(state);
	return(cl->clk);
}


void clock_init(clockline_t *cl, char *clock_name, clock_state_t initial_state) {
	cl->clock_name=clock_name;
	cl->clk=initial_state;
	cl->clk_=CLOCK_STATE_INV(cl->clk);
	
};
