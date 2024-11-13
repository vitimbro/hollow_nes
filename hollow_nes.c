//------------------------------------------------------------------------------//
//               Hollow NES - A Demake of Hollow Knight for the NES             //
//------------------------------------------------------------------------------//


// Standard Libraries
#include <stdlib.h>
#include <string.h>

// NES-Specific Libraries
#include "neslib.h"    // NES Library with useful NES functions
#include <nes.h>       // CC65 NES Header (PPU definitions)

// Arithmetic and VRAM Utilities
#include "bcd.h"       // BCD arithmetic support
//#link "bcd.c"
#include "vrambuf.h"   // VRAM update buffer
//#link "vrambuf.c"


// CHR and Nametable Data
//#resource "game_tileset_1.chr"       // Character set (CHR) data
//#link "tileset.s"

// Level Nametables
#include "nametable_game_0.h"
#include "nametable_game_1.h"
#include "nametable_game_2.h"
#include "nametable_game_3.h"
#include "nametable_game_4.h"

// Menu Nametable
#include "nametable_menu.h"

// Death Nametable
#include "nametable_death.h"

// FamiTone Music and Sound Data
//#link "famitone2.s"
void __fastcall__ famitone_update(void);

// Music Tracks
//#link "hollow_knight_theme_data.s"      // Menu song
extern char menu_music_data[];


// Gameplay Music 
//#link "dirtmouth_data.s"
extern char game_music_data[];



// Sound Effects
//#link "sound_effects.s"
extern char sfx_data[];


//------------------------------------------------------------------------------//
//                             CONSTANTS AND DEFINES                            //
//------------------------------------------------------------------------------//


// Screen Dimensions (NES Resolution: 256x240)
#define SCREEN_WIDTH 256                        // Width of the screen in pixels
#define SCREEN_HEIGHT 240                       // Height of the screen in pixels
#define SCREEN_LEFT_EDGE 0                      // Left edge of the screen
#define SCREEN_RIGHT_EDGE (SCREEN_WIDTH - 16)   // Right edge, accounting for sprite width
#define SCREEN_UP_EDGE 0                        // Top edge of the screen
#define SCREEN_DOWN_EDGE (SCREEN_HEIGHT - 16)   // Bottom edge, accounting for sprite height

// Tile and Nametable Constants
#define TILE_SIZE 8                      // Size of each tile in pixels
#define NAMETABLE_WIDTH 32               // Number of tiles per row in the nametable
#define TILE_MASK (TILE_SIZE - 1)        // Mask for tile alignment
#define ALIGN_TO_TILE(x) ((x) & ~TILE_MASK) // Aligns position to nearest TILE_SIZE

// Subpixel Calculations
#define SUBPIXELS 16                     // Subpixels per pixel (for smoother movement)


// Player Position & Movement
#define PLAYER_INIT_X 16                 // Initial X position in pixels
#define PLAYER_INIT_Y 0                  // Initial Y position in pixels
#define PLAYER_ACCELERATION 4            // Acceleration in subpixels
#define PLAYER_DECELERATION 3            // Deceleration when stopping
#define PLAYER_SPEED 32                  // Max speed in subpixels

// Physics Constants
#define GRAVITY 4                        // Gravity applied to the player
#define FALL_GRAVITY 8                  // Increased gravity during falls
#define MAX_FALL_SPEED 96                // Max fall speed in subpixels
#define JUMP_SPEED -96                   // Initial jump velocity
#define JUMP_COOLDOWN 8                  // Frames before jump is allowed again


// Animation Timing Constants
#define ANIM_DELAY_IDLE 32               // Delay between idle frames
#define ANIM_DELAY_RUN 5                 // Delay between run frames
#define ANIM_DELAY_JUMP 1
#define ANIM_DELAY_FALL 1
#define ANIM_DELAY_ATTACK 6
#define ANIM_DELAY_HEAL 14

// Animation Frame Counts
#define IDLE_ANIM_FRAMES 2               // Frames in idle animation
#define RUN_ANIM_FRAMES 3                // Frames in run animation
#define JUMP_ANIM_FRAMES 1
#define FALL_ANIM_FRAMES 1 
#define ATTACK_ANIM_FRAMES 5
#define HEAL_ANIM_FRAMES 6

// Gameplay and Timing Constants
#define STATE_CHANGE_DELAY 10            // Minimum frames between state changes
#define MAX_JUMP_HOLD_TIME 40            // Max frames for holding jump
#define ATTACK_COOLDOWN 36               // Cooldown frames after attack
#define FADE_TIME 6                      // Fade in/out duration in frames

// Game State Definitions
#define STATE_MENU  0
#define STATE_GAME  1
#define STATE_DEATH 2

// Background Scrolling Constants
#define NES_MIRRORING 1
#define SCROLL_SPEED (PLAYER_SPEED / SUBPIXELS)

// Nametable Parameters
#define NUM_GAME_NAMETABLES 5            // Total game nametables
#define NAMETABLE_SIZE 256               // Size of each nametable in pixels

// Life and Soul Constants
#define MAX_LIVES 3                    // Maximum number of lives
#define MAX_SOUL 60                    // Maximum amount of soul
#define SOUL_PER_HIT 10                // Soul gained per hit on enemy
#define SOUL_COST_HEAL 30              // Soul required to heal

// Define collision types
#define COLLISION_NONE 0
#define COLLISION_SOLID 1
#define COLLISION_SPIKE 2
#define COLLISION_INTERACTIVE 3

#define DAMAGE_COOLDOWN 90 // Set cooldown time (frames) between spike damage


#define DIALOGUE_COOLDOWN 30  // Set cooldown for skipping dialogue

#define ARROW_TILE 0x1eb   // Define tile ID for the up arrow sprite
#define ARROW_Y_OFFSET -24  // Position the arrow 8 pixels above the player
#define ARROW_ATTR 2        // Default attribute (e.g., no flipping, palette 0)


// Soul indicators
#define TILE_SOUL_TOP_FULL_1 0xa5
#define TILE_SOUL_TOP_FULL_2 0xa6
#define TILE_SOUL_BOTTOM_FULL_1 0xb5
#define TILE_SOUL_BOTTOM_FULL_2 0xb6

#define TILE_SOUL_TOP_EMPTY_1 0xa9
#define TILE_SOUL_TOP_EMPTY_2 0xaa
#define TILE_SOUL_BOTTOM_EMPTY_1 0xb9
#define TILE_SOUL_BOTTOM_EMPTY_2 0xba


// Lives indicators
#define TILE_MASK_FULL 0xb7
#define TILE_MASK_EMPTY 0xb8

// Define Elder Bug position in nametable 1
#define ELDER_BUG_X 72  // Adjust for center positioning in nametable
#define ELDER_BUG_Y 168   // Adjust for Y-axis positioning


// Metasprites (Define player appearance)
#define DEF_METASPRITE_2x2(name,code,pal) \
    const unsigned char name[]={          \
        0, 0, (code)+0, pal,              \
        8, 0, (code)+1, pal,              \
        0, 8, (code)+2, pal,              \
        8, 8, (code)+3, pal,              \
        128                               \
    };

// Metasprite Horizontal-Flip Definition                        
#define DEF_METASPRITE_2x2_H_FLIP(name,code,pal) \
    const unsigned char name[]={                 \
        8, 0, (code)+0, (pal)|OAM_FLIP_H,       \
        0, 0, (code)+1, (pal)|OAM_FLIP_H,       \
        8, 8, (code)+2, (pal)|OAM_FLIP_H,       \
        0, 8, (code)+3, (pal)|OAM_FLIP_H,       \
        128                                     \
    };

// Metasprite Vertical-Flip Definition  
#define DEF_METASPRITE_2x2_V_FLIP(name,code,pal) \
    const unsigned char name[]={                 \
        8, 0, (code)+3, (pal)|OAM_FLIP_V,       \
        0, 0, (code)+2, (pal)|OAM_FLIP_V,       \
        8, 8, (code)+1, (pal)|OAM_FLIP_V,       \
        0, 8, (code)+0, (pal)|OAM_FLIP_V,       \
        128                                     \
    };

