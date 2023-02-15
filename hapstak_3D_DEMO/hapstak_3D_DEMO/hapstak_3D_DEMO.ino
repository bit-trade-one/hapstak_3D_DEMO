/*
  Pipe Demo for PUI  
  version 0.1 (20220725)
  
  hapStak + ATOM Matrix + Encoder Unit

  Copyright (C) 2022 Foster Electric Co., Ltd.


  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <M5Atom.h>
#include <math.h>

#include "FS.h"
#include "SPIFFS.h"

#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorWAVRepeatable.h"
#include "AudioOutputI2S.h"

#include "Unit_Encoder.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////

#define FLG_WRITE_FILES 0 // 信号ファイルの書き込みを行うかどうか。信号ファイルを初めて書き込むときや変更したときは1、それ以外は0にする。

#define FILE_NAME "/file%02d.wav"  // 振動信号ファイルのファイル名。"%02d"に2桁でNo.が入る。先頭に"/"が必要。
#define BOOT_NAME "/boot.wav"  // 起動音ファイルのファイル名。先頭に"/"が必要。
#define TICK_NAME "/tick.wav"  // 波形切り替え振動ファイルのファイル名。先頭に"/"が必要。

const float DEFAULT_GAIN = 1.0; // 振動信号のデフォルトゲイン。0.0～1.0
const float NINE_GAIN = 0.9; // GAIN9
const float EIGHT_GAIN = 0.8; // GAIN8
const float SEVEN_GAIN = 0.7; // GAIN7
const float SIX_GAIN = 0.6; // GAIN6
const float FIVE_GAIN = 0.5; // GAIN5
const float FOUR_GAIN = 0.4; // GAIN4
const float THREE_GAIN = 0.3; // GAIN3
const float TWO_GAIN = 0.2; // GAIN2
const float ONE_GAIN = 0.1; // GAIN1
const float BEEP_GAIN = 0.5; // Beep音、起動音のゲイン。0.0～1.0
const float TICK_GAIN = 0.2;  // 波形切り替え振動のゲイン。0.0～1.0

const float ACCEL_THRESHOLD = 3.0;  // Swingトリガー時の閾値加速度[G]

const int INITIAL_DELAY = 0;  // 動作開始前のdelay値
const int LOOP_DELAY = 5; // メインループのdelay値

const int FADEIN_STEP = int(0 / LOOP_DELAY); // フェードインのステップ数
const int FADEOUT_STEP = int(0 / LOOP_DELAY); // フェードアウトのステップ数

const long LONG_COUNT = int(200 / LOOP_DELAY);  // 長押しの反応時間
const long LONG_LONG_COUNT = int(600 / LOOP_DELAY); // 超長押しの反応時間
const long MULTI_COUNT = int(50 / LOOP_DELAY);  // ダブル・トリプルクリックの反応時間

//////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
  int dispNumber;
  int fileNumber;
  int triggerMode;
  float gain;
  uint8_t dispColor[3];
} TrackData;

const int TRIG_LOOP = 0;
const int TRIG_BUTTON = 1;
const int TRIG_SWING = 2;

#define COLOR_BLACK { 0x00, 0x00, 0x00 }
#define COLOR_WHITE { 0xFF, 0xFF, 0xFF }
#define COLOR_RED { 0xFF, 0x00, 0x00 }
#define COLOR_GREEN { 0x00, 0xFF, 0x00 }
#define COLOR_BLUE { 0x00, 0x00, 0xFF }
// #define COLOR_RED { 0xFF, 0x80, 0x00 }
// #define COLOR_GREEN { 0x00, 0xFF, 0x80 }
// #define COLOR_BLUE { 0x80, 0x00, 0xFF }

const int BUTTON_FIRST = 0;
const int ACCEL_FIRST = 3;
const int LOOP_FIRST = 8;


const int TRACK_COUNT = 17; // 振動再生モード（トラック）の数

// トラックデータ
// LED表示の番号(0～F), 振動波形ファイルの番号, 駆動モードの種類, 再生ゲイン, LED表示色
TrackData trackData[TRACK_COUNT] = {
   // Loop

  { 0x1, 1, TRIG_LOOP, FIVE_GAIN, COLOR_RED }, // LOOP_FIRST
  { 0x2, 2, TRIG_LOOP, SEVEN_GAIN, COLOR_RED }, 
  { 0x3, 3, TRIG_LOOP, FIVE_GAIN, COLOR_RED },
  { 0x4, 4, TRIG_LOOP, DEFAULT_GAIN, COLOR_RED },
  { 0x5, 5, TRIG_LOOP, DEFAULT_GAIN, COLOR_RED },
  { 0x6, 6, TRIG_LOOP, FIVE_GAIN, COLOR_RED },
  { 0x7, 7, TRIG_LOOP, FIVE_GAIN, COLOR_RED },
  { 0x8, 8, TRIG_LOOP, FIVE_GAIN, COLOR_RED }, 
  { 0x9, 9, TRIG_LOOP, DEFAULT_GAIN, COLOR_RED },
  { 0xA, 10, TRIG_LOOP, DEFAULT_GAIN, COLOR_RED },
  { 0xB, 12, TRIG_LOOP, FIVE_GAIN, COLOR_RED },
  { 0xC, 13, TRIG_LOOP, FIVE_GAIN, COLOR_RED },
  { 0xD, 14, TRIG_LOOP, SEVEN_GAIN, COLOR_RED },
  { 0xE, 11, TRIG_LOOP, DEFAULT_GAIN, COLOR_RED }, 
  { 0xF, 15, TRIG_LOOP, FIVE_GAIN, COLOR_RED },
  { 0x0, 17, TRIG_LOOP, SEVEN_GAIN, COLOR_RED },
  { 0x10, 18, TRIG_LOOP, DEFAULT_GAIN, COLOR_RED },


  // SWING


  
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////

#define CONFIG_I2S_BCK_PIN      19
#define CONFIG_I2S_LRCK_PIN     33
#define CONFIG_I2S_DATA_PIN     22

AudioGeneratorWAVRepeatable *wav;
AudioFileSourceSPIFFS *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;

int currentIndex = 0;
TrackData currentData;

double curGain = 0.0; // フェードイン・アウト時のゲイン
double fadeDelta = 0.0; // フェードイン・アウト時の変化量

bool isIMUInit = false;
bool isError = false;

Unit_Encoder encUnit;

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
  M5.begin(true, false, true);
  delay(50);

  delay(INITIAL_DELAY);
  Serial.begin(115200);

  Serial.printf("SPIFF init...\n");
  SPIFFS.begin();

  #if FLG_WRITE_FILES
  WriteFiles();
  #endif

  Serial.printf("WAV playback init...\n");

  audioLogger = &Serial;
  
  out = new AudioOutputI2S();
  out->SetPinout(CONFIG_I2S_BCK_PIN, CONFIG_I2S_LRCK_PIN, CONFIG_I2S_DATA_PIN);
  out->SetChannels(1);
  out->SetGain(DEFAULT_GAIN);
  if (!out) {
    isError = true;
    Serial.printf("I2S init FAILED!\n");
  }

  wav = new AudioGeneratorWAVRepeatable();
  if (!wav) {
    isError = true;
    Serial.printf("generator init FAILED!\n");
  }

  Serial.printf("IMU init...\n");

  if (M5.IMU.Init() != 0) {
    isIMUInit = false;
    Serial.printf("FAILED!\n");
  } else {
    isError = false;
    isIMUInit = true;
  }
  
  

  LoadFile(BOOT_NAME, BEEP_GAIN, false);
  Play();
  for (int i = 0; ; i ++) {
    if (!(wav->loop())) { break; }
    if (i % 10 == 0) {
      rainbow();
    }
    delay(5);
  }
  Stop();

  encUnit.begin();
  signed short int encoder_value = encUnit.getEncoderValue();
  int current_num = ((encoder_value / 2) % TRACK_COUNT + TRACK_COUNT) % TRACK_COUNT;

  SelectTrack(current_num);
}

void loop()
{
  AudioLoop();
  ButtonLoop();
  EncoderLoop();
  AccelLoop();
  DisplayLoop();
  
  delay(LOOP_DELAY);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void ButtonLoop()
{
  static unsigned long pressCnt = 0;
  static unsigned long releaseCnt = ULONG_MAX;
  static int multiCnt = 0;

  M5.update();
  
  if (M5.Btn.isPressed()) {
    if (pressCnt < ULONG_MAX) {
      pressCnt ++;
    }

    if (pressCnt == 1) {
      SingleDown();  // 押したとき
    } else if (pressCnt == LONG_COUNT) {
      LongPress();
    } else if (pressCnt == LONG_LONG_COUNT) {
      LongLongPress();
    }

    releaseCnt = 0;
  } else {
    if (releaseCnt < ULONG_MAX) {
      releaseCnt ++;
    }

    if ((pressCnt > 0) && (pressCnt < LONG_COUNT)) {
      multiCnt ++;
    }
    
    if (releaseCnt == MULTI_COUNT) {
      if (multiCnt == 1) {
        SinglePress();  // 離したとき
      } else if (multiCnt == 2) {
        DoublePress();
      } else if (multiCnt >= 3) {
        TriplePress();
      }
      multiCnt = 0;
    }
    
    pressCnt = 0;
  }
}

void SingleDown() {
  if (currentData.triggerMode == TRIG_BUTTON) {
    LoadTrack();
    Play();
  }
}

void SinglePress() {
  if (currentData.triggerMode == TRIG_LOOP) {
    if (!(wav->isRunning())) {
      LoadTrack();
      FadePlay();
      //Play();
    } else {
      FadeStop();
      //Stop();
    }
  }
}

void DoublePress() {

}

void TriplePress() {
  SelectTrack(0);
}

void LongPress() {
  NextTrack();
}

void LongLongPress() {
  if (currentIndex < ACCEL_FIRST) {
    SelectTrack(ACCEL_FIRST);
  } else if (currentIndex < LOOP_FIRST) {
    SelectTrack(LOOP_FIRST);
  } else {
    SelectTrack(BUTTON_FIRST);
  }  
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void EncoderLoop()
{
  static int last_num = currentIndex;
  static bool last_btn = false;

  signed short int encoder_value = encUnit.getEncoderValue();
  bool btn_status = encUnit.getButtonStatus();

  int current_num = (encoder_value / 2) % TRACK_COUNT;

  if (last_num != current_num) {
      TrackData t = trackData[current_num];
      encUnit.setLEDColor(2, t.dispColor[0]<<16 | t.dispColor[1]<<8 | t.dispColor[2]);
      PlayTick();
      SelectTrack(current_num);
      encUnit.setLEDColor(2, 0x000000);
      last_num = current_num;
  }

  if (last_btn != btn_status) {
      if (!btn_status) {
          encUnit.setLEDColor(1, 0xFFFFFF);
          SingleDown();
          encUnit.setLEDColor(1, 0x000000);
      }
      last_btn = btn_status;
  }
}

void PlayTick()
{
  LoadFile(TICK_NAME, TICK_GAIN, false);
  Play();
  while (wav->loop()) { /*noop*/ }
  Stop();
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////

