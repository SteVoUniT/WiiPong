#include <grrlib.h>
#include <ogc/pad.h>
#include <stdlib.h>
#include <stdio.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 60
#define BALL_SIZE 80
#define PADDLE_SPEED 7
#define BALL_SPEED 3
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

    // Load font using GRRLIB_LoadTTFFromFile
    GRRLIB_ttfFont *font = GRRLIB_LoadTTFFromFile("font.ttf");
    if (!font) {
        printf("Error: Could not load font.ttf!\n");
        GRRLIB_FreeTexture(ballTexture);
        GRRLIB_Exit();
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
            ball.x = player1.x + player1.w;
            ball.dx = -ball.dx;
            int hitPosition = (ball.y + ball.size / 2) - (player1.y + PADDLE_HEIGHT / 2);
            ball.dy = hitPosition / (PADDLE_HEIGHT / 4);
        }

        if (ball.dx > 0 && ball.x + ball.size >= player2.x && ball.y + ball.size >= player2.y && ball.y <= player2.y + player2.h) {
            ball.x = player2.x - ball.size;
            ball.dx = -ball.dx;
            int hitPosition = (ball.y + ball.size / 2) - (player2.y + PADDLE_HEIGHT / 2);
            ball.dy = hitPosition / (PADDLE_HEIGHT / 4);
        }

        // Scoring
        if (ball.x <= 0) { player2_score++; ResetBall(&ball); }
        if (ball.x >= SCREEN_WIDTH) { player1_score++; ResetBall(&ball); }

        // Rendering
        GRRLIB_FillScreen(0x000000FF); // Clear screen to black

        // Draw paddles and ball
        GRRLIB_Rectangle(player1.x, player1.y, player1.w, player1.h, PADDLE_COLOR, true);
        GRRLIB_Rectangle(player2.x, player2.y, player2.w, player2.h, PADDLE_COLOR, true);
        float scaleX = (float)BALL_SIZE / ballTexture->w;
        float scaleY = (float)BALL_SIZE / ballTexture->h;
        GRRLIB_DrawImg(ball.x, ball.y, ballTexture, 0, scaleX, scaleY, PADDLE_COLOR);

        // Draw score
        char *label1 = "PLAYER 1";
        char *label2 = "PLAYER 2";

        int label_font_size = 20;
        int score_font_size = 36;

        int label1_width = GRRLIB_WidthTTF(font, label1, label_font_size);
        int label2_width = GRRLIB_WidthTTF(font, label2, label_font_size);

        int padding = 60;
        int label1_x = SCREEN_WIDTH / 2 - padding - label1_width;
        int label2_x = SCREEN_WIDTH / 2 + padding;

        GRRLIB_PrintfTTF(label1_x, 20, font, label1, label_font_size, 0xB22234FF);
        GRRLIB_PrintfTTF(label2_x, 20, font, label2, label_font_size, 0x0033A0FF);

        char score1_text[8], score2_text[8];
        snprintf(score1_text, sizeof(score1_text), "%d", player1_score);
        snprintf(score2_text, sizeof(score2_text), "%d", player2_score);

        int score1_width = GRRLIB_WidthTTF(font, score1_text, score_font_size);
        int score2_width = GRRLIB_WidthTTF(font, score2_text, score_font_size);

        GRRLIB_PrintfTTF(label1_x + (label1_width - score1_width) / 2, 45, font, score1_text, score_font_size, 0xB22234FF);
        GRRLIB_PrintfTTF(label2_x + (label2_width - score2_width) / 2, 45, font, score2_text, score_font_size, 0x0033A0FF);

        GRRLIB_Render();
    }

    // Cleanup
    GRRLIB_FreeTTF(font);
    GRRLIB_FreeTexture(ballTexture);
    GRRLIB_Exit();
    return 0;
}
