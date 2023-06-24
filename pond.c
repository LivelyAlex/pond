#include <curses.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#define uint unsigned int

#define SIN_PI_OVER_4 0.7

#define GREEN   1
#define YELLOW  2
#define NORMAL  3
#define BLUE    4
#define MAGENTA 5
#define RED     6

#define LEFT    0b0001
#define RIGHT   0b0010
#define UP      0b0100
#define DOWN    0b1000

#define WATER   0
#define PAD     1
#define FLOWER  2

#define PLOUF_ARRAY_LENGTH 16
#define LILYPAD_DENSITY    0.005
#define FLOWER_DENSITY     0.002
#define FISH_DENSITY       0.01
#define DEATH_ZONE         8 // after that many characters offscreen, KILL the entity

void print_help() {
    printf("A cute TTY pond simulator. Frogs included.\n");
    printf("Use the \"Q\" key to quit. Recommended over Ctrl+C.\n");
    printf("\n");
    printf("Usage: pond [OPTIONS]\n");
    printf("\n");
    printf("Options:\n");
    printf("--help, -h           Prints this help and exits\n");
    printf("--screensaver, -s    Starts the program in screensaver mode\n");
    printf("                     (exits on any key press)\n");
    printf("--delay              In screensaver mode, delays the closing of the program\n");
    printf("                     just a little bit.\n");
    printf("--quiet, -q          Doesn't show the report at the end (if you run the\n");
    printf("                     program in screensaver mode, quiet is activated by default)\n");
    printf("--rain, -r           Forces the rain on\n");
    printf("--dry, -d            Forces the rain off\n");
    printf("--flowers, -f        Forces flowers on, for a lovely spring look\n");
    printf("--no-flowers, -nf    Forces flowers off, for a minimalist look\n");
    printf("--intrepid-frogs, -i Frogs won't get scared by key or mouse presses\n");
    printf("--all-the-frogs, -a  Gives you all the frogs\n");
    printf("--debug              Shows *some* debug information\n");
}

bool OPTION_SCREENSAVER = false;
int  OPTION_SCREENSAVER_DELAY = 0;
bool OPTION_FORCE_RAIN = false;
bool OPTION_FORCE_NO_RAIN = false;
bool OPTION_FORCE_FLOWERS = false;
bool OPTION_FORCE_NO_FLOWERS = false;
bool OPTION_QUIET = false;
bool OPTION_INTREPID_FROGS = false;
bool OPTION_ALL_FROGS = false;
bool OPTION_DEBUG = false;

void parse_args(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        char* str = argv[i];
        if (strcmp(str, "--help") == 0 || strcmp(str, "-h") == 0) {
            print_help();
            exit(0);
        } else if (strcmp(str, "--screensaver") == 0 || strcmp(str, "-s") == 0) {
            OPTION_SCREENSAVER = true;
            OPTION_QUIET = true;
        } else if (strcmp(str, "--delay") == 0) {
            OPTION_SCREENSAVER_DELAY = 10;
        } else if (strcmp(str, "--rain") == 0 || strcmp(str, "-r") == 0) {
            OPTION_FORCE_RAIN = true;
        } else if (strcmp(str, "--dry") == 0 || strcmp(str, "-d") == 0) {
            OPTION_FORCE_NO_RAIN = true;
        } else if (strcmp(str, "--flowers") == 0 || strcmp(str, "-f") == 0) {
            OPTION_FORCE_FLOWERS = true;
        } else if (strcmp(str, "--no-flowers") == 0 || strcmp(str, "-nf") == 0) {
            OPTION_FORCE_NO_FLOWERS = true;
        } else if (strcmp(str, "--intrepid-frogs") == 0 || strcmp(str, "-i") == 0) {
            OPTION_INTREPID_FROGS = true;
        } else if (strcmp(str, "--all-the-frogs") == 0 || strcmp(str, "-a") == 0) {
            OPTION_ALL_FROGS = true;
        } else if (strcmp(str, "--debug") == 0) {
            OPTION_DEBUG = true;
        } else if (strcmp(str, "--quiet") == 0 || strcmp(str, "-q") == 0) {
            OPTION_QUIET = true;
        } else {
            printf("Unknown option : \"%s\". Try pond -h for help\n", str);
            printf("(Note: some options aren't implemented yet)\n");
            exit(-1);
        }
    }
}

struct Sprite {
    uint   height;
    uint   width;
    uint   frameCount;
    char*  frameBuffer[];
};

const int PLOUF_TICKS_PER_FRAME = 1;
const int PLOUF_LIFETIME = PLOUF_TICKS_PER_FRAME * 4;
int RANDOM_PLOUF_CHANCE = 100;
int NEW_FROG_CHANCE = 100; // 1/NEW_FROG_CHANCE is the actual probability
const int FROG_ARRAY_SIZE = 16;
int FROGS_SPAWNED = 0;

