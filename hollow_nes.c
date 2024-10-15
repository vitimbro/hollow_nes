// Hollow NES - A demake of Hollow Knight for the NES

#include <stdlib.h>
#include <string.h>

#include "neslib.h" // include NESLIB header with useful NES functions
#include <nes.h> // include CC65 NES Header (PPU definitions)

#include "bcd.h" // BCD arithmetic support
//#link "bcd.c"

#include "vrambuf.h" // VRAM update buffer
//#link "vrambuf.c"

// CHR data
//#resource "hollow_nes_chr.chr"
//#link "hollow_nes_chr.s"

//include nametables 
#include "hollow_nes_nt_1.h"

// CONSTANTS

// Constants for screen edges (NES resolution is 256x240)
#define SCREEN_RIGHT_EDGE 240
#define SCREEN_LEFT_EDGE 0
#define SCREEN_UP_EDGE 0
#define SCREEN_DOWN_EDGE 224 // Normally 224 for gameplay area

// Constants for clarity and maintainability
#define TILE_SIZE 8         // Size of a tile in pixels (8x8)
#define NAMETABLE_WIDTH 32   // Number of tiles per row in the nametable
#define SCREEN_WIDTH 256     // Width of the screen in pixels
#define SCREEN_HEIGHT 240    // Height of the screen in pixels

// Constants for player attributes
#define PLAYER_INIT_X 64 // Initial X position
#define PLAYER_INIT_Y 150 // Initial Y position
#define PLAYER_SPEED 2 // Player movement speed

// Constants for physics
#define GRAVITY 1
#define MAX_FALL_SPEED 4
#define JUMP_SPEED -8
#define JUMP_COOLDOWN 30

// Animations delay between frames
#define ANIM_DELAY_IDLE 32
#define ANIM_DELAY_RUN 4

// Define the number of frames for each animation
#define STAND_ANIM_FRAMES 2
#define RUN_ANIM_FRAMES 4


// PALETTE setup

/*{pal:"nes",layout:"nes"}*/
const char PALETTE[32] = { 
  0x0C,			// screen color

  0x0F,0x30,0x2D,0x00,	// background palette 0
  0x0F,0x20,0x03,0x00,	// background palette 1
  0x0F,0x30,0x10,0x00,	// background palette 2
  0x0F,0x30,0x11,0x00,   // background palette 3

  0x16,0x35,0x24,0x00,	// sprite palette 0
  0x00,0x37,0x25,0x00,	// sprite palette 1
  0x0D,0x30,0x22,0x00,	// sprite palette 2
  0x0D,0x27,0x2A	// sprite palette 3
};

// Define properties for each tile in the tileset (256 possible tiles)
const unsigned char collision_properties[256] = {
 // 0 = no collision, 1 = solid
 // 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 1
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 2
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 3
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 4
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 5
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 6
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 7
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 8
    0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 9
    0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // a
    0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // b
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // c
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // d
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // e
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // f
  
};

// METASPRITES

// Metasprites define the player appearance in the game (2x2 tiles)
// Normal and flipped versions are defined for left/right movement

// define a 2x2 metasprite
#define DEF_METASPRITE_2x2(name,code,pal)\
const unsigned char name[]={\
        0,      0,      (code)+0,   pal, \
        8,      0,      (code)+1,   pal, \
        0,      8,      (code)+2,   pal, \
        8,      8,      (code)+3,   pal, \
        128};

// define a 2x2 metasprite, flipped horizontally
#define DEF_METASPRITE_2x2_FLIP(name,code,pal)\
const unsigned char name[]={\
        8,      0,      (code)+0,   (pal)|OAM_FLIP_H, \
        0,      0,      (code)+1,   (pal)|OAM_FLIP_H, \
        8,      8,      (code)+2,   (pal)|OAM_FLIP_H, \
        0,      8,      (code)+3,   (pal)|OAM_FLIP_H, \
        128};

// Define metasprites for the player 

// facing right
DEF_METASPRITE_2x2(playerRStand1, 0xd1, 2);
DEF_METASPRITE_2x2(playerRStand2, 0xe1, 2);

DEF_METASPRITE_2x2(playerRRun1, 0xd6, 2);
DEF_METASPRITE_2x2(playerRRun2, 0xe6, 2);
DEF_METASPRITE_2x2(playerRRun3, 0xf6, 2);

// facing left
DEF_METASPRITE_2x2_FLIP(playerLStand1, 0xd1, 2);
DEF_METASPRITE_2x2_FLIP(playerLStand2, 0xe1, 2);

DEF_METASPRITE_2x2_FLIP(playerLRun1, 0xd6, 2);
DEF_METASPRITE_2x2_FLIP(playerLRun2, 0xe6, 2);
DEF_METASPRITE_2x2_FLIP(playerLRun3, 0xf6, 2);

// ANIMATION SEQUENCES

// These arrays control the animation frames for different states

