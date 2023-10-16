using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using LightInject;
using RTGen.Exceptions;
using RTGen.Interfaces;
using RTGen.Types;

namespace RTGen.Util
{
    class Plugins
    {
        private static readonly string RTLibName = "RTGen.Library";
        private static readonly AssemblyName RTLibAssemblyName = new AssemblyName {CodeBase = RTLibName + ".dll"};
        private static readonly Type ParserType = typeof(IParser);
        private static readonly Type GeneratorType = typeof(IGenerator);
        private static readonly Type ConfigGeneratorType = typeof(IConfigGenerator);

        private readonly ServiceContainer _container;

        public Plugins()
        {
            _container = new ServiceContainer(
                new ContainerOptions {
                    EnablePropertyInjection = false
                }
            );
        }

        ~Plugins()
        {
            _container.Dispose();
        }

        private void RegisterRtLib()
        {
            Assembly rtLibAssembly = null;
            foreach (Assembly assembly in AppDomain.CurrentDomain.GetAssemblies())
            {
                if (assembly.GetName().Name == RTLibName)
                {
                    rtLibAssembly = assembly;
                }
            }

            if (rtLibAssembly == null)
            {
                throw new RTGenException("Could not access RTGen Library. No plugins will be loaded.");
            }

            _container.RegisterAssembly(rtLibAssembly);
        }

        public bool DiscoverAndLoad(ProgramOptions options)
        {
            const string parserExt = ".parser";
            const string generatorExt = ".generator";
            const string pluginExt = ".plugin";
            const string dllExt = ".dll";

            RegisterRtLib();

            List<string> pluginCandidates = new List<string>();

            IEnumerable<string> files = null;
            try
            {
                string location = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location) ?? Directory.GetCurrentDirectory();
                files = Directory.EnumerateFiles(location);
            }
            catch (Exception e)
            {
                Log.Warning("Could not access directory info. No plugins will be loaded.");

                if (options.Verbose)
                {
                    Log.Warning(e.Message);
                }
            }

            if (files == null)
            {
                return false;
            }

            foreach (string file in files)
            {
                string fileName = Path.GetFileName(file);

                // Don't load the RTGen Library or LightInject twice (can result in some wierd behaviour and errors)
                if (fileName == RTLibAssemblyName.CodeBase || fileName == "LightInject.dll")
                {
                    continue;
                }

                string extension = Path.GetExtension(file);

                // Check for valid extension
                if (extension == parserExt || extension == generatorExt || extension == pluginExt || extension == dllExt)
                {
                    pluginCandidates.Add(file);
                }
            }

            List<string> validAssemblies = GetValidAssemblies(pluginCandidates);
            foreach (string plugin in validAssemblies)
            {
                RegisterAssembly(plugin, options.Verbose);
            }

            return true;
        }

        private void RegisterAssembly(string plugin, bool verbose)
        {
            try
            {
                Assembly asm = Assembly.LoadFile(plugin);
                _container.RegisterAssembly(asm);
            }
            catch (Exception e)
            {
                Log.Warning($"Error while loading plugin \"{Path.GetFileName(plugin)}\".");

                if (verbose)
                {
                    Log.Warning(e.Message);
                }
            }
        }

        private List<string> GetValidAssemblies(List<string> pluginCandidates)
        {
            List<string> validAssemblies = new List<string>();

            AppDomain plugins = AppDomain.CreateDomain("plugins");
            {
                foreach (string candidate in pluginCandidates)
                {
                    if (AssemblyEx.CheckIsPlugin(plugins, candidate))
                    {
                        validAssemblies.Add(candidate);
                    }
                }
            }
            AppDomain.Unload(plugins);
            return validAssemblies;
        }

        public IParser GetParser(IParserOptions options)
        {
            return _container.CanGetInstance(ParserType, options.InputLanguage)
                ? _container.GetInstance<IParser>(options.InputLanguage)
                : null;
        }

        public IConfigGenerator GetConfigGenerator(IGeneratorOptions options)
        {
            IConfigGenerator generator = _container.GetInstance<IConfigGenerator>(options.Language);
            generator.Options = options;

            return generator;
        }

        public IGenerator GetGenerator(IRTFile file, IGeneratorOptions options)
        {
            IGenerator generator = _container.GetInstance<IGenerator>(options.Language);
            generator.Options = options;
            generator.RtFile = file;

            return generator;
        }

        public ExitCode PrintAllVersionInfo()
        {
            foreach (ServiceRegistration sr in _container.AvailableServices)
            {
                string dllName = Log.Verbose
                                     ? sr.ImplementingType.Assembly.Location
                                     : sr.ImplementingType.Assembly.ManifestModule.Name;

                string versionInfo = null;

                if (sr.ServiceType == GeneratorType)
                {
                    IGenerator parser = (IGenerator)sr.ImplementingType.GetConstructor(new Type[]{})?.Invoke(null);
                    versionInfo = PrintVersionInfo(parser?.Version);
                }
                else if (sr.ServiceType == ConfigGeneratorType)
                {
                    IConfigGenerator parser = (IConfigGenerator)sr.ImplementingType.GetConstructor(new Type[]{})?.Invoke(null);
                    versionInfo = PrintVersionInfo(parser?.Version);
                }

                Log.Info($"{sr.ServiceName}: {sr.ImplementingType.FullName}{versionInfo} [{dllName}]");
            }

            return ExitCode.Success;
        }

        private string PrintVersionInfo(IVersionInfo versionInfo)
        {
            if (versionInfo == null)
            {
                return "";
            }
            return $" v{versionInfo.Major}.{versionInfo.Minor}.{versionInfo.Patch}";
        }

        public void LogParsers()
        {
            foreach (ServiceRegistration sr in _container.AvailableServices)
            {
                if (sr.ServiceType == ParserType)
                {
                    string dllName = Log.Verbose
                                         ? sr.ImplementingType.Assembly.Location
                                         : sr.ImplementingType.Assembly.ManifestModule.Name;

                    Log.Info($"{sr.ServiceName}: {sr.ImplementingType.FullName} [{dllName}]");
                }
            }
        }
    }
}
