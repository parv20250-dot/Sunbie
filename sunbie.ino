/*This code is for Sunbie (a Tamatogochi like device that enables the user to have a lil pet sprite). In this programme 
the various components are configured and the readings from the sensors are interpreted so as to allow the Sprite to react 
to any changes in the environment. It also allows for the user to interact with the Sunbie. This programme has been developed
as a part of the Hack Club Project- Half Life */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ADXL345_U.h>
#include <DHT.h>
#include <ezBuzzer.h>
#include <Preferences.h>
#include <math.h>

//This is the board's pins (wrt to the Waveshare ESP32-C3)
const int I2C_SDA_PIN = 6; //OLED AND ADXL 345 SDA PIN
const int I2C_SCL_PIN = 5; //OLED AND ADXL 345 SCL PIN
const int DHT_PIN = 4; //DHT11 SENSOR PIN
const int BUTTON_ONE_PIN =1; //BUTTON ONE-opens and confirms the menu
const int BUTTON_TWO_PIN = 3;//BUTTON TWO-shows and hides statistics
const int BUZZER_PIN = 7; //Connects to the buzzer 

const uint8_t OLED_ADDRESS = 0X3C;//OLED address
const uint8_t ADXL345_ADDRESS = 0x53; //ADXL345 address (I think?)

//Starting stats (stats range from 0-100 but these the starting values all new pets get by default)
const int STARTING_JOY = 70;
const int STARTING_ENERGY = 75;
const int STARTING_FULLNESS= 65;
const int STARTING_HEALTH = 70;

//Set true, upload once then set it to false if you want a fresh pet (if true this makes a fresh pet)
const bool RESET_SAVED_PET = false;

/*There are five actions that the user can do to interact with the pet (and five corresponding reactions),there are also passive 
reactions such as responses to the heat and cold or low health or being shaken*/
enum PetReaction {
    NAP_REACTION,
    JUMP_REACTION,
    HEART_REACTION,
    RUN_REACTION,
    DIZZY_REACTION,
    COLD_REACTION,
    HOT_REACTION,
    SICK_REACTION,
    };

struct MenuItem {
    const char *label;
    int joyChange;
    int energyChange;
    int fullnessChange;
    int healthChange;
    PetReaction reaction;};

const MenuItem MENU_ITEMS[] = {
    {"NAP", 1, 18, -4, 3, NAP_REACTION}, //pet takes a nap, the changes in the attributes are mentioned in the brackets
    {"PLAY", 12, -9, -5, -4, RUN_REACTION},//pet plays, same as above
    {"FEED", 3, -2, 15, 1, JUMP_REACTION},//pet is fed, same as above
    {"PET", 7, 2, 0, 0, HEART_REACTION}, //pet the pet (pun intended), same as above
    {"MEDS", -7, -3, 0, 7, JUMP_REACTION}, //pet is given medicine same as above (pet will be sad cuz medicine bitter but increases health)
};

const int MENU_ITEM_COUNT = sizeof(MENU_ITEMS)/sizeof(MENU_ITEMS[0]); //calculates how many menu items there are (avoids hardcoding)

//makes menu ball move less 
const float MENU_TILT_LIMIT = 6.0f;

//To change ADXL345 orientation (if its flipped)
// 1. If sensor is turned 90 degrees, set SWAP_ADXL345_AXES  to true
// 2. If a firection feels backwards, change from 1.0f to -1.0f.
const bool SWAP_ADXL345_AXES = false;
const float MENU_X_DIRECTION = 1.0f;
const float MENU_Y_DIRECTION = -1.0f;

//A dead zone, ball rests unless boards is tilted, the threshold for shaking is defined
const float MENU_DEAD_ZONE = 0.8f;
const float SHAKE_THRESHOLD = 7.0f; 

//Shake movements effects are defined below(what happens on shaking the Sunbie aside from the dizzy reaction)
const int SHAKE_JOY_CHANGE = 5;
const int SHAKE_ENERGY_CHANGE =-2;
const int SHAKE_FULLNESS_CHANGE =-1;
const int SHAKE_HEALTH_CHANGE =-4;

//Dizzy is triggered when the device is shaken  thrice in 3 seconds
const uint8_t DIZZY_SHAKE_COUNT =3;
const uint16_t DIZZY_WINDOW_MS = 3000;
const uint32_t DIZZY_DURATION_MS = 3500;
const int DIZZY_JOY_CHANGE = -6;
const int DIZZY_ENERGY_CHANGE = -4;
const int DIZZY_HEALTH_CHANGE = -8;

//Sick if health is below 20, stays sick till health is more than 30
const int SICK_MIN_HEALTH = 20;
const int SICK_MAX_HEALTH = 30;
const int MEDS_MENU_INDEX = 4;

