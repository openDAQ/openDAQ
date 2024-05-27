/*
 * Copyright 2022-2024 Blueberry d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#define _WIN32


using Daq.Core.Types;
using Daq.Core.Objects;


namespace Daq.Core.OpenDAQ;


/// <summary>Factory functions of the &apos;OpenDAQ&apos; library.</summary>
public static partial class OpenDAQFactory
{
    //void daqOpenDaqGetVersion(unsigned int* major, unsigned int* minor, unsigned int* revision); cdecl;
    [DllImport(OpenDAQDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void daqOpenDaqGetVersion(out int major, out int minor, out int revision);

    /// <summary>
    /// Gets the SDK version of the &apos;openDAQ&apos; library.
    /// </summary>
    public static Version SdkVersion
    {
        get
        {
            //get the SDK version
            daqOpenDaqGetVersion(out int major, out int minor, out int revision);

            //create and return .NET System object
            return new Version(major, minor, revision);
        }
    }


    #region LoggerSink

    /// <summary>Creates a Logger Sink object with Stderr as a target.</summary>
    /// <returns></returns>
    public static LoggerSink StdErrLoggerSink()
    {
        /*
            inline LoggerSinkPtr StdErrLoggerSink()
            {
                return LoggerSinkPtr(StdErrLoggerSink_Create());
            }
        */

        return OpenDAQFactory.CreateStdErrLoggerSink();
    }

    /// <summary>Creates a Logger Sink object with Stdout as a target.</summary>
    /// <returns></returns>
    public static LoggerSink StdOutLoggerSink()
    {
        /*
            inline LoggerSinkPtr StdOutLoggerSink()
            {
                return LoggerSinkPtr(StdOutLoggerSink_Create());
            }
        */

        return OpenDAQFactory.CreateStdOutLoggerSink();
    }

    /// <summary>Creates a Logger Sink object with rotating files as a target.</summary>
    /// <param name="fileName">The base name of the files.</param>
    /// <param name="maxFileSize">The maximum size of each file.</param>
    /// <param name="maxFiles">The maximum count of files.</param>
    /// <returns></returns>
    public static LoggerSink RotatingFileLoggerSink(string fileName,
                                                    nuint maxFileSize,
                                                    nuint maxFiles)
    {
        /*
            inline LoggerSinkPtr RotatingFileLoggerSink(const StringPtr& fileName, SizeT maxFileSize, SizeT maxFiles)
            {
                return LoggerSinkPtr(RotatingFileLoggerSink_Create(fileName, maxFileSize, maxFiles));
            }
        */

        return OpenDAQFactory.CreateRotatingFileLoggerSink(fileName, maxFileSize, maxFiles);
    }

    /// <summary>Creates a Logger Sink object with basic file as a target.</summary>
    /// <param name="fileName">The name of the file.</param>
    /// <returns></returns>
    public static LoggerSink BasicFileLoggerSink(string fileName)
    {
        /*
            inline LoggerSinkPtr BasicFileLoggerSink(const StringPtr& fileName)
            {
                return LoggerSinkPtr(BasicFileLoggerSink_Create(fileName));
            }
        */

        return OpenDAQFactory.CreateBasicFileLoggerSink(fileName);
    }

#if _WIN32
    /// <summary>Creates a Logger Sink object with WinDebug output as a target.</summary>
    /// <returns></returns>
    public static LoggerSink WinDebugLoggerSink()
    {
        /*
            inline LoggerSinkPtr WinDebugLoggerSink()
            {
                return LoggerSinkPtr(WinDebugLoggerSink_Create());
            }
        */

        return OpenDAQFactory.CreateWinDebugLoggerSink();
    }
