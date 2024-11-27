/*
Arduino CAN display for Life Racing / Syvecs ECU
Eliot Mansfield 2024

Arduino MKR Zero with Arduino CANBUS Shield with 2.8" ILI9341 SPI Display

Pinout:
Display     MKR Zero             
VCC         VCC (3.3v)            
GND         GND                   
CS          PIN5                 
Reset       Reset (next to gnd)   
DC          PIN4                  
SDI(MOSI)   PIN8(marked MOSI)    
SCLK        PIN9(marked SCK)      
LED         VCC (3.3v)           
SDO(MISO)   PIN10(marked MISO)    

Momentary switch between digital input 6 and ground to change pages

Version 1.0 - Baseline 22/10
Version 1.1 - Added FRAME33 knock retard frames
*/


#include <CAN.h>
#include "SPI.h"
#include "Adafruit_ILI9341.h"

// Define expected CAN frame IDs
#define FRAME29 0x61C
#define FRAME30 0x61D
#define FRAME31 0x61E
#define FRAME32 0x61F
#define FRAME33 0x620
#define TOTAL_FRAMES 5  // Total number of frames to wait for
#define TFT_DC 4
#define TFT_CS 5
//Spacing between lines
#define LINE1 15
#define LINE2 35
#define LINE3 55
#define LINE4 75
#define LINE5 95
#define LINE6 115
#define LINE7 135
#define LINE8 155
#define LINE9 175
#define LINE10 195
#define LINE11 215
#define LINE12 235
//First and Second Columns of Data
#define COL1 75        //1st data column for big font
#define COL2 190       //2nd data column for big font
#define COLA 58        //1st data column of a three row
#define COLB 175       //2nd  data column of a three row
#define COLC 275       //3rd  data column of a three row
#define FONTCENTRE 10  //offset applied to line number to align a smaller font with a bigger font
//Set the size of the text..1 is smaller 3 is bigger
#define TEXTSIZE 2   //Smaller text font
#define TEXTSIZE1 4  //larger text font
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define buttonpin 6   //Digital Input pin
int currentcal = 13;  //initialise and store a known invalid cal, to force a correct cal to be populated on first loop.
int buttonstatus = 1;
int pageIndex = 0;  // Current page index (0 = page 1, 1 = page 2)
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);


struct CANFrame {
  unsigned long id;  // CAN Frame ID
  int length;        // CAN Frame length
  byte data[8];      // CAN Frame data
};

CANFrame canBuffer[TOTAL_FRAMES];                                          // Buffer to store the 4 CAN frames
bool frameReceived[TOTAL_FRAMES] = { false, false, false, false, false };  // Flags to check if each frame is received
int framesCollected = 0;                                                   // Counter for the number of frames received

void setup() {
  Serial.begin(115200);

  pinMode(buttonpin, INPUT_PULLUP);

  if (!CAN.begin(250E3)) {
    Serial.println("Starting CAN failed!");
    while (1)
      ;
  }

  tft.begin();
  drawscreen();  //Function that draws the basic screen

  // delay(3000);  // wait a while for serial to start
  // Serial.println("Life/Syvecs CAN Bus Display Setup Complete...waiting for CAN packets.");
}

void loop() {
  // Try to read incoming CAN frames
  if (CAN.parsePacket()) {
    unsigned long id = CAN.packetId();

    // Determine which buffer slot this frame belongs to
    int bufferIndex = -1;
    if (id == FRAME29) bufferIndex = 0;
    else if (id == FRAME30) bufferIndex = 1;
    else if (id == FRAME31) bufferIndex = 2;
    else if (id == FRAME32) bufferIndex = 3;
    else if (id == FRAME33) bufferIndex = 4;

    // If the frame ID matches one of the expected frames
    if (bufferIndex != -1 && !frameReceived[bufferIndex]) {
      canBuffer[bufferIndex].id = id;
      canBuffer[bufferIndex].length = CAN.packetDlc();

      for (int i = 0; i < CAN.packetDlc(); i++) {
        canBuffer[bufferIndex].data[i] = CAN.read();
      }

      // Mark this frame as received
      frameReceived[bufferIndex] = true;
      framesCollected++;
    }
  }

  buttonstatus = digitalRead(buttonpin);  //If digital button pressed - increment screen
  if (buttonstatus == 0) {
    //Increment pageIndex and loop back if needed
    pageIndex = (pageIndex + 1) % 2;  // Assuming 2 pages; modify as needed
    // Clear the screen and draw the new page
    tft.fillScreen(BLACK);
    if (pageIndex == 0) {
      drawscreen();  // Page 1 (current display)
    } else if (pageIndex == 1) {
      drawscreenPage2();  // Page 2 (new data)
    }
  }

  // If all 5 frames have been received, process and display them
  if (framesCollected == TOTAL_FRAMES) {
    for (int i = 0; i < TOTAL_FRAMES; i++) {
      processCANFrame(canBuffer[i]);
    }

    // Reset the buffer for the next set of frames
    for (int i = 0; i < TOTAL_FRAMES; i++) {
      frameReceived[i] = false;
    }
    framesCollected = 0;  // Reset the counter
  }
}





