# The MultiReader2 block base the example modules share; header-only, as a block may add interfaces (IRecorder)
add_library(reader_fb_base INTERFACE)
target_include_directories(reader_fb_base INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
