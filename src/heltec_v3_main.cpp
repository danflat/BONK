#if defined(ARDUINO) && defined(BONK_HELTEC_V3)

#include "bonk/airwhere_bridge.hpp"
#include "bonk/ble_tx_mux.hpp"
#include "bonk/features.hpp"
#include "bonk/radio_arbiter.hpp"
#include "bonk/splash.hpp"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Wire.h>
#include <esp_sleep.h>
#include <esp_timer.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace {

using Pins = bonk::HeltecV3Pins;

constexpr std::size_t kSerialLineCapacity = bonk::BleFrame::kMaxPayload;
constexpr char kBleDeviceName[] = "BONK";
constexpr std::uint32_t kLoopDelayMs = 20;
constexpr std::uint32_t kFlarmActivityBudgetUs = 25'000;

HardwareSerial gps_serial{1};
HardwareSerial flarm_serial{2};
Adafruit_SSD1306 display{128, 64, &Wire, -1};
bonk::BleTxMux ble_mux{};
bonk::RadioArbiter activity_arbiter{};

NimBLEServer *ble_server = nullptr;
NimBLECharacteristic *flarm_characteristic = nullptr;
std::atomic<std::uint16_t> connection_count{0};

bool display_ready = false;
std::uint16_t active_targets = 0;
std::uint8_t battery_percent = 100;
std::uint32_t last_ui_update_ms = 0;
std::uint32_t button_hold_start_ms = 0;
bool button_is_held = false;

class SerialLineReader {
  public:
    template <typename Callback>
    void poll(HardwareSerial &serial, Callback callback) noexcept {
        while (serial.available() > 0) {
            const int next = serial.read();
            if (next < 0) {
                return;
            }
            const auto byte = static_cast<std::uint8_t>(next);

            if (dropping_) {
                if (byte == '\n') {
                    dropping_ = false;
                    size_ = 0;
                }
                continue;
            }

            if (size_ >= buffer_.size()) {
                dropping_ = true;
                size_ = 0;
                ++dropped_lines_;
                continue;
            }

            buffer_[size_++] = byte;
            if (byte == '\n') {
                callback(buffer_.data(), size_);
                size_ = 0;
            }
        }
    }

    [[nodiscard]] std::uint32_t droppedLines() const noexcept {
        return dropped_lines_;
    }

  private:
    std::array<std::uint8_t, kSerialLineCapacity> buffer_{};
    std::size_t size_ = 0;
    std::uint32_t dropped_lines_ = 0;
    bool dropping_ = false;
};

SerialLineReader gps_lines{};
SerialLineReader flarm_lines{};

class ServerCallbacks final : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer *, NimBLEConnInfo &) override {
        connection_count.fetch_add(1, std::memory_order_relaxed);
        digitalWrite(Pins::led, HIGH);
    }

    void onDisconnect(NimBLEServer *, NimBLEConnInfo &, int) override {
        const auto current = connection_count.load(std::memory_order_relaxed);
        if (current > 0) {
            connection_count.fetch_sub(1, std::memory_order_relaxed);
        }
        if (connection_count.load(std::memory_order_relaxed) == 0) {
            digitalWrite(Pins::led, LOW);
        }
        NimBLEDevice::startAdvertising();
    }
};

ServerCallbacks server_callbacks{};

[[nodiscard]] bool bleConnected() noexcept {
    return connection_count.load(std::memory_order_relaxed) > 0;
}

void drawSplash(bonk::BootStage stage) noexcept {
    if (!display_ready) {
        return;
    }
    const auto frame = bonk::makeSplashFrame(stage);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println(frame.product.data());
    display.setTextSize(1);
    display.setCursor(0, 22);
    display.println(frame.tagline.data());
    display.setCursor(0, 40);
    display.println(frame.status.data());
    display.drawRect(0, 54, 128, 9, SSD1306_WHITE);
    const auto width = static_cast<std::int16_t>(
        (static_cast<std::uint16_t>(frame.progress_percent) * 124U) / 100U);
    display.fillRect(2, 56, width, 5, SSD1306_WHITE);
    display.display();
}

