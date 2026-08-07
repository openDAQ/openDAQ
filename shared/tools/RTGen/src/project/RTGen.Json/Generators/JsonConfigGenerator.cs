using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using RTGen.Cpp;
using RTGen.Interfaces;
using RTGen.Types;
using RTGen.Util;

namespace RTGen.Json.Generators
{
    /// <summary>Generates the single merged API description (invoked as
    /// `rtgen --language=json --config`): discovers every core header that declares
    /// interfaces or class factories, parses each with the C++ parser in-process
    /// (rtgen's CLI handles one source file per invocation, which is not a parser
    /// limitation — <see cref="CppParser"/> is stateless per parse), and links the
    /// results: resolves inheritance chains into absolute vtable slots, verifies
    /// interface IDs against the UUIDv5 scheme, scans the preprocessor-only error-code
    /// headers and enum-only headers, and nests every factory under the interface it
    /// constructs. Headers the grammar rejects verbatim are re-parsed from a sanitized
    /// copy (see <see cref="ParseHeader"/>).
    ///
    /// Options: --source points at the core sources root,
    /// --outputDir/--output/--extension name the merged output file.</summary>
    class JsonConfigGenerator : IConfigGenerator
    {
        private const string DefaultNamespace = "daq";

        private static readonly Regex ErrTypeRegex = new Regex(
            @"#define\s+OPENDAQ_ERRTYPE_(\w+)\s+0x([0-9A-Fa-f]+)u?");
        private static readonly Regex ErrCodeRegex = new Regex(
            @"#define\s+(OPENDAQ_ERR_\w+)\s+OPENDAQ_ERROR_CODE\(\s*OPENDAQ_ERRTYPE_(\w+)\s*,\s*0x([0-9A-Fa-f]+)u?\s*\)");
        private static readonly Regex SuccessRegex = new Regex(
            @"#define\s+(OPENDAQ_SUCCESS)\s+0x([0-9A-Fa-f]+)u?");
        private static readonly Regex CommentRegex = new Regex(
            @"/\*.*?\*/|//[^\n]*", RegexOptions.Singleline);
        private static readonly Regex EnumRegex = new Regex(
            @"enum(?:\s+class)?\s+(\w+)\s*(?::\s*\w+)?\s*\{(.*?)\}\s*;", RegexOptions.Singleline);
        private static readonly Regex DeclRegex = new Regex(
            @"^\s*(DECLARE_(TEMPLATED_)?OPENDAQ_(CUSTOM_)?INTERFACE[A-Z_]*|OPENDAQ_DECLARE_CLASS_FACTORY[A-Z_]*)\s*\(",
            RegexOptions.Multiline);
        private static readonly Regex ExportedFunctionRegex = new Regex(
            @"extern\s+""C""\s+([\w:]+)\s+PUBLIC_EXPORT\s+(\w+)\s*\(([^)]*)\)\s*;",
            RegexOptions.Singleline);
        private static readonly Regex FunctionPointerTypedefRegex = new Regex(
            @"typedef\s+[\w:]+\s*\(\s*\*\s*\w+\s*\)\s*\([^)]*\)\s*;");
        private static readonly Regex UsingAliasRegex = new Regex(
            @"^\s*using\s+\w+\s*=[^;]*;", RegexOptions.Multiline);
        private const string PlaceholderInterface = "IRtGenSanitizerPlaceholder";

        private IGeneratorOptions _options;

        public IVersionInfo Version => new VersionInfo
        {
            Major = 1,
            Minor = 0,
            Patch = 0
        };

        public IGeneratorOptions Options
        {
            get => _options;
            set
            {
                _options = value;
                if (string.IsNullOrEmpty(_options.GeneratedExtension))
                {
                    _options.GeneratedExtension = ".json";
                }
                if (string.IsNullOrEmpty(_options.Filename))
                {
                    _options.Filename = "opendaq";
                }
            }
        }

