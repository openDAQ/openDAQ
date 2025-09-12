import opendaq

max_file_size = 100
max_no_of_files = 100

instanceBuilder = opendaq.InstanceBuilder()

simpleSink = opendaq.BasicFileLoggerSink("sink.txt") # Custom file path is the argument to the function

rotatingSink = opendaq.RotatingFileLoggerSink("rotatingSink.txt", max_file_size, max_no_of_files)

instanceBuilder.add_logger_sink(simpleSink)
instanceBuilder.add_logger_sink(rotatingSink)

# Sinks themselves can be also added to a custom logger if preferred

# This script will by default create sink files 

instance = instanceBuilder.build()