// standing
const unsigned char* const playerLStandSeq[STAND_ANIM_FRAMES] = {
  playerLStand1, playerLStand2 
};

const unsigned char* const playerRStandSeq[STAND_ANIM_FRAMES] = {
  playerRStand1, playerRStand2 
};


// running
const unsigned char* const playerLRunSeq[RUN_ANIM_FRAMES] = {
  playerLRun1, playerLRun2, playerLRun3, 
  playerLRun1 
};

const unsigned char* const playerRRunSeq[RUN_ANIM_FRAMES] = {
  playerRRun1, playerRRun2, playerRRun3,
  playerRRun1
};

// Player state constants
typedef enum { 
  STATE_IDLE,
  STATE_RUN
} PlayerState;




// Player position and movement variables

// player x/y positions
byte player_x;
byte player_y;
// player x/y deltas per frame (signed)
sbyte player_dx;
sbyte player_dy;

bool player_facing_left;
bool player_facing_right;
bool is_on_ground = false;
bool can_jump = true;

unsigned char jump_timer = 0;

unsigned char player_state; // Player state 
unsigned char frame = 0;  // animation frame counter
unsigned char current_anim_frame_count = 0;  // Variable to store the number of frames in the current sequence
unsigned char anim_delay_counter = 0; // animation frame delay counter 
unsigned char current_anim_delay = ANIM_DELAY_IDLE; 
const unsigned char* const* current_seq; // current animation sequence


// Function prototypes
void setup_graphics();
void initialize_player();

unsigned char get_tile_at(unsigned char x, unsigned char y);
bool check_collision(int new_x, int new_y);
bool check_collision_for_player(int new_x, int new_y);

void update_player();
void update_player_input();
void update_player_physics();
void update_player_position();
void update_player_state();

void animate_player(unsigned char* oam_id, unsigned char* anim_frame);
const unsigned char* const* get_animation_sequence();
void update_animation_sequence(const unsigned char* const* new_seq, unsigned char* anim_frame);
void update_animation_frame(unsigned char* anim_frame);
void draw_current_frame(unsigned char* oam_id, unsigned char anim_frame);

// Set up graphics and palette for the game
// setup PPU and tables
void setup_graphics() {
  oam_clear(); // clear sprites from OAM
  pal_all(PALETTE); // set palette colors
  ppu_on_all(); // turn on PPU rendering
}


// Initialize the player with starting position, speed, and state
// Sets the player to the idle state, facing right.
void initialize_player() {
    player_x = PLAYER_INIT_X;        // Set initial x-position
    player_y = PLAYER_INIT_Y;        // Set initial y-position
    player_dx = 0;                   // No horizontal movement at start
    player_dy = 0;                   // No vertical movement at start
    player_facing_right = true;      // Default to facing right
    player_facing_left = false;
    player_state = STATE_IDLE;       // Start in the idle state
}


// Update player state and movement based on input
void update_player() {
  
  update_player_input();     // Update player input
  update_player_physics();   // Handles physics (gravity and jump)
  update_player_position();  // Update position based on collisions
  update_player_state();     // Update player state based on movement (running, idle, etc.)

}


void update_player_input() {
  
  char pad = pad_poll(0); // Get input from controller 0 (first player)
  
  // Horizontal movement
  if ((pad & PAD_LEFT) && (player_x > SCREEN_LEFT_EDGE)) {
      player_dx = -PLAYER_SPEED;
      player_facing_right = false;
      player_facing_left = true;
  } 
  else if (pad & PAD_RIGHT && player_x < SCREEN_RIGHT_EDGE) {
      player_dx = PLAYER_SPEED;
      player_facing_left = false;
      player_facing_right = true;
  } 
  else {
      player_dx = 0;
  }

  // Jumping
  if (is_on_ground && (pad & PAD_A) && can_jump) {
    player_dy = JUMP_SPEED;       // apply jump force
    is_on_ground = false;         // player is no longer on the ground
    can_jump = false;             // Prevent jumping again until cooldown is over
    jump_timer = JUMP_COOLDOWN;   // Start jump cooldown
  }
}

void update_player_physics(int *new_y) {

  // Vertical movement
  
  // Apply gravity
  if (!is_on_ground) {     // If the player is in the air
    new_y += player_dy;    // Increase the y-position based on vertical speed
    player_dy += GRAVITY;  // Apply gravity to vertical speed
    
    // limit fall speed
    if (player_dy > MAX_FALL_SPEED) {
      player_dy = MAX_FALL_SPEED; 
    }
  }  
  
  // Cooldown management to allow jumping again after a delay
  if (jump_timer > 0) {
    jump_timer--;
    if (jump_timer == 0) {
        can_jump = true;    // Reset jump ability after cooldown ends
    }
  }
}