        public void GenerateConfigFile(string templatePath)
        {
            string outputDir = string.IsNullOrEmpty(Options.OutputDir) ? "." : Options.OutputDir;
            string outputPath = Path.Combine(outputDir, Options.Filename + Options.GeneratedExtension);
            string coreRoot = GetSourceOption();
            if (string.IsNullOrEmpty(coreRoot))
            {
                throw new RTGen.Exceptions.GeneratorException(
                    "--source=<core dir> is required (the directory to discover headers in).");
            }

            var interfaces = new List<OrderedDict>();
            var interfacesByName = new Dictionary<string, OrderedDict>();
            var enums = new Dictionary<string, OrderedDict>();
            var factories = new Dictionary<string, OrderedDict>();
            var coveredSources = new HashSet<string>();

            Action<OrderedDict, string> addInterface = (iface, header) =>
            {
                string name = iface.GetString("name");
                OrderedDict existing;
                if (interfacesByName.TryGetValue(name, out existing))
                {
                    string kept = existing.GetString("header");
                    Log.Warning($"Duplicate interface {name} from {header} ignored (kept {kept}).");
                    return;
                }
                interfaces.Add(iface);
                interfacesByName.Add(name, iface);
            };

            var parser = new CppParser();
            var parserOptions = (IParserOptions)Options;
            var exportedFunctions = new List<OrderedDict>();
            int parsedCount = 0;
            int failedCount = 0;

            foreach (KeyValuePair<string, string> moduleHeader in DiscoverHeaders(coreRoot))
            {
                string header = RelativeHeader(moduleHeader.Key, coreRoot);
                string library = moduleHeader.Value;

                OrderedDict data;
                try
                {
                    parserOptions.LibraryInfo.Name = library;
                    IRTFile rtFile = ParseHeader(parser, parserOptions, moduleHeader.Key,
                                                 exportedFunctions, header, library);
                    rtFile.SourceFileName = Path.GetFileName(moduleHeader.Key);
                    data = new JsonGenerator { Options = Options, RtFile = rtFile }.BuildModel(header);
                    parsedCount++;
                }
                catch (Exception e)
                {
                    failedCount++;
                    Log.Warning($"FAILED {header}: {e.Message}");
                    continue;
                }

                coveredSources.Add(Path.GetFileName(header));

                foreach (OrderedDict iface in DictItems(data.GetList("interfaces")))
                {
                    if (iface.GetString("name") == PlaceholderInterface)
                    {
                        continue;
                    }
                    iface.Set("library", library);
                    iface.Set("header", header);
                    addInterface(iface, header);
                }

                foreach (OrderedDict enumeration in DictItems(data.GetList("enums")))
                {
                    enumeration.Set("header", header);
                    ResolveEnumValues(enumeration);
                    if (!enums.ContainsKey(enumeration.GetString("name")))
                    {
                        enums.Add(enumeration.GetString("name"), enumeration);
                    }
                }

                foreach (OrderedDict factory in DictItems(data.GetList("factories")))
                {
                    factory.Set("library", library);
                    factory.Set("header", header);
                    string factoryName = factory.GetString("name");
                    if (factories.ContainsKey(factoryName))
                    {
                        Log.Warning($"Duplicate factory {factoryName} from {header} ignored.");
                        continue;
                    }
                    factories.Add(factoryName, factory);
                }
            }

            if (parsedCount == 0)
            {
                throw new RTGen.Exceptions.GeneratorException(
                    $"No parseable interface headers found under \"{coreRoot}\".");
            }

            VerifyInterfaceIds(interfacesByName);
            AssignVtableSlots(interfaces, interfacesByName);
            string callingConvention = HoistCallingConvention(interfaces);
            List<object> unmatchedFactories = NestFactories(factories, interfacesByName);

            // The supplemental scan sweeps every header under the include roots, which
            // also finds internal enums (eval-value lexer/parser, sha1, type traits).
            // Keep only enums the API surface actually references; this reproduces
            // exactly the enum set the curated language bindings expose.
            HashSet<string> referenced = CollectReferencedTypeNames(interfaces);
            foreach (OrderedDict enumeration in ScanSupplementalEnums(coreRoot, coveredSources))
            {
                string name = enumeration.GetString("name");
                if (referenced.Contains(name) && !enums.ContainsKey(name))
                {
                    enums.Add(name, enumeration);
                }
            }

            var root = new OrderedDict
            {
                { "schema", "opendaq-bindings/1" },
                { "namespace", DefaultNamespace },
                {
                    "abi", new OrderedDict
                    {
                        { "calling_convention", callingConvention == "stdcall"
                            ? "INTERFACE_FUNC: __stdcall on 32-bit Windows, platform default elsewhere"
                            : callingConvention },
                        { "intf_id_scheme", "RFC 4122 UUIDv5, DNS namespace, name '<Interface>.<namespace>'" },
                        { "vtable", "single inheritance, declaration order; absolute_vtable_slot indexes the "
                                    + "whole vtable, relative_vtable_slot only the interface's own methods" }
                    }
                },
                { "interfaces", interfaces.OrderBy(i => i.GetString("name"), StringComparer.Ordinal).ToList<object>() },
                { "enums", enums.Values.OrderBy(e => e.GetString("name"), StringComparer.Ordinal).ToList<object>() },
                { "functions", exportedFunctions
                    .OrderBy(f => f.GetString("name"), StringComparer.Ordinal).ToList<object>() },
                { "error_codes", ScanErrorCodes(coreRoot) }
            };
            if (unmatchedFactories.Count > 0)
            {
                root.Add("factories", unmatchedFactories);
            }

            Directory.CreateDirectory(outputDir);
            File.WriteAllText(outputPath, JsonValue.Serialize(root, 0) + "\n", new UTF8Encoding(false));

            Log.Info($"Parsed {parsedCount} headers ({failedCount} parse failures, expected: see README.md); "
                     + $"linked {interfaces.Count} interfaces, {enums.Count} enums, "
                     + $"{factories.Count} factories, {((List<object>)root.Get("error_codes")).Count} error codes "
                     + $"-> {outputPath}");
        }