struct Plouf {
    //struct Sprite* sprite;
    int  x;
    int  y;
    uint currentFrame;
    uint ticksUntilNextFrame;
    int  ticksToLive; // when reaches 0, DEATH.
};

struct Plouf ploufArray[PLOUF_ARRAY_LENGTH];
bool isPloufArrayIndexFree[PLOUF_ARRAY_LENGTH];

struct WaterLily {
    int x;
    int y;
    short color;
    bool flower;
    bool orientation; //true = V
    bool flipped;
};

struct Frog {
    int x, y;
    short color;
    char eyes; // o_o
    char mouth;
    char* sides;
    short direction;
    int jumpiness; // general tendency of that frog to jump around
    int jump_distance; // will be applied twice, so it's really the half jump distance
    int swim_distance; // not used
    int jump_height;
    int croakiness; // you can guess
    int blinkiness;
    int swimminess; // how long the frog will surface in between swims
    int eye_wetness; // hydration of the eyes. When reaches 0, triggers a blink.
    int urge_to_croak;
    int croak; // ticks left till croak end 0 = not croaking
    int urge_to_jump;
    int jumps_in_a_row;
    int jumps_left_to_do; // number of jumps left to do after an urge to jump
    int time_between_jumps;
    int waiting_for_new_jump; // time between jumps in a row
    int jumping; // 0 = not jumping, 2 = jumping, Y = Y - jump height, 1 = landing
    bool in_water; // when in water, jumps become swims
    bool stubborn; // always keeps the same general direction
    bool loves_land; // always stops when landing on land
    uint coolness_score;
    // maybe add time_alive, and calculate the score at despawn.
    // that means we also have to compare the not-yet-despawn frogs
    // at the end to the coolest frog.
    //
    // but i like the idea because a frog that sticks around for longer
    // has more chance getting spotted by the player who could recognize it in the end text
};

struct Frog coolest_frog = {
    .coolness_score = 0,
};

struct Sprite PLOUF_SPRITE = {
    .height = 5,
    .frameCount = 4,
    .frameBuffer = 
        {
"\n\
\n\
       .\n\
\n\
\0",
"\n\
       _\n\
     (   )\n\
       -\n\
\0",
"          \n\
     .---.\n\
    (  .  )\n\
       _\n\
\0",
"    .  -  .\n\
 .     _     .\n\
(    (   )    )\n\
 .     -     .\n\
   .   _  ."
        }
};

char* waterLilyLeafV = "\
     #####\n\
  ###########\n\
###############\n\
###############\n\
###### #######\n\
  ###   ####";

char* lily_pad_leaf_ascii_new = "\
    .=====.\n\
  ==&;::::%==\n\
=&::::;;::::;&=\n\
=;:.::%:%:;;:;=\n\
==:::;^:&.::==\n\
 ^==:   ====:";

int lily_pad_leaf_offset_y = 3;
int lily_pad_leaf_offset_x = 7;

char* waterlily_flower = "\
    ._^^.^ .\n\
  .^~\\:%|~^/:~\n\
  _~_~#^/#~_^_-\n\
 ._~~%:#=:#~;-~.\n\
    /~; ^U ^\\";

char* waterlily_flower_small = "\
    ._^^.^ .\n\
  .^~\\:%|~^/:~\n\
  _~_~#^/#~_^_-\n\
    /~; ^U ^\\";

char* ascii_frog = "\
  omo\n\
 (_^_)";
// m for mouth, that will be replaced in runtime dw
char* frog_blink = "\
  -m-\n\
 (_^_)";

char* frog_croak = "\
  omo\n\
( _^_ )";

char* frog_swimming = "\n  omo";
char* from_swimming_blinking = "\n  -m-";

char alternate_eyes[] = {'0', '@', '.', 'T', '^', 'e', 'c', 'v', 'p', '$'};
int alternate_eyes_size = 10;

char alternate_mouths[] = {'~', 'w', '-'};
int alternate_mouths_size = 3;

char* alternate_sides[] = {"[]", "{}", "/\\"};
int alternate_sides_size = 3;
/*struct Sprite loadSprite(char filename[]) {
    FILE* fptr;
    fptr = fopen(filename, "r");
    char buffer[2048];
    fgets(buffer, 2048, fptr);
    // read header
    bool cr = false;
    for (int i = 0; cr == false; i++) {
        if (buffer[i] == '\n') {
            cr = true;
            printf("carriage retuuuuurn");
        }
    }
}*/

void calculate_coolness_score(struct Frog* frog) {
    uint color_score = 10; // so that color doesnt have that much influence
    uint eye_score = 1;
    uint mouth_score = 1;
    uint sides_score = 1;
    uint random_score = 10 + rand() % 5; // make things just a bit less predictable

    if (frog->color == YELLOW)
        color_score += 1;
    else if (frog->color == RED)
        color_score += 2;

    if (frog->eyes != 'o')
        eye_score = 13;
    if (frog->mouth != '_')
        mouth_score = 10;
    if (frog->sides[0] != '(')
        sides_score = 12;

    uint score = color_score * eye_score * mouth_score * sides_score * random_score;
    frog->coolness_score = score;
}

