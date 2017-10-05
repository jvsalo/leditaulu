#include "flashmem.h"
#include <EEPROM.h>

FlashMem::FlashMem() {
  Serial.println("Initializing flashmem");

  EEPROM.begin(FLASHMEM_SZ);

  for(size_t i = 0; i < sizeof(this->content); i++) {
    this->content_bytes[i] = EEPROM.read(FLASHMEM_ADDR + i);
    // Serial.print((char)this->content_bytes[i]);
  }

  Serial.println("");
}

bool FlashMem::validate() {
  if (this->content.ssid_len <= FLASHMEM_MAX_SSID_LEN &&
      this->content.pwd_len  <= FLASHMEM_MAX_PWD_LEN)
  {
    this->content.ssid[content.ssid_len] = 0x00;
    this->content.pwd[content.pwd_len] = 0x00;
    return true;
  }

  return false;
}

String FlashMem::get_ssid() {
  if (!this->validate())
    return "";

  return String((char*)this->content.ssid);
}

String FlashMem::get_password() {
  if (!this->validate())
    return "";

  return String((char*)this->content.pwd);
}

bool FlashMem::set_ssid(String &ssid) {
  if (ssid.length() > FLASHMEM_MAX_SSID_LEN)
    return false;

  for (size_t i = 0; i < ssid.length(); i++)
    this->content.ssid[i] = ssid.c_str()[i];

  this->content.ssid[ssid.length()] = 0;
  this->content.ssid_len = ssid.length();
  return true;
}

bool FlashMem::set_password(String &password) {
  if (password.length() > FLASHMEM_MAX_PWD_LEN)
    return false;

  for (size_t i = 0; i < password.length(); i++)
    this->content.pwd[i] = password.c_str()[i];

  this->content.pwd[password.length()] = 0;
  this->content.pwd_len = password.length();
  return true;
}

bool FlashMem::commit() {
  for (size_t i = 0; i < sizeof(this->content); i++) {
    EEPROM.write(FLASHMEM_ADDR + i, this->content_bytes[i]);
    // Serial.print((char)this->content_bytes[i]);
  }

  // Serial.println("");
  return EEPROM.commit();
}
