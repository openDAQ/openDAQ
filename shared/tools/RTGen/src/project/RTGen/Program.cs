using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;

using Mono.Options;
using RTGen.Exceptions;
using RTGen.Interfaces;
using RTGen.Types;
using RTGen.Util;

namespace RTGen
{
    enum ExitCode
    {
        Success,
        OptionsError,
        PluginsError,
        ParserError,
        GeneratorError,
        RtGenException,
        UnknownException
    }

    static class Program
    {
        static int Main(string[] args)
        {
#if DEBUG
            Debug.Listeners.Add(new TextWriterTraceListener(Console.Out));
#endif

            if (!ParseOptions(args, out ProgramOptions options))
            {
                return (int) ExitCode.OptionsError;
            }

            if (options.GenerateGuid)
            {
                return (int) ExitCode.Success;
            }

            if (options.Verbose)
            {
                Log.Verbose = true;
            }

            Plugins plugins = new Plugins();
            if (!plugins.DiscoverAndLoad(options))
            {
                return (int) ExitCode.PluginsError;
            }

            if (options.PrintVersion)
            {
                return (int) plugins.PrintAllVersionInfo();
            }

            ExitCode exitCode = options.GenerateConfig
                          ? GenerateConfigFile(plugins.GetConfigGenerator(options), options)
                          : GenerateFile(plugins, options);

#if DEBUG
            bool warnings = Log.HasWarnings;
#endif
            return (int) exitCode;
        }

        private static ExitCode GenerateConfigFile(IConfigGenerator generator, ProgramOptions options)
        {
            string templatePath = Utility.GetTemplate(options.TemplatePath);

            try
            {
                if (Log.Verbose)
                {
                    Log.Info($"Generating {options.Language} config file for: {options.LibraryInfo.OutputName}");
                }
                generator.GenerateConfigFile(templatePath);
            }
            catch (RTGenException e)
            {
                Log.Warning("Error occurred while generating the config file.");

                if (Log.Verbose)
                {
                    Log.Error(e.Message);
                }

                if (Debugger.IsAttached)
                {
                    throw;
                }

                return ExitCode.RtGenException;
            }
            catch (Exception e)
            {
                Log.Error("Unknown exception occurred: " + e.Message);
                if (Log.Verbose)
                {
                    Log.Warning(e.StackTrace);
                }

                if (Debugger.IsAttached)
                {
                    throw;
                }

                return ExitCode.UnknownException;
            }

            return ExitCode.Success;
        }

        private static ExitCode GenerateFile(Plugins plugins, ProgramOptions options)
        {
            IParser parser = plugins.GetParser(options);
            if (parser == null)
            {
                Log.Error($"Could not find a parser for input language: {options.InputLanguage}");

                Log.Info("Known parsers:");
                plugins.LogParsers();

                return ExitCode.ParserError;
            }

            ExitCode retCode = ParseInputs(parser, options, out var interfaceFile);
            if (retCode != ExitCode.Success)
            {
                return retCode;
            }

            if (parser.FileType == ParsedFile.Interface && interfaceFile.Classes.Count == 0)
            {
                Log.Error("No classes/interfaces found in the input file.");
                return ExitCode.ParserError;
            }

            ExitCode exitCode = GenerateOutputs(plugins.GetGenerator(interfaceFile, options), options);
            if (options.Verbose)
            {
                string serialized = Json.Serialize(interfaceFile);
                File.WriteAllText(options.Language + ".json", serialized);
            }

            return exitCode;
        }

        private static ExitCode ParseInputs(IParser parser, ProgramOptions options, out IRTFile rtFile)
        {
            try
            {
                if (Log.Verbose)
                {
                    Log.Info($"  parsing {Path.GetFullPath(options.InputFile)}");
                }
                rtFile = parser.Parse(options.InputFile, options);
                rtFile.SourceFileName = Path.GetFileName(options.InputFile) ?? "";

                ChangePredefinedTypesAttributeInfo(options.PredefinedTypeArguments, rtFile.AttributeInfo);
            }
            catch (RTGenException e)
            {
                rtFile = null;

                Log.Error(e.Message);
                if (Log.Verbose)
                {
                    Log.Warning(e.StackTrace);
                }

                if (Debugger.IsAttached)
                {
                    throw;
                }

                return e is ParserException
                    ? ExitCode.ParserError
                    : ExitCode.RtGenException;
            }
            catch (Exception e)
            {
                rtFile = null;

                Log.Error($"Unknown exception occurred: {e.Message}. ({e.Source})");

                if (Log.Verbose)
                {
                    Log.Warning(e.StackTrace);
                }

                if (Debugger.IsAttached)
                {
                    throw;
                }
                return ExitCode.UnknownException;
            }

            return ExitCode.Success;
        }