void addPlouf(int x, int y) {
   for (int i = 0; i < PLOUF_ARRAY_LENGTH; i++) {
       if (isPloufArrayIndexFree[i]) {
            struct Plouf newPlouf = {
                .x = x,
                .y = y,
                .currentFrame = 0,
                .ticksUntilNextFrame = PLOUF_TICKS_PER_FRAME,
                .ticksToLive = PLOUF_LIFETIME,
            };
            ploufArray[i] = newPlouf;
            isPloufArrayIndexFree[i] = false;
            return;
       }
   }
}

void render_str(WINDOW* win, char* str, int Y, int X, int center_y, int center_x, bool transparent) {
    int i = 0;
    int x = X - center_x;
    int y = Y - center_y;
    bool startedRendering = false;
    while (str[i] != '\0') {
        if (str[i] == '\n') {
            y += 1;
            x = X - center_x;
            startedRendering = false;
        } else {
            if (y >= LINES)
                return;
            if ((x < COLS) && (x >= 0) && (y >= 0)) {
                wmove(win, y, x);
                // so that spaces are only opaque if they're not precedeing the drawing
                bool skip = false;
                if ((transparent || !startedRendering) && str[i] == ' ') {
                    skip = true;
                } else if (str[i] != ' ') {
                    startedRendering = true;
                }

                if (!skip) {
                    waddch(win, str[i]);
                }
            }
            x ++;
        }
        i++;
    }
}

//'eye character' will replace all 'o's
void render_frog(WINDOW* win, char* str, int Y, int X, int center_y, int center_x, bool transparent, char eye_char, char mouth_char, char side_char1, char side_char2) {
    int i = 0;
    int x = X - center_x;
    int y = Y - center_y;
    bool startedRendering = false;
    while (str[i] != '\0') {
        if (str[i] == '\n') {
            y += 1;
            x = X - center_x;
            startedRendering = false;
        } else {
            if (y >= LINES)
                return;
            if ((x < COLS) && (x >= 0) && (y >= 0)) {
                wmove(win, y, x);
                // so that spaces are only opaque if they're not precedeing the drawing
                bool skip = false;
                if ((transparent || !startedRendering) && str[i] == ' ') {
                    skip = true;
                } else if (str[i] != ' ') {
                    startedRendering = true;
                }

                if (!skip) {
                    if (str[i] == 'o') {
                        waddch(win, eye_char);
                    } else if (str[i] == 'm') {
                        waddch(win, mouth_char);
                    } else if (str[i] == '(') {
                        waddch(win, side_char1);
                    } else if (str[i] == ')') {
                        waddch(win, side_char2);
                    } else {
                        waddch(win, str[i]);
                    }
                }
            }
            x ++;
        }
        i++;
    }
}

void fill_terrain_with(short terrain[LINES][COLS], char* str, short type, int Y, int X, int center_y, int center_x, bool transparent) {
    int i = 0;
    int x = X - center_x;
    int y = Y - center_y;
    bool startedRendering = false;
    while (str[i] != '\0') {
        if (str[i] == '\n') {
            y += 1;
            x = X - center_x;
            startedRendering = false;
        } else {
            if (y >= LINES)
                return;
            if ((x < COLS) && (x >= 0) && (y >= 0)) {
                // so that spaces are only opaque if they're not precedeing the drawing
                bool skip = false;
                if ((transparent || !startedRendering) && str[i] == ' ') {
                    skip = true;
                } else if (str[i] != ' ') {
                    startedRendering = true;
                }

                if (!skip) {
                    terrain[y][x] = type;
                }
            }
            x ++;
        }
        i++;
    }
}

void set_up_waterlilies(short terrain[LINES][COLS], struct WaterLily lily_pad_array[], int n_waterlilies, int n_lily_flowers) {
    for (int i = 0; i < n_waterlilies + n_lily_flowers; i++) {
        short color = GREEN; //rand() % 3 == 0 ? YELLOW : GREEN;
        int x, y;
        do {
            y = rand() % LINES;
            x = rand() % COLS;
        } while (terrain[y][x] != WATER);
        bool flower = i >= n_waterlilies + 1;
        if (flower)
            color = rand() % 3 == 0 ? MAGENTA : NORMAL;
        struct WaterLily newLily = {.x = x, .y = y, .color = color, .flower = flower};
        lily_pad_array[i] = newLily;
        if (flower) {
            fill_terrain_with(terrain, waterlily_flower_small, FLOWER, y, x, lily_pad_leaf_offset_y, lily_pad_leaf_offset_x, false);
        } else {
            fill_terrain_with(terrain, lily_pad_leaf_ascii_new, PAD, y, x, lily_pad_leaf_offset_y, lily_pad_leaf_offset_x, false);
        }
    }
}

