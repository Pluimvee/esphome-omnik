// Copyright (c) 2026 Pluimvee (Erik Veer)
// Omnik Inverter WiFi Bridge - ESPHome External Component
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <ESP8266WiFi.h>
#include <lwip/def.h>

namespace esphome {
namespace omnik_bridge {

///////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////
class OmnikBridge : public Component {
 public:
  void set_ap_ssid(const std::string &ssid) { ap_ssid_ = ssid; }
  void set_ap_password(const std::string &pass) { ap_pass_ = pass; }
  void set_ap_ip(const std::string &ip) { ap_ip_.fromString(ip.c_str()); ap_gateway_ = ap_ip_; }
  void set_ap_subnet(const std::string &subnet) { ap_subnet_.fromString(subnet.c_str()); }
  void set_port(uint16_t port) { port_ = port; }
  void set_channel(int8_t channel) { ap_channel_ = channel; }

  void set_temperature_sensor(sensor::Sensor *s) { temperature_ = s; }
  void set_power_sensor(sensor::Sensor *s) { power_ = s; }
  void set_ac_frequency_sensor(sensor::Sensor *s) { ac_frequency_ = s; }
  void set_ac_voltage_sensor(sensor::Sensor *s) { ac_voltage_ = s; }
  void set_energy_today_sensor(sensor::Sensor *s) { energy_today_ = s; }
  void set_energy_total_sensor(sensor::Sensor *s) { energy_total_ = s; }
  void set_operating_hours_sensor(sensor::Sensor *s) { operating_hours_ = s; }
  void set_voltage_pv1_sensor(sensor::Sensor *s) { voltage_pv1_ = s; }
  void set_current_pv1_sensor(sensor::Sensor *s) { current_pv1_ = s; }
  void set_current_ac1_sensor(sensor::Sensor *s) { current_ac1_ = s; }

  void set_connected_sensor(binary_sensor::BinarySensor *s) { connected_ = s; }

  void set_logger_id_sensor(text_sensor::TextSensor *s) { logger_id_ = s; }
  void set_inverter_id_sensor(text_sensor::TextSensor *s) { inverter_id_ = s; }

  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void setup() override;
  void loop() override;
  void dump_config() override;

///////////////////////////////////////////////////////////////////////////////////////
protected:
  struct OmnikHeader {
    uint8_t start;
    uint8_t payload_lg;
    uint8_t msgtype[2];
    int32_t logger_id_1;
    int32_t logger_id_2;
  } __attribute__((packed));

  struct OmnikPayload {
    uint8_t dummy[3];
    char inverter_id[16];
    uint16_t temperature;
    uint16_t voltage_pv1;
    uint16_t voltage_pv2;
    uint16_t voltage_pv3;
    uint16_t current_pv1;
    uint16_t current_pv2;
    uint16_t current_pv3;
    uint16_t current_ac1;
    uint16_t current_ac2;
    uint16_t current_ac3;
    uint16_t voltage_ac1;
    uint16_t voltage_ac2;
    uint16_t voltage_ac3;
    uint16_t frequency_ac1;
    uint16_t power_ac1;
    uint16_t frequency_ac2;
    uint16_t power_ac2;
    uint16_t frequency_ac3;
    uint16_t power_ac3;
    uint16_t energy_today;
    uint32_t energy_total;
    uint32_t operating_hours;
  } __attribute__((packed));

  void start_ap_();
  void stop_ap_();
  void handle_client_();
  bool handle_message_(const uint8_t *msg, size_t lg);
  void mark_unavailable_();

  std::string ap_ssid_;
  std::string ap_pass_;
  IPAddress ap_ip_{0, 0, 0, 0};
  IPAddress ap_gateway_{0, 0, 0, 0};
  IPAddress ap_subnet_{255, 255, 255, 0};
  uint16_t port_{10004};
  int8_t ap_channel_{0};
  bool ap_started_{false};
  bool sensors_available_{false};
  uint32_t last_client_time_{0};
  uint32_t last_message_time_{0};
  static constexpr uint32_t CONNECTED_TIMEOUT_MS = 6 * 60 * 1000UL;
  static constexpr uint32_t AVAILABILITY_TIMEOUT_MS = 30 * 60 * 1000UL;
  WiFiServer *server_{nullptr};

  sensor::Sensor *temperature_{nullptr};
  sensor::Sensor *power_{nullptr};
  sensor::Sensor *ac_frequency_{nullptr};
  sensor::Sensor *ac_voltage_{nullptr};
  sensor::Sensor *energy_today_{nullptr};
  sensor::Sensor *energy_total_{nullptr};
  sensor::Sensor *operating_hours_{nullptr};
  sensor::Sensor *voltage_pv1_{nullptr};
  sensor::Sensor *current_pv1_{nullptr};
  sensor::Sensor *current_ac1_{nullptr};

  binary_sensor::BinarySensor *connected_{nullptr};

  text_sensor::TextSensor *logger_id_{nullptr};
  text_sensor::TextSensor *inverter_id_{nullptr};
};

}  // namespace omnik_bridge
}  // namespace esphome
