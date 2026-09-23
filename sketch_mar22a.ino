#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

// === 設定區 ===
const char* prefix = ".1.Free-WiFi"; // 名稱
const int total_ssids = 79;         // 生成數量 (建議不超過 80

// 封包 Header
uint8_t packet[128] = { 
  0x80, 0x00, 0x00, 0x00, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0xc0, 0x6c, 
  0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, 
  0x64, 0x00, 
  0x01, 0x04, 
  0x00 
};

void setup() {
  delay(300);
  WiFi.mode(WIFI_OFF);
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(1);
  Serial.begin(115200);
}

void loop() {
  // 快速輪播生成編號
  for (int i = 1; i <= total_ssids; i++) {
    
    // 1. 動態組裝名稱 (例如: SkyNet_Bot_42)
    char ssid_buffer[32];
    sprintf(ssid_buffer, "%s%02d", prefix, i); // %02d 表示補零的兩位數
    
    // 2. 根據編號 i 產生固定的 MAC (確保編號 01 永遠是同一個 MAC)
    uint8_t mac_last_byte = (uint8_t)i;
    uint8_t mac_second_byte = (uint8_t)(i * 3); // 稍微混淆一下
    
    generatePacket(ssid_buffer, mac_second_byte, mac_last_byte);
    
    // 極速發送：因為數量多，延遲要縮極短才能在手機 timeout 前發完一輪
    delay(5); 
  }
}

void generatePacket(char* ssid, uint8_t mac2, uint8_t mac1) {
  int ssid_len = strlen(ssid);

  // 設定 MAC: 隨機前綴 + 固定後綴 (確保手機認得它是同一台)
  packet[10] = packet[16] = 0xAA;
  packet[11] = packet[17] = 0xBB;
  packet[12] = packet[18] = 0x02;
  packet[13] = packet[19] = 0x03;
  packet[14] = packet[20] = mac2; 
  packet[15] = packet[21] = mac1;

  packet[37] = ssid_len;

  for (int i = 0; i < ssid_len; i++) {
    packet[38 + i] = ssid[i];
  }

  uint8_t postSSID[] = {
    0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, 
    0x03, 0x01, 0x01 
  };

  for (int i = 0; i < 13; i++) {
    packet[38 + ssid_len + i] = postSSID[i];
  }

  int packetSize = 38 + ssid_len + 13;
  wifi_send_pkt_freedom(packet, packetSize, 0);
}