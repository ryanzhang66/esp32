#include <Wire.h>   //Arduino的库文件
#include <WiFi.h>   //ESP32的库文件
#include <PubSubClient.h>   //在“管理库”搜索“PubSubClient”，下载库文件
#include <ArduinoJson.h>    //在“管理库”搜索“ArduinoJson”，下载库文件，我这里使用的版本是5.13.5，如果是6及以上则会报错

/*MQTT连接配置*/
/*-----------------------------------------------------*/
const char* ssid = "A304";        //接入wifi的名字
const char* password = "wildfire";//接入wifi的密码
const char* mqttServer = "9d1b8ed274.st1.iotda-device.cn-north-4.myhuaweicloud.com";   //在华为云IoT的 总览->接入信息->MQTT（1883）后面的网址，这个是华为云的地址
const int   mqttPort = 1883;
//以下3个参数可以由HMACSHA256算法生成，为硬件通过MQTT协议接入华为云IoT平台的鉴权依据
const char* clientId = "64f7fcd5a559fd7cd411a77b_20230906_0_0_2023090813";
const char* mqttUser = "64f7fcd5a559fd7cd411a77b_20230906";
const char* mqttPassword = "aedf0c38d55bafce65ea4a9665c618bd9bd3a6788f7c13865710a2cc0866a98d";
 
WiFiClient espClient; //ESP32WiFi模型定义
PubSubClient client(espClient);

//华为云IoT的产品->查看->Topic管理->设备上报属性数据的 $oc/devices/{你的设备ID}/sys/properties/report
const char* topic_properties_report = "$oc/devices/{64f7fcd5a559fd7cd411a77b_20230906}/sys/properties/report";   
 
//接收到命令后上发的响应topic
char* topic_Commands_Response = "$oc/devices/设备ID/sys/commands/response/request_id=";
int i;
 
/*******************************************************/
/*
 * 作用：  ESP32的WiFi初始化以及与MQTT服务器的连接
 * 参数：  无
 * 返回值：无
 */
void MQTT_Init()
{
//WiFi网络连接部分
  WiFi.begin(ssid, password); //开启ESP32的WiFi
  while (WiFi.status() != WL_CONNECTED) { //ESP尝试连接到WiFi网络
    delay(3000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to the WiFi network");
 
 
//MQTT服务器连接部分
  client.setServer(mqttServer, mqttPort); //设置连接到MQTT服务器的参数
 
  client.setKeepAlive (60); //设置心跳时间
 
  while (!client.connected()) { //尝试与MQTT服务器建立连接
    Serial.println("Connecting to MQTT...");
  
    if (client.connect(clientId, mqttUser, mqttPassword )) {
  
      Serial.println("connected");  
  
    } else {
  
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(6000);
  
    }
  }
//接受平台下发内容的初始化
}

/*
 * 作用：  上报到MQTT服务器任务
 * 参数：  (int)可以继续增加
 * 返回值：无
 * 命名说明：Capacity：容量
 */
void TASK_Capacity_Report(int capacity)
{
 
//以下部分代码调用了ArduinoJson库将属性上报消息打包为JSON格式
//此部分代码可以通过ArduinoJson库的官方网站直接生成
  StaticJsonBuffer<300> JSONbuffer; //定义静态的JSON缓冲区用于存储JSON消息
  JsonObject& root = JSONbuffer.createObject();
  JsonArray& services = root.createNestedArray("services");
  JsonObject& service_1 = services.createNestedObject();
  JsonObject& properties_1_1 = service_1.createNestedObject("properties");
   
  service_1["service_id"] = "Manhole_Cover";    //这里的填的是 产品->查看->服务列表的服务名字
  properties_1_1["test"] = capacity;            //这里是服务里面的属性
  properties_1_1["test_2"] = 2;                 //这里是服务里面的属性
  properties_1_1["test_3"] = 3;                 //这里是服务里面的属性
  //可以继续增加...（但如果要上传变的数据，需要传参）
 
  //  root.prettyPrintTo(Serial);//调试用，将JSON打印到串口
 
 //以下将生成好的JSON格式消息格式化输出到字符数组中，便于下面通过PubSubClient库发送到服务器
  char JSONmessageBuffer[100];
  root.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  
  Serial.println("Sending message to MQTT topic..");
  Serial.println(JSONmessageBuffer);
 
 
//以下代码将打包好的JSON数据包通过PubSubClient库发送到服务器 
  if (client.publish(topic_properties_report, JSONmessageBuffer) == true) {
    Serial.println("Success sending message");
  } else {
    Serial.println("Error sending message");
  }
 
//由于本函数是放在loop中不断循环执行，所以添加client.loop()进行保持硬件的活跃度
//避免由于长时间未向服务器发送消息而被服务器踢下线
  client.loop();
  Serial.println("-------------");
  
}

void setup()
{
  MQTT_Init();

}

void loop()
{
  TASK_Capacity_Report(i);
  i=i+1;
}
