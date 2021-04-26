#include <Arduino.h>
#include <simio.h>
#include <Wire.h>
#include <stdint.h>
#include <EEPROM.h>
// #include <LiquidCryst5al_I2C.h>

// For using in proteus
#define DEBUG

// If you need EEPROM or not
#define USE_EEPROM

// Set this with values between 0 and 255 depending on the value of the photoresistance when a coin cuts the beam.
#define LASER_FLOOR 50

// The number of coin counter you have
#define COL_COUNT 3

// Identification for columns, by default : 50 centimes, 1 and 2 euros.
#define CT50 0
#define EUR1 1
#define EUR2 2

#ifdef DEBUG
unsigned long tempser;
#endif

#ifdef DEBUG
#include <SoftwareSerial.h>
#endif

#ifdef USE_EEPROM
unsigned long eeprom_timer;
#endif

// Data struct representing column
typedef struct s_column
{
	uint16_t count;					// the number of coin in it
	float value;					// the value of ONE coin
	Mlpx_7Seg_BCD disp;				// explicit (SIODM library)
	uint8_t analogPin;				// explicit
	Button button;					// explicit (SIODM library)
	unsigned long detec_time_inter; // Min time between 2 decrement
} Column;

// ---- Data allocation ----
Column columns[COL_COUNT];
unsigned long tempslcd;
Button reset;
float balance = 0, cachelcd;
// -------------------------

// LiquidCrystal_I2C lcd(0x3f, 2, 1, 0, 4, 5, 6, 7); // 0x3f is the I2C bus address for an unmodified backpack 3e proteus

void setup()
{
#ifdef DEBUG
	Serial.begin(9600);
	Serial.print("Blindey project, initialization...\n");
#endif

#ifdef USE_EEPROM // Get coin number from cold memory
	columns[CT50].count = EEPROM.read(CT50 * sizeof(uint16_t));
	columns[EUR1].count = EEPROM.read(EUR1 * sizeof(uint16_t));
	columns[EUR2].count = EEPROM.read(EUR2 * sizeof(uint16_t));
#else
	columns[EUR1].count = 0;
	columns[EUR2].count = 0;
#endif

	columns[CT50].value = 0.5; // 50 centimes is 0.5 euros
	columns[EUR1].value = 1.0; // and so on
	columns[EUR2].value = 2.0; // and so on

	// --- Devices initialization, refer to schemas for pin mapping ---
	columns[CT50].analogPin = A0;
	columns[EUR1].analogPin = A1;
	columns[EUR2].analogPin = A2;

	// lcd.begin(16, 2);
	// lcd.setBacklightPin(3, 1);
	// lcd.setBacklight(HIGH);
	// lcd.print("Hello !");
	// lcd.setCursor(0, 1);
	// lcd.print("Initialization :");

	create_Mlpx_7Seg_BCD(&(columns[CT50].disp), 22, 23, 24, 25, true, 2, 2, 3);
	create_Mlpx_7Seg_BCD(&(columns[EUR1].disp), 28, 29, 30, 31, true, 2, 4, 5);
	create_Mlpx_7Seg_BCD(&(columns[EUR2].disp), 34, 35, 36, 37, true, 2, 6, 7);

	create_button(&(columns[CT50].button), 11, true);
	create_button(&(columns[EUR1].button), 12, true);
	create_button(&(columns[EUR2].button), 13, true);

	create_button(&reset, 53, true);
	// ----------------------------------------------------------------
#ifdef DEBUG
	Serial.begin(9600);
	Serial.print("Initialization finished.\n");
#endif
}

void loop()
{

	balance = 0;
	for (uint8_t i = 0; i < COL_COUNT; i++)
		balance += columns[i].count * columns[i].value;

	// if ((cachelcd != balance) && ((millis() - tempslcd) > 5000))
	// {
	// 	cachelcd = balance;
	// 	lcd.clear();
	// 	lcd.setCursor(0, 0);
	// 	lcd.print("Le total est de :");
	// 	lcd.setCursor(0, 1);
	// 	lcd.print(balance);
	// 	lcd.print(balance > 1 ? " euros" : "euro"); // change with your money name

	// 	tempslcd = millis();
	// }

	for (uint8_t c = 0; c < COL_COUNT; c++)
	{
		print_int_Mlpx_7Seg_BCD(&(columns[c].disp), columns[c].count); // explicit (SIODM library)

		// If the button corresponding to the columns[c] is pressed, incrementing
		if (is_pushed_button(&(columns[c].button)) && columns[c].count > 0)
		{
			columns[c].count--;
#ifdef USE_EEPROM
			eeprom_timer = millis();
#endif
		}

		// Same but decrementing when beam is cut.
		if ((analogRead(columns[c].analogPin) <= LASER_FLOOR) && (millis() - columns[c].detec_time_inter >= 100) && columns[c].count < 99)
		{
			columns[c].count++;
			columns[c].detec_time_inter = millis();
#ifdef USE_EEPROM
			eeprom_timer = millis();
#endif
		}
	}

#ifdef DEBUG
	if ((millis() - tempser) > 1000)
	{
		Serial.println(balance);
		tempser = millis();
	}
#endif

#ifdef USE_EEPROM
	if ((millis() - eeprom_timer) > 60000)
	{
		EEPROM.update(CT50 * sizeof(uint16_t), columns[CT50].count);
		EEPROM.update(EUR1 * sizeof(uint16_t), columns[EUR1].count);
		EEPROM.update(EUR2 * sizeof(uint16_t), columns[EUR2].count);
		eeprom_timer = millis();
	}
#endif

	if (is_pushed_button(&reset))
	{
		for (uint8_t i = 0; i < COL_COUNT; i++)
			columns[i].count = 0;
#ifdef USE_EEPROM
		EEPROM.update(CT50 * sizeof(uint16_t), 0);
		EEPROM.update(EUR1 * sizeof(uint16_t), 0);
		EEPROM.update(EUR2 * sizeof(uint16_t), 0);
#endif
	}
}

// ---------------------------------------------------------------------------
