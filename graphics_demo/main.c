//
//
// gcc graphics_demo/main.c -o demo -I /usr/local/Cellar/sdl2/2.0.8/include/SDL2 -L /usr/local/Cellar/sdl2/2.0.8/lib -l SDL2-2.0.0o
//
//

#include "SDL.h"
#include <stdlib.h>

#include "defines.h"
#include "connectome.h"
#include "muscles.h"

typedef struct {
  int x;
  int y;
  double theta;
} Sprite;

typedef struct {
  double x;
  double y;
  double vx;
  double vy;
  double theta;
} WormPhysicalState;

typedef struct {
  int left;
  int right;
} MuscleState;

typedef struct {
  Connectome connectome;
  MuscleState muscle;
  double motor_ab_fire_avg;
} WormBioState;

typedef struct {
  WormPhysicalState phys_state;
  WormBioState bio_state;
} Worm;

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

void worm_phys_state_update(WormPhysicalState* const worm, MuscleState* const muscle) {
  const double dt = 1.0;

  const double velocity = (muscle->left + muscle->right)/2.0;

  const double dtheta_scale = 2.0;
  const double dtheta = (muscle->left - muscle->right)*dtheta_scale;

  worm->theta += dtheta*dt;

  worm->vx = velocity*cos(worm->theta*M_PI/180.0);
  worm->vy = velocity*sin(worm->theta*M_PI/180.0);

  worm->x += worm->vx*dt;
  worm->y += worm->vy*dt;
}

void sprite_update(Sprite* const sprite, WormPhysicalState* const worm) {
  sprite->x = (int)worm->x;
  sprite->y = (int)worm->y;
  sprite->theta = 90.0 + atan2(worm->vy, worm->vx)*180.0/M_PI;
}

void worm_update(Worm* worm, const uint16_t* stim_neuron, int len_stim_neuron) {
  Connectome* ctm = &worm->bio_state.connectome;

  //
  // Run one tick of neural emulation
  //

  ctm_neural_cycle(ctm, stim_neuron, len_stim_neuron);

  //
  // Aggregate muscle states
  //

  uint16_t body_total = 0;
  // Gather totals on body muscles
  for(int i = 0; i < BODY_MUSCLES; i++) {
    uint16_t left_id = READ_WORD(left_body_muscle, i);
    uint16_t right_id = READ_WORD(right_body_muscle, i);

    int16_t left_val = ctm_get_weight(ctm, left_id);
    int16_t right_val = ctm_get_weight(ctm, right_id);

    if(left_val < 0) {
      left_val = 0;
    }

    if(right_val < 0) {
      right_val = 0;
    }

    body_total += (left_val + right_val);
  }

  uint16_t norm_body_total = 255.0 * ((float) body_total) / 600.0;

  // Gather total for neck muscles
  uint16_t left_neck_total = 0;
  uint16_t right_neck_total = 0;
  for(int i = 0; i < NECK_MUSCLES; i++) {
    uint16_t left_id = READ_WORD(left_neck_muscle, i);
    uint16_t right_id = READ_WORD(right_neck_muscle, i);

    int16_t left_val = ctm_get_weight(ctm, left_id);
    int16_t right_val = ctm_get_weight(ctm, right_id);

    if(left_val < 0) {
      left_val = 0;
    }

    if(right_val < 0) {
      right_val = 0;
    }

    left_neck_total += left_val;
    right_neck_total += right_val;
  }

  // Log A and B type motor neuron activity
  uint8_t motor_neuron_a_sum = 0;
  uint8_t motor_neuron_b_sum = 0;

  for(int i = 0; i < SIG_MOTOR_B; i++) {
    uint8_t id = READ_WORD(sig_motor_neuron_b, i);
    motor_neuron_b_sum += ctm_get_discharge(ctm, id);
  }

  for(int i = 0; i < SIG_MOTOR_A; i++) {
    uint8_t id = READ_WORD(sig_motor_neuron_a, i);
    motor_neuron_a_sum += ctm_get_discharge(ctm, id);
  }
  
  // Sum (with weights) and add contribution to running average of significant activity
  float motor_neuron_sum_total = (-1*motor_neuron_a_sum) + motor_neuron_b_sum;

  worm->bio_state.motor_ab_fire_avg = (worm->bio_state.motor_ab_fire_avg + (5.0*worm->bio_state.motor_ab_fire_avg))/(5.0 + 1.0);

  // Set left and right totals, scale neck muscle contribution
  int32_t left_total = (6*left_neck_total) + norm_body_total;
  int32_t right_total = (6*right_neck_total) + norm_body_total;

  if(worm->bio_state.motor_ab_fire_avg < 0.27) { // Magic number read off from c_matoduino simulation
    left_total *= -1;
    right_total *= -1;
  }

  worm->bio_state.muscle.left = left_total / 12;
  worm->bio_state.muscle.right = right_total / 12;

  worm_phys_state_update(&worm->phys_state, &worm->bio_state.muscle);
}

int main(int argc, char* argv[]) {
  // Initialize graphics window
  SDL_Window* win;
  SDL_Renderer* rend;

  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(640, 480, 0, &win, &rend);
  SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

  // Pull image for worm sprite and create texture
  SDL_Surface* surf;
  SDL_Texture* tex;

  surf = SDL_LoadBMP("./graphics_demo/worm.bmp");
  tex = SDL_CreateTextureFromSurface(rend, surf);
  SDL_FreeSurface(surf);

  // Create and initialize worm
  Worm worm;
  worm.phys_state = (WormPhysicalState) {320.0, 240.0, 0.0, 0.0, 90.0};
  ctm_init(&worm.bio_state.connectome);
  worm.bio_state.muscle = (MuscleState) {0, 0};
  worm.bio_state.motor_ab_fire_avg = 0.0;

  Sprite sprite = {(int)worm.phys_state.x, (int)worm.phys_state.y, worm.phys_state.theta};

  for(int i=0; i < 500; i++) {
    SDL_SetRenderDrawColor(rend, 128, 128, 128, 255);
    SDL_RenderClear(rend);

    worm_update(&worm, nose_touch, 10);
    
    if(worm.phys_state.x > 640.0) {
      worm.phys_state.x = 0.0;
    }
    else if(worm.phys_state.x < 0.0) {
      worm.phys_state.x = 640.0;
    }
    if(worm.phys_state.y > 480.0) {
      worm.phys_state.y = 0.0;
    }
    else if(worm.phys_state.y < 0.0) {
      worm.phys_state.y = 480.0;
    }

    sprite_update(&sprite, &worm.phys_state);

    SDL_Rect sprite_rect;
    sprite_rect.x = sprite.x;
    sprite_rect.y = sprite.y;
    sprite_rect.w = 40;
    sprite_rect.h = 40;

    SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);

    SDL_RenderCopyEx(rend, tex, NULL, &sprite_rect, sprite.theta, NULL, SDL_FLIP_NONE);
    SDL_RenderDrawRect(rend, &sprite_rect);

    SDL_RenderPresent(rend);

    SDL_Delay(100);
    //break;
  }
}