void processCANFrame(CANFrame& frame) {
  unsigned long pngID = frame.id;
  byte* data = frame.data;

  if (pageIndex == 0) {  //Display Data for the First Page
    if (pngID == FRAME29) {
      Serial.println("Frame29");
      //Slot 1 - Closed Loop Lambda Target Air/Fuel Ratio
      unsigned int s1 = data[0];
      unsigned int s2 = data[1];
      float cllTarg1 = (((s1 * 256) + s2) * .001);
      // Serial.print("cllTarg1: ");
      // Serial.println(cllTarg1);
      tft.setTextColor(WHITE, BLACK);
      tft.setTextSize(TEXTSIZE1);
      tft.setCursor(COL1, LINE3);
      tft.println(cllTarg1);
      tft.setTextColor(GREEN, BLACK);
      tft.setTextSize(TEXTSIZE1);
      tft.setCursor(COL2, LINE3);
      tft.println(cllTarg1 * 14.71);  //In AFR

      //slot 2 - Manifold Absolute Pressure
      // unsigned int s3 = data[2];
      // unsigned int s4 = data[3];
      // float map1 = (((s3 * 256) + s4));
      // Serial.print("map1: ");
      // Serial.println(map1);

      //slot 2 - Engine Oil Pressure
      double s3 = data[2];
      double s4 = data[3];
      //float eop = (((s1 * 256) + s2) / 1000);
      float eop = (((s3 * 256) + s4) * 0.0145038);
      // Serial.print("EOP: ");
      // Serial.println(eop);
      //https://passionford.com/forum/ford-sierra-sapphire-rs500-cosworth/487800-oil-pressure-question-on-a-yb-cosworth.html
      //https://passionford.com/forum/general-car-related-discussion/240155-cosworth-oil-pressure-query.html
      if (eop <= 25) {
        tft.setTextColor(RED, BLACK);
      } else if ((eop >= 26) && (eop < 34)) {
        tft.setTextColor(YELLOW, BLACK);
      } else {
        tft.setTextColor(GREEN, BLACK);
      }
      if ((eop > 0) && (eop < 200)) {
        tft.setTextSize(TEXTSIZE1);
        tft.setCursor((COL2 + 30), LINE1);
        tft.print(eop, 0);
        tft.println(" ");
      } else {
        tft.setTextSize(TEXTSIZE1);
        tft.setCursor((COL2 + 30), LINE1);
        tft.print(0, 0);
        tft.println("  ");
      }


      //slot 3 - Lambda Reading
      unsigned int s5 = data[4];
      unsigned int s6 = data[5];
      float lam1 = (((s5 * 256) + s6) * .001);
      // Serial.print("lam1: ");
      // Serial.println(lam1);
      tft.setTextColor(WHITE, BLACK);
      tft.setTextSize(TEXTSIZE1);
      tft.setCursor(COL1, LINE5);
      tft.println(lam1);
      tft.setTextColor(GREEN, BLACK);
      tft.setTextSize(TEXTSIZE1);
      tft.setCursor(COL2, LINE5);
      tft.println(lam1 * 14.71);  //In AFR

      //slot 4 - Correction amount applied to FuelFinal based on Closed loop lambda, 1 is no correction… Lower than 1 is removing fuel…. Higher is adding
      double s7 = data[6];
      double s8 = data[7];
      float fuelMltCll1 = (((s7 * 256) + s8) / 4096);
      fuelMltCll1 = (fuelMltCll1 * 100) - 100;  // convert into percentage
      // Serial.print("fuelMltCll1: ");
      // Serial.println(fuelMltCll1);
      tft.setTextColor(WHITE, BLACK);
      tft.setTextSize(TEXTSIZE1);
      tft.setCursor(COL1, LINE7);
      tft.print(fuelMltCll1, 2);
      tft.print("% ");
    } else if (pngID == FRAME30) {
      Serial.println("Frame30");
      //SLOT1 - Fuel injector duty %
      double s1 = data[0];
      double s2 = data[1];
      float fuelDutyPri1 = (((s1 * 256) + s2) / 40.96);
      // Serial.print("fuelDutyPri1: ");
      // Serial.println(fuelDutyPri1);
      if (fuelDutyPri1 >= 95) {
        tft.setTextColor(RED, BLACK);
      } else if ((fuelDutyPri1 >= 85) && (fuelDutyPri1 < 95)) {
        tft.setTextColor(YELLOW, BLACK);
      } else {
        tft.setTextColor(WHITE, BLACK);
      }
      tft.setTextSize(TEXTSIZE1);
      tft.setCursor(COL1, LINE1);
      tft.print(fuelDutyPri1, 0);
      tft.println("% ");

      //slot 2 - Act1 - Air charge temp
      double s3 = data[2];
      double s4 = data[3];
      float act1 = (((s3 * 256) + s4) / 10);
      // Serial.print("act1: ");
      // Serial.println(act1);
      tft.setTextColor(WHITE, BLACK);
      tft.setTextSize(TEXTSIZE);
      tft.setCursor(COLB, LINE11);
      tft.println(act1, 0);


      //slot 3 - ect1 - Engine Coolant temp
      double s5 = data[4];
      double s6 = data[5];
      float ect1 = (((s5 * 256) + s6) / 10);
      // Serial.print("ect1: ");
      // Serial.println(ect1);
      tft.setTextColor(WHITE, BLACK);
      tft.setTextSize(TEXTSIZE);
      tft.setCursor(COLA, LINE11);
      tft.println(ect1, 0);


      //slot 4 - Calibration Select
      unsigned int s7 = data[6];
      unsigned int s8 = data[7];
      float calSelect = (((s7 * 256) + s8) + 1);
      // Serial.print("calSelect: ");
      // Serial.println(calSelect);
      tft.setTextColor(WHITE, BLACK);
      tft.setTextSize(TEXTSIZE);
      tft.setCursor(COLC, LINE11);
      tft.println(calSelect, 0);
      if (calSelect != currentcal && currentcal != 13) {  //Routine to show a big calselect number if the cal changes.
        tft.fillScreen(BLACK);
        tft.setTextColor(WHITE, BLACK);
        tft.setTextSize(2);
        tft.setCursor(100, 1);
        tft.println("Calibration");
        tft.setCursor(100, 30);
        tft.setTextSize(25);
        tft.println(calSelect, 0);
        delay(500);    //not the best way - but i dont care if we miss a few frames whilst it's showing the cal
        drawscreen();  //redraw the basic screen layout after displaying the large cal select number
      }
      currentcal = calSelect;  //Store the currentcal so it can detect changes to the calselect



    } else if (pngID == FRAME31) {
      Serial.println("Frame31");
      //slot 1 - Battery Voltage
      double s1 = data[0];
      double s2 = data[1];
      float vbat = (((s1 * 256) + s2) / 1000);
      // Serial.print("Vbat: ");
      // Serial.println(vbat);
      tft.setTextColor(WHITE, BLACK);
      tft.setTextSize(TEXTSIZE);
      tft.setCursor(COLB, LINE9);
      tft.println(vbat);

      //Slot 4 - Engine Sync state
      unsigned int syncState = data[7];
      // Serial.print("syncState: ");
      // Serial.println(syncState);

      tft.setTextSize(TEXTSIZE);
      tft.setCursor(COLA, LINE9);
      switch (syncState) {
        case 0:
          tft.setTextColor(RED, BLACK);
          tft.println("Stop");
          break;
        case 1:
          tft.setTextColor(WHITE, BLACK);
          tft.println("Crk");
          break;
        case 2:
          tft.setTextColor(YELLOW, BLACK);
          tft.println("360 ");
          break;
        case 3:
          tft.setTextColor(GREEN, BLACK);
          tft.println("720 ");
          break;
        default:
          tft.setTextColor(BLACK, WHITE);
          tft.println(syncState);  //unkown value - so print the value
      }

      //Slot 2 - Limp Mode
      //SOME ARE COMMENTED OUT TO SAVE ON STORAGE SPACE AS THEY WOULDN'T BE APPLICABLE TO MY VEHICLE
      unsigned int limpMode = data[3];
      // Serial.print("limpMode: ");
      // Serial.println(limpMode);
      tft.setTextSize(TEXTSIZE);
      tft.setCursor(COLA, LINE10);
      switch (limpMode) {
        case 0:
          tft.setTextColor(WHITE, BLACK);
          tft.println("None            ");
          break;

        case 1:
          tft.setTextColor(RED, BLACK);
          tft.println("Limp Switch On  ");
          break;

        case 2:
          tft.setTextColor(RED, BLACK);
          tft.println("Coolant Cold    ");
          break;

          // case 3:
          // tft.setTextColor(RED,BLACK);
          // tft.println("Oil Cold");
          // break;

        case 4:
          tft.setTextColor(RED, BLACK);
          tft.println("Sens Warning Lvl");
          break;

          // case 5:
          // tft.setTextColor(RED,BLACK);
          // tft.println("Auto Trans");
          // break;

          // case 6:
          // tft.setTextColor(RED,BLACK);
          // tft.println("Vehicle Speed Fail");
          // break;

        case 100:
          tft.setTextColor(RED, BLACK);
          tft.println("Eng Oil Pressure");
          break;

          // case 101:
          // tft.setTextColor(RED,BLACK);
          // tft.println("CCP Trip");
          // break;

        case 102:
          tft.setTextColor(RED, BLACK);
          tft.println("Knock Shutdown  ");
          break;

          // case 103:
          //   tft.setTextColor(RED, BLACK);
          //   tft.println("Eng Oil Temp Trip");
          //   break;

        case 104:
          tft.setTextColor(RED, BLACK);
          tft.println("Engine Coolant Trip ");
          break;

        case 105:
          tft.setTextColor(RED, BLACK);
          tft.println("Fuel Press Trip ");
          break;

        case 106:
          tft.setTextColor(RED, BLACK);
          tft.println("Preignition Shutd");
          break;

          // case 107:
          // tft.setTextColor(RED,BLACK);
          // tft.println("Time on Load Limit");
          // break;

        default:
          tft.setTextColor(BLACK, WHITE);
          tft.println(limpMode);  //unkown value - so print the value
      }
    }
  }


  if (pageIndex == 1) {  //Display Data for the Second Page

    if (pngID == FRAME32) {
      Serial.println("Frame32");
      //SLOT1 - Cylinder 1 Knock
      double s1 = data[0];
      double s2 = data[1];
      float cyl01Knock = (((s1 * 256) + s2) / 40.96);
      tft.setTextSize(TEXTSIZE1);
      tft.setCursor(COL1 + 10, LINE3);
      tft.print(cyl01Knock, 0);
      tft.println("% ");

      //SLOT2 - Cylinder 2 Knock
      double s3 = data[2];
      double s4 = data[3];
      float cyl02Knock = (((s3 * 256) + s4) / 40.96);
      tft.setTextSize(TEXTSIZE1);
      tft.setCursor(COL1 + 10, LINE5);
      tft.print(cyl02Knock, 0);
      tft.println("% ");

      //SLOT3 - Cylinder 3 Knock
      double s5 = data[4];
      double s6 = data[5];
      float cyl03Knock = (((s5 * 256) + s6) / 40.96);
      tft.setTextSize(TEXTSIZE1);
      tft.setCursor(COL1 + 10, LINE7);
      tft.print(cyl03Knock, 0);
      tft.println("% ");

      //SLOT4 - Cylinder 4 Knock
      double s7 = data[6];
      double s8 = data[7];
      float cyl04Knock = (((s7 * 256) + s8) / 40.96);
      tft.setTextSize(TEXTSIZE1);
      tft.setCursor(COL1 + 10, LINE9);
      tft.print(cyl04Knock, 0);
      tft.println("% ");
    }

    if (pngID == FRAME33) {
      Serial.println("Frame33");
      //SLOT1 - Cylinder 1 Knock Retard
      double s1 = data[0];
      double s2 = data[1];
      float cyl01KnockIgnRtd = (((s1 * 256) + s2) / 32);
      tft.setTextSize(TEXTSIZE1);
      tft.setCursor(COL2 + 10, LINE3);
      if (cyl01KnockIgnRtd <= 1.99) {
        tft.setTextColor(GREEN, BLACK);
      } else if ((cyl01KnockIgnRtd >= 2) && (cyl01KnockIgnRtd < 4)) {
        tft.setTextColor(YELLOW, BLACK);
      } else {
        tft.setTextColor(RED, BLACK);
      }
      tft.print(cyl01KnockIgnRtd, 1);
      tft.println("' ");

      //SLOT2 - Cylinder 2 Knock Retard
      double s3 = data[2];
      double s4 = data[3];
      float cyl02KnockIgnRtd = (((s3 * 256) + s4) / 32);
      tft.setTextSize(TEXTSIZE1);
      tft.setCursor(COL2 + 10, LINE5);
      if (cyl02KnockIgnRtd <= 1.99) {
        tft.setTextColor(GREEN, BLACK);
      } else if ((cyl02KnockIgnRtd >= 2) && (cyl02KnockIgnRtd < 4)) {
        tft.setTextColor(YELLOW, BLACK);
      } else {
        tft.setTextColor(RED, BLACK);
      }
      tft.print(cyl02KnockIgnRtd, 1);
      tft.println("' ");

      //SLOT3 - Cylinder 3 Knock Retard
      double s5 = data[4];
      double s6 = data[5];
      float cyl03KnockIgnRtd = (((s5 * 256) + s6) / 32);
      tft.setTextSize(TEXTSIZE1);
      tft.setCursor(COL2 + 10, LINE7);
      if (cyl03KnockIgnRtd <= 1.99) {
        tft.setTextColor(GREEN, BLACK);
      } else if ((cyl03KnockIgnRtd >= 2) && (cyl03KnockIgnRtd < 4)) {
        tft.setTextColor(YELLOW, BLACK);
      } else {
        tft.setTextColor(RED, BLACK);
      }
      tft.print(cyl03KnockIgnRtd, 1);
      tft.println("' ");

      //SLOT4 - Cylinder 4 Knock Retard
      double s7 = data[6];
      double s8 = data[7];
      float cyl04KnockIgnRtd = (((s7 * 256) + s8) / 32);
      tft.setTextSize(TEXTSIZE1);
      tft.setCursor(COL2 + 10, LINE9);

      if (cyl04KnockIgnRtd <= 1.99) {
        tft.setTextColor(GREEN, BLACK);
      } else if ((cyl04KnockIgnRtd >= 2) && (cyl04KnockIgnRtd < 4)) {
        tft.setTextColor(YELLOW, BLACK);
      } else {
        tft.setTextColor(RED, BLACK);
      }
      tft.print(cyl04KnockIgnRtd, 1);
      tft.println("' ");
      tft.setTextColor(WHITE, BLACK);  //Cleanup colour back to White
    }
  }
}