        // -------------------------------------------------------------------
        //  Linking
        // -------------------------------------------------------------------

        private static void VerifyInterfaceIds(Dictionary<string, OrderedDict> interfaces)
        {
            foreach (OrderedDict iface in interfaces.Values)
            {
                if (iface.GetString("name") == "IUnknown")
                {
                    // DEFINE_EXTERNAL_INTFID(UnknownGuid): the COM IUnknown IID, not a UUIDv5.
                    continue;
                }
                string ns = iface.GetString("namespace") ?? DefaultNamespace;
                string expected = Utility.InterfaceUuid($"{iface.GetString("name")}.{ns}").ToString();
                string actual = iface.GetString("id");
                if (actual == null)
                {
                    iface.Set("id", expected);
                }
                else if (actual != expected)
                {
                    Log.Warning($"Interface {iface.GetString("name")}: parsed ID {actual} "
                                + $"does not match UUIDv5 scheme ({expected}).");
                }
            }
        }

        private static void AssignVtableSlots(List<OrderedDict> interfaces,
                                              Dictionary<string, OrderedDict> interfacesByName)
        {
            Func<string, HashSet<string>, long?> slotCount = null;
            slotCount = (name, trail) =>
            {
                OrderedDict iface;
                if (!interfacesByName.TryGetValue(name, out iface) || !trail.Add(name))
                {
                    return null;
                }
                long own = iface.GetList("methods").Count;
                string baseName = iface.GetString("base");
                if (baseName == null)
                {
                    return own;
                }
                long? below = slotCount(baseName, trail);
                return below == null ? null : below + own;
            };

            foreach (OrderedDict iface in interfaces)
            {
                string baseName = iface.GetString("base");
                long? firstSlot = baseName == null ? 0 : slotCount(baseName, new HashSet<string>());
                if (firstSlot == null)
                {
                    Log.Warning($"Interface {iface.GetString("name")}: base "
                                + $"{baseName} not found, vtable slots not assigned.");
                    continue;
                }

                // The relative slot is the method's index among the interface's own
                // methods; the absolute slot adds the size of the inherited vtable.
                long relative = 0;
                foreach (OrderedDict method in DictItems(iface.GetList("methods")))
                {
                    method.Set("absolute_vtable_slot", firstSlot.Value + relative);
                    method.Set("relative_vtable_slot", relative);
                    relative++;
                }
            }
        }