//Temperature limits for temp reactions (too cold if below 10 and too hot if above 30),buffer avoids flickering
const float TOO_COLD_C = 10.0f;
const float TOO_HOT_C = 30.0f;
const float TEMP_BUFFER_C = 1.0f;

//32 by 32 pixel sprite's bitmap is defines below (my sprite is a litlle fox)
const int PET_SPRITE_WIDTH = 32;
const int PET_SPRITE_HEIGHT = 32;

//The pet wanders around unless its asleep . The durations of the reactions as well as the time taken for some of them are defined below
const uint16_t PET_WALK_PIXEL_MS= 70;
const uint16_t PET_PRE_JUMP_MS = 230;
const uint16_t PET_JUMP_MS = 430;
const int PET_JUMP_HEIGHT = 16;
const uint32_t NAP_DURATION = 48000;
const uint16_t HEARTS_DURATION_MS = 1600;
const uint16_t PLAY_LAP_MS = 800;
const uint8_t PLAY_LAP_COUNT = 2;

//Each byte of the bitmap (bmp image) of the sprite is shown below
const uint8_t PROGMEM PET_SPRITE[]=
{0x04, 0x00, 0x03, 0x00, 0x0F, 0x00, 0x06, 0x00,
0x0D, 0x80, 0x0E, 0x80, 0x0A, 0xC0, 0x1B, 0xC0,
0x13, 0x6F, 0xB7, 0x40, 0x17, 0x3B, 0xE7, 0x40,
0x16, 0x90, 0x49, 0x40, 0x14, 0x80, 0x09, 0x40,
0x1A, 0x00, 0x03, 0x40, 0x0E, 0x00, 0x03, 0xC4,
0x4C, 0x00, 0x01, 0x8C, 0x4C, 0x00, 0x01, 0x80,
0x08, 0x00, 0x00, 0x80, 0x18, 0x00, 0x00, 0x80,
0x11, 0xE0, 0x3C, 0x40, 0x23, 0x10, 0x66, 0x20,
0x10, 0x07, 0x00, 0x42, 0x08, 0x0B, 0x81, 0x80,
0x06, 0x05, 0x03, 0x38, 0x03, 0x80, 0x0C, 0x48,
0x00, 0xFF, 0xF8, 0x8C, 0x60, 0x87, 0x19, 0x84,
0x21, 0x80, 0x0D, 0x06, 0x07, 0x80, 0x0F, 0x02,
0x04, 0x80, 0x0B, 0x02, 0x08, 0x80, 0x09, 0x02,
0x08, 0x81, 0x10, 0x46, 0x08, 0xC5, 0x10, 0xC4,
0x08, 0x46, 0x11, 0x8C, 0x04, 0x66, 0x31, 0x98,
0x07, 0x6F, 0xB3, 0xE0, 0x01, 0xFF, 0xFC, 0x00
};
const unsigned char epd_bitmap_dizzy [] PROGMEM = {
	0x00, 0x00, 0x00, 0xf0, 0x00, 0xf0, 0x01, 0xf2, 0x02, 0xf9, 0x08, 0xc1, 0x30, 0x01, 0x40, 0x02, 
	0x86, 0x04, 0x8f, 0x10, 0x9f, 0x40, 0x67, 0x80, 0x06, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00}; 
//bitmap for dizzy reaction

