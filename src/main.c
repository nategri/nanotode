// Simple test of connectome interfaces
//
// Compile with:
// gcc -I./include -o ./nanotode_test src/main.c src/muscles.c src/connectome.c src/neural_rom.c
//

#include "defines.h"
#include "connectome.h"
#include "muscles.h"

int main() {
  
  // Arrays for neurons describing 'nose touch' and
  // food-seeking behaviors
  const uint16_t nose_touch[] = {
    N_FLPR, N_FLPL, N_ASHL, N_ASHR, N_IL1VL, N_IL1VR,
    N_OLQDL, N_OLQDR, N_OLQVR, N_OLQVL
  };

  const uint16_t chemotaxis[] = {
    N_ADFL, N_ADFR, N_ASGR, N_ASGL, N_ASIL, N_ASIR,
    N_ASJR, N_ASJL
  };

  // Create connectome struct
  Connectome connectome;

  // Initialize struct
  ctm_init(&connectome);

  // Run c. elegans emulation
  // (100 neural cycles of each behavior)
  for(uint8_t i = 0; i < 100; i++) {
    ctm_neural_cycle(&connectome, chemotaxis, 8);
  }

  for(uint8_t i = 0; i < 100; i++) {
    ctm_neural_cycle(&connectome, nose_touch, 10);
  }

  // Query final state for which A and B motor
  // neurons discharged
  uint8_t motor_a_result[MOTOR_A];
  uint8_t motor_b_result[MOTOR_B];

  ctm_discharge_query(&connectome, motor_neuron_a, motor_a_result, MOTOR_A);
  ctm_discharge_query(&connectome, motor_neuron_b, motor_b_result, MOTOR_B);

  return 0;
}
