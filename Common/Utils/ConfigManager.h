#include <string>
#include <mutex>

namespace MINEASMALM {

    /**
     * @brief 시스템 인프라 설정 (main에서 주로 사용)
     */
    struct SystemInfraConfig {
        int totalTubes;                     // 전체 발사관 개수        
        int ddsDomainId;                    // DDS 설정

        SystemInfraConfig()
            : totalTubes(6)
            , ddsDomainId(83)        {}
    };

    /**
     * @brief 비즈니스 로직 설정 (Factory에서 객체 생성 시 사용)
     */
    struct BusinessLogicConfig {
        // 업데이트 주기들
        double engagementPlanUpdateInterval_sec;     // 교전계획 업데이트 주기
        double weaponStatusUpdateInterval_sec;       // 무장상태 업데이트 주기

        BusinessLogicConfig()
            : engagementPlanUpdateInterval_sec(1.0)
            , weaponStatusUpdateInterval_sec(1.0)        {}
    };

    /**
     * @brief 무장 제원 정보
     */
    struct WeaponSpecification {
        std::string name;
        double maxRange_km;
        double maxSpeed_mps;
        double cruiseSpeed_mps;
        double launchDelay_sec;
        double maxDepth_m;
        double maxAltitude_m;
        int maxWaypoints;
        bool requiresWaypoints;
        std::string description;

        WeaponSpecification()
            : name("Unknown"), maxRange_km(0.0), maxSpeed_mps(0.0), cruiseSpeed_mps(0.0), launchDelay_sec(3.0)
            , launchDelay_sec(0.0), maxDepth_m(0.0), maxAltitude_m(0.0)
            , maxWaypoints(0), requiresWaypoints(false), description("") {}
    };

    /**
     * @brief 통합 설정 관리자 (실용적 접근법)
     *
     * 하나의 설정 파일에서 모든 설정을 로드하되,
     * 용도별로 구조화하여 제공합니다.
     */
    class ConfigManager {
    public:
        static ConfigManager& GetInstance();

        /**
         * @brief 설정 파일 로드
         */
        bool LoadFromFile(const std::string& configFilePath = "config.ini");

        /**
         * @brief 시스템 인프라 설정 조회 (main에서 사용)
         */
        const SystemInfraConfig& GetSystemInfraConfig() const;

        /**
         * @brief 비즈니스 로직 설정 조회 (Factory에서 사용)
         */
        const BusinessLogicConfig& GetBusinessLogicConfig() const;

        /**
         * @brief 무장 제원 조회 (Factory에서 사용)
         */
        const WeaponSpecification& GetWeaponSpec(EN_WPN_KIND weaponKind) const;

        /**
         * @brief 지원되는 무장 종류 목록
         */
        std::vector<EN_WPN_KIND> GetSupportedWeaponKinds() const;

        /**
         * @brief 무장 지원 여부 확인
         */
        bool IsWeaponSupported(EN_WPN_KIND weaponKind) const;

        /**
         * @brief 무장 이름 조회
         */
        std::string GetWeaponName(EN_WPN_KIND weaponKind) const;

        /**
         * @brief 설정 로드 여부 확인
         */
        bool IsLoaded() const { return m_loaded; }

        /**
         * @brief 설정 정보 출력
         */
        void PrintAllConfigs() const;

    private:
        ConfigManager() = default;
        ~ConfigManager() = default;

        ConfigManager(const ConfigManager&) = delete;
        ConfigManager& operator=(const ConfigManager&) = delete;

        void LoadSystemInfraConfig(const ConfigReader& config);
        void LoadBusinessLogicConfig(const ConfigReader& config);
        void LoadWeaponSpecs(const ConfigReader& config);

        std::string WeaponKindToString(EN_WPN_KIND kind) const;
        EN_WPN_KIND StringToWeaponKind(const std::string& str) const;

        SystemInfraConfig m_systemInfraConfig;
        BusinessLogicConfig m_businessLogicConfig;
        std::map<EN_WPN_KIND, WeaponSpecification> m_weaponSpecs;

        mutable std::mutex m_mutex;
        bool m_loaded = false;

        static const WeaponSpecification s_defaultWeaponSpec;
    };
} // namespace MINEASMALM