        private static void ChangePredefinedTypesAttributeInfo(IDictionary<string, ISampleTypes> predefinedTypeArguments, IAttributeInfo info)
        {
            foreach (var (_, types) in predefinedTypeArguments)
            {
                foreach (ITypeName sampleType in types.Types.SelectMany(t => t))
                {
                    ((PredefinedTypeName)sampleType).SetAttributeInfo(info);
                }
            }
        }

        private static ExitCode GenerateOutputs(IGenerator generator, ProgramOptions options)
        {
            string templatePath = Utility.GetTemplate(options.TemplatePath);

            try
            {
                if (!options.GenerateConfig)
                {
                    Dictionary<string, string> typeMappings = new Dictionary<string, string>();
                    generator.RegisterTypeMappings(typeMappings);

                    foreach (KeyValuePair<string, string> mapping in typeMappings)
                    {
                        generator.RtFile.AttributeInfo.TypeMappings.Add(mapping.Key, mapping.Value);
                    }
                }

                generator.GenerateFile(templatePath);
            }
            catch (GeneratorException e)
            {
                Log.Warning($"Error occurred while generating the output file: {e.Message}");
                if (Debugger.IsAttached)
                {
                    throw;
                }

                return ExitCode.GeneratorError;
            }
            catch (RTGenException e)
            {
                Log.Warning("Error occurred while generating the output file.");

                if (Log.Verbose)
                {
                    Log.Error(e.Message);
                }

                if (Debugger.IsAttached)
                {
                    throw;
                }

                return ExitCode.RtGenException;
            }
            catch (Exception e)
            {
                Log.Error("Unknown exception occurred: " + e.Message);
                if (Log.Verbose)
                {
                    Log.Warning(e.StackTrace);
                }

                if (Debugger.IsAttached)
                {
                    throw;
                }

                return ExitCode.UnknownException;
            }

            return ExitCode.Success;
        }

        private static void ParsePredefinedTypes(string sampleTypesFile, ProgramOptions opts)
        {
            if (File.Exists(sampleTypesFile))
            {
                try
                {
                    string json = File.ReadAllText(sampleTypesFile);
                    opts.PredefinedTypeArguments = Json.Deserialize<Dictionary<string, ISampleTypes>>(json);
                }
                catch (Exception e)
                {
                    throw new RTGenException("The predefined type arguments failed to parse.", e);
                }
            }
            else
            {
                throw new RTGenException($"The predefined type arguments file does not exists: \"{sampleTypesFile}\"");
            }
        }