short get_terrain(short terrain[LINES][COLS], int y, int x) {
    if ((y < 0) || (y > LINES) || (x < 0) || (x > COLS)) {
        return WATER;
    }

    return terrain[y][x];
}

short random_direction() {
    switch (rand() % 4) {
        case 0: // Y axis
            return ((rand() % 2 == 0) ? UP : DOWN);
        case 1: // X axis
            return rand() % 2 == 0 ? LEFT : RIGHT;
        default: //both (twice as likely)
            return (rand() % 2 == 0 ? UP : DOWN) | (rand() % 2 == 0 ? LEFT : RIGHT);
    }
}

// gets the next clockwise direction; anti clockwise if reverse = true
short get_next_direction(short direction, bool reverse) {
    if (direction == UP) {
        return reverse ? UP | LEFT : UP | RIGHT;
    } else if (direction == (UP | RIGHT)) {
        return reverse ? UP : RIGHT;
    } else if (direction == RIGHT) {
        return reverse ? UP | RIGHT : DOWN | RIGHT;
    } else if (direction == (DOWN | RIGHT)) {
        return reverse ? RIGHT : DOWN;
    } else if (direction == DOWN) {
        return reverse ? DOWN | RIGHT : DOWN | LEFT;
    } else if (direction == (DOWN | LEFT)) {
        return reverse ? DOWN : LEFT;
    } else if (direction == LEFT) {
        return reverse ? DOWN | LEFT : UP | LEFT;
    } else if (direction == (UP | LEFT)) {
        return reverse ? LEFT : UP;
    } else {
        return LEFT; // frogs are left-leaning
    }
}


// this can move a lot of other things than frogs, too!
void frog_move(short direction, int distance, int* y, int* x) {
    float x_direction_multiplier = (direction & 0b11) == RIGHT ? 1 : (direction & 0b11) == LEFT ? -1 : 0;
    float y_direction_multiplier = (direction & 0b1100) == DOWN ? 1 : (direction & 0b1100) == UP ? -1 : 0;

    if (x_direction_multiplier != 0 && y_direction_multiplier != 0) {
        x_direction_multiplier *= SIN_PI_OVER_4;
        y_direction_multiplier *= SIN_PI_OVER_4;
    }

    *x += distance * x_direction_multiplier;
    *y += distance * y_direction_multiplier;
}

short frog_predict_landing_tile(short terrain[LINES][COLS], short direction, short distance, int y, int x) {
    int _x = x;
    int _y = y;
    frog_move(direction, distance, &_y, &_x); 
    frog_move(direction, distance, &_y, &_x); 
    return get_terrain(terrain, _y, _x);
}

