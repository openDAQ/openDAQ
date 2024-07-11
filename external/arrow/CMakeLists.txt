opendaq_dependency(
    NAME                arrow
    REQUIRED_VERSION    17.0.0
    GIT_REPOSITORY      https://github.com/apache/arrow
    GIT_REF             6a2e19a
    GIT_SHALLOW         OFF
    EXPECT_TARGET       Arrow::arrow_static
    FETCH_LOG_LEVEL     WARNING
)
