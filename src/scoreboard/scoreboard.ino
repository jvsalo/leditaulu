/*
 * Led disp driver
 */

#include <Wire.h>
#include <WebSocketsServer.h>
#include <ESP8266mDNS.h>
#include "RF24.h"
#include "circular_buffer.h"
#include "constants.h"
#include "flashmem.h"
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

#define DOWN 0
#define UP 1

#define LONG_PRESS_TIME_MILLIS 2000

#define SCOREBOARD_DNS_NAME "scoreboard"

unsigned char button_state[SW_COUNT];
unsigned int button_time[SW_COUNT];
bool button_handled[SW_COUNT];

FlashMem flash;

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
#define NUMBER_OF_DISPLAYS 10 // Two displays per controller

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
String player_A_name = "Player A";
String player_B_name = "Player B";

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
    Wire.setClock(50000);
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

WebSocketsServer webSocket = WebSocketsServer(81);

struct ScoreboardState {
    int break_points;
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
        ScoreboardState initialState = {0,0,0,0,0};
        undoBuf.push_item(initialState);
    }

    void addPointsToBreak(int points) {
        ScoreboardState state = undoBuf.peek_item();
        state.break_points += points;
        undoBuf.push_item(state);
    }

    void addBreakToPlayer(Player player) {
        ScoreboardState state = undoBuf.peek_item();
        if (player == A) state.player_A_points += state.break_points;
        else state.player_B_points += state.break_points;
        state.break_points = 0;
        undoBuf.push_item(state);
    }

    void addPointsToPlayer(Player player, int points) {
        ScoreboardState state = undoBuf.peek_item();
        if (player == A) state.player_A_points += points;
        else state.player_B_points += points;
        undoBuf.push_item(state);
    }

    void addSetToPlayer(Player player) {
        ScoreboardState state = undoBuf.peek_item();
        if (player == A) state.player_A_sets++;
        else state.player_B_sets++;
        state.break_points = 0;
        state.player_A_points = 0;
        state.player_B_points = 0;
        undoBuf.push_item(state);
    }

    void undo() {
        undoBuf.pop_item();
    }

    void reset() {
        ScoreboardState initialState = {0, 0, 0, 0, 0};
        undoBuf.push_item(initialState);
    }

    void genStateMsg(char* msg, size_t size) {
        ScoreboardState state = undoBuf.peek_item();
        int written;
        written = snprintf(msg, size, "{\"scoreA\":\"%d\",\"scoreB\":\"%d\",\"setA\":\"%d\",\"setB\":\"%d\",\"break\":\"%d\",\"nameA\":\"%s\",\"nameB\":\"%s\"}",
                 state.player_A_points, state.player_B_points, state.player_A_sets,
                 state.player_B_sets, state.break_points, player_A_name.c_str(), player_B_name.c_str());
        if (written >= size) {
            Serial.println("state msg buffer too small");
        }
    }

    void drawState(LedDisplayDriver driver) {
        ScoreboardState state = undoBuf.peek_item();
        /* Player A score */
        driver.setNumber(DISP_A_SCORE_0, state.player_A_points / 100);
        driver.setNumber(DISP_A_SCORE_1, (state.player_A_points % 100) / 10);
        driver.setNumber(DISP_A_SCORE_2, state.player_A_points % 10);
        /* Player B score */
        driver.setNumber(DISP_B_SCORE_0, state.player_B_points / 100);
        driver.setNumber(DISP_B_SCORE_1, (state.player_B_points % 100) / 10);
        driver.setNumber(DISP_B_SCORE_2, state.player_B_points % 10);
        /* Break */
        driver.setNumber(DISP_BREAK_0, (state.break_points % 100) / 10);
        driver.setNumber(DISP_BREAK_1, state.break_points % 10);
        /* Sets */
        driver.setNumber(DISP_A_SET, state.player_A_sets % 10);
        driver.setNumber(DISP_B_SET, state.player_B_sets % 10);

        driver.writeLedState();

        char stateMsg[200];
        genStateMsg(stateMsg, 200);
        webSocket.broadcastTXT(stateMsg);
    }

    ScoreboardState getState() {
        return undoBuf.peek_item();
    }
  private:
    CircularBuffer<ScoreboardState, 10> undoBuf;
};

LedDisplayDriver driver;
Scoreboard scoreboard;

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

/* Set these to your desired credentials. */
const char *ssid = "tuxera-visitors";
const char *password = "12angrymen1957";

ESP8266WebServer server(80);

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println(String("") + String(num, DEC) + " disconnected");
            break;
        case WStype_CONNECTED:
            Serial.println(String("") + String(num, DEC) + " connected");
            char stateMsg[200];
            scoreboard.genStateMsg(stateMsg, 200);
            webSocket.sendTXT(num, stateMsg);
            break;
        case WStype_TEXT:
            Serial.println(String("") + String(num, DEC) + " sent " + (char*)payload);
            break;
        case WStype_BIN:
            Serial.println(String("") + String(num, DEC) + " sent binary");
            break;
    }
}