void update_player_position() {
    int new_x = player_x + player_dx;  // new horizontal position
    int new_y = player_y + player_dy;  // new vertical position

    // Check for collisions and update position accordingly
  
    // Horizontal collision
    if (!check_collision_for_player(new_x, player_y)) {
        player_x = new_x;  // Update X position if no horizontal collision
    }

    // Vertical collision
    if (!check_collision_for_player(player_x, new_y)) {
        player_y = new_y;  // Update Y position if no vertical collision
        is_on_ground = false;  // Player is falling unless a collision is detected below
    } else {
        player_dy = 0; // Stop falling if a collision is detected below
        is_on_ground = true;   // Player is on the ground
    }
}  


// Update the player's state (running or idle) based on movement
// Adjusts the animation delay and frame count depending on the state.
void update_player_state() {
  if (player_dx != 0) {
      player_state = STATE_RUN;
      current_anim_delay = ANIM_DELAY_RUN;
      current_anim_frame_count = RUN_ANIM_FRAMES;
} else {
      player_state = STATE_IDLE;
      current_anim_delay = ANIM_DELAY_IDLE;
      current_anim_frame_count = STAND_ANIM_FRAMES;
  }
}



// Main function to handle player animation
void animate_player(unsigned char* oam_id, unsigned char* anim_frame) {
    const unsigned char* const* new_seq = get_animation_sequence();  // Get current animation sequence
    
    update_animation_sequence(new_seq, anim_frame);   // Update animation sequence and frame
    update_animation_frame(anim_frame);               // Handle frame updates based on delay
    draw_current_frame(oam_id, *anim_frame);          // Draw the current frame
}

// Select the animation sequence based on player's state and direction
const unsigned char* const* get_animation_sequence() {
    if (player_state == STATE_RUN) {
        return player_facing_right ? playerRRunSeq : playerLRunSeq;  // Running right or left
    } 
    else if (player_state == STATE_IDLE) {
        return player_facing_right ? playerRStandSeq : playerLStandSeq;  // Idle right or left
    }
    return current_seq;  // Default to current sequence if state is unknown
}

// Update the animation sequence and reset frame if sequence changes
void update_animation_sequence(const unsigned char* const* new_seq, unsigned char* anim_frame) {
    if (new_seq != current_seq) {
        *anim_frame = 0;  // Reset animation frame
        anim_delay_counter = 0;  // Reset delay counter
        current_seq = new_seq;  // Update current sequence
    }
}

// Handle the animation frame updates based on the delay
void update_animation_frame(unsigned char* anim_frame) {
    if (anim_delay_counter == 0) {
        *anim_frame = (*anim_frame + 1) % current_anim_frame_count;  // Update frame with wraparound
    }
    anim_delay_counter = (anim_delay_counter + 1) % current_anim_delay;  // Update delay counter
}

// Draw the current frame using meta-sprites
void draw_current_frame(unsigned char* oam_id, unsigned char anim_frame) {
    *oam_id = oam_meta_spr(player_x, player_y, *oam_id, current_seq[anim_frame]);
}





// Get the tile index at a specific (x, y) pixel coordinate
// Returns the corresponding tile index from the nametable.
unsigned char get_tile_at(unsigned char x, unsigned char y) {
    // Convert pixel coordinates to tile coordinates (8x8 tiles)
    unsigned char tile_x = x / TILE_SIZE;
    unsigned char tile_y = y / TILE_SIZE;

    // Return the tile index from the nametable
    return hollow_nes_nt_1[tile_x + (tile_y * NAMETABLE_WIDTH)];
}


// Check for collision at a specific (x, y) coordinate
// Determines if the tile at the given coordinates is solid based on collision properties.
bool check_collision(int new_x, int new_y) {
    // Get the tile index from the nametable
    unsigned char tile_index = get_tile_at(new_x, new_y);  // Get tile index at new position
    
    // Check the collision properties for that tile
    return (collision_properties[tile_index] == 1);      // Return true if the tile is solid
}



// Check if the player's bounding box collides with solid tiles
// Checks all four corners of the player's sprite for collisions.
bool check_collision_for_player(int new_x, int new_y) {
    return check_collision(new_x, new_y) ||             // top-left
           check_collision(new_x + 15, new_y) ||        // top-right
           check_collision(new_x, new_y + 16) ||        // bottom-left
           check_collision(new_x + 15, new_y + 16);     // bottom-right
}






  


// Main game loop
void main(void) {
  char oam_id = 0;
  unsigned char anim_frame = 0;

  // Load background
  vram_adr(NAMETABLE_A);
  vram_write(hollow_nes_nt_1, 1024);

  // Initialize graphics and player
  setup_graphics();

  initialize_player();

  // Game loop
  while (1) {
    oam_id = 0; // Reset sprite OAM ID

    // Update player movement and state
    update_player();

    // Animate player
    animate_player(&oam_id, &anim_frame);

    // Hide unused sprites
    if (oam_id != 0) oam_hide_rest(oam_id);

    // Wait for next frame
    ppu_wait_frame();
  }
  
}
