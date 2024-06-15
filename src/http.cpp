#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// 红外避障传感器引脚(前)
// const int headMH
// 前照灯
extern int headlight;
// 停车灯&危险报警灯光引脚
extern int parklight; 
// 定义驱动电机1控制引脚
extern int motor_a_1;
extern int motor_a_2;
// 定义驱动电机2控制引脚
extern int motor_b_1;
extern int motor_b_2;
// 总电源开关
boolean power_switch = false;
// 总驱动电源开关
boolean motor_switch = false;
// 总照明开关
boolean light_switch = false;


/**
 * 根据文件后缀获取html协议的返回内容类型
 */
String getContentType(String fileName,AsyncWebServerRequest * request){
  if(request->hasParam("download")){
    return "application/octet-stream";
  }else if(fileName.endsWith(".htm")){
    return "text/html";
  }else if(fileName.endsWith(".html")){
    return "text/html";
  }else if(fileName.endsWith(".css")){
    return "text/css";
  }else if(fileName.endsWith(".js")){
    return "text/javascript";
  }else if(fileName.endsWith(".png")){
    return "image/png";
  }else if(fileName.endsWith(".gif")){
    return "image/gif";
  }else if(fileName.endsWith(".jpg")){
    return "image/jpeg";
  }else if(fileName.endsWith(".ico")){
    return "image/x-icon";
  }else if(fileName.endsWith(".xml")){
    return "text/xml";
  }else if(fileName.endsWith(".pdf")){
    return "application/x-pdf";
  }else if(fileName.endsWith(".zip")){
    return "application/x-zip";
  }else if(fileName.endsWith(".gz")){
    return "application/x-gzip";
  }else{
    return "text/plain";
  }
}

/* NotFound处理 
 * 用于处理没有注册的请求地址 
 * 一般是处理一些页面请求 
 */
void handleNotFound(AsyncWebServerRequest * request){
  String path = request->url();
  Serial.print("load url:");
  Serial.println(path);
  String contentType = getContentType(path,request);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz)){
      path += ".gz";
    }
    //Send index.htm as text
    request->send(SPIFFS, path, contentType);
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET ) ? "GET" : "POST";
  request->send(404, "text/plain", message);
}

/**
 * 请求打开主控页面
 */
void handleHomePage(AsyncWebServerRequest * request) {
  //Send index.htm as text
  request->send(SPIFFS, "/index.htm", "text/plain");
  return;
}

/**
 * 电机置为低电平
 */
void init_motor_level(){
  digitalWrite(motor_a_1,LOW);
  digitalWrite(motor_a_2,LOW);
  digitalWrite(motor_b_1,LOW);
  digitalWrite(motor_b_2,LOW);
}

/**
 * 照明置为低电平
 */
void init_light_level(){
  digitalWrite(headlight,LOW);
  digitalWrite(parklight,LOW);
}

/**
 * 全部置为低电平，用于开机和停机
 */
void init_level(){
  init_motor_level();
  init_light_level();
}

/**
 * 处理总电气开关
 * 启动：1  关闭：0
 */
boolean power_switch_handel(int value){
   if(0 == value){ 
      // 总电源开关
      power_switch = false;
      // 总驱动电源开关
      motor_switch = false;
      // 总照明开关
      light_switch = false;
      // 所有相关的引脚都置位低电平
      init_level();
   }else{
      // 总电源开关
      power_switch = true;
      // 总驱动电源开关
      motor_switch = true;
      // 总照明开关
      light_switch = true;
   }
  return true;
}

/**
 * 处理总驱动开关
 * 启动：1  关闭：0
 */
boolean motor_switch_handel(int value){
  boolean result = false;
   if(0 == value){ 
      // 总驱动电源开关
      motor_switch = false;
      // 所有相关的引脚都置位低电平
      init_motor_level();
   }else{
      // 总驱动电源开关
      if(power_switch){
        motor_switch = true;
        result = true;
      }
   }
   return result;
}

/**
 * 处理灯光
 * 灯光(0x04):
 *  总开关启动：0x01  关闭：0x00
 *  前照灯：0x02
 *  停车灯：0x03
 */
boolean light_switch_handel(int fun,int value){
  boolean result = false;
  boolean light_status;
  switch(fun){
    case 0:
      // 关闭所有灯光
      init_light_level();
      // 电气照明开关 断开
      light_switch = false;
      result = true;
      break;
    case 1:
      // 电气照明开关 合闸
      if(power_switch){
        light_switch = true;
        result = true;
      }
      break;
    case 2:
      // 开启&关闭前照灯
      // 操作灯光的前提是：总电气开关已经打开 且 总照明开关已经打开
      if(power_switch && light_switch){
        light_status = value == 0 ? false : true;
        digitalWrite(headlight,light_status);
        result = true;
      }
      break;
    case 3:
      // 开启&关闭停车灯
      light_status = digitalRead(parklight);
      // 操作灯光的前提是：总电气开关已经打开 且 总照明开关已经打开
      if(power_switch && light_switch){
        light_status = value == 0 ? false : true;
        digitalWrite(headlight,light_status);
        result = true;
      }
      break;
    default:
      // 空转，什么指令都不执行
      break;  
  }
  return result;
}

