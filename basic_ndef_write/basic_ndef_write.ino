/**************************************************************************/
/*! 
    @file     ntag2xx_updatendef.pde
    @author   KTOWN (Adafruit Industries)
    @license  BSD (see license.txt)

    This example will wait for any NTAG203 or NTAG213 card or tag,
    and will attempt to add or update an NDEF URI at the start of the
    tag's memory.

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
#include <originalWire.h>
#include <Adafruit_PN532.h>

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield

// Or use this line for a breakout or shield with an I2C connection:
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

/*  
    We can encode many different kinds of pointers to the card,
    from a URL, to an Email address, to a phone number, and many more
    check the library header .h file to see the large # of supported
    prefixes! 
*/
// For a http://www. url:
//char * url = "christinedierk.com";
//uint8_t ndefprefix = NDEF_URIPREFIX_HTTP_WWWDOT;         //0x01

// for an email address
//char * url = "mail@example.com";
//uint8_t ndefprefix = NDEF_URIPREFIX_MAILTO;            //0x06

// for a phone number
//char * url = "+1 212 555 1212";
//uint8_t ndefprefix = NDEF_URIPREFIX_TEL;              //0x05

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
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();  
}

//writes a URL to the nail
boolean writeURL(uint8_t dataLength, uint8_t pageOffset, char * url, uint8_t ndefprefix){
  
  uint8_t pageBuffer[4] = { 0, 0, 0, 0 };
  uint8_t wrapperSize = 8;                   // Remove NDEF record overhead from the URI data (pageHeader below)
  uint8_t len = strlen(url);                  // Figure out how long the string is
  char * urlcopy = url;
  if ((len > 0) || (len+1 > (dataLength-wrapperSize))){        //only proceed if URI payload will fit in dataLen
    // Setup the record header
    // length of 8 instead of length of 12 (we have 7 bytes for a page header, but include the first character of the url so that we have full pages)
    uint8_t pageHeader[wrapperSize] =
    {
       /* NDEF Message TLV - URI Record */
      0x03,         /* Tag Field (0x03 = NDEF Message) */
      len+5,        /* Payload Length (not including 0xFE trailer) */
      0xD1,         /* NDEF Record Header (TNF=0x1:Well known record + SR + ME + MB) */
      0x01,         /* Type Length for the record type indicator */
      len+1,        /* Payload len */
      0x55,         /* Record Type Indicator (0x55 or 'U' = URI Record) */
      ndefprefix, /* URI Prefix (ex. 0x01 = "http://www.") */
      url[0]
    };

    //because the first character is included in the pageHeader
    len -= 1;
    urlcopy+=1;

    // Write 8 byte header (2 pages of data starting at page 4; includes first byte of url)
    memcpy (pageBuffer, pageHeader, 4);
    if (!(nfc.ntag2xx_WritePage (4 + pageOffset, pageBuffer)))
      return 0;
    memcpy (pageBuffer, pageHeader+4, 4);                                 //only 3 bytes to be copied
    if (!(nfc.ntag2xx_WritePage (5 + pageOffset, pageBuffer)))
      return 0;         //only successful if both of these went through
      
    // Write rest of URI (starting at page 6)
    uint8_t currentPage = 6 + pageOffset;
    while (len > 0) {                 //equivalent to len > 0
      if (len < 4) {              //not a full page left to write
        memset(pageBuffer, 0, 4);
        memcpy(pageBuffer, urlcopy, len);
        pageBuffer[len] = 0xFE; // NDEF record footer
        if (!(nfc.ntag2xx_WritePage (currentPage, pageBuffer)))
          return 0;
        //Done!
        return 1;
      }else if (len == 4){        //exactly one page left to write
        memcpy(pageBuffer, urlcopy, len);
        if (!(nfc.ntag2xx_WritePage (currentPage, pageBuffer)))
          return 0;
        memset(pageBuffer, 0, 4);
        pageBuffer[0] = 0xFE; // NDEF record footer
        currentPage++;
        if (!(nfc.ntag2xx_WritePage (currentPage, pageBuffer)))
          return 0;
      } else {                    //more than one full page left to write
          memcpy(pageBuffer, urlcopy, 4);
          if (!(nfc.ntag2xx_WritePage (currentPage, pageBuffer)))
            return 0;
          currentPage++;
          urlcopy+=4;
          len-=4;
      }
    }
    
  }

  //everything was successful!
  return true;
}

