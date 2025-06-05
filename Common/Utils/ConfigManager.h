#pragma once

#include <map>
#include <string>
#include <memory>
#include <mutex>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace WeaponControl {

/**
 * @brief 설정 관리자 (싱글톤)
 * 
 * INI 파일 형식의 설정 파일을 읽고 쓰는 기능을 제공합니다.
 */
class ConfigManager {
public:
    // 싱글톤 인스턴스 획득
    static ConfigManager& GetInstance();
    
    // ==========================================================================
    // 설정 파일 로드/저장
    // ==========================================================================
    
    /**
     * @brief 설정 파일 로드
     * @param filename 설정 파일 경로
     * @return 성공 여부
     */
    bool LoadFromFile(const std::string& filename);
    
    /**
     * @brief 설정 파일 저장
     * @param filename 설정 파일 경로
     * @return 성공 여부
     */
    bool SaveToFile(const std::string& filename) const;
    
    /**
     * @brief 기본 설정 파일들 로드
     * @return 성공 여부
     */
    bool LoadConfigs();
    
    // ==========================================================================
    // 설정값 읽기 (타입별)
    // ==========================================================================
    
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const;
    int GetInt(const std::string& key, int defaultValue = 0) const;
    float GetFloat(const std::string& key, float defaultValue = 0.0f) const;
    double GetDouble(const std::string& key, double defaultValue = 0.0) const;
    bool GetBool(const std::string& key, bool defaultValue = false) const;
    
    // ==========================================================================
    // 설정값 쓰기
    // ==========================================================================
    
    void SetString(const std::string& key, const std::string& value);
    void SetInt(const std::string& key, int value);
    void SetFloat(const std::string& key, float value);
    void SetDouble(const std::string& key, double value);
    void SetBool(const std::string& key, bool value);
    
    // ==========================================================================
    // 유틸리티
    // ==========================================================================
    
    /**
     * @brief 설정이 로드되었는지 확인
     */
    bool IsLoaded() const { return m_loaded; }
    
    /**
     * @brief 모든 설정 출력 (디버그용)
     */
    void PrintAllSettings() const;
    
    /**
     * @brief 설정 초기화
     */
    void Clear();

private:
    ConfigManager() : m_loaded(false) {}
    ~ConfigManager() = default;
    
    // 복사 및 이동 금지
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) = delete;
    ConfigManager& operator=(ConfigManager&&) = delete;
    
    // ==========================================================================
    // 헬퍼 함수들
    // ==========================================================================
    
    std::string Trim(const std::string& str) const;
    void CreateDefaultConfig();
    
    // ==========================================================================
    // 멤버 변수
    // ==========================================================================
    
    std::map<std::string, std::string> m_config;
    mutable std::mutex m_configMutex;
    bool m_loaded;
};

} // namespace WeaponControl
