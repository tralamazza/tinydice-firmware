#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>

FUSES =
{
  .low = (FUSE_SPIEN & FUSE_SUT0 & FUSE_CKSEL0),
  .high = HFUSE_DEFAULT,
};


const uint8_t button = (1 << PIN1);//gives pin position
const uint32_t sleep_loops = 70000;


 uint8_t left[]  = {
	(1 << DDB0), // 3 is on
	(1 << DDB2), // 3 is on
	(1 << DDB3),//4 is on
	(1 << DDB0)//4 is on
};

 uint8_t right[]  = {
	(1 << DDB2), // 3 is on
	(1 << DDB3), // 3 is on
	(1 << DDB4),//4 is on
	(1 << DDB3)//4 is on
};

 uint8_t dice[]  = {
	0b0001, //1
	0b0010, //2
	0b0011, //3
	0b0110, //4
	0b0111, //5
	0b1110, //6
	0b1111
};


static uint16_t random_seed = 1;

static uint16_t
random(void)
{
  random_seed ^= random_seed << 13;
  random_seed ^= random_seed >> 9;
  random_seed ^= random_seed << 7;
  return (random_seed);
}


void switchLED(int on, int location, int isright)
{
	DDRB = dice[0];
	if (isright){
		PORTB =  right[location] | button;// what is high/ pull up
	} else {
		PORTB =  left[location] | button;// what is high/ pull up
	}
	if (on) {
		DDRB = right[location] | left[location]; //what is output
	} else {
		DDRB = 0;
	}

}

ISR(INT0_vect)
{
	GIMSK &= ~_BV(INT0);
}

int main(void)
{

	int n1=0;
	int n2=0;
	int buttontime=2;
	int buttonpressed=0;
	uint32_t time_loops = 0;

	PORTB |= button;
 
	for ( ; ; ) {
		//if button down increment button time
		if ((PINB & button) == 0) {
			buttontime++;
			buttonpressed=1;
			n1=n2=6;
			time_loops = 0;
		} else {
			if (buttonpressed){
				random_seed += buttontime;
				if (random_seed == 0)
					random_seed = 1;
				while ((n1 = (random() & 7)) >= 6)
					/* NOTHING */;
				while ((n2 = (random() & 7)) >= 6)
					/* NOTHING */;
				buttonpressed=0;
			}
		}


		for (int i=0; i<4; i++) {	
			switchLED(dice[n1]&(1<<i), i, 1);
			for (volatile long i = 0; i < 10; i++)
				/* NOTHING */;
				switchLED(dice[n2]&(1<<i), i, 0);
			for (volatile long i = 0; i < 10; i++)
				/* NOTHING */;
		}

		time_loops++;
		if (time_loops >= sleep_loops) {
			n1=n2=6;
			time_loops = 0;
			DDRB = 0;
			PORTB = button;

			set_sleep_mode(SLEEP_MODE_PWR_DOWN);

			cli();
			while ((PINB & button) != 0) {
				GIMSK |= _BV(INT0);
				sleep_enable();
				sei();
				sleep_cpu();
				GIMSK &= ~_BV(INT0);
				sleep_disable();
			}
		}
	}
}