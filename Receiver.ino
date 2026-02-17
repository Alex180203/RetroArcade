#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_now.h>

#define BUZZER_PIN 25

TFT_eSPI tft = TFT_eSPI();

struct Character { int x, y, dx, dy; };
struct ButtonData { int dx; int dy; };

int lives = 3, score = 0, totalDots = 0;
Character pacman; // Pac-Man starting in the center
Character ghosts[2]; // Two ghosts starting in the center

const int mazeWidth = 20;
const int mazeHeight = 15;
const int cellSize = 16;

// 20x15 Maze Layout (1 = Wall, 0 = Path, 2 = Eaten dot)
int maze[mazeHeight][mazeWidth] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,1,0,0,0,1,0,0,0,1,0,1,0,0,0,0,1},
    {1,0,1,0,1,1,1,0,1,1,1,0,1,0,1,1,1,1,0,1},
    {1,0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,1},
    {1,0,1,1,1,1,1,0,1,0,1,1,1,0,1,1,1,1,0,1},
    {1,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,1},
    {1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,0,1,1,1,1},
    {1,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,1,0,0,1},
    {1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,0,1,0,1,1},
    {1,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,1},
    {1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,0,1,1,1,1},
    {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

void drawMaze() {
    tft.fillScreen(TFT_BLACK);
    totalDots = 0;
    for (int y = 0; y < mazeHeight; y++) {
        for (int x = 0; x < mazeWidth; x++) {
            int xPos = x * cellSize;
            int yPos = y * cellSize;
            if (maze[y][x] == 1) 
                tft.fillRect(xPos + 2, yPos + 2, cellSize - 4, cellSize - 4, TFT_BLUE);
            else if (maze[y][x] == 0) {
                tft.fillCircle(xPos + 8, yPos + 8, 2, TFT_WHITE);
                totalDots++;
            }
        }
    }
}

void updateDisplay() {
    drawMaze();
    tft.fillCircle(pacman.x, pacman.y, 8, TFT_YELLOW);
    for (int i = 0; i < 2; i++)
        tft.fillCircle(ghosts[i].x, ghosts[i].y, 8, TFT_RED);
    tft.setCursor(10, 10);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.print("Lives: "); tft.print(lives);
    tft.setCursor(180, 10);
    tft.print("Score: "); tft.print(score);
}

bool isValidMove(int x, int y) {
    int mazeX = x / cellSize, mazeY = y / cellSize;
    return (maze[mazeY][mazeX] == 0 || maze[mazeY][mazeX] == 2);
}
bool isCollision(int x, int y) {
    for (int i = 0; i < 2; i++) {
        if (abs(ghosts[i].x - x) < cellSize && abs(ghosts[i].y - y) < cellSize) {
            return true;
        }
    }
    return false;
}
void randomizeStartPosition(Character &character) {
    int randX, randY;
    bool validPosition = false;
    while (!validPosition) {
        randX = random(1, mazeWidth - 1); // Random x-coordinate (excluding walls)
        randY = random(1, mazeHeight - 1); // Random y-coordinate (excluding walls)
        
        int xPos = randX * cellSize;
        int yPos = randY * cellSize;
        
        if (isValidMove(xPos, yPos) && !isCollision(xPos, yPos)) {
            validPosition = true;
            character.x = xPos;
            character.y = yPos;
        }
    }
}
void moveGhost(Character &ghost) {
    // Randomly move the ghost to nearby cells, ensuring it doesn't go outside the grid.
    int direction = random(0, 4);  // Random direction: 0=up, 1=right, 2=down, 3=left
    
    int newX = ghost.x;
    int newY = ghost.y;

    if (direction == 0) newY -= cellSize; // Up
    else if (direction == 1) newX += cellSize; // Right
    else if (direction == 2) newY += cellSize; // Down
    else if (direction == 3) newX -= cellSize; // Left

    // Ensure the ghost stays within bounds and on a valid cell
    if (isValidMove(newX, newY)) {
        ghost.x = newX;
        ghost.y = newY;
    }
}

void movePacman(int dx, int dy) {
    int newX = pacman.x + dx * cellSize;
    int newY = pacman.y + dy * cellSize;
    int mazeX = newX / cellSize;
    int mazeY = newY / cellSize;

    // Check if the new position is a valid move
    if (isValidMove(newX, newY)) {
        if (maze[mazeY][mazeX] == 0) {
            score += 10; // Increment score for eating a dot
            maze[mazeY][mazeX] = 2; // Mark dot as eaten
            totalDots--;
        }
        pacman.x = newX;
        pacman.y = newY;
        updateDisplay(); // Update display after the move

        // After Pac-Man moves, update the ghosts' positions independently
        for (int i = 0; i < 2; i++) {
            moveGhost(ghosts[i]);
        }
    }

    // Check if Pac-Man has eaten all the dots and needs to reset
    if (totalDots == 0) {
        drawMaze(); // Reset maze with all dots
    }

    // Check for collisions with ghosts
    for (int i = 0; i < 2; i++) {
        if (abs(ghosts[i].x - pacman.x) < cellSize && abs(ghosts[i].y - pacman.y) < cellSize) {
            lives--; // Decrease lives when Pac-Man collides with a ghost
           
          randomizeStartPosition(pacman);
           randomizeStartPosition(ghosts[0]);
           randomizeStartPosition(ghosts[1]);
           updateDisplay(); // Update display after collision
           
            if (lives == 0) {
                // Game over condition
                tft.fillScreen(TFT_BLACK);
                tft.setCursor(60, 80);
                tft.setTextSize(3);
                tft.setTextColor(TFT_RED);
                tft.print("Game Over");
                while (1); // Halt the game
            }
        }
    }
}

void onDataReceived(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
    ButtonData data;
    memcpy(&data, incomingData, sizeof(data));
    movePacman(data.dx, data.dy);
}

void setup() {
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed!");
        return;
    }
    esp_now_register_recv_cb(onDataReceived);
    tft.init();
    tft.setRotation(1);
    randomizeStartPosition(pacman);
    randomizeStartPosition(ghosts[0]);
    randomizeStartPosition(ghosts[1]);
    drawMaze();
    updateDisplay();
}

void loop() {
    // Other game logic here (e.g., player controls, collision checks, etc.)
    // Movement logic is handled in the movePacman function
}
