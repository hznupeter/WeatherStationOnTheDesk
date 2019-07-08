#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <DHT.h>
#define DHTPIN 12//DHT11传感器连接管脚
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);
char auth[] = "4c77138d712c48d7a6f922cf3d******";//授权码
char ssid[] = "ssid";//wifi名称
char pass[] = "pass";//wifi密码
String host = "api.seniverse.com";
String APIKEY = "wactucc9uy******"; //心知天气API
String city = "Hangzhou";
String language = "en";
String  daily_json;
int Humidity, Temperature, show_daily_count = 0;
BlynkTimer timer;

struct daily_data {//气象信息结构体
  String date;//日期
  int code;//气象代码
  int temp_high;//最高温度
  int temp_low;//最低温度
};
struct daily_data  today, tomorrow, tomorrow2;//初始化结构体，今天，明天，后天

static unsigned char week1[] = {//日
  0x00, 0x00, 0xFE, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0xFE, 0x03, 0x02, 0x02, 0x02, 0x02,  0x02, 0x02, 0x02, 0x02, 0xFE, 0x03, 0x02, 0x02};
static unsigned char week2[] = {//一
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x07, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static unsigned char week3[] = {//二
  0x00, 0x00, 0x00, 0x00, 0xFE, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0xFF, 0x07, 0x00, 0x00};
static unsigned char week4[] = {//三
  0x00, 0x00, 0xFE, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x01, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x07};
static unsigned char week5[] = {//四
  0x00, 0x00, 0xFE, 0x07, 0x92, 0x04, 0x92, 0x04, 0x92, 0x04, 0x92, 0x04, 0x92, 0x04, 0x0A, 0x07,  0x06, 0x04, 0x02, 0x04, 0xFE, 0x07, 0x02, 0x04};
static unsigned char week6[] = {//五
  0x00, 0x00, 0xFF, 0x03, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0xFE, 0x01, 0x08, 0x01, 0x08, 0x01,  0x08, 0x01, 0x04, 0x01, 0x04, 0x01, 0xFF, 0x07};
static unsigned char week7[] = {//六
  0x10, 0x00, 0x20, 0x00, 0x20, 0x00, 0x00, 0x00, 0xFF, 0x07, 0x00, 0x00, 0x88, 0x00, 0x08, 0x01,  0x04, 0x02, 0x04, 0x02, 0x02, 0x04, 0x01, 0x04};
static unsigned char Celsius[] = {//摄氏度符号
  0x06, 0x00, 0x00, 0x89, 0x2F, 0x00, 0x69, 0x30, 0x00, 0x36, 0x20, 0x00, 0x10, 0x20, 0x00, 0x18,  0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x10, 0x00, 0x00, 0x30, 0x20, 0x00, 0x60, 0x10, 0x00, 0x80, 0x0F, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static unsigned char Point[] = {//地标标志9*14
  0x38, 0x00, 0x7C, 0x00, 0xFE, 0x00, 0xEE, 0x00, 0xC7, 0x01, 0x83, 0x01, 0x01, 0x01, 0x83, 0x01, 0xC7, 0x01,0xEE, 0x00, 0xFE, 0x00, 0x7C, 0x00, 0x38, 0x00, 0x10, 0x00};
WidgetRTC rtc;//创建rtc组件
DynamicJsonDocument doc;
void showIcon(int code, int x, int y) { //显示天气图标函数
  if (code < 4) { //晴天
    u8g2.setFont(u8g2_font_open_iconic_weather_2x_t);
    u8g2.drawGlyph(x, y, 69);
  }
  else if (code < 9)  { //多云
    u8g2.setFont(u8g2_font_open_iconic_weather_2x_t);
    u8g2.drawGlyph(x, y, 65);
  }
  else if (code < 10)  { //阴天
    u8g2.setFont(u8g2_font_open_iconic_weather_2x_t);
    u8g2.drawGlyph(x, y, 64);
  }
  else if (code < 20)  { //雨
    u8g2.setFont(u8g2_font_open_iconic_weather_2x_t);
    u8g2.drawGlyph(x, y, 67);
  }
}

void showWeekday(int weekday, int x, int y) { //显示星期中文字函数
  switch (weekday) {//中文显示星期
    case 1:
      u8g2.drawXBM(x, y, 12, 12, week1);
      break;
    case 2:
      u8g2.drawXBM(x, y, 12, 12, week2);
      break;
    case 3:
      u8g2.drawXBM(x, y, 12, 12, week3);
      break;
    case 4:
      u8g2.drawXBM(x, y, 12, 12, week4);
      break;
    case 5:
      u8g2.drawXBM(x, y, 12, 12, week5);
      break;
    case 6:
      u8g2.drawXBM(x, y, 12, 12, week6);
      break;
    case 7:
      u8g2.drawXBM(x, y, 12, 12, week7);
      break;
  }
}

BLYNK_WRITE(V2) {
  daily_json = param.asStr();
  Serial.println(daily_json);
  deserializeJson(doc, daily_json);
  JsonObject obj = doc.as<JsonObject>();
  String date = obj["results"][0]["daily"][0]["date"].as<String>();//获取今天日期
  today.date = date.substring(5, 10); //截取字符串，只保留日期
  today.code = obj["results"][0]["daily"][0]["code"];//获取今天天气代码
  today.temp_high = obj["results"][0]["daily"][0]["high"];//获取今天天气温度
  today.temp_low = obj["results"][0]["daily"][0]["low"];//获取今天天气温度

  date = obj["results"][0]["daily"][1]["date"].as<String>();//获取明天日期
  tomorrow.date = date.substring(5, 10); //截取字符串，只保留日期
  tomorrow.code = obj["results"][0]["daily"][1]["code"];//获取明天天气代码
  tomorrow.temp_high = obj["results"][0]["daily"][1]["high"];//获取明天天气温度
  tomorrow.temp_low = obj["results"][0]["daily"][1]["low"];//获取明天天气温度

  date = obj["results"][0]["daily"][2]["date"].as<String>();//获取后天日期
  tomorrow2.date = date.substring(5, 10); //截取字符串，只保留日期
  tomorrow2.code = obj["results"][0]["daily"][2]["code"];//获取后天天气代码
  tomorrow2.temp_high = obj["results"][0]["daily"][2]["high"];//获取后天天气温度
  tomorrow2.temp_low = obj["results"][0]["daily"][2]["low"];//获取后天天气温度
}
void clockDisplay() { //显示时间及当前温湿度
  u8g2.setFontPosTop();
  u8g2.clearBuffer();
  if (second() == 0) { //当秒钟为0时，读取温湿度值，即每分钟读取一次
    Humidity = dht.readHumidity();
    Temperature = dht.readTemperature(); //摄氏度
  }
  Serial.println(Temperature);
  String currentTime = "" ;
  currentTime += (hour() < 10) ? "0" + String(hour()) : String(hour());
  currentTime += (minute() < 10) ? ":0" + String(minute()) : ":" + String(minute());
  currentTime += (second() < 10) ? ":0" + String(second()) : ":" + String(second());
  String currentDate = String(year());
  currentDate += (month() < 10) ? "-0" + String(month()) : "-" + String(month());
  currentDate += (day() < 10) ? "-0" + String(day()) : "-" + String(day());
  u8g2.drawFrame(0, 0, 128, 64);//画框
  u8g2.setFont(u8g2_font_timR14_tr);
  u8g2.setCursor(20, 6);
  if (Humidity < 100) {
    u8g2.print(String(Humidity) + "% " + String(Temperature) + "C"); //显示当前温湿度
    //u8g2.drawXBM(80, 6, 18, 18, Celsius);//显示摄氏度符号
  }
  u8g2.setFont(u8g2_font_helvR18_tf);
  u8g2.setCursor(15, 25);
  u8g2.print(currentTime);//显示当前时间
  u8g2.setFont(u8g2_font_timR12_tr);
  u8g2.setCursor(18, 50);
  u8g2.print(currentDate);//显示当前日期
  showWeekday(weekday(), 95, 50); //显示当前星期
  u8g2.sendBuffer();
}

void dailyDisplay() { //显示预报数据
  show_daily_count++;
  u8g2.setFontPosTop();
  u8g2.clearBuffer();
  u8g2.drawHLine(0, 18, 128);//水平分割线
  u8g2.drawVLine(42, 18, 64);//第一条竖直分割线
  u8g2.drawVLine(85, 18, 64);//第二条竖直分割线
  u8g2.drawXBM(18, 0, 9, 14, Point);//显示地标
  u8g2.setFont(u8g2_font_helvR12_tf);
  u8g2.setCursor(30, 0);
  u8g2.print(city);//显示城市名称
  showIcon(today.code, 12, 21);//显示今天天气图标
  showIcon(tomorrow.code, 54, 21);//显示明天天气图标
  showIcon(tomorrow2.code, 96, 21);//显示后天天气图标
  u8g2.setFont(u8g2_font_helvR08_tf);
  u8g2.setCursor(8, 43);
  u8g2.print(String(today.temp_low) + "~" + String(today.temp_high));//显示今天最低温度和最高温度
  u8g2.setCursor(50, 43);
  u8g2.print(String(tomorrow.temp_low) + "~" + String(tomorrow.temp_high));//显示明天最低温度和最高温度
  u8g2.setCursor(93, 43);
  u8g2.print(String(tomorrow2.temp_low) + "~" + String(tomorrow2.temp_high));//显示后天最低温度和最高温度
  u8g2.setCursor(8, 55);
  u8g2.print(today.date);//显示今天日期
  u8g2.setCursor(50, 55);
  u8g2.print(tomorrow.date);//显示明天日期
  u8g2.setCursor(93, 55);
  u8g2.print(tomorrow2.date);//显示后天日期
  u8g2.sendBuffer();
}
void Display() { //显示当前时间和气温
  Serial.println(show_daily_count);
 if (show_daily_count > 0 && show_daily_count < 5) {
    dailyDisplay();//显示天气预报
  }
  else  {
    clockDisplay();//显示时钟
  }
}
void displaySwitch() { //中断函数
  Blynk.virtualWrite(V2, "https://" + host + "/v3/weather/daily.json?key=" + APIKEY + "&location=" + city + "&language=en&unit=c&start=0&days=5");
  show_daily_count = 1;
}
BLYNK_CONNECTED() {
  rtc.begin();//连上后同步时间
}

void setup() {
  Serial.begin(9600);
  u8g2.begin();
  u8g2.setDisplayRotation(U8G2_R2);//调整屏幕显示方向
  Blynk.begin(auth, ssid, pass);//自建服务器ip模式
  Blynk.virtualWrite(V2, "https://" + host + "/v3/weather/daily.json?key=" + APIKEY + "&location=" + city + "&language=en&unit=c&start=0&days=5");
  setSyncInterval(10 * 60); // 设置同步间隔时间，10分钟
  attachInterrupt(digitalPinToInterrupt(14), displaySwitch, RISING);
  timer.setInterval(1000L, Display);//每隔1s，运行clockDisplay，显示时间和温湿度
  Humidity = dht.readHumidity();
  Temperature = dht.readTemperature(); //摄氏度
}

void loop() {
  Blynk.run();
  timer.run();
}