void DisplayLoop()
{
  static int num = -1;

  if (currentData.dispNumber != num) {
    DisplayNumber(currentData.dispNumber, currentData.dispColor);
    num = currentData.dispNumber;
  }
  
  DisplayState();
}

const uint8_t digitData[18][25] = {
  { 0,1,1,0,0, 1,0,0,1,0, 1,0,0,0,0, 1,0,0,1,1, 0,1,1,0,0, },  // "G"
  { 0,0,1,0,0, 0,1,1,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,1,1,1,0, },  // "1"
  { 0,1,1,0,0, 1,0,0,1,0, 0,0,1,0,0, 0,1,0,0,0, 1,1,1,1,0, },  // "2"
  { 1,1,1,0,0, 0,0,0,1,0, 0,1,1,0,0, 0,0,0,1,0, 1,1,1,0,0, },  // "3"
  { 0,0,1,0,0, 0,1,1,0,0, 1,0,1,0,0, 1,1,1,1,0, 0,0,1,0,0, },  // "4"
  { 1,1,1,1,0, 1,0,0,0,0, 1,1,1,0,0, 0,0,0,1,0, 1,1,1,0,0, },  // "5"
  { 0,1,1,0,0, 1,0,0,0,0, 1,1,1,0,0, 1,0,0,1,0, 0,1,1,0,0, },  // "6"
  { 1,1,1,1,0, 0,0,0,1,0, 0,0,1,0,0, 0,1,0,0,0, 0,1,0,0,0, },  // "7"
  { 0,1,1,0,0, 1,0,0,1,0, 0,1,1,0,0, 1,0,0,1,0, 0,1,1,0,0, },  // "8"
  { 0,1,1,0,0, 1,0,0,1,0, 0,1,1,1,0, 0,0,0,1,0, 0,1,1,0,0, },  // "9"
  { 0,1,1,0,0, 1,0,0,1,0, 1,1,1,1,0, 1,0,0,1,0, 1,0,0,1,0, },  // "A"
  { 1,1,1,0,0, 1,0,0,1,0, 1,1,1,0,0, 1,0,0,1,0, 1,1,1,0,0, },  // "B"
  { 0,1,1,0,0, 1,0,0,1,0, 1,0,0,0,0, 1,0,0,1,0, 0,1,1,0,0, },  // "C"
  { 1,1,1,0,0, 1,0,0,1,0, 1,0,0,1,0, 1,0,0,1,0, 1,1,1,0,0, },  // "D"
  { 1,1,1,1,0, 1,0,0,0,0, 1,1,1,0,0, 1,0,0,0,0, 1,1,1,1,0, },  // "E"
  { 1,1,1,1,0, 1,0,0,0,0, 1,1,1,0,0, 1,0,0,0,0, 1,0,0,0,0, },  // "F"
  { 1,0,0,1,0, 1,0,0,1,0, 1,1,1,1,0, 1,0,0,1,0, 1,0,0,1,0, },  // "H"
  { 0,1,1,1,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,1,1,1,0, },  // "I"
};


