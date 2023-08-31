// サーボモータを曲げセンサーで動かす
// by Gounbeee
// coaramause.com
// 2023


// 今回使用するモータドライバー、PCA9685を動かすためのライブラリ
//https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library
#include <Adafruit_PWMServoDriver.h>  


// ESP32が、2つのコアを持つことを利用し、
// Core 0 -> サーボモータを動かす
// Core 1 -> 曲げセンサーの入力を受ける
// で動かしたく思います。
// 
// そのため、まず2つのタスクをしてするためのオブジェクトを用意します。
TaskHandle_t tsk1;
TaskHandle_t tsk2;


// --------------------------------------------------------


// 上記で取り入れたライブラリを使って、サーボドライバー制御用オブジェクトを作ります。
Adafruit_PWMServoDriver servoDrv = Adafruit_PWMServoDriver(0x40);





// --------------------------------------------------------
// ここは、Task2用の、センサーのためのグローバルな情報です。
// 基本的には、別タスクになっている以上、同じメモリの場所を参照するときは
// 注意が必要です。共有している情報が、書き換えられる時間帯は、まだ情報が確定していないためです。
// よって、タスク間の通信をちゃんと行なって、「待ってもらう」などの処理が必要なときもありますが、
// ここでは省略します。

// ADC（Analog-to-Digital Converter）の機能を持つPINです。
int sensorPin = 4;

// センサーから受けた情報です。
// センサーからの情報を元に、サーボモータが動くべき地点を計算します。
int sensorValue = 0;




// このスケッチが実行されたとき、一度実行され必要な情報の準備を行う関数
void setup() {

  // Serial通信（ここではUSB）を通し、
  // Arduino IDEが持つ「シリアルモニター」を使って
  // 値を出力するためのセットアップ 
  Serial.begin(115200); 


  // --------------------------

  // センサーのためのPIN設定
  pinMode(sensorPin, INPUT);

  // --------------------------

  // サーボモータのためのセットアップ
  servoDrv.begin();

  // 今回使用するサーボモータを60Hzの速度で動くように設定
  servoDrv.setPWMFreq(60); 


  delay(10);


  // TASKを設定
  // サーボモータ用
  // 下記に定義している関数を指す
  //
  // Task1Job :  タスクの名前。この情報を用いて、タスク間のやり取りをしないといけないときもあります
  // "tsk1"   :  タスクの名前。この情報を用いて、タスク間のやり取りをしないといけないときもあります
  // 10000    :  このタスクが使用するメモリの大きさ
  // NULL.    :  このタスクに送る情報
  // 1        :  タスク処理の優先順位
  // &tsk1    :  タスクオブジェクトを指すポインター。このタスクの情報を見るための入り口のようなものです
  // 0.       :  このタスクを担当するコアの番号
  xTaskCreatePinnedToCore(
                    Task1Job,   
                    "tsk1",
                    10000,
                    NULL,
                    1,        
                    &tsk1,     
                    0);                  
  delay(500); 



  xTaskCreatePinnedToCore(
                    Task2Job,   
                    "tsk2",    
                    10000,       
                    NULL,        
                    1,          
                    &tsk2,
                    1);       
    delay(500); 



}





void loop() {
  // 通常の LOOPは使用しません。
}






// タスク１
// サーボモータを動かす処理
void Task1Job( void * pvParameters ) {

  // TASKは、独立したループを持つ必要があります。
  // 下記のFor文はそのためのものです。
  // また、For文はdelay(1)でもさせるようにしましょう。

  
  for(;;) {

    // サーボドライバーオブジェクトを使用し、
    // 最初から５個までのモータにセンサーから受け取った結果を反映し
    // 回転させる。
    servoDrv.writeMicroseconds(0, sensorValue );
    servoDrv.writeMicroseconds(1, sensorValue );
    servoDrv.writeMicroseconds(2, sensorValue );
    servoDrv.writeMicroseconds(3, sensorValue );
    servoDrv.writeMicroseconds(4, sensorValue );
    

    delay(10);

  }

}


// タスク2
// センサーの入力を受け取る処理
void Task2Job( void * pvParameters ) {

  for(;;) {

    // 最初に設定した、PIN４からセンサーの情報を受け取る
    sensorValue = analogRead(sensorPin); 
    //Serial.println(sensorValue);
    //Serial.println("  |  ");

    // 曲げセンサーの入力値は、0から4095(4096までの幅)
    // ですが、フィルムをある程度曲げるまではずっと4095を得られ、
    // そこから一気に1300辺りまで下がって変動しました。
    // そこで、一旦、4095（曲っていない状態）は、擬似的に数字を減らしました。
    if (sensorValue == 4095) sensorValue = 2000;

    // その上、map関数を使用して、値をスケールします。
    sensorValue = map(sensorValue, 0, 2000, 500, 2000);

    // さらに、今回のサーボモータは500~2500までのパルス幅を入力とし、
    // その値が、0°から270°までの回転上の位置を意味しているので、
    // その値を外れるような値が出ないようにconstrain関数でで切り取ります。
    sensorValue = constrain(sensorValue, 500, 2500);

    Serial.println(sensorValue);

    delay(20);
  }

}





