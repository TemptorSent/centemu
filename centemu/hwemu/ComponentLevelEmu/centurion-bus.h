
typedef centurion_card_connector_t {
	nibble_t PWRb0; /* B1-B4 Power pins: B1,B2,B3,B4 -> GND, +5V, -12V, +12V */
	word_t A; /* B5-B20 Address Lines (b5-b20->A0-A15) */
	byte_t D; /* B21-B28 Data Lines (b21-b28->D0-D7) */
	bit_t Unkb29; /* B29 Unknown */
	bit_t Intb30; /* B30 ?Interrupt Passthrough */
	clock_state_t clk; /* B31 5MHz Clock */
	bit_t RST; /* B32 Reset signal (Select button) */
	bit_t Intb33; /* B33 ?Interrupt Passthrough */
	octal_t PWRb1; /* B34-B36 Power pins: B34,B35,B36 -> -12V, +5V, GND */

	nibble_t PWRt0; /* T1-T4 Power pins: T1,T2,x,T4 -> GND, +5V, x, +12V */
	bit_t Unkt5; /* T5 Unknown */
	bit_t RW; /* T6 Read/Write (Polarity?) */
	bit_t WAIT; /* T7 Wait State? (Clear low?) */
	bit_t READY; /* T8 Data Ready? (Active low?) */
	nibble_t Unkt09_12; /* T9-T12 Unknown */
	nibble_t Intt13_16; /* T13-T16 Interrupts? */
	bit_t Unkt17; /* T17 Unknown */
	nibble_t Intt18_21; /* T18-T21 Interrupts? */
	bit_t Unkt22; /* T22 Unknown */
	octal_t Abank; /* T23-T25 High order address lines / bank select */
	bit_t Unkt26; /* T26 Unknown */
	bit_t Intt27; /* T27 Interrupt? */
	bit_t Unkt28; /* T28 Unknown */
	bit_t IntACK; /* T29 Interrupt Acknowledge? */
	bit_t Intt30; /* T30 Interrupt Passthrough? */
	bit_t Unkt31; /* T31 Unknown */
	bit_t Unkt32; /* T32 Unknown */
	bit_t IntPRT; /* T33 Interrupt - Printer? */
	octal_t PWRt1; /* T34-T36 Power pins: T34,T35,T36 -> -12V, +5V, GND */
} centurion_card_connector_t;
