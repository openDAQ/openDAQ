cmake_minimum_required(VERSION 3.20)

file(READ "opendaq_examples_version" examples_version)
string(STRIP ${examples_version} examples_version)

string(REGEX REPLACE "^([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" examples_version_major_minor_patch "${examples_version}")

set(OPENDAQ_EXAMPLES_VERSION "${examples_version_major_minor_patch}")

project(opendaq_examples VERSION ${OPENDAQ_EXAMPLES_VERSION})

add_compile_definitions(MODULE_PATH="${OPENDAQ_MODULES_DIR}")

option(OPENDAQ_CONSOLE_APP "Console application example." ON)
option(OPENDAQ_DEVICE "Device example without a discovery server." ON)
option(OPENDAQ_CLIENT_DISCOVERY "Client example that connects to all discoverable ${SDK_NAME} devices." ON)
option(OPENDAQ_CLIENT_LOCAL "Client example that connects to a ${SDK_NAME} device on localhost." ON)
option(OPENDAQ_CLIENT_AUTHENTICATION "Client example that connects to a ${SDK_NAME} device on localhost using username and password." ON)
option(OPENDAQ_QUICK_START_APP "Full example obtained by the end of the quick start guide." ON)
option(OPENDAQ_STREAM_READER "Stream reader example that loads a reference device and outputs data" ON)
option(OPENDAQ_MULTI_READER "Multi reader example that aligns multiple signals" ON)
option(OPENDAQ_TAIL_READER "Tail reader example that uses manually generated data." ON)
option(OPENDAQ_READER_BASICS "Reader basics example that uses manually generated data." ON)
option(OPENDAQ_READER_WITH_TIME_STAMPS "Time Reader example that uses manually generated data." ON)
option(OPENDAQ_EMPTY_EXAMPLE "Empty example source for user examples." ON)
option(OPENDAQ_FUNCTION_BLOCK "Function block example using an averager and renderer" ON)
option(OPENDAQ_CSV_WRITER "Example that connects to available devices, writing their data to a .csv file" ON)
option(OPENDAQ_STREAMING_CONFIG "Streaming configuration example that modifies the configuration of OpcUa enabled device" ON)
option(OPENDAQ_RECONNECTION "Example on how to detect connection loss and reconnection" ON)
option(OPENDAQ_IP_MODIFICATION "Example on how to modify IP configuration parameters of the openDAQ device" ON)
option(OPENDAQ_LICENSING "Licensing example." ON)
option(OPENDAQ_READ_WRITE_AUDIO "Example on how to read and write wav files with the audio module." ON)
option(OPENDAQ_LOGGER_USAGE "Example that show how to correctly use the logger and logging features in openDAQ" ON)

if (OPENDAQ_EMPTY_EXAMPLE)
  add_subdirectory(empty_example)
endif()

if (OPENDAQ_CLIENT_DISCOVERY)
  add_subdirectory(client_discovery)
endif()

if (OPENDAQ_CLIENT_LOCAL)
  add_subdirectory(client_local)
endif()

if (OPENDAQ_CLIENT_AUTHENTICATION)
  add_subdirectory(client_authentication)
endif()

if (OPENDAQ_DEVICE)
  add_subdirectory(device_server)
endif()

if (OPENDAQ_QUICK_START_APP)
  add_subdirectory(quick_start)
endif()

if (OPENDAQ_STREAM_READER)
  add_subdirectory(stream_reader_example)
endif()

if (OPENDAQ_MULTI_READER)
  add_subdirectory(multi_reader_example)
  add_subdirectory(multi_reader_howto_example)
endif()

if (OPENDAQ_TAIL_READER)
  add_subdirectory(tail_reader_example)
endif()

if (OPENDAQ_READER_BASICS)
  add_subdirectory(reader_basics_example)
endif()

if (OPENDAQ_READER_TIMEOUTS)
  add_subdirectory(reader_timeouts_example)
endif()

if (OPENDAQ_READER_WITH_TIME_STAMPS)
  add_subdirectory(reader_with_time_stamps)
endif()

if (OPENDAQ_FUNCTION_BLOCK)
  add_subdirectory(function_block_example)
endif()

if (OPENDAQ_CSV_WRITER)
  add_subdirectory(csv_writer)
endif()

if (OPENDAQ_STREAMING_CONFIG)
  add_subdirectory(streaming_configuration_example)
endif()

if (OPENDAQ_RECONNECTION)
  add_subdirectory(reconnection_example)
endif()

if (OPENDAQ_IP_MODIFICATION)
  add_subdirectory(ip_modification)
endif()

if(OPENDAQ_LICENSING AND DAQMODULES_LICENSING_MODULE)
	set(LICENSING_EXAMPLE_REQUIREMENTS_MET ON)
	if(LINUX)
		find_package(PkgConfig QUIET)
		if(NOT PKGCONFIG_FOUND)
			message("Cannot build licensing example, PkgConfig not found!")
			set(LICENSING_EXAMPLE_REQUIREMENTS_MET OFF)
		else()
			pkg_check_modules(GPGME QUIET gpgme)
			if(NOT GPGME_FOUND)
				message("Cannot build licensing example, GPGme not found!")
				set(LICENSING_EXAMPLE_REQUIREMENTS_MET OFF)
			endif()
		endif()
	endif()
	if(LICENSING_EXAMPLE_REQUIREMENTS_MET)
		add_subdirectory(licensing_example)
	endif()
endif()

if (OPENDAQ_READ_WRITE_AUDIO)
  add_subdirectory(read_write_audio_file_example)
endif()

if(OPENDAQ_LOGGER_USAGE)
  add_subdirectory(logger_example)
endif()
  
if (OPENDAQ_READ_WRITE_AUDIO)
  add_subdirectory(read_write_audio_file_example)
endif()

