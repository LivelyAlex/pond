#include <curses.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <locale.h>

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
#define DEATH_ZONE         10 // after that many characters offscreen, KILL the entity

struct Sprite {
    uint   height;
    uint   width;
    uint   frameCount;
    char*  frameBuffer[];
};

const int PLOUF_TICKS_PER_FRAME = 1;
int PLOUF_LIFETIME = PLOUF_TICKS_PER_FRAME * 4;
int PLOUF_CHANCE = 100;
int NEW_FROG_CHANCE = 200; // 1/NEW_FROG_CHANCE is the actual probability
const int FROG_ARRAY_SIZE = 16;


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
    int waiting_for_new_jump; // time between jumps in a row
    int jumping; // 0 = not jumping, 2 = jumping, Y = Y - jump height, 1 = landing
    bool in_water; // when in water, jumps become swims
    bool stubborn; // always keeps the same general direction
    bool loves_land; // always stops when landing on land
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
  o_o\n\
 (_^_)";

char* frog_blink = "\
  -_-\n\
 (_^_)";

char* frog_croak = "\
  o_o\n\
( _^_ )";

char* frog_swimming = "\n  o_o";
char* from_swimming_blinking = "\n  -_-";


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
            color = MAGENTA;
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
                frog->urge_to_jump = 1000; // the urge to jump is set later, this is just a hack not to avoid this loop next time
                frog->waiting_for_new_jump = frog->in_water ? 4 : 8;
                frog->jumping = 2;
                if (rand() % 3 < 2) {
                    //change of direction
                    bool reverse = rand() % 2 == 0;
                    frog->direction = get_next_direction(frog->direction, reverse);
                }
                if (frog->in_water) {
                    addPlouf(frog->x, frog->y);
                    // this allows the frog to leap out of the water if the jump ends on land
                    if (frog_predict_landing_tile(terrain, frog->direction, frog->jump_distance, frog->y, frog->x) != WATER) {
                        frog->in_water = false;
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
                    bool was_in_water = frog->in_water;
                    
                    if(get_terrain(terrain, frog->y, frog->x) == WATER) {
                        frog->in_water = true;
                        addPlouf(frog->x, frog->y);
                    } else {
                        frog->in_water = false;
                    }

                    if (frog->loves_land && was_in_water && !frog->in_water) {
                        frog->jumps_left_to_do = 0;
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
            render_str(win, *str_to_render, y, frog->x, 1, 2, false);
            
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
            int jumpiness = 30 + rand() % 200;
            int croakiness = 10 + rand() % 50;
            struct Frog frog = {
                .x = x,
                .y = y,
                .color = color,
                .direction = direction,
                .eye_wetness = 5 + rand() % 20,
                .croakiness = croakiness,
                .swimminess = 3 + rand() % 20,
                .urge_to_croak = croakiness,
                .urge_to_jump = jumpiness,
                .jumps_in_a_row = 2 + rand() % 4,
                .jumping = 0,
                .jump_distance = 2,
                .jump_height = 3,
                .jumpiness = jumpiness,
                .croak = 0,
                .in_water = false,
                .stubborn = rand() % 4 == 0 ? false : true, // it is a well known fact that most frogs are stubborn
                .loves_land = rand() % 2 == 0 ? true : false,
            };
            frogArray[i] = frog;
            isIndexFree[i] = false;
            return i;
        }
    }
    return -1;
}

void tick_frog_spawner(struct Frog frog_array[], bool is_index_free[]) {
    if (rand() % NEW_FROG_CHANCE == 0) {
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
            frog_array[i].urge_to_jump = 0; // so that spawned frog swim immediately
        }
    }
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    //curses inits
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, true); // non blocking wait for char
    start_color();

    init_pair(GREEN, COLOR_GREEN, COLOR_BLACK);
    init_pair(YELLOW, COLOR_YELLOW, COLOR_BLACK);
    init_pair(NORMAL, COLOR_WHITE, COLOR_BLACK);
    init_pair(BLUE, COLOR_BLUE, COLOR_BLACK);
    init_pair(MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(RED, COLOR_RED, COLOR_BLACK);

    WINDOW* win = stdscr;

    // my inits
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
    int n_lily_flowers = FLOWER_DENSITY * (COLS * LINES);
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

    while (!quit) {
        wattron(win, COLOR_PAIR(NORMAL));
        wclear(win);

        tick_frog_spawner(frog_array, is_frog_array_index_free);
        // fish catching little insects on the surface of the water
        if (rand() % PLOUF_CHANCE == 0) {
            move(0,0);
            int y = rand() % LINES;
            int x = rand() % COLS;
            if (terrain[y][x] == WATER)
                addPlouf(x, y);
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

        wrefresh(win);

        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 100000000;
        nanosleep(&ts, NULL);

        if (getch() == 'q' || getch() == 'Q')
            quit = true;
    }

    endwin();
    /*for (int j = 0; j < LINES; j++) {
        for (int i = 0; i < COLS; i++) {
            printf("%d", terrain[j][i]);
        }
        printf("\n");
    }*/
}
