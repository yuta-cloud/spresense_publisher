#include <SDHCI.h>
#include <NetPBM.h>
#include <DNNRT.h>
#include <SDHCI.h>
#include <stdio.h>  /* for sprintf */
#include <Camera.h>
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
/* CS はハードウェア制御のため設定しなくてもよい*/
#define TFT_RST 8
#define TFT_DC 9
#define TFT_CS 10
Adafruit_ILI9341 tft= Adafruit_ILI9341(&SPI,TFT_DC, TFT_CS, TFT_RST);

#define BAUDRATE                (115200)
#define TOTAL_PICTURE_COUNT     (10)

int take_picture_count = 0;

DNNRT dnnrt;
DNNVariable input(24*24);
SDClass SD;

void CamCB(CamImage img){
  if (img.isAvailable()) {
  //img.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);

  CamImage small;
  CamErr camerrr = img.clipAndResizeImageByHW(small, 64, 24, 255, 215, 24, 24);
  Serial.print(camerrr);
  int err = small.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);
  img.convertPixFormat(CAM_IMAGE_PIX_FMT_RGB565);
  uint16_t* buff_for_lcd = (uint16_t*)img.getImgBuff();

  for (int i = 24; i < 216; i++) {
    buff_for_lcd[i * 320 + 63] = 0xf800;
    buff_for_lcd[i * 320 + 64] = 0xf800;
    buff_for_lcd[i * 320 + 255] = 0xf800;
    buff_for_lcd[i * 320 + 256] = 0xf800;
  }
  
  for(int j = 64; j < 256; j++) {
    buff_for_lcd[23 * 320 + j] = 0xf800;
    buff_for_lcd[24 * 320 + j] = 0xf800;
    buff_for_lcd[215 * 320 + j] = 0xf800;
    buff_for_lcd[216 * 320 + j] = 0xf800;
  }
  tft.drawRGBBitmap(0 ,0 ,buff_for_lcd, 320, 240);
  
  //Serial.println(err);
  /*
  char filename[16] = {0};
  sprintf(filename, "PICT%03d.JPG", take_picture_count);
  
  SD.remove(filename);
  File myFile = SD.open(filename, FILE_WRITE);
  myFile.write(img.getImgBuff(), img.getImgSize());
  myFile.close();
  */

  uint16_t* tmp = (uint16_t*)small.getImgBuff();

  float *dnnbuf = input.data();
  float f_max = 0.0;
  for (int n = 0; n < 24*24; ++n) {
    dnnbuf[n] = (float)((tmp[n] & 0x07E0) >> 5);
    if (dnnbuf[n] > f_max) f_max = dnnbuf[n];
    
  }
          
   /* normalization */
  for (int n = 0; n < 24*24; ++n) {
    dnnbuf[n] /= f_max;
  }
  String gStrResult = "?";
  dnnrt.inputVariable(input, 0);
  dnnrt.forward();
  DNNVariable output = dnnrt.outputVariable(0);
  int index = output.maxIndex();
  //indexが0だったら笑顔
  Serial.printf("index: %d value: %f\n", index, output[index]);
  }
  take_picture_count++;
}

void setup() {

  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // モデルの読み込み
  File nnbfile = SD.open("face_express.nnb");
  if (!nnbfile) {
    Serial.print("nnb not found");
    return;
  }
  int ret = dnnrt.begin(nnbfile);
  if (ret < 0) {
    Serial.println("Runtime initialization failure.");
    if (ret == -16) {
      Serial.print("Please install bootloader!");
      Serial.println(" or consider memory configuration!");
    } else {
      Serial.println(ret);
    }
    return;
  }
  Serial.println("Set still picture format");
  theCamera.setStillPictureImageFormat(
     320,
     240,
     CAM_IMAGE_PIX_FMT_JPG);

    
    tft.begin();
    tft.setRotation(1);
    theCamera.begin();
    theCamera.startStreaming(true, CamCB);
}


void loop() {
}
