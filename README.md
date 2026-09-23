# ESP8266 Wi-Fi Beacon 封包實驗
---
#利用esp8266製造出大量的假網路

## 1. 實驗簡介

本實驗使用 **ESP8266** 搭配 Arduino IDE，透過 ESP8266 的底層 Wi-Fi API 建立並送出自訂的 **802.11 Beacon Frame**。

Beacon Frame 是無線基地台定期廣播的管理封包，主要用途是讓附近的裝置知道：

* 附近有哪些 Wi-Fi 網路
* Wi-Fi 網路的 SSID 名稱
* 無線網路的基本參數
* 該無線網路的 MAC Address（BSSID）

本程式透過修改 Beacon Frame 中的 **SSID** 與 **MAC Address**，讓 ESP8266 在測試環境中產生多組不同的 Wi-Fi 網路資訊。

> ⚠️ 本程式涉及低階 Wi-Fi 封包傳送。請只在自己的設備、實驗室或明確授權的測試環境中使用，避免干擾其他人的無線網路。

---

# 2. 使用環境

| 項目       | 內容                          |
| -------- | --------------------------- |
| 開發板      | ESP8266                     |
| 開發環境     | Arduino IDE                 |
| 程式語言     | C++                         |
| Wi-Fi 標準 | IEEE 802.11                 |
| 使用 API   | ESP8266 Wi-Fi Low-Level API |
| 傳輸封包     | Beacon Frame                |

---

# 3. 引入函式庫

```cpp
#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}
```

### `ESP8266WiFi.h`

這是 ESP8266 的 Wi-Fi 函式庫，可以用來控制 ESP8266 的 Wi-Fi 功能。

例如：

```cpp
WiFi.mode(WIFI_OFF);
```

可以關閉一般的 Wi-Fi 功能。

### `user_interface.h`

這裡使用 ESP8266 比較底層的 Wi-Fi API。

本程式後面使用：

```cpp
wifi_set_opmode()
wifi_promiscuous_enable()
wifi_send_pkt_freedom()
```

這些函式可以進一步控制 ESP8266 的 Wi-Fi 行為。

---

# 4. 設定區

```cpp
const char* prefix = ".1.Free-WiFi";
const int total_ssids = 100;
```

這兩個變數用來控制產生的 Wi-Fi 名稱。

### `prefix`

```cpp
const char* prefix = ".1.Free-WiFi";
```

代表 Wi-Fi 名稱的前綴。

程式會在後面加入編號  ** (有試著將網路都命名成同一個，會受到 SSID + BSSID 等資訊以及手機掃描/顯示邏輯影響而最終只顯示一個) **

例如：

```text
.1.Free-WiFi01
.1.Free-WiFi02
.1.Free-WiFi03
...
```

### `total_ssids`

```cpp
const int total_ssids = 100;
```

代表程式要產生幾組名稱。

例如：

```cpp
total_ssids = 10;
```

就會產生：

```text
.1.Free-WiFi01
.1.Free-WiFi02
...
.1.Free-WiFi10
```

---

# 5. Beacon Frame 封包

程式預先建立一個封包陣列：

```cpp
uint8_t packet[128] = {
  0x80, 0x00, 0x00, 0x00,
  ...
};
```

`uint8_t` 可以理解成「一個 8-bit 的資料」。

因此：

```cpp
uint8_t packet[128];
```

就是建立一個最多可以存放 128 bytes 的資料陣列。

這個陣列會被當作 Wi-Fi 封包的資料內容。

---

# 6. Wi-Fi 模式設定

```cpp
void setup() {
  delay(300);
  WiFi.mode(WIFI_OFF);
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(1);
  Serial.begin(115200);
}
```

## `WiFi.mode(WIFI_OFF)`

```cpp
WiFi.mode(WIFI_OFF);
```

先關閉 ESP8266 原本的 Wi-Fi 模式。

---

## `wifi_set_opmode(STATION_MODE)`

