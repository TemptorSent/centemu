#pragma once
#include <stdint.h>
typedef enum clock_state {
	CLK_LO=0x0, /* b00 */
	CLK_LH=0x1, /* b01 */
	CLK_HI=0x3, /* b11 */
	CLK_HL=0x2 /* b10 */
} clock_state_t;

typedef struct clockline_t {
	clock_state_t clk,clk_;
	char *clock_name;
} clockline_t;

static char *clock_state_name_short[4]= { "LO", "LH", "HL", "HI" };
static char *clock_state_name_full[4]= { "LOW", "LOW->HIGH", "HIGH->LOW", "HIGH" };
static char *clock_state_name_desc[4]= { "LOW", "RISING EDGE", "FALLING EDGE", "HIGH" };

#define CLOCK_IS_LO(clk) (clk == CLK_LO)
#define CLOCK_IS_LH(clk) (clk == CLK_LH)
#define CLOCK_IS_HI(clk) (clk == CLK_HI)
#define CLOCK_IS_HL(clk) (clk == CLK_HL)

#define CLOCK_STATE_NAME_SHORT(state) clock_state_name_short[state]
#define CLOCK_STATE_NAME_FULL(state) clock_state_name_full[state]
#define CLOCK_STATE_NAME_DESC(state) clock_state_name_desc[state]

#define CLOCK_STATE_NEXT(state) (state==CLK_LO? CLK_LH : state==CLK_LH? CLK_HI : state==CLK_HI? CLK_HL : CLK_LO)
#define CLOCK_STATE_INV(state) (state==CLK_LO? CLK_HI : state==CLK_LH? CLK_HL : state==CLK_HI? CLK_LO : CLK_LH)

clock_state_t clock_advance(clockline_t *cl);
clock_state_t clock_set(clockline_t *cl,clock_state_t state);
void clock_init(clockline_t *clockline, char *clockline_name, clock_state_t initial_state);