/**
 * 前进
 */
void forward(){
  // 驱动电机在运行，停车灯处于低电平
  digitalWrite(parklight,LOW);
  digitalWrite(motor_a_1,HIGH);
  digitalWrite(motor_a_2,LOW);
  digitalWrite(motor_b_1,HIGH);
  digitalWrite(motor_b_2,LOW);
}

/**
 * 后退
 */
void backward(){
   // 驱动电机在运行，停车灯处于低电平
   digitalWrite(parklight,LOW);
  // 假如当前状态不是后退
  digitalWrite(motor_a_1,LOW);
  digitalWrite(motor_a_2,HIGH);
  digitalWrite(motor_b_1,LOW);
  digitalWrite(motor_b_2,HIGH);
}

/**
 * 停车(松开前进&后退)
 */
void suspend(){
  // 车灯高电平
  digitalWrite(parklight,HIGH);
  // 假如当前状态不是停车
  digitalWrite(motor_a_1,LOW);
  digitalWrite(motor_a_2,LOW);
  digitalWrite(motor_b_1,LOW);
  digitalWrite(motor_b_2,LOW);
}

/**
 * 左转
 */
void turn_left(){
   // 驱动电机在运行，停车灯处于低电平
   digitalWrite(parklight,LOW);
   // 左电机后退
   digitalWrite(motor_a_1,LOW);
   digitalWrite(motor_a_2,HIGH);
   // 右电机前进
   digitalWrite(motor_b_1,HIGH);
   digitalWrite(motor_b_2,LOW);
}

/**
 * 右转
 */
void turn_right(){
   // 驱动电机在运行，停车灯处于低电平
   digitalWrite(parklight,LOW);
    // 左电机前进
   digitalWrite(motor_a_1,HIGH);
   digitalWrite(motor_a_2,LOW);
   // 右电机后退
   digitalWrite(motor_b_1,LOW);
   digitalWrite(motor_b_2,HIGH);
}

/**
 * 处理转向
 * 转向(0x02):
 *  左转：1  松开：0
 *  右转：2  松开：0
 */
boolean turn_handel(int value){
  // 转向的前提是：总电气开关已经打开 且 总驱动电源开关
  if(false == power_switch || false == motor_switch){
    return false;
  }
  switch(value){
    case 0:
      // 停车
      suspend();
      break;
    case 1:
      // 左转
      turn_left();
      break;
    case 2:
      // 右转
      turn_right();
      break;
    default:
      // 空转，什么指令都不执行
      break;  
  }
  return false;
}

/**
 * 处理前进、后退、停车
 * 方向(0x03):
 *  前进：0x01  松开：0x00
 *  后退：0x02  松开：0x00
 */
boolean gear_handel(int value){
  // 驱动的前提是：总电气开关已经打开 且 总驱动电源开关
  if(false == power_switch || false == motor_switch){
    return false;
  }
  switch(value){
    case 0:
      // 停车
      suspend();
      break;
    case 1:
      // 前进
      forward();
      break;
    case 2:
      // 倒车
      backward();
      break;
    default:
      // 空转，什么指令都不执行
      break;  
  }
  return true;
}

/**
 * 转向接口
 */
void handTurn(AsyncWebServerRequest * request){
  AsyncWebServerResponse *response;
  if(request->hasArg("level")) {
    int level = request->arg("level").toInt();
    // 处理转向
    turn_handel(level);
    response = request->beginResponse(200, "text/json", "{\"code\":0,\"msg\":\"设置成功\"}");
  }else{
    response = request->beginResponse(200, "text/json", "{\"code\":-1,\"msg\":\"缺少参数\"}");
  }
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}

/**
 * 档位接口
 */
void handGear(AsyncWebServerRequest * request){
  AsyncWebServerResponse *response;
  if(request->hasArg("level")) {
    int level = request->arg("level").toInt();
    // 处理驱动
    gear_handel(level);
    response = request->beginResponse(200, "text/json", "{\"code\":0,\"msg\":\"设置成功\"}");
  }else{
    response = request->beginResponse(200, "text/json", "{\"code\":-1,\"msg\":\"缺少参数\"}");
  }
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}

/**
 * 开关接口
 */
void handCtrl(AsyncWebServerRequest * request){
  AsyncWebServerResponse *response;
  if(request->hasArg("fun") && request->hasArg("level")) {
    int fun = request->arg("fun").toInt();
    int level = request->arg("level").toInt();
    if (1 == fun){
      power_switch_handel(level);
    }else if (2 == fun){
      motor_switch_handel(level);
    }else if (3 == fun){
      
    }else{

    }
    response = request->beginResponse(200, "text/json", "{\"code\":0,\"msg\":\"设置成功\"}");
  }else{
    response = request->beginResponse(200, "text/json", "{\"code\":-1,\"msg\":\"缺少参数\"}");
  }
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}