/**************************************************************************/
/*!
    @file     readntag203.pde
    @author   KTOWN (Adafruit Industries)
    @license  BSD (see license.txt)

    This example will wait for any NTAG203 or NTAG213 card or tag,
    and will attempt to read from it.

    This is an example sketch for the Adafruit PN532 NFC/RFID breakout boards
    This library works with the Adafruit NFC breakout
      ----> https://www.adafruit.com/products/364

    Check out the links above for our tutorials and wiring diagrams
    These chips use SPI or I2C to communicate.

    Adafruit invests time and resources providing this open source code,
    please support Adafruit and open-source hardware by purchasing
    products from Adafruit!
*/
/**************************************************************************/
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  (2)
#define PN532_MOSI (3)
#define PN532_SS   (4)
#define PN532_MISO (5)

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield

#define DOC_START "$@#"
#define DOC_END "&+^"
#define PAGE_SKIP '%'
#define PAGE_START '*'
#define METADATA_PAGE 225
#define LOCAL_POINTER_PAGE 220
#define CONTENT_SIZE 10
#define CONTENT_START 4
// Uncomment just _one_ line below depending on how your breakout or shield
// is connected to the Arduino:

// Use this line for a breakout with a software SPI connection (recommended):
//Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// Use this line for a breakout with a hardware SPI connection.  Note that
// the PN532 SCK, MOSI, and MISO pins need to be connected to the Arduino's
// hardware SPI SCK, MOSI, and MISO pins.  On an Arduino Uno these are
// SCK = 13, MOSI = 11, MISO = 12.  The SS line can be any digital IO pin.
//Adafruit_PN532 nfc(PN532_SS);

// Or use this line for a breakout or shield with an I2C connection:
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

#if defined(ARDUINO_ARCH_SAMD)
// for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
// also change #define in Adafruit_PN532.cpp library file
#define Serial SerialUSB
#endif

String url = "";
int url_length = 0;

void setup(void) {
#ifndef ESP8266
  while (!Serial); // for Leonardo/Micro/Zero
#endif
  Serial.begin(115200);
  Serial.println("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();

  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

  // configure board to read RFID tags
  nfc.SAMConfig();

  Serial.println("Waiting for an ISO14443A Card ...");
}

void loop(void) {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  // Wait for an NTAG203 card.  When one is found 'uid' will be populated with
  // the UID, and uidLength will indicate the size of the UUID (normally 7)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");

    if (uidLength == 7)
    {
      uint8_t data[32];
      uint8_t metadata[32]; // 4 bytes - pointer numData mostRecent 0xFE
      uint8_t lpdata[32]; // Local Pointer data

      // We probably have an NTAG2xx card (though it could be Ultralight as well)
      Serial.println("Seems to be an NTAG2xx tag (7 byte UID)");

      // NTAG2x3 cards have 39*4 bytes of user pages (156 user bytes),
      // starting at page 4 ... larger cards just add pages to the end of
      // this range:

      // See: http://www.nxp.com/documents/short_data_sheet/NTAG203_SDS.pdf

      // TAG Type       PAGES   USER START    USER STOP
      // --------       -----   ----------    ---------
      // NTAG 203       42      4             39
      // NTAG 213       45      4             39
      // NTAG 215       135     4             129
      // NTAG 216       231     4             225

      success = nfc.ntag2xx_ReadPage(METADATA_PAGE, metadata);
      success = nfc.ntag2xx_ReadPage(LOCAL_POINTER_PAGE, lpdata);
      

      if (success) {
        
        Serial.print(DOC_START);
        for (uint8_t i = 0; i < CONTENT_SIZE; i++)
        {
          //reads the tag and puts the data in the data variable; does this one page at a time
          success = nfc.ntag2xx_ReadPage( (CONTENT_START + (((metadata[0]+lpdata[0])%metadata[1])*CONTENT_SIZE))+i, data);


          Serial.print(PAGE_START);


          // Display the results, depending on 'success'
          if (success)
          {
            // Dump the page data
            nfc.PrintHex(data, 4);

          }
          else
          {
            Serial.print(PAGE_SKIP);// Page skip character
          }
        }
        Serial.print(DOC_END);

      }
      /*
            Serial.print(DOC_START);
            for (uint8_t i = 0; i < 22; i++)
            {
              //reads the tag and puts the data in the data variable; does this one page at a time
              success = nfc.ntag2xx_ReadPage(i, data);


              Serial.print(PAGE_START);


              // Display the results, depending on 'success'
              if (success)
              {
                // Dump the page data
                nfc.PrintHex(data, 4);

              }
              else
              {
                Serial.print(PAGE_SKIP);// Page skip character
              }
            }
            Serial.print(DOC_END);
      */
    }
    else
    {
      Serial.println("This doesn't seem to be an NTAG203 tag (UUID length != 7 bytes)!");
    }
    //Serial.println(url_length);
    //Serial.println(url);

    //Keyboard.begin();


    //Keyboard.end();

    //flush out URL so we can run again
    url = "";

    // Wait a bit before trying again
    //Serial.println("\n\nSend a character to scan another tag!");
    Serial.flush();
    /*
      while (!Serial.available());
      while (Serial.available()) {
      Serial.read();
      }
      Serial.flush();
    */
    //delay(3000);
    delay(10);

  }
}


