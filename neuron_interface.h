#ifndef NEURON_INTERFACE_H
#define NEURON_INTERFACE_H

#include <stdint.h>

#include "neuron_interface.h"

// Struct for representing a neuron connection
typedef struct {
  const uint16_t id;
  const int8_t weight;
} NeuronConnection;

// Parse a word of the ROM into a neuron id and connection weight
const NeuronConnection _parse_rom_word(const uint16_t);

#endif
