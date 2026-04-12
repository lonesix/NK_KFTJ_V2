#include "sensor_manager.h"
#include "esp_log.h"
#include <stdio.h>
#include "cJSON.h"
#include <map>
#include <set>

static const char* TAG = "SensorMgr";

int SensorManager::findIndex(const std::string& name, const std::string& property) const {
    for (int i = 0; i < m_list.size(); i++) {
        if (m_list[i].name == name && m_list[i].property == property) {
            return i;
        }
    }
    return -1;
}
//只返回第一个
int SensorManager::findIndex( const std::string& property) const {
    for (int i = 0; i < m_list.size(); i++) {
        if (m_list[i].property == property) {
            return i;
        }
    }
    return -1;
}

void SensorManager::addOrUpdate(const std::string& name, const std::string& property, const std::string& value) {
    int idx = findIndex(name, property);
    if (idx >= 0) {
        m_list[idx].value = value;
        ESP_LOGI(TAG, "Updated: %s - %s = %s", name.c_str(), property.c_str(), value.c_str());
    } else {
        SensorData data;
        data.name = name;
        data.property = property;
        data.value = value;
        m_list.push_back(data);
        ESP_LOGI(TAG, "Added: %s - %s = %s", name.c_str(), property.c_str(), value.c_str());
    }
}

bool SensorManager::parseJson(const char* json) {
    cJSON* root = cJSON_Parse(json);
    if (!root) {
        ESP_LOGE(TAG, "JSON parse failed");
        return false;
    }

    cJSON* name = cJSON_GetObjectItemCaseSensitive(root, "name");
    cJSON* prop = cJSON_GetObjectItemCaseSensitive(root, "property");
    cJSON* val = cJSON_GetObjectItemCaseSensitive(root, "value");

    if (!cJSON_IsString(name) || !cJSON_IsString(prop) || !cJSON_IsString(val)) {
        ESP_LOGE(TAG, "JSON field error");
        if(delete_after_parse)cJSON_Delete(root);
        return false;
    }

    addOrUpdate(name->valuestring, prop->valuestring, val->valuestring);

    if(delete_after_parse)cJSON_Delete(root);
    return true;
}

bool SensorManager::parseJson(cJSON* root) {
    // cJSON* root = cJSON_Parse(json);
    if (!root) {
        ESP_LOGE(TAG, "JSON parse failed");
        return false;
    }

    cJSON* name = cJSON_GetObjectItemCaseSensitive(root, "name");
    cJSON* prop = cJSON_GetObjectItemCaseSensitive(root, "property");
    cJSON* val = cJSON_GetObjectItemCaseSensitive(root, "value");

    if (!cJSON_IsString(name) || !cJSON_IsString(prop) || !cJSON_IsString(val)) {
        ESP_LOGE(TAG, "JSON field error");
        if(delete_after_parse)cJSON_Delete(root);
        return false;
    }

    addOrUpdate(name->valuestring, prop->valuestring, val->valuestring);

    if(delete_after_parse)cJSON_Delete(root);
    return true;
}

std::string SensorManager::getValue(const std::string& name, const std::string& property) const {
    int idx = findIndex(name, property);
    if (idx >= 0) {
        return m_list[idx].value;
    }
    return "";
}

std::string SensorManager::getValue( const std::string& property) const {
    int idx = findIndex( property);
    if (idx >= 0) {
        return m_list[idx].value;
    }
    return "";
}

void SensorManager::printAll() const {
    printf("\n===== Sensor List =====\n");
    for (const auto& d : m_list) {
        printf("[%s] %s = %s\n", d.name.c_str(), d.property.c_str(), d.value.c_str());
    }
    printf("=======================\n");
}

std::string SensorManager::generateSensorJson(const std::string& mac) const {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "mac", mac.c_str());

    cJSON* sensorObj = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "sensor", sensorObj);

    // 按传感器名称分组
    std::map<std::string, std::map<std::string, std::string>> sensorMap;
    for (const auto& d : m_list) {
        sensorMap[d.name][d.property] = d.value;
    }

    // 组装每个传感器
    for (const auto& sensorItem : sensorMap) {
        const std::string& name = sensorItem.first;
        const auto& propMap = sensorItem.second;

        cJSON* dataArray = cJSON_CreateArray();
        cJSON* dataObj = cJSON_CreateObject();

        for (const auto& prop : propMap) {
            cJSON_AddStringToObject(dataObj, prop.first.c_str(), prop.second.c_str());
        }

        cJSON_AddItemToArray(dataArray, dataObj);
        cJSON_AddItemToObject(sensorObj, name.c_str(), dataArray);
    }

    char* jsonStr = cJSON_PrintUnformatted(root);
    std::string result = jsonStr;

    // 释放cJSON内存
    cJSON_Delete(root);
    cJSON_free(jsonStr);

    return result;
}