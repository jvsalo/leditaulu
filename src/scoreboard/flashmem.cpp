#include "flashmem.h"
#include <EEPROM.h>

FlashMem::FlashMem() {
  this->content_valid = false;

  EEPROM.begin(FLASHMEM_SZ);

  for(size_t i = 0; i < sizeof(content); i++)
    ((char*)&this->content)[i] = EEPROM.read(FLASHMEM_ADDR + i);

  if (content.ssid_len <= 127 && content.pwd_len <= 127)
    this->content_valid = true;

  this->content.ssid[content.ssid_len] = 0x00;
  this->content.pwd[content.pwd_len] = 0x00;
}

bool FlashMem::is_valid() {
  return this->content_valid;
}

bool FlashMem::get_ssid(String &ssid) {
  if (!this->content_valid)
    return false;

  ssid = String((char*)this->content.ssid);
}

bool FlashMem::get_password(String &password) {
  if (!this->content_valid)
    return false;

  password = String((char*)this->content.pwd);
}

bool FlashMem::set_ssid(String &ssid) {
  if (ssid.length() > 127)
    return false;

  for (size_t i = 0; i < ssid.length(); i++)
    this->content.ssid[i] = ssid.c_str()[i];

  this->content.ssid[ssid.length()] = 0;
  this->content.ssid_len = ssid.length();
  return true;
}

bool FlashMem::set_password(String &password) {
  if (password.length() > 127)
    return false;

  for (size_t i = 0; i < password.length(); i++)
    this->content.pwd[i] = password.c_str()[i];

  this->content.pwd[password.length()] = 0;
  this->content.pwd_len = password.length();
  return true;
}

bool FlashMem::commit() {
  for (size_t i = 0; i < sizeof(content); i++)
    EEPROM.write(FLASHMEM_ADDR + i, ((char*)&this->content)[i]);

  return EEPROM.commit();
}
