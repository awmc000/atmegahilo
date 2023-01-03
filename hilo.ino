/* ======================================
 * ------------- H i - L o --------------
 *          Number Guessing Game
 * --------------------------------------
 * 
 * Designed and developed by Alexander McColm over December 31st, 2022 to January 2nd, 2023.
 * 
 * A simple number-guessing game played with 3 buttons and an OLED screen.
 * 
 * Controls:
 *    In order to guess the number, increment and decrement to it with + and -
 *    and then press the "Guess" button.

 *    Buttons represent minus, submit guess, and plus, in that order.
 * 
 * Components used:
 *    1x    Arduino Uno
 *    1x    0.96" OLED display with SSD1306 supporting I2C
 *    3x    Push buttons
 *   10x    Different lengths of wire
 *
 * Libraries used:
 *    Wire                ─────── For TWI/I2C communication.
 *    Adafruit_GFX        ───┐
 *    Adafruit_SSSD1306   ───┴─── For the OLED display being used.
 *
 * Resources used:
 *    Debounce tutorial, Arduino
 *    Language reference, Arduino
 *    "arduino oled i2c tutorial" by MissionCritical on Youtube
 *
 * Inspired by a learning exercise in the early chapters of "Learn Java the 
 * Easy Way" from No Starch Press I did a couple years ago.
 *    
 *
 * Wishlist / possible features to add:
 *    Track the number of guesses.
 *    Add point system/high score.
 *    Add bitmap image graphics.
 *    Add sounds of some sort.
 *    Replace both plus and minus buttons with a rotary encoder
 *    that allows the user to scroll to the number they are selecting.
 *    Replace all uses of pinMode and digitalRead with direct use of registers.
 * ====================================== */
#include <Wire.h> // for twi / i2c
#include <Adafruit_SSD1306.h> // for this specific OLED
#include <Adafruit_GFX.h> // required by Adafruit_SSD1306.h

// Constants for SSD1306
#define DISPLAY_HEIGHT 64
#define DISPLAY_WIDTH 128
#define TEXT_SIZE 2
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
#define OLED_RESET     -1 // sharing Arduino reset pin
#define SCREEN_ADDRESS 0x3C // found address to be 0x3c by a test program

// the object representing the display itself
Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, OLED_RESET);

// constants for buttons and debounce
#define BUTTON_MINUS 3
#define BUTTON_GUESS 2
#define BUTTON_PLUS 4

#define DEBOUNCE_DELAY 75

// Variables for debouncing
// m will represent the minus button, g the player_guess button, and p the plus button.

byte m_last_steady_state = LOW;
byte m_last_flickerable_state = LOW;
byte m_current_state;

byte g_last_steady_state = LOW;
byte g_last_flickerable_state = LOW;
byte g_current_state;

byte p_last_steady_state = LOW;
byte p_last_flickerable_state = LOW;
byte p_current_state;

unsigned long m_last_debounce_time = 0;
unsigned long g_last_debounce_time = 0;
unsigned long p_last_debounce_time = 0;

// Constants for game logic
#define GAME_START_NUMBER 5
#define GAME_LEVEL_MULTIPLIER 2
#define GAME_MAX_NUMBER 50

// variables for game logic
unsigned short int player_guess = 0;
unsigned short int number = 5;
unsigned short int level = 1;
unsigned short int max_number = GAME_START_NUMBER * GAME_LEVEL_MULTIPLIER * level;
unsigned short int guesses = 0;

/* ======================================
 * ------------ game_state --------------
 * This enumeration represents the states of the game,
 * as an aid to a sort of state machine design.
 * 
 * start         ───────  start screen - press any button to play
 *
 * enter_guess   ───────  enter a player_guess - shows the number the player has 
 *                       chosen, and allows them to change it
 *
 * too_low       ───┐     player is told their player_guess was lower or higher
 * too_high      ───┴─── than secret number, and returned to enter_guess.
 *
 * correct       ───────  player guess right. if last level, go to win. otherwise, 
 *                       if they hit plus or player_guess, go to next level. 
 *
 * win           ───────  player finished all 5 levels. player can push a button to 
 *                       restart from level 1.
 * ====================================== */
enum game_state {
  start,          // start screen - press any button to play
  enter_guess,    // enter a player_guess - shows the number the player has chosen, and allows them to change it
  too_low,        // player is told their player_guess was lower than secret number, and returned to enter_guess
  too_high,       // player is told their player_guess was higher than secret number, and returned to enter_guess
  correct,
  win
};

game_state current_state = start;

// forward declarations for functions

void button_checks();

// screens/states
void start_screen();
void enter_guess_screen();

void too_low_screen();
void too_high_screen();
void correct_screen();
void win_screen();

// screen helpers
void bad_guess_screen(bool high);
void title_font();

// buttons
void press_minus();
void press_guess();
void press_plus();

// setup and game logic
void setup_game();


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Wire.begin();

  // set pin modes - to be replaced by bitwise manipulations of data direction register
  pinMode(BUTTON_MINUS, INPUT_PULLUP);
  pinMode(BUTTON_GUESS, INPUT_PULLUP);
  pinMode(BUTTON_PLUS , INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // infinite loop
  }

  // setting text color and size
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(TEXT_SIZE);
  display.clearDisplay();
}