        /// <summary>Moves every factory into the interface it constructs (factories are
        /// declared next to their main interface and act as its constructors). Provenance
        /// is kept on the factory only when it differs from the interface's; factories
        /// whose interface is unknown stay top-level.</summary>
        private static List<object> NestFactories(Dictionary<string, OrderedDict> factories,
                                                  Dictionary<string, OrderedDict> interfacesByName)
        {
            var unmatched = new List<object>();
            foreach (OrderedDict factory in factories.Values
                .OrderBy(f => f.GetString("name"), StringComparer.Ordinal))
            {
                string interfaceName = factory.GetString("interface");
                OrderedDict iface;
                if (!interfacesByName.TryGetValue(interfaceName, out iface))
                {
                    Log.Warning($"Factory {factory.GetString("name")}: constructed interface "
                                + $"{interfaceName} not found, kept top-level.");
                    unmatched.Add(factory);
                    continue;
                }

                factory.Remove("interface");
                if (factory.GetString("header") == iface.GetString("header"))
                {
                    factory.Remove("header");
                }
                if (factory.GetString("library") == iface.GetString("library"))
                {
                    factory.Remove("library");
                }

                List<object> list = iface.GetList("factories");
                if (list == null)
                {
                    list = new List<object>();
                    iface.Add("factories", list);
                }
                list.Add(factory);
            }
            return unmatched;
        }

        /// <summary>All type names appearing in method/factory signatures (including
        /// generic arguments), used to limit the supplemental enum scan to enums the
        /// API surface actually uses.</summary>
        private static HashSet<string> CollectReferencedTypeNames(List<OrderedDict> interfaces)
        {
            var referenced = new HashSet<string>();

            Action<OrderedDict> walkType = null;
            walkType = type =>
            {
                if (type == null)
                {
                    return;
                }
                string name = type.GetString("name");
                if (name != null)
                {
                    referenced.Add(name);
                }
                foreach (OrderedDict generic in DictItems(type.GetList("generic_arguments")))
                {
                    walkType(generic);
                }
                // Container element types are plain names, not nested type objects.
                foreach (string element in new[] { type.GetString("key_type"), type.GetString("value_type") })
                {
                    if (element != null)
                    {
                        referenced.Add(element);
                    }
                }
            };

            Action<OrderedDict> walkSignature = member =>
            {
                walkType(member.GetDict("return_type"));
                foreach (OrderedDict arg in DictItems(member.GetList("arguments")))
                {
                    walkType(arg.GetDict("type"));
                }
            };

            foreach (OrderedDict iface in interfaces)
            {
                foreach (OrderedDict method in DictItems(iface.GetList("methods")))
                {
                    walkSignature(method);
                }
                foreach (OrderedDict factory in DictItems(iface.GetList("factories")))
                {
                    walkSignature(factory);
                }
            }
            return referenced;
        }

        /// <summary>Removes the per-method calling convention when it is uniform across the
        /// whole API (it always is: INTERFACE_FUNC) and returns the hoisted value.</summary>
        private static string HoistCallingConvention(List<OrderedDict> interfaces)
        {
            var conventions = new HashSet<string>();
            foreach (OrderedDict iface in interfaces)
            {
                foreach (OrderedDict method in DictItems(iface.GetList("methods")))
                {
                    conventions.Add(method.GetString("calling_convention") ?? "stdcall");
                }
            }

            if (conventions.Count > 1)
            {
                Log.Warning("Calling convention differs between methods ("
                            + string.Join(", ", conventions) + "); keeping per-method values.");
                return string.Join("|", conventions.OrderBy(c => c));
            }

            foreach (OrderedDict iface in interfaces)
            {
                foreach (OrderedDict method in DictItems(iface.GetList("methods")))
                {
                    method.Remove("calling_convention");
                }
            }
            return conventions.FirstOrDefault() ?? "stdcall";
        }

