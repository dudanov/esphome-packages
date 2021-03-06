#include "esphome.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/remote_base/nec_protocol.h"

using namespace esphome;
using namespace esphome::light;
using namespace esphome::remote_base;

#define FERON_ADDRESS_DIM_BH  0xB721
#define FERON_ADDRESS_NATIVE  0xB7A0
#define FERON_POWER_OFF       0x13EC

constexpr float kelvin_to_mireds(unsigned k) { return 1e6 / k; }

class FeronLightOutput : public LightOutput {
 public:
  LightTraits get_traits() override {
    auto traits = LightTraits();
    traits.set_supported_color_modes({ColorMode::COLOR_TEMPERATURE});
    traits.set_min_mireds(kelvin_to_mireds(6500)); // 153
    traits.set_max_mireds(kelvin_to_mireds(2700)); // 371
    return traits;
  }
  void write_state(LightState *state) override {
    float color, brightness;
    uint16_t address = FERON_ADDRESS_NATIVE, command = FERON_POWER_OFF;
    state->current_values_as_ct(&color, &brightness);
    if (brightness > 0.0f) {
      address = FERON_ADDRESS_DIM_BH;
      command = static_cast<uint16_t>(0.5f + brightness * 15.0f) * 16 + static_cast<uint16_t>(15.5f - color * 15.0f);
      command += ~command * 256;
    }
    ESP_LOGD("feron_light", "Set color: %f, brightness: %f. NEC command: 0x%04X", color, brightness, command);
    auto transmit = this->remote_->transmit();
    NECProtocol().encode(transmit.get_data(), {address, command});
    transmit.perform();
  }
  void set_remote_transmitter(RemoteTransmitterBase *remote) { this->remote_ = remote; }

 protected:
  RemoteTransmitterBase *remote_;
};
