/*
 * Zachary Blais
 * Arduino-Pong-v1.0
 * 11/7/2025
 *
 * A simple 2-player pong game designed for SSD1306 I2C OLED screens. Game contains a main menu and a pause/resume feature.
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Display Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pins
const int BUTTON_PIN = 3;
const int BUZZER_PIN = 2;
const int PLAYER_ONE = A1;
const int PLAYER_TWO = A2;

// Game State
bool isMainMenu = true;
bool isGamePaused = false;

// Score State
int scoreOne = 0;
int scoreTwo = 0;
const int WINNING_SCORE = 8;

// Paddle and Ball States
int playerOneY = 25;
int playerTwoY = 25;
const int PLAYER_ONE_X = 11;
const int PLAYER_TWO_X = 115;
int ballX = SCREEN_WIDTH / 2;
int ballY = SCREEN_HEIGHT / 2;
int ballDirX = 1;
int ballDirY = 1;
int prevBallX = ballX;
int prevBallY = ballY;

// Debounce and Button Timing
bool lastButtonState = HIGH;
bool buttonStateStable = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

unsigned long buttonPressStart = 0;
bool buttonHeld = false;

// State Tracking
bool lastMenuState = true;
bool lastPauseState = false;

// Function Definitions
void handleButton();
void mainMenu();
void gameScreen();
void showPauseScreen();
void displayCourt();
void updateBall();
void bounceSound();
void scoreSound();
void resetBall();
void resetPaddles();
void displayWinner();
void resetGame();

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  display.clearDisplay();
  mainMenu();
}

void loop() {
  handleButton();

  // Main Menu Logic
  if (isMainMenu) {
    if (isMainMenu != lastMenuState) {
      resetGame();
      mainMenu();
      lastMenuState = isMainMenu;
    }
    return; // Do not update game
  } 

  // Pause screen
  if (isGamePaused) {
    if (isGamePaused != lastPauseState) {
      showPauseScreen();
      lastPauseState = isGamePaused;
    }
    return;
  } else {
    if (isGamePaused != lastPauseState) {
      gameScreen();
      lastPauseState = isGamePaused;
    }
  }

  // Check for winner
  if (scoreOne == WINNING_SCORE || scoreTwo == WINNING_SCORE) {
    displayWinner();
    resetGame();
    isMainMenu = true;
    isGamePaused = false;
    lastMenuState = false;
    lastPauseState = true;
    return;
  }

  // Game update
  displayCourt();
  updateBall();
  updatePaddles();
  display.display();
}

// Button handling
void handleButton() {
  int reading = digitalRead(BUTTON_PIN);  // Read the current state of the button (LOW when pressed)

  // Check if the button input changed since last read
  if (reading != lastButtonState) {
    lastDebounceTime = millis();  // Reset debounce timer
  }

  // If the button state has been stable longer than the debounce delay
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Check if the stable state actually changed
    if (reading != buttonStateStable) {
      buttonStateStable = reading;  // Update the stable state

      // If button is pressed (Low due to pull-up configuration)
      if (buttonStateStable == LOW) {
        buttonPressStart = millis();  // Record the time the press started
        buttonHeld = true;            // Mark that the button is being held
      }

      // If button is released after being held
      if (buttonStateStable == HIGH && buttonHeld) {
        unsigned long pressDuration = millis() - buttonPressStart;  // Calculate how long the button was held
        buttonHeld = false;  // Reset hold flag

        if (pressDuration >= 3000) {
          // Long press, return to main menu
          isMainMenu = true;
          isGamePaused = false;
          lastMenuState = false;
          lastPauseState = true;
        } else {
          // Short press
          if (isMainMenu) {
            // If on main menu, start the game
            gameScreen();
            isMainMenu = false;
            isGamePaused = false;
            lastMenuState = true;
            lastPauseState = false;
          } else {
            // If in game, toggle pause/unpause
            isGamePaused = !isGamePaused;
          }
        }
      }
    }
  }

  // Save the current reading for next loop iteration
  lastButtonState = reading;
}

// Screens
void mainMenu() {
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(28, 10);
  display.println("PONG");
  display.setTextSize(1);
  display.setCursor(28, 45);
  display.println("PRESS BUTTON");
  display.display();
}

// Clear screen and draw court
void gameScreen() {
  display.clearDisplay();
  displayCourt();

  // Redraw current ball/paddle when resuming game
  display.fillRect(PLAYER_TWO_X, playerTwoY, 2, 16, SSD1306_WHITE);
  display.fillRect(PLAYER_ONE_X, playerOneY, 2, 16, SSD1306_WHITE);
  display.drawPixel(ballX, ballY, SSD1306_WHITE);

  countdown();
  display.display();
}

void showPauseScreen() {
  // Draw black rectangle background behind the pause text
  display.fillRect(20, 20, 56, 24, SSD1306_BLACK);

  // Draw a white rectangle around the text
  display.drawRect(20, 20, 90, 24, SSD1306_WHITE);

  // Draw the pause text inside the rectangle
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30, 25);
  display.print("PAUSED");

  // Update the display
  display.display();
}


// Game drawing
void displayCourt() {
  // Draw a rectangle border around the screen
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);

  // Dashed center line
  for (int y = 0; y < SCREEN_HEIGHT; y += 4) {
    display.drawFastVLine(SCREEN_WIDTH / 2, y, 2, SSD1306_WHITE);
  }

  // Scores
  displayScore();
}

void displayScore() {
  // Clear old score area with a black rectangle
  display.fillRect(54, 0, 20, 12, SSD1306_BLACK);

  // Draw the updated scores
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(56, 0);
  display.print(scoreOne);  // Player 1 score
  display.setCursor(67, 0);
  display.print(scoreTwo);  // Player 2 score
}

// Game Logic
void updateBall() {

  // Erase old ball (if not touching top/bottom borders)
  if (prevBallY >= 1 && prevBallY < SCREEN_HEIGHT - 2) {
    display.fillRect(prevBallX - 1, prevBallY - 1, 2, 2, SSD1306_BLACK);
  }

  // Move ball
  ballX += ballDirX;
  ballY += ballDirY;

  // Bounce top/bottom
  if (ballY <= 1) {
    ballY = 1;
    ballDirY = -ballDirY;
    bounceSound();
  } else if (ballY >= SCREEN_HEIGHT - 3) {
    ballY = SCREEN_HEIGHT - 3;
    ballDirY = -ballDirY;
    bounceSound();
  }

  // Check horizontal wall collisions for scoring
  if (ballX <= 1) {
    scoreTwo++;
    playerScored();
  } else if (ballX >= SCREEN_WIDTH - 2) {
    scoreOne++;
    playerScored();
  }

  // Paddle collisions
  if ((ballX == PLAYER_ONE_X + 2) && ((ballY >= playerOneY) && (ballY <= playerOneY + 16))) {
    ballDirX = -ballDirX;
  } else if ((ballX == PLAYER_TWO_X) && ((ballY >= playerTwoY) && (ballY <= playerTwoY + 16))) {
    ballDirX = -ballDirX;
  }

  display.drawPixel(ballX, ballY, SSD1306_WHITE);

  prevBallX = ballX;
  prevBallY = ballY;
}

void updatePaddles() {
  int playerOne = analogRead(PLAYER_ONE);
  int playerTwo = analogRead(PLAYER_TWO);

  // Move player two
  if (playerTwo > 550 && playerTwoY <= (SCREEN_HEIGHT - 2) - 16) {
    display.fillRect(PLAYER_TWO_X, playerTwoY, 2, 1, SSD1306_BLACK);
    playerTwoY++;
  } else if (playerTwo < 450 && playerTwoY > 1) {
    display.fillRect(PLAYER_TWO_X, playerTwoY + 15, 2, 1, SSD1306_BLACK);
    playerTwoY--;
  }

  // Move player one and erase previous position
  if (playerOne < 450 && playerOneY <= (SCREEN_HEIGHT - 2) - 16) {
    display.fillRect(PLAYER_ONE_X, playerOneY, 2, 1, SSD1306_BLACK);
    playerOneY++;
  } else if (playerOne > 550 && playerOneY > 1) {
    display.fillRect(PLAYER_ONE_X, playerOneY + 15, 2, 1, SSD1306_BLACK);
    playerOneY--;
  }

  // Redraw paddles
  display.fillRect(PLAYER_TWO_X, playerTwoY, 2, 16, SSD1306_WHITE);
  display.fillRect(PLAYER_ONE_X, playerOneY, 2, 16, SSD1306_WHITE);
}

void resetBall() {
  ballX = SCREEN_WIDTH / 2;
  ballY = SCREEN_HEIGHT / 2;
  ballDirX = (random(0, 2) == 0) ? 1 : -1;
  ballDirY = (random(0, 2) == 0) ? 1 : -1;
  prevBallX = ballX;
  prevBallY = ballY;
  display.display();
}

// Resets upon exit to main menu
void resetGame() {
  resetBall();
  resetPaddles();
  display.display();

  scoreOne = 0;
  scoreTwo = 0;
}

void resetPaddles() {
  display.fillRect(PLAYER_TWO_X, playerTwoY, 2, 16, SSD1306_BLACK);
  display.fillRect(PLAYER_ONE_X, playerOneY, 2, 16, SSD1306_BLACK);

  playerOneY = 25;
  playerTwoY = 25;

  // Only redraw paddles when game not over
  if  (scoreOne != WINNING_SCORE && scoreTwo != WINNING_SCORE) {
    display.fillRect(PLAYER_TWO_X, playerTwoY, 2, 16, SSD1306_WHITE);
    display.fillRect(PLAYER_ONE_X, playerOneY, 2, 16, SSD1306_WHITE);
  }

  display.display();
}

void displayWinner() {
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(18, 20);

  if (scoreOne == WINNING_SCORE) {
    display.print("PLAYER 1");
  } else {
    display.print("PLAYER 2");
  }

  display.setCursor(35, 40);
  display.print("WINS!");

  display.display();
  delay(3000);
}

void playerScored() {
  scoreSound();
  resetBall();
  resetPaddles();
  displayScore();
  display.display();

  // Countdown next round if no winner yet
  if (scoreOne != WINNING_SCORE && scoreTwo != WINNING_SCORE) {
      countdown();
  }
}

void countdown() {
  display.fillRect(59, 27, 14, 14, SSD1306_BLACK);
  display.drawCircle(64, 32, 7, SSD1306_WHITE);

  display.setTextSize(1);

  for (int i = 3; i > 0; i--) {
    display.setCursor(62, 29);
    display.print(i);
    display.display();
    delay(1000);

    display.fillRect(61, 29, 7, 7, SSD1306_BLACK);
    display.display();
  }

  display.fillRect(57, 25, 16, 16, SSD1306_BLACK);
}

void bounceSound() {
  tone(BUZZER_PIN, 600, 40);
}

void scoreSound() {
  tone(BUZZER_PIN, 200, 80);
}