        // -------------------------------------------------------------------
        //  Parsing with sanitize-and-retry
        // -------------------------------------------------------------------

        /// <summary>Parses the header as-is; when the grammar rejects it, retries with a
        /// sanitized copy that strips the constructs rtgen's parser cannot handle (macro
        /// bodies, function-pointer typedefs, using-aliases, inline template helpers) and
        /// injects a placeholder interface into factory-only headers (the parser crashes
        /// on factories when the file declares no interface). Freestanding extern "C"
        /// PUBLIC_EXPORT declarations are recorded into the functions section, since the
        /// grammar cannot parse them either.</summary>
        private static IRTFile ParseHeader(CppParser parser, IParserOptions parserOptions,
                                           string path, List<OrderedDict> exportedFunctions,
                                           string header, string library)
        {
            try
            {
                return parser.Parse(path, parserOptions);
            }
            catch (Exception)
            {
                var exported = new List<OrderedDict>();
                string sanitized = SanitizeHeaderSource(File.ReadAllText(path), exported, header, library);

                string tempDir = Path.Combine(Path.GetTempPath(), "rtgen-json-sanitized");
                Directory.CreateDirectory(tempDir);
                string tempPath = Path.Combine(tempDir, Path.GetFileName(path));
                File.WriteAllText(tempPath, sanitized, new UTF8Encoding(false));

                IRTFile rtFile = parser.Parse(tempPath, parserOptions);
                // Contribute the extracted exports only when the retry actually parsed.
                exportedFunctions.AddRange(exported);
                return rtFile;
            }
        }

        private static string SanitizeHeaderSource(string text, List<OrderedDict> exported,
                                                   string header, string library)
        {
            text = StripMacroDefinitions(text);

            text = ExportedFunctionRegex.Replace(text, match =>
            {
                exported.Add(BuildExportedFunction(match, header, library));
                return "";
            });

            text = FunctionPointerTypedefRegex.Replace(text, "");
            text = UsingAliasRegex.Replace(text, "");
            text = StripTemplates(text);

            // Factory-only header: the parser needs a current interface for the factory's
            // out-parameter namespace; give it a throwaway one (dropped after the merge).
            if (!text.Contains("DECLARE_OPENDAQ_INTERFACE") && DeclRegex.IsMatch(text))
            {
                int marker = text.IndexOf("BEGIN_NAMESPACE_OPENDAQ", StringComparison.Ordinal);
                if (marker >= 0)
                {
                    int lineEnd = text.IndexOf('\n', marker);
                    if (lineEnd >= 0)
                    {
                        text = text.Insert(lineEnd + 1,
                            $"DECLARE_OPENDAQ_INTERFACE({PlaceholderInterface}, IBaseObject)\n{{\n}};\n");
                    }
                }
            }
            return text;
        }

        /// <summary>Blanks #define directives including their line continuations. Macro
        /// bodies routinely contain tokens the grammar cannot lex (->, #stringification,
        /// class definitions).</summary>
        private static string StripMacroDefinitions(string text)
        {
            string[] lines = text.Split('\n');
            bool inDefine = false;
            for (int i = 0; i < lines.Length; i++)
            {
                bool blank = inDefine || lines[i].TrimStart().StartsWith("#define");
                if (blank)
                {
                    inDefine = lines[i].TrimEnd().EndsWith("\\");
                    lines[i] = "";
                }
            }
            return string.Join("\n", lines);
        }

