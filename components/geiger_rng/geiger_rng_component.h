#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "shared_ring_buffer.h"

#ifdef USE_ESP8266
#include "ESPAsyncTCP.h"
#else
#include "AsyncTCP.h"
#endif

#include <unordered_map>

class GeigerRNGComponent : public esphome::Component {
public:
  GeigerRNGComponent(esphome::InternalGPIOPin *pin, uint16_t port,
                     uint32_t internal_filter)
      : pin_(pin), port_(port), internal_filter_(internal_filter),
        async_server_(AsyncServer(port)) {
    isr_pin_ = pin_->to_isr();
  }

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override {
    return esphome::setup_priority::AFTER_WIFI;
  }

protected:
  static void interrupt_handler(GeigerRNGComponent *);
  static void client_connected(void *, AsyncClient *);
  String host_port_for(AsyncClient *) const;

  esphome::InternalGPIOPin *pin_;
  esphome::ISRInternalGPIOPin isr_pin_;
  uint16_t port_;
  uint32_t internal_filter_;

  SharedRingBuffer<uint32_t, 32> buffer_;
  uint32_t last_edge_{0}, last_pulse_{0};

  bool get_random_byte(uint8_t *);
  uint8_t random_bits_{0};
  int n_random_bits_{0};

  AsyncServer async_server_;
  std::unordered_map<AsyncClient *, String> async_clients_;
};
