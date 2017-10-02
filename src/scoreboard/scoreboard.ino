/*
 * Led disp driver
 */

#include <Wire.h>
#include "RF24.h"
#include "circular_buffer.h"
#include "constants.h"
/* RADIO STUFF */

/* Radio driver. CE on pin GPIO15, CSN on GPIO2 */
RF24 nrf24(15,2);

const uint64_t nrf24_remote_txpipe     = 0x531ab153ddc134ef;
const uint64_t nrf24_scoreboard_txpipe = 0x1389dfca645fabef;

#define NRF24_CHANNEL       85
#define NRF24_PA            RF24_PA_MAX
#define NRF24_RATE          RF24_250KBPS
#define NRF24_PAYLOAD_SZ    5
#define NRF24_RETRIES       5

#define SERIAL_DEBUG 1

void serial_msg(const String& msg) {
  #ifdef SERIAL_DEBUG
    Serial.println(msg);
    Serial.flush();
  #endif
}

void radio_setup() {
  serial_msg("Initializing radio module...");
  if (!nrf24.begin()) serial_msg("Failed to init nrf24");

  nrf24.setChannel(NRF24_CHANNEL);
  nrf24.setDataRate(NRF24_RATE);
  nrf24.setAutoAck(1);
  nrf24.setPayloadSize(NRF24_PAYLOAD_SZ);
  nrf24.openWritingPipe(nrf24_scoreboard_txpipe); 
  nrf24.openReadingPipe(1, nrf24_remote_txpipe);
  nrf24.startListening();

  if (nrf24.getChannel() != NRF24_CHANNEL)
    return;

  if (nrf24.getPayloadSize() != NRF24_PAYLOAD_SZ)
    return;

  serial_msg("Radio module initialized. Current settings:");
}

/* END RADIO STUFF */

#define NUMBER_OF_CONTROLLERS 6
#define NUMBER_OF_DISPLAYS 12 // Two displays per controller

/*
 *   +---6---+
 *   |       |
 *   1       5
 *   |       |
 *   +---0---+
 *   |       |
 *   4       2
 *   |       |
 *   +---3---+ 77
 *             77
 */
const uint8_t led_numbers[] = {0x7E, 0x24, 0x79, 0x6D, 0x27, 0x4F, 0x5F, 0x64, 0x7F, 0x6F};

class LedDisplayDriver
{
public:
  LedDisplayDriver()
  {
    memset(led_state, 0, sizeof(led_state));
  }

  void init()
  {
    Wire.begin();
    for (int i = 0; i < NUMBER_OF_CONTROLLERS; ++i) {
      Wire.beginTransmission(controllerToAddress(i));
      Wire.write("\x00\x01", 2);
      int ret = Wire.endTransmission();
      if (ret) Serial.println(String("Failed to init ") + controllerToAddress(i) + " Error: " + ret);
    }
  }

  void setNumber(int disp, int number)
  {
    for (int i = 0; i < 8; ++i) {
      if ((1 << i) & led_numbers[number]) {
        setLedState(disp, i, true);
      } else {
        setLedState(disp, i, false);
      }
    }
  }

  void displayOff(int disp)
  {
    for (int i = 0; i < 8; ++i) {
      setLedState(disp, i, false);
    }
  }

  void displayFull(int disp)
  {
    for (int i = 0; i < 8; ++i) {
      setLedState(disp, i, true);
    }
  }

  void setLedState(int disp, int led, bool value)
  {
    if (value) {
      led_state[disp/2][(disp % 2) * 2 + led / 4] |= 1 << ((led % 4) * 2);
    } else {
      led_state[disp/2][(disp % 2) * 2 + led / 4] &= ~(1 << ((led % 4) * 2));
    }
  }

  void writeLedState()
  {
    for (int i = 0; i < NUMBER_OF_CONTROLLERS; ++i) {
      Wire.beginTransmission(controllerToAddress(i));
      unsigned char ctrl = 0x94;
      Wire.write(ctrl);
      Wire.write(led_state[i], 4);
      int ret = Wire.endTransmission();
      if (ret != 0) {
        Serial.println(String("Failed to transmit to ") + i + ". Error: " + ret);
      }
    }
  }
  
private:
  unsigned char led_state[NUMBER_OF_CONTROLLERS][4];
  
