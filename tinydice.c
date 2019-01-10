#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>


#define LOOPS_UNTIL_SLEEP 	20000
#define LOOPS_UNTIL_FLIP 	80
#define BTN_MASK 			_BV(PB1)

#define BUTTON_RELEASED()	(PINB & BTN_MASK)
#define BUTTON_PRESSED()	(!BUTTON_RELEASED())

// http://eleccelerator.com/fusecalc/fusecalc.php?chip=attiny13a
FUSES =
{
	.low = (FUSE_SPIEN & FUSE_SUT0 & FUSE_CKSEL0 & FUSE_CKDIV8),
	.high = HFUSE_DEFAULT,
};

enum td_state {
	TD_RESULT,
	TD_SHUFFLING,
};

static uint8_t right_plex[] = {
	_BV(DDB0),
	_BV(DDB2),
	_BV(DDB3),
	_BV(DDB0)
};

static uint8_t left_plex[] = {
	_BV(DDB2),
	_BV(DDB3),
	_BV(DDB4),
	_BV(DDB3)
};

static uint8_t dice_combi[] = {
	0b0001, // 1
	0b0010, // 2
	0b0011, // 3
	0b0110, // 4
	0b0111, // 5
	0b1110, // 6
	0b1111, // all on
};

// INT0 and PB1 (button) share the same pin (6)
EMPTY_INTERRUPT(INT0_vect);

static void
display_off(void)
{
	DDRB = 0; // all input
	PORTB = BTN_MASK; // pull-up only button pin
}

static void
go_sleep(void)
{
	display_off();
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	cli();
	GIMSK = _BV(INT0);
	sleep_enable();
	sei();
	sleep_cpu();
	// wake up here
	GIMSK &= ~_BV(INT0);
	sleep_disable();
}

// 16 bit xorshift http://www.retroprogramming.com/2017/07/xorshift-pseudorandom-numbers-in-z80.html
static uint16_t
random(void)
{
	static uint16_t random_state = 1; // XXX seed from flash?

	random_state ^= random_state << 9;
	random_state ^= random_state >> 7;
	random_state ^= random_state << 13;
	return (random_state);
}

// https://cvsweb.openbsd.org/cgi-bin/cvsweb/~checkout~/src/lib/libc/crypt/arc4random_uniform.c
static uint8_t
random_dice(void)
{
	uint16_t x;
	do {
		x = random();
	} while (x > 0xFFFB);
	return x % 6;
}

static void
display(uint8_t number, uint8_t * side)
{
	for (int i = 0; i < 4; i++) {
		display_off();
		if (dice_combi[number] & _BV(i)) {
			DDRB = right_plex[i] | left_plex[i];
			PORTB = BTN_MASK | side[i];
			__builtin_avr_delay_cycles(30); // XXX LED intensity
		}
	}
}

int
main(void)
{
	/*
	  states (action -> new state):
		- show result (no button press for a while -> powerdown OTHERWISE button press -> shuffle)
		- shuffle (button release -> show result)
		- powerdown (button press -> shuffle)
	*/
	enum td_state state = TD_RESULT;
	uint8_t number_left = 0, number_right = 0; // 0 -> "1"
	uint8_t show_dice = 1;
	uint32_t sleep_counter = 0, flip_counter = 0;

	display_off();
	for (;;) {
		switch (state) {
			case TD_RESULT:
				if (++sleep_counter >= LOOPS_UNTIL_SLEEP) {
					sleep_counter = 0;
					go_sleep();
				}
				if (BUTTON_PRESSED()) {
					flip_counter = 0;
					state = TD_SHUFFLING;
				}
				break;
			case TD_SHUFFLING:
				if (BUTTON_RELEASED()) {
					show_dice = 1;
					sleep_counter = 0;
					state = TD_RESULT;
				} else if (++flip_counter >= LOOPS_UNTIL_FLIP) {
					flip_counter = 0;
					number_left = random_dice();
					number_right = random_dice();
					show_dice = !show_dice;
					if (!show_dice) {
						display_off();
					}
				}
				break;
		}
		if (show_dice) {
			display(number_left, left_plex);
			display(number_right, right_plex);
		}
	}
}