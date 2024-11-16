// Dino Game for CH32V003
// Based on wokwi project https://wokwi.com/projects/346178932556431954
// Ported by ohdarling

#include <stdio.h>
#include "driver.h"
#include "sprints.h"

#define delay DLY_ms

uint32_t sys_millis = 0;
uint32_t sys_last_tick = 0;

int millis() {
    int ms = (STK->CNT - sys_last_tick) / DLY_MS_TIME;
    sys_last_tick = STK->CNT;
    sys_millis += ms;
    return sys_millis;
}

void tone(int pin, int freq, int duration) {
    JOY_sound(freq, duration);
}

// ===================================================================================
// Global Variables
// ===================================================================================

void oled_screen_begin() {
    display_init();
    display_clear();
    display_flush();
}

void oled_screen_drawStr(int x, int y, const char* str) {
    display_draw_str(x, y, str);
}

void oled_screen_drawXBMP(int x, int y, int w, int h, const uint8_t* data) {
    display_draw_xbmp(x, y, w, h, data);
}

// Sounds we use for the hit effects
#define jumpSound 700
#define blahSound 125
#define speedSound 1000
#define DBOUNCE 180

// Game states
#define gameStart 0
#define gameEnd 1
#define gamePlaying 2

#define buzzer 0

volatile int gameStatus = gameStart;

// Other Game attributes

// various variables
int currentStateCLK;
int lastStateCLK;
int MyScore = 0;
int dinoMove = 0;
volatile int jumping = 0;
int cloudx = 128;
int obstacles[2] = {1, 4};
int obstaclex[2] = {128, 200};
int speed = 6;
unsigned long startTime = 0, curTime;
int lastBeep = 0;

// ===================================================================================
// Function Prototypes
// ===================================================================================
void draw();
void moveDino();
void moveCloud();
void moveObstacles();
void checkCollision();
void u8g_prepare();
void drawDino();
void drawShape(int, int);
void drawObsticles();
void resetGame();

// ===================================================================================
// Main Function
// ===================================================================================
int main(void) {
    // Setup
    JOY_init();
    oled_screen_begin();
    resetGame();

    // Loop
    while (1) {
        millis();
        display_clear();
        if (JOY_act_pressed()) {
            if (gameStatus == gamePlaying) {
                if (jumping == 0) {
                    jumping = 1;
                    tone(buzzer, jumpSound, 100);
                }
            } else if (gameStatus == gameStart) {
                gameStatus = gamePlaying;
            } else {
                gameStatus = gameStart;
            }
        }
        draw();
        display_flush();
        if (gameStatus == gamePlaying) {
            moveDino();
            moveCloud();
            moveObstacles();
        }
        DLY_ms(15);
    }
}

// ===================================================================================
// Functions
// ===================================================================================

// Display the score on the 7seg display
void ShowScore() {
    char buf[20];
    sprintf(buf, "%d", MyScore);
    oled_screen_drawStr(0, 0, buf);

    if (gameStatus == gamePlaying) {
        curTime = millis();
        MyScore = (curTime - startTime) * speed / 1000;
        // if (MyScore > lastBeep) {
        //     tone(buzzer, 1000, 100);
        //     delay(150);
        //     tone(buzzer, 1250, 100);
        //     lastBeep = MyScore;
        // }
    }
}

void StartStopGame() {
    static unsigned long last_interrupt = 0;
    if (millis() - last_interrupt > DBOUNCE) {
        if (gameStatus == gamePlaying) {
            if (jumping == 0) {
                jumping = 1;
                tone(buzzer, jumpSound, 100);
            }
        } else if (gameStatus == gameStart) {
            gameStatus = gamePlaying;
        } else {
            gameStatus = gameStart;
        }
    }
    last_interrupt = millis();  // note the last time the ISR was called
}

void resetGame() {
    MyScore = 0;
    startTime = millis();
    obstaclex[0] = 128;
    obstaclex[1] = 200;
    dinoMove = 0;
}

