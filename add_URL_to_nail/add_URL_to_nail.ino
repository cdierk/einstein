/*
 * Christine Dierk
 * 
 * November 20, 2017
 * 
 * Code that reads last block of nail, 
 * determines where to write url (overwrites oldest data block, if necessary), 
 * writes data over i2c, 
 * and updates last block to reflect new number of URLs/data chunks on nail
 *
 */

#include <originalWire.h>
#include <Adafruit_PN532.h>

#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

void setup() {
  // put your setup code here, to run once:
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
  uint8_t wrapperSize = 8;                    // Remove NDEF record overhead from the URI data (pageHeader below)
  uint8_t len = strlen(url);                  // Figure out how long the string is
  char * urlcopy = url;
  if ((len > 0) || (len+1 > (dataLength-wrapperSize))){        //only proceed if URI payload will fit in dataLen
    // Setup the record header
    // length of 8 (we have 7 bytes for a page header, but include the first character of the url so that we have full pages)
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

void loop() {
  // put your main code here, to run repeatedly:

  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t dataLength;
  uint8_t pointer;
  uint8_t numData;
  uint8_t pageOffset;
  uint8_t mostRecent;

  // 1.) Wait for an NTAG203 card.  When one is found 'uid' will be populated with
  // the UID, and uidLength will indicate the size of the UID (normally 7)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

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

          pointer = data[0];
          numData = data[1];
          mostRecent = data[2];

          mostRecent++;

          if (mostRecent == 20){
            mostRecent = 0;
          }

          pageOffset = mostRecent * 10; //10 pages per URL/data

          // 5.) Erase the old data area
          Serial.print("Erasing previous data area ");
          for (uint8_t i = 4; i < 14; i++) 
          {
            memset(data, 0, 4);
            success = nfc.ntag2xx_WritePage(i + pageOffset, data);
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

          // THIS IS WHERE YOU SET THE URL/email/phone #
          char * url = "ebay.com";
          uint8_t ndefprefix = NDEF_URIPREFIX_HTTP_WWWDOT;         //0x01

          //actually write the URLs
          success = writeURL(dataLength, pageOffset, url, ndefprefix);

          //update page 225 (pointer and number of entries)
          if (numData < 20){                                      //maximum of 20 entries
            numData++;
          }

          pointer = mostRecent;

          data[0] = pointer;
          data[1] = numData;
          data[2] = mostRecent;
          data[3] = 0xFE;

          nfc.ntag2xx_WritePage(225, data);

          data[0] = 0x00;
          data[1] = 0x00;
          data[2] = 0x00;
          data[3] = 0x00;

          nfc.ntag2xx_WritePage(220, data);
          
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
