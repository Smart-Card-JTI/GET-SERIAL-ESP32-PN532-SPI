#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <Time.h>
#include <TimeLib.h>

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  (18)
#define PN532_MISO (19)
#define PN532_MOSI (23)
#define PN532_SS   (22)

// Use this line for a breakout with a SPI connection:
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

void setup(void) {
  Serial.begin(115200);
  Serial.println("Hello!");
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }

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

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);

    if (uidLength == 4) {
      // We probably have a Mifare Classic card ...
      uint32_t cardid = uid[0];
      cardid <<= 8;
      cardid |= uid[1];
      cardid <<= 8;
      cardid |= uid[2];
      cardid <<= 8;
      cardid |= uid[3];
      Serial.print("Seems to be a Mifare Classic card #");
      Serial.println(cardid);
      Serial.println("");
      Serial.println("");

      // We probably have a Mifare Classic card ...
      Serial.println("Seems to be a Mifare Classic card (4 byte UID)");

      // Now we need to try to authenticate it for read/write access
      // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
      Serial.println("Trying to authenticate block 4 with default KEYA value");
      uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

      // Start with block 4 (the first block of sector 1) since sector 0
      // contains the manufacturer data and it's probably better just
      // to leave it alone unless you know what you're doing
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya);

      if (success)
      {
        Serial.println("Sector 1 (Blocks 4..7) has been authenticated");
        uint8_t temp[48];

        // If you want to write something to block 4 to test with, uncomment
        // the following line and this text should be read back in a minute
        // data = { 'a', 'd', 'a', 'f', 'r', 'u', 'i', 't', '.', 'c', 'o', 'm', 0, 0, 0, 0};
        // success = nfc.mifareclassic_WriteDataBlock (4, data);

        // Try to read the contents of block 4
        //        success = nfc.mifareclassic_ReadDataBlock(4, data);
        int index = 0;
        for (int i = 4; i < 7; i++) {
          uint8_t data[16];
          success = nfc.mifareclassic_ReadDataBlock(i, data);
          //          nfc.PrintHexChar(data, 16);

          for (int j = 0; j < 16; j++) {
            temp[index++] = data[j];
            //            index++;
          }
        }

        if (success)
        {
          // Data seems to have been read ... spit it out
          Serial.println("Reading Block 4:");
          nfc.PrintHexChar(temp, 48);
          Serial.println("");

          uint8_t nopol[10];
          uint8_t tanggal[4];
          uint8_t nip[18];
          uint8_t expired[4];
          uint8_t stMasuk;
          uint8_t kodeGate;
          uint8_t stKartu;

          //          copy nopol
          index = 0;
          for (int i = 0; i < 10; i++) {
            nopol[i] = temp[index++];
            //            index++;
          }
          //copy tanggal
          for (int i = 0; i < 4; i++) {
            tanggal[i] = temp[index++];
            //            index++;
          }
          stMasuk = temp[index++];
          kodeGate = temp[index++];
          //          copy nip
          for (int i = 0; i < 18; i++) {
            nip[i] = temp[index++];
          }
          //copy expired
          for (int i = 0; i < 4; i++) {
            expired[i] = temp[index++];
          }
          stKartu = temp[index];

          Serial.print("Nopol: ");
          nfc.PrintHexChar(nopol, 10);
          Serial.println();
          Serial.print("Tanggal: " );
          nfc.PrintHexChar(tanggal, 4);
          Serial.println();
          Serial.print("Nip: " );
          nfc.PrintHexChar(nip, 18);
          Serial.println();
          Serial.print("Expired: " );
          nfc.PrintHexChar(expired, 4);
          Serial.println();
          Serial.print("Status Masuk: " );
          Serial.println(stMasuk);
          Serial.print("Status Kartu: " );
          Serial.println(stKartu);
          Serial.print("Kode Gate: " );
          Serial.println(kodeGate);
          Serial.print("Tanggal long: " );
          char str[8] = "";
          array_to_string(tanggal, 4, str);
          Serial.println(str);
          uint64_t result = getUInt64fromHex(str);
          unsigned int add7Hour = 7 * 3600;
          result += add7Hour;

          char buff[32];
          sprintf(buff, "%02d.%02d.%02d %02d:%02d:%02d", day(result), month(result), year(result), hour(result), minute(result), second(result));

          Serial.println(buff);

          printf(" bignum = %lld\n", result);
          String tgl = uint64ToString(result);
          Serial.println(tgl);
          time_t ti = tgl.toInt();
          char timeStringBuff[50]; //50 chars should be enough
          strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", localtime(&ti));
          Serial.println(timeStringBuff);

          unsigned long offset_days = 7;    // 7 hours
          unsigned long t_unix_date1, t_unix_date2;
          t_unix_date1 = result;
          Serial.print("t_unix_date1: ");
          Serial.println(t_unix_date1);
          //          offset_days = offset_days * 86400;    // convert number of days to seconds
          offset_days = 7 * 3600;
          t_unix_date2 = result + offset_days;
          Serial.print("t_unix_date2: ");
          Serial.println(t_unix_date2);
          printf("Date1: %4d-%02d-%02d %02d:%02d:%02d\n", year(t_unix_date1), month(t_unix_date1), day(t_unix_date1), hour(t_unix_date1), minute(t_unix_date1), second(t_unix_date1));
          printf("Date2: %4d-%02d-%02d %02d:%02d:%02d\n", year(t_unix_date2), month(t_unix_date2), day(t_unix_date2), hour(t_unix_date2), minute(t_unix_date2), second(t_unix_date2));

          // Wait a bit before reading the card again
          delay(1000);
        }
        else
        {
          Serial.println("Ooops ... unable to read the requested block.  Try another key?");
        }
      }
      else
      {
        Serial.println("Ooops ... authentication failed: Try another key?");
      }
    }
    delay(2000);
  }
}

String uint64ToString(uint64_t input) {
  String result = "";
  uint8_t base = 10;

  do {
    char c = input % base;
    input /= base;

    if (c < 10)
      c += '0';
    else
      c += 'A' - 10;
    result = c + result;
  } while (input);
  return result;
}

void array_to_string(byte array[], unsigned int len, char buffer[])
{
  for (unsigned int i = 0; i < len; i++)
  {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i * 2 + 0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
    buffer[i * 2 + 1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
  }
  buffer[len * 2] = '\0';
}

uint64_t getUInt64fromHex(char const *str)
{
  uint64_t accumulator = 0;
  for (size_t i = 0 ; isxdigit((unsigned char)str[i]) ; ++i)
  {
    char c = str[i];
    accumulator *= 16;
    if (isdigit(c)) /* '0' .. '9'*/
      accumulator += c - '0';
    else if (isupper(c)) /* 'A' .. 'F'*/
      accumulator += c - 'A' + 10;
    else /* 'a' .. 'f'*/
      accumulator += c - 'a' + 10;

  }

  return accumulator;
}

void long2Hex[](unsigned long val, char buffer[15]) {
  ltoa(val, buffer, 16);
}