  unsigned char controllerToAddress(int controller)
  {
    /** Addresses by board:
      * Board 1: 0x6E 0x6F
      * Board 2: 0x6C 0x67
      * Board 3: 0x6A 0x6D
      */
    switch (controller) {
      case 0:
        return 0x6E;
      case 1:
        return 0x6F;
      case 2:
        return 0x6C;
      case 3:
        return 0x67;
      case 4:
        return 0x6A;
      case 5:
        return 0x6D;
      default:
        Serial.println(String("Invalid controller number (") + controller + ")");
        return 0x00;
    }
  }
};


struct ScoreboardState {
    int breakPoints;
    int player_A_sets;
    int player_B_sets;
    int player_A_points;
    int player_B_points;
};

class Scoreboard {
  public:
    enum Player {
      A,
      B
    };

    Scoreboard() {
        ScoreboardState initialState;
        undoBuf.push_item(initialState);
    }

    void addPointsToBreak(int points) {
        ScoreboardState state = undoBuf.peak_item();
        state.breakPoints += points;
        undoBuf.push_item(state);
    }

    void addBreakToPlayer(Player player) {
        ScoreboardState state = undoBuf.peak_item();
        if (player == A) state.player_A_points += state.breakPoints;
        else state.player_B_points += state.breakPoints;
        state.breakPoints = 0;
        undoBuf.push_item(state);
    }

    void addPointsToPlayer(Player player, int points) {
        ScoreboardState state = undoBuf.peak_item();
        if (player == A) state.player_A_points += points;
        else state.player_B_points += points;
        undoBuf.push_item(state);
    }

    void addSetToPlayer(Player player) {
        ScoreboardState state = undoBuf.peak_item();
        if (player == A) state.player_A_sets++;
        else state.player_B_sets++;
        state.breakPoints = 0;
        state.player_A_points = 0;
        state.player_B_points = 0;
        undoBuf.push_item(state);
    }

    void undo() {
        undoBuf.pop_item();
    }

    void reset() {
        ScoreboardState initialState;
        undoBuf.push_item(initialState);
    }

  private:
    CircularBuffer<ScoreboardState, 10> undoBuf;
};

LedDisplayDriver driver;

void setup() {
  Serial.begin(115200);
  delay(200);
  radio_setup();
  driver.init();
  delay(200);
  Serial.println("Hello, led display here");
  for (int i = 0; i < NUMBER_OF_DISPLAYS; ++i) {
    driver.displayFull(i);
  }
  driver.writeLedState();

  /*delay(500);
  for (int i = 0; i < NUMBER_OF_DISPLAY; ++i) {
    driver.displayOff(i);
  }
  driver.writeLedState();*/
}

void parse_payload(uint8_t* buf) {
  if (buf[0] != 4) {
    Serial.println("length not 4");
  }
  uint8_t* events = (uint8_t*)(buf + 1);
  for (int btn = 0; btn < 10; ++btn) {
    serial_msg(String("Reading event ") + btn);
    if (btn < 8) {
      if (events[0] & (1 << btn)) {
        Serial.println(String("Button ") + btn + " down");
        for (int disp = 0; disp < NUMBER_OF_DISPLAYS; ++disp) {
          driver.setNumber(disp, btn%10);
        }
      }
    } else {
      if (events[1] & (1 << btn%8)) {
        Serial.println(String("Button ") + btn + " down");
        for (int disp = 0; disp < NUMBER_OF_DISPLAYS; ++disp) {
          driver.setNumber(disp, btn%10);
        }
      }
    }
    if (btn < 8) {
      if (events[2] & (1 << btn)) {
        Serial.println(String("Button ") + btn + " up");
        for (int disp = 0; disp < NUMBER_OF_DISPLAYS; ++disp) {
          driver.displayOff(disp);
        }
      }
    } else {
      if (events[3] & (1 << btn%8)) {
        Serial.println(String("Button ") + btn + " up");
        for (int disp = 0; disp < NUMBER_OF_DISPLAYS; ++disp) {
          driver.displayOff(disp);
        }
      }
    }
    driver.writeLedState();
  }
}

int currentNumber = 0;
void loop() {
  uint8_t pipe;
  uint8_t buf[5];

  if (nrf24.available(&pipe)) {
    nrf24.read(buf, sizeof(buf));
    serial_msg("Read radio");
    parse_payload(buf);
  }
  // put your main code here, to run repeatedly:
  /*for (int i = 0; i < NUMBER_OF_DISPLAYS; ++i) {
    driver.setNumber(i, (currentNumber + i) % 10);
  }
  driver.writeLedState();
  Serial.println(String("Current number: ") + currentNumber);
  currentNumber = (currentNumber + 1) % 10;
  delay(2000);*/
}