// Metasprites Elder Bug
#define DEF_METASPRITE_2x3(name,code,pal) \
    const unsigned char name[]={          \
        0, 0, (code)+0, pal,              \
        8, 0, (code)+1, pal,              \
        0, 8, (code)+2, pal,              \
        8, 8, (code)+3, pal,              \
        0, 16, (code)+4, pal,              \
        8, 16, (code)+5, pal,              \
        128                               \
    };




//------------------------------------------------------------------------------//
//                              PALETTE setup                                   //
//------------------------------------------------------------------------------//

/*{pal:"nes",layout:"nes"}*/
const char PALETTE[32] = { 
  0x0C,			 // screen color

  0x0F,0x30,0x2D,0x00,	 // background palette 0
  0x0F,0x20,0x03,0x00,	 // background palette 1
  0x0F,0x2D,0x20,0x00,	 // background palette 2
  0x0F,0x2D,0x32,0x00,   // background palette 3

  0x16,0x35,0x24,0x00,	 // sprite palette 0
  0x00,0x37,0x25,0x00,	 // sprite palette 1
  0x0D,0x30,0x22,0x00,	 // sprite palette 2
  0x0D,0x27,0x2A,	 // sprite palette 3
};

//------------------------------------------------------------------------------//
//                            COLLISION TABLE                                   //
//------------------------------------------------------------------------------//


// Define properties for each tile in the tileset (256 possible tiles)
const unsigned char collision_properties[256] = {
 
  // 0 = none, 1 = solid, 2 = spike, 3 = dialogue, 4 = bench
  
 // 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 1
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 2
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 3
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 4
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 5
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, // 6
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, // 7
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, // 8
    0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, // 9
    0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // a
    0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // b
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // c
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // d
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // e
    0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, // f
  
};



//--------------------------------------------------------------------------------//
//                         PLAYER METASPRITES DEFINITIONS                         //
//--------------------------------------------------------------------------------//

//------------------------------- FACING RIGHT -----------------------------------//

DEF_METASPRITE_2x2(player_R_idle_1, 0x100, 2);      // Idle Right 
DEF_METASPRITE_2x2(player_R_idle_2, 0x110, 2);

DEF_METASPRITE_2x2(player_R_run_1, 0x105, 2);       // Running Right 
DEF_METASPRITE_2x2(player_R_run_2, 0x115, 2);
DEF_METASPRITE_2x2(player_R_run_3, 0x125, 2);

DEF_METASPRITE_2x2(player_R_jump, 0x10a, 2);        // Jumping Right
DEF_METASPRITE_2x2(player_R_fall, 0x12a, 2);        // Falling Right

DEF_METASPRITE_2x2(player_R_attack_1, 0x130, 2);    // Attacking Right 
DEF_METASPRITE_2x2(player_R_attack_2, 0x140, 2);
DEF_METASPRITE_2x2(player_R_attack_3, 0x150, 2);

DEF_METASPRITE_2x2(strike_R, 0x145, 2);             // Strike Right

DEF_METASPRITE_2x2(player_R_heal_1, 0x14a, 2);      // Healing Right
DEF_METASPRITE_2x2(player_R_heal_2, 0x15a, 2);
DEF_METASPRITE_2x2(player_R_heal_3, 0x16a, 2);


//------------------------------- FACING LEFT ------------------------------------//

DEF_METASPRITE_2x2_H_FLIP(player_L_idle_1, 0x100, 2);   // Idle Left
DEF_METASPRITE_2x2_H_FLIP(player_L_idle_2, 0x110, 2);

DEF_METASPRITE_2x2_H_FLIP(player_L_run_1, 0x105, 2);    // Running Left
DEF_METASPRITE_2x2_H_FLIP(player_L_run_2, 0x115, 2);
DEF_METASPRITE_2x2_H_FLIP(player_L_run_3, 0x125, 2);

DEF_METASPRITE_2x2_H_FLIP(player_L_jump, 0x10a, 2);     // Jumping Left
DEF_METASPRITE_2x2_H_FLIP(player_L_fall, 0x12a, 2);     // Falling Left

DEF_METASPRITE_2x2_H_FLIP(player_L_attack_1, 0x130, 2); // Attacking Left
DEF_METASPRITE_2x2_H_FLIP(player_L_attack_2, 0x140, 2);
DEF_METASPRITE_2x2_H_FLIP(player_L_attack_3, 0x150, 2);

DEF_METASPRITE_2x2_H_FLIP(strike_L, 0x145, 2);          // Strike Left

DEF_METASPRITE_2x2_H_FLIP(player_L_heal_1, 0x14a, 2);   // Healing Left
DEF_METASPRITE_2x2_H_FLIP(player_L_heal_2, 0x15a, 2);
DEF_METASPRITE_2x2_H_FLIP(player_L_heal_3, 0x16a, 2);


//------------------------------- FACING UP --------------------------------------//

DEF_METASPRITE_2x2(player_U_attack, 0x19a, 2);          // Attacking Up

DEF_METASPRITE_2x2(strike_U, 0x165, 2);                 // Strike Up


//------------------------------- FACING DOWN ------------------------------------//

DEF_METASPRITE_2x2(player_D_attack, 0x18a, 2);          // Attack Down

DEF_METASPRITE_2x2_V_FLIP(strike_D, 0x165, 2);          // Strike Down


//--------------------------------------------------------------------------------//
//                         MOBS AND NPCS METASPRITES DEFINITIONS                         //
//--------------------------------------------------------------------------------//


DEF_METASPRITE_2x3(elder_bug_idle_1, 0x1d0, 2);
DEF_METASPRITE_2x3(elder_bug_idle_2, 0x1e0, 2);


//----------------------------------------------------------------------------------------//
//                               PLAYER ANIMATION SEQUENCES                               //
//----------------------------------------------------------------------------------------//

// Idle sequences
const unsigned char* const player_L_idle_seq[IDLE_ANIM_FRAMES] = { player_L_idle_1, player_L_idle_2 };
const unsigned char* const player_R_idle_seq[IDLE_ANIM_FRAMES] = { player_R_idle_1, player_R_idle_2 };

// Run sequences
const unsigned char* const player_L_run_seq[RUN_ANIM_FRAMES] = { player_L_run_1, player_L_run_2, player_L_run_3 };
const unsigned char* const player_R_run_seq[RUN_ANIM_FRAMES] = { player_R_run_1, player_R_run_2, player_R_run_3 };

// Jump sequences
const unsigned char* const player_L_jump_seq[JUMP_ANIM_FRAMES] = { player_L_jump };
const unsigned char* const player_R_jump_seq[JUMP_ANIM_FRAMES] = { player_R_jump };

// Fall sequences
const unsigned char* const player_L_fall_seq[FALL_ANIM_FRAMES] = { player_L_fall };
const unsigned char* const player_R_fall_seq[FALL_ANIM_FRAMES] = { player_R_fall };

// Attack sequences
const unsigned char* const player_L_attack_seq[ATTACK_ANIM_FRAMES] = { player_L_attack_1, player_L_attack_1, player_L_attack_2, player_L_attack_3, player_L_attack_3 };
const unsigned char* const player_R_attack_seq[ATTACK_ANIM_FRAMES] = { player_R_attack_1, player_R_attack_1, player_R_attack_2, player_R_attack_3, player_R_attack_3 };
const unsigned char* const player_U_attack_seq[ATTACK_ANIM_FRAMES] = { player_R_attack_1, player_R_attack_1, player_U_attack, player_R_attack_3, player_R_attack_3 };
const unsigned char* const player_D_attack_seq[ATTACK_ANIM_FRAMES] = { player_R_attack_1, player_D_attack, player_D_attack, player_R_attack_3, player_R_attack_3 };