void loop() {
  // put your main code here, to run repeatedly:
  
  button_checks();

  switch (current_state) {
    case start:
      start_screen();
      break;

    case enter_guess:
      enter_guess_screen();
      break;

    case too_low:
      too_low_screen();
      break;

    case too_high:
      too_high_screen();
      break;

    case correct:
      correct_screen();
      break;

    case win:
      win_screen();
      break;

  }

}

void button_checks() {
  // Minus Button
  m_current_state = digitalRead(BUTTON_MINUS);

  if (m_current_state != m_last_flickerable_state) {
    m_last_debounce_time = millis();
    m_last_flickerable_state = m_current_state;
  }

  if ((millis() - m_last_debounce_time) > DEBOUNCE_DELAY) {
    if ((m_last_steady_state == HIGH) && (m_current_state == LOW)) {
      press_minus();
    }
    m_last_steady_state = m_current_state;
  }

  // Guess Button
  g_current_state = digitalRead(BUTTON_GUESS);

  if (g_current_state != g_last_flickerable_state) {
    g_last_debounce_time = millis();
    g_last_flickerable_state = g_current_state;
  }

  if ((millis() - g_last_debounce_time) > DEBOUNCE_DELAY) {
    if ((g_last_steady_state == HIGH) && (g_current_state == LOW)) {
      press_guess();
    }
    g_last_steady_state = g_current_state;
  }

  // Plus Button
  p_current_state = digitalRead(BUTTON_PLUS);

  if (p_current_state != p_last_flickerable_state) {
    p_last_debounce_time = millis();
    p_last_flickerable_state = p_current_state;
  }

  if ((millis() - p_last_debounce_time) > DEBOUNCE_DELAY) {
    if ((p_last_steady_state == HIGH) && (p_current_state == LOW)) {
      press_plus();
    }
    p_last_steady_state = p_current_state;
  }
}

void title_font() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(2);
}

void start_screen() {
  title_font();
  display.println("\n H i - L o ");
  display.display();
}

void enter_guess_screen() {
  title_font();
  
  display.print(" GUESS..? \n\n  ");
  display.println(player_guess);
  
  display.setTextSize(1);
  
  display.print("\nLVL: ");
  display.print(level);
  //display.print(" MAX: ");
  //display.print(max_number);
  display.print(" GUESSES: ");
  display.print(guesses);
  
  display.display();
}

void bad_guess_screen(bool high) {
  title_font();
  if (high) {
    display.println("TOO HIGH!");
  } else {
      display.println("TOO LOW!");
  }
  display.setTextSize(1);
  display.println("Press any button to keep guessing.");
  display.display();
}

void too_low_screen() {
  bad_guess_screen(false);
}

void too_high_screen()
{
  bad_guess_screen(true);
}

void correct_screen()
{
  title_font();
  display.println("CORRECT!");
  display.setTextSize(1);
  display.print("Press GUESS button to continue.");
  display.display();
}

void win_screen() {
  title_font();
  display.println(" WINNER! ");
  display.setTextSize(1);
  display.print("You won the game!\nPress GUESS to restart.");
  display.display();
}

void press_minus() {
  // check current state and respond accordingly
  switch (current_state) {
    case start:
      current_state = enter_guess;
      setup_game();
      break;

    case enter_guess:
      if (player_guess > 0)
        player_guess--;
      break;

    case too_low:
    case too_high:
      current_state = enter_guess;
      break;

    case correct:
      break;

    case win:
      break;
  }
}

void press_guess() {
  switch (current_state) {
    case start:
      current_state = enter_guess;
      setup_game();
      break;

    case enter_guess:
      if (player_guess < number) {
        current_state = too_low;
        guesses++;
      } else if (player_guess > number) {
        current_state = too_high;
        guesses++;
      } else { // player is correct
        player_guess = 0;
        current_state = correct;
      }
      break;

    case too_low:
    case too_high:
      current_state = enter_guess;
      break;

    case correct:
      // if we are at the last level, the player has won the game
      if ((GAME_START_NUMBER * GAME_LEVEL_MULTIPLIER * level) >= GAME_MAX_NUMBER) {
        current_state = win;
        player_guess = 0;
      } else { // otherwise, we keep going
        setup_game();
        current_state = enter_guess;
        level++;
        Serial.println(level);
        player_guess = 0;
      }
      break;

    case win:
      current_state = start;
      level = 1;
      player_guess = 0;
      break;
  }
}

void press_plus() {
  switch (current_state) {
    case start:
      current_state = enter_guess;
      setup_game();
      break;

    case enter_guess:
      if (player_guess < 1000)
        player_guess++;
      break;

    case too_low:
    case too_high:
      current_state = enter_guess;
      break;
  }
}

void setup_game() {
  max_number = GAME_START_NUMBER * GAME_LEVEL_MULTIPLIER * level;
  guesses = 0;
  number = random(max_number);
  Serial.print("The new random number is : ");
  Serial.println(number);
}
