#include "flashmem.h"
#include <EEPROM.h>

FlashMem::FlashMem() {
  this->content_valid = false;

  EEPROM.begin(FLASHMEM_SZ);

  for(size_t i = 0; i < sizeof(content); i++)
    this->content_bytes[i] = EEPROM.read(FLASHMEM_ADDR + i);

  if (this->content.ssid_len <= 127 &&
      this->content.pwd_len  <= 127)
  {
    this->content_valid = true;
    this->content.ssid[content.ssid_len] = 0x00;
    this->content.pwd[content.pwd_len] = 0x00;
  }
}

bool FlashMem::is_valid() {
  return this->content_valid;
}

String FlashMem::get_ssid() {
  if (!this->content_valid)
    return "";

  return String((char*)this->content.ssid);
}

String FlashMem::get_password() {
  if (!this->content_valid)
    return "";

  return String((char*)this->content.pwd);
}

bool FlashMem::set_ssid(String &ssid) {
  if (!this->content_valid)
    return false;

  for (size_t i = 0; i < ssid.length(); i++)
    this->content.ssid[i] = ssid.c_str()[i];

  this->content.ssid[ssid.length()] = 0;
  this->content.ssid_len = ssid.length();
  return true;
}

bool FlashMem::set_password(String &password) {
  if (!this->content_valid)
    return false;

  for (size_t i = 0; i < password.length(); i++)
    this->content.pwd[i] = password.c_str()[i];

  this->content.pwd[password.length()] = 0;
  this->content.pwd_len = password.length();
  return true;
}

bool FlashMem::commit() {
  for (size_t i = 0; i < sizeof(content); i++)
    EEPROM.write(FLASHMEM_ADDR + i, this->content_bytes[i]);

  if (this->content.ssid_len <= 127 &&
      this->content.pwd_len  <= 127)
    this->content_valid = true;

  return EEPROM.commit();
}