// Heal sequences
const unsigned char* const player_L_heal_seq[HEAL_ANIM_FRAMES] = { player_L_heal_1, player_L_heal_2, player_L_heal_1, player_L_heal_2, player_L_heal_3, player_L_heal_3 };
const unsigned char* const player_R_heal_seq[HEAL_ANIM_FRAMES] = { player_R_heal_1, player_R_heal_2, player_R_heal_1, player_R_heal_2, player_R_heal_3, player_R_heal_3 };

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------//

// Elder bug Idle sequence
const unsigned char* const elderbug_idle_seq[IDLE_ANIM_FRAMES] = { elder_bug_idle_1, elder_bug_idle_2 };




//-------------------------------------------------------------------------------------------//
//                                 VARIABLES
//-------------------------------------------------------------------------------------------//


// PLAYER STATE CONSTANTS
typedef enum { 
    STATE_IDLE,    // Player is idle
    STATE_RUN,     // Player is running
    STATE_JUMP,    // Player is jumping
    STATE_FALL,     // Player is falling
    STATE_ATTACK,    // Player is attacking
    STATE_HEAL,       // Player is healing
    STATE_DIALOGUE   // Player is in a dialogue
} PlayerState;


// PLAYER POSITION AND MOVEMENT VARIABLES

// Player position in pixels
byte player_x;                  // Player X position in pixels
byte player_y;                  // Player Y position in pixels

// Player position in subpixels (for smoother movement)
int player_x_sub = 0;     // Player X position in subpixels (1/16 pixel precision)
int player_y_sub = 0;     // Player Y position in subpixels (1/16 pixel precision)

// Player velocity in subpixels
int player_x_vel_sub = 0;   // Player velocity in the X direction (subpixels)
int player_y_vel_sub = 0;   // Player velocity in the Y direction (subpixels)

// Player state variables
bool player_facing_left = false;       // Indicates if the player is facing left
bool player_facing_right = false;      // Indicates if the player is facing right
bool is_on_ground = false;             // Indicates if the player is on the ground
bool can_jump = true;                  // Indicates if the player can jump
bool has_landed = false;               // flag to track landing

// Jump control variables
unsigned char jump_timer = 0;          // Timer for jump cooldown
int jump_hold_timer = 0;               // Timer to control jump height

// Attack mechanics
bool is_attacking = false;
unsigned char attack_timer = 0;

// Healing mechanics
bool is_healing = false;        // True if the player is in a healing state


bool can_interact = false;      // Indicates if the player can interact with background element

// Player animation variables
PlayerState player_state;                          // Current player state 
unsigned char frame = 0;                             // Animation frame counter
unsigned char current_anim_frame_count = 0;          // Number of frames in the current animation sequence
unsigned char anim_delay_counter = 0;                // Delay counter for animation frames
unsigned char current_anim_delay = ANIM_DELAY_IDLE;  // Current animation delay
const unsigned char* const* current_seq;             // Pointer to the current animation sequence
unsigned char anim_frame = 0;

// Frame counters for state changes
int frames_since_last_state_change = 0;  // Frame counter

// Game state variable
unsigned char game_state = STATE_MENU; // Start with menu


// Scrolling and nametable variables
unsigned int scroll_x = 0;
int current_nametable_load = 0; 
int current_nametable_player = 0;

// Array of nametable references
const unsigned char* nametables[] = {
    nametable_game_0,
    nametable_game_1,
    nametable_game_2,
    nametable_game_3,
    nametable_game_4
};


// Global variable to track the current nametable
unsigned char current_nametable_index = 0;


// Attack direction constants
typedef enum { 
    ATTACK_UP,    
    ATTACK_DOWN,     
    ATTACK_LEFT,    
    ATTACK_RIGHT    
} AttackDirection;

unsigned char attack_direction;


// Player Soul and Life Variables
unsigned char player_lives = MAX_LIVES;       // Player's current lives, starts at max
unsigned char player_soul = 0;                // Player's current soul, starts at 0

// Global variables to store previous values
unsigned char previous_soul = 0;
unsigned char previous_lives = 0;

int damage_cooldown = 0;  // Initialize cooldown counter


// Dialogue System

typedef struct {
    const char* text;  // Text to display
    int next;          // Index of the next line (-1 if end)
} Dialogue;

Dialogue dialogues[] = {
    {"HELLO!", 1},
    {"HOW ARE YOU DOING?", 2},
    {"DID YOU KNOW YOU COULD  HEAL BY PRESSING \x1d ?", 3},
    {"YOU CAN ALSO ATTACK BY  PRESSING B.", -1}
};

// Global variables for dialogue
int current_dialogue_index = 0;
bool is_dialogue_active = false;

int dialogue_cooldown = 0; // Add a global or static cooldown variable


unsigned char soul_tile_top_1 = TILE_SOUL_TOP_FULL_1;
unsigned char soul_tile_top_2 = TILE_SOUL_TOP_FULL_2; 
unsigned char soul_tile_bottom_1 = TILE_SOUL_BOTTOM_FULL_1;
unsigned char soul_tile_bottom_2 = TILE_SOUL_BOTTOM_FULL_2;

unsigned char mask_tile_1 = TILE_MASK_FULL;
unsigned char mask_tile_2 = TILE_MASK_FULL;
unsigned char mask_tile_3 = TILE_MASK_FULL;


unsigned char elder_bug_anim_frame = 0;    // Animation frame index for Elder Bug
unsigned char elder_bug_delay_counter = 0; // Frame delay for idle animation


//------------------------------------------------------------------------------------//
//                              FUNCTION PROTOTYPES                                   //
//------------------------------------------------------------------------------------//

// Setup functions
void setup_graphics();
void setup_audio();
void initialize_player();

// Tile and collision detection
unsigned char get_tile_at(unsigned char x, unsigned char y);
unsigned char check_collision(int x, int y);
bool handle_collision(unsigned char collision_type);
bool check_player_horizontal_collision(int new_x, int player_y);
bool check_player_vertical_collision(int new_x, int player_y);
void handle_spike_collision();


// Player update and input handling
void update_player();
void handle_player_input();
void handle_horizontal_movement_input(char pad);
void move_player_left();
void move_player_right();
void stop_horizontal_movement();

// Jump mechanics and physics
void handle_jump_input(char pad);
void player_jump();
void apply_player_physics();
void apply_gravity();
void update_jump_timer();

// Movement handling
void handle_player_movement();
int handle_subpixel_movement(int *subpixel_pos, int velocity);
bool handle_horizontal_collision(int* new_x);
bool handle_vertical_collision(int* new_y);

// Attack mechanics
void handle_attack_input(char pad);
void draw_strike(unsigned char* oam_id);

// Healing mechanics
void handle_heal_input(char pad);
void attempt_heal();

// Interaction mechanics
void handle_interact_input(char pad);

// Player state and animation
void update_player_animation_state();
void set_running_state();
void set_idle_state();
void set_jumping_state();
void set_falling_state();
void set_attacking_state();
void set_healing_state();

// Animation sequence and frame handling
void animate_player(unsigned char* oam_id, unsigned char* anim_frame);
const unsigned char* const* get_animation_sequence();
void update_animation_sequence(const unsigned char* const* new_seq, unsigned char* anim_frame);
void update_animation_frame(unsigned char* anim_frame);
void draw_current_frame(unsigned char* oam_id, unsigned char anim_frame);

// Game state and menu handling
void update_menu();
void setup_menu();
void update_game();
void setup_game();
void update_death();
void setup_death();
void check_game_state();

// Visual effects
void fade_in();
void fade_out();

// Background scrolling and nametable loading
void load_nametable(int index);
void scroll_background();


// Dialogue System
void handle_dialogue();
void load_dialogue_box();
void load_dialogue_page();
void clear_dialogue_box();
void clear_dialogue_page();
void handle_dialogue_input(char pad);
void update_dialogue_cooldown();


void reset_player_position();

void update_interaction_indicator(unsigned char* oam_id);

void load_hud();
void update_soul_indicator();
void update_lives_indicator();
void update_hud();


void load_new_nametable(unsigned char new_nametable_index);
void check_screen_transition();


//---------------------------------------------------------------------------------------//
//                            SETUP AND INITIALIZATION                                   //
//---------------------------------------------------------------------------------------//