#endif

    private static LogLevel getEnvLogLevel(string envStr,
                                           int defaultLevel)
    {
        /*
            inline LogLevel getEnvLogLevel(const std::string& envStr, int defaultLevel)
            {
                int level = defaultLevel;
                char* env = std::getenv(envStr.c_str());
                if (env != nullptr)
                {
                    try
                    {
                        level = std::stoi(env);
                        if (level < 0 || level >= OPENDAQ_LOG_LEVEL_DEFAULT)
                            level = defaultLevel;
                    }
                    catch (...)
                    {
                        level = defaultLevel;
                    }
                }

                return static_cast<LogLevel>(level);
            }
        */

        int level = defaultLevel;
        string env = Environment.GetEnvironmentVariable(envStr);
        if (env != null)
        {
            try
            {
                if (int.TryParse(env, out level))
                if ((level < 0) || (level >= (int)LogLevel.Default))
                    level = defaultLevel;
            }
            catch
            {
                level = defaultLevel;
            }
        }

        return (LogLevel)level;
    }

    private static void getEnvFileSinkLogLevelAndFileName(out LogLevel level,
                                                          out string fileName)
    {
        /*
            inline void getEnvFileSinkLogLevelAndFileName(LogLevel& level, std::string& fileName)
            {
                level = getEnvLogLevel("OPENDAQ_SINK_FILE_LOG_LEVEL", OPENDAQ_LOG_LEVEL_TRACE);
                char* env = std::getenv("OPENDAQ_SINK_FILE_FILE_NAME");
                if (env != nullptr)
                    fileName = env;
                else
                    fileName.clear();
            }
        */

        level = getEnvLogLevel("OPENDAQ_SINK_FILE_LOG_LEVEL", (int)LogLevel.Trace);
        string env = Environment.GetEnvironmentVariable("OPENDAQ_SINK_FILE_FILE_NAME");
        if (env != null)
            fileName = env;
        else
            fileName = null;
    }

    /// <summary>Creates a list of Sink objects.</summary>
    /// <param name="fileName">The base file name for rotating files sink.</param>
    /// <returns>The list of default Sink objects.</returns>
    /// <remarks>
    /// The rotating files sink is present in the created list if <paramref name="fileName"/> is provided,
    /// otherwise only Stdout and WinDebug sinks are present
    /// </remarks>
    public static IListObject<LoggerSink> DefaultSinks(string fileName = null)
    {
        /*
            inline ListPtr<ILoggerSink> DefaultSinks(const StringPtr& fileName = nullptr)
            {
                auto sinks = List<ILoggerSink>();

                auto consoleSinkLogLevel = getEnvLogLevel("OPENDAQ_SINK_CONSOLE_LOG_LEVEL", OPENDAQ_LOG_LEVEL_INFO);
                if (consoleSinkLogLevel != LogLevel::Off)
                {
                    auto consoleSink = StdOutLoggerSink();
                    consoleSink.setLevel(consoleSinkLogLevel);
                    sinks.pushBack(consoleSink);
                }

            #if defined(_WIN32)
                auto winDebugSinkLogLevel = getEnvLogLevel("OPENDAQ_SINK_WINDEBUG_LOG_LEVEL", OPENDAQ_LOG_LEVEL_INFO);
                if (winDebugSinkLogLevel != LogLevel::Off)
                {
                    auto winDebugSink = WinDebugLoggerSink();
                    winDebugSink.setLevel(winDebugSinkLogLevel);
                    sinks.pushBack(winDebugSink);
                }
            #endif

                std::string fileSinkFileName;
                LogLevel fileSinkLogLevel;
                getEnvFileSinkLogLevelAndFileName(fileSinkLogLevel, fileSinkFileName);
                if (fileSinkFileName.empty() && fileName.assigned())
                    fileSinkFileName = fileName.toStdString();

                if (!fileSinkFileName.empty())
                {
                    auto fileSink = RotatingFileLoggerSink(fileSinkFileName, 1048576, 5);
                    fileSink.setLevel(fileSinkLogLevel);
                    sinks->pushBack(fileSink);
                }

                return sinks;
            }
        */

        var sinks = CoreTypesFactory.CreateList<LoggerSink>();

        var consoleSinkLogLevel = getEnvLogLevel("OPENDAQ_SINK_CONSOLE_LOG_LEVEL", (int)LogLevel.Info);
        if (consoleSinkLogLevel != LogLevel.Off)
        {
            var consoleSink = StdOutLoggerSink();
            consoleSink.Level = consoleSinkLogLevel;
            sinks.Add(consoleSink);
        }

#if _WIN32
        var winDebugSinkLogLevel = getEnvLogLevel("OPENDAQ_SINK_WINDEBUG_LOG_LEVEL", (int)LogLevel.Info);
        if (winDebugSinkLogLevel != LogLevel.Off)
        {
            var winDebugSink = WinDebugLoggerSink();
            winDebugSink.Level = winDebugSinkLogLevel;
            sinks.Add(winDebugSink);
        }
#endif

        string fileSinkFileName;
        LogLevel fileSinkLogLevel;
        getEnvFileSinkLogLevelAndFileName(out fileSinkLogLevel, out fileSinkFileName);
        if (string.IsNullOrEmpty(fileSinkFileName) && !string.IsNullOrEmpty(fileName))
            fileSinkFileName = fileName;

        if (!string.IsNullOrEmpty(fileSinkFileName))
        {
            var fileSink = RotatingFileLoggerSink(fileSinkFileName, 1048576, 5);
            fileSink.Level = fileSinkLogLevel;
            sinks.Add(fileSink);
        }

        return sinks;
    }

    /*
        inline LogLevel LogLevelFromString(const StringPtr logLevelName)
        {
            std::string name = logLevelName.toStdString();
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);

            if (name == "trace")
                return LogLevel::Trace;
            if (name == "debug")
                return LogLevel::Debug;
            if (name == "info")
                return LogLevel::Info;
            if (name == "warn")
                return LogLevel::Warn;
            if (name == "error")
                return LogLevel::Error;
            if (name == "critical")
                return LogLevel::Critical;
            if (name == "off")
                return LogLevel::Off;

            return LogLevel::Trace;
        }
    */

    #endregion //LoggerSink .................................................................................

    /// <summary>Creates a Context with the given Scheduler, Logger, Property Object Class Manager, and Module Manager.</summary>
    /// <param name="scheduler">The scheduler the context has access to.</param>
    /// <param name="logger">The logger the context has access to.</param>
    /// <param name="typeManager">The type manager.</param>
    /// <param name="moduleManager">The module manager.</param>
    /// <param name="authenticationProvider">The authentication provider.</param>
    /// <param name="options">The options.</param>
    /// <returns>The Context instance.</returns>
    public static Context Context(Scheduler scheduler,
                                  Logger logger,
                                  TypeManager typeManager,
                                  ModuleManager moduleManager,
                                  AuthenticationProvider authenticationProvider,
                                  IDictObject<StringObject, BaseObject> options = null)
    {
        /*
            inline ContextPtr Context(const SchedulerPtr& scheduler,
                                      const LoggerPtr& logger,
                                      const TypeManagerPtr& typeManager,
                                      const ModuleManagerPtr& moduleManager,
                                      const DictPtr<IString, IBaseObject> options = Dict<IString, IBaseObject>())
            {
                ContextPtr obj(Context_Create(scheduler, logger, typeManager, moduleManager, options));
                return obj;
            }
        */

        if (options == null)
        {
            options = CoreTypesFactory.CreateDict<StringObject, BaseObject>();
        }

        return CreateContext(scheduler, logger, typeManager, moduleManager, authenticationProvider, options);
    }

    /// <summary>
    /// Creates a new Instance, using the default logger and module manager. The module manager
    /// searches for module shared libraries at the given module path, using the executable directory if left empty.
    /// </summary>
    /// <param name="modulePath">The module search path to be used by the Module manager.</param>
    /// <param name="localId">The local id of the instance.</param>
    /// <returns>The DAQ instance.</returns>
    /// <remarks>
    /// If localId is empty, the local id will be set to the OPENDAQ_INSTANCE_ID environment variable if available.
    /// Otherwise a random UUID will be generated for the local id.
    /// </remarks>
    public static Instance Instance(string modulePath = null,
                                    string localId = null)
    {
        /*
            inline InstancePtr Instance(const std::string& modulePath = "", const std::string& localId = "")
            {
                const auto logger = Logger();
                const auto scheduler = Scheduler(logger);
                const auto moduleManager = ModuleManager(modulePath);
                const auto typeManager = TypeManager();
                const auto context = Context(scheduler, logger, typeManager, moduleManager);

                StringPtr localIdStr;
                if (!localId.empty())
                    localIdStr = localId;

                InstancePtr obj(Instance_Create(context, localIdStr));
                return obj;
            }
        */

        using var logger        = Logger();
        using var scheduler     = Scheduler(logger, 0);
        using var moduleManager = ModuleManager(modulePath);
        using var typeManager   = TypeManager();
        using var authenticationProvider = AuthenticationProvider();
        using var context       = Context(scheduler, logger, typeManager, moduleManager, authenticationProvider);

        //instantiate default parameters if null
        using StringObject localIdStr = localId ?? string.Empty;

        return CreateInstance(context, localIdStr);
    }

    /// <summary>Creates a Logger object with a given log severity level and default set of sinks.</summary>
    /// <param name="fileName">The name used for the rotating files Sink. <see cref="RotatingFileLoggerSink"/>.</param>
    /// <param name="level">The default minimal severity level of the messages to be logged.</param>
    /// <returns>The Logger instance.</returns>
    public static Logger Logger(string fileName = null,
                                LogLevel level = LogLevel.Info)
    {
        /*
            inline LoggerPtr Logger(const StringPtr& fileName = nullptr, LogLevel level = LogLevel(OPENDAQ_LOG_LEVEL))
            {
                return LoggerPtr(Logger_Create(DefaultSinks(fileName), level));
            }
        */

        using var sinks = DefaultSinks(fileName);

        return CreateLogger(sinks, level);
    }

    /// <summary>Creates a Logger object with given sinks, and log severity level.</summary>
    /// <param name="sinks">The list of Sink objects. List members are of type &apos;ILoggerSink&apos;.</param>
    /// <param name="level">The default minimal severity level of the messages to be logged.</param>
    /// <returns>The Logger instance.</returns>
    public static Logger LoggerWithSinks(IListObject<LoggerSink> sinks,
                                         LogLevel level = LogLevel.Info)
    {
        /*
            inline LoggerPtr LoggerWithSinks(ListPtr<ILoggerSink>sinks, LogLevel level = LogLevel(OPENDAQ_LOG_LEVEL))
            {
                return LoggerPtr(Logger_Create(sinks, level));
            }
        */

        return CreateLogger(sinks, level);
    }

    /// <summary>
    /// Creates a ModuleManager that loads modules at a given search path. If the search path is empty,
    /// it searches the executable folder and its subfolders.Otherwise, it searches the for the relative directory
    /// based on the current working directory.
    /// </summary>
    /// <param name="searchPath">The location of the module libraries.</param>
    /// <returns>The ModuleManager instance.</returns>
    /// <remarks></remarks>
    public static ModuleManager ModuleManager(string searchPath)
    {
        /*
            inline ModuleManagerPtr ModuleManager(const StringPtr& searchPath)
            {
                ModuleManagerPtr obj(ModuleManager_Create(searchPath));
                return obj;
            }
        */

        using StringObject searchPathStr = searchPath ?? ".";

        return CreateModuleManager(searchPathStr);
    }

    /// <summary>Creates a new Type manager.</summary>
    /// <remarks>The Type manager should usually be created only as part of the Context creation.</remarks>
    /// <returns>The <see cref="TypeManager"/> instance.</returns>
    public static TypeManager TypeManager()
    {
        /*
            inline TypeManagerPtr TypeManager()
            {
                TypeManagerPtr obj(TypeManager_Create());
                return obj;
            }
        */

        return CoreTypesFactory.CreateTypeManager();
    }

    /// <summary>Creates an instance of a Scheduler with the specified amount of <paramref name="numWorkers"/> threads.</summary>
    /// <param name="logger">The logger instance.</param>
    /// <param name="numWorkers">The amount of worker threads. If <c>0</c> then maximum number of concurrent threads supported by the implementation is used.</param>
    /// <returns>A Scheduler instance with the specified amount of worker threads.</returns>
    public static Scheduler Scheduler(Logger logger,
                                      nuint numWorkers = 0)
    {
        /*
            inline SchedulerPtr Scheduler(LoggerPtr logger, SizeT numWorkers = 0)
            {
                SchedulerPtr obj(Scheduler_Create(logger, numWorkers));
                return obj;
            }
        */

        return CreateScheduler(logger, numWorkers);
    }

    #region AuthenticationProvider

    /// <summary>Creates a default authentication provider with only anonymous authentication allowed.</summary>
    /// <returns>The &apos;AuthenticationProvider&apos; object.</returns>
    public static AuthenticationProvider AuthenticationProvider()
    {
        return AuthenticationProvider(true);
    }

    /// <summary>Creates an empty authentication provider without any user.</summary>
    /// <returns>The &apos;AuthenticationProvider&apos; object.</returns>
    /// <param name="allowAnonymous">True if anonymous authentication is allowed.</param>
    public static AuthenticationProvider AuthenticationProvider(bool allowAnonymous)
    {
        using var userList = CoreTypesFactory.CreateList<BaseObject>();
        return CoreObjectsFactory.CreateStaticAuthenticationProvider(allowAnonymous, userList);
    }

    /// <summary>Creates an authentication provider out of static list of users.</summary>
    /// <returns>The &apos;AuthenticationProvider&apos; object.</returns>
    /// <param name="allowAnonymous">True if anonymous authentication is allowed.</param>
    /// <param name="userList">List of User objects.</param>
    public static AuthenticationProvider StaticAuthenticationProvider(bool allowAnonymous, IListObject<BaseObject> userList)
    {
        return CoreObjectsFactory.CreateStaticAuthenticationProvider(allowAnonymous, userList);
    }

    /// <summary>Creates an authentication provider out of json file.</summary>
    /// <returns>The &apos;AuthenticationProvider&apos; object.</returns>
    /// <param name="filename">File path to a json file containing a list of serialized User objects.</param>
    public static AuthenticationProvider JsonFileAuthenticationProvider(string filename)
    {
        return CoreObjectsFactory.CreateJsonFileAuthenticationProvider(filename);
    }

    /// <summary>Creates an authentication provider out of json string.</summary>
    /// <returns>The &apos;AuthenticationProvider&apos; object.</returns>
    /// <param name="jsonString">Json string containg a list of serialized User objects.</param>
    public static AuthenticationProvider JsonStringAuthenticationProvider(string jsonString)
    {
        return CoreObjectsFactory.CreateJsonStringAuthenticationProvider(jsonString);
    }

    #endregion AuthenticationProvider

    #region ConfigProvider

    /// <summary>
    /// Creates a Json configuration provider.
    /// </summary>
    /// <param name="filename">The filename.</param>
    /// <returns>The <see cref="ConfigProvider"/>.</returns>
    public static ConfigProvider JsonConfigProvider(string filename = null)
    {
        ConfigProvider obj = OpenDAQFactory.CreateJsonConfigProvider(filename);
        return obj;
    }

    /// <summary>
    /// Creates an environment configuration provider.
    /// </summary>
    /// <returns>The <see cref="ConfigProvider"/>.</returns>
    public static ConfigProvider EnvConfigProvider()
    {
        ConfigProvider obj = OpenDAQFactory.CreateEnvConfigProvider();
        return obj;
    }

    /// <summary>
    /// Creates a command-line argument configuration provider.
    /// </summary>
    /// <param name="args">The command-line arguments.</param>
    /// <returns>The <see cref="ConfigProvider"/>.</returns>
    public static ConfigProvider CmdLineArgsConfigProvider(IList<string> args)
    {
        using IListObject<BaseObject> cmdLineArgs = CoreTypesFactory.CreateList<BaseObject>();

        foreach (string arg in args)
            cmdLineArgs.Add(arg);

        ConfigProvider obj = OpenDAQFactory.CreateCmdLineArgsConfigProvider(cmdLineArgs);
        return obj;
    }

    #endregion ConfigProvider

    #region InstanceBuilder

    /// <summary>
    /// Creates a InstanceBuilder with no parameters configured.
    /// </summary>
    /// <returns>The <see cref="InstanceBuilder"/>.</returns>
    public static InstanceBuilder InstanceBuilder()
    {
        InstanceBuilder obj = OpenDAQFactory.CreateInstanceBuilder();
        return obj;
    }

    #endregion InstanceBuilder
}