void tick_frog(WINDOW* win, short terrain[LINES][COLS], struct Frog frog_array[], bool is_index_free[]) {
    for (int i = 0; i < FROG_ARRAY_SIZE; i++) {
        if (!is_index_free[i]) {
            struct Frog* frog = &frog_array[i];
            bool blinking = false;
            frog->urge_to_jump--;
            frog->waiting_for_new_jump--;
            if (frog->urge_to_jump <= 0 || (frog->jumps_left_to_do > 0 && frog->waiting_for_new_jump <= 0)) {
                // initial jump
                if (frog->urge_to_jump <= 0) {
                    frog->jumps_left_to_do = frog->jumps_in_a_row;
                    // every frog is stubborn when it is in water
                    if (!frog->stubborn && !frog->in_water) {
                        frog->direction = random_direction();
                    }
                }
                frog->urge_to_jump = 1000; // the urge to jump is set later, this is just a hack to avoid this loop next time
                frog->waiting_for_new_jump = frog->in_water ? 4 : frog->time_between_jumps;
                frog->jumping = 2;
                if (rand() % 3 == 0) {
                    //change of direction
                    bool reverse = rand() % 2 == 0;
                    frog->direction = get_next_direction(frog->direction, reverse);
                }
                if (frog->in_water) {
                    addPlouf(frog->x, frog->y);
                    // this allows the frog to leap out of the water if the jump ends on land
                    if (frog_predict_landing_tile(terrain, frog->direction, frog->jump_distance, frog->y, frog->x) != WATER) {
                        frog->in_water = false;
                        if (frog->loves_land) {
                            frog->jumps_left_to_do = 1;
                        }
                    }
                }
            }

            int y = frog->y; // in a variable bc we might want to alter it for the jump, and not alter frog.y
            char* default_str = "";
            char** str_to_render = &default_str; 
            if (frog->jumping > 0) {
                frog_move(frog->direction, frog->jump_distance, &frog->y, &frog->x);
                y = frog->y;

                // first tick of the jump
                if (frog->jumping == 2) {
                    if (!frog->in_water) {
                        y -= frog->jump_height;
                    }
                }

                frog->jumping--;

                // landing
                if (frog->jumping == 0) {
                    if (frog->x < -DEATH_ZONE || frog->x > COLS + DEATH_ZONE || frog->y < -DEATH_ZONE || frog->y > LINES + DEATH_ZONE) {
                        // sorry lil frog
                        is_index_free[i] = true;
                        continue;
                    }
                    
                    if(get_terrain(terrain, frog->y, frog->x) == WATER) {
                        frog->in_water = true;
                        addPlouf(frog->x, frog->y);
                    } else {
                        frog->in_water = false;
                    }

                    frog->jumps_left_to_do--;
                    // last landing of this jump series
                    if (frog->jumps_left_to_do <= 0) {
                        if (frog->in_water) {
                            frog->urge_to_jump = frog->swimminess; // when in water, doesn't stay as long
                        } else {
                            frog->urge_to_jump = frog->jumpiness;
                        }
                    }
                }
                if (!frog->in_water) { 
                    str_to_render = &ascii_frog;
                }
            } else {// non-jumping frog
                frog->eye_wetness--;
                if (frog->eye_wetness <= 0) {
                    frog->eye_wetness = rand() % 50 + 10;
                    blinking = true;
                }
                if (frog->croak > 0) {
                    frog->croak--;
                } else if (frog->urge_to_croak <= 0) {
                    frog->croak = 4;
                    frog->urge_to_croak = rand() % 100 + 50;
                }
                else {
                    frog->urge_to_croak--;
                }
                if (!frog->in_water) {
                    str_to_render = frog->croak > 0 ? &frog_croak : blinking ? &frog_blink : &ascii_frog;
                } else if (!(frog->waiting_for_new_jump > 0)) {
                    // if the frog is swimming, we don't display its cute eyes until it has stopped swimming (swimming = jumping when underwater
                    str_to_render = blinking ? &from_swimming_blinking : &frog_swimming;
                }
            }

           
            wattron(win, A_BOLD | COLOR_PAIR(frog->color));
            render_frog(win, *str_to_render, y, frog->x, 1, 2, false, frog->eyes, frog->mouth, frog->sides[0], frog->sides[1]);
            
        }
    }
}
void tickPlouf(WINDOW* win) {
    for (int i = 0; i < PLOUF_ARRAY_LENGTH; i ++) {
        if (!isPloufArrayIndexFree[i]) {
            struct Plouf* plouf = &ploufArray[i];
            plouf->ticksToLive--;
            if (plouf->ticksToLive <= 0) {
                isPloufArrayIndexFree[i] = true;
                // if we change to ptr later, free memory here
                continue;
            }
            plouf->ticksUntilNextFrame--;
            if (plouf->ticksUntilNextFrame <= 0) {
                plouf->ticksUntilNextFrame = PLOUF_TICKS_PER_FRAME;
                plouf->currentFrame++;
            }
            render_str(win, PLOUF_SPRITE.frameBuffer[plouf->currentFrame], plouf->y, plouf->x, 2, 7, true);
        }
    }
}

int spawn_frog(struct Frog frogArray[], bool isIndexFree[], int y, int x, short direction) {
    for (int i = 0; i < FROG_ARRAY_SIZE; i++) {
        if (isIndexFree[i]) {
            short color = rand() % 3 < 2 ? GREEN : rand() % 5 == 0 ? RED : YELLOW;
            char eyes = 'o';
            if (rand()% 15 == 0) {
                eyes = alternate_eyes[rand() % alternate_eyes_size];            
            }
            char mouth = '_';
            if (rand() % 20 == 0) {
                mouth = alternate_mouths[rand() % alternate_mouths_size];
            }
            char* sides = "()";
            if (rand() % 30 == 0) {
                sides = alternate_sides[rand() % alternate_sides_size];
            }
            int jumpiness = 30 + rand() % 200;
            int croakiness = 10 + rand() % 60;
            int blinkiness = 5 + rand() % 20;
            struct Frog frog = {
                .x = x,
                .y = y,
                .color = color,
                .eyes = eyes,
                .mouth = mouth,
                .sides = sides,
                .direction = direction,
                .eye_wetness = blinkiness,
                .blinkiness = blinkiness,
                .croakiness = croakiness,
                .swimminess = 3 + rand() % 20,
                .urge_to_croak = croakiness,
                .urge_to_jump = jumpiness,
                .jumps_in_a_row = 2 + rand() % 4,
                .time_between_jumps = 2 + rand() % 6,
                .jumping = 0,
                .jump_distance = 2,
                .jump_height = 3,
                .jumpiness = jumpiness,
                .croak = 0,
                .in_water = false,
                .stubborn = rand() % 4 == 0 ? false : true, // it is a well known fact that most frogs are stubborn
                .loves_land = rand() % 2 == 0 ? true : false,
            };

            // very important step
            if (!OPTION_QUIET) {
                calculate_coolness_score(&frog);
                if (frog.coolness_score > coolest_frog.coolness_score) {
                    coolest_frog = frog;
                }
            }
            frogArray[i] = frog;
            isIndexFree[i] = false;
            FROGS_SPAWNED++;
            return i;
        }
    }
    return -1;
}