// Set up graphics and palette for the game
// setup PPU and tables
void setup_graphics() {
  oam_clear(); // clear sprites from OAM
  pal_all(PALETTE); // set palette colors
  bank_bg(0);
  bank_spr(1);
  ppu_on_all(); // turn on PPU rendering
}


void setup_audio() {
  famitone_init(menu_music_data); // Initialize FamiTone with the menu music by default
  sfx_init(sfx_data);             // Initialize sound effects
  nmi_set_callback(famitone_update); // Set FamiTone update function to be called during NMI
}



// Initialize the player with starting position, speed, and state
// Sets the player to the idle state, facing right.
void initialize_player() {
    player_x = PLAYER_INIT_X;        // Set initial x-position in pixels
    player_y = PLAYER_INIT_Y;        // Set initial y-position in pixels
    
    // Set initial positions in subpixels (multiply by SUBPIXELS)
    player_x_sub = player_x * SUBPIXELS;  
    player_y_sub = player_y * SUBPIXELS;  
    
    player_x_vel_sub = 0;                   // No horizontal movement at start
    player_y_vel_sub = 0;                   // No vertical movement at start
    player_facing_right = true;               // Default to facing right
    player_state = STATE_IDLE;                // Start in the idle state
  
    player_lives = MAX_LIVES;
    player_soul = 60;
}


//------------------------------------------------------------------------------------------//
//                                 PLAYER MOVEMENT FUNCTIONS                                //
//------------------------------------------------------------------------------------------//


// Update player input, physics, position, and state
void update_player() {
    handle_player_input();            // Separate input handling
    apply_player_physics();           // Apply gravity and physics
    handle_player_movement();         // Check collisions and move player
    update_player_animation_state();  // Update player animation state
  
    if (damage_cooldown > 0) {
        damage_cooldown--;  // Reduce cooldown counter each frame
    }
  
}


//-----------------------------------------------------------------------//
//                        Handle player Input                            //
//-----------------------------------------------------------------------//


void handle_player_input() {
    char pad = pad_poll(0);      
  
    // If the player is healing, skip movement and attack inputs
    if (is_healing) return;
    if (is_dialogue_active) {
      handle_dialogue_input(pad);
      return;
    }
      
    handle_horizontal_movement_input(pad); // Use current poll state for movement
    handle_jump_input(pad);                // Use current poll state for jump
    handle_attack_input(pad);     
    handle_heal_input(pad);
    handle_interact_input(pad);
  
}

//------------------------------- Jump Input ------------------------------------//

void handle_jump_input(char pad) { 
    if (is_on_ground && (pad & PAD_A) && can_jump) {
        player_jump();  // Initiate jump
        sfx_play(1,1);
    }

    // If the jump button is being held and the player is jumping
    if ((pad & PAD_A) && !is_on_ground && jump_hold_timer > 0) {
        jump_hold_timer--;  // Continue holding jump for a higher jump
    } else {
        jump_hold_timer = 0;  // Stop holding if button is released
    }
}

//------------------------------- Movement Input ------------------------------------//

void handle_horizontal_movement_input(char pad) {
    if (pad & PAD_LEFT) {
        move_player_left();
    } else if (pad & PAD_RIGHT) {
        move_player_right();
    } else {
        stop_horizontal_movement();  
    }
}

//------------------------------- Attack Input ------------------------------------//

void handle_attack_input(char pad) {
    // Start an attack if B is pressed, player is not already attacking, and cooldown has finished
    if ((pad & PAD_B) && !is_attacking && attack_timer == 0) {
        is_attacking = true;
        attack_timer = ATTACK_COOLDOWN;  // Start cooldown
        sfx_play(0,0);
      
        // Set the attack direction
        if (pad & PAD_UP) {
            attack_direction = ATTACK_UP;
        } else if ((pad & PAD_DOWN) && !is_on_ground) {
            attack_direction = ATTACK_DOWN;
        } else {
            attack_direction = player_facing_right ? ATTACK_RIGHT : ATTACK_LEFT;
        }
    }
  
    // Decrement cooldown timer
    if (attack_timer > 0) {
        attack_timer--;
    }
}

//------------------------------- Heal Input ------------------------------------//


void handle_heal_input(char pad) {
    // Check if down pad is held, player is idle, and they have less than max lives
    if ((pad & PAD_DOWN) && player_state == STATE_IDLE && player_lives < MAX_LIVES) {
        attempt_heal();  // Try to heal the player
    }
}

//------------------------------- Interact Input ------------------------------------//


void handle_interact_input(char pad) {
    // Check if the Up button is pressed and player is idle
    if (pad & PAD_UP && player_state == STATE_IDLE && can_interact) {
        player_state = STATE_DIALOGUE;
        is_dialogue_active = true;
        handle_dialogue();  // Open the dialogue box
    }
}


//-----------------------------------------------------------------------//
//                        Handle Healing                                 //
//-----------------------------------------------------------------------//

void attempt_heal() {
    // Check if player has enough soul to heal
    if (player_soul >= SOUL_COST_HEAL) {
        is_healing = true;              // Enter healing state
        player_soul -= SOUL_COST_HEAL;  // Deduct soul
        player_lives++;                 // Increase lives by one
        sfx_play(0, 0);                 // Play healing sound
        set_healing_state();            // Update player to healing state
        player_x_vel_sub = 0;           // Stop player movement
        player_y_vel_sub = 0;           // Stop vertical movement
    } else {
        // Optional: Play "not enough soul" sound or animation
    }
}

//-----------------------------------------------------------------------//
//                        Horizontal Movement                            //
//-----------------------------------------------------------------------//


// Function to move the player to the left
void move_player_left() {
//      // Check if the player is already at the leftmost boundary
//    if (player_x <= SCREEN_LEFT_EDGE) {
//        player_x_vel_sub = 0;  // Stop further leftward movement
//        return;  // Exit function early if at the boundary
//    }
    // Increase negative velocity (moving left) if it doesn't exceed the max speed
    if (player_x_vel_sub > -PLAYER_SPEED) {
        player_x_vel_sub -= PLAYER_ACCELERATION;  // Accelerate left by subtracting subpixels
        if (player_x_vel_sub < -PLAYER_SPEED) {
            player_x_vel_sub = -PLAYER_SPEED;  // Clamp the velocity to -MAX_SPEED to prevent overshooting
        }
    }
   
    // Update player's facing direction
    player_facing_right = false;
    player_facing_left = true;
}

// Function to move the player to the right
void move_player_right() {
//  if (player_x >= SCREEN_RIGHT_EDGE) {
//        player_x_vel_sub = 0;  // Stop further leftward movement
//        return;  // Exit function early if at the boundary
//    }
    // Increase positive velocity (moving right) if it doesn't exceed the max speed
    if (player_x_vel_sub < PLAYER_SPEED) {
        player_x_vel_sub += PLAYER_ACCELERATION;  // Accelerate right by adding subpixels
        if (player_x_vel_sub > PLAYER_SPEED) {
            player_x_vel_sub = PLAYER_SPEED;  // Clamp the velocity to MAX_SPEED to prevent overshooting
        }
    }
    // Update player's facing direction
    player_facing_left = false;
    player_facing_right = true;
}

void stop_horizontal_movement() {
    if (player_x_vel_sub > 0) {
        player_x_vel_sub -= PLAYER_DECELERATION;
        if (player_x_vel_sub < 0) player_x_vel_sub = 0;  // Evita valores negativos
    } else if (player_x_vel_sub < 0) {
        player_x_vel_sub += PLAYER_DECELERATION;
        if (player_x_vel_sub > 0) player_x_vel_sub = 0;  // Evita valores positivos
    }
}


//---------------------------------------------------------------------//
//                        Vertical Movement                            //
//---------------------------------------------------------------------//

void player_jump() {
    player_y_vel_sub = JUMP_SPEED;
    is_on_ground = false;
    can_jump = false;
    jump_hold_timer = MAX_JUMP_HOLD_TIME;
}

