/*
    协处理器(通信处理) - MCP协议版本
*/

#include <cJSON.h>
#include <esp_log.h>

#include <cstring>
#include "system_info.h"
#include "device_state.h"
#include "application.h"
#include "board.h"
#include "config.h"
#include "mcp_server.h"
// #include "otto_movements.h"
#include "sdkconfig.h"
#include "settings.h"
#include "uart_comm.h"

#define TAG "XieCoprocessor"

class XieCoprocessor {
public:
    XieCoprocessor() {
        uc_uart = new UartComm(UART_NUM, TX_PIN, RX_PIN, BAUD_RATE, BUF_SIZE);
        uc_uart->init();
        if (xie_task_handle_ == nullptr)
        {
            // 启动串口接收任务
            xTaskCreate([](void *arg)
            {
                XieCoprocessor* xie = (XieCoprocessor*)arg;
                while (true) {
                    xie->uc_uart->receiveDataCjson();
                    vTaskDelay(pdMS_TO_TICKS(10));  // 每 ms 检查一次接收的数据
            } }, "uart_receive_task", 4096, this, 1, &xie_task_handle_);
        }
        if (xie_sensor_data_task_handle_ == nullptr) {
        
                // // 启动串口接收任务
                xTaskCreate([](void *arg)
                {
                    XieCoprocessor* xie = (XieCoprocessor*)arg;
                    auto& board = Board::GetInstance();
                    auto network = Board::GetInstance().GetNetwork();
                    auto& app = Application::GetInstance();
                    vTaskDelay(pdMS_TO_TICKS(3000));
                    std::string url = app.GetSensorDataUrl();
                    std::string explain_url_; // 传感器数据上传地址
                    bool update_flag = true;
                    
                    explain_url_ = "https://changeisgreat.cn/user/client/client/addSensorInfo";
                    while(1)
                    {
                    vTaskDelay(pdMS_TO_TICKS(3000));
                    if (app.GetDeviceState() == DeviceState::kDeviceStateIdle) {
                        if (update_flag) {
                            url = app.GetSensorDataUrl();
                            if (url.empty())
                            {
                                explain_url_ = "https://changeisgreat.cn/user/client/client/addSensorInfo";
                            }else {
                                explain_url_ = url;
                                ESP_LOGW(TAG,"数据上报接口以更新 url: %s", explain_url_.c_str());
                            }
                            update_flag = false;
                        }

                    }else {
                        // 网络未连接或设备不在空闲状态，不执行上传操作
                        continue;
                    }
                    auto http = network->CreateHttp(4);
                    
                //     // std::string boundary = "----ESP32_CAMERA_BOUNDARY";

                //     // http->SetHeader("Device-Id", SystemInfo::GetMacAddress().c_str());
                //     // http->SetHeader("Client-Id", Board::GetInstance().GetUuid().c_str());
                //     // if (!explain_token_.empty()) {
                //     //     http->SetHeader("Authorization", "Bearer " + explain_token_);
                //     // }
                //     // http->SetHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
                //     // http->SetHeader("Transfer-Encoding", "chunked");
                //     // url地址
                    std::string body = app.g_sensor.generateSensorJson(SystemInfo::GetMacAddress());
                    ESP_LOGI(TAG, "Sent body to %s", body.c_str());
                    http->SetHeader("Content-Type", "application/json");
                    http->SetContent(std::move(body));
                    // http->SetKeepAlive(true);
                    

                    if (!http->Open("POST", explain_url_)) {
                        ESP_LOGE(TAG, "Failed to connect to explain URL");
                        continue;
                    }

                    
                    // http->Write(body.c_str(), body.size());
                    

                    if (http->GetStatusCode() != 200) {
                        ESP_LOGE(TAG, "Failed to upload Sensor data, status code: %d", http->GetStatusCode());
                        continue;
                        // throw std::runtime_error("Failed to upload photo");
                    }

                    std::vector<char> buffer(4096/2); 

                    int read_len = 0;
                    std::string result = http->ReadAll();;
                    // while (true) {
                    //     // 调用底层 Read 函数读取数据
                    //     read_len = http->Read(buffer.data(), buffer.size());
                
                    //     if (read_len > 0) {
                    //         // 成功读取到数据，追加到结果字符串中
                    //         result.append(buffer.data(), read_len);
                    //     } 
                    //     else if (read_len == 0) {
                    //         // read_len 为 0 表示服务器关闭了连接（Connection: close）
                    //         // 或者读取到了 Chunked 编码的结束符
                    //         ESP_LOGI(TAG, "Connection closed or end of stream. Total bytes read: %d", result.length());
                    //         break;
                    //     } 
                    //     else {
                    //         // read_len < 0 表示读取过程中发生错误
                    //         ESP_LOGE(TAG, "Error occurred while reading data: %d", read_len);
                    //         break;
                    //     }
                    // }
                    ESP_LOGW(TAG, "Upload result: %s", result.c_str());
                    http->Close();
                    
    
                    }
                } , "uart_receive_task", 1024*5, this, 1, &xie_sensor_data_task_handle_);
        }
        
        RegisterMcpTools();



    }

