#ifndef FLASHMEM_H
#define FLASHMEM_H

#include <Arduino.h>

#define FLASHMEM_SZ 512
#define FLASHMEM_ADDR 0x00

class FlashMem {
  private:
    struct flash_content {
      uint8_t ssid_len;
      uint8_t ssid[128];
      uint8_t pwd_len;
      uint8_t pwd[128];
    } __attribute__((packed));

    bool content_valid;
    struct flash_content content;

  public:
    FlashMem();
    bool is_valid();
    bool get_ssid(String &ssid);
    bool get_password(String &ssid);
    bool set_ssid(String &ssid);
    bool set_password(String &password);
    bool commit();
};

#endif
