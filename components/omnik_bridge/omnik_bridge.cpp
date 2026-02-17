// Copyright (c) 2026 Pluimvee (Erik Veer)
// Omnik Inverter WiFi Bridge - ESPHome External Component
#include "omnik_bridge.h"
#include "esphome/core/log.h"

namespace esphome {
namespace omnik_bridge {

static const char *const TAG = "omnik_bridge";

///////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////
void OmnikBridge::setup() {
  ESP_LOGI(TAG, "Setup: SSID='%s' IP=%s port=%u channel=%d",
           this->ap_ssid_.c_str(), this->ap_ip_.toString().c_str(),
           this->port_, this->ap_channel_);
  this->server_ = new WiFiServer(this->port_);

  // We start without any sensor values and wait for a message to arrive
  // if not within connection tieout after the reboot, we will initialize the values with defaults
  this->last_message_time_ = millis() - (AVAILABILITY_TIMEOUT_MS - CONNECTED_TIMEOUT_MS);  
  this->sensors_available_ = true;
}

///////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////
void OmnikBridge::loop() 
{
  const bool wifi_connected = (WiFi.status() == WL_CONNECTED);

  if (!wifi_connected) {
    if (this->ap_started_) {
      this->stop_ap_();
    }
  } else {
    if (!this->ap_started_) {
      this->start_ap_();
    }
    this->handle_client_();
  }

  // Evaluate timeouts after handling client traffic so fresh messages are included.
  const uint32_t now_ms = millis();
  if (this->connected_ != nullptr) 
  {
    const bool conn = (this->last_client_time_ > 0) && ((now_ms - this->last_client_time_) <= CONNECTED_TIMEOUT_MS);
    this->connected_->publish_state(conn);
  }

  if (this->sensors_available_)
  {
    const bool expired = (this->last_message_time_ > 0) && ((now_ms - this->last_message_time_) > AVAILABILITY_TIMEOUT_MS);
    // Mark measurement sensors unavailable after 30 minutes without valid messages, to avoid showing stale data indefinitely.
    if (expired) 
      this->mark_unavailable_();
  }
}

///////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////
void OmnikBridge::dump_config() {
  ESP_LOGCONFIG(TAG, "Omnik Bridge:");
  ESP_LOGCONFIG(TAG, "  AP SSID: %s", this->ap_ssid_.c_str());
  ESP_LOGCONFIG(TAG, "  AP IP: %s", this->ap_ip_.toString().c_str());
  ESP_LOGCONFIG(TAG, "  Port: %u", this->port_);
  ESP_LOGCONFIG(TAG, "  Channel: %d", this->ap_channel_);
}

///////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////
void OmnikBridge::start_ap_() 
{
  ESP_LOGI(TAG, "start_ap_ called, ssid='%s' pass_len=%d", this->ap_ssid_.c_str(), this->ap_pass_.length());
  ESP_LOGI(TAG, "WiFi status=%d mode=%d sta_connected=%s sta_channel=%d",
           static_cast<int>(WiFi.status()), static_cast<int>(WiFi.getMode()),
           (WiFi.status() == WL_CONNECTED) ? "yes" : "no", WiFi.channel());
  if (this->ap_ssid_.empty() || this->ap_pass_.empty()) {
    ESP_LOGE(TAG, "AP credentials not set");
    return;
  }

  int8_t channel = this->ap_channel_;
  if (channel <= 0) {
    channel = WiFi.channel();
  }

  WiFi.mode(WIFI_AP_STA);

  bool ap_cfg_ok = WiFi.softAPConfig(this->ap_ip_, this->ap_gateway_, this->ap_subnet_);
  ESP_LOGI(TAG, "softAPConfig ip=%s gw=%s subnet=%s -> %s",
           this->ap_ip_.toString().c_str(),
           this->ap_gateway_.toString().c_str(),
           this->ap_subnet_.toString().c_str(),
           ap_cfg_ok ? "ok" : "fail");

  WiFi.softAPConfig(this->ap_ip_, this->ap_gateway_, this->ap_subnet_);
  if (!WiFi.softAP(this->ap_ssid_.c_str(), this->ap_pass_.c_str(), channel)) {
    ESP_LOGE(TAG, "Failed to start AP");
    return;
  }

  ESP_LOGI(TAG, "softAP IP=%s channel=%d", WiFi.softAPIP().toString().c_str(), channel);

  this->server_->begin();
  this->ap_started_ = true;
  ESP_LOGI(TAG, "AP started on %s channel %d", WiFi.softAPIP().toString().c_str(), channel);
}

///////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////
void OmnikBridge::stop_ap_() {
  if (this->server_ != nullptr) {
    this->server_->stop();
  }
  WiFi.softAPdisconnect(true);
  this->ap_started_ = false;
}

///////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////
void OmnikBridge::handle_client_() {
  if (this->server_ == nullptr) {
    return;
  }

  WiFiClient client = this->server_->accept();
  if (!client) {
    return;
  }
  this->last_client_time_ = millis();

  uint8_t buffer[300];
  size_t length = 0;
  const uint32_t start_ms = millis();
  // Keep this tight to avoid blocking the main loop and starving the API.
  while (client.connected() && (millis() - start_ms) < 50 && length < sizeof(buffer)) {
    while (client.available() && length < sizeof(buffer)) {
      buffer[length++] = static_cast<uint8_t>(client.read());
    }
    delay(0);
  }
  if (length > 0) {
    ESP_LOGI(TAG, "Client connected from %s", client.remoteIP().toString().c_str());
    const bool parsed_ok = this->handle_message_(buffer, length);
    if (!parsed_ok) {
      ESP_LOGW(TAG, "Frame rejected (len=%u)", static_cast<unsigned>(length));
    }
  }
  client.stop();
}

///////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////
bool OmnikBridge::handle_message_(const uint8_t *msg, size_t lg) {
  if (lg < sizeof(OmnikHeader)) {
    ESP_LOGW(TAG, "Message too short for header");
    return false;
  }
  auto *hdr = reinterpret_cast<const OmnikHeader *>(msg);
  if (hdr->start != 0x68) {
    ESP_LOGW(TAG, "Invalid start byte: %u", hdr->start);
    return false;
  }
  size_t total_expected = static_cast<size_t>(hdr->payload_lg) + sizeof(OmnikHeader) + 2;
  if (lg < total_expected) {
    ESP_LOGW(TAG, "Message length %u shorter than expected %u", lg, total_expected);
    return false;
  }
  if (hdr->logger_id_1 != hdr->logger_id_2) {
    ESP_LOGW(TAG, "Logger ID mismatch");
    return false;
  }
  if (hdr->payload_lg < sizeof(OmnikPayload)) {
    ESP_LOGW(TAG, "Payload too short");
    return false;
  }
  // mark sensors to be valid
  this->last_message_time_ = millis();
  this->sensors_available_ = true;

  // set the sensor values
  if (this->logger_id_ != nullptr) {
    char id_buf[32];
    itoa(hdr->logger_id_1, id_buf, 10);
    this->logger_id_->publish_state(id_buf);
  }
  auto *payl = reinterpret_cast<const OmnikPayload *>(msg + sizeof(OmnikHeader));
  if (this->inverter_id_ != nullptr) {
    char inverter_buf[17];
    memcpy(inverter_buf, payl->inverter_id, sizeof(payl->inverter_id));
    inverter_buf[16] = 0;
    this->inverter_id_->publish_state(inverter_buf);
  }
  if (this->temperature_ != nullptr)
    this->temperature_->publish_state(ntohs(payl->temperature) / 10.0f);
  if (this->power_ != nullptr)
    this->power_->publish_state(static_cast<float>(ntohs(payl->power_ac1)));
  if (this->energy_today_ != nullptr)
    this->energy_today_->publish_state(ntohs(payl->energy_today) / 100.0f);
  if (this->energy_total_ != nullptr)
    this->energy_total_->publish_state(ntohl(payl->energy_total) / 10.0f);
  if (this->operating_hours_ != nullptr)
    this->operating_hours_->publish_state(static_cast<float>(ntohl(payl->operating_hours)));
  if (this->ac_frequency_ != nullptr)
    this->ac_frequency_->publish_state(ntohs(payl->frequency_ac1) / 100.0f);
  if (this->ac_voltage_ != nullptr)
    this->ac_voltage_->publish_state(ntohs(payl->voltage_ac1) / 10.0f);
  if (this->voltage_pv1_ != nullptr)
    this->voltage_pv1_->publish_state(ntohs(payl->voltage_pv1) / 10.0f);
  if (this->current_pv1_ != nullptr)
    this->current_pv1_->publish_state(ntohs(payl->current_pv1) / 10.0f);
  if (this->current_ac1_ != nullptr)
    this->current_ac1_->publish_state(ntohs(payl->current_ac1) / 10.0f);

  ESP_LOGI(TAG, "Message parsed successfully: AC Power %dW", ntohs(payl->power_ac1));
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////
void OmnikBridge::mark_unavailable_() {
  this->sensors_available_ = false;
  // Mark all inverter-derived entities unavailable after timeout.
  if (this->temperature_ != nullptr)
    this->temperature_->publish_state(NAN);
  if (this->power_ != nullptr)
    this->power_->publish_state(0.0f);
  if (this->ac_frequency_ != nullptr)
    this->ac_frequency_->publish_state(0.0f);
  if (this->ac_voltage_ != nullptr)
    this->ac_voltage_->publish_state(0.0f);
  if (this->voltage_pv1_ != nullptr)
    this->voltage_pv1_->publish_state(0.0f);
  if (this->current_pv1_ != nullptr)
    this->current_pv1_->publish_state(0.0f);
  if (this->current_ac1_ != nullptr)
    this->current_ac1_->publish_state(0.0f);

  ESP_LOGI(TAG, "No data for 30 minutes, measurement sensors marked unavailable");
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

}  // namespace omnik_bridge
}  // namespace esphome

