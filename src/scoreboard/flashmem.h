#ifndef FLASHMEM_H
#define FLASHMEM_H

#include <Arduino.h>

#define FLASHMEM_SZ 512
#define FLASHMEM_ADDR 0x00

#define FLASHMEM_MAX_SSID_LEN (128-1)
#define FLASHMEM_MAX_PWD_LEN (128-1)

class FlashMem {
  private:
    struct flash_content {
      uint8_t ssid_len;
      uint8_t ssid[128];
      uint8_t pwd_len;
      uint8_t pwd[128];
    } __attribute__((packed));

    union {
      struct flash_content content;
      uint8_t content_bytes[sizeof(struct flash_content)];
    };

  public:
    FlashMem();
    bool validate();
    String get_ssid();
    String get_password();
    bool set_ssid(String &ssid);
    bool set_password(String &password);
    bool commit();
};

#endif
