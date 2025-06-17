#include "ConfigReader.h"
#include <iostream>
#include <algorithm>

namespace MINEASMALM {

    bool ConfigReader::LoadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << filename << std::endl;
            return false;
        }

        m_config.clear();
        m_sections.clear();
        std::string line;
        std::string currentSection;

        while (std::getline(file, line)) {
            line = Trim(line);

            // 빈 줄이나 주석 무시
            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }

            // 섹션 처리 [SectionName]
            if (line[0] == '[' && line.back() == ']') {
                currentSection = line.substr(1, line.length() - 2);
                currentSection = Trim(currentSection);
                m_sections.insert(currentSection);  // 섹션 목록에 추가
                continue;
            }

            // 키=값 처리
            size_t equalPos = line.find('=');
            if (equalPos != std::string::npos) {
                std::string key = Trim(line.substr(0, equalPos));
                std::string value = Trim(line.substr(equalPos + 1));

                // 값에서 따옴표 제거
                if (value.length() >= 2 && value[0] == '"' && value.back() == '"') {
                    value = value.substr(1, value.length() - 2);
                }

                std::string fullKey = MakeFullKey(currentSection, key);
                m_config[fullKey] = value;
            }
        }

        file.close();
        m_loaded = true;

        std::cout << "Configuration loaded from: " << filename << std::endl;
        std::cout << "Total settings loaded: " << m_config.size() << std::endl;
        std::cout << "Sections found: " << m_sections.size() << std::endl;

        return true;
    }

    std::string ConfigReader::GetString(const std::string& section, const std::string& key, const std::string& defaultValue) const {
        std::string fullKey = MakeFullKey(section, key);
        auto it = m_config.find(fullKey);
        return (it != m_config.end()) ? it->second : defaultValue;
    }

    int ConfigReader::GetInt(const std::string& section, const std::string& key, int defaultValue) const {
        std::string value = GetString(section, key, "");
        if (value.empty()) {
            return defaultValue;
        }

        try {
            return std::stoi(value);
        }
        catch (const std::exception&) {
            std::cerr << "Invalid integer value for [" << section << "]." << key << ": " << value << std::endl;
            return defaultValue;
        }
    }

    double ConfigReader::GetDouble(const std::string& section, const std::string& key, double defaultValue) const {
        std::string value = GetString(section, key, "");
        if (value.empty()) {
            return defaultValue;
        }

        try {
            return std::stod(value);
        }
        catch (const std::exception&) {
            std::cerr << "Invalid double value for [" << section << "]." << key << ": " << value << std::endl;
            return defaultValue;
        }
    }

    bool ConfigReader::GetBool(const std::string& section, const std::string& key, bool defaultValue) const {
        std::string value = GetString(section, key, "");
        if (value.empty()) {
            return defaultValue;
        }

        // 대소문자 구분 없이 비교
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);

        if (value == "true" || value == "1" || value == "yes" || value == "on") {
            return true;
        }
        else if (value == "false" || value == "0" || value == "no" || value == "off") {
            return false;
        }

        std::cerr << "Invalid boolean value for [" << section << "]." << key << ": " << value << std::endl;
        return defaultValue;
    }

    std::string ConfigReader::Trim(const std::string& str) const {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return "";
        }

        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    std::string ConfigReader::MakeFullKey(const std::string& section, const std::string& key) const {
        return section + "." + key;
    }

    std::vector<std::string> ConfigReader::GetAllSections() const {
        std::vector<std::string> sections;
        for (const auto& section : m_sections) {
            sections.push_back(section);
        }
        return sections;
    }

    std::vector<std::string> ConfigReader::GetKeysInSection(const std::string& section) const {
        std::vector<std::string> keys;
        std::string sectionPrefix = section + ".";

        for (const auto& pair : m_config) {
            if (pair.first.substr(0, sectionPrefix.length()) == sectionPrefix) {
                std::string key = pair.first.substr(sectionPrefix.length());
                keys.push_back(key);
            }
        }

        return keys;
    }

} // namespace MINEASMALM