//Function that redraws the the first screen
void drawscreen() {
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE, BLACK);
  // tft.setFont(&FreeMonoBoldOblique12pt7b);

  tft.setRotation(3);  // Set rotation to landscape mode
  tft.setTextColor(WHITE);
  tft.setTextSize(TEXTSIZE);

  // tft.setCursor(1, LINE1);
  // tft.println("     map:");
  tft.setCursor(1, LINE1 + FONTCENTRE);
  tft.println(" duty:");

  tft.setCursor(175, LINE1 + FONTCENTRE);
  tft.println("oil:");


  tft.setCursor(1, LINE3 + FONTCENTRE);
  tft.println(" targ:");


  tft.setCursor(1, LINE5 + FONTCENTRE);
  tft.println("  afr:");

  tft.setCursor(1, LINE7 + FONTCENTRE);
  tft.println("Corr%:");

  tft.setCursor(120, LINE9);
  tft.println("vbat:");
  tft.setCursor(1, LINE9);
  tft.println("Sync:");

  tft.setCursor(1, LINE10);
  tft.println("Limp:");
  tft.setCursor(1, LINE11);
  tft.println(" clt:");
  tft.setCursor(120, LINE11);
  tft.println(" act:");
  tft.setCursor(230, LINE11);
  tft.println("cal:");
}


void drawscreenPage2() {  //Knock Screen
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(TEXTSIZE);
  tft.setCursor(1, LINE1 + FONTCENTRE);
  tft.println("Knock  Level     Retard");

  // Display additional data in page 2
  tft.setCursor(1, LINE3 + FONTCENTRE);
  tft.println("Cyl 1:");

  tft.setCursor(1, LINE5 + FONTCENTRE);
  tft.println("Cyl 2:");

  tft.setCursor(1, LINE7 + FONTCENTRE);
  tft.println("Cyl 3:");

  tft.setCursor(1, LINE9 + FONTCENTRE);
  tft.println("Cyl 4:");

  // You can populate this with CAN data or other relevant information
}