void setupWebSocket() {
    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent);
}

void handleSetNames() {
    Serial.println("Setting names");
    int error = 0;
    if (server.hasArg("PlayerA")) {
        Serial.println("Setting player A");
        player_A_name = server.arg("PlayerA");
    } else {
        server.send(400, "text/html");
        return;
    }
    if (server.hasArg("PlayerB")) {
        Serial.println("Setting player B");
        player_B_name = server.arg("PlayerB");
    } else {
        server.send(400, "text/html");
        return;
    }
    char stateMsg[200];
    scoreboard.genStateMsg(stateMsg, 200);
    server.sendHeader("Location", "/");
    server.send(302, "text/html");
    webSocket.broadcastTXT(stateMsg);
}

void handleNameSettings() {
    Serial.println("Handle name settings");
    const char* response = "<form action='/setnames'>Player A name:<br><input type='text' name='PlayerA'><br>Player B name:<br><input type='text' name='PlayerB'><br><br><input type='submit' value='submit'></form>";
    server.send(200, "text/html", response);
}

void handleWifiSettings() {
    Serial.println("Handle wifi settings");
    const char* response = "<form action='/setwifi'>SSID:<br><input type='text' name='SSID'><br>Password:<br><input type='text' name='PASSWORD'><br><br><input type='submit' value='submit'></form>";
    server.send(200, "text/html", response);
}

void handleSetWifi() {
    Serial.println("Handle set wifi");
    if (server.hasArg("SSID") && server.hasArg("PASSWORD")) {
        String ssid = server.arg("SSID");
        String password = server.arg("PASSWORD");
        flash.set_ssid(ssid);
        flash.set_password(password);
        if (!flash.commit()) {
          server.send(400, "text/html");
          return;
        }
        Serial.println(String("Setting wifi to ") + ssid + "/" + password);
        server.sendHeader("Location","/");
        server.send(302, "text/html");
    } else {
        server.send(400, "text/html");
    }
}

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
 * connected to this access point to see it.
 */
void handleRoot() {
  Serial.println("Handle root");
  const char* response = "<!DOCTYPE html><head><title>Biklu scoreboard</title><style type='text/css'>.elem{display: inline-block; vertical-align: top; text-align:center; margin: 10px; font-size: 25px; font-family: verdana;}.subelem{margin: 10px;}</style><script>function renderScore(score){var A_score=document.getElementById('A_score'); var B_score=document.getElementById('B_score'); var brk=document.getElementById('break'); var A_name=document.getElementById('player_A'); var B_name=document.getElementById('player_B'); A_score.innerHTML='' + score.scoreA + ' (' + score.setA + ')'; B_score.innerHTML='' + '(' + score.setB + ') ' + score.scoreB; brk.innerHTML='' + score.break; A_name.innerHTML='' + score.nameA; B_name.innerHTML='' + score.nameB;}window.onload=function(){var connection=new WebSocket('ws://'+location.hostname+':81/',['arduino']); connection.onopen=function (){console.log('Opened websocket connection'); connection.send('Connected');}; connection.onerror=function (error){console.log('WebSocket Error ', error);}; connection.onmessage=function (e){console.log('got message: ' + e.data); renderScore(JSON.parse(e.data));};}</script></head><body> <div id=container> <div class=elem> <div class=subelem id=player_A></div><div class=subelem id=A_score></div></div><div class=elem> <div class=subelem>BREAK</div><div class=subelem id=break></div></div><div class=elem> <div class=subelem id=player_B></div><div class=subelem id=B_score></div></div></div></body>";

  Serial.println("Created response");
  server.send(200, "text/html", response);
  Serial.println("Sent response");
}

void setupWebServer() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  delay(500);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.on("/", handleRoot);
  server.on("/names", handleNameSettings);
  server.on("/setnames", handleSetNames);
  server.on("/wifi", handleWifiSettings);
  server.on("/setwifi", handleSetWifi);
  server.begin();
  Serial.println("HTTP server started");
}

void setupMDNS() {
    if (MDNS.begin(SCOREBOARD_DNS_NAME)) {
        Serial.println("MDNS responder started");
    }
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  radio_setup();
  driver.init();
  delay(200);
  Serial.println("Hello, led display here");
  setupWebServer();
  setupWebSocket();
  setupMDNS();
  for (int i = 0; i < SW_COUNT; ++i) {
    button_handled[i] = true;
  }
  scoreboard.drawState(driver);
  //scoreboard.drawState(driver);
  /*delay(500);
  for (int i = 0; i < NUMBER_OF_DISPLAY; ++i) {
    driver.displayOff(i);
  }
  driver.writeLedState();*/

  flash.init();
  if (!flash.is_valid())
    Serial.println("WiFi parameters not valid");

  else
    Serial.println(String("Wifi parameters: SSID = ") + flash.get_ssid() +
                   ", password = " + flash.get_password());
}