void DisplayNumber(int num, uint8_t* color) {  
  uint8_t buf[2+25*3];

  buf[0] = 5;
  buf[1] = 5;

  for (int i = 0; i < 25; i ++) {
    for (int j = 0; j < 3; j ++) {
      buf[2+i*3+j] = (digitData[num][i] == 0) ? 0x00 : color[j];
    }
  }

  M5.dis.displaybuff(buf, 0, 0);
}

void DisplayState() {
  M5.dis.drawpix(4, isError ? 0x00FF00 : 0x000000);
  
  if (!wav) { return; }
  M5.dis.drawpix(24, wav->isRunning() ? 0xFFFFFF : 0x000000);
}

void FillDisplay(uint32_t grb) {
  uint8_t buf[2+25*3];

  buf[0] = 5;
  buf[1] = 5;

  for (int i = 0; i < 25; i ++) {
    buf[2+i*3+0] = (grb & 0xFF00) >> 8;
    buf[2+i*3+1] = (grb & 0xFF0000) >> 16;
    buf[2+i*3+2] = grb & 0xFF;
  }

  M5.dis.displaybuff(buf, 0, 0);
}

void rainbow()
{
  static int ptr = 0;
  static CRGB colors[] = { 0xff0000, 0xff8000, 0xffff00, 0x80ff00, 0x00ff00, 0x00ff80, 0x00ffff, 0x0080ff, 0x0000ff, 0x8000ff, 0xff00ff, 0xff0080, };

  int i, j, t, x, y, p;
  p = ptr;
  for (i = 0; i < 9; i++) {
    t = 4 - abs(4 - i);
    for (j = 0; j <= t; j++) {
      if (i <= 4) {
        x = j;
        y = t - j;
      }
      else {
        x = (4 - t) + j;
        y = 4 - j;
      }          
      M5.dis.drawpix(x, y, colors[p]);
    }
    p++;
    p = (p == 12) ? 0 : p;
  }
  //delay(100);
  ptr++;
  ptr = (ptr == 12) ? 0 : ptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void AccelLoop()
{
  float accX = 0, accY = 0, accZ = 0;
  float gyroX = 0, gyroY = 0, gyroZ = 0;
  float temp = 0;

  if (isIMUInit == false) {
    return;
  }
    
  M5.IMU.getAccelData(&accX, &accY, &accZ);
  //M5.IMU.getGyroData(&gyroX, &gyroY, &gyroZ);
  //M5.IMU.getTempData(&temp);
  
  //Serial.printf("%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\r\n", accX, accY, accZ, gyroX, gyroY, gyroZ, temp);
  //Serial.printf("%.2f,%.2f,%.2f\r\n", accX, accY, accZ);
  
  float accTotal = sqrt(accZ * accZ);

  if ((currentData.triggerMode == TRIG_SWING) && (accTotal > ACCEL_THRESHOLD)) {
    LoadTrack();
    Play();
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void AudioLoop()
{
  if (!wav) { return; }
  if (!(wav->isRunning())) { return; }

  if (!(wav->loop())) { // wav->loop()はリピートしない場合のみfalseを返す
    wav->stop();
  }

  if (!(wav->Repeat)) { return; }

  if (fadeDelta < 0) {  // フェードアウト処理
    curGain += fadeDelta;
    if (curGain > 0) {
      out->SetGain(curGain);
    } else {
      fadeDelta = 0.0;
      wav->stop();
    }
  } else if (fadeDelta > 0) { // フェードイン処理
    curGain += fadeDelta;
    if (curGain < currentData.gain) {
      out->SetGain(curGain);
    } else {
      out->SetGain(currentData.gain);
      fadeDelta = 0.0;
    }
  }
}

void LoadTrack()
{
  char fileName[32];
  sprintf(fileName, FILE_NAME, currentData.fileNumber);
    
  LoadFile(fileName, currentData.gain, currentData.triggerMode == TRIG_LOOP);
}

void SelectTrack(int index)
{
  if (!wav) { return; }
  if (wav->isRunning()) {
    Stop();
  }
  
  currentIndex = index;
  currentData = trackData[currentIndex];

  Serial.printf("SelectTrack:%d\r\n", currentData.dispNumber);

  if (currentData.triggerMode == TRIG_LOOP) {
    LoadTrack();
    FadePlay(); 
  }
}

void NextTrack()
{  
  SelectTrack((currentIndex + 1) % TRACK_COUNT);
}

void PrevTrack()
{
  SelectTrack((currentIndex + TRACK_COUNT - 1) % TRACK_COUNT);
}

void LoadFile(char* fileName, float gain, bool repeat)
{
  if (!wav) { return; }
  if (wav->isRunning()) {
    Stop();
  }
  
  Serial.printf("Load:%s\r\n", fileName);
  
  file = new AudioFileSourceSPIFFS(fileName);
  if (!file) {
    Serial.printf("file ERROR!\r\n");
  }
  
  id3 = new AudioFileSourceID3(file);
  if (!id3) {
    Serial.printf("id3 ERROR!\r\n");
  }

  out->SetGain(gain);
  wav->Repeat = repeat;
}

void Play()
{
  if (wav->isRunning()) {
    return;
  }

  fadeDelta = 0.0;
  
  Serial.print("Play\r\n");
  bool result = wav->begin(id3, out);
  if (!result) {
    isError = true;
    Serial.print("wav begin ERROR!\r\n");
  }
}

void Stop()
{
  Serial.print("Stop\r\n");
  wav->stop();

  fadeDelta = 0.0;
}

void FadePlay()
{
  Serial.print("FadePlay\r\n");
  curGain = 0.0;
  out->SetGain(curGain);
  bool result = wav->begin(id3, out);
  if (!result) {
    isError = true;
    Serial.print("wav begin ERROR!\r\n");
  }
  fadeDelta = currentData.gain / FADEIN_STEP;
}

void FadeStop()
{
  Serial.print("FadeStop\r\n");
  curGain = currentData.gain;
  fadeDelta = -currentData.gain / FADEOUT_STEP;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

#if FLG_WRITE_FILES

#include "./wav/file01.h"
#include "./wav/file02.h"
#include "./wav/file03.h"
#include "./wav/file04.h"
#include "./wav/file05.h"
#include "./wav/file06.h"
#include "./wav/file07.h"
#include "./wav/file08.h"
#include "./wav/file09.h"
#include "./wav/file10.h"
#include "./wav/file11.h"
#include "./wav/file12.h"
#include "./wav/file13.h"
#include "./wav/file14.h"
#include "./wav/file15.h"
#include "./wav/file16.h"
#include "./wav/file17.h"
#include "./wav/file18.h"
#include "./wav/boot.h"
#include "./wav/tick.h"

const int FILE_COUNT = 18;  // 振動ファイルの数（起動音は含めない）。
const unsigned char* wavePtr[FILE_COUNT] = { file01, file02, file03, file04, file05, file06, file07, file08, file09, file10, file11, file12, file13, file14, file15, file16, file17, file18, };
int waveSize[FILE_COUNT] = { sizeof(file01), sizeof(file02), sizeof(file03), sizeof(file04), sizeof(file05), sizeof(file06), sizeof(file07), sizeof(file08), sizeof(file09), sizeof(file10), sizeof(file11), sizeof(file12), sizeof(file13), sizeof(file14), sizeof(file15), sizeof(file16), sizeof(file17),sizeof(file18),};

bool WriteFile(char* fileName, const unsigned char* wavePtr, int waveSize)
{
    Serial.println(fileName);
    Serial.printf("data size = %d\n", waveSize);

    Serial.println("file open");
    File fp = SPIFFS.open(fileName, FILE_WRITE);
  
    Serial.println("file write start");
    fp.write(wavePtr, waveSize);
    fp.close();
    Serial.println("file write end");
  
    fp = SPIFFS.open(fileName, FILE_READ);
    size_t fileSize = fp.size();
    Serial.printf("file size = %d\n", fileSize);

    bool result = fileSize == waveSize;

    Serial.printf("file write %s\n", result ? "successed" : "FAILED");
    Serial.printf("\n");

    return result;
}

void WriteFiles()
{  
  FillDisplay(0xFFFFFF);
  delay(3000);
  
  Serial.println("SPIFFS file write");

  Serial.println("SPIFFS format start");
  SPIFFS.format();
  Serial.println("SPIFFS format end");

  FillDisplay(0x000000);

  int i;
  bool result;
  
  for (i = 0; i < FILE_COUNT; i ++) {
    char fileName[32];
    sprintf(fileName, FILE_NAME, i + 1);
    result = WriteFile(fileName, wavePtr[i], waveSize[i]);
    M5.dis.drawpix(i, result ? 0xFF0000 : 0x00FF00);
  }

  result = WriteFile(BOOT_NAME, boot, sizeof(boot));
  M5.dis.drawpix(i, result ? 0xFF0000 : 0x00FF00);

  result = WriteFile(TICK_NAME, tick, sizeof(tick));
  M5.dis.drawpix(i, result ? 0xFF0000 : 0x00FF00);

  delay(5000);
}

#endif