    ~XieCoprocessor() {
        
    }
    UartComm* uc_uart;
    std::string uc_string;
private:
    TaskHandle_t xie_task_handle_ = nullptr;
    TaskHandle_t xie_sensor_data_task_handle_ = nullptr; // 传感器数据上传任务句柄
    QueueHandle_t xie_queue_;

    void RegisterMcpTools() {
        auto& mcp_server = McpServer::GetInstance();

        ESP_LOGI(TAG, "开始注册MCP工具...");

        // 协处理器基础传感器数据
        mcp_server.AddTool("self.kftj.get_temperature",
            "获取开发套件上的温度传感器采集的温度数据(单位摄氏度)",
            PropertyList(),
            [this](const PropertyList& properties) -> ReturnValue {
                auto& app = Application::GetInstance();
                std::string status =
                "{\"temperature\":" + app.GetTemperature() +  + "}";
                printf("温度值: %s\n", status.c_str());
                return status; // 返回温度值
            });
        mcp_server.AddTool("self.kftj.get_humidity",
            "获取开发套件上的湿度传感器采集的湿度数据",
            PropertyList(),
            [this](const PropertyList& properties) -> ReturnValue {
                auto& app = Application::GetInstance();
                std::string status =
                "{\"temperature\":" + app.GetHumidity() + "%" + "}";
                printf("温度值: %s\n", status.c_str());
                return status;  // 返回湿度值
            });

        mcp_server.AddTool("self.kftj.set_rgbled",
                           "设置开发套件上的rgb灯,参数全0为关灯状态,根据rgb颜色表使rgb灯亮起不同颜色。red: 红色值(0-255); green: 绿色值(0-255); blue: 蓝色值(0-255)",
                           PropertyList({Property("red", kPropertyTypeInteger, 0, 255),
                                         Property("green", kPropertyTypeInteger, 0, 255),
                                         Property("blue", kPropertyTypeInteger, 0, 255)}),
                           [this](const PropertyList& properties) -> ReturnValue {
                               int red = properties["red"].value<int>();
                               int green = properties["green"].value<int>();
                               int blue = properties["blue"].value<int>();
                               int rgb = (blue << 16) | (green << 8) | red;
                               std::string rgb_str = std::to_string(rgb);

                               // 设置RGB灯颜色
                               // TODO: 调用设置RGB灯颜色的函数
                               std::string status = "{\"name\":\"WS2812\",\"type\":\"write\",\"property\":\"rgb\",\"value\":\"" + rgb_str +"\"}";
                               uc_uart->sendData(status.c_str());
                               return true;
                           });

        

        ESP_LOGI(TAG, "MCP工具注册完成");
    }


};

static XieCoprocessor* g_xie_coprocessor = nullptr;

void InitializeXieCoprocessor() {
    if (g_xie_coprocessor == nullptr) {
        g_xie_coprocessor = new XieCoprocessor();
        ESP_LOGI(TAG, "协处理器已初始化并注册MCP工具");
    }
}

