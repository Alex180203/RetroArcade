#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_now.h>

#define BUZZER_PIN 25

TFT_eSPI tft = TFT_eSPI();

struct Character { int x, y, dx, dy; };
struct ButtonData { int dx; int dy; };

int lives = 3, score = 0, totalDots = 0;
Character pacman;
Character ghosts[2];

const int mazeWidth = 18; 
const int mazeHeight = 15;
const int cellSize = 13;

const int mazeXOffset = (240 - (mazeWidth * cellSize)) / 2;
const int mazeYOffset = 50; 

int maze[mazeHeight][mazeWidth] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1},
    {1,0,1,0,1,1,1,0,1,1,1,0,1,0,1,1,1,1},
    {1,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,0,1,0,1,1,1,0,1,1,1,1},
    {1,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,1},
    {1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,0,1,1},
    {1,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,1,1},
    {1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,0,1,1},
    {1,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,1,1},
    {1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,0,1,1},
    {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

int originalMaze[mazeHeight][mazeWidth];

void copyMaze() {
    for (int y = 0; y < mazeHeight; y++) {
        for (int x = 0; x < mazeWidth; x++) {
            maze[y][x] = originalMaze[y][x];
        }
    }
}
void saveOriginalMaze() {
    for (int y = 0; y < mazeHeight; y++) {
        for (int x = 0; x < mazeWidth; x++) {
            originalMaze[y][x] = maze[y][x];
        }
    }
}

void drawMaze() {
    tft.fillScreen(TFT_BLACK);
    totalDots = 0;
    for (int y = 0; y < mazeHeight; y++) {
        for (int x = 0; x < mazeWidth; x++) {
            int xPos = (x * cellSize) + mazeXOffset;
            int yPos = (y * cellSize) + mazeYOffset;
            
            if (maze[y][x] == 1) 
                tft.fillRect(xPos + 1, yPos + 1, cellSize - 2, cellSize - 2, TFT_BLUE);
            else if (maze[y][x] == 0) {
                tft.fillCircle(xPos + 6, yPos + 6, 2, TFT_WHITE);
                totalDots++;
            }
        }
    }
}

void updateDisplay() {
    drawMaze();
    
    tft.fillCircle(pacman.x, pacman.y, 6, TFT_YELLOW);
    for (int i = 0; i < 2; i++)
        tft.fillCircle(ghosts[i].x, ghosts[i].y, 6, TFT_RED);
        
    tft.setCursor(10, 0);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.print("Lives: "); 
    tft.setCursor(10, 30);
    tft.print(lives);
    tft.setCursor(130, 0);
    tft.print("Score: "); 
    tft.setCursor(130,30);
    tft.print(score);
}

bool isValidMove(int x, int y) {
    int mazeX = (x - mazeXOffset) / cellSize;
    int mazeY = (y - mazeYOffset) / cellSize;
    
    if (mazeX < 0 || mazeX >= mazeWidth || mazeY < 0 || mazeY >= mazeHeight) {
        return false;
    }
    
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
        randX = random(1, mazeWidth - 1);
        randY = random(1, mazeHeight - 1);
        
        int xPos = (randX * cellSize) + mazeXOffset;
        int yPos = (randY * cellSize) + mazeYOffset;
        
        int centerX = xPos + (cellSize / 2);
        int centerY = yPos + (cellSize / 2);

        if (isValidMove(centerX, centerY) && !isCollision(centerX, centerY)) {
            validPosition = true;
            character.x = centerX;
            character.y = centerY;
        }
    }
}

void moveGhost(Character &ghost) {
    int direction = random(0, 4);
    int newX = ghost.x;
    int newY = ghost.y;

    if (direction == 0) newY -= cellSize;
    else if (direction == 1) newX += cellSize;
    else if (direction == 2) newY += cellSize;
    else if (direction == 3) newX -= cellSize;

    if (isValidMove(newX, newY)) {
        ghost.x = newX;
        ghost.y = newY;
    }
}

void resetGame() {
    Serial.println(">>> RESETTING GAME <<<");
    lives = 3;
    score = 0;
    
    copyMaze();
    
    randomizeStartPosition(pacman);
    randomizeStartPosition(ghosts[0]);
    randomizeStartPosition(ghosts[1]);
    
    updateDisplay();
}

void movePacman(int dx, int dy) {
    int newX = pacman.x + dx * cellSize;
    int newY = pacman.y + dy * cellSize;

    if (isValidMove(newX, newY)) {
        int mazeX = (newX - mazeXOffset) / cellSize;
        int mazeY = (newY - mazeYOffset) / cellSize;

        if (maze[mazeY][mazeX] == 0) {
            score += 10;
            maze[mazeY][mazeX] = 2; // Dot eaten
            totalDots--;
        }
        pacman.x = newX;
        pacman.y = newY;
        
        for (int i = 0; i < 2; i++) {
            moveGhost(ghosts[i]);
        }
        updateDisplay(); // Redraw AFTER moving
    }

    if (totalDots == 0) {
        resetGame(); // Simple restart if win
    }

    for (int i = 0; i < 2; i++) {
        if (abs(ghosts[i].x - pacman.x) < cellSize && abs(ghosts[i].y - pacman.y) < cellSize) {
            lives--;
           
            randomizeStartPosition(pacman);
            randomizeStartPosition(ghosts[0]);
            randomizeStartPosition(ghosts[1]);
            updateDisplay();
           
            if (lives == 0) {
                tft.fillScreen(TFT_BLACK);
                tft.setCursor(39, 110);
                tft.setTextSize(3);
                tft.setTextColor(TFT_RED);
                tft.print("Game Over");
                delay(3000); // Wait a bit
                resetGame(); // Restart automatically
            }
        }
    }
}

void onDataReceived(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
    ButtonData data;
    memcpy(&data, incomingData, sizeof(data));
    
    Serial.print("RX: dx="); Serial.print(data.dx); Serial.print(" dy="); Serial.println(data.dy);

    // --- CHECK FOR RESET COMMAND ---
    if (data.dx == 99 && data.dy == 99) {
        resetGame();
        return;
    }

    // Normal Movement
    movePacman(data.dx, data.dy);
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    
    Serial.println("ESP32 Receiver Ready");
    Serial.print("MAC: "); Serial.println(WiFi.macAddress());

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed!");
        return;
    }
    esp_now_register_recv_cb(onDataReceived);
    
    tft.init();
    tft.setRotation(1);
    
    saveOriginalMaze(); // Backup the map so we can reset it later
    resetGame();        // Initial start
}

void loop() {
    // Empty
}