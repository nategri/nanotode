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

//
// Structs for worm sprite
// and worm state
//

// Worm sprite
typedef struct {
  int x;
  int y;
  int w;
  int h;
  double theta;
} Sprite;

// Worm states
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
  int meta_left_neck;
  int meta_right_neck;
  int meta_body;
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

//
// Structs for muscle cell state display
//

typedef struct {
  SDL_Rect rect;
  uint8_t rgba[4];
} MuscleDisplayCell;

typedef struct {
  MuscleDisplayCell left_d_cell[19];
  MuscleDisplayCell left_v_cell[19];
  MuscleDisplayCell right_d_cell[19];
  MuscleDisplayCell right_v_cell[19];
} MuscleDisplay;

//
// Structs movement component display
//

typedef struct {
  SDL_Rect left_body;
  SDL_Rect right_body;
  SDL_Rect left_neck;
  SDL_Rect right_neck;
  uint8_t color_body[4];
  uint8_t color_neck[4];
} MotionComponentDisplay;

// Set position and colors of motion component display
void motion_component_display_init(MotionComponentDisplay* const motion_comp_disp) {
  // Make this position stuff less hardcoded, eh?
  motion_comp_disp->left_body = (SDL_Rect) {20, 160, 22, 0};
  motion_comp_disp->left_neck = (SDL_Rect) {20, 160, 22, 0};

  motion_comp_disp->right_body = (SDL_Rect) {44, 160, 22, 0};
  motion_comp_disp->right_neck = (SDL_Rect) {44, 160, 22, 0};

  motion_comp_disp->color_neck[0] = 180;
  motion_comp_disp->color_neck[1] = 76;
  motion_comp_disp->color_neck[2] = 211;
  motion_comp_disp->color_neck[3] = 255;

  motion_comp_disp->color_body[0] = 140;
  motion_comp_disp->color_body[1] = 59;
  motion_comp_disp->color_body[2] = 165;
  motion_comp_disp->color_body[3] = 255;
}

void motion_component_display_update(MotionComponentDisplay* const motion_component_disp, Worm* const worm) {
  const double scale = 0.5;

  double left_body_comp = worm->bio_state.muscle.meta_body;
  double right_body_comp = worm->bio_state.muscle.meta_body;
  double left_neck_comp = worm->bio_state.muscle.meta_left_neck;
  double right_neck_comp = worm->bio_state.muscle.meta_right_neck;

  motion_component_disp->left_body.h = (int) (left_body_comp*scale);
  motion_component_disp->left_body.y = 160 - (int) (left_body_comp*scale);

  motion_component_disp->right_body.h = (int) (right_body_comp*scale);
  motion_component_disp->right_body.y = 160 - (int) (right_body_comp*scale);

  motion_component_disp->left_neck.h = (int) left_neck_comp*scale;
  motion_component_disp->left_neck.y = 160 - (((int) left_neck_comp*scale) + motion_component_disp->left_body.h);

  motion_component_disp->right_neck.h = (int) right_neck_comp*scale;
  motion_component_disp->right_neck.y = 160 - (((int) right_neck_comp*scale) + motion_component_disp->right_body.h);
}
void motion_component_display_draw(SDL_Renderer* rend, MotionComponentDisplay* const motion_component_disp) {
  uint8_t* rgba;

  rgba = motion_component_disp->color_body;
  SDL_SetRenderDrawColor(rend, rgba[0], rgba[1], rgba[2], rgba[3]);
  SDL_RenderFillRect(rend, &(motion_component_disp->left_body));
  SDL_RenderFillRect(rend, &(motion_component_disp->right_body));

  rgba = motion_component_disp->color_neck;
  SDL_SetRenderDrawColor(rend, rgba[0], rgba[1], rgba[2], rgba[3]);
  SDL_RenderFillRect(rend, &(motion_component_disp->left_neck));
  SDL_RenderFillRect(rend, &(motion_component_disp->right_neck));
}