        /// <summary>Blanks template declarations/definitions (inline smart-pointer helper
        /// functions next to factory macros); they are C++ convenience sugar with no ABI
        /// of their own. Newlines are preserved so parser positions stay meaningful.</summary>
        private static string StripTemplates(string text)
        {
            var sb = new StringBuilder(text);
            int searchFrom = 0;
            while (true)
            {
                int start = FindWord(sb, "template", searchFrom);
                if (start < 0)
                {
                    return sb.ToString();
                }

                int pos = start + "template".Length;
                while (pos < sb.Length && char.IsWhiteSpace(sb[pos]))
                {
                    pos++;
                }
                if (pos >= sb.Length || sb[pos] != '<')
                {
                    searchFrom = start + 1;
                    continue;
                }

                int angleDepth = 0;
                int braceDepth = 0;
                bool sawBrace = false;
                int end = -1;
                for (; pos < sb.Length; pos++)
                {
                    char c = sb[pos];
                    if (c == '<' && braceDepth == 0) angleDepth++;
                    else if (c == '>' && braceDepth == 0 && angleDepth > 0) angleDepth--;
                    else if (c == '{') { braceDepth++; sawBrace = true; }
                    else if (c == '}')
                    {
                        braceDepth--;
                        if (sawBrace && braceDepth == 0) { end = pos + 1; break; }
                    }
                    else if (c == ';' && braceDepth == 0 && angleDepth == 0 && !sawBrace)
                    {
                        end = pos + 1;
                        break;
                    }
                }
                if (end < 0)
                {
                    return sb.ToString();
                }
                while (end < sb.Length && (char.IsWhiteSpace(sb[end]) || sb[end] == ';'))
                {
                    if (sb[end] == ';') { end++; break; }
                    end++;
                }
                for (int i = start; i < end; i++)
                {
                    if (sb[i] != '\n')
                    {
                        sb[i] = ' ';
                    }
                }
                searchFrom = start;
            }
        }

        private static int FindWord(StringBuilder sb, string word, int from)
        {
            string text = sb.ToString();
            for (int index = text.IndexOf(word, from, StringComparison.Ordinal);
                 index >= 0;
                 index = text.IndexOf(word, index + 1, StringComparison.Ordinal))
            {
                bool startOk = index == 0 || !char.IsLetterOrDigit(text[index - 1]) && text[index - 1] != '_';
                int after = index + word.Length;
                bool endOk = after >= text.Length || !char.IsLetterOrDigit(text[after]) && text[after] != '_';
                if (startOk && endOk)
                {
                    return index;
                }
            }
            return -1;
        }

        private static OrderedDict BuildExportedFunction(Match match, string header, string library)
        {
            var arguments = new List<object>();
            foreach (string rawArg in match.Groups[3].Value.Split(','))
            {
                string arg = string.Join(" ", rawArg.Split((char[])null, StringSplitOptions.RemoveEmptyEntries));
                if (arg.Length == 0)
                {
                    continue;
                }
                int nameStart = arg.LastIndexOfAny(new[] { ' ', '*', '&' }) + 1;
                arguments.Add(new OrderedDict
                {
                    { "name", arg.Substring(nameStart) },
                    { "type", CppTypeDesc(arg.Substring(0, nameStart)) }
                });
            }

            return new OrderedDict
            {
                { "name", match.Groups[2].Value },
                { "return_type", CppTypeDesc(match.Groups[1].Value) },
                { "arguments", arguments },
                { "library", library },
                { "header", header }
            };
        }

        private static OrderedDict CppTypeDesc(string declaration)
        {
            int pointers = declaration.Count(c => c == '*');
            string name = declaration.Replace("*", "").Replace("&", "").Trim();
            bool isConst = name.StartsWith("const ");
            if (isConst)
            {
                name = name.Substring("const ".Length).Trim();
            }
            if (name.StartsWith("daq::"))
            {
                name = name.Substring("daq::".Length);
            }
            var result = new OrderedDict { { "name", name } };
            if (pointers > 0)
            {
                result.Add("pointers", pointers);
            }
            if (isConst)
            {
                result.Add("is_const", true);
            }
            return result;
        }

        // -------------------------------------------------------------------
        //  Header discovery
        // -------------------------------------------------------------------