void loop(void) 
{
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t dataLength;

  // Require some user feedback before running this example!
  Serial.println("\r\nPlace your NDEF formatted NTAG2xx tag on the reader to update the");
  Serial.println("NDEF record and press any key to continue ...\r\n");
  // Wait for user input before proceeding
  while (!Serial.available());
  // a key was pressed1
  while (Serial.available()) Serial.read();

  // 1.) Wait for an NTAG203 card.  When one is found 'uid' will be populated with
  // the UID, and uidLength will indicate the size of the UID (normally 7)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  // It seems we found a valid ISO14443A Tag!
  if (success) 
  {
    // 2.) Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");
    
    if (uidLength != 7)
    {
      Serial.println("This doesn't seem to be an NTAG203 tag (UUID length != 7 bytes)!");
    }
    else
    {
      uint8_t data[32];
      
      // We probably have an NTAG2xx card (though it could be Ultralight as well)
      Serial.println("Seems to be an NTAG2xx tag (7 byte UID)");	  
      

      // NTAG 216       231     4             225      


      // 3.) Check if the NDEF Capability Container (CC) bits are already set
      // in OTP memory (page 3)
      memset(data, 0, 4);
      success = nfc.ntag2xx_ReadPage(3, data);
      if (!success)
      {
        Serial.println("Unable to read the Capability Container (page 3)");
        return;
      }
      else
      {
        // If the tag has already been formatted as NDEF, byte 0 should be:
        // Byte 0 = Magic Number (0xE1)
        // Byte 1 = NDEF Version (Should be 0x10)
        // Byte 2 = Data Area Size (value * 8 bytes)
        // Byte 3 = Read/Write Access (0x00 for full read and write)
        if (!((data[0] == 0xE1) && (data[1] == 0x10)))
        {
          Serial.println("This doesn't seem to be an NDEF formatted tag.");
          Serial.println("Page 3 should start with 0xE1 0x10.");
        }
        else
        {
          // 4.) Determine and display the data area size
          dataLength = data[2]*8;
          Serial.print("Tag is NDEF formatted. Data area size = ");
          Serial.print(dataLength);
          Serial.println(" bytes");

          // Read page 255 to determine pointers
          nfc.ntag2xx_ReadPage(225, data);
          nfc.PrintHexChar(data, 4);

          //THIS IS ONLY IF WE NEED TO MANUALLY UPDATE POINTERS
          //index of current data
          data[0] = 0x02;
          //number of total data
          data[1] = 0x05;
          //pointer to most recent data
          data[2] = 0x04;
          data[3] = 0xFE;
          nfc.ntag2xx_WritePage(225, data);

          // 5.) Erase the old data area
          Serial.print("Erasing previous data area ");
          for (uint8_t i = 4; i < (dataLength/4)+4; i++) 
          {
            memset(data, 0, 4);
            success = nfc.ntag2xx_WritePage(i, data);
            Serial.print(".");
            if (!success)
            {
              Serial.println(" ERROR!");
              return;
            }
          }
          Serial.println(" DONE!");
          
          // 6.) Try to add a new NDEF URI record
          Serial.print("Writing URI as NDEF Record ... ");

          //actually write the URLs
          success = (writeURL(dataLength, 0, "google.com", NDEF_URIPREFIX_HTTP_WWWDOT)) && (writeURL(dataLength, 10, "christinedierk.com", NDEF_URIPREFIX_HTTP_WWWDOT))
            && (writeURL(dataLength, 20, "tkbala.com", NDEF_URIPREFIX_HTTP_WWWDOT)) && (writeURL(dataLength, 30, "cdierk@berkeley.edu", NDEF_URIPREFIX_MAILTO)) 
            && (writeURL(dataLength, 40, "tkbala@berkeley.edu", NDEF_URIPREFIX_MAILTO));
          
          if (success)
          {
            Serial.println("DONE!");
          }
          else
          {
            Serial.println("ERROR! (URI length?)");
          }
                    
        } // CC contents NDEF record check
      } // CC page read check
    } // UUID length check
    
    // Wait a bit before trying again
    Serial.flush();
    while (!Serial.available());
    while (Serial.available()) {
    Serial.read();
    }
    Serial.flush();    
  } // Start waiting for a new ISO14443A tag
}