```cpp
wifi_set_opmode(STATION_MODE);
```

將 ESP8266 設定為 Station Mode。

也就是一般連接 Wi-Fi 時使用的模式。

---

## `wifi_promiscuous_enable(1)`

```cpp
wifi_promiscuous_enable(1);
```

開啟較低階的 Wi-Fi 操作功能。

這讓程式可以進一步處理 802.11 封包。

---

## Serial

```cpp
Serial.begin(115200);
```

啟動 Serial Monitor：

```text
115200 baud
```

方便進行除錯與觀察程式狀態。

---

# 7. `loop()` 主程式

```cpp
void loop() {
  for (int i = 1; i <= total_ssids; i++) {

    char ssid_buffer[32];

    sprintf(
      ssid_buffer,
      "%s%02d",
      prefix,
      i
    );

    uint8_t mac_last_byte = (uint8_t)i;
    uint8_t mac_second_byte = (uint8_t)(i * 3);

    generatePacket(
      ssid_buffer,
      mac_second_byte,
      mac_last_byte
    );

    delay(5);
  }
}
```

程式會不斷執行：

```text
1
 ↓
產生 SSID
 ↓
產生對應的 MAC
 ↓
建立封包
 ↓
送出
 ↓
下一個 SSID
 ↓
...
```

跑完一次之後，`loop()` 又會重新開始。

因此會持續循環。

---

# 8. SSID 名稱產生

```cpp
char ssid_buffer[32];

sprintf(
  ssid_buffer,
  "%s%02d",
  prefix,
  i
);
```

假設：

```cpp
prefix = ".1.Free-WiFi";
i = 7;
```

因為：

```text
%02d
```

代表「至少兩位數，不足前面補 0」。

因此最後會得到：

```text
.1.Free-WiFi07
```

---

# 9. MAC Address 的產生

程式根據編號產生不同的 MAC Address 部分：

```cpp
uint8_t mac_last_byte = (uint8_t)i;

uint8_t mac_second_byte =
    (uint8_t)(i * 3);
```

例如：

```text
i = 1
i = 2
i = 3
```

就會產生不同的數值。

之後這些數值會被放進 Beacon Frame 的 MAC Address 欄位。

---

# 10. `generatePacket()`

```cpp
void generatePacket(
  char* ssid,
  uint8_t mac2,
  uint8_t mac1
)
```

這個函式主要負責：

1. 取得 SSID 長度
2. 修改 MAC Address
3. 將 SSID 放入封包
4. 加入 SSID 後面的 Wi-Fi 資訊
5. 計算封包長度
6. 傳送封包

可以把它想成：

```text
SSID
 ↓
MAC Address
 ↓
Beacon Frame
 ↓
完整封包
 ↓
傳送
```

---

# 11. 設定 MAC Address

```cpp
packet[10] = packet[16] = 0xAA;
packet[11] = packet[17] = 0xBB;
packet[12] = packet[18] = 0x02;
packet[13] = packet[19] = 0x03;
packet[14] = packet[20] = mac2;
packet[15] = packet[21] = mac1;
```

這裡是在修改封包中的 MAC Address 欄位。

可以看到：

```cpp
packet[10] ~ packet[15]
```

以及：

```cpp
packet[16] ~ packet[21]
```

被設定成相同的值。

因此封包中的不同 MAC 欄位會保持一致。

---

# 12. 將 SSID 放入封包

首先取得 SSID 長度：

```cpp
int ssid_len = strlen(ssid);
```

例如：

```text
.1.Free-WiFi01
```

會計算它有多少個字元。

接著：

```cpp
packet[37] = ssid_len;
```

把 SSID 長度寫入封包。

然後：

```cpp
for (int i = 0; i < ssid_len; i++) {
  packet[38 + i] = ssid[i];
}
```

逐一把 SSID 的字元放進 `packet`。

例如：

```text
SSID
 ↓
. 1 . F r e e ...
 ↓
packet[38]
packet[39]
packet[40]
...
```

