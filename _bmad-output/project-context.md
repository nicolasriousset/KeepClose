---
project_name: 'KeepClose'
user_name: 'Nicolas'
date: '2026-04-27'
sections_completed: ['technology_stack', 'domain_context', 'implementation_rules', 'dont_miss_rules']
status: 'complete'
rule_count: 28
optimized_for_llm: true
---

# Project Context for AI Agents

_This file contains critical rules and patterns that AI agents must follow when implementing code in this project. Focus on unobvious details that agents might otherwise miss._

---

## Technology Stack & Versions

- **Hardware**: LilyGo T-Watch 2020 V3 (ESP32-based smartwatch)
- **IDE**: Arduino IDE
- **ESP32 Board Package**: Espressif Systems v2.0.17 — LOCKED, do not upgrade (v3.0+ breaks compatibility)
- **TWatch Library**: TTGO TWatch Library (`LilyGoWatch.h`) — master branch
- **BLE Library**: NimBLE (NimBLEDevice, NimBLEAdvertisedDevice, NimBLEBeacon, NimBLEEddystoneTLM)
- **UI Library**: LVGL v7.x (old API)
- **Power Management IC**: AXP202 (via `ttgo->power`)
- **Motor**: accessed via `ttgo->motor`
- **Language**: C++11 / Arduino framework
- **Board config**: `#define LILYGO_WATCH_2020_V3` must be active in `config.h`

## Project Domain & Architecture Vision

KeepClose is a proximity alert system for visually impaired users. It is converging with the **Petit Poucet** project (same hardware need, different software logic):
- **KeepClose**: alerts when user moves away from a tracked beacon
- **Petit Poucet**: guides user toward beacons (multiple beacons, collect nearest first)

**Shared constraints that shape all implementation decisions:**
- System must work WITHOUT internet, Wi-Fi, or third-party networks
- GPS is ineffective indoors — BLE RSSI distance estimation is the primary indoor technique
- 10cm precision is military-grade and unreachable in civilian BLE
- Target users are visually impaired: smartphone cannot be the primary interface; feedback must be tactile (vibration) and/or audio
- Avoid repetitive beeps — prefer spatialized or adaptive sound
- Vibration feedback is preferred: non-anxiety-inducing per user testing (Précanne workshop, Jan 2025)

## Critical Implementation Rules

### BLE / NimBLE Rules

- Use **NimBLE exclusively** — do NOT mix with Arduino standard BLE library
- `BLEDevice::getScan()` is used (not `NimBLEDevice::getScan()`) in existing code — stay consistent
- Beacon filter: device name must start with `"Holy"` (case-insensitive via `startsWithIgnoreCase()`)
- iBeacon detection: manufacturer data must be exactly **25 bytes**, bytes 0–1 = `0x4C 0x00`
- Distance model: `pow(10.0, (txPower - rssi) / (10.0 * n))` with `n=2.0`
- Distance smoothed via **MovingAverage** (buffer size 3) — always use the averager, never raw RSSI
- Scan is non-blocking: `SCAN_INTERVAL=2000ms`, `SCAN_DURATION=1000ms` via `millis()`
- Update `lastDetectionTime` on every beacon detection — it signals beacon presence/loss

### LVGL v7 API Rules

- **LVGL v7 old API only** — not v8/v9
- `lv_label_create(parent, NULL)` — second arg is always `NULL`
- Style: `lv_obj_set_style_local_text_font(obj, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &font)`
- Alignment: `lv_obj_align(obj, NULL, LV_ALIGN_CENTER, 0, 0)` — `NULL` for screen-relative alignment
- `lv_task_handler()` must be called **every loop iteration** — required for LVGL event processing
- UI labels are in French: "Recherche...", "Attention !", "!!! ALERTE !!!"

### T-Watch / AXP202 Hardware Rules

- Screen on: `ttgo->openBL()` + `ttgo->displayWakeup()`
- Screen off: `ttgo->closeBL()` + `ttgo->displaySleep()`
- Vibration: `ttgo->motor->onec()` (single pulse — no duration control)
- Button: interrupt on `AXP202_INT`, `FALLING` edge → sets `irq = true` flag (processed in `loop()`)
- IRQ sequence: `readIRQ()` → `isPEKShortPressIRQ()` → `clearIRQ()`
- Init order in `setup()`: `ttgo->begin()` → `motor_begin()` → `lvgl_begin()` → enable IRQ → `clearIRQ()`

### State Machine Rules

- States: `WAITING → GUARDING → WARNING → ALARM`
- `WAITING`: beacon not acquired, no alert
- `GUARDING`: beacon < `WARNING_DISTANCE` (10m), no alert
- `WARNING`: beacon between 10m–20m, vibrate every 1s, screen on
- `ALARM`: beacon > `ALARM_DISTANCE` (20m), vibrate every 1s, screen on
- Button in alert state → `resetState()`: clears state, resets `MovingAverage`, returns to `WAITING`
- All state transitions must call `updateLabels()` and log to Serial

### Timing & Non-Blocking Design

- All timing via `millis()` — **never** `delay()` in main logic
- `delay(100)` at end of `loop()` is acceptable — sets effective ~10Hz poll rate
- `SCREEN_TIMEOUT=10000ms` — screen off when idle and not in alert state
- `VIBRATION_INTERVAL=1000ms` — minimum gap between vibration pulses

## Critical Don't-Miss Rules

- **NEVER upgrade** ESP32 board package beyond v2.0.17 — v3.0+ breaks TWatch library
- **NEVER remove** `LILYGO_WATCH_2020_V3` from `config.h` — required for hardware abstraction layer
- **NEVER call** `lv_task_handler()` conditionally — must run every loop
- **Do NOT use GPS** for indoor positioning — BLE RSSI is the correct indoor approach
- **Do NOT design smartphone-first UX** — target users are visually impaired; wearable is primary
- **Do NOT add blocking code** inside `NimBLEScanCallbacks::onResult()` or state transitions
- **Multi-beacon future**: tracking `lastDetectionTime` per beacon UUID (not globally) will be required
- **Beacon name filter `"Holy"`** is hardware-specific — make it configurable before Petit Poucet merge

---

## Usage Guidelines

**For AI Agents:**
- Read this file before implementing any code
- Follow ALL rules exactly as documented
- When in doubt, prefer the more restrictive option
- Update this file if new patterns emerge

**For Humans:**
- Keep this file lean and focused on agent needs
- Update when technology stack or hardware changes
- Review when starting a new feature or merging with Petit Poucet
- Remove rules that become obvious over time

_Last Updated: 2026-04-27_
