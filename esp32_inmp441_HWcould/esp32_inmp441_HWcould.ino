#include <Wire.h>   //Arduino的库文件
#include <WiFi.h>   //ESP32的库文件
#include <PubSubClient.h>   //在“管理库”搜索“PubSubClient”，下载库文件
#include <ArduinoJson.h>    //在“管理库”搜索“ArduinoJson”，下载库文件，我这里使用的版本是5.13.5，如果是6及以上则会报错
#include <driver/i2s.h>

/*MQTT连接配置*/
/*-----------------------------------------------------*/
const char* ssid = "yuizhu";        //接入wifi的名字
const char* password = "zhangran20040906";//接入wifi的密码
const char* mqttServer = "9d1b8ed274.st1.iotda-device.cn-north-4.myhuaweicloud.com";   //在华为云IoT的 总览->接入信息->MQTT（1883）后面的网址，这个是华为云的地址
const int   mqttPort = 1883;
//以下3个参数可以由HMACSHA256算法生成，为硬件通过MQTT协议接入华为云IoT平台的鉴权依据
const char* clientId = "653242e11418363e9c5c8d89_20231020_0_0_2023102009";
const char* mqttUser = "653242e11418363e9c5c8d89_20231020";
const char* mqttPassword = "fdfbe29a34b5f4af8d64ddb2e74d6d4e134b1a73c0ffe8395762e3b24ac735d6";
 
WiFiClient espClient; //ESP32WiFi模型定义
PubSubClient client(espClient);

//华为云IoT的产品->查看->Topic管理->设备上报属性数据的 $oc/devices/{你的设备ID}/sys/properties/report
const char* topic_properties_report = "$oc/devices/{653242e11418363e9c5c8d89_20231020}/sys/properties/report";   
 
//接收到命令后上发的响应topic
char* topic_Commands_Response = "$oc/devices/设备ID/sys/commands/response/request_id=";
int i;
//按照接线确定编号
#define I2S_WS 15
#define I2S_SD 32
#define I2S_SCK 14
 
// 使用I2S处理器
#define I2S_PORT I2S_NUM_0
 
// 定义缓冲区长度
#define bufferLen 64
int16_t sBuffer[bufferLen];
 
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
void TASK_Capacity_Report(int buf[100])
{
 int a[10]={1,2,3,4,5,6,7,8,9,0};
//以下部分代码调用了ArduinoJson库将属性上报消息打包为JSON格式
//此部分代码可以通过ArduinoJson库的官方网站直接生成
  StaticJsonBuffer<300> JSONbuffer; //定义静态的JSON缓冲区用于存储JSON消息
  JsonObject& root = JSONbuffer.createObject();
  JsonArray& services = root.createNestedArray("services");
  JsonObject& service_1 = services.createNestedObject();
  JsonObject& properties_1_1 = service_1.createNestedObject("properties");
   
  service_1["service_id"] = "Manhole_Cover";    //这里的填的是 产品->查看->服务列表的服务名字
  properties_1_1["test"] = buf;            //这里是服务里面的属性
  //properties_1_1["test_2"] = a[3];                 //这里是服务里面的属性
  //properties_1_1["test_3"] = 3;                 //这里是服务里面的属性
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

void i2s_install() {
  //设置I2S处理器配置
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = bufferLen,
    .use_apll = false
  };
 
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}
 
void i2s_setpin() {
  // 设置I2S引脚配置
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };
 
  i2s_set_pin(I2S_PORT, &pin_config);
}
 
void inmp441_init() {
  // 设置串口监视器
  Serial.begin(115200);
  Serial.println(" ");
  delay(1000);
  // 设置I2S
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);
  delay(500);
}
 
int inmp441_get() {
 
  // 获取I2S数据并将其放入数据缓冲区
  size_t bytesIn = 0;
  esp_err_t result = i2s_read(I2S_PORT, &sBuffer, bufferLen, &bytesIn, portMAX_DELAY);
 
  if (result == ESP_OK)
  {
    // 读取I2S数据缓冲区
    int16_t samples_read = bytesIn / 8;
    if (samples_read > 0) {
      float mean = 0;
      for (int16_t i = 0; i < samples_read; ++i) {
        mean += (sBuffer[i]);
      }
       // 取数据读数的平均值
      mean /= samples_read;
      // 串口绘图   
      return mean;
    }
  }
}

void setup()
{
  MQTT_Init();
  inmp441_init();
}

int inmp441_buf[100];

void loop()
{
  for (int i=0; i<100; i++)
  {
    inmp441_buf[i] = inmp441_get();
  }
  TASK_Capacity_Report(inmp441_buf);
  delay(50U);
}
