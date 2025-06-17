#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>
#include <fstream>
#include <sstream>

namespace MINEASMALM {

    /**
     * @brief 간단한 INI 파일 읽기 클래스
     *
     * INI 파일 형식의 설정 파일을 읽어서 키-값 쌍으로 제공합니다.
     * [Section] 형태의 섹션과 key=value 형태의 설정을 지원합니다.
     */
    class ConfigReader {
    public:
        /**
         * @brief INI 파일 로드
         * @param filename 읽을 INI 파일 경로
         * @return 성공 여부
         */
        bool LoadFromFile(const std::string& filename);

        /**
         * @brief 문자열 값 읽기
         * @param section 섹션 이름
         * @param key 키 이름
         * @param defaultValue 기본값
         * @return 설정값 또는 기본값
         */
        std::string GetString(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;

        /**
         * @brief 정수 값 읽기
         * @param section 섹션 이름
         * @param key 키 이름
         * @param defaultValue 기본값
         * @return 설정값 또는 기본값
         */
        int GetInt(const std::string& section, const std::string& key, int defaultValue = 0) const;

        /**
         * @brief 실수 값 읽기
         * @param section 섹션 이름
         * @param key 키 이름
         * @param defaultValue 기본값
         * @return 설정값 또는 기본값
         */
        double GetDouble(const std::string& section, const std::string& key, double defaultValue = 0.0) const;

        /**
         * @brief 불린 값 읽기
         * @param section 섹션 이름
         * @param key 키 이름
         * @param defaultValue 기본값
         * @return 설정값 또는 기본값
         */
        bool GetBool(const std::string& section, const std::string& key, bool defaultValue = false) const;

        /**
         * @brief 설정이 로드되었는지 확인
         */
        bool IsLoaded() const { return m_loaded; }

        /**
         * @brief 모든 섹션 이름 목록 반환
         * @return 섹션 이름 목록
         */
        std::vector<std::string> GetAllSections() const;

        /**
         * @brief 특정 섹션의 모든 키 목록 반환
         * @param section 섹션 이름
         * @return 키 이름 목록
         */
        std::vector<std::string> GetKeysInSection(const std::string& section) const;

    private:
        /**
         * @brief 문자열 앞뒤 공백 제거
         */
        std::string Trim(const std::string& str) const;

        /**
         * @brief 섹션.키 형태의 전체 키 생성
         */
        std::string MakeFullKey(const std::string& section, const std::string& key) const;

        std::map<std::string, std::string> m_config;  // section.key -> value 맵
        std::set<std::string> m_sections;  // 섹션 이름 목록
        bool m_loaded = false;
    };

} // namespace MINEASMALM
