# OTA Troubleshooting FreeRTOS

This is a simple  **broken** example of an OTA update that is triggered from discovering a new version is available by subscribing to `ota_troubleshooting/version` on an MQTT broker (expects a number, and any number != the last successfully run firmware update will trigger an update).

**What's broken?** If `xSemaphoreTake()` is used, the actual file download of `httpUpdate.update()` slows down, acts erratically, and very often doesn't complete, allowing enough time for the watchdog to kick in and reboot the device. It looks something like this:

```
...
[ 65921][D][HTTPUpdate.cpp:252] handleUpdate():  - current version: 1

[ 66023][D][HTTPUpdate.cpp:306] handleUpdate(): runUpdate flash...

[ 66023][D][Updater.cpp:133] begin(): OTA Partition: app1
OTA: HTTP update process at 0 of 615840 bytes...
OTA: HTTP update process at 0 of 615840 bytes...
OTA: HTTP update process at 4096 of 615840 bytes...
OTA: HTTP update process at 8192 of 615840 bytes...
OTA: HTTP update process at 12288 of 615840 bytes...
OTA: HTTP update process at 16384 of 615840 bytes...
OTA: HTTP update process at 20480 of 615840 bytes...
OTA: HTTP update process at 24576 of 615840 bytes...
OTA: HTTP update process at 28672 of 615840 bytes...
OTA: HTTP update process at 32768 of 615840 bytes...
OTA: HTTP update process at 36864 of 615840 bytes...
OTA: HTTP update process at 40960 of 615840 bytes...
OTA: HTTP update process at 45056 of 615840 bytes...
OTA: HTTP update process at 49152 of 615840 bytes...
OTA: HTTP update process at 53248 of 615840 bytes...
OTA: HTTP update process at 57344 of 615840 bytes...
OTA: HTTP update process at 61440 of 615840 bytes...
OTA: HTTP update process at 65536 of 615840 bytes...

abort() was called at PC 0x400908e3 on core 0
=> 0x400908e3: task_wdt_isr at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_system/task_wdt.c:176
      (inlined by) task_wdt_isr at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_system/task_wdt.c:136


Backtrace: 0x40026792:0x3ffc27b0 0x4002c12d:0x3ffc27d0 0x4003079d:0x3ffc27f0 0x400908e3:0x3ffc2870 0x40027c55:0x3ffc2890 0x4009a931:0x3ffe82a0 0x400838e9:0x3ffe82c0 0x400839fb:0x3ffe82f0 0x40083af6:0x3ffe8320 0x40083ba0:0x3ffe8340 0x400eb882:0x3ffe8380 0x400888e9:0x3ffe83b0 0x40088911:0x3ffe83d0 0x400eb7a1:0x3ffe83f0 0x400e2bd2:0x3ffe8410 0x40087182:0x3ffe8430 0x40087938:0x3ffe84a0 0x40087aeb:0x3ffe8540 0x400821ca:0x3ffe8640 0x400822c4:0x3ffe86a0
=> 0x40026792: panic_abort at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_system/panic.c:408
=> 0x4002c12d: esp_system_abort at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_system/esp_system.c:137
=> 0x4003079d: abort at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/newlib/abort.c:46
=> 0x400908e3: task_wdt_isr at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_system/task_wdt.c:176
      (inlined by) task_wdt_isr at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_system/task_wdt.c:136
=> 0x40027c55: _xt_lowint1 at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/freertos/port/xtensa/xtensa_vectors.S:1114
=> 0x4009a931: lwip_ioctl at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/lwip/lwip/src/api/sockets.c:3936
=> 0x400838e9: WiFiClientRxBuffer::r_available() at /Users/ken.martin/.platformio/packages/framework-arduinoespressif32/libraries/WiFi/src/WiFiClient.cpp:56
=> 0x400839fb: WiFiClientRxBuffer::fillBuffer() at /Users/ken.martin/.platformio/packages/framework-arduinoespressif32/libraries/WiFi/src/WiFiClient.cpp:81 (discriminator 4)
=> 0x40083af6: WiFiClientRxBuffer::read(unsigned char*, unsigned int) at /Users/ken.martin/.platformio/packages/framework-arduinoespressif32/libraries/WiFi/src/WiFiClient.cpp:117 (discriminator 3)
=> 0x40083ba0: WiFiClient::read(unsigned char*, unsigned int) at /Users/ken.martin/.platformio/packages/framework-arduinoespressif32/libraries/WiFi/src/WiFiClient.cpp:493
=> 0x400eb882: WiFiClient::read() at /Users/ken.martin/.platformio/packages/framework-arduinoespressif32/libraries/WiFi/src/WiFiClient.cpp:400
=> 0x400888e9: Stream::timedRead() at /Users/ken.martin/.platformio/packages/framework-arduinoespressif32/cores/esp32/Stream.cpp:36
=> 0x40088911: Stream::readBytes(char*, unsigned int) at /Users/ken.martin/.platformio/packages/framework-arduinoespressif32/cores/esp32/Stream.cpp:285
=> 0x400eb7a1: Stream::readBytes(unsigned char*, unsigned int) at /Users/ken.martin/.platformio/packages/framework-arduinoespressif32/cores/esp32/Stream.h:103
=> 0x400e2bd2: UpdateClass::writeStream(Stream&) at /Users/ken.martin/.platformio/packages/framework-arduinoespressif32/libraries/Update/src/Updater.cpp:371
=> 0x40087182: HTTPUpdate::runUpdate(Stream&, unsigned int, String, int) at /Users/ken.martin/.platformio/packages/framework-arduinoespressif32/libraries/HTTPUpdate/src/HTTPUpdate.cpp:426
=> 0x40087938: HTTPUpdate::handleUpdate(HTTPClient&, String const&, bool) at /Users/ken.martin/.platformio/packages/framework-arduinoespressif32/libraries/HTTPUpdate/src/HTTPUpdate.cpp:341
=> 0x40087aeb: HTTPUpdate::update(WiFiClient&, String const&, String const&) at /Users/ken.martin/.platformio/packages/framework-arduinoespressif32/libraries/HTTPUpdate/src/HTTPUpdate.cpp:58
=> 0x400821ca: Comms::ota_perform_update() at src/Comms.cpp:139
=> 0x400822c4: task__handleComms(void*) at src/main.cpp:47




ELF file SHA256: 0000000000000000

Rebooting...
ESP-ROM:esp32s2-rc4-20191025
Build:Oct 25 2019
rst:0x3 (RTC_SW_SYS_RST),boot:0xa (SPI_FAST_FLASH_BOOT)
Saved PC:0x400261e5
=> 0x400261e5: esp_restart_noos_dig at /home/runner/work/esp32-arduino-lib-builder/esp32-arduino-lib-builder/esp-idf/components/esp_system/esp_system.c:53 (discriminator 1)
...
```

I have these two vars in place:

```c
#define MUTEX_PERMIT_TO_EXIT 1
#define MUTEX_USE 1 // enabling this will make OTA updates fail most of the time
```

Comment out `#define MUTEX_USE 1` and the update works perfectly and reliably. Uncommented and the failures occur.