// Set positions and default colors of muscle display cells
void muscle_display_init(MuscleDisplay* const muscle_disp) {
  int x = 20;
  int y = 180;
  const uint8_t w = 10;
  const uint8_t h = 10;
  // Pad doesn't quite mean what you think it did (it's really "2")
  const uint8_t pad = 12;

  // For neck muscles
  for(uint8_t i = 0; i < 4; i++) {
    muscle_disp->left_d_cell[i].rect = (SDL_Rect) {x, y, w, h};
    muscle_disp->left_v_cell[i].rect = (SDL_Rect) {x+pad, y, w, h};
    muscle_disp->right_v_cell[i].rect = (SDL_Rect) {x+2*pad, y, w, h};
    muscle_disp->right_d_cell[i].rect = (SDL_Rect) {x+3*pad, y, w, h};
    y += pad;
  }
  
  // For body muscles
  y += h;
  for(uint8_t i = 4; i < 19; i++) {
    muscle_disp->left_d_cell[i].rect = (SDL_Rect) {x, y, w, h};
    muscle_disp->left_v_cell[i].rect = (SDL_Rect) {x+pad, y, w, h};
    muscle_disp->right_v_cell[i].rect = (SDL_Rect) {x+2*pad, y, w, h};
    muscle_disp->right_d_cell[i].rect = (SDL_Rect) {x+3*pad, y, w, h};
    y += pad;
  }

  // Assign default colors
  for(uint8_t i = 0; i < 19; i++) {
    for(uint8_t j = 0; j < 4; j++) {
      muscle_disp->left_d_cell[i].rgba[j] = 255;
      muscle_disp->left_v_cell[i].rgba[j] = 255;
      muscle_disp->right_d_cell[i].rgba[j] = 255;
      muscle_disp->right_v_cell[i].rgba[j] = 255;
    }
  }
}

void muscle_display_cell_color(MuscleDisplayCell* const cell, uint8_t i, float weight) {
  if(weight < 0) {
    cell[i].rgba[0] = 255 + weight;
    cell[i].rgba[1] = 255 + weight;
    cell[i].rgba[2] = 255;
  }
  else {
    cell[i].rgba[0] = 255;
    cell[i].rgba[1] = 255 - weight;
    cell[i].rgba[2] = 255 - weight;
  }
}

void muscle_display_update(MuscleDisplay* const muscle_disp, Worm* const worm) {
  Connectome* ctm = &worm->bio_state.connectome;

  const double scale = 30.0;

  //
  // Neck muscles
  //

  // Dorsal
  for(int8_t i = 0; i < 4; i++) {
    double l_wt = ctm_get_weight(ctm, left_neck_muscle[i])*scale;
    double r_wt = ctm_get_weight(ctm, right_neck_muscle[i])*scale;

    muscle_display_cell_color(muscle_disp->left_d_cell, i, l_wt);
    muscle_display_cell_color(muscle_disp->right_d_cell, i, r_wt);
  }

  // Ventral
  uint8_t cell_idx;
  for(uint8_t i = 4; i < 8; i++) {
    double l_wt = ctm_get_weight(ctm, left_neck_muscle[i])*scale;
    double r_wt = ctm_get_weight(ctm, right_neck_muscle[i])*scale;

    cell_idx = i - 4;

    muscle_display_cell_color(muscle_disp->left_v_cell, cell_idx, l_wt);
    muscle_display_cell_color(muscle_disp->right_v_cell, cell_idx, r_wt);
  }

  //
  // Body muscles
  //

  // Dorsal
  for(int8_t i = 0; i < 15; i++) {
    double l_wt = ctm_get_weight(ctm, left_body_muscle[i])*scale;
    double r_wt = ctm_get_weight(ctm, right_body_muscle[i])*scale;

    cell_idx = i + 4;

    muscle_display_cell_color(muscle_disp->left_d_cell, cell_idx, l_wt);
    muscle_display_cell_color(muscle_disp->right_d_cell, cell_idx, r_wt);
  }

  // Ventral
  for(uint8_t i = 15; i < 30; i++) {
    double l_wt = ctm_get_weight(ctm, left_body_muscle[i])*scale;
    double r_wt = ctm_get_weight(ctm, right_body_muscle[i])*scale;

    cell_idx = i + 4 - 15;

    muscle_display_cell_color(muscle_disp->left_v_cell, cell_idx, l_wt);
    muscle_display_cell_color(muscle_disp->right_v_cell, cell_idx, r_wt);
  }
}

void muscle_display_draw(SDL_Renderer* rend, MuscleDisplay* const muscle_disp) {
  for(uint8_t i = 0; i < 19; i++) {
    uint8_t* rgba;

    rgba = muscle_disp->left_d_cell[i].rgba;
    SDL_SetRenderDrawColor(rend, rgba[0], rgba[1], rgba[2], rgba[3]);
    SDL_RenderFillRect(rend, &(muscle_disp->left_d_cell[i].rect));

    rgba = muscle_disp->left_v_cell[i].rgba;
    SDL_SetRenderDrawColor(rend, rgba[0], rgba[1], rgba[2], rgba[3]);
    SDL_RenderFillRect(rend, &(muscle_disp->left_v_cell[i].rect));

    rgba = muscle_disp->right_d_cell[i].rgba;
    SDL_SetRenderDrawColor(rend, rgba[0], rgba[1], rgba[2], rgba[3]);
    SDL_RenderFillRect(rend, &(muscle_disp->right_d_cell[i].rect));

    rgba = muscle_disp->right_v_cell[i].rgba;
    SDL_SetRenderDrawColor(rend, rgba[0], rgba[1], rgba[2], rgba[3]);
    SDL_RenderFillRect(rend, &(muscle_disp->right_v_cell[i].rect));
  }
}

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
  const double dt = 0.0005;

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

  if(!(worm->phys_state.vx == 0 && worm->phys_state.vy == 0)) {
    sprite->theta = 90.0 + atan2(worm->phys_state.vy, worm->phys_state.vx)*180.0/M_PI;
  }
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

  // Combine neck contribution with body and log meta information
  // for motion component visualizer
  int32_t neck_contribution = left_neck_total - right_neck_total;
  int32_t left_total;
  int32_t right_total;
  if(neck_contribution < 0) {
    left_total = 6*abs(neck_contribution) + norm_body_total;
    right_total = norm_body_total;
    worm->bio_state.muscle.meta_left_neck = 6*abs(neck_contribution);
    worm->bio_state.muscle.meta_right_neck = 0;
  }
  else {
    left_total = norm_body_total;
    right_total = 6*abs(neck_contribution) + norm_body_total;
    worm->bio_state.muscle.meta_left_neck = 0;
    worm->bio_state.muscle.meta_right_neck = 6*abs(neck_contribution);
  }

  worm->bio_state.muscle.meta_body = norm_body_total;

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

  if(worm->bio_state.motor_ab_fire_avg > 5.5) { // Magic number read off from c_matoduino simulation
    //printf("%f\n", worm->bio_state.motor_ab_fire_avg);
    //printf("reverse");
    left_total *= -1;
    right_total *= -1;
  }

  worm->bio_state.muscle.left = left_total / 0.5;
  worm->bio_state.muscle.right = right_total / 0.5;


  worm_phys_state_update(&worm->phys_state, &worm->bio_state.muscle);
}

double dot(double const* a, double const* b) {
  return a[0]*b[0] + a[1]*b[1];
}

// Handles effects on worm's physical state if wall collision occurs
// returns true if its nose is touching a wall
uint8_t collide_with_wall(Worm* const worm) {
  // Define normal vectors
  static const double nvec_bottom[] = {0.0, -1.0};
  static const double nvec_top[] = {0.0, 1.0};
  static const double nvec_right[] = {-1.0, 0.0};
  static const double nvec_left[] = {1.0, 0.0};

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
  // Bottom
  if(worm->phys_state.y > WINDOW_Y - SPRITE_H) {
    worm->phys_state.y = WINDOW_Y - SPRITE_H;
    dot_prod = dot(v, nvec_bottom);
    collide = 1;
  }
  // Top
  else if(worm->phys_state.y < 0.0) {
    worm->phys_state.y = 0.0;
    dot_prod = dot(v, nvec_top);
    collide = 1;
  }

  // Remember, its notice won't touch if it's backing up
  if(collide && dot_prod < 0 && worm->bio_state.muscle.left > 0 && worm->bio_state.muscle.right > 0) {
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

  // Create and initialize muscle display
  MuscleDisplay muscle_display;
  muscle_display_init(&muscle_display);

  // Create and initialize motion component
  MotionComponentDisplay motion_component_display;
  motion_component_display_init(&motion_component_display);

  // Create and initialize worm
  Worm worm;
  worm.phys_state = (WormPhysicalState) {WINDOW_X/2, WINDOW_Y/2, 0.0, 0.0, 90.0};
  ctm_init(&worm.bio_state.connectome);
  worm.bio_state.muscle = (MuscleState) {0, 0, 0, 0, 0};
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

    if(i % 120 == 0) {
      if(nose_touching) {
        worm_update(&worm, nose_touch, 10);
      }
      else {
        worm_update(&worm, chemotaxis, 8);
      }
    }
    else {
      worm_phys_state_update(&(worm.phys_state), &(worm.bio_state.muscle));
    }

    if(nose_touching) {
      SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
    }
    else {
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

    motion_component_display_update(&motion_component_display, &worm);
    motion_component_display_draw(rend, &motion_component_display);

    muscle_display_update(&muscle_display, &worm);
    muscle_display_draw(rend, &muscle_display);

    SDL_RenderPresent(rend);

    //SDL_Delay(100);
    //break;
  }

  SDL_Quit();
}