        /// <summary>The public include roots under the core sources directory, each with
        /// the library its headers belong to. corecontainers installs its headers into
        /// the coretypes include directory and is part of that library.</summary>
        private static IEnumerable<KeyValuePair<string, string>> ModuleRoots(string coreRoot)
        {
            var roots = new List<KeyValuePair<string, string>>
            {
                new KeyValuePair<string, string>(
                    Path.Combine(coreRoot, "coretypes", "include", "coretypes"), "coretypes"),
                new KeyValuePair<string, string>(
                    Path.Combine(coreRoot, "corecontainers", "include", "coretypes"), "coretypes"),
                new KeyValuePair<string, string>(
                    Path.Combine(coreRoot, "coreobjects", "include", "coreobjects"), "coreobjects"),
            };
            string opendaqRoot = Path.Combine(coreRoot, "opendaq");
            if (Directory.Exists(opendaqRoot))
            {
                roots.AddRange(Directory.GetDirectories(opendaqRoot)
                    .OrderBy(dir => dir, StringComparer.Ordinal)
                    .Select(dir => new KeyValuePair<string, string>(
                        Path.Combine(dir, "include", "opendaq"), "opendaq")));
            }
            return roots.Where(root => Directory.Exists(root.Key));
        }

        private static IEnumerable<string> IncludeRoots(string coreRoot)
        {
            return ModuleRoots(coreRoot).Select(root => root.Key);
        }

        /// <summary>Headers that declare interfaces or class factories, paired with their
        /// library name. Everything else (smart pointers, impls, factory helpers) has no
        /// ABI declarations to parse.</summary>
        private static IEnumerable<KeyValuePair<string, string>> DiscoverHeaders(string coreRoot)
        {
            foreach (KeyValuePair<string, string> root in ModuleRoots(coreRoot))
            {
                foreach (string header in HeadersUnder(root.Key))
                {
                    // Match on define-stripped text so the factory macro machinery
                    // (factory.h) is not mistaken for declarations.
                    if (DeclRegex.IsMatch(StripMacroDefinitions(File.ReadAllText(header))))
                    {
                        yield return new KeyValuePair<string, string>(header, root.Value);
                    }
                }
            }
        }

        // -------------------------------------------------------------------
        //  Scans over C++ sources (headers rtgen cannot parse)
        // -------------------------------------------------------------------

        private static IEnumerable<string> HeadersUnder(string root)
        {
            return Directory.GetFiles(root, "*.h", SearchOption.AllDirectories)
                .OrderBy(file => file, StringComparer.Ordinal);
        }

        private List<object> ScanErrorCodes(string coreRoot)
        {
            var errorHeaders = new List<string>();
            foreach (string root in IncludeRoots(coreRoot))
            {
                errorHeaders.AddRange(HeadersUnder(root)
                    .Where(file => Path.GetFileNameWithoutExtension(file).Contains("errors")));
            }

            var families = new Dictionary<string, long>();
            foreach (string header in errorHeaders)
            {
                foreach (Match match in ErrTypeRegex.Matches(File.ReadAllText(header)))
                {
                    families[match.Groups[1].Value] = Convert.ToInt64(match.Groups[2].Value, 16);
                }
            }

            var codes = new List<object>();
            var seen = new HashSet<string>();
            foreach (string header in errorHeaders)
            {
                string text = File.ReadAllText(header);
                string relHeader = RelativeHeader(header, coreRoot);
                foreach (Match match in SuccessRegex.Matches(text))
                {
                    if (seen.Add(match.Groups[1].Value))
                    {
                        codes.Add(new OrderedDict
                        {
                            { "name", match.Groups[1].Value },
                            { "code", Convert.ToInt64(match.Groups[2].Value, 16) },
                            { "header", relHeader }
                        });
                    }
                }
                foreach (Match match in ErrCodeRegex.Matches(text))
                {
                    string name = match.Groups[1].Value;
                    long family;
                    if (!seen.Add(name) || !families.TryGetValue(match.Groups[2].Value, out family))
                    {
                        continue;
                    }
                    codes.Add(new OrderedDict
                    {
                        { "name", name },
                        { "code", 0x80000000L | (family << 16) | Convert.ToInt64(match.Groups[3].Value, 16) },
                        { "header", relHeader }
                    });
                }
            }
            return codes.OrderBy(c => (long)((OrderedDict)c).Get("code")).ToList();
        }

