/***
 * SPDX-License-Identifier: MIT
 * 
 ! ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!!! Attention !!!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 !
 ! 请在编译此项目前，将 FreeRTOS 配置文件中的 configUSE_PREEMPTION 定义为 0 ，以便开启协同式调度
 !      关于此调度方式的详情可在 https://www.freertos.org/zh-cn-cmn-s/a00110.html 中找到。
 ! FreeRTOS的配置文件是 .pio\libdeps\uno\FreeRTOS\src\FreeRTOSConfig.h （需要完成依赖安装） 
 !
 ! Proteus 中，单片机的ELF文件路径为 ..\.pio\build\uno\firmware.elf
 !
 ! ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~!!! Attention !!!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * 
 * 智能教室控制系统，能够实时读取室内温湿度，并实现按照人员分布开关灯。
 * 
 * 串口通讯格式：
 *      后端查询温度、湿度、气压、光照强度：
 *      后端向前端发送字符串： "query env"
 *      后端向前端返回字符串： "env,温度,湿度,气压,光照强度,距离" 均为浮点数
 * 
 *      后端查询LED状态：
 *      后端向前端发送字符串： "query led"
 *      后端向前端返回字符串： "led,1,0,1,0,STATE" 1 代表开，0代表关，STATE 有四种：on、off、track、alone
 * 
 *      后端控制灯亮灭： 灯的序号从 0 开始，总共 4 盏灯
 *      后端："control led on" "control led off" "control led track" "control led alone 灯的序号,0/1" 
 *      返回："ok" "fail"
 * 
*/


#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <SPI.h>
#include <DHT11.h>
// #include <semphr.h>

// **** 硬件控制信息声明区 ****

void(* resetFunc) (void) = 0;   // 在地址 0 位置声明 reset 函数

const int DHT11_PIN = 4;        // 温湿度传感器引脚
const int GP2D12_PIN = A0;      // 红外传感器引脚
const int BUZZER_PIN = 5;       // 蜂鸣器引脚
const int LDR_PIN = A1;         // 光敏传感器引脚
const int LED_NUM = 4;          // LED 灯数量

// **** 数据存储声明区 ****

struct ALL_DATA {
    float temp;     // 温度
    float hum;      // 湿度
    float light;    // 光照强度
    float press;    // 气压
    float distance; // 检测到的距离
} env_data {0, 0, 0, 0, 0};         // 环境数据

class LED_CON {
private:
    enum LightType {off, track, allon, alone};  // 灯的控制状态，全关，跟踪，全开，单独控制
    const int led_pin[LED_NUM] {6, 7, 8, 9};    // 灯的引脚
    const int led_num {LED_NUM};                // 灯的数量
    bool led_state[LED_NUM];                    // 每个灯的状态
    LightType allset;                           // 全局设置
    // SemaphoreHandle_t led_con_right;            // 灯光控制权，互斥信号量
public:
    LED_CON();
    ~LED_CON();
    void init();                        // 用于初始化灯光控制
    bool set_off();                     // 设置全关
    bool set_allon();                   // 设置全开
    bool set_track();                   // 设置跟踪
    bool set_alone(int n, bool state);  // 设置单独开关某一个
    bool change_position(int position); // 改变灯光跟踪位置
    String query_led_state();           // 查看灯光开关状态
}led_control;

// **** 函数以及进程句柄声明区 ****

void serial_send_func(void* pvParameters);      // 处理串口交互信息
TaskHandle_t serial_send_handler;

void env_read_func(void* pvParameters);         // 从硬件中读取环境信息
TaskHandle_t env_read_handler;

void distance_read_func(void* pvParameters);    // 红外传感器读取位置
TaskHandle_t distance_read_handler;

bool control_func(String comdata);          // 处理控制信息
String serial_read_func();                  // 读取串口信息
String env_to_string_func();                // 将存储的信息以 env 格式返回


// **** setup() ****

void setup() {
    Serial.setTimeout(100); // 设置串口超时时间 100ms
    Serial.begin(9600);     // 初始化串口，波特率为 9600
    led_control.init();
    
    xTaskCreate (env_read_func,
        "env_read_func",
        128,
        NULL,
        2,
        &env_read_handler
    );
    xTaskCreate (serial_send_func,
        "serial_send_func",
        128,
        NULL,
        2,
        &serial_send_handler
    );
    xTaskCreate (distance_read_func,
        "distance_read_func",
        64,
        NULL,
        2,
        &distance_read_handler
    );

}


void loop() {
    resetFunc();
}

void serial_send_func(void *pvParameters) {
    String comdata; // 存储串口数据
    for(;;) {
        comdata = serial_read_func();
        if(comdata[0] == 'q') {
            if(comdata[6] == 'e') {
                Serial.print(env_to_string_func());
            }
            else if(comdata[6] == 'l') {
                Serial.println("led"+led_control.query_led_state());
            }
        }
        else if(comdata[0] == 'c') {
            // Serial.println(comdata); // debug 输出调试函数
            if(control_func(comdata)) Serial.println("ok");
            else Serial.println("fail");
        }
        else if(comdata == "Reset") resetFunc();
        else {
            Serial.println(comdata); // 输出调试函数
        }
    }

}

