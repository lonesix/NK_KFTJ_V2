#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <string>
#include <vector>
#include "cJSON.h"

// 单条传感器数据结构
struct SensorData {
    std::string name;
    std::string property;
    std::string value;
};

class SensorManager {
private:
    std::vector<SensorData> m_list;
    int findIndex(const std::string& name, const std::string& property) const;
    int findIndex(const std::string& property) const;
    bool delete_after_parse = false; // 是否解析后删除
    
public:
    void enableDeleteRoot(bool flag) { delete_after_parse = flag; }
    // 新增或更新数据
    void addOrUpdate(const std::string& name, const std::string& property, const std::string& value);

    // 解析JSON并更新
    bool parseJson(const char* json);
    bool parseJson(cJSON* root);
    // 查询数值
    std::string getValue(const std::string& name, const std::string& property) const;
    std::string getValue( const std::string& property) const;
    // 打印所有数据
    void printAll() const;
    std::string generateSensorJson(const std::string& mac) const; // 生成传感器JSON
};

#endif // SENSOR_MANAGER_H