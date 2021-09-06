#include <inttypes.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#if defined(__AVR_ATtiny13__)
  #define WDINTEN WDTIE
#else
  #define WDINTEN WDIE        //ATTinyCore 
#endif

#define PinStatOut PB1
#define PinPTTOut  PB3
#define PinPTTIn   PB4

static volatile uint8_t i = 0;
const uint8_t defaultTOut = 20; // Keep this =<250(s) or timer i will never reach it... Probably don't want >~4m10s PTT timeout anyways...

ISR(WDT_vect) {
  WDTCR |= (1<<WDE); // reset WDE (but don't reset "WDIE" here, only do that in main loop as the "feed watchdog" command)
  ( i > 250 ) ? i=0 : i++;
  if (i >= defaultTOut) { PORTB &= ~(1 << PinPTTOut); } // TOut occurred -- set PTT off
//  ie. if WDIE not set in main loop, then next trigger of WDE will re-boot the ATTiny instead of calling this ISR
}

ISR(PCINT0_vect) {
  if ( (PINB & (1 << PinPTTIn)) == (1 << PinPTTIn) ) { // PB4 released
    PORTB &= ~(1 << PinPTTOut); // set PTT off 
  } else { // PB4 pressed
    i = 0; // reset PTT Time Out timer
    PORTB |= (1 << PinPTTOut); // set PTT on
  }
}

void LED_toggle(byte count) {for (byte x=0;x<count;x++) {delay(250); PORTB ^= (1<<PinStatOut); }} // status led
void boot_reason_flash(byte reason) { // NB. after s/w-upload usually get 6 flashes
  if ((reason&(1<<WDRF))!=0) LED_toggle(2*2); // Watchdog = 2 flashes
  if ((reason&(1<<PORF))!=0) LED_toggle(3*2); // PowerOn = 3 flashes
  if ((reason&(1<<BORF))!=0) LED_toggle(4*2); // BrownOut = 4 flashes
  if ((reason&(1<<EXTRF))!=0) LED_toggle(6*2); // ExternalReset = 6 flashes
}

void setup() {
  ADCSRA &= ~(1<<ADEN); // ADC off
  
  DDRB = 0x0F;                 // enable PB0-PB3 as outputs
  PORTB &= ~(1 << PinPTTOut);  // Start with PTT output on PB3 off
  PORTB &= ~(1 << PinStatOut); // Start with status output on PB1 off
  
  PORTB |= (1 << PinPTTIn);    // enable pullup on PB4

  byte boot_reason=MCUSR; MCUSR=0x00;    // NB. MCUSR must be zeroed or watchdog will keep rebooting
  WDTCR|=(1<<WDCE)|(1<<WDE); WDTCR=0x00; // disable watchdog
  boot_reason_flash(boot_reason);

  WDTCR = (0 << WDP3) | (1 << WDP2) | (1 << WDP1) | (0 << WDP0); // set watchdog-timing to 1secs
  WDTCR |= (1 << WDE) | (1 << WDINTEN); // Set watchdog timer mode as BOTH WDE (reboot) and WDIE (ISR-interrupt)

  GIMSK |= (1 << PCIE);   // pin change interrupt enable
  PCMSK |= (1 << PCINT4); // pin change interrupt enabled for PCINT4

  sei();
}

void loop() {
//    if (i < 10) { WDTCR|=(1<<WDINTEN); } // TEST: feed the watchdog for ~10s after last pushbutton
    WDTCR|=(1<<WDINTEN); // feed the watchdog
    
    sleep_enable();
    sleep_cpu();
}