// PHYSICS
void apply_player_physics() {
    apply_gravity();    // Refactored gravity logic
    update_jump_timer(); // Handle jump cooldowns
  
}

// Gravity application logic
void apply_gravity() {
    // Check if the jump button is released or the hold time is over
    if (jump_hold_timer == 0) {
        // Apply increased gravity when falling
        player_y_vel_sub += FALL_GRAVITY;
    } else {
        // Apply normal gravity when holding jump
        player_y_vel_sub += GRAVITY;
    }

    // Cap the fall speed
    if (player_y_vel_sub > MAX_FALL_SPEED) {
        player_y_vel_sub = MAX_FALL_SPEED;
    }
}

void update_jump_timer() {
    if (jump_timer > 0) {
        jump_timer--;
        if (jump_timer == 0) {
            can_jump = true;  // Allow jumping once cooldown has expired
        }
    }
}

//-----------------------------------------------------------------//
//                        Handle Movement                          //
//-----------------------------------------------------------------//


// Function to handle subpixel arithmetic
int handle_subpixel_movement(int *player_pos_sub, int player_vel_sub) {
    int pixel_movement = 0;

    // Update the subpixel position by adding the velocity (in subpixels)
    *player_pos_sub += player_vel_sub;

    // Convert subpixels to full pixels when it exceeds 16 subpixels (i.e., 1 pixel)
    while (abs(*player_pos_sub) >= 16) {
        if (*player_pos_sub > 0) {
            pixel_movement++;
            *player_pos_sub -= 16;
        } else {
            pixel_movement--;
            *player_pos_sub += 16;
        }
    }

    return pixel_movement;  // Return the number of pixels the player should move
}

void handle_player_movement() {
    // Declare new temporary X and Y variables to store the player's updated position
    int new_x = player_x;
    int new_y = player_y;

    // Booleans to check if the player collides horizontally or vertically
    bool collided_horizontally;
    bool collided_vertically;

    // Handle subpixel movement for both X and Y axes
    new_x += handle_subpixel_movement(&player_x_sub, player_x_vel_sub);
    new_y += handle_subpixel_movement(&player_y_sub, player_y_vel_sub);

    // Check for collisions horizontally and vertically before updating the player's position
    collided_horizontally = handle_horizontal_collision(&new_x);
    collided_vertically = handle_vertical_collision(&new_y);

    // Only update the player's pixel position if there is no collision
    if (!collided_horizontally) {
        player_x = new_x;  // Update X position if no horizontal collision
    }
    if (!collided_vertically) {
        player_y = new_y;  // Update Y position if no vertical collision
    }
}

//---------------------------------------------------------------------------------//
//                        Handle Player Collision                                  //
//---------------------------------------------------------------------------------//


// Main collision handling function
bool handle_collision(unsigned char collision_type) {
    if (collision_type == COLLISION_SOLID) {
        // Solid object - stop movement
        return true;
    } 
    
    if (collision_type == COLLISION_SPIKE) {
        handle_spike_collision();
    } 
    
    if (collision_type == COLLISION_INTERACTIVE) {
        can_interact = true;
        // Display the up arrow sprite above the player
        oam_spr(player_x, player_y + ARROW_Y_OFFSET, ARROW_TILE, 0, 0); // Position and display the arrow
    }
    
    if (check_collision(player_x, player_y) != COLLISION_INTERACTIVE) {
        can_interact = false;
    }
  
    return false;
}

// Handle spike collision - reduce player life with cooldown check
void handle_spike_collision() {
    if (damage_cooldown == 0) { // Only damage if cooldown has elapsed
        player_lives--;
        sfx_play(0,0);
        if (player_lives > 0) {
            fade_out();
            reset_player_position(); // Reset player position if needed
            damage_cooldown = DAMAGE_COOLDOWN; // Start cooldown
            fade_in();
        } else {
            // Player death/game over state
            game_state = STATE_DEATH;
            fade_out();
            setup_death();
            fade_in();
        }
    }
}


bool handle_horizontal_collision(int* new_x) {
    if (check_player_horizontal_collision(*new_x, player_y)) {
        // Ajusta a posição com base na direção do movimento
      
        if (player_x_vel_sub > 0) {
            player_x = ALIGN_TO_TILE(*new_x);  // Ajuste fino para colisão na direita
        } else {
            player_x = ALIGN_TO_TILE(*new_x) + TILE_SIZE;  // Ajuste fino para colisão na esquerda
        }
      
        player_x_vel_sub = 0;  // Para o movimento horizontal
        return true;    // Colisão ocorreu
    }
    return false;  // Sem colisão
}


bool handle_vertical_collision(int* new_y) {
    if (check_player_vertical_collision(player_x, *new_y)) {
        if (player_y_vel_sub >= 0) {  
            player_y = ALIGN_TO_TILE(*new_y);  
            is_on_ground = true;

            // Set cooldown only when landing for the first time
            if (!has_landed) {
                jump_timer = JUMP_COOLDOWN;
                can_jump = false;  // Prevent jumping immediately
                has_landed = true; // Mark as landed
            }
        } else {  
            player_y = ALIGN_TO_TILE(*new_y) + TILE_SIZE;  
        }
        player_y_vel_sub = 0;  
        return true;
    }

    // Player is in the air, reset landing flag
    if (player_y_vel_sub != 0) {
        is_on_ground = false;
        has_landed = false;  // Reset landing flag when airborne
    }

    return false;
}

// Reset player position function for reuse in multiple conditions
void reset_player_position() {
   // to-do (or not)
}


//------------------------------------------------------------------------------------------//
//                                 COLLISION FUNCTIONS                                      //
//------------------------------------------------------------------------------------------//


// Get the tile index at a specific (x, y) pixel coordinate
// Returns the corresponding tile index from the nametable.
unsigned char get_tile_at(unsigned char x, unsigned char y) {
    // Convert pixel coordinates to tile coordinates (8x8 tiles)
    unsigned char tile_x = x / TILE_SIZE;
    unsigned char tile_y = y / TILE_SIZE;

    // Return the tile index from the nametable
    return nametables[current_nametable_index][tile_x + (tile_y * NAMETABLE_WIDTH)];
}


// Checks the collision type at a specific (x, y) coordinate
unsigned char check_collision(int x, int y) {
    unsigned char tile_index = get_tile_at(x, y);
    return collision_properties[tile_index];
}

// Check if the player's bounding box collides with solid tiles
// Checks all corners of the player's sprite for collisions.


// Check if the player's bounding box collides with different types of tiles
bool check_player_horizontal_collision(int new_x, int player_y) {
    bool is_solid_collision = false;
    
    // Check all relevant corners for horizontal collision
    unsigned char collision_type = check_collision(new_x + 6, player_y + 4);
    if (collision_type != COLLISION_NONE) {
        handle_collision(collision_type);
        if (collision_type == COLLISION_SOLID) is_solid_collision = true;
    }

    // Additional corners
    collision_type = check_collision(new_x + 10, player_y + 4);
    if (collision_type != COLLISION_NONE) {
        handle_collision(collision_type);
        if (collision_type == COLLISION_SOLID) is_solid_collision = true;
    }

    collision_type = check_collision(new_x + 6, player_y + 15);
    if (collision_type != COLLISION_NONE) {
        handle_collision(collision_type);
        if (collision_type == COLLISION_SOLID) is_solid_collision = true;
    }

    collision_type = check_collision(new_x + 10, player_y + 15);
    if (collision_type != COLLISION_NONE) {
        handle_collision(collision_type);
        if (collision_type == COLLISION_SOLID) is_solid_collision = true;
    }

    return is_solid_collision;
}



