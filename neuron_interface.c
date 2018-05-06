#include "neuron_interface.h"

// Parse a word of the ROM into a neuron id and connection weight
const NeuronConnection _parse_rom_word(const uint16_t rom_word) {
  uint8_t* rom_byte;
  rom_byte = (uint8_t*)&rom_word;

  uint16_t id = rom_byte[1] + ((rom_byte[0] & 0b10000000) << 1);
  
  uint8_t weight_bits = rom_byte[0] & 0b01111111;
  weight_bits = weight_bits + ((weight_bits & 0b01000000) << 1);
  int8_t weight = (int8_t)weight_bits;

  NeuronConnection neuron_conn = {id, weight};

  return neuron_conn;
}
