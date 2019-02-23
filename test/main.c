// Simple test of connectome interfaces
//
// Compile with:
// gcc -ggdb -I./source -o ./nanotode_test test/main.c source/muscles.c source/connectome.c source/neural_rom.c
//

#include <stdio.h>

#include "defines.h"
#include "connectome.h"
#include "muscles.h"

void print_motor_ab_discharges(FILE* const f, const uint8_t* a, const uint8_t* b) {
  for(uint8_t i = 0; i < MOTOR_A; i++) {
    fprintf(f, "%d ", a[i]);
  }
  for(uint8_t i = 0; i < MOTOR_B - 1; i++) {
    fprintf(f, "%d ", b[i]);
  }
  fprintf(f, "%d\n", b[MOTOR_B - 1]);
}

int main() {
  // Open file for writing
  FILE* file = fopen("motor_ab.dat", "w");
  
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

  // Perform burn-in
  for(uint16_t i = 0; i < 1000; i++) {
    ctm_neural_cycle(&connectome, chemotaxis, 8);
  }

  double motor_neuron_avg = 5.25;

  // Query final state for which A and B motor
  // neurons discharged
  uint8_t motor_a_result[MOTOR_A];
  uint8_t motor_b_result[MOTOR_B];

  // Run c. elegans emulation
  // (100 neural cycles of each behavior)
  for(uint16_t i = 0; i < 1000; i++) {
    ctm_neural_cycle(&connectome, chemotaxis, 8);
    ctm_discharge_query(&connectome, motor_neuron_b, motor_b_result, MOTOR_B);
    ctm_discharge_query(&connectome, motor_neuron_a, motor_a_result, MOTOR_A);
    print_motor_ab_discharges(file, motor_a_result, motor_b_result);
  }

  for(uint16_t i = 0; i < 1000; i++) {
    ctm_neural_cycle(&connectome, nose_touch, 10);
    ctm_discharge_query(&connectome, motor_neuron_b, motor_b_result, MOTOR_B);
    ctm_discharge_query(&connectome, motor_neuron_a, motor_a_result, MOTOR_A);
    print_motor_ab_discharges(file, motor_a_result, motor_b_result);
  }



  fclose(file);
  return 0;
}
