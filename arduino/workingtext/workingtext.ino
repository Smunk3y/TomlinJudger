#define RESTORE_SPI_GPIO() SPCR = 0
#include <SPI.h>            
#include <SD.h>          
#include <TMRpcm.h>  
#include <Adafruit_GFX.h>  
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;
#define SD_CS     53      
#define PALETTEDEPTH   8    
char namebuf[32] = "/";
File root;
int pathlen;
TMRpcm tmrpcm;

//colors for text
#define BLACK           0x0000      /*   0,   0,   0 */
#define DARKGREEN       0x03E0      /*   0, 128,   0 */
#define BLUE            0x001F      /*   0,   0, 255 */
#define GREEN           0x07E0      /*   0, 255,   0 */
#define RED             0xF800      /* 255,   0,   0 */
#define MAGENTA         0xF81F      /* 255,   0, 255 */
#define YELLOW          0xFFE0      /* 255, 255,   0 */
#define WHITE           0xFFFF      /* 255, 255, 255 */
#define GREENYELLOW     0xAFE5      /* 173, 255,  47 */

String lastLocation = "";  // Variable to store the last known location



void setup() {
     uint16_t ID;
    Serial.begin(9600);
    ID = tft.readID();
    tft.begin(ID);
    tft.fillScreen(0x001F);
    tft.setTextColor(0xFFFF, 0x0000);
    bool good = SD.begin(SD_CS);
    if (!good) {
        Serial.print(F("cannot start SD"));
        while (1);
    }
    root = SD.open(namebuf);
    pathlen = strlen(namebuf);

    char bmpname[] = "jjstts.bmp";
    drawdabmp(bmpname);
}

void loop() {
    if (Serial.available()) {
        String data = Serial.readStringUntil('\n');
        int separatorIndex = data.indexOf(':');
        if (separatorIndex != -1) {
            String key = data.substring(0, separatorIndex);
            key.trim();
            String value = data.substring(separatorIndex + 1);
            value.trim();
            
            // Update display for all data
            displayData(key, value);

            // Check and update the image only if the location has changed
            if (key == "Location" && value != lastLocation) {
                lastLocation = value;  // Update last known location
                updateLocationImage(value);
            }
        }
    }
}

void displayData(String key, String value) {
    int yPos = getYPositionForKey(key);
    tft.fillRect(0, yPos, tft.width(), 20, BLACK);  // Clear the area
    tft.setCursor(0, yPos);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print(key + ": " + value);
}

void updateLocationImage(String location) {
    // Draw the image for the new location
    // Ensure this does not overlap with the text area
    if (location == "Pittsburgh") {
        drawdabmp("pit.bmp");
    } else if (location == "Buffalo") {
        drawdabmp("buf.bmp");
    } else if (location == "Kansas City") {
        drawdabmp("kc.bmp");
    }
}

int getYPositionForKey(String key) {
    if (key == "Location") return 345;
    if (key == "Win Percentage") return 370;
    if (key == "Points For") return 395;
    if (key == "Points Against") return 420;

    return 320; // Default position for other keys
}


void drawdabmp(char bmploc[]){
    uint8_t ret;
    uint32_t start;
    start = millis();
    tft.fillScreen(0);
    ret = showBMP(bmploc, 0, 0);
}




#define BMPIMAGEOFFSET 54
#define BUFFPIXEL      20

uint16_t read16(File& f) {
    uint16_t result;         // read little-endian
    f.read((uint8_t*)&result, sizeof(result));
    return result;
}




uint32_t read32(File& f) {
    uint32_t result;
    f.read((uint8_t*)&result, sizeof(result));
    return result;
}