void moveDino() {
    checkCollision();
    if (gameStatus == gameEnd) {
        return;
    }

    if (jumping == 0)
        dinoMove = (dinoMove + 1) % 3;
    else {
        if (jumping == 1) {
            dinoMove = dinoMove + 8;
            if (dinoMove > 32)
                jumping = 2;
        } else {
            dinoMove = dinoMove - 8;
            if (dinoMove < 8) {
                jumping = 0;
                dinoMove = 0;
            }
        }
    }
}

void moveCloud() {
    cloudx--;
    if (cloudx < -38)
        cloudx = 128;
}

void moveObstacles() {
    int obx = obstaclex[0];
    obx = obx - speed;
    if (obx < -20) {
        obstaclex[0] = obstaclex[1];
        obstaclex[1] = obstaclex[0] + (JOY_random() % 46) + 80;
        obstacles[0] = obstacles[1];
        obstacles[1] = (JOY_random() % 6) + 1;
    } else {
        obstaclex[0] = obx;
        obstaclex[1] -= speed;
    }
}
// ************************************************

void draw(void) {
    if (gameStatus == gamePlaying) {
        ShowScore();
        drawDino();
        drawShape(0, cloudx);
        drawObsticles();
    } else if (gameStatus == gameStart) {
        oled_screen_drawStr(0, 8, "Welcome to");
        oled_screen_drawStr(10, 24, "Dino!!");
        oled_screen_drawStr(0, 40, "Push to begin");
        resetGame();
    } else {
        ShowScore();
        oled_screen_drawXBMP(14, 12, 100, 15, gameOver);
        drawDino();
        drawShape(0, cloudx);
        drawObsticles();
    }
}

void drawDino(void) {
    if (gameStatus == gameEnd) {
        oled_screen_drawXBMP(0, 43 - dinoMove, 20, 21, dinoBlah);
        return;
    }
    switch (dinoMove) {
        case -1:
            oled_screen_drawXBMP(0, 43, 20, 21, dinoBlah);
            break;
        case 0:
            oled_screen_drawXBMP(0, 43, 20, 21, dinoJump);
            break;
        case 1:
            oled_screen_drawXBMP(0, 43, 20, 21, dinoLeft);
            break;
        case 2:
            oled_screen_drawXBMP(0, 43, 20, 21, dinoRight);
            break;
        default:
            oled_screen_drawXBMP(0, 43 - dinoMove, 20, 21, dinoJump);
            break;
    }
}

void drawShape(int shape, int x) {
    switch (shape) {
        case 0:
            oled_screen_drawXBMP(x, 5, 39, 12, cloud);
            break;
        case 1:
            oled_screen_drawXBMP(x, 44, 10, 20, oneCactus);
            break;
        case 2:
            oled_screen_drawXBMP(x, 44, 20, 20, twoCactus);
            break;
        case 3:
            oled_screen_drawXBMP(x, 44, 20, 20, threeCactus);
            break;
        case 4:
            oled_screen_drawXBMP(x, 52, 6, 12, oneCactusSmall);
            break;
        case 5:
            oled_screen_drawXBMP(x, 52, 12, 12, twoCactusSmall);
            break;
        case 6:
            oled_screen_drawXBMP(x, 52, 17, 12, threeCactusSmall);
            break;
    }
}

void checkCollision() {
    if (gameStatus != gamePlaying) {
        return;
    }

    int obx = obstaclex[0];
    int obw, obh;

    switch (obstacles[0]) {
        case 0:
            obw = 39;
            obh = 10;
            break;
        case 1:
            obw = 10;
            obh = 20;
            break;
        case 2:
            obw = 17;
            obh = 20;
            break;
        case 3:
            obw = 17;
            obh = 20;
            break;
        case 4:
            obw = 6;
            obh = 12;
            break;
        case 5:
            obw = 12;
            obh = 12;
            break;
        case 6:
            obw = 17;
            obh = 12;
            break;
    }
    if (obx > 15 || obx + obw < 5 || dinoMove > obh - 3) {
    } else {
        gameStatus = gameEnd;
        tone(buzzer, 125, 100);
        delay(150);
        tone(buzzer, 125, 100);
    }
}
void drawObsticles() {
    drawShape(obstacles[0], obstaclex[0]);
    drawShape(obstacles[1], obstaclex[1]);
}