const unsigned char epd_bitmap_sick_reaction [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xe0, 0x40, 0x00, 0x10, 0x40, 0x00, 0x10, 0x3f, 
	0xff, 0xe0, 0x00, 0x00, 0x20, 0x00, 0x00, 0x20, 0x00, 0x00, 0x19, 0x80, 0x83, 0x11, 0x81, 0x42, 
	0x29, 0x81, 0xc3, 0xca, 0x80, 0x00, 0x94, 0x80, 0x01, 0xa8, 0x80, 0x01, 0x50, 0x80, 0x01, 0xb0, 
	0x40, 0x1f, 0xe0, 0x00, 0x43, 0x20, 0x20, 0x00, 0x40, 0x10, 0x00, 0x80, 0x0c, 0x01, 0x00, 0x01, 
	0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//bitmap for sick reaction

const unsigned char epd_bitmap_warm_reaction [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x0c, 0x00, 0x1c, 0x30, 0x3e, 0x28, 0x36, 0x6c, 0x22, 0x44, 
	0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//bitmap for warm reaction

const unsigned char epd_bitmap_cold_reaction1 [] PROGMEM = {
	0x00, 0xc0, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x1c, 0xc6, 0x00, 0x00, 
	0x18, 0xc6, 0x00, 0x00, 0x1c, 0xcf, 0x00, 0x00, 0x02, 0xd0, 0x00, 0x00, 0x61, 0xe0, 0x80, 0x00, 
	0x7f, 0xc7, 0x80, 0x00, 0xff, 0xc7, 0xc0, 0x00, 0x61, 0xe1, 0x00, 0x00, 0x02, 0xd0, 0x00, 0x00, 
	0x04, 0xc8, 0x00, 0x00, 0x1c, 0xc6, 0x02, 0x00, 0x18, 0xc6, 0x02, 0x00, 0x05, 0xc8, 0x07, 0x80, 
	0x01, 0xe0, 0xe3, 0x28, 0x00, 0xc0, 0x62, 0x30, 0x00, 0x00, 0xe2, 0x38, 0x00, 0x00, 0x12, 0x40, 
	0x00, 0x01, 0x0a, 0x84, 0x00, 0x01, 0x87, 0x0c, 0x00, 0x03, 0xff, 0xff, 0x00, 0x01, 0x87, 0x04, 
	0x00, 0x00, 0x0a, 0xc0, 0x00, 0x00, 0x12, 0x60, 0x00, 0x00, 0xe2, 0x38, 0x00, 0x00, 0x62, 0x38, 
	0x00, 0x00, 0xa7, 0x28, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
//bitmap for cold reaction

//The following part of the code define the width and height of the OLED display then defines centre of display,positions of menu items
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int MENU_CENTER_X = 64;
const int MENU_CENTER_Y = 32;
const int MENU_ITEM_X[] = { 64, 106,  32,  22,  96};
const int MENU_ITEM_Y[] = { 10,  28,  54,  28,  54};
//It defines the range of the menu ball's movement, prevents misclicks, defines intervals for the sensor readings and gravity
const int MENU_BALL_X_RANGE = 44;
const int MENU_BALL_Y_RANGE = 21;
const uint16_t BUTTON_DEBOUNCE_MS = 30;
const uint16_t ADXL345_READ_INTERVAL_MS =30;
const uint16_t DHT_READ_INTERVAL_MS = 2200;
const uint16_t SHAKE_COOLDOWN_MS = 650;
const float STANDARD_GRAVITY = 9.80665f;

//The code below initialises hardware to the ESP32-C3
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
DHT dht (DHT_PIN, DHT11);
ezBuzzer buzzer(BUZZER_PIN);
Preferences preferences;

//creates a struct to hold the pets stats
struct PetState {
    int joy;
    int energy;
    int fullness;
    int health;
};

//creates a struct for both buttons to hold their states
struct ButtonState {
    int pin;
    bool stableState;
    bool lastRawState;
    uint32_t lastChangedAt;
};

enum View {
    PET_VIEW,
    MENU_VIEW,
    STATS_VIEW,
};

enum TempState {
    TEMP_OK,
    TEMP_COLD,
    TEMP_HOT};

//default pet states and individual button states are defined below
PetState pet = {STARTING_JOY, STARTING_ENERGY, STARTING_FULLNESS, STARTING_HEALTH};
ButtonState buttonOne = {BUTTON_ONE_PIN, HIGH, HIGH, 0};
ButtonState buttonTwo = {BUTTON_TWO_PIN, HIGH, HIGH,0};
View currentView = PET_VIEW;
//Defines default values,sets the default sensor detection to false to prevent errors down the line if sensors are not detected
//Defines default values for other components before reading are taken 
bool adxlFound =false;
bool dhtFound =false;
float accelerationX = 0.0f;
float accelerationY = 0.0f;
float accelerationZ = STANDARD_GRAVITY;
float temperatureC = NAN;
float humidity = NAN;
float menuBallX = MENU_CENTER_X;
float menuBallY = MENU_CENTER_Y;
float menuCenterAccelerationX = 0.0f;
float menuCenterAccelerationY = 0.0f;
int selectedMenuItem = -1;
uint32_t lastAdxlReadAt = 0;
uint32_t lastDhtReadAt = 0;
uint32_t lastShakeAt = 0;
uint32_t shakeAnimationEndsAt = 0;
uint32_t petJumpStartedAt = 0;
uint32_t nappingUntil = 0;
uint32_t heartAnimationEndsAt = 0;
uint32_t playRunStartedAt = 0;
int nappingPetX = 0; 
int frozenPetX = 0;//pet stands still while frozen or sick
uint32_t dizzyUntil = 0;
uint32_t firstShakeAt = 0;
uint8_t shakeCount =0;
bool petIsSick = false; //pet not sick by default, number of shakes in a row monitored,not dizzy by default
TempState tempState = TEMP_OK; //tempearture is ok by default when started (before readings)

//Sounds are defined below for the buzzer
int napNotes[] = {784,659,523};
int napDur[] = {4,4,2};
int happyNotes [] ={523,659,784, 1047};
int happyDur[] = {8, 8, 8, 4};
int sadNotes[] = {392, 330, 262}; 
int sadDur[] = {8, 8, 4};
int dizzyNotes[] = {988, 784, 988, 784, 880, 659, 880, 659, 392};
int dizzyDur[] = {8, 8, 8, 8, 8, 8, 8, 8, 4};
int sickNotes[] = {523, 440, 349, 262}; 
int sickDur[] = {4, 4, 4, 2};
int recoverNotes[] = {392, 523, 659, 784}; 
int recoverDur[] = {8, 8, 8, 4};
int coldNotes[] = {1047, 988, 1047, 988, 1047, 988}; 
int coldDur[] = {16, 16, 16, 16, 16, 16};
int hotNotes[] = {330, 392, 330, 392, 330}; 
int hotDur[] = {8, 8, 8, 8, 4};
int deniedNotes[] = {196, 165}; 
int deniedDur[] = {8, 4};
#define PLAY_SOUND(notes, durations) playSound(notes, durations, sizeof(durations) / sizeof((durations)[0]))

//defines conditions for pet walking
int petWalkingX(uint32_t now);
int keepInRange(int value, int smallest, int largest) {return constrain(value, smallest, largest);
}

//defines range of change in joy,energy, fullness and health in between 0-100(or contrains it ig)
void changePet (int joyChange, int energyChange, int fullnessChange, int healthChange)
{pet.joy = keepInRange(pet.joy+ joyChange,0, 100);
pet.energy =keepInRange(pet.energy+ energyChange, 0, 100);
pet.fullness = keepInRange(pet.fullness+ fullnessChange,0 , 100);
pet.health = keepInRange (pet.health + healthChange,0 ,100);
}

//loads pet, if stats are saved it loads stats else it loads default values
void loadPet(){
    preferences.begin("starbie",false);
    if (RESET_SAVED_PET){preferences.clear();
    }
    pet.joy = preferences.getInt("joy",STARTING_JOY);
    pet.energy = preferences.getInt("energy",STARTING_ENERGY);
    pet.fullness = preferences.getInt("full",STARTING_FULLNESS);
    pet.health = preferences.getInt("health",STARTING_HEALTH);}

//saves pet stats into flash memory(?)
void savePet(){
    preferences.putInt("joy",pet.joy);
    preferences.putInt("energy",pet.energy);
    preferences.putInt("full",pet.fullness);
    preferences.putInt("health",pet.health);}

//plays sound
void playSound(int *notes, int *durations, int length){buzzer.stop();
    buzzer.playMelody(notes,durations,length);} 

//sets up button and does not consider false presses also triggers button(i think?)
void setUpButton(ButtonState &button) {
  pinMode(button.pin, INPUT_PULLUP);
  button.stableState = digitalRead(button.pin);
  button.lastRawState = button.stableState;
  button.lastChangedAt = millis();}

bool wasPressed (ButtonState & button){
    const bool rawState = digitalRead(button.pin);
    const uint32_t now = millis();
    if (rawState != button.lastRawState){button.lastRawState = rawState;
        button.lastChangedAt =now;}
    if (rawState != button.stableState && now-button.lastChangedAt >= BUTTON_DEBOUNCE_MS){
        button.stableState = rawState;
        return button.stableState == LOW;
    }
    return false;
}

//updating adxl345 reading
void updateAdxl(){
    if (!adxlFound || millis() - lastAdxlReadAt < ADXL345_READ_INTERVAL_MS) {
    return;
  }
  lastAdxlReadAt = millis();
  sensors_event_t acceleration;
  accel.getEvent(&acceleration);
  accelerationX = acceleration.acceleration.x;
  accelerationY = acceleration.acceleration.y;
  accelerationZ = acceleration.acceleration.z;
}

//updating and calibrating dht11 readings
void updateDht(){
    if (!dhtFound || millis() - lastDhtReadAt < DHT_READ_INTERVAL_MS) {
    return;}
    lastDhtReadAt = millis();
    const float newHumidity = dht.readHumidity();
    const float newTemperatureC = dht.readTemperature();
    if(!isnan(newHumidity)){
        humidity = newHumidity;}
    if (!isnan(newTemperatureC)){
        temperatureC = newTemperatureC;}
}

//Showcases what pet is currently doing and calculates movement parametres
int petWalkingX(uint32_t now){
    const int farthestX = SCREEN_WIDTH- PET_SPRITE_WIDTH;//furthest the pet can move around
    const uint32_t roundTrip = static_cast<uint32_t>(farthestX) * 2;
    const uint32_t step = (now/PET_WALK_PIXEL_MS) % roundTrip;
    return step <= farthestX ? step : roundTrip - step;
}
uint32_t playRunDuration(){
    return static_cast<uint32_t>(PLAY_LAP_MS) * PLAY_LAP_COUNT;}

bool isNapping(){
    return nappingUntil != 0 && millis() < nappingUntil;}

bool isPlaying (){
    return playRunStartedAt != 0 && millis() - playRunStartedAt < playRunDuration();}
//calculates the pets horizontal position during a run
int playRunX(uint32_t now) {
    const int farthestX = SCREEN_WIDTH - PET_SPRITE_WIDTH;
    const uint32_t lapAge = (now - playRunStartedAt) % PLAY_LAP_MS;
    const float progress = static_cast<float>(lapAge) / PLAY_LAP_MS;
    return progress < 0.5f ? static_cast<int>(progress * 2.0f *farthestX)
                           :static_cast<int>((1.0f - progress) * 2.0f *farthestX);
}

bool isDizzy(){
    return dizzyUntil != 0 && millis()<dizzyUntil;}//dizzy reaction

void stopAllReactions(){
    nappingUntil =0;
    heartAnimationEndsAt = 0;
    playRunStartedAt = 0;
    petJumpStartedAt = 0;
    shakeAnimationEndsAt = 0;
    dizzyUntil = 0;} //this function allows for all ongoing reactions to be stopped (if pet is sick)

// Resets napping and playing timers back to zero once their duration has expired
void updatePetTimers(){
    if (nappingUntil != 0 && ! isNapping()){
        nappingUntil = 0;} //pet wakes up once nap time is over
    if (playRunStartedAt != 0 && !isPlaying()){
        playRunStartedAt = 0;} //play stops after two laps
}

void updateSickState(){
    if (!petIsSick && pet.health <= SICK_MIN_HEALTH) {
        stopAllReactions(); //stops all reactions when health is below 20 and pet not already sick
        frozenPetX = petWalkingX(millis()); 
        petIsSick = true;
        PLAY_SOUND(sickNotes, sickDur);} //plays notes for sick
    else if (petIsSick && pet.health >= SICK_MAX_HEALTH) { 
        petIsSick = false; //makes pet healthy again when its above the health limit for sick and its sick
        PLAY_SOUND(recoverNotes, recoverDur);}} //plays notes for recovery

void updateTempState(){
    if (isnan(temperatureC)) {
        return;} //no reading yet, keep the old state
    TempState newState = tempState;
    if (temperatureC < TOO_COLD_C) {
        newState = TEMP_COLD;} //if temperature reading is too cold sprite state is too cold
    else if (temperatureC > TOO_HOT_C) {
        newState = TEMP_HOT;} //if temperature reading is too hot sprite state is too hot
    else if (tempState == TEMP_COLD && temperatureC >= TOO_COLD_C + TEMP_BUFFER_C) {
        newState = TEMP_OK;} //if temp is too cold but its within buffer, sprite state is ok
    else if (tempState == TEMP_HOT && temperatureC <= TOO_HOT_C - TEMP_BUFFER_C) {
        newState = TEMP_OK;}//if temp is too hot but its within buffer, sprite state is ok
    if (newState != tempState) {
        tempState = newState; //updates the temperature state if new is different from old
        if (tempState == TEMP_COLD) {
            PLAY_SOUND(coldNotes, coldDur);} //if updated temp state is cold plays cold music/notes(idk what to call it)
        else if (tempState == TEMP_HOT) {
            PLAY_SOUND(hotNotes, hotDur);} //if updated temp state is hot plays hot music/notes
    }
}

//AXDL345 acts as a mouse when the menu icon is open. The position when button1 is pressed becomes the centre 
void updateMenuBall() {
    float xTilt = accelerationX - menuCenterAccelerationX;
    float yTilt = accelerationY - menuCenterAccelerationY;
    if (SWAP_ADXL345_AXES) {
        const float oldXTilt = xTilt;
        xTilt = yTilt;
        yTilt = oldXTilt;} //swaps x and y values/readings
        xTilt = constrain(xTilt * MENU_X_DIRECTION,-MENU_TILT_LIMIT,MENU_TILT_LIMIT); //allow for motion only positivelt ot negatively within set constraints
        yTilt = constrain(yTilt * MENU_Y_DIRECTION,-MENU_TILT_LIMIT,MENU_TILT_LIMIT);//allow for motion only positivelt ot negatively within set constraints
        const float targetX = MENU_CENTER_X + (xTilt / MENU_TILT_LIMIT)* MENU_BALL_X_RANGE;
        const float targetY = MENU_CENTER_Y + (yTilt / MENU_TILT_LIMIT)*MENU_BALL_Y_RANGE;

//This makes ball feel smoother
menuBallX += (targetX - menuBallX) * 0.20f;
menuBallY += (targetY - menuBallY) * 0.20f;

//tilt direction past dead zone with menu open selects menu item
//deadzone stays unselcted while flat
if (fabsf(xTilt) < MENU_DEAD_ZONE && fabsf(yTilt) < MENU_DEAD_ZONE) {
    selectedMenuItem = -1;} 
else { float targetX = 64.0f + (xTilt*30.0f);
    float targetY = 32.0f + (yTilt*30.0f); //Converts tilts value to screen target coordinates
    int bestIndex =- 1;
    float shortestDistanceSq = 999999.0f;
    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
    float dx = targetX - MENU_ITEM_X[i];
    float dy = targetY - MENU_ITEM_Y[i];
    float distanceSq = (dx * dx) + (dy * dy);

    if (distanceSq < shortestDistanceSq) {
      shortestDistanceSq = distanceSq;
      bestIndex = i;}
    }
selectedMenuItem = bestIndex;
}}
void startDizzy(){
    const uint32_t now = millis();
    stopAllReactions();
    frozenPetX = petWalkingX(now);
    dizzyUntil = now + DIZZY_DURATION_MS;
    shakeCount = 0;
    changePet(DIZZY_JOY_CHANGE,DIZZY_ENERGY_CHANGE, 0, DIZZY_HEALTH_CHANGE); //updates changes in stats when shaken
    savePet(); //saves new pet stats
    PLAY_SOUND(dizzyNotes, dizzyDur);} //plays sound/notes for dizzy pet


void checkForShake () {
    if (!adxlFound || currentView != PET_VIEW||isDizzy()|| petIsSick){
        return;} //a dizzy or sick pet can't be shaken
        const float magnitude = sqrtf(accelerationX * accelerationX + accelerationY * accelerationY + accelerationZ * accelerationZ);
    const uint32_t now = millis();
    if (fabsf(magnitude - STANDARD_GRAVITY) >= SHAKE_THRESHOLD &&
             now - lastShakeAt >= SHAKE_COOLDOWN_MS){
        lastShakeAt = now;
        nappingUntil = 0;  //Shake wakes up sleeping pet
        shakeAnimationEndsAt = now+350;
        changePet(SHAKE_JOY_CHANGE, SHAKE_ENERGY_CHANGE,SHAKE_FULLNESS_CHANGE,SHAKE_HEALTH_CHANGE);
        savePet();
        if (shakeCount == 0 || now - firstShakeAt > DIZZY_WINDOW_MS){
            shakeCount = 0;
            firstShakeAt = now;}
        shakeCount++;
        if (shakeCount >= DIZZY_SHAKE_COUNT){
            startDizzy();} // plaays dizzy reaction is shake count is more than 3
        else {
            buzzer.beep(60);}}
}

//opens menu, resets selection and startin position depending on how user is holding the device
void openMenu (){
    currentView = MENU_VIEW;
    selectedMenuItem = -1;
    menuBallX = MENU_CENTER_X;
    menuBallY = MENU_CENTER_Y;
    menuCenterAccelerationX = accelerationX;
    menuCenterAccelerationY = accelerationY;
    buzzer.beep(30);} //plays sound on opening menu


void chooseMenuItem() { 
    if (selectedMenuItem == -1){return;} //keeps menu open untill the user makes a choice
    if (petIsSick && selectedMenuItem != MEDS_MENU_INDEX){
        PLAY_SOUND(deniedNotes, deniedDur);return;}//can only feed sick pet meds
    const MenuItem & item = MENU_ITEMS [selectedMenuItem];
    changePet(item.joyChange, item.energyChange, item.fullnessChange, item.healthChange);
    savePet();
    const uint32_t now = millis(); //this retrives menu option according to choice,updates stats,time
    nappingUntil = 0;
    dizzyUntil = 0;
    heartAnimationEndsAt = 0;
    playRunStartedAt = 0;//clears any timers/actions pet does new action
    if  (item.reaction == NAP_REACTION) {
        petJumpStartedAt = 0;
        nappingPetX = petWalkingX(now);
        nappingUntil = now + NAP_DURATION;} //pet stops walking and sleeps
    else if (item.reaction == RUN_REACTION) {
        petJumpStartedAt = 0;
        playRunStartedAt = now;
        heartAnimationEndsAt = now + playRunDuration();} //cancels and jumps,makes pet run(play),plays heart animation
    else{
        petJumpStartedAt = now;
        if(item.reaction == HEART_REACTION) {
            heartAnimationEndsAt = now + HEARTS_DURATION_MS;}} //jump reaction for some actions and starts heart reaction
    if (item.reaction == NAP_REACTION){
        PLAY_SOUND(napNotes, napDur);} //plays nap sound/notes if napping
    else if (selectedMenuItem == MEDS_MENU_INDEX){
        PLAY_SOUND(sadNotes, sadDur);} //plays sad music if meds given
    else {
        PLAY_SOUND(happyNotes, happyDur);} //plays happy notes if it happy (feed,pet,play)
    currentView = PET_VIEW;
    } //Closes menu and shows pet again

void handleButtons() { //what happens when buttons are pressed
    if(wasPressed(buttonOne)){
        if (currentView == MENU_VIEW){
            chooseMenuItem();}
        else {
            openMenu();}  //Button 1- if menu is not opened it opens menu,
    }
    if (wasPressed(buttonTwo)){ //displays stats view
        currentView = currentView == STATS_VIEW ? PET_VIEW : STATS_VIEW;}
}


void drawHeart (int x, int y){
    display.fillRect(x - 2, y, 2, 2, SSD1306_WHITE);
    display.fillRect(x + 1, y, 2, 2, SSD1306_WHITE);
    display.fillRect(x - 3, y + 2, 7, 2, SSD1306_WHITE);
    display.fillRect(x - 2, y + 4, 5, 1, SSD1306_WHITE);
    display.fillRect(x - 1, y + 5, 3, 1, SSD1306_WHITE);
    display.drawPixel(x, y + 6, SSD1306_WHITE);
} //draws a heart 

void drawHearts(uint32_t now, int petX, int petY){
    if (now >= heartAnimationEndsAt){
        return;} //checks timer before drawing next heart
    //calculates progress of drawing heart between 0-1
    const float progress = 1.0f - static_cast<float>(heartAnimationEndsAt - now) / HEARTS_DURATION_MS;
    const int rise = static_cast<int>(progress * 18.0f);
    drawHeart(petX +10, petY -3 -rise);
    drawHeart(petX + 22, petY - 9 - rise/2);}

void drawMusicNote(int x, int y) {
  display.drawPixel(x - 3, y + 2, SSD1306_WHITE);
  display.drawPixel(x - 2, y + 2, SSD1306_WHITE);
  display.drawPixel(x - 3, y + 3, SSD1306_WHITE);
  display.drawPixel(x - 2, y - 2, SSD1306_WHITE);
  display.drawPixel(x - 2, y - 1, SSD1306_WHITE);
  display.drawPixel(x - 2, y, SSD1306_WHITE);
  display.drawPixel(x - 2, y + 1, SSD1306_WHITE);
  display.drawPixel(x - 2, y - 3, SSD1306_WHITE);
  display.drawPixel(x - 1, y - 3, SSD1306_WHITE);
  display.drawPixel(x, y - 3, SSD1306_WHITE);
  display.drawPixel(x + 1, y - 3, SSD1306_WHITE);
  display.drawPixel(x + 2, y - 3, SSD1306_WHITE);
  display.drawPixel(x + 2, y - 2, SSD1306_WHITE);
  display.drawPixel(x + 2, y - 1, SSD1306_WHITE);
  display.drawPixel(x + 2, y, SSD1306_WHITE);
  display.drawPixel(x + 2, y + 1, SSD1306_WHITE);
  display.drawPixel(x + 1, y + 2, SSD1306_WHITE);
  display.drawPixel(x + 2, y + 2, SSD1306_WHITE);
  display.drawPixel(x + 1, y + 3, SSD1306_WHITE);
}

void drawSleepZs(uint32_t now, int petX, int petY) {//Z's float upwards then loop back down when pet naps
    const int rise = static_cast<int>((now/300UL) % 15UL);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(petX + 28, petY - 8 -rise /2);
    display.print("z");} //sets text size and color then determines where the text should start from,prints Z

void drawDizzy(uint32_t now, int petX, int petY){
    display.drawBitmap(petX + 8, petY - 16, epd_bitmap_dizzy, 16, 16, SSD1306_WHITE);}

void drawSick(uint32_t now, int petX, int petY){
    display.drawBitmap(petX + 4, petY - 24, epd_bitmap_sick_reaction, 24, 24, SSD1306_WHITE);}
    
void drawCold (uint32_t now, int petX, int petY){
    display.drawBitmap(petX, petY - 30, epd_bitmap_cold_reaction1, 32, 32, SSD1306_WHITE);}

void drawHot (uint32_t now, int petX, int petY){
    display.drawBitmap(petX + 16, petY - 16, epd_bitmap_warm_reaction, 16, 16, SSD1306_WHITE);}

void drawPet(){
    display.clearDisplay();
    const uint32_t now = millis(); //calculates pets position on the screen,overlays effects
    int petX = isNapping() ? nappingPetX : petWalkingX(now);
    if (isPlaying()) {
        petX =playRunX(now);}
    int petY = SCREEN_HEIGHT- PET_SPRITE_HEIGHT; //The pet wanders by default,nap/play take over
    if(isDizzy()|| petIsSick){
        petX = frozenPetX;}  //dizzy or sick pets don't move
    if (now < shakeAnimationEndsAt){
        petX += static_cast<int>(sinf(now / 18.0f) * 3.0f);}
    if (!isNapping() && petJumpStartedAt != 0){
        const uint32_t animationAge = now - petJumpStartedAt;
        if (animationAge < PET_PRE_JUMP_MS){
            petX += static_cast<int>(sinf(now/16.0f) * 3.0f);}
        else if (animationAge < PET_PRE_JUMP_MS + PET_JUMP_MS){
            const float jumpProgress = static_cast<float>(animationAge-PET_PRE_JUMP_MS)/PET_JUMP_MS;
            petY -= static_cast <int>(sinf(jumpProgress * PI)*PET_JUMP_HEIGHT);}
        else{
            petJumpStartedAt = 0;}
    }
    petX = constrain(petX,0,SCREEN_WIDTH-PET_SPRITE_WIDTH);
    display.drawBitmap(petX, petY , PET_SPRITE, PET_SPRITE_WIDTH, PET_SPRITE_HEIGHT, SSD1306_WHITE);
    //draws pets image on display in white
    if (!isNapping() && (now / 250) % 2 == 0) {
       drawMusicNote(petX - 4, petY + 18);}
    if (isNapping()){
        drawSleepZs (now, petX, petY);}
    drawHearts(now, petX, petY); //ZZZ if sleeping heart animation if affection action active
    if (isDizzy()){
        drawDizzy(now,petX, petY);}
    if(petIsSick){
        drawSick(now, petX, petY);}
    if(tempState == TEMP_COLD){
        drawCold(now, petX, petY);}
    if(tempState == TEMP_HOT){
        drawHot(now,petX , petY);}} //draws hot,cold,sick and dizzy reactions 

void drawMenuItem(int item) {
  const int boxWidth = 33;
  const int boxHeight = 12;
  const int boxX = MENU_ITEM_X[item] - boxWidth / 2;
  const int boxY = MENU_ITEM_Y[item] - boxHeight / 2;
  const bool isSelected = item == selectedMenuItem;
  if (isSelected) {
    display.fillRoundRect(boxX, boxY, boxWidth, boxHeight, 3, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);}
 else {display.drawRoundRect(boxX, boxY, boxWidth, boxHeight, 3, SSD1306_WHITE);
       display.setTextColor(SSD1306_WHITE);}
  display.setTextSize(1);
  const int labelLength = strlen(MENU_ITEMS[item].label);
  display.setCursor(MENU_ITEM_X[item] - labelLength * 3, MENU_ITEM_Y[item] - 3);
  display.print(MENU_ITEMS[item].label);
  display.setTextColor(SSD1306_WHITE);} //This displays the menu items in seperate boxes(?)


void drawMenu(){
    display.clearDisplay();
    for (int item =0; item < MENU_ITEM_COUNT; item++){
        drawMenuItem(item);}
    display.drawCircle(MENU_CENTER_X, MENU_CENTER_Y, 11, SSD1306_WHITE);
    display.fillCircle(static_cast<int>(menuBallX),static_cast<int>(menuBallY), 4,  
                       SSD1306_WHITE);} //draws menu items and then the menu center mouse circle

void drawStatBar (int y,const char * label, int value){
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(4,y);
    display.print(label);
    display.drawRect(48, y, 57, 8, SSD1306_WHITE); //outline of progress bar
    display.fillRect(49, y+1, map(value, 0, 100, 0, 55), 6, SSD1306_WHITE ); //it is filled UP
    display.setCursor(109, y);
    display.print(value);}

void drawStats(){
    display.clearDisplay();
    drawStatBar(2,  "JOY",    pet.joy);
    drawStatBar(14, "ENG",    pet.energy);  
    drawStatBar(26, "FULL",   pet.fullness);
    drawStatBar(38, "HLTH",   pet.health);   //displays the stats as numbers
    display.setCursor(4, 53);
    if (!isnan(temperatureC)) {
        display.print("TEMP ");
        display.print(static_cast<int>(temperatureC));
        display.print("C");}
    else {display.print("TEMP --");}
    display.setCursor(76, 53);
    if (!isnan(humidity)) {
       display.print("H ");
       display.print(static_cast<int>(humidity));
       display.print("%");}
    else {display.print("H --");}
}

void drawCurrentView(){
    if (currentView == MENU_VIEW){
        drawMenu();}
    else if (currentView == STATS_VIEW){
        drawStats();}
    else {drawPet();}
    display.display();} //determines what the current view is and display it

void setup(){
    Serial.begin(115200);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    setUpButton(buttonOne);
    setUpButton(buttonTwo);
    loadPet();
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
       Serial.println("OLED not found. Check power, GND, SDA, SCL, and address.");
       while (true) {delay(10);}}
    adxlFound = accel.begin(0x53);
    if (adxlFound) {
        accel.setRange(ADXL345_RANGE_8_G);
        accel.setDataRate(ADXL345_DATARATE_100_HZ);}
    else{Serial.println("ADXL345 not found. The menu will not move until it is connected.");}
    dht.begin();
    dhtFound = true;
    drawCurrentView();} //Checks if all hardware components are connected, gives a warning if essential components
    //like the accelerometer (conrols menu) and the display are not connected

void loop() {
  updateAdxl();         
  updateDht();          
  updatePetTimers(); 
  buzzer.loop();
  updateSickState();
  updateTempState(); 
  handleButtons();      
  if (currentView == MENU_VIEW) {updateMenuBall();}   // Update menu cursor selection based on tilt angles
  else {checkForShake();}  // Detects shakes
  drawCurrentView();     // Draw current view (either stats or menu or pet view(normal display))
  delay(16);}            //smooth display animations