        /// <summary>Enums declared in headers rtgen never parses successfully: files with
        /// enums but no interfaces (e.g. sample_type.h) are rejected with "no classes
        /// found", so they are scanned directly.</summary>
        private List<OrderedDict> ScanSupplementalEnums(string coreRoot, HashSet<string> coveredSources)
        {
            var result = new List<OrderedDict>();
            foreach (string root in IncludeRoots(coreRoot))
            {
                foreach (string header in HeadersUnder(root))
                {
                    string fileName = Path.GetFileName(header);
                    if (coveredSources.Contains(fileName)
                        || Path.GetFileNameWithoutExtension(header).EndsWith("_impl"))
                    {
                        continue;
                    }

                    string text = CommentRegex.Replace(File.ReadAllText(header), "");
                    foreach (Match match in EnumRegex.Matches(text))
                    {
                        var options = new List<object>();
                        foreach (string rawPart in match.Groups[2].Value.Split(','))
                        {
                            string part = rawPart.Trim();
                            if (part.Length == 0 || !char.IsLetter(part[0]) && part[0] != '_')
                            {
                                continue;
                            }
                            var option = new OrderedDict();
                            int assign = part.IndexOf('=');
                            if (assign >= 0)
                            {
                                option.Add("name", part.Substring(0, assign).Trim());
                                option.Add("value", part.Substring(assign + 1).Trim());
                            }
                            else
                            {
                                option.Add("name", part);
                            }
                            options.Add(option);
                        }
                        if (options.Count == 0)
                        {
                            continue;
                        }
                        var enumeration = new OrderedDict
                        {
                            { "name", match.Groups[1].Value },
                            { "options", options },
                            { "header", RelativeHeader(header, coreRoot) }
                        };
                        ResolveEnumValues(enumeration);
                        result.Add(enumeration);
                    }
                }
            }
            return result;
        }

        private static void ResolveEnumValues(OrderedDict enumeration)
        {
            long nextValue = 0;
            foreach (OrderedDict option in DictItems(enumeration.GetList("options")))
            {
                object raw = option.Get("value");
                if (raw == null)
                {
                    option.Set("value", nextValue++);
                    continue;
                }
                long parsed;
                if (TryParseInt(raw.ToString(), out parsed))
                {
                    option.Set("value", parsed);
                    nextValue = parsed + 1;
                }
                else
                {
                    option.Remove("value");
                    option.Set("raw_value", raw.ToString());
                }
            }
        }

        private static bool TryParseInt(string text, out long value)
        {
            text = text.Trim().TrimEnd('u', 'U', 'l', 'L');
            try
            {
                value = text.StartsWith("0x") || text.StartsWith("0X")
                    ? Convert.ToInt64(text.Substring(2), 16)
                    : Convert.ToInt64(text, 10);
                return true;
            }
            catch (Exception)
            {
                value = 0;
                return false;
            }
        }

        // -------------------------------------------------------------------
        //  Helpers
        // -------------------------------------------------------------------

        private string GetSourceOption()
        {
            // --source lands in ProgramOptions.InputFile, which is not part of
            // IGeneratorOptions (config generation does not parse a source file),
            // so it is read via reflection.
            var property = Options.GetType().GetProperty("InputFile");
            return property?.GetValue(Options, null) as string;
        }

        private static IEnumerable<OrderedDict> DictItems(List<object> list)
        {
            return list == null ? Enumerable.Empty<OrderedDict>() : list.OfType<OrderedDict>();
        }

        private static string RelativeHeader(string header, string coreRoot)
        {
            // Headers are enumerated relative to --source, so with a repository-
            // relative --source (e.g. "core") they are already repository-relative
            // like all other header provenance; only normalize separators.
            return header.Replace('\\', '/');
        }
    }
}
