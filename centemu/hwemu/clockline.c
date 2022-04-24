#include "clockline.h"

clock_state_t clock_advance(clockline_t *cl) {
	cl->clk=CLOCK_STATE_NEXT(cl->clk);
	return(cl->clk);
};

void clock_init(clockline_t *clock_line, char *clock_name, clock_state_t initial_state) {
	clock_line->clock_name=clock_name;
	clock_line->clk=initial_state;
};