        private static bool ParseOptions(string[] args, out ProgramOptions programOptions)
        {
            ProgramOptions opts = new ProgramOptions();

            var options = new OptionSet
            {
                {"v|verbose", "Show verbose warnings and errors",
                    v =>
                    {
                        opts.Verbose = !string.IsNullOrEmpty(v);
                    }
                },
                {"c|continueOnErrors", "Continue when parsing errors occur.",
                    (bool c) => opts.ContinueOnParseErrors = c
                },
                {"l|lang|language=", "The language use for generating.",
                    lang =>
                    {
                        opts.TemplatePath = $"{lang}.template";
                        opts.Language = lang;
                    }
                },
                {"version", "Print version information for all components.", ver => opts.PrintVersion = true },
                {"config", "Generate config file only.", config => opts.GenerateConfig = config != null },
                {"lv|lib-version=", "Library semantic version info.", version => opts.LibraryInfo.Version = Utility.ParseVersionInfo(version) },
                {"ln|lib|library=", "Library name.", lib => opts.LibraryInfo.Name = lib },
                {"ns|namespace=", "Library namespace.", ns => opts.LibraryInfo.Namespace = new Namespace(ns) },
                {"cns|core-namespace=", "Core namespace override.", ns => opts.CoreNamespace = new Namespace(ns) },
                {"nt|no-timestamp", "Date & time provided to the generators is fixed", nt =>
                    {
                        opts.UseDebugTimeStamp = nt != null;
                    }
                },
                {"lo|lib-output=", "Library output filename.", name => opts.LibraryInfo.OutputName = name },
                {"s|src|source=", "Input file to parse", input => opts.InputFile = input},
                {"d|dir|outputDir=", "Where to put the generated files", dir => opts.OutputDir = dir},
                {"f|def|outputSourceDir=", outputDef => opts.OutputSourceDir = outputDef },
                {"p|impl", "Generate smart pointer implementation", impl => opts.GenerateWrapper = !string.IsNullOrEmpty(impl) },
                {"i|inlang|inputLanguage=", "The language of the input file.", inLang => opts.InputLanguage = inLang},
                {"o|out|output=", "Generated output file name", o => opts.Filename = o},
                {"t|template=", "Template file to use for code generation", template => opts.TemplatePath = template },
                {"e|ext|extension=", "Generated file extension", ext => opts.GeneratedExtension = ext},
                {"x|suffix", "Generated filename suffix", suffix => opts.FileNameSuffix = suffix},
                {"m|macros=", "Json file describing predefined type arguments", m => ParsePredefinedTypes(m, opts) },
                {"g|guid", "Generate UUID v5 for the specified interface name", g => opts.GenerateGuid = !string.IsNullOrEmpty(g) },
                {"rt", "Use RT Core namespace for generating the interface UUID.",
                    g =>
                    {
                        opts.RtGuid = !string.IsNullOrEmpty(g);
                        if (opts.RtGuid)
                        {
                            opts.GenerateGuid = true;
                        }
                    }
                },
                {"h|?|help", "Shows this help message", h => opts.ShowHelp = h != null},
            };

            try
            {
                List<string> extra = options.Parse(args);

                if (opts.GenerateGuid)
                {
                    bool result = GenerateGuid(opts, extra);
                    programOptions = opts;
                    return result;
                }

                // if no explicit sources specified parse them from extras
                if (string.IsNullOrEmpty(opts.InputFile) && extra.Count > 0)
                {
                    foreach (string candidateFile in extra)
                    {
                        if (File.Exists(candidateFile))
                        {
                            opts.InputFile = candidateFile;
                        }
                        else
                        {
                            Log.Warning($"Could not find file: \"{candidateFile}\".");
                        }
                    }
                }
            }
            catch (OptionException e)
            {
                Log.Error(e.Message);
                Log.Warning("Use --help for more information and a list of options");

                programOptions = null;
                return false;
            }

            if (opts.PrintVersion)
            {
                programOptions = opts;
                return true;
            }

            if (string.IsNullOrEmpty(opts.InputLanguage))
            {
                opts.InputLanguage = "cpp";
            }

            if (opts.ShowHelp)
            {
                Console.WriteLine();
                Console.WriteLine("Options:");
                options.WriteOptionDescriptions(Console.Out);

                programOptions = null;
                return false;
            }

            if (opts.GenerateConfig && (string.IsNullOrEmpty(opts.TemplatePath) || Path.GetFileName(opts.TemplatePath) == $"{opts.Language}.template" ))
            {
                opts.TemplatePath = $"{opts.Language}.config.template";
            }

            if (string.IsNullOrEmpty(opts.TemplatePath))
            {
                Log.Error("No template file specified!");
                Log.Warning("Use --help to show more option information.");

                programOptions = null;
                return false;
            }

            if (!opts.GenerateConfig && string.IsNullOrEmpty(opts.InputFile))
            {
                Log.Error("No input file specified!");
                Log.Warning("Use --help to show more option information.");

                programOptions = null;
                return false;
            }

            if (string.IsNullOrEmpty(opts.OutputDir))
            {
                if (opts.GenerateConfig)
                {
                    Log.Error("Output directory must be specified when generating a config file!");
                    programOptions = null;
                    return false;
                }
                else
                {
                    opts.OutputDir = Path.GetDirectoryName(Path.GetFullPath(opts.InputFile)) ?? "";
                }
            }

            programOptions = opts;
            return true;
        }

        private static bool GenerateGuid(ProgramOptions opts, List<string> extra)
        {
            if (extra.Count == 0)
            {
                Log.Error("Must specify at least one interface name.");
                return false;
            }

            foreach (string interfaceName in extra)
            {
                string interfaceFullName = opts.RtGuid
                    ? (interfaceName + ".Core.RT.Dewesoft")
                    : interfaceName;

                Guid uuid = Utility.InterfaceUuid(interfaceFullName);

                Console.WriteLine(interfaceFullName + ":");
                Console.WriteLine("\t" + uuid.ToString("D").ToUpperInvariant());
                Console.WriteLine("\t" + uuid.ToString("X").ToUpperInvariant().Replace('X', 'x'));
            }

            return true;
        }
    }
}
