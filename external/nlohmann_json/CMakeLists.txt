set(JSON_Install ON)

opendaq_dependency(
    NAME                nlohmann_json
    REQUIRED_VERSION    3.11.3
    GIT_REPOSITORY      https://github.com/nlohmann/json.git
    GIT_REF             v3.11.3
    EXPECT_TARGET       nlohmann_json::nlohmann_json
)