void tick_frog_spawner(struct Frog frog_array[], bool is_index_free[], bool force_spawn) {
    if (force_spawn || (rand() % NEW_FROG_CHANCE == 0)) {
        short direction;
        int x = rand() % COLS;
        int y = rand() % LINES;


        switch(rand() % 4) {
            case 0:
                x = -DEATH_ZONE; // distance from edge of screen is death zone
                direction = RIGHT;
                break;
            case 1:
                x = COLS + DEATH_ZONE;
                direction = LEFT;
                break;
            case 2:
                y = - DEATH_ZONE;
                direction = DOWN;
                break;
            case 3: 
                y = LINES + DEATH_ZONE;
                direction = UP;
                break;
        }

        int i = spawn_frog(frog_array, is_index_free, y, x, direction);
        if (i != -1) {
            frog_array[i].urge_to_jump = 50; // so that spawned frog swims 5s after spawning (not 0 so that it doesnt spawn right after a panic and looks silly
            frog_array[i].in_water = true;
        }
    }
}

int main(int argc, char* argv[]) {
    parse_args(argc, argv);
    int pause_ms = 100;
    srand(time(NULL));
    //curses inits
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    //if (!OPTION_DEBUG) // debug step by step
    nodelay(stdscr, true); // non blocking wait for char
    start_color();
    // to make the frogs run when we click on screen
    mousemask(BUTTON1_PRESSED | BUTTON2_PRESSED, NULL);

    init_pair(GREEN, COLOR_GREEN, COLOR_BLACK);
    init_pair(YELLOW, COLOR_YELLOW, COLOR_BLACK);
    init_pair(NORMAL, COLOR_WHITE, COLOR_BLACK);
    init_pair(BLUE, COLOR_BLUE, COLOR_BLACK);
    init_pair(MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(RED, COLOR_RED, COLOR_BLACK);

    WINDOW* win = stdscr;
   // keypad(win, true);      
    // my inits
    int INCOLS = COLS; //IN for initial
    int INLINES = LINES;
    bool rain = rand() % 5 == 0;
    bool flower_season = rand() % 2 == 0;

    // applying options here
    if (OPTION_ALL_FROGS) {
        NEW_FROG_CHANCE = 1;
    }
    if (OPTION_FORCE_FLOWERS) {
        flower_season = true;
    }
    if (OPTION_FORCE_NO_FLOWERS) {
        flower_season = false;
    }
    if (OPTION_FORCE_RAIN) {
        rain = true;
    }
    if (OPTION_FORCE_NO_RAIN) {
        rain = false;
    }

    if (rain) {
        RANDOM_PLOUF_CHANCE = 1;
        if (NEW_FROG_CHANCE > 1)
            NEW_FROG_CHANCE /= 2;
    }


    short terrain[LINES][COLS];
    for (int i = 0; i < COLS * LINES; i++) {
        terrain[0][i] = WATER;
    }
    // animation pool
    for (int i = 0; i < PLOUF_ARRAY_LENGTH; i++) {
        isPloufArrayIndexFree[i] = true;
    }

    // waterlilies leaves
    int n_lily_pad_leaves = LILYPAD_DENSITY * (COLS * LINES);
    int n_lily_flowers = flower_season ? FLOWER_DENSITY * (COLS * LINES) : 0;
    if (n_lily_flowers == 0)
        n_lily_flowers = 1;
    struct WaterLily lily_pad_array[n_lily_pad_leaves + n_lily_flowers];
    set_up_waterlilies(terrain, lily_pad_array, n_lily_pad_leaves, n_lily_flowers);


    //frogs !!!!
    struct Frog frog_array[FROG_ARRAY_SIZE];
    bool is_frog_array_index_free[FROG_ARRAY_SIZE];
    for (int i = 0; i < FROG_ARRAY_SIZE; i++) {
        is_frog_array_index_free[i] = true;
    }
    // spawn_frog(frog_array, is_frog_array_index_free, lily_pad_array[0].y, lily_pad_array[0].x, LEFT);
    spawn_frog(frog_array, is_frog_array_index_free, -DEATH_ZONE, rand() % COLS, DOWN);
    spawn_frog(frog_array, is_frog_array_index_free, LINES + DEATH_ZONE, rand() % COLS, UP);

    bool quit = false;
    bool window_resizing = false;
    int last_panic = 0;
    int number_of_plouf_by_loop = rain ? 2 : 1;
    int quit_delay = 0;
    while (!quit || quit_delay > 0) {
        if (quit) {
            quit_delay--;
        }

        last_panic--;
        int frog_count = 0;
        for (int i = 0; i < FROG_ARRAY_SIZE; i++) {
            if (!is_frog_array_index_free[i])
                frog_count ++;
        }

        wattron(win, COLOR_PAIR(NORMAL));
        werase(win);

        tick_frog_spawner(frog_array, is_frog_array_index_free, frog_count == 0);
        // fish catching little insects on the surface of the water
        for (int i = 0; i < number_of_plouf_by_loop; i++) {
            if (rand() % RANDOM_PLOUF_CHANCE == 0) {
                move(0,0);
                int y = rand() % LINES;
                int x = rand() % COLS;
                if (terrain[y][x] == WATER)
                    addPlouf(x, y);
            }
        }

        tickPlouf(win);

        // render waterlilies
        for (int i = 0; i < n_lily_pad_leaves + n_lily_flowers; i++) {
            struct WaterLily* lily_pad = &lily_pad_array[i];
            //wattron(win, waterLily->color);
            wattron(win, COLOR_PAIR(lily_pad->color));
            if (lily_pad->flower)
                wattron(win, A_BOLD);

            render_str(win, lily_pad->flower ? waterlily_flower_small : lily_pad_leaf_ascii_new, lily_pad->y, lily_pad->x, lily_pad_leaf_offset_y, lily_pad_leaf_offset_x, true);
            wattroff(win, COLOR_PAIR(lily_pad->color));
            if (lily_pad -> flower)
                wattroff(win, A_BOLD);
            //wattron(win, COLOR_PAIR(0));
        }
        tick_frog(win, terrain, frog_array, is_frog_array_index_free);
        wattroff(win, A_BOLD);

        //debug
        if (OPTION_DEBUG) {
            wattron(win, COLOR_PAIR(NORMAL));
            wmove(win, 0, 0);
            waddch(win, (frog_count / 10) + 0x30);
            waddch(win, (frog_count % 10) + 0x30);
        }

        wrefresh(win);
        int ch = getch();
        if (ch != ERR) {
            if (OPTION_SCREENSAVER) {
                quit = true;
                quit_delay = OPTION_SCREENSAVER_DELAY;
            }

            if (!OPTION_INTREPID_FROGS && last_panic <= 0) {
                for (int i = 0; i < FROG_ARRAY_SIZE; i++) {
                    if (!is_frog_array_index_free[i]) {
                        frog_array[i].jumping = 2;
                        frog_array[i].jumps_left_to_do = 8;
                        frog_array[i].loves_land = false;
                        short direction = 0;

                        int x = frog_array[i].x;
                        int y = frog_array[i].y;

                        int cols_third = COLS / 3;
                        int lines_third = LINES / 3;

                        direction += x < cols_third ? LEFT : x > 2 * cols_third ? RIGHT : 0;
                        direction += y < lines_third ? UP : y > 2 * lines_third ? DOWN : 0;
                        if (direction != 0) {
                            frog_array[i].direction = direction;
                        }

                    }
                }
                last_panic = 30;
            }
        }
        
        if (ch == 'q' || ch == 'Q')
            quit = true;

        while (getch() != ERR) {} //empty buffer (bc clicks fill the buffer and block exiting for a while)
        
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = pause_ms * 1000000;
        nanosleep(&ts, NULL);

        if (COLS != INCOLS || LINES != INLINES) {
            quit = true;
            window_resizing = true;
            // resizing actually mostly works, but sometimes shows undefined behaviour so let's just be safe
        }
    }

    endwin();
    /*for (int j = 0; j < LINES; j++) {
        for (int i = 0; i < COLS; i++) {
            printf("%d", terrain[j][i]);
        }
        printf("\n");
    }*/

    if (!OPTION_QUIET) {
        if (window_resizing) {
            printf("[exiting due to terminal window resizing]\n");
        }
        char* weather = rain ? "rainy" : "nice";
        char* flower_opt = flower_season ? ", the lily pads were flowering!" : ".";
        printf("That was a %s day at the pond%s\n", weather, flower_opt);
        printf("%d unique frogs came to visit!\n", FROGS_SPAWNED);
        if (FROGS_SPAWNED < 20) {
            printf("That's not a lot, but I didn't stay for long either.\n");
            rain ? printf("I was starting to get soaked!\n") : 0 + 0;
        } else if (FROGS_SPAWNED < 50) {
            printf("That's a lot of frogs! I wonder how many more there is..\n");
        } else if (FROGS_SPAWNED < 200) {
            if (rain) {
                printf("I might have catched a cold..\n");
            } else {
                printf("I had no idea such a little pond could contain so many frogs..\n");
            }
        } else {
            printf("Finally! After observing frogs for so long, I have completed my PhD on frogs!\n");
            if (rain) {
                printf("*You hold a soaked piece of paper that once was your PhD, but it has become\nillegible... You consider coming back when it's not raining.*\n");
            } else {
                printf("*You hold the last page of you PhD; its final conclusion is that frogs are\ncute. You are correct.*\n");
            }
        }
        printf("Anyway, here is the coolest frog I saw today:\n");
        //printf("%c", ascii_frog[0]);
        char str_ascii[20];
        strcpy(str_ascii, ascii_frog);

        /*int i = 0;
        while (str[i] != '\0') {
            i++;
        }*/

        //printf("%d", i);

        str_ascii[2] = coolest_frog.eyes;
        str_ascii[4] = coolest_frog.eyes;
        str_ascii[3] = coolest_frog.mouth;
        str_ascii[7] = coolest_frog.sides[0];
        str_ascii[11] = coolest_frog.sides[1];

        char* color_str;
        //char* color_escp;
        //char* color_normal = "\033[0;37m";
        //not sure about color output. If the user has like black text over white, it would mess their terminal.
        if (coolest_frog.color == GREEN) {
            color_str = "green. That seems to be the most common color!";

        } else if (coolest_frog.color == YELLOW) {
            color_str = "a pale yellow. I wonder if it's toxic?";
        } else {
            color_str = "red! That's a rare color.";
        }

        // i'm tired so that's how i'm displaying my frog don't @ me
        printf("\n#=========#\n");
        printf(  "|         |\n");
        printf(  "|   %c%c%c   |\n", str_ascii[2], str_ascii[3], str_ascii[4]);
        printf(  "| %s  |\n", str_ascii + 6); // no way that worked lol that's so dumb
        printf(  "|         |\n");
        printf(  "#=========#\n\n");
        //printf("\n%s\n\n", str_ascii);
        printf("It's %s\n", color_str);

        if (coolest_frog.eyes != 'o') {
            printf("Look at those eyes!\n");
            switch (coolest_frog.eyes) {
                case '0':
                    printf("They're so tall and wide open!\n");
                    break;
                case 'c':
                    printf("What a shifty look..\n");
                    break;
                case '$':
                    printf("This frog seems to have been corrupted by greed...\n");
                    break;
                case 'v':
                    printf("It looks very soothed..\n");
                    break;
                case 'e':
                    printf("It looks very suspicious!\n");
                    break;
                case '^':
                    printf("They're so full of joy! That frog looks very happy.\n");
                    break;
                case 'p':
                    printf("Oh no! It's sad..\n");
                    break;
                case 'T':
                    printf("Are those.. Brows??\n");
                    break;
                case '.':
                    printf("They're so tiny!\n");
                    break;
                case '@':
                    printf("This frog is in another universe it seems..\n");
                    break;
                default:
                    printf("I've never seen anything like it..\n");
            }
        }
        if (coolest_frog.mouth != '_') {
            printf("This mouth is unusual..\n");
            switch (coolest_frog.mouth) {
                case '~':
                    printf("It seems that frog is really confused!\n");
                    break;
                case '-':
                    printf("It's not that noticeable, but's it's a bit higher than usual!\n");
                    break;
                case 'w':
                    printf("I'm not really sure how to interpret it..\n"); // I'm too old
                                                                                            break;
                default:
                    printf("I've never seen anything like it..\n");
            }
        }
        if (coolest_frog.sides[0] != '(') {
            printf("That body is really weird..\n");
            switch (coolest_frog.sides[0]) {
                case '{':
                    printf("Look at that, it's super spiky!!\n");
                    break;
                case '[':
                    printf("It looks super boxy. Is that a robot frog??..\n");
                    break;
                case '/':
                    printf("It was very triangular. Kind of reminds me of a famous vilain..\n");
                    break;
                default:
                    printf("I've never seen anything like it..\n");
            }
        }

        printf("Here is a little something I noticed about this frog:\n");
        int trivia = rand() % 5;
        switch(trivia) {
            case 0:
                printf("It usually jumped %d times in a row.\n", coolest_frog.jumps_in_a_row);
                break;
            case 1:
                if (coolest_frog.stubborn) {
                    printf("It was very stubborn. Most frogs are.\n");
                } else {
                    printf("It was not stubborn at all! That's not common.\n");
                }
                break;
            case 2:
                if (coolest_frog.loves_land) {
                    printf("It prefers to be on the ground than in water!\n");
                } else {
                    printf("It loved to swim!\n");
                }
                break;
            case 3:
                if (coolest_frog.croakiness < 30) {
                    printf("It could not stop croaking!\n");
                } else {
                    printf("It was very quiet.\n");
                }
                break;
            case 4:
                if (coolest_frog.blinkiness < 10) {
                    printf("It was blinking a lot!\n");
                } else if (coolest_frog.blinkiness > 20) {
                    printf("It almost never blinked..\n");
                } else {
                    printf("The blinking frequency of that frog was perfectly average!\n");
                }
        }

    }
}
