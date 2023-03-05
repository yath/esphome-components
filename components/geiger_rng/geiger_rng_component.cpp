#include "geiger_rng_component.h"

#include <cinttypes>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

using namespace esphome;

static const char *const TAG = "geiger_rng";

void IRAM_ATTR GeigerRNGComponent::interrupt_handler(GeigerRNGComponent *c) {
  const uint32_t now = ::micros();

  const bool falling = !c->isr_pin_.digital_read();
  if (falling) {
    const uint32_t pulse_width = now - c->last_edge_;
    if (pulse_width < c->internal_filter_) {
      ESP_LOGD(TAG, "Ignoring short %" PRIu32 "µs pulse", pulse_width);
    } else {
      const uint32_t delta = c->last_edge_ - c->last_pulse_;
      ESP_LOGV(TAG, "Registering %" PRIu32 "µs delta for %" PRIu32 "µs pulse",
               delta, pulse_width);
      c->buffer_.push(delta);
      c->last_pulse_ = c->last_edge_;
    }
  }

  c->last_edge_ = now;
}

void GeigerRNGComponent::setup() {
  pin_->setup();
  pin_->attach_interrupt(interrupt_handler, this, gpio::INTERRUPT_ANY_EDGE);
  async_server_.setNoDelay(true);
  async_server_.begin();
  async_server_.onClient(GeigerRNGComponent::client_connected, this);
}

String GeigerRNGComponent::host_port_for(AsyncClient *client) const {
  const auto c = async_clients_.find(client);
  if (c == async_clients_.end()) {
    return {"(unknown client)"};
  }

  return c->second;
}

void GeigerRNGComponent::client_connected(void *arg, AsyncClient *client) {
  GeigerRNGComponent *c = reinterpret_cast<GeigerRNGComponent *>(arg);

  String host_port = client->remoteIP().toString() + ':' + client->remotePort();
  ESP_LOGI(TAG, "Client %s connected", host_port.c_str());

  client->setNoDelay(true);
  c->async_clients_.insert({client, host_port});

  client->onDisconnect(
      [c](void *, AsyncClient *client) {
        ESP_LOGI(TAG, "Client %s disconnected",
                 c->host_port_for(client).c_str());
        c->async_clients_.erase(client);
      },
      nullptr);

  client->onError(
      [c](void *, AsyncClient *client, err_t err) {
        ESP_LOGW(TAG, "Client %s disconnected with error %ld (%s)",
                 c->host_port_for(client).c_str(), err,
                 client->errorToString(err));
        c->async_clients_.erase(client);
      },
      nullptr);

  client->onTimeout(
      [c](void *, AsyncClient *client, uint32_t time) {
        ESP_LOGW(TAG, "Client %s disconnected after timeout (%" PRIu32 "ms)",
                 c->host_port_for(client).c_str(), time);
        c->async_clients_.erase(client);
      },
      nullptr);
}

void GeigerRNGComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "RNG:");
  LOG_PIN("  Pin: ", pin_);
  ESP_LOGCONFIG(TAG, "  Port: %" PRIu16, port_);
  ESP_LOGCONFIG(TAG, "  Filtering pulses shorter than %" PRIu32 "µs",
                internal_filter_);
}

bool GeigerRNGComponent::get_random_byte(uint8_t *val) {
  while (buffer_.size() >= 2 &&
         n_random_bits_ < sizeof(random_bits_) * CHAR_BIT) {
    uint32_t t1, t2;
    if (!buffer_.pop(&t1) || !buffer_.pop(&t2)) {
      ESP_LOGE(TAG, "Ring buffer empty despite size check (currently: %d)",
               buffer_.size());
      continue;
    };

    if (t1 == t2)
      continue;
    const bool random_bit = (t1 < t2);
    random_bits_ = (random_bits_ << 1) | random_bit;
    n_random_bits_++;
  }

  constexpr auto want_n_bits = sizeof(*val) * CHAR_BIT;
  if (n_random_bits_ >= want_n_bits) {
    constexpr auto mask = (1ULL << want_n_bits) - 1;
    *val = random_bits_ & mask;
    random_bits_ >>= want_n_bits;
    n_random_bits_ -= want_n_bits;
    return true;
  }

  return false;
}

void GeigerRNGComponent::loop() {
  uint8_t val;
  if (!get_random_byte(&val))
    return;

  ESP_LOGD(TAG, "random number: %" PRIu8, val);
  for (auto [client, host_port] : async_clients_) {
    client->add(reinterpret_cast<char *>(&val), sizeof(val));
    if (client->canSend()) {
      if (!client->send()) {
        ESP_LOGW(TAG, "can't send data to client %s", host_port.c_str());
      }
    }
  }
}
