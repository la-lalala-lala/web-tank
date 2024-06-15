#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>


/**
 * 程序入口
 * 指令说明：
 * 程序只接受 2 个字节长度的指令，
 * 第一个字节为一级功能大类；
 * 第二个字节为二级功能小类；
 * 
 * 程序每次返回3个字节长度的数据
 * 第一个字节为数据标识
 * 第二个字节和第三个为数据值
 * 其中：第二个字节为高位整数部分，第三个字节为低位小数位；
 * 对于特殊的布尔类型：true：全部填0xFF 0xFF；false：全部填0x00 0x00
 * 传送和接收的字节数据中，每个字节表示的范围为：0~255
 * 
 * 程序每隔30s主动上报自己的心跳数据（0x01 任意值 任意值），如果在30s没有接收到回复，设备进入紧急状态，停止驱动
 * 设备主动发送的心跳数据为：0x01 0xFF 0xFF
 * 控制端的回复数据为：0x01 任意
 * 
 * 电源(0x00):
 *  启动：0x01  关闭：0x00
 * 心跳(0x01)  fffffff
 *  回复：0x01 0xff
 * 转向(0x02):
 *  左转：0x01  松开：0x00
 *  右转：0x02  松开：0x00
 * 方向(0x03):
 *  前进：0x01  松开：0x00
 *  后退：0x02  松开：0x00
 * 灯光(0x04):
 *  总开关启动：0x01  关闭：0x00
 *  前照灯：0x02
 *  停车灯：0x03
 */

// 红外避障传感器引脚(前)
// int headMH = 7;
// 前照灯
int headlight = 26;
// 停车灯&危险报警灯光引脚
int parklight = 25; 
// 定义驱动电机1控制引脚
int motor_a_1 = 18;
int motor_a_2 = 19;
// 定义驱动电机2控制引脚
int motor_b_1 = 22;
int motor_b_2 = 23;

// 总电源开关
boolean power_switch = false;
// 总驱动电源开关
boolean motor_switch = false;
// 总照明开关
boolean light_switch = false;

AsyncWebServer server(80);

//作为本地开启热点后的默认连接配置
String ap_ssid = "environment";
String ap_password = "123406789";

// 作为WiFi热点时的配置
IPAddress ap_ip(192,168,4,10);//AP端IP
IPAddress ap_gateway(192,168,4,1);//AP端网关
IPAddress ap_netmask(255,255,255,0);//AP端子网掩码

// 因为http没有头文件，所以在使用它的时候，提前定义一下
String getContentType(String fileName,AsyncWebServerRequest * request);
void handleNotFound(AsyncWebServerRequest * request);
void handleHomePage(AsyncWebServerRequest * request);
void handTurn(AsyncWebServerRequest * request);
void handGear(AsyncWebServerRequest * request);
void handCtrl(AsyncWebServerRequest * request);

void init_level();

// 采用AP模式
void config_ap() {
  Serial.println("Turn On Wi-Fi...");
    // 断开连接（防止已连接）
  WiFi.disconnect();
  // 设置成A模式
  WiFi.mode(WIFI_AP);
  // 设置AP网络参数
  WiFi.softAPConfig(ap_ip,ap_gateway,ap_netmask);
  // 设置AP账号密码
  WiFi.softAP(ap_ssid.c_str(),ap_password.c_str());
  // 板子的ip，也就是热点的ip
  Serial.print("apip：");
  Serial.println(WiFi.softAPIP());
  Serial.print("mac：");
  Serial.println(WiFi.softAPmacAddress().c_str());
  Serial.println();
}

// web服务器设置
void config_web_server(){
  // serving static content for GET requests on '/' from SPIFFS directory '/'
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html").setCacheControl("max-age=86400");
  // 打开登录页面
  server.on ("/", handleHomePage);
  server.on("/api/turn.action",HTTP_POST,handTurn);
  server.on("/api/gear.action",HTTP_POST,handGear);
  server.on("/api/ctrl.action",HTTP_POST,handCtrl);
  server.onNotFound(handleNotFound);
  server.begin();
}

void setup() {
  // 定义波特率，以便控制台打印
  Serial.begin(115200);
  SPIFFS.begin(true);
  // 定义各引脚为输出模式
  pinMode(headlight,OUTPUT);
  pinMode(parklight,OUTPUT);
  pinMode(motor_a_1,OUTPUT);
  pinMode(motor_a_2,OUTPUT);
  pinMode(motor_b_1,OUTPUT);
  pinMode(motor_b_2,OUTPUT);
  // pinMode(headMH,INPUT);
  // 启动时刻，全部置位低电平
  init_level();
  config_ap();
  config_web_server();
}


void loop() {

}