uint8_t showBMP(char *nm, int x, int y)
{
    File bmpFile;
    int bmpWidth, bmpHeight;    // W+H in pixels
    uint8_t bmpDepth;           // Bit depth (currently must be 24, 16, 8, 4, 1)
    uint32_t bmpImageoffset;    // Start of image data in file
    uint32_t rowSize;           // Not always = bmpWidth; may have padding
    uint8_t sdbuffer[3 * BUFFPIXEL];    // pixel in buffer (R+G+B per pixel)
    uint16_t lcdbuffer[(1 << PALETTEDEPTH) + BUFFPIXEL], *palette = NULL;
    uint8_t bitmask, bitshift;
    boolean flip = true;        // BMP is stored bottom-to-top
    int w, h, row, col, lcdbufsiz = (1 << PALETTEDEPTH) + BUFFPIXEL, buffidx;
    uint32_t pos;               // seek position
    boolean is565 = false;      //




    uint16_t bmpID;
    uint16_t n;                 // blocks read
    uint8_t ret;




    if ((x >= tft.width()) || (y >= tft.height()))
        return 1;               // off screen




    bmpFile = SD.open(nm);      // Parse BMP header
    bmpID = read16(bmpFile);    // BMP signature
    (void) read32(bmpFile);     // Read & ignore file size
    (void) read32(bmpFile);     // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile);       // Start of image data
    (void) read32(bmpFile);     // Read & ignore DIB header size
    bmpWidth = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    n = read16(bmpFile);        // # planes -- must be '1'
    bmpDepth = read16(bmpFile); // bits per pixel
    pos = read32(bmpFile);      // format
    if (bmpID != 0x4D42) ret = 2; // bad ID
    else if (n != 1) ret = 3;   // too many planes
    else if (pos != 0 && pos != 3) ret = 4; // format: 0 = uncompressed, 3 = 565
    else if (bmpDepth < 16 && bmpDepth > PALETTEDEPTH) ret = 5; // palette
    else {
        bool first = true;
        is565 = (pos == 3);               // ?already in 16-bit format
        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * bmpDepth / 8 + 3) & ~3;
        if (bmpHeight < 0) {              // If negative, image is in top-down order.
            bmpHeight = -bmpHeight;
            flip = false;
        }




        w = bmpWidth;
        h = bmpHeight;
        RESTORE_SPI_GPIO();
        if ((x + w) >= tft.width())       // Crop area to be loaded
            w = tft.width() - x;
        if ((y + h) >= tft.height())      //
            h = tft.height() - y;




        if (bmpDepth <= PALETTEDEPTH) {   // these modes have separate palette
            //bmpFile.seek(BMPIMAGEOFFSET); //palette is always @ 54
            bmpFile.seek(bmpImageoffset - (4<<bmpDepth)); //54 for regular, diff for colorsimportant
            bitmask = 0xFF;
            if (bmpDepth < 8)
                bitmask >>= bmpDepth;
            bitshift = 8 - bmpDepth;
            n = 1 << bmpDepth;
            lcdbufsiz -= n;
            palette = lcdbuffer + lcdbufsiz;
            for (col = 0; col < n; col++) {
                pos = read32(bmpFile);    //map palette to 5-6-5
                palette[col] = ((pos & 0x0000F8) >> 3) | ((pos & 0x00FC00) >> 5) | ((pos & 0xF80000) >> 8);
            }
        }




        RESTORE_SPI_GPIO();
        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x + w - 1, y + h - 1);
        for (row = 0; row < h; row++) { // For each scanline...
            // Seek to start of scan line.  It might seem labor-
            // intensive to be doing this on every line, but this
            // method covers a lot of gritty details like cropping
            // and scanline padding.  Also, the seek only takes
            // place if the file position actually needs to change
            // (avoids a lot of cluster math in SD library).
            uint8_t r, g, b, *sdptr;
            int lcdidx, lcdleft;
            if (flip)   // Bitmap is stored bottom-to-top order (normal BMP)
                pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
            else        // Bitmap is stored top-to-bottom
                pos = bmpImageoffset + row * rowSize;
            if (bmpFile.position() != pos) { // Need seek?
                bmpFile.seek(pos);
                buffidx = sizeof(sdbuffer); // Force buffer reload
            }




            for (col = 0; col < w; ) {  //pixels in row
                lcdleft = w - col;
                if (lcdleft > lcdbufsiz) lcdleft = lcdbufsiz;
                for (lcdidx = 0; lcdidx < lcdleft; lcdidx++) { // buffer at a time
                    uint16_t color;
                    // Time to read more pixel data?
                    if (buffidx >= sizeof(sdbuffer)) { // Indeed
                        bmpFile.read(sdbuffer, sizeof(sdbuffer));
                        buffidx = 0; // Set index to beginning
                        r = 0;
                    }
                    switch (bmpDepth) {          // Convert pixel from BMP to TFT format
                        case 32:
                        case 24:
                            b = sdbuffer[buffidx++];
                            g = sdbuffer[buffidx++];
                            r = sdbuffer[buffidx++];
                            if (bmpDepth == 32) buffidx++; //ignore ALPHA
                            color = tft.color565(r, g, b);
                            break;
                        case 16:
                            b = sdbuffer[buffidx++];
                            r = sdbuffer[buffidx++];
                            if (is565)
                                color = (r << 8) | (b);
                            else
                                color = (r << 9) | ((b & 0xE0) << 1) | (b & 0x1F);
                            break;
                        case 1:
                        case 4:
                        case 8:
                            if (r == 0)
                                b = sdbuffer[buffidx++], r = 8;
                            color = palette[(b >> bitshift) & bitmask];
                            r -= bmpDepth;
                            b <<= bmpDepth;
                            break;
                    }
                    lcdbuffer[lcdidx] = color;




                }
                RESTORE_SPI_GPIO();
                tft.pushColors(lcdbuffer, lcdidx, first);
                first = false;
                col += lcdidx;
            }           // end cols
        }               // end rows
        tft.setAddrWindow(0, 0, tft.width() - 1, tft.height() - 1); //restore full screen
        ret = 0;        // good render
    }
    bmpFile.close();
                RESTORE_SPI_GPIO();
    return (ret);
}


