/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Sakari Tanskanen (sakari.tanskanen@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define SERIAL_DEBUG

#include <avr/sleep.h>
#include "RF24.h"
#include "printf.h"

#define BOARD_REMOTE_VER "1.0"

/* Radio driver. CE on pin D2, CSN on D3 */
RF24 nrf24(2, 3);

/* Radio settings */
const uint64_t nrf24_remote_txpipe     = 0x531ab153ddc134ef;
const uint64_t nrf24_scoreboard_txpipe = 0x1389dfca645fabef;

#define NRF24_CHANNEL       85
#define NRF24_PA            RF24_PA_MAX
#define NRF24_RATE          RF24_250KBPS
#define NRF24_PAYLOAD_SZ    5
#define NRF24_RETRIES       5

#define SW_COUNT 10
/* Remote button index */
#define SW_2   0
#define SW_3   1
#define SW_4_L 2
#define SW_4_R 3
#define SW_5   4
#define SW_6   5
#define SW_7   6
#define SW_8   7
#define SW_9   8
#define SW_10  9

const uint8_t SW_PINS[SW_COUNT] = {A4, A5, 6, 7, A3, A2, A1, 8, 5, 4};
volatile unsigned int sw_debounce_ticks[SW_COUNT] = {0};
/* Button debounce timer (Timer2 ticks, 488 Hz) */
#define BTN_DEBOUNCE_DURATION 20

uint16_t button_up_events;
uint16_t button_down_events;
uint16_t button_state;
uint16_t button_last_event;

/* Time to stay awake when there is no user activity (ms) */
#define IDLE_ON_TIME 10000

/* Timestamp for last user activity */
unsigned long last_activity_time = 0;

/* Serial TX is used to drive potentiometer, use only for debug */

void serial_msg(const String& msg) {
  #ifdef SERIAL_DEBUG
    Serial.println(msg);
  #endif
}

void halt(const char *msg) {
  serial_msg(msg);
}

/* Multiplex driver timer interrupt */
ISR(TIMER2_OVF_vect) {
  /* Update button debounce timers */
  for (int i = 0; i < SW_COUNT; ++i) {
    if (sw_debounce_ticks[i]) {
      if (!--sw_debounce_ticks[i]) {
        /* Check pin state and compare to button_state. Generate event if different */
        if ((button_state & 1 << i) != (button_last_event & (1 << i))) {
          button_last_event ^= 1 << i;
          if (button_last_event & 1 << i) {
            button_down_events |= 1 << i;
          } else {
            button_up_events |= 1 << i;
          }
        }
      }
    }
  }
}

void btn_read_debounce(uint8_t sw_index)
{
  if (digitalRead(SW_PINS[sw_index])) {
    if (!(button_state & 1 << sw_index)) {
      /* State changed - debounce time passed? */
      if (!(sw_debounce_ticks[sw_index])) {
        button_down_events |= 1 << sw_index;
        button_last_event |= 1 << sw_index;
      }

      /* Reset debounce timer on every state change */
      sw_debounce_ticks[sw_index] = BTN_DEBOUNCE_DURATION;
    }

    button_state |= 1 << sw_index;
  }

  else {
    if (button_state & 1 << sw_index) {
      if (!(sw_debounce_ticks[sw_index])) {
        button_up_events |= 1 << sw_index;
        button_last_event &= ~(1 << sw_index);
      }
      /* Reset debounce timer on every state change */
      sw_debounce_ticks[sw_index] = BTN_DEBOUNCE_DURATION;
    }

    button_state &= ~(1 << sw_index);
  }
}

ISR(PCINT0_vect) {
  for (int i = 0; i < SW_COUNT; ++i) {
    btn_read_debounce(i);
  }
}

ISR(PCINT1_vect) {
  for (int i = 0; i < SW_COUNT; ++i) {
    btn_read_debounce(i);
  }
}

/* Footswitch button interrupt */
ISR(PCINT2_vect) {
  for (int i = 0; i < SW_COUNT; ++i) {
    btn_read_debounce(i);
  }
}

void input_setup() {
  /* Initialize buttons, internal pullups disabled by default */
  pinMode(SW_2, INPUT);
  pinMode(SW_3, INPUT);
  pinMode(SW_4_L, INPUT);
  pinMode(SW_4_R, INPUT);
  pinMode(SW_5, INPUT);
  pinMode(SW_6, INPUT);
  pinMode(SW_7, INPUT);
  pinMode(SW_8, INPUT);
  pinMode(SW_9, INPUT);
  pinMode(SW_10, INPUT);

  serial_msg("pinModes set");
  Serial.flush();

  noInterrupts();
  /* Setup Timer2 to decrement button debounce counters */
  TCCR2A = 0;                              /* Zero TC2 control registers */
  TCCR2B = 0;
  TCNT2 = 0;                               /* Initialize counter value */
  TCCR2B |= (1 << CS22);                   /* 1/64 prescaler, 488 Hz */
  TIMSK2 |= (1 << TOIE2);                  /* Enable overflow interrupt */
  
  PCMSK0 |= (1 << PCINT0);  /* Add PCINT0 to PCINT0 vector */
  PCMSK1 |= (1 << PCINT9);  /* Add PCINT9 to PCINT1 vector */
  PCMSK1 |= (1 << PCINT10); /* Add PCINT10 to PCINT1 vector */
  PCMSK1 |= (1 << PCINT11); /* Add PCINT11 to PCINT1 vector */
  PCMSK1 |= (1 << PCINT12); /* Add PCINT12 to PCINT1 vector */
  PCMSK1 |= (1 << PCINT13); /* Add PCINT13 to PCINT1 vector */
  PCMSK2 |= (1 << PCINT20); /* Add PCINT20 to PCINT2 vector */
  PCMSK2 |= (1 << PCINT21); /* Add PCINT21 to PCINT2 vector */
  PCMSK2 |= (1 << PCINT22); /* Add PCINT22 to PCINT2 vector */
  PCMSK2 |= (1 << PCINT23); /* Add PCINT23 to PCINT2 vector */
  PCICR  |= (1 << PCIE0);   /* Enable PCMSK0 interrupt */
  PCICR  |= (1 << PCIE1);   /* Enable PCMSK1 interrupt */
  PCICR  |= (1 << PCIE2);   /* Enable PCMSK2 interrupt */
  interrupts();
  serial_msg("input_setup done");
  Serial.flush();
}

void radio_setup() {
  serial_msg("Initializing radio module...");
  if (!nrf24.begin()) halt("Unable to initialize radio module");

  nrf24.setChannel(NRF24_CHANNEL);
  nrf24.setDataRate(NRF24_RATE);
  nrf24.setAutoAck(1);
  nrf24.setPayloadSize(NRF24_PAYLOAD_SZ);
  nrf24.openWritingPipe(nrf24_remote_txpipe);
  nrf24.openReadingPipe(1, nrf24_scoreboard_txpipe);
  nrf24.stopListening();

  if (nrf24.getChannel() != NRF24_CHANNEL)
    halt("Failed to set radio channel");

  if (nrf24.getPayloadSize() != NRF24_PAYLOAD_SZ)
    halt("Failed to set payload size");

  serial_msg("Radio module initialized. Current settings:");

  #ifdef SERIAL_DEBUG
    nrf24.printDetails();
  #endif
}

void setup() {
  #ifdef SERIAL_DEBUG
    Serial.begin(38400);
    delay(2000);
    serial_msg("Scoreboard remote version " BOARD_REMOTE_VER);
  #endif
  serial_msg("Remote starting..");
  Serial.flush();
  printf_begin();
  input_setup();
  radio_setup();
}

/*
 * Generic packet format:
 *
 *  +-------------+-------------------------------+
 *  | Payload len |      Payload + padding        |
 *  +-------------+-------------------------------+
 *      1 byte         NRF24_PAYLOAD_SZ - 1
 *
 *
 * Scoreboard payload format:
 * 
 *  +---------------+---------------+
 *  |  Down events  |   Up events   |
 *  +---------------+---------------+
 *      2 bytes         2 bytes
 */
bool sendmsg(const unsigned char *msg, size_t sz) {
  unsigned char msg_out[NRF24_PAYLOAD_SZ] = {0};

  msg_out[0] = sz;
  memcpy(msg_out+1, msg, sz);

  serial_msg("trying to write..");
  Serial.flush();
  size_t attempts;
  for (attempts = 0; attempts < NRF24_RETRIES; attempts++) {
    if (nrf24.write(msg_out, NRF24_PAYLOAD_SZ))
      break;
    serial_msg("Attempted to write..");
    Serial.flush();
  }
  serial_msg("Write returned");

  if (attempts == NRF24_RETRIES) {
    serial_msg("Failed to send/receive ACK");
    return false;
  }
  return true;
}

bool send_btn_events(uint16_t down_events, uint16_t up_events) {
  uint16_t msg[2] = {0};
  msg[0] = down_events;
  msg[1] = up_events;
  serial_msg("calling sendmsg()");
  Serial.flush();
  return sendmsg((uint8_t*)msg, 4);
}

void deep_sleep() {
  nrf24.powerDown();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  cli();

  /* Re-check sleep condition to avoid race */
  if (button_state) {
    sei();
    nrf24.powerUp();
    return;
  }

  /* Reset debounce timers */
  for (int i = 0; i < SW_COUNT; ++i) {
    sw_debounce_ticks[i] = 0;
  }

  sleep_enable();
  sleep_bod_disable();
  sei();
  sleep_cpu(); /* Guaranteed to execute before any interrupts */
  sleep_disable();

  /* Power up radio */
  nrf24.powerUp();
}

void loop() {
  while (true){//zmillis() - last_activity_time < IDLE_ON_TIME) {
    /* Read button events and timers */
    noInterrupts();
    uint16_t cur_btn_up_events = button_up_events;
    button_up_events = 0x0000;
    uint16_t cur_btn_down_events = button_down_events;
    button_down_events = 0x0000;
    interrupts();

    if (cur_btn_up_events || cur_btn_down_events) {
      last_activity_time = millis();
      send_btn_events(cur_btn_down_events, cur_btn_up_events);      
    }

    /* Always handle button events */
    for (int i = 0; i < SW_COUNT; ++i) {
      if (cur_btn_up_events & (1 << i)) {
        serial_msg(String("Button ") + i + " up");
      }
      if (cur_btn_down_events & (1 << i)) {
        serial_msg(String("Button ") + i + " down");
      }
    }
  }

  //deep_sleep();
  last_activity_time = millis();
}
