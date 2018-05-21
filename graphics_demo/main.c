//
//
// gcc graphics_demo/main.c -o demo -I /usr/local/Cellar/sdl2/2.0.8/include/SDL2 -L /usr/local/Cellar/sdl2/2.0.8/lib -l SDL2-2.0.0o
//
//

#include <stdlib.h>
#include <time.h>

#include "SDL.h"

#include "defines.h"
#include "connectome.h"
#include "muscles.h"

#define WINDOW_X 640
#define WINDOW_Y 480

#define SPRITE_W 40
#define SPRITE_H 40

typedef struct {
  int x;
  int y;
  int w;
  int h;
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

  const double dtheta_scale = 8.0;
  const double dtheta = (muscle->left - muscle->right)*dtheta_scale;

  worm->theta += dtheta*dt;

  worm->vx = velocity*cos(worm->theta*M_PI/180.0);
  worm->vy = velocity*sin(worm->theta*M_PI/180.0);

  worm->x += worm->vx*dt;
  worm->y += worm->vy*dt;
}

void sprite_update(Sprite* const sprite, Worm* const worm) {
  sprite->x = (int)worm->phys_state.x;
  sprite->y = (int)worm->phys_state.y;
  sprite->theta = 90.0 + atan2(worm->phys_state.vy, worm->phys_state.vx)*180.0/M_PI;
  if((worm->bio_state.muscle.left < 0) && (worm->bio_state.muscle.right < 0)) {
    sprite->theta += 180;
  }
}

void worm_update(Worm* const worm, const uint16_t* stim_neuron, int len_stim_neuron) {
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
  double motor_neuron_sum = 0;

  for(int i = 0; i < MOTOR_B; i++) {
    uint16_t id = READ_WORD(motor_neuron_b, i);
    motor_neuron_sum += ctm_get_discharge(ctm, id);
    //printf("%d\n", ctm_get_discharge(ctm, id));
  }

  for(int i = 0; i < MOTOR_A; i++) {
    uint16_t id = READ_WORD(motor_neuron_a, i);
    motor_neuron_sum += ctm_get_discharge(ctm, id);
    //printf("%d\n", ctm_get_discharge(ctm, id));
  }

  const double motor_total = MOTOR_A + MOTOR_B;

  worm->bio_state.motor_ab_fire_avg = (motor_neuron_sum + (motor_total*worm->bio_state.motor_ab_fire_avg))/(motor_total + 1.0);
  printf("%f\n", worm->bio_state.motor_ab_fire_avg);

  // Set left and right totals, scale neck muscle contribution
  int32_t left_total = (6*left_neck_total) + norm_body_total;
  int32_t right_total = (6*right_neck_total) + norm_body_total;

  if(worm->bio_state.motor_ab_fire_avg > 5.5) { // Magic number read off from c_matoduino simulation
    //printf("%f\n", worm->bio_state.motor_ab_fire_avg);
    //printf("reverse");
    left_total *= -1;
    right_total *= -1;
  }

  worm->bio_state.muscle.left = left_total / 12;
  worm->bio_state.muscle.right = right_total / 12;

  worm_phys_state_update(&worm->phys_state, &worm->bio_state.muscle);
}

double dot(double const* a, double const* b) {
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

// Handles effects on worm's physical state if wall collision occurs
// returns true if its nose is touching a wall
uint8_t collide_with_wall(Worm* const worm) {
  // Define normal vectors
  static const double nvec_bottom[] = {0.0, -1.0};
  static const double nvec_top[] = {0.0, 1.0};
  static const double nvec_right[] = {1.0, 0.0};
  static const double nvec_left[] = {-1.0, 0.0};

  double v[] = {worm->phys_state.vx, worm->phys_state.vy};
  double dot_prod = 0;

  uint8_t collide;

  // Right
  if(worm->phys_state.x > WINDOW_X - SPRITE_W) {
    worm->phys_state.x = WINDOW_X - SPRITE_W;
    dot_prod = dot(v, nvec_right);
    collide = 1;
  }
  // Left
  else if(worm->phys_state.x < 0.0) {
    worm->phys_state.x = 0.0;
    dot_prod = dot(v, nvec_left);
    collide = 1;
  }
  // Top
  if(worm->phys_state.y > WINDOW_Y - SPRITE_H) {
    worm->phys_state.y = WINDOW_Y - SPRITE_H;
    dot_prod = dot(v, nvec_top);
    collide = 1;
  }
  // Bottom
  else if(worm->phys_state.y < 0.0) {
    worm->phys_state.y = 0.0;
    dot_prod = dot(v, nvec_bottom);
    collide = 1;
  }

  /*if(collide) {
    worm->phys_state.vx = 0.0;
    worm->phys_state.vy = 0.0;
  }*/

  if(collide && (dot_prod > 0)) {
    return 1;
  }
  else {
    return 0;
  }
}

int main(int argc, char* argv[]) {
  // Initialize graphics window
  SDL_Window* win;
  SDL_Renderer* rend;

  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(WINDOW_X, WINDOW_Y, 0, &win, &rend);
  SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

  // Pull image for worm sprite and create texture
  SDL_Surface* surf;
  SDL_Texture* tex;

  surf = SDL_LoadBMP("./graphics_demo/worm.bmp");
  tex = SDL_CreateTextureFromSurface(rend, surf);
  SDL_FreeSurface(surf);

  // Create and initialize worm
  Worm worm;
  worm.phys_state = (WormPhysicalState) {WINDOW_X/2, WINDOW_Y/2, 0.0, 0.0, 90.0};
  ctm_init(&worm.bio_state.connectome);
  worm.bio_state.muscle = (MuscleState) {0, 0};
  worm.bio_state.motor_ab_fire_avg = 5.25;

  Sprite sprite = {(int)worm.phys_state.x, (int)worm.phys_state.y, SPRITE_W, SPRITE_H, worm.phys_state.theta};

  // Burn in worm state
  srand(time(NULL));
  int rand_int = (rand() % 1000) + 500;
  for(int i = 0; i < rand_int; i++) {
      worm_update(&worm, chemotaxis, 8);
  }

  // Begin graphical simulation
  uint8_t nose_touching = 0;
  SDL_Event sdl_event;

  // Main animation loop
  for(int i=0; 1; i++) {

    // Check for keypress---quit if there is one
    if(SDL_PollEvent(&sdl_event)) {
      if(sdl_event.type == SDL_KEYDOWN) {
        break;
      }
    }

    SDL_SetRenderDrawColor(rend, 128, 128, 128, 255);
    SDL_RenderClear(rend);

    if(nose_touching) {
      worm_update(&worm, nose_touch, 10);
      SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
    }
    else {
      worm_update(&worm, chemotaxis, 8);
      SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);
    }

    //printf("%d %d\n", worm.bio_state.muscle.left, worm.bio_state.muscle.right);

    nose_touching = collide_with_wall(&worm);

    sprite_update(&sprite, &worm);

    SDL_Rect sprite_rect;
    sprite_rect.x = sprite.x;
    sprite_rect.y = sprite.y;
    sprite_rect.w = sprite.w;
    sprite_rect.h = sprite.h;


    SDL_RenderCopyEx(rend, tex, NULL, &sprite_rect, sprite.theta, NULL, SDL_FLIP_NONE);
    SDL_RenderDrawRect(rend, &sprite_rect);

    SDL_RenderPresent(rend);

    SDL_Delay(100);
    //break;
  }

  SDL_Quit();
}
