# Changelog

Changes to the ESP32PLC firmware and web UI. Newest first. Entries list the commit on the `Dev` branch.
Library changes in [ESP32ModbusMaster](https://github.com/richcj10/ESP32ModbusMaster) are noted where they affect the PLC.

## 2026-10-14 — Control task, MQTT stability

**Critical traffic no longer waits on WiFi or MQTT**
- Inputs, Modbus polling/writes and the Fireworks module now run in their own `control` task (core 1, priority above `loop()`). MQTT connects, display redraws and WiFi recovery in `loop()` can't delay them. The task starts before WiFi, so Modbus runs while WiFi connects.
- Modules choose where `update()` runs with `realtime()`. Only Fireworks opts in; the LED strip stays in `loop()` because it shares FastLED with the status LED.
- Device online/offline MQTT messages from the control task are queued and published by `loop()` (PubSubClient isn't thread-safe).
- The Log tab's Main loop card also shows the control task's timing.

**MQTT**
- Retries forever: every 5 s for the first 3 failures, then every 30 s. Each attempt is capped by a 2 s socket timeout. It used to try 6 times back to back (15 s timeout each), then lock MQTT out until reboot.
- Reboots after 15 min of continuous failure while WiFi is up, or after 8 drops within 30 min. Never for bad credentials, and never mid firing sequence or firmware job.
- New **Auth failed** badge state; failures are logged with the reason (e.g. `not authorized`).
- Leaving the password blank on the MQTT tab now keeps the stored password. Saving the form used to erase it.
- Client ID is now unique per device (`ESPPLC-` plus the full MAC in hex). The old ID could be shared by two PLCs, which then knocked each other off the broker.
- Removed a reboot-on-publish-errors check that could never fire.

**WiFi**
- No longer reboots ~1 s after any WiFi drop. Waits for automatic reconnect and reboots only after 5 min of continuous loss, never while the recovery AP is up.

**Bootloader version check**
- Keeps the whole bus silent for 8 s per round (up to 3 rounds) so the device's bootloader can time out back into its app. Any traffic restarted that timeout, so the device stayed in its bootloader. The failure message no longer wrongly says the device needs flashing.

**ESP32ModbusMaster**
- A group that has never been polled is due immediately. Devices show *valid* within seconds of boot instead of waiting a full poll interval (about a minute with 30 s version groups).
- The write queue is locked: writes arrive from the web server, MQTT and modules while the control task drains it.

## 2026-09-27 — `1516fb4` Faster builds, manual bootloader-version check
- Version strings live in a generated header used by one small file, so a commit no longer rebuilds all 150 files.
- ccache actually runs: the ESP32 build script was overwriting the compiler prefix.
- SCons include cache: a no-change build takes about 7.5 s instead of 15.8 s.
- **Check** button on the FW Update tab reads a device's bootloader version on demand.

## 2026-09-26 — `0279052` Guided bootloader update, UI fixes
- **Update BL** on the FW Update tab: flashes ModBusBLUpdater, finds the new bootloader (own address or 200), restores the app and the address, reports the version change.
- Log tab: Pause scroll, Copy, Save .txt, loads recent lines on open.
- Device address change verifies the device answers at the new address before saving `Remote.json`.
- FW progress no longer gets stuck; the current tab survives a page reload (`#fw`, `#log`, ...).
- `/fw/list` and `/api/devices/status` cover all 30 devices instead of 8.

## 2026-09-25 — `cab9cc4` Cleanup, OneWire
- Removed ~3,900 lines of dead code; flash −279 KB, internal RAM −55 KB.
- OneWire DS18B20 module with MQTT and Home Assistant discovery, settings in `/OneWire.json`, card on the I/O page.

## 2026-09-25 — `c5d6b0b` Modules, device recovery, diagnostics
- Modules self-register; choose them with `custom_modules` in `platformio.ini`; switch them on or off at runtime on the App tab.
- Boot-button reset: hold 5 s for a network reset, 15 s for a factory reset; both restart into an open AP.
- Runtime log level, main-loop timing, NVS usage, MQTT live status badge.

## Earlier
See `git log` for history before 2026-09-23.