bool check_player_vertical_collision(int player_x, int new_y) {
    bool is_solid_collision = false;

    // Check all relevant corners for vertical collision
    unsigned char collision_type = check_collision(player_x + 6, new_y + 4);
    if (collision_type != COLLISION_NONE) {
        handle_collision(collision_type);
        if (collision_type == COLLISION_SOLID) is_solid_collision = true;
    }

    collision_type = check_collision(player_x + 8, new_y + 4);
    if (collision_type != COLLISION_NONE) {
        handle_collision(collision_type);
        if (collision_type == COLLISION_SOLID) is_solid_collision = true;
    }

    collision_type = check_collision(player_x + 6, new_y + 16);
    if (collision_type != COLLISION_NONE) {
        handle_collision(collision_type);
        if (collision_type == COLLISION_SOLID) is_solid_collision = true;
    }

    collision_type = check_collision(player_x + 8, new_y + 16);
    if (collision_type != COLLISION_NONE) {
        handle_collision(collision_type);
        if (collision_type == COLLISION_SOLID) is_solid_collision = true;
    }

    return is_solid_collision;
}


//-----------------------------------------------------------------------------//
//                        Handle Player State                                  //
//-----------------------------------------------------------------------------//


// STATE AND ANIMATION
void update_player_animation_state() {
    // Increment the frame counter
    frames_since_last_state_change++;

    // Only allow state change if enough frames have passed
    if (frames_since_last_state_change < STATE_CHANGE_DELAY) {
        return;  // Do not change state until the delay threshold is reached
    }
  
    // Check if player is in the air
    if (!is_on_ground) {
        // Only change to jumping state if player is moving upwards
        if (player_y_vel_sub < 0) {
            if (player_state != STATE_JUMP) {
                set_jumping_state();   // Player is jumping (moving upward)
                frames_since_last_state_change = 0;  // Reset counter
            }
        } 
        // Change to falling state if player is moving downwards
        else if (player_y_vel_sub > 0) {
            if (player_state != STATE_FALL) {
                set_falling_state();   // Player is falling (moving downward)
                frames_since_last_state_change = 0;  // Reset counter
            }
        }
    } 
    // Check if player is on the ground
    else {
        // Change to running if player is moving horizontally
        if (player_x_vel_sub != 0) {
            if (player_state != STATE_RUN) {
                set_running_state();  // Running if moving horizontally
                frames_since_last_state_change = 0;  // Reset counter
            }
        } 
        // Change to idle if player is not moving horizontally
        else {
            if (player_state != STATE_IDLE) {
                set_idle_state(); // Idle if not moving
                frames_since_last_state_change = 0;  // Reset counter
            }
        }
    }
  
    if (is_attacking) {
        set_attacking_state();
        frames_since_last_state_change = 0;  // Reset frames counter for the next state
    }
  
    // Check if player is healing
    if (is_healing) {
        set_healing_state();
        frames_since_last_state_change = 0;  // Reset frames counter for the next state
    }
   
  
}

//------------------------------- Set Player States ------------------------------------//

void set_running_state() {
    player_state = STATE_RUN;
    current_anim_delay = ANIM_DELAY_RUN;
    current_anim_frame_count = RUN_ANIM_FRAMES;
}

void set_idle_state() {
    player_state = STATE_IDLE;
    current_anim_delay = ANIM_DELAY_IDLE;
    current_anim_frame_count = IDLE_ANIM_FRAMES;
}

void set_jumping_state() {
    player_state = STATE_JUMP;
    current_anim_delay = ANIM_DELAY_JUMP;
    current_anim_frame_count = JUMP_ANIM_FRAMES;
}

void set_falling_state(){
    player_state = STATE_FALL;
    current_anim_delay = ANIM_DELAY_FALL;
    current_anim_frame_count = FALL_ANIM_FRAMES;
}

void set_attacking_state(){
    player_state = STATE_ATTACK;
    current_anim_delay = ANIM_DELAY_ATTACK;
    current_anim_frame_count = ATTACK_ANIM_FRAMES;
}

void set_healing_state(){
    player_state = STATE_HEAL;
    current_anim_delay = ANIM_DELAY_HEAL;
    current_anim_frame_count = HEAL_ANIM_FRAMES;
}



//------------------------------------------------------------------------------------------//
//                                 PLAYER ANIMATION FUNCTIONS                               //
//------------------------------------------------------------------------------------------//

// Main function to handle player animation
void animate_player(unsigned char* oam_id, unsigned char* anim_frame) {
    const unsigned char* const* new_seq = get_animation_sequence();  // Get current animation sequence
    update_animation_sequence(new_seq, anim_frame);   // Update animation sequence and frame
    
    // Show strike at the start of the attack animation
    if (is_attacking && *anim_frame > 1) {
        draw_strike(oam_id);  // Draw the strike at the start of the attack animation
    }
    
    update_animation_frame(anim_frame);  // Handle frame updates based on delay
    draw_current_frame(oam_id, *anim_frame);  // Draw the current frame

    // End the attack after all animation frames have displayed
    if (is_attacking && *anim_frame == ATTACK_ANIM_FRAMES - 1) {
        is_attacking = false;  // Stop attacking after the animation finishes
    }
  
    // End the attack after all animation frames have displayed
    if (is_healing && *anim_frame == HEAL_ANIM_FRAMES - 1) {
        is_healing = false;  // Stop attacking after the animation finishes
    }
}


