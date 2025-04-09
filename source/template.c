#include <grrlib.h>
#include <ogc/pad.h>
#include <stdlib.h>
#include <stdio.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 60
#define BALL_SIZE 60
#define PADDLE_SPEED 5
#define BALL_SPEED 1
#define PADDLE_COLOR 0xFFFFFFFF

// Paddle and ball structure
typedef struct {
    int x, y, w, h;
} Paddle;

typedef struct {
    int x, y, dx, dy, size;
} Ball;

void ResetBall(Ball *ball) {
    ball->x = SCREEN_WIDTH / 2;
    ball->y = SCREEN_HEIGHT / 2;
    ball->dx = (rand() % 2 == 0) ? BALL_SPEED : -BALL_SPEED;
    ball->dy = (rand() % 2 == 0) ? BALL_SPEED : -BALL_SPEED;
}

int main(void) {
    GRRLIB_Init();
    PAD_Init();

    // Load ball texture
    GRRLIB_texImg *ballTexture = GRRLIB_LoadTextureFromFile("/linux.png");
    if (!ballTexture) {
        printf("Error: Could not load ball texture!\n");
        return -1;
    }

    Paddle player1 = {20, SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2, PADDLE_WIDTH, PADDLE_HEIGHT};
    Paddle player2 = {SCREEN_WIDTH - 30, SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2, PADDLE_WIDTH, PADDLE_HEIGHT};
    Ball ball;
    ResetBall(&ball);

    int player1_score = 0, player2_score = 0;
    bool running = true;

    while (running) {
        PAD_ScanPads();
        u32 pressed = PAD_ButtonsDown(0);
        if (pressed & PAD_BUTTON_START) running = false;

        // Paddle movement
        u32 held = PAD_ButtonsHeld(0);
        if (held & PAD_BUTTON_UP && player1.y > 0) player1.y -= PADDLE_SPEED;
        if (held & PAD_BUTTON_DOWN && player1.y < SCREEN_HEIGHT - PADDLE_HEIGHT) player1.y += PADDLE_SPEED;

        // AI Movement
        if (ball.y < player2.y) player2.y -= PADDLE_SPEED;
        if (ball.y > player2.y + PADDLE_HEIGHT) player2.y += PADDLE_SPEED;

        // Ball movement
        ball.x += ball.dx;
        ball.y += ball.dy;

        // Collision with top/bottom
        if (ball.y <= 0 || ball.y >= SCREEN_HEIGHT - BALL_SIZE) ball.dy = -ball.dy;

        // Collision with paddles with angle variation
        if (ball.dx < 0 && ball.x <= player1.x + player1.w && ball.y + ball.size >= player1.y && ball.y <= player1.y + player1.h) {
            ball.x = player1.x + player1.w; // Prevent overlap
            ball.dx = -ball.dx;
            int hitPosition = (ball.y + ball.size / 2) - (player1.y + PADDLE_HEIGHT / 2);
            ball.dy = hitPosition / (PADDLE_HEIGHT / 4);
        }

        if (ball.dx > 0 && ball.x + ball.size >= player2.x && ball.y + ball.size >= player2.y && ball.y <= player2.y + player2.h) {
            ball.x = player2.x - ball.size; // Prevent overlap
            ball.dx = -ball.dx;
            int hitPosition = (ball.y + ball.size / 2) - (player2.y + PADDLE_HEIGHT / 2);
            ball.dy = hitPosition / (PADDLE_HEIGHT / 4);
        }

        // Scoring
        if (ball.x <= 0) { player2_score++; ResetBall(&ball); }
        if (ball.x >= SCREEN_WIDTH) { player1_score++; ResetBall(&ball); }

        // Rendering
        GRRLIB_FillScreen(0x000000FF);
        GRRLIB_Rectangle(player1.x, player1.y, player1.w, player1.h, PADDLE_COLOR, true);
        GRRLIB_Rectangle(player2.x, player2.y, player2.w, player2.h, PADDLE_COLOR, true);

        // Draw scaled ball texture
        float scaleX = (float)BALL_SIZE / ballTexture->w;
        float scaleY = (float)BALL_SIZE / ballTexture->h;
        GRRLIB_DrawImg(ball.x, ball.y, ballTexture, 0, scaleX, scaleY, PADDLE_COLOR);

        // Swap buffers
        GRRLIB_Render();
    }

    // Free texture memory
    GRRLIB_FreeTexture(ballTexture);
    GRRLIB_Exit();
    return 0;
}