void onButtonDown(int btn) {
  switch (btn) {
    case BTN_ADD_1:
      scoreboard.addPointsToBreak(1);
      break;
    case BTN_ADD_2:
      scoreboard.addPointsToBreak(2);
      break;
    case BTN_ADD_3:
      scoreboard.addPointsToBreak(3);
      break;
    case BTN_ADD_4:
      scoreboard.addPointsToBreak(4);
      break;
    case BTN_ADD_5:
      scoreboard.addPointsToBreak(5);
      break;
    case BTN_ADD_6:
      scoreboard.addPointsToBreak(6);
      break;
    case BTN_ADD_7:
      scoreboard.addPointsToBreak(7);
      break;
    case BTN_TO_A:
      button_state[btn] = DOWN;
      button_time[btn] = millis();
      button_handled[btn] = false;
      break;
    case BTN_TO_B:
      button_state[btn] = DOWN;
      button_time[btn] = millis();
      button_handled[btn] = false;
      break;
    case BTN_RST:
      button_state[btn] = DOWN;
      button_time[btn] = millis();
      button_handled[btn] = false;
      break;
    default:
      Serial.println("Unknown button");
  }
  scoreboard.drawState(driver);
}

void onButtonUp(int btn) {
  switch (btn) {
    case BTN_ADD_1:
      break;
    case BTN_ADD_2:
      break;
    case BTN_ADD_3:
      break;
    case BTN_ADD_4:
      break;
    case BTN_ADD_5:
      break;
    case BTN_ADD_6:
      break;
    case BTN_ADD_7:
      break;
    case BTN_TO_A:
      if (button_state[btn] == DOWN && (millis() - button_time[btn]) < LONG_PRESS_TIME_MILLIS) {
        scoreboard.addBreakToPlayer(Scoreboard::Player::A);
      }
      button_state[btn] = UP;
      break;
    case BTN_TO_B:
      if (button_state[btn] == DOWN && (millis() - button_time[btn]) < LONG_PRESS_TIME_MILLIS) {
        scoreboard.addBreakToPlayer(Scoreboard::Player::B);
      }
      button_state[btn] = UP;
      break;
    case BTN_RST:
      if (button_state[btn] == DOWN && (millis() - button_time[btn]) < LONG_PRESS_TIME_MILLIS) {
        scoreboard.undo();
      }
      button_state[btn] = UP;
      break;
    default:
      Serial.println("Unknown up event");
  }
  scoreboard.drawState(driver);
}

void parse_payload(uint8_t* buf) {
  if (buf[0] != 4) {
    Serial.println("length not 4");
  }
  uint8_t* events = (uint8_t*)(buf + 1);
  for (int btn = 0; btn < SW_COUNT; ++btn) {
    if (btn < 8) {
      if (events[0] & (1 << btn)) {
        Serial.println(String("Button ") + btn + " down");
        onButtonDown(btn);
      }
    } else {
      if (events[1] & (1 << btn%8)) {
        Serial.println(String("Button ") + btn + " down");
        onButtonDown(btn);
      }
    }
    if (btn < 8) {
      if (events[2] & (1 << btn)) {
        Serial.println(String("Button ") + btn + " up");
        onButtonUp(btn);
      }
    } else {
      if (events[3] & (1 << btn%8)) {
        Serial.println(String("Button ") + btn + " up");
        onButtonUp(btn);
      }
    }
  }
}

bool check_long_press(int btn) {
  if (button_state[btn] == DOWN && (millis() - button_time[btn]) >= LONG_PRESS_TIME_MILLIS && !button_handled[btn])
    return true;
  else
    return false;
}

void check_buttons() {
  bool needToDraw = false;
  if (check_long_press(BTN_TO_A)) {
    Serial.println("Long pressed TO_A");
    scoreboard.addSetToPlayer(Scoreboard::Player::A);
    button_handled[BTN_TO_A] = true;
    needToDraw = true;
  }
  if (check_long_press(BTN_TO_B)) {
    Serial.println("Long pressed TO_B");
    scoreboard.addSetToPlayer(Scoreboard::Player::B);
    button_handled[BTN_TO_B] = true;
    needToDraw = true;
  }
  if (check_long_press(BTN_RST)) {
    Serial.println("Long pressed RST");
    scoreboard.reset();
    button_handled[BTN_RST] = true;
    needToDraw = true;
  }
  if (needToDraw)
    scoreboard.drawState(driver);
}

int currentNumber = 0;
void loop() {
  uint8_t pipe;
  uint8_t buf[5];

  check_buttons();
  if (nrf24.available(&pipe)) {
    nrf24.read(buf, sizeof(buf));
    parse_payload(buf);
  }
  server.handleClient();
  webSocket.loop();
  // put your main code here, to run repeatedly:
  /*for (int i = 0; i < NUMBER_OF_DISPLAYS; ++i) {
    driver.setNumber(i, (currentNumber + i) % 10);
  }
  driver.writeLedState();
  Serial.println(String("Current number: ") + currentNumber);
  currentNumber = (currentNumber + 1) % 10;
  delay(2000);*/
}