---

# 13. SSID 後面的資訊

程式建立：

```cpp
uint8_t postSSID[] = {
  0x01, 0x08,
  0x82, 0x84, 0x8b, 0x96,
  0x24, 0x30, 0x48, 0x6c,
  0x03, 0x01, 0x01
};
```

這些資料屬於 Beacon Frame 後面的 Information Elements（IE）。

簡單理解：

```text
Beacon Frame
│
├── Header
│
├── MAC Address
│
├── SSID
│
└── 其他 Wi-Fi 資訊
```

這些資訊讓接收端能夠依照 802.11 的格式解析封包。

---

# 14. 計算封包大小

```cpp
int packetSize = 38 + ssid_len + 13;
```

這裡把封包的不同部分加起來：

```text
38
+
SSID 長度
+
13
=
完整封包長度
```

例如 SSID 長度是 `14`：

```text
38 + 14 + 13
= 65 bytes
```

因此這次封包大小就是：

```text
65 bytes
```

---

# 15. 傳送封包

```cpp
wifi_send_pkt_freedom(
  packet,
  packetSize,
  0
);
```

這是整個程式最重要的低階 API。

它會將：

```text
packet
```

中的資料交給 ESP8266 的 Wi-Fi 硬體進行傳送。

和一般使用：

```cpp
WiFi.begin()
```

連接 Wi-Fi 不同，這裡是在較低階的層級處理 802.11 封包。

---

# 16. 程式整體流程

```text
                 ESP8266
                    │
                    ▼
              關閉一般 Wi-Fi
                    │
                    ▼
             設定低階 Wi-Fi
                    │
                    ▼
              i = 1 開始
                    │
                    ▼
             建立 SSID 名稱
                    │
                    ▼
             建立 MAC 資訊
                    │
                    ▼
            修改 Beacon Frame
                    │
                    ▼
              傳送封包
                    │
                    ▼
               delay(5)
                    │
                    ▼
                i++
                    │
                    ▼
             是否完成全部？
              /          \
            否             是
            │              │
            └──────────────┘
                    │
                    ▼
                  loop()
```

---

# 17. 程式中幾個重要觀念

## `uint8_t`

```cpp
uint8_t
```

代表 8-bit 無號整數。

範圍：

```text
0 ~ 255
```

常用於處理：

* Byte
* MAC Address
* 網路封包
* 二進位資料

---

## `char[]`

```cpp
char ssid_buffer[32];
```

可以用來儲存字串。

例如：

```text
.1.Free-WiFi01
```

---

## `strlen()`

```cpp
strlen(ssid);
```

取得 C-style string 的長度。

---

## `sprintf()`

```cpp
sprintf(ssid_buffer, "%s%02d", prefix, i);
```

可以按照指定格式組合字串。

---

## `delay()`

```cpp
delay(5);
```

讓 ESP8266 暫停約 5 ms。

---

# 18. 實驗重點

這份程式主要可以用來學習：

* ESP8266 Wi-Fi 架構
* IEEE 802.11 Beacon Frame
* MAC Address
* SSID
* Information Element（IE）
* Byte Array
* `uint8_t`
* C/C++ 字串處理
* ESP8266 Low-Level Wi-Fi API
* 無線封包的基本結構

其中最重要的概念是：

> **Wi-Fi 網路名稱（SSID）其實是被放在 802.11 管理封包中的資料，而手機或電腦透過接收到 Beacon Frame，就能知道附近有哪些 Wi-Fi 網路。**

---

# 19. 注意事項

本實驗會直接操作 Wi-Fi 的低階封包，因此不應在公共場所或未經授權的環境長時間執行。

建議使用：

* 自己的 ESP8266
* 自己的測試手機
* 自己的測試網路
* 實驗室環境
* 明確獲得授權的測試環境

實驗的目的應以理解 **802.11 封包結構、ESP8266 Wi-Fi API 與無線網路運作原理** 為主。

---
