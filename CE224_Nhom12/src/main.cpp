/*-------------------------------- Includes --------------------------------*/
#include <mbed.h>
#include "Adafruit_SSD1306.h"

/*--------------------------------- Define ---------------------------------*/
// begin
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define DISPLAY_ADDRESS 0x3C
#define REFRESH_TIME 5
#define I2C_MASTER_ADDRESS 0x01
#define I2C_SENSOR_ADDRESS 0x30
// Flags
#define NEW_SENSING_CYCLE_FLAG (1UL << 0)
#define DONE_SENSING_FLAG (1UL << 1)
// end
/*------------------------------- Namespace -------------------------------*/
using namespace std;
using namespace mbed;
using namespace rtos;
/*-------------------------------- Typedef --------------------------------*/
typedef struct
{
  float alcohol = 0.0, methane = 0.0, carbon_monoxide = 0.0;
} Package;
/*------------------------------- Instances--------------------------------*/
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
EventFlags flags; // Event flags
Thread mainThread;
Thread sensorThread;
Thread displayThread;
Thread checkFlags;
// Thread memMgrThread;
Ticker senseTicker;
Queue<Package, 16> Data; // Queue for raw input data
MemoryPool<Package, 16> memPool;

/*-------------------------- Function prototypes --------------------------*/
void App_Task();
void processInput_Task();
void display_Task();
void startNewSensingCycle();
// void memMgr_Task();

/*---------------------------------- Code ----------------------------------*/
void setup()
{
  Serial.begin(9600); // Initialize serial port
  Wire.begin();       // Initialize I2C

  display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS); // Initialize display

  senseTicker.attach(&startNewSensingCycle, chrono::seconds(REFRESH_TIME)); // Auto set Sensing flag every 5 seconds
  mainThread.start(App_Task);
  sensorThread.start(processInput_Task);
  displayThread.start(display_Task);
}

void loop()
{
}

void App_Task()
{
  while (1)
  {
  }
}

void processInput_Task()
{

  while (1)
  {
    flags.wait_any(NEW_SENSING_CYCLE_FLAG); // Wait for sensing signal

    Serial.println("Sensing..."); // Start sensing
    Package *pack = memPool.try_alloc();
    Wire.requestFrom(I2C_SENSOR_ADDRESS, sizeof(Package));
    Wire.readBytes((byte *)pack, sizeof(Package));

    Data.try_put(pack); // Push data to queue

    flags.set(DONE_SENSING_FLAG);

    Serial.println("Done sensing"); // End sensing
  }
}

void display_Task()
{
  float displayAlcohol = 0.0, displayCH4 = 0.0, displayCO = 0.0;
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  while (1)
  {
    if (flags.wait_any(DONE_SENSING_FLAG) == DONE_SENSING_FLAG) // If new data processed
    {
      Package *pack = nullptr;
      Data.try_get(&pack);
      displayAlcohol = pack->alcohol;
      displayCH4 = pack->methane;
      displayCO = pack->carbon_monoxide;
      memPool.free(pack);

      Serial.println("Done reading");
    }

    Serial.println("Displaying...");
    Serial.println(displayAlcohol);
    Serial.println(displayCH4);
    Serial.println(displayCO);

    display.setCursor(0, 0);
    display.clearDisplay();
    display.print("Alcohol: ");
    display.print(displayAlcohol);
    display.println(" ppm");
    display.print("CH4: ");
    display.print(displayCH4);
    display.println(" ppm");
    display.print("CO: ");
    display.print(displayCO);
    display.println(" ppm");
    display.display();

    ThisThread::sleep_for(chrono::seconds(1));
  }
}

void startNewSensingCycle()
{
  flags.set(NEW_SENSING_CYCLE_FLAG); // Set sensing flag every 5 seconds
}
