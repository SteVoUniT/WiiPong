#include <grrlib.h>
#include <ogc/pad.h>
#include <stdlib.h>
#include <stdio.h>
#include <ogc/lwp_watchdog.h> // For timer functions

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 60
#define BALL_SIZE 80
#define PADDLE_SPEED 7
#define BALL_SPEED 3
#define PADDLE_COLOR 0xFFFFFFFF
#define WINNING_SCORE 5

// Game states
typedef enum {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_GAME_OVER
} GameState;

// Menu options
typedef enum {
    MENU_START_GAME,
    MENU_OPTIONS,
    MENU_EXIT,
    MENU_OPTION_COUNT
} MenuOption;

// Structure for paddles
typedef struct {
    int x, y, w, h;
} Paddle;

// Structure for the ball
typedef struct {
    int x, y, dx, dy, size;
} Ball;

// Resets the ball to the center with random direction
void ResetBall(Ball *ball) {
    ball->x = SCREEN_WIDTH / 2;
    ball->y = SCREEN_HEIGHT / 2;
    ball->dx = (rand() % 2 == 0) ? BALL_SPEED : -BALL_SPEED;
    ball->dy = (rand() % 2 == 0) ? BALL_SPEED : -BALL_SPEED;
}

// Entry point
int main(void) {
    GRRLIB_Init();
    PAD_Init();

    // Set random seed
    srand(gettime());

    // Load texture for ball image
    GRRLIB_texImg *ballTexture = GRRLIB_LoadTextureFromFile("/linux.png");
    if (!ballTexture) {
        printf("Error: Could not load ball texture!\n");
        return -1;
    }

    // Load title screen logo
    GRRLIB_texImg *logoTexture = GRRLIB_LoadTextureFromFile("/logo.png");
    // If logo can't be loaded, we'll draw text instead

    // Load TTF font
    GRRLIB_ttfFont *font = GRRLIB_LoadTTFFromFile("font.ttf");
    if (!font) {
        printf("Error: Could not load font.ttf!\n");
        GRRLIB_FreeTexture(ballTexture);
        if (logoTexture) GRRLIB_FreeTexture(logoTexture);
        GRRLIB_Exit();
        return -1;
    }

    // Initialize players and ball
    Paddle player1 = {20, SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2, PADDLE_WIDTH, PADDLE_HEIGHT};
    Paddle player2 = {SCREEN_WIDTH - 30, SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2, PADDLE_WIDTH, PADDLE_HEIGHT};
    Ball ball;
    ResetBall(&ball);

    int player1_score = 0, player2_score = 0;
    bool running = true;

    // Title screen variables
    GameState gameState = STATE_TITLE;
    MenuOption selectedOption = MENU_START_GAME;
    int menuY = SCREEN_HEIGHT / 2 + 40;
    int menuSpacing = 40;

    // Animation variables for title screen
    float titleBallX = SCREEN_WIDTH / 4;
    float titleBallY = SCREEN_HEIGHT / 4;
    float titleBallDX = 2.0f;
    float titleBallDY = 1.5f;

    // Menu option labels
    const char* menuOptions[MENU_OPTION_COUNT] = {
        "Start Game",
        "Options",
        "Exit"
    };

    // Main game loop
    while (running) {
        PAD_ScanPads();
        u32 pressed = PAD_ButtonsDown(0);
        u32 held = PAD_ButtonsHeld(0);

        // Global exit game
        if (pressed & PAD_BUTTON_START && (held & PAD_BUTTON_B)) running = false;

        // State-specific logic
        switch (gameState) {
            case STATE_TITLE:
                // Title screen menu navigation
                if (pressed & PAD_BUTTON_UP) {
                    selectedOption = (selectedOption == 0) ? MENU_OPTION_COUNT - 1 : selectedOption - 1;
                }
                if (pressed & PAD_BUTTON_DOWN) {
                    selectedOption = (selectedOption + 1) % MENU_OPTION_COUNT;
                }

                // Menu selection
                if (pressed & PAD_BUTTON_A) {
                    switch (selectedOption) {
                        case MENU_START_GAME:
                            // Reset game state and start playing
                            player1_score = 0;
                            player2_score = 0;
                            player1.y = SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2;
                            player2.y = SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2;
                            ResetBall(&ball);
                            gameState = STATE_PLAYING;
                            break;
                        case MENU_OPTIONS:
                            // Options menu would go here - not implemented yet
                            break;
                        case MENU_EXIT:
                            running = false;
                            break;
                    }
                }

                // Animate ball on title screen
                titleBallX += titleBallDX;
                titleBallY += titleBallDY;

                // Bounce off edges
                if (titleBallX <= 0 || titleBallX >= SCREEN_WIDTH - BALL_SIZE/2) {
                    titleBallDX = -titleBallDX;
                }
                if (titleBallY <= 0 || titleBallY >= SCREEN_HEIGHT - BALL_SIZE/2) {
                    titleBallDY = -titleBallDY;
                }
                break;

                        case STATE_PLAYING:
                            // Player 1 controls
                            if (held & PAD_BUTTON_UP && player1.y > 0) player1.y -= PADDLE_SPEED;
                            if (held & PAD_BUTTON_DOWN && player1.y < SCREEN_HEIGHT - PADDLE_HEIGHT) player1.y += PADDLE_SPEED;

                            // Simple AI for Player 2
                            if (ball.y < player2.y + player2.h/2 - 10) player2.y -= PADDLE_SPEED;
                            if (ball.y > player2.y + player2.h/2 + 10) player2.y += PADDLE_SPEED;

                            // Keep paddles on screen
                            if (player1.y < 0) player1.y = 0;
                            if (player1.y > SCREEN_HEIGHT - PADDLE_HEIGHT) player1.y = SCREEN_HEIGHT - PADDLE_HEIGHT;
                            if (player2.y < 0) player2.y = 0;
                            if (player2.y > SCREEN_HEIGHT - PADDLE_HEIGHT) player2.y = SCREEN_HEIGHT - PADDLE_HEIGHT;

                            // Ball movement
                            ball.x += ball.dx;
            ball.y += ball.dy;

            // Bounce off top/bottom walls
            if (ball.y <= 0 || ball.y >= SCREEN_HEIGHT - BALL_SIZE) ball.dy = -ball.dy;

            // Player 1 collision
            if (ball.dx < 0 &&
                ball.x < player1.x + player1.w &&
                ball.x + ball.size > player1.x &&
                ball.y + ball.size > player1.y &&
                ball.y < player1.y + player1.h) {

                ball.x = player1.x + player1.w; // resolve overlap
                ball.dx = -ball.dx;
            int hitPos = (ball.y + ball.size / 2) - (player1.y + player1.h / 2);
            ball.dy = hitPos / (PADDLE_HEIGHT / 4);
                }

                // Player 2 collision
                if (ball.dx > 0 &&
                    ball.x + ball.size > player2.x &&
                    ball.x < player2.x + player2.w &&
                    ball.y + ball.size > player2.y &&
                    ball.y < player2.y + player2.h) {

                    ball.x = player2.x - ball.size; // resolve overlap
                    ball.dx = -ball.dx;
                int hitPos = (ball.y + ball.size / 2) - (player2.y + player2.h / 2);
                ball.dy = hitPos / (PADDLE_HEIGHT / 4);
                    }

                    // Scoring
                    if (ball.x <= 0) {
                        player2_score++;
                        ResetBall(&ball);
                    }
                    if (ball.x >= SCREEN_WIDTH) {
                        player1_score++;
                        ResetBall(&ball);
                    }

                    // Game over condition
                    if (player1_score >= WINNING_SCORE || player2_score >= WINNING_SCORE) {
                        gameState = STATE_GAME_OVER;
                    }
                    break;

                    case STATE_GAME_OVER:
                        // Restart game
                        if (pressed & PAD_BUTTON_A) {
                            gameState = STATE_TITLE;
                        }
                        // Back to title screen
                        if (pressed & PAD_BUTTON_B) {
                            gameState = STATE_TITLE;
                        }
                        break;
        }

        // --- Rendering ---
        GRRLIB_FillScreen(0x000000FF); // clear screen to black

        // Draw based on game state
        switch (gameState) {
            case STATE_TITLE:
                // Draw title logo or text
                if (logoTexture) {
                    float scaleX = 0.5f;
                    float scaleY = 0.5f;
                    GRRLIB_DrawImg(
                        SCREEN_WIDTH/2 - (logoTexture->w * scaleX)/2,
                                   SCREEN_HEIGHT/4 - (logoTexture->h * scaleY)/2,
                                   logoTexture, 0, scaleX, scaleY, 0xFFFFFFFF
                    );
                } else {
                    // Draw text title if no logo is available
                    const char* title = "WII PONG";
                    int titleSize = 60;
                    int titleWidth = GRRLIB_WidthTTF(font, title, titleSize);
                    GRRLIB_PrintfTTF(
                        SCREEN_WIDTH/2 - titleWidth/2,
                        SCREEN_HEIGHT/4 - titleSize/2,
                        font, title, titleSize, 0xFFFFFFFF
                    );
                }

                // Draw animated ball
                float scaleX = (float)BALL_SIZE / 2 / ballTexture->w;
                float scaleY = (float)BALL_SIZE / 2 / ballTexture->h;
                GRRLIB_DrawImg(titleBallX, titleBallY, ballTexture, 0, scaleX, scaleY, PADDLE_COLOR);

                // Draw menu options
                for (int i = 0; i < MENU_OPTION_COUNT; i++) {
                    uint32_t color = (i == selectedOption) ? 0xFFFF00FF : 0xCCCCCCFF;
                    int fontSize = (i == selectedOption) ? 28 : 24;
                    int textWidth = GRRLIB_WidthTTF(font, menuOptions[i], fontSize);

                    // Draw selector arrow if selected
                    if (i == selectedOption) {
                        GRRLIB_PrintfTTF(
                            SCREEN_WIDTH/2 - textWidth/2 - 30,
                            menuY + i * menuSpacing,
                            font, ">", fontSize, color
                        );
                    }

                    GRRLIB_PrintfTTF(
                        SCREEN_WIDTH/2 - textWidth/2,
                        menuY + i * menuSpacing,
                        font, menuOptions[i], fontSize, color
                    );
                }

                // Draw instructions
                const char* instructions = "Navigate with UP/DOWN, select with A";
                int instructionsSize = 16;
                int instructionsWidth = GRRLIB_WidthTTF(font, instructions, instructionsSize);
                GRRLIB_PrintfTTF(
                    SCREEN_WIDTH/2 - instructionsWidth/2,
                    SCREEN_HEIGHT - 40,
                    font, instructions, instructionsSize, 0xCCCCCCFF
                );
                break;

                case STATE_PLAYING:
                    // Draw center line
                    for (int y = 0; y < SCREEN_HEIGHT; y += 15) {
                        GRRLIB_Rectangle(
                            SCREEN_WIDTH/2 - 2,
                            y,
                            4,
                            10,
                            0x888888FF,
                            true
                        );
                    }

                    // Draw paddles
                    GRRLIB_Rectangle(player1.x, player1.y, player1.w, player1.h, PADDLE_COLOR, true);
                    GRRLIB_Rectangle(player2.x, player2.y, player2.w, player2.h, PADDLE_COLOR, true);

                    // Draw ball with scaling
                    scaleX = (float)BALL_SIZE / ballTexture->w;
                    scaleY = (float)BALL_SIZE / ballTexture->h;
                    GRRLIB_DrawImg(ball.x, ball.y, ballTexture, 0, scaleX, scaleY, PADDLE_COLOR);

                    // Draw scores
                    char *label1 = "PLAYER 1";
                    char *label2 = "PLAYER 2";
                    int label_font_size = 20;
                    int score_font_size = 36;

                    int label1_width = GRRLIB_WidthTTF(font, label1, label_font_size);
                    int label2_width = GRRLIB_WidthTTF(font, label2, label_font_size);
                    int padding = 60;

                    int label1_x = SCREEN_WIDTH / 2 - padding - label1_width;
                    int label2_x = SCREEN_WIDTH / 2 + padding;

                    GRRLIB_PrintfTTF(label1_x, 20, font, label1, label_font_size, 0xB22234FF); // red
                    GRRLIB_PrintfTTF(label2_x, 20, font, label2, label_font_size, 0x0033A0FF); // blue

                    char score1_text[8], score2_text[8];
                    snprintf(score1_text, sizeof(score1_text), "%d", player1_score);
                    snprintf(score2_text, sizeof(score2_text), "%d", player2_score);

                    int score1_width = GRRLIB_WidthTTF(font, score1_text, score_font_size);
                    int score2_width = GRRLIB_WidthTTF(font, score2_text, score_font_size);

                    GRRLIB_PrintfTTF(label1_x + (label1_width - score1_width) / 2, 45, font, score1_text, score_font_size, 0xB22234FF);
                    GRRLIB_PrintfTTF(label2_x + (label2_width - score2_width) / 2, 45, font, score2_text, score_font_size, 0x0033A0FF);
                    break;

                case STATE_GAME_OVER:
                    // Draw semi-transparent overlay
                    GRRLIB_Rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00000088, true);

                    // Show winner
                    const char* winner = player1_score > player2_score ? "PLAYER 1 WINS!" : "PLAYER 2 WINS!";
                    int winnerSize = 30;
                    int winnerWidth = GRRLIB_WidthTTF(font, winner, winnerSize);
                    GRRLIB_PrintfTTF(
                        SCREEN_WIDTH / 2 - winnerWidth / 2,
                        SCREEN_HEIGHT / 2 - 40,
                        font, winner, winnerSize, 0xFFFFFFFF
                    );

                    // Draw final score
                    char finalScore[32];
                    snprintf(finalScore, sizeof(finalScore), "Score: %d - %d", player1_score, player2_score);
                    int scoreSize = 24;
                    int scoreWidth = GRRLIB_WidthTTF(font, finalScore, scoreSize);
                    GRRLIB_PrintfTTF(
                        SCREEN_WIDTH / 2 - scoreWidth / 2,
                        SCREEN_HEIGHT / 2,
                        font, finalScore, scoreSize, 0xFFFFFFFF
                    );

                    // Draw instructions
                    const char* gameOverInstr = "Press A to return to title screen";
                    int instrSize = 20;
                    int instrWidth = GRRLIB_WidthTTF(font, gameOverInstr, instrSize);
                    GRRLIB_PrintfTTF(
                        SCREEN_WIDTH / 2 - instrWidth / 2,
                        SCREEN_HEIGHT / 2 + 50,
                        font, gameOverInstr, instrSize, 0xFFFFFFFF
                    );
                    break;
        }

        GRRLIB_Render();
    }

    // Cleanup
    GRRLIB_FreeTTF(font);
    GRRLIB_FreeTexture(ballTexture);
    if (logoTexture) GRRLIB_FreeTexture(logoTexture);
    GRRLIB_Exit();
    return 0;
}
