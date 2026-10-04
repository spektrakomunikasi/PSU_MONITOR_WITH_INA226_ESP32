
# PSU_MONITOR_WITH_INA226_ESP32

PSU monitor berbasis **ESP32** dengan tampilan **TFT ST7789 240x240**, sensor arus/tegangan **INA226**, dan sensor suhu heatsink **DS18B20**.

## Fitur

- Tampilan utama:
  - `E = xx.xx V`
  - `I = xx.xx A`
- Bar indikator:
  - VOLT `0–25V`
  - AMPER `0–20A`
- Monitoring suhu:
  - `SUHU: xx.x C`
- Kontrol fan otomatis:
  - Fan **ON** jika suhu melewati setpoint (default 40°C)
  - Fan **OFF** dengan hysteresis (default 38°C)
- Setpoint fan bisa diubah lewat **Serial Monitor** (tanpa edit ulang sketch)
- Simpan setting ke EEPROM

---

## Hardware yang digunakan

- ESP32 Dev Board
- INA226 (I2C)
- TFT ST7789 240x240 (SPI)
- DS18B20 + resistor pull-up 4.7k
- Fan DC + driver (MOSFET/transistor/relay module)

---

## Wiring

## 1) INA226 (I2C)

- `VCC` -> `3V3`
- `GND` -> `GND`
- `SDA` -> `GPIO21`
- `SCL` -> `GPIO22`

> Alamat INA226 pada project ini: `0x44`

## 2) TFT ST7789 240x240 (SPI)

- `VCC` -> `3V3`
- `GND` -> `GND`
- `SCL/SCK` -> `GPIO18`
- `SDA/MOSI` -> `GPIO23`
- `RES/RST` -> `GPIO4`
- `DC` -> `GPIO2`
- `CS` -> `-1` (sesuai setting TFT_eSPI jika modul tanpa CS terpisah)
- `BLK` -> `3V3`

## 3) DS18B20

- `VDD` -> `3V3`
- `GND` -> `GND`
- `DQ` -> `GPIO27`
- Resistor **4.7k** antara `DQ` dan `3V3`

## 4) Fan Control

- `FAN_PIN` (default `GPIO26`) -> input driver fan (bukan langsung ke fan besar)

---

## Konfigurasi TFT_eSPI

Edit file `User_Setup.h` pada library TFT_eSPI sesuai pin board kamu.
Contoh konfigurasi yang dipakai:

```cpp
#define ST7789_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 240
#define CGRAM_OFFSET

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_MISO -1
#define TFT_CS   -1
#define TFT_DC   2
#define TFT_RST  4

#define SPI_FREQUENCY  20000000
```

> Jika orientasi belum pas, coba `tft.setRotation(1)` atau `tft.setRotation(3)`.

---

## Library yang dibutuhkan

Install via Arduino Library Manager:

- `TFT_eSPI`
- `INA226_WE`
- `OneWire`
- `DallasTemperature`
- `EEPROM` (sudah bawaan ESP32 core)

---

## Perintah Serial Monitor

Set baudrate: **115200**, newline: **Newline**

- `HELP`  
  Menampilkan daftar perintah
- `SHOW`  
  Menampilkan setting saat ini
- `SET 45`  
  Set suhu fan ON ke 45°C
- `HYS 3`  
  Set hysteresis 3°C (fan OFF di 42°C jika ON=45)
- `SAVE`  
  Simpan setting ke EEPROM

---

## Catatan troubleshooting

- Jika suhu tampil `--.- C` atau `-127.0 C`:
  - cek wiring DS18B20
  - pastikan resistor pull-up 4.7k terpasang
- Jika muncul `INA226 NOT FOUND`:
  - cek wiring I2C
  - scan I2C address (project ini pakai `0x44`)
- Jika layar blank/geser:
  - cek `User_Setup.h` TFT_eSPI
  - coba ubah rotasi display

---

## License

MIT License