void drawRuntimeScreen() noexcept {
    if (!display_ready) {
        return;
    }

    char battery_bar[11] = "[        ]";
    const std::uint8_t filled = static_cast<std::uint8_t>(
        (static_cast<std::uint16_t>(battery_percent) * 8U) / 100U);
    for (std::uint8_t index = 0; index < filled && index < 8U; ++index) {
        battery_bar[index + 1U] = '|';
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("BONK");
    display.setCursor(0, 10);
    display.printf("%s %u%%\n", battery_bar, battery_percent);
    display.println("----------------");
    display.setCursor(0, 32);
    display.println(bleConnected() ? "STATUS: PAIRED" : "STATUS: SEARCHING");
    display.println("Name: BONK");
    display.printf("FLARM count: %u", active_targets);
    display.display();
}

void initializeDisplay() noexcept {
    pinMode(Pins::vext, OUTPUT);
    digitalWrite(Pins::vext, LOW);
    pinMode(Pins::oled_reset, OUTPUT);
    digitalWrite(Pins::oled_reset, HIGH);
    delay(150);

    Wire.begin(Pins::oled_sda, Pins::oled_scl, 400'000U);
    display_ready = display.begin(SSD1306_SWITCHCAPVCC, bonk::kOledAddress, false, false);
    if (!display_ready) {
        Serial.println("OLED init failed, bypassing display");
        return;
    }
    drawSplash(bonk::BootStage::Starting);
}

void initializeBattery() noexcept {
    pinMode(Pins::adc_control, OUTPUT);
    // Preserve the known-working MicroPython polarity. Meshtastic's stock
    // Heltec V3 variant uses the opposite level, so BONK owns this explicitly.
    digitalWrite(Pins::adc_control, HIGH);
    analogReadResolution(12);
    analogSetPinAttenuation(Pins::battery_adc, ADC_11db);
}

void initializeBle() noexcept {
    NimBLEDevice::init(kBleDeviceName);
    NimBLEDevice::setMTU(247);
    ble_server = NimBLEDevice::createServer();
    ble_server->setCallbacks(&server_callbacks);

    auto *service = ble_server->createService("FFE0");
    flarm_characteristic = service->createCharacteristic(
        "FFE1",
        NIMBLE_PROPERTY::NOTIFY |
            NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::WRITE_NR,
        bonk::BleFrame::kMaxPayload);
    service->start();

    auto *advertising = NimBLEDevice::getAdvertising();
    advertising->setName(kBleDeviceName);
    advertising->addServiceUUID(service->getUUID());
    constexpr std::uint16_t interval_units = static_cast<std::uint16_t>(
        bonk::kBleAdvertisingIntervalMs * 1000U / 625U);
    advertising->setMinInterval(interval_units);
    advertising->setMaxInterval(interval_units);
    advertising->enableScanResponse(true);
    advertising->start();
}

void queueTelemetry(const std::uint8_t *data, std::size_t size) noexcept {
    if (!ble_mux.enqueue(bonk::BleChannel::FlarmTelemetry, data, size)) {
        Serial.println("BLE telemetry frame rejected");
    }
}

void drainBleTelemetry() noexcept {
    if (!bleConnected() || flarm_characteristic == nullptr) {
        return;
    }

    bonk::BleFrame frame{};
    for (std::uint8_t sent = 0; sent < 4U && ble_mux.pop(frame); ++sent) {
        if (!flarm_characteristic->notify(frame.payload.data(), frame.size)) {
            Serial.println("BLE notification failed");
            return;
        }
    }
}

void serviceFlarm() noexcept {
    flarm_lines.poll(flarm_serial, [](const std::uint8_t *data, std::size_t size) {
        const auto now_us = static_cast<bonk::Micros>(esp_timer_get_time());
        const auto foreground = activity_arbiter.requestFlarm(
            now_us,
            bonk::RadioOperation::Receive,
            kFlarmActivityBudgetUs);

        std::uint16_t count = 0;
        if (bonk::parsePflauTargetCount(data, size, count)) {
            active_targets = count;
        }
        queueTelemetry(data, size);

        if (foreground.granted()) {
            static_cast<void>(activity_arbiter.release(
                foreground.lease.token,
                static_cast<bonk::Micros>(esp_timer_get_time())));
        }
    });
}

void serviceGps() noexcept {
    gps_lines.poll(gps_serial, [](const std::uint8_t *data, std::size_t size) {
        queueTelemetry(data, size);
    });
}

void updateBatteryAndScreen(std::uint32_t now_ms) noexcept {
    if (static_cast<std::uint32_t>(now_ms - last_ui_update_ms) <
        bonk::kUiRefreshIntervalMs) {
        return;
    }
    battery_percent = bonk::batteryPercentFromAdc(
        static_cast<std::uint32_t>(analogRead(Pins::battery_adc)),
        4095U);
    drawRuntimeScreen();
    last_ui_update_ms = now_ms;
}

[[noreturn]] void powerDownDevice() noexcept {
    Serial.println("Executing system shutdown sequence");
    if (display_ready) {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor(10, 24);
        display.println("SHUTTING DOWN...");
        display.display();
        delay(1'000);
        display.ssd1306_command(SSD1306_DISPLAYOFF);
    }

    digitalWrite(Pins::led, LOW);
    digitalWrite(Pins::adc_control, LOW);
    digitalWrite(Pins::vext, HIGH);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    esp_deep_sleep_start();
    for (;;) {
    }
}

void servicePowerButton(std::uint32_t now_ms) noexcept {
    if (digitalRead(Pins::program_button) == LOW) {
        if (!button_is_held) {
            button_is_held = true;
            button_hold_start_ms = now_ms;
        } else if (static_cast<std::uint32_t>(now_ms - button_hold_start_ms) >
                   bonk::kShutdownHoldMs) {
            powerDownDevice();
        }
    } else {
        button_is_held = false;
    }
}

} // namespace

void setup() {
    Serial.begin(115'200);
    delay(50);

    pinMode(Pins::led, OUTPUT);
    digitalWrite(Pins::led, LOW);
    pinMode(Pins::program_button, INPUT_PULLUP);

    initializeDisplay();
    initializeBattery();
    drawSplash(bonk::BootStage::RadioSelfTest);

    gps_serial.begin(
        bonk::kGpsBaud,
        SERIAL_8N1,
        Pins::gps_rx,
        Pins::gps_tx);
    flarm_serial.begin(
        bonk::kFlarmBaud,
        SERIAL_8N1,
        Pins::flarm_rx,
        Pins::flarm_tx);
    drawSplash(bonk::BootStage::FlarmReady);

    initializeBle();
    drawSplash(bonk::BootStage::BleReady);

    // The stock Meshtastic driver is not linked yet. Keeping mesh disabled
    // prevents any caller from bypassing the integration gate by accident.
    activity_arbiter.setMeshEnabled(false);
    drawSplash(bonk::BootStage::MeshStandby);
    delay(250);
    drawSplash(bonk::BootStage::Ready);
    delay(500);

    Serial.println("BONK Arduino flight bridge operational");
    drawRuntimeScreen();
    last_ui_update_ms = millis();
}

void loop() {
    delay(kLoopDelayMs);
    const std::uint32_t now_ms = millis();

    servicePowerButton(now_ms);
    serviceFlarm();
    serviceGps();
    drainBleTelemetry();
    updateBatteryAndScreen(now_ms);
    static_cast<void>(activity_arbiter.tick(
        static_cast<bonk::Micros>(esp_timer_get_time())));
}

#endif
