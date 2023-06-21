#include <curses.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <locale.h>

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

#define WATER   1
#define PAD     2
#define FLOWER  3

#define PLOUF_ARRAY_LENGTH 16
#define LILYPAD_DENSITY    0.003
#define FLOWER_DENSITY     0.001
#define FISH_DENSITY       0.01
struct Sprite {
    uint   height;
    uint   width;
    uint   frameCount;
    char*  frameBuffer[];
};

const int PLOUF_TICKS_PER_FRAME = 1;
int PLOUF_LIFETIME = PLOUF_TICKS_PER_FRAME * 4;
int PLOUF_CHANCE = 100;
const int FROG_ARRAY_SIZE = 8;


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
    bool orientation; //true = V
    bool flipped;
};

struct Frog {
    int x, y;
    short color;
    short direction;
    int jumpiness; // general tendency of that frog to jump around
    int jump_distance; // will be applied twice, so it's really the half jump distance
    int jump_height;
    int croakiness; // you can guess
    int blinkiness;
    int eye_wetness; // hydration of the eyes. When reaches 0, triggers a blink.
    int urge_to_croak;
    int croak; // ticks left till croak end 0 = not croaking
    int urge_to_jump;
    int jumping; // 0 = not jumping, 2 = jumping, Y = Y - jump height, 1 = landing
    int height;
    bool swimming;
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

void set_up_waterlilies(short terrain[LINES][COLS], struct WaterLily lily_pad_array[], int n_waterlilies) {
    for (int i = 0; i < n_waterlilies; i++) {
        short color = GREEN; //rand() % 3 == 0 ? YELLOW : GREEN;
        int x, y;
        do {
            y = rand() % LINES;
            x = rand() % COLS;
        } while (terrain[y][x] != WATER);
        struct WaterLily newLily = {.x = x, .y = y, .color = color};
        lily_pad_array[i] = newLily;
        fill_terrain_with(terrain, lily_pad_leaf_ascii_new, PAD, y, x, lily_pad_leaf_offset_y, lily_pad_leaf_offset_x, false);
    }
}

void tick_frog(WINDOW* win, short terrain[LINES][COLS], struct Frog frogArray[], bool isIndexFree[]) {
    for (int i = 0; i < FROG_ARRAY_SIZE; i++) {
        if (!isIndexFree[i]) {
            struct Frog* frog = &frogArray[i];
            bool blinking = false;
            frog->urge_to_jump--;
            if (frog->urge_to_jump <= 0) {
                frog->jumping = 2;
                frog->urge_to_jump = frog->jumpiness;
                //here, do change direction later.
                //a chance to stay on course, a chance to chose an adjacent direction
                //short next_direction(current, flip); flip = true : we go one to the left instead of one to the right
            }

            int y = frog->y; // in a variable bc we might want to alter it for the jump, and not alter frog.y
            char** str_to_render;
            if (frog->jumping > 0) {
                int x_direction_multiplier = (frog->direction & 0b11) == RIGHT ? 1 : (frog->direction & 0b11) == LEFT ? -1 : 0;
                int y_direction_multiplier = (frog->direction & 0b1100) == DOWN ? 1 : (frog->direction & 0b1100) == UP ? -1 : 0;
                frog->x += frog->jump_distance * x_direction_multiplier;
                frog->y += frog->jump_distance * y_direction_multiplier;
                y = frog->y;
                if (frog->jumping == 2) {
                    y -= frog->jump_height;
                }

                str_to_render = &ascii_frog;
                frog->jumping--;

                // landing
                if (frog->jumping == 0) {
                    if(terrain[frog->y][frog->x] == WATER) {
                        frog->swimming = true;
                        addPlouf(frog->x, frog->y);
                    } else {
                        frog->swimming = false;
                    }
                }
            } else {// the jump cancels everything that comes afterwards
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
                if (!frog->swimming)
                    str_to_render = frog->croak > 0 ? &frog_croak : blinking ? &frog_blink : &ascii_frog;
                else {
                    str_to_render = blinking ? &from_swimming_blinking : &frog_swimming;
                }
            }

           
            wattron(win, A_BOLD | COLOR_PAIR(frog->color));
            render_str(win, *str_to_render, y, frog->x, 2, 4, false);
            
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
            render_str(win, PLOUF_SPRITE.frameBuffer[plouf->currentFrame], plouf->y, plouf->x, 3, 8, true);
        }
    }
}

void spawn_frog(struct Frog frogArray[], bool isIndexFree[], int y, int x) {
    for (int i = 0; i < FROG_ARRAY_SIZE; i++) {
        if (isIndexFree[i]) {
            short color = rand() % 2 == 0 ? GREEN : rand() % 3 == 0 ? RED : YELLOW;
            int jumpiness = 20 + rand() % 200;
            struct Frog frog = {
                .x = x,
                .y = y,
                .color = color,
                .direction = LEFT,
                .eye_wetness = 20,
                .urge_to_croak = 50,
                .urge_to_jump = jumpiness,
                .jumping = 0,
                .jump_distance = 3,
                .jump_height = 3,
                .jumpiness = jumpiness,
                .croak = 0,
                .swimming = false,
            };
            frogArray[i] = frog;
            isIndexFree[i] = false;
            return;
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
    struct WaterLily lily_pad_array[n_lily_pad_leaves];
    set_up_waterlilies(terrain, lily_pad_array, n_lily_pad_leaves);

    //frogs !!!!
    struct Frog frog_array[FROG_ARRAY_SIZE];
    bool is_frog_array_index_free[FROG_ARRAY_SIZE];
    for (int i = 0; i < FROG_ARRAY_SIZE; i++) {
        is_frog_array_index_free[i] = true;
    }
    spawn_frog(frog_array, is_frog_array_index_free, lily_pad_array[0].y, lily_pad_array[0].x);

    bool quit = false;

    while (!quit) {
        wattron(win, COLOR_PAIR(NORMAL));
        wclear(win);

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
        for (int i = 0; i < n_lily_pad_leaves; i++) {
            struct WaterLily* lily_pad = &lily_pad_array[i];
            //wattron(win, waterLily->color);
            wattron(win, COLOR_PAIR(lily_pad->color));
            render_str(win, lily_pad_leaf_ascii_new, lily_pad->y, lily_pad->x, lily_pad_leaf_offset_y, lily_pad_leaf_offset_x, true);
            wattroff(win, COLOR_PAIR(lily_pad->color));
            //wattron(win, COLOR_PAIR(0));
        }
        wattron(win, A_BOLD | COLOR_PAIR(MAGENTA));
        render_str(win, waterlily_flower_small, 7, 20, 4, 7, true);
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
    for (int j = 0; j < LINES; j++) {
        for (int i = 0; i < COLS; i++) {
            printf("%d", terrain[j][i]);
        }
        printf("\n");
    }
}