bool control_func(String comdata) {
    switch (comdata[13])
    {
    case 'n': led_control.set_allon();
        break;
    case 'f': led_control.set_off();
        break;
    case 'r': led_control.set_track();
        break;
    case 'l' : if(!led_control.set_alone(comdata[18]-'0', comdata[20]-'0')) return false;
        break;
    default:
        return false;
        break;
    }

    return true;
}

String serial_read_func() {
    String comdata;
    for(;;){
        if (Serial.available() != 0){
            comdata = Serial.readString();
            comdata.trim();
            break;
        }
        else {
            taskYIELD();
            led_control.change_position(int((env_data.distance-10) / 20));
        }
    }
    return comdata;
}

void env_read_func(void *pvParameters) {
    Adafruit_BMP085 bmp;                    // 气压传感器控制声明
    DHT11 dht(DHT11_PIN);                   // 温湿度传感器控制声明
    bmp.begin();
    // Serial.println("Init Success!");     // debug
    for(;;){
        // Serial.println("Start to Read env!");  // debug
        env_data.press = bmp.readPressure();
        env_data.light = analogRead(LDR_PIN);
        env_data.hum = dht.readHumidity();
        env_data.temp = dht.readTemperature();
        // Serial.println("ENV READ SUCCESS!"); // debug
        vTaskDelay(2 * portTICK_PERIOD_MS);
        // taskYIELD();
    }
    
}

void distance_read_func(void *pvParameters) {
    int val{0};
    while(1) {
        val = analogRead(GP2D12_PIN);
        env_data.distance = 2547.8/((float)val*0.49-10.41)-0.42;
        if ( env_data.distance > 80 || env_data.distance < 10 ) {
            Serial.println("ERROR: Over Range!");
        }
        env_data.distance = int(env_data.distance * 10)/10.0;
        vTaskDelay(2 * portTICK_PERIOD_MS);
        // taskYIELD();
    }
}

String env_to_string_func() {
    String str = "env,";
    str += env_data.temp;
    str += ',';
    str += env_data.hum;
    str += ',';
    str += env_data.press;
    str += ',';
    str += env_data.light;
    str += ',';
    str += env_data.distance;
    str += "\r\n";

    return str;
}

LED_CON::LED_CON(){
    allset = off;
    for(int i = 0; i < led_num; ++i) {
        led_state[i] = false;
    }
}

LED_CON::~LED_CON() {
}

void LED_CON::init() {
    for(int i = 0; i < led_num; ++i) {
        pinMode(led_pin[i], OUTPUT);
        digitalWrite(led_pin[i], led_state[i]);
    }
}

bool LED_CON::set_off() {
    // if(xSemaphoreTake(led_con_right, portMAX_DELAY) != pdPASS) {
    //     return false;
    // }
    allset = off;
    for(int i = 0; i < led_num; ++i) {
            led_state[i] = false;
            digitalWrite(led_pin[i], led_state[i]);
    }
    // xSemaphoreGive(led_con_right);
    return true;
}

bool LED_CON::set_allon() {
    // if(xSemaphoreTake(led_con_right, portMAX_DELAY) != pdPASS) {
    //     return false;
    // }
    allset = allon;
    for(int i = 0; i < led_num; ++i) {
            led_state[i] = true;
            digitalWrite(led_pin[i], led_state[i]);
    }
    // xSemaphoreGive(led_con_right);
    return true;
}

bool LED_CON::set_track() {
    // if(xSemaphoreTake(led_con_right, portMAX_DELAY) != pdPASS) {
    //     return false;
    // }
    allset = track;
    // xSemaphoreGive(led_con_right);
    return true;
}

bool LED_CON::set_alone(int n, bool state) {
    if(n<0 || n > led_num-1) return false;
    allset = alone;
    led_state[n] = state;
    digitalWrite(led_pin[n], led_state[n]);
    return true;
}

bool LED_CON::change_position(int position) {
    if(allset != track) return false;
    // if(xSemaphoreTake(led_con_right, portMAX_DELAY) != pdPASS) {
    //     return false;
    // }
    allset = track;
    for(int i = 0; i < led_num; ++i) {
        if(i == position) {
            led_state[i] = true;
            digitalWrite(led_pin[i], led_state[i]);
        }
        else {
            led_state[i] = false;
            digitalWrite(led_pin[i], led_state[i]);
        }
    }
    // xSemaphoreGive(led_con_right);
    return true;
}

String LED_CON::query_led_state() {
    String res = ",";
    for(int i = 0; i < led_num; ++i) {
        res += led_state[i];
        res += ',';
    }
    switch (allset)
    {
    case allon:
        res += "on";
        break;
    case off:
        res += "off";
        break;
    case track:
        res += "track";
        break;
    case alone:
        res += "alone";
        break;
    default:
        res += "error";
        break;
    }
    return res;
}