// Select the animation sequence based on player's state and direction
const unsigned char* const* get_animation_sequence() {
    if (player_state == STATE_RUN) {
        return player_facing_right ? player_R_run_seq : player_L_run_seq;  // Running right or left
    } 
    else if (player_state == STATE_IDLE) {
        return player_facing_right ? player_R_idle_seq : player_L_idle_seq;  // Idle right or left
    }
    
    else if (player_state == STATE_JUMP) {
        return player_facing_right ? player_R_jump_seq : player_L_jump_seq;  // Jumping right or left
    }
    else if (player_state == STATE_FALL) {
        return player_facing_right ? player_R_fall_seq : player_L_fall_seq;  // Falling right or left
    }
    else if (player_state == STATE_ATTACK) {
        switch (attack_direction) {
            case ATTACK_UP:    return player_U_attack_seq;
            case ATTACK_DOWN:  return player_D_attack_seq;
            case ATTACK_RIGHT: return player_R_attack_seq;
            case ATTACK_LEFT:  return player_L_attack_seq;
        }
    }
    else if (player_state == STATE_HEAL) {
        return player_facing_right ? player_R_heal_seq : player_L_heal_seq;  // Healing right or left
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




void draw_strike(unsigned char* oam_id) {
    int strike_x = player_x;
    int strike_y = player_y;

    switch (attack_direction) {
        case ATTACK_UP:
            strike_y -= 16;  // Adjust Y for upward strike
            *oam_id = oam_meta_spr(strike_x, strike_y, *oam_id, strike_U);
            break;
        case ATTACK_DOWN:
            strike_y += 16;  // Adjust Y for downward strike
            *oam_id = oam_meta_spr(strike_x, strike_y, *oam_id, strike_D);
            break;
        case ATTACK_RIGHT:
            strike_x += 13;
            *oam_id = oam_meta_spr(strike_x, strike_y, *oam_id, strike_R);
            break;
        case ATTACK_LEFT:
            strike_x -= 13;
            *oam_id = oam_meta_spr(strike_x, strike_y, *oam_id, strike_L);
            break;
    }
}

//---------------------------------------------------------------------------------------//

// Function to animate the Elder Bug
void animate_elder_bug(unsigned char* oam_id) {
    // Idle animation update
    if (elder_bug_delay_counter == 0) {
        elder_bug_anim_frame = (elder_bug_anim_frame + 1) % IDLE_ANIM_FRAMES;
    }
    elder_bug_delay_counter = (elder_bug_delay_counter + 1) % ANIM_DELAY_IDLE;

    // Draw the current Elder Bug frame in a fixed position
    *oam_id = oam_meta_spr(ELDER_BUG_X, ELDER_BUG_Y, *oam_id, elderbug_idle_seq[elder_bug_anim_frame]);
}

//---------------------------------------------------------------------------------------//
//                             GAME STATE FUNCTIONS                                      //
//---------------------------------------------------------------------------------------//


//------------------------------- Setup States ------------------------------------//

// Load the nametable for the menu state
void setup_menu() {
  
  famitone_init(menu_music_data); // Initialize menu music 
  ppu_off(); // Turn off rendering to safely update VRAM
  vram_adr(NAMETABLE_A);
  vram_unrle(nametable_menu);
  ppu_on_all(); // Turn rendering back on
  
  music_play(0); // Play the menu music
}

// Load the nametable for the game state
void setup_game() {
  ppu_off(); // Turn off rendering to safely update VRAM
  vram_adr(NAMETABLE_A);
  vram_write(nametable_game_0, 1024);
  vram_adr(NAMETABLE_B);
  vram_write(nametable_game_1, 1024);
  ppu_on_all(); // Turn rendering back on
  
  music_play(0); // Play the gameplay music
}

// Load the nametable for the death state
void setup_death() {
    ppu_off(); // Turn off rendering to safely update VRAM
    vram_adr(NAMETABLE_A);
    vram_unrle(nametable_death); // Load the death screen nametable
    oam_clear();  // Clear all sprites
    ppu_on_all(); // Turn rendering back on
    music_stop(); // Play death or game over music
}


//------------------------------- Update States ------------------------------------//

// Handle the menu state
void update_menu() {
  // Wait for Start button to begin the game
  if (pad_trigger(0) & PAD_START) {
    fade_out(); // Fade out before changing the state
    music_stop();                 // Stop menu music
    delay(60);
    game_state = STATE_GAME; // Switch to game state
    famitone_init(game_music_data); // Initialize gameplay music
    setup_game(); // Load game nametable
    load_hud();
    fade_in(); // Fade in after loading the new state
    initialize_player();
  }
}

// Handle the game state
void update_game() {
  char oam_id = 0; // Reset sprite OAM ID

  // Update player movement and state
  update_player();

  // Animate player
  animate_player(&oam_id, &anim_frame);
  
  update_interaction_indicator(&oam_id);  // Update the interaction arrow indicator
  
  // Draw Elder Bug
  if (current_nametable_index == 0) {  // Only draw if player is in nametable 1
      animate_elder_bug(&oam_id);
  }
  
  update_hud();
  
  check_screen_transition();

  // Hide unused sprites
  if (oam_id != 0) oam_hide_rest(oam_id);
}


// Handle the death state
void update_death() {
    // Wait for Start button to return to the main menu
    if (pad_trigger(0) & PAD_START) {
        fade_out(); // Fade out before changing the state
        music_stop(); // Stop death music
        game_state = STATE_MENU; // Switch back to menu
        setup_menu(); // Load menu nametable
        fade_in(); // Fade in after loading the menu
    }
}

//------------------------------- Check State ------------------------------------//

// Check the current game state and update accordingly
void check_game_state() {
    if (game_state == STATE_MENU) {
        update_menu();
    } else if (game_state == STATE_GAME) {
        update_game();
    } else if (game_state == STATE_DEATH) {
        update_death();
    }
}
//-----------------------------------------------------------------------------//
//                        Fade In/Out Functions                                //
//-----------------------------------------------------------------------------//


void fade_in() {
    char i;
    for (i = 0; i <= 4; ++i) {
        pal_bright(i); // Increase brightness
        delay(FADE_TIME); // Wait for next frame
    }
}
int fade_done = 0;
void fade_out() {
    signed char i;
    fade_done = 0;
    for (i = 4; i >= 0; --i) {
        pal_bright(i); // Decrease brightness
        delay(FADE_TIME); // Wait for next frame to smooth the transition
    }
  fade_done = 1;
}




//---------------------------------------------------------------------------------------//
//                             SCROLL FUNCTIONS                                          //
//---------------------------------------------------------------------------------------//

/*

// Function to load a nametable based on index and VRAM location
void load_nametable(int index) {
    if (index >= 0 && index < NUM_GAME_NAMETABLES) {
        ppu_off();
      	if (index % 2 == 0){
        	vram_adr(NAMETABLE_A);
        	vram_write(nametables[index], 1024);
        }else{
          	vram_adr(NAMETABLE_B);
        	vram_write(nametables[index], 1024);
        }
        ppu_on_all();
    }
}

void scroll_background() {
    char pad = pad_poll(0);  // Get input from controller 0 (first player)
    
    // Scroll right: check if moving right and within the scroll limit
    if ((pad & PAD_RIGHT) &&   (player_x >= SCREEN_WIDTH / 2)&&(scroll_x < NAMETABLE_SIZE * (NUM_GAME_NAMETABLES - 1))) {
        scroll_x += PLAYER_SPEED / SUBPIXELS;
        player_x = SCREEN_WIDTH / 2;  // Keep player centered after scrolling

        // Load the next nametable if crossing into a new nametable
        if (scroll_x >= NAMETABLE_SIZE * (current_nametable_load) && (current_nametable_load + 1 < NUM_GAME_NAMETABLES)) {
            current_nametable_load++;  // Move to the next nametable
            load_nametable(current_nametable_load);  // Load into NAMETABLE_B
        }
     
     
    }

    // Scroll left: check if moving left and within the scroll limit
    if ((pad & PAD_LEFT) && (player_x <= SCREEN_WIDTH / 2) && (scroll_x > 0)) {
        scroll_x -= PLAYER_SPEED / SUBPIXELS;
        player_x = SCREEN_WIDTH / 2;  // Keep player centered after scrolling

      	
        // Load the previous nametable if crossing into a previous boundary
        if (scroll_x < NAMETABLE_SIZE * current_nametable_load && current_nametable_load > 0) {
            current_nametable_load--;  // Move to the previous nametable
            load_nametable(current_nametable_load);  // Load into NAMETABLE_A
        }
    }


    // Apply the scroll to the PPU
    scroll(scroll_x, 0);  // Apply horizontal scroll
}

*/


// Function to change the current nametable
void load_new_nametable(unsigned char new_nametable_index) {
    // Fade out the screen
    fade_out();
    
    // Update the current nametable index
    current_nametable_index = new_nametable_index;

    // Set VRAM address to start writing nametable
    ppu_off();
    vram_adr(NAMETABLE_A);
    vram_write(nametables[current_nametable_index], 1024);
    ppu_on_all();
  
    load_hud();

    // Fade in the screen
    fade_in();
}

// Function to check for screen boundaries and load new nametable if needed
void check_screen_transition() {
    // Check if player is at screen boundaries (left or right)
    if (player_x <= 1) {
        // Move to the left nametable
        if (current_nametable_index != 0) {
            player_x = SCREEN_RIGHT_EDGE - 1;  // Reposition player on right side
            load_new_nametable(current_nametable_index - 1);
            
        } else {
            player_x = 1;  // Keep player within screen if no nametable on the left
        }
    } else if (player_x >= SCREEN_RIGHT_EDGE) {
        // Move to the right nametable
        if (current_nametable_index < NUM_GAME_NAMETABLES - 1) {
            player_x = 1;  // Reposition player on left side
            load_new_nametable(current_nametable_index + 1);
            
        } else {
            player_x = SCREEN_RIGHT_EDGE;  // Keep player within screen if no nametable on the right
        }
    }
}



//------------------------------------------------------------------------------------//
//                              Dialogue Box Function                                 //
//------------------------------------------------------------------------------------//


void handle_dialogue(){
        char pad = pad_poll(0);
	load_dialogue_box();
        clear_dialogue_page();
  	load_dialogue_page();     
}

void load_dialogue_box() {
  
    // write text to name table
    ppu_off();
  
    vram_adr(NTADR_A(2,2));		// set address
    vram_write("\x6b", 1);	        // write bytes to video RAM
    vram_fill(0x7a, 26);
    vram_write("\x6c", 1);
  
    vram_adr(NTADR_A(2,3));
    vram_write("\x6a", 1);
    vram_adr(NTADR_A(29,3));
    vram_write("\x6a", 1);
  
    vram_adr(NTADR_A(2,4));
    vram_write("\x6a", 1);
    vram_adr(NTADR_A(29,4));
    vram_write("\x6a", 1);
    
    vram_adr(NTADR_A(2,5));
    vram_write("\x6a", 1);
    vram_adr(NTADR_A(29,5));
    vram_write("\x6a", 1);
  
    vram_adr(NTADR_A(2,6));
    vram_write("\x6a", 1);
    vram_adr(NTADR_A(29,6));
    vram_write("\x6a", 1);
  
    vram_adr(NTADR_A(2,7));
    vram_write("\x6a", 1);
    vram_adr(NTADR_A(29,7));
    vram_write("\x6a", 1);
  
    vram_adr(NTADR_A(2,8));		// set address
    vram_write("\x7b", 1);	        // write bytes to video RAM
    vram_fill(0x7a, 26);
    vram_write("\x7c", 1);
  
    ppu_on_all();

}


// Function to handle dialogue pages
void load_dialogue_page() {
    const char* text = dialogues[current_dialogue_index].text;
    int text_length = strlen(text);
    int line_length = 0;
  
    ppu_off();

    // Set VRAM address to start of dialogue box
    vram_adr(NTADR_A(4, 4));

    // Write up to the first 24 characters on the first line
    line_length = (text_length > 24) ? 24 : text_length;
    vram_write(text, line_length);

    // If text is longer than 24 characters, move to the next line
    if (text_length > 24) {
        vram_adr(NTADR_A(4, 5));  // Move to the second line
        line_length = (text_length > 48) ? 24 : (text_length - 24);
        vram_write(text + 24, line_length);  // Write next 24 characters
    }

    // If text is longer than 48 characters, move to the third line
    if (text_length > 48) {
        vram_adr(NTADR_A(4, 6));  // Move to the third line
        line_length = text_length - 48;
        vram_write(text + 48, line_length);  // Write remaining characters
    }

    ppu_on_all();
}

// Clear the dialogue box by writing empty spaces
void clear_dialogue_page() {
    // Fill page with spaces or set tiles to empty
    ppu_off();
    
    vram_adr(NTADR_A(3, 3));
    vram_fill(0x00, 25);
    vram_adr(NTADR_A(3, 4));
    vram_fill(0x00, 25);
    vram_adr(NTADR_A(3, 5));
    vram_fill(0x00, 25);
    vram_adr(NTADR_A(3, 6));
    vram_fill(0x00, 25);
  
    ppu_on_all();
    
}

// Clear the dialogue box by writing empty spaces
void clear_dialogue_box() {
    ppu_off(); // Turn off rendering to safely update VRAM
    vram_adr(NAMETABLE_A);
    vram_write(nametable_game_0, 1024);
    ppu_on_all();
}

// Handling dialogue interaction 
void handle_dialogue_input(char pad) {
    update_dialogue_cooldown();
  
    if (dialogue_cooldown == 0 && (pad & PAD_A)) {  // Only allow input if cooldown is zero
        if (dialogues[current_dialogue_index].next == -1) {
            is_dialogue_active = false;
            player_state = STATE_IDLE;  // Return control to player
            clear_dialogue_box();
            clear_dialogue_page();
            load_hud();
            update_hud();
            delay(20);
        } else {
            clear_dialogue_page();
            current_dialogue_index += 1;
            load_dialogue_page();
        }
        dialogue_cooldown = DIALOGUE_COOLDOWN;  // Set the cooldown to 15 frames (adjust as needed)
    }
}

// Call this function in your game loop to update the cooldown
void update_dialogue_cooldown() {
    if (dialogue_cooldown > 0) {
        dialogue_cooldown--;
    }
}


// Function to display the interaction indicator
void update_interaction_indicator(unsigned char* oam_id) {
    if (can_interact) {
        // Display the up arrow sprite above the player
        *oam_id = oam_spr(player_x, player_y + ARROW_Y_OFFSET, ARROW_TILE, ARROW_ATTR, *oam_id);
    }
}



//---------------------------------------------------------------------------------------//
//                             HUD FUNCTIONS                                             //
//---------------------------------------------------------------------------------------//


void load_hud() {
  
    ppu_off();
   
    vram_adr(NTADR_A(3, 3));
    vram_write("\x95\x96", 2);
    vram_adr(NTADR_A(5, 4));
    vram_write("\xa7\xa8", 2);
  
    // Write soul tiles to VRAM
    vram_adr(NTADR_A(3, 4));
    vram_write(&soul_tile_top_1, 2);
    vram_adr(NTADR_A(3, 5));
    vram_write(&soul_tile_bottom_1, 2);
    
    vram_adr(NTADR_A(5, 5));
    vram_write(&mask_tile_1, 1);
    vram_adr(NTADR_A(6, 5));
    vram_write(&mask_tile_2, 1);
    vram_adr(NTADR_A(7, 5));
    vram_write(&mask_tile_3, 1);
    
    ppu_on_all();
}


// Function to update the soul indicator
void update_soul_indicator() {

    if (player_soul != previous_soul) {
        previous_soul = player_soul;  // Update previous value
      
        // Determine which tiles to use based on player_soul
        if (player_soul == 60) {   // Full soul
            soul_tile_top_1 = TILE_SOUL_TOP_FULL_1;
            soul_tile_top_2 = TILE_SOUL_TOP_FULL_2;
            soul_tile_bottom_1 = TILE_SOUL_BOTTOM_FULL_1;
            soul_tile_bottom_2 = TILE_SOUL_BOTTOM_FULL_2;
        } else if (player_soul >= 30) {  // Half soul
            soul_tile_top_1 = TILE_SOUL_TOP_EMPTY_1;
            soul_tile_top_2 = TILE_SOUL_TOP_EMPTY_2;
            soul_tile_bottom_1 = TILE_SOUL_BOTTOM_FULL_1;
            soul_tile_bottom_2 = TILE_SOUL_BOTTOM_FULL_2;
        } else {  // Empty soul
            soul_tile_top_1 = TILE_SOUL_TOP_EMPTY_1;
            soul_tile_top_2 = TILE_SOUL_TOP_EMPTY_2;
            soul_tile_bottom_1 = TILE_SOUL_BOTTOM_EMPTY_1;
            soul_tile_bottom_2 = TILE_SOUL_BOTTOM_EMPTY_2;
        }

        ppu_off();
        // Write soul tiles to VRAM
        vram_adr(NTADR_A(3, 4));
        vram_write(&soul_tile_top_1, 2);
        vram_adr(NTADR_A(3, 5));
        vram_write(&soul_tile_bottom_1, 2);

        ppu_on_all();
    }
}

// Function to update the lives indicator
void update_lives_indicator() {
    mask_tile_1 = player_lives >= 1 ? TILE_MASK_FULL : TILE_MASK_EMPTY;
    mask_tile_2 = player_lives >= 2 ? TILE_MASK_FULL : TILE_MASK_EMPTY;
    mask_tile_3 = player_lives >= 3 ? TILE_MASK_FULL : TILE_MASK_EMPTY;

    if (player_lives != previous_lives) {
        previous_lives = player_lives;  // Update previous value
      
        ppu_off();
        // Write mask tiles to VRAM
        vram_adr(NTADR_A(5, 5));
        vram_write(&mask_tile_1, 1);
        vram_adr(NTADR_A(6, 5));
        vram_write(&mask_tile_2, 1);
        vram_adr(NTADR_A(7, 5));
        vram_write(&mask_tile_3, 1);

        ppu_on_all();
    }
}

// Call this function whenever player health or soul changes
void update_hud() {
    update_soul_indicator();
    update_lives_indicator();
}







//---------------------------------------------------------------------------------------//
//                             MAIN GAME LOOP                                            //
//---------------------------------------------------------------------------------------//


// Main function
void main(void) {
  // Setup graphics and load the initial nametable for the menu
  setup_graphics();
  setup_audio();
  pal_bright(0);
  delay(60);
  setup_menu(); // Load menu screen initially
  fade_in(); // Fade in after loading the new state


  // Game loop
  while (1) {
    check_game_state(); // Check and update based on the game state
    ppu_wait_nmi();     // Wait for the next NMI (synchronizing game logic with V-blank)
    nmi_set_callback(famitone_update);
    if (fade_done == 1){
    // scroll_background();
    }
    
  }
}
