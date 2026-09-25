using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using RTGen.Interfaces;
using RTGen.Interfaces.Doc;
using RTGen.Types;

namespace RTGen.Json.Generators
{
    /// <summary>Emits a machine-readable JSON description of the parsed C++ interface header.
    /// Unlike the other generators it performs no name or type mapping: it describes the C++
    /// ABI surface (interfaces, vtable method order, factories, enums, aliases) so that
    /// downstream binding generators can derive their own representation from one file.</summary>
    class JsonGenerator : IGenerator
    {
        private IGeneratorOptions _options;

        public IRTFile RtFile { get; set; }

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
            }
        }

        public void RegisterTypeMappings(Dictionary<string, string> mappings)
        {
            // Intentionally empty: the JSON output preserves the raw C++ type names.
        }

        public string Generate(string templatePath)
        {
            return JsonValue.Serialize(BuildFile(), 0);
        }

        public void GenerateFile(string templatePath)
        {
            string fileName = !string.IsNullOrEmpty(Options.Filename)
                ? Options.Filename
                : Path.GetFileNameWithoutExtension(RtFile.SourceFileName);

            string outputDir = string.IsNullOrEmpty(Options.OutputDir) ? "." : Options.OutputDir;
            Directory.CreateDirectory(outputDir);

            string outputPath = Path.Combine(outputDir, fileName + Options.GeneratedExtension);
            File.WriteAllText(outputPath, Generate(templatePath) + Environment.NewLine, new UTF8Encoding(false));
        }

        // ------------------------------------------------------------------
        //  Object model construction
        // ------------------------------------------------------------------

        private OrderedDict BuildFile()
        {
            // --source as given on the command line (relative paths stay relative);
            // it lives in ProgramOptions.InputFile which is not on IGeneratorOptions.
            var inputFileProperty = Options.GetType().GetProperty("InputFile");
            string sourcePath = inputFileProperty?.GetValue(Options, null) as string;
            return BuildModel(!string.IsNullOrEmpty(sourcePath) ? sourcePath : RtFile.SourceFileName);
        }

        /// <summary>Builds the JSON object model for the parsed header; also called by
        /// <see cref="JsonConfigGenerator"/>, which parses headers in-process.</summary>
        internal OrderedDict BuildModel(string sourcePath)
        {
            var root = new OrderedDict
            {
                { "schema", "opendaq-rtgen-json/1" },
                { "source", sourcePath?.Replace('\\', '/') },
                { "library", Options.LibraryInfo?.Name },
                { "namespace", Options.LibraryInfo?.Namespace?.Raw }
            };

            var interfaces = RtFile.Classes.Select(BuildInterface).ToList<object>();
            root.Add("interfaces", interfaces);

            if (RtFile.Enums != null && RtFile.Enums.Count > 0)
            {
                root.Add("enums", RtFile.Enums.Select(BuildEnum).ToList<object>());
            }

            if (RtFile.Factories != null && RtFile.Factories.Count > 0)
            {
                root.Add("factories", RtFile.Factories.Select(BuildFactory).ToList<object>());
            }

            if (RtFile.GlobalMethods != null && RtFile.GlobalMethods.Count > 0)
            {
                root.Add("global_methods", RtFile.GlobalMethods.Select(BuildMethod).ToList<object>());
            }

            return root;
        }

        private OrderedDict BuildInterface(IRTInterface rtClass)
        {
            var iface = new OrderedDict
            {
                { "name", rtClass.Type.UnmappedName }
            };
            AddNamespace(iface, rtClass.Type.Namespace?.Raw);
            iface.Add("id", GuidOrNull(rtClass.Type.Guid));

            if (rtClass.BaseType != null && !string.IsNullOrEmpty(rtClass.BaseType.UnmappedName))
            {
                iface.Add("base", QualifiedName(rtClass.BaseType.UnmappedName,
                                                rtClass.BaseType.Namespace?.Raw));
            }

            AddDoc(iface, rtClass.Documentation);

            // Declaration order == vtable order within this interface. Absolute vtable
            // slots are computed by the link step once all base interfaces are known.
            iface.Add("methods", rtClass.Methods.Select(BuildMethod).ToList<object>());

            return iface;
        }

        private OrderedDict BuildMethod(IMethod method)
        {
            Dictionary<string, string> documented = DocumentedDirections(method.Documentation);
            var result = new OrderedDict
            {
                { "name", method.Name },
                { "return_type", method.ReturnType != null ? BuildType(method.ReturnType) : null },
                {
                    "arguments",
                    method.Arguments.Select(arg => BuildArgument(arg, documented)).ToList<object>()
                }
            };

            if (!string.IsNullOrEmpty(method.CallingConvention))
            {
                result.Add("calling_convention", method.CallingConvention);
            }

            if (method.IsIgnored != GeneratorType.None)
            {
                result.Add("ignored_for", method.IsIgnored.ToString());
            }

            AddDoc(result, method.Documentation);
            return result;
        }

        /// <summary>The direction the doc comment documents for each parameter. rtgen's
        /// model only knows whether a tag is exactly @param[out], so @param[in,out] is
        /// read back off the raw tag text - it is the only place the in-out parameters
        /// (the readers' sample counts) are distinguishable from plain out ones.</summary>
        private static Dictionary<string, string> DocumentedDirections(IDocComment doc)
        {
            var directions = new Dictionary<string, string>();
            if (doc == null)
            {
                return directions;
            }

            foreach (IDocTag tag in doc.Tags)
            {
                var param = tag as IDocParam;
                if (param == null || tag.TagType != TagType.Param)
                {
                    continue;
                }
                string raw = (param.RawText ?? string.Empty).Replace(" ", string.Empty);
                string direction = raw.StartsWith("@param[in,out]", StringComparison.Ordinal)
                    ? "inout"
                    : param.IsOut ? "out" : "in";
                directions[param.ParamName] = direction;
            }
            return directions;
        }

        private OrderedDict BuildArgument(IArgument arg, Dictionary<string, string> documented)
        {
            var result = new OrderedDict
            {
                { "name", arg.Name },
                { "type", BuildType(arg.Type) }
            };

            // The pointer shape says whether the callee writes through the argument, but
            // it cannot tell an argument the callee also reads: the readers' counts are
            // annotated as arrays, which hides their out pointer. Only @param[in,out]
            // documents that, so it wins over the shape.
            string direction = arg.IsOutParam || arg.IsOutPointer ? "out" : "in";
            string documentedDirection;
            if (documented.TryGetValue(arg.Name, out documentedDirection)
                && documentedDirection == "inout")
            {
                direction = "inout";
            }
            result.Add("direction", direction);
            if (arg.IsConst)
            {
                result.Add("is_const", true);
            }
            if (arg.IsStealRef)
            {
                result.Add("steals_reference", true);
            }
            if (arg.AllowNull)
            {
                result.Add("allow_null", true);
            }
            if (arg.IsPolymorphic)
            {
                result.Add("is_polymorphic", true);
            }
            if (!string.IsNullOrEmpty(arg.DefaultValue))
            {
                result.Add("default_value", arg.DefaultValue);
            }

            return result;
        }

        private OrderedDict BuildType(ITypeName type)
        {
            if (type == null)
            {
                return null;
            }

            string modifiers = type.Modifiers ?? string.Empty;

            var result = new OrderedDict
            {
                { "name", type.UnmappedName }
            };
            AddNamespace(result, type.Namespace?.Raw);

            int pointers = modifiers.Count(c => c == '*');
            if (pointers > 0)
            {
                result.Add("pointers", pointers);
            }
            if (modifiers.Contains("&"))
            {
                result.Add("is_reference", true);
            }

            if (type.HasGenericArguments && type.GenericArguments != null)
            {
                AddElementTypes(result, type.UnmappedName,
                                type.GenericArguments.Select(BuildType).ToList());
            }

            return result;
        }

        /// <summary>Spells out the element types of the container interfaces, which the
        /// headers annotate with [elementType]/[templateType]: a list has a value type, a
        /// dictionary a key and a value type. The generic arguments of everything else
        /// (the genuine C++ templates, IEvent&lt;TSender, TArgs&gt; and std::shared_ptr)
        /// are dropped: they do not affect the ABI, which passes the plain interface
        /// pointer either way.</summary>
        private static void AddElementTypes(OrderedDict target, string typeName,
                                            List<OrderedDict> generics)
        {
            // Element types are bare interface or enum names: a generic that carries a
            // foreign namespace, pointers or nested generics is not one, and BuildType
            // has already decided which of those need spelling out.
            List<string> names = generics
                .Select(g => g.Count == 1 ? g.GetString("name") : null)
                .ToList();
            if (names.Any(n => n == null))
            {
                return;
            }

            if (typeName == "IList" && names.Count == 1)
            {
                target.Add("value_type", names[0]);
            }
            else if (typeName == "IDict" && names.Count == 2)
            {
                target.Add("key_type", names[0]);
                target.Add("value_type", names[1]);
            }
        }

        private OrderedDict BuildFactory(IRTFactory factory)
        {
            Dictionary<string, string> factoryDirections = DocumentedDirections(factory.Documentation);
            var result = new OrderedDict
            {
                // The name is also the extern "C" symbol exported by the core library.
                { "name", factory.Name },
                // The factory macros always expand to functions returning daq::ErrCode.
                { "return_type", new OrderedDict { { "name", "ErrCode" } } },
                { "interface", factory.InterfaceName },
                {
                    "arguments",
                    factory.Arguments
                        .Select(arg => BuildArgument(arg, factoryDirections))
                        .ToList<object>()
                }
            };

            if (factory.Options != FactoryOptions.None)
            {
                result.Add("options", factory.Options.ToString());
            }
            if (factory.IsGeneric)
            {
                // Template factories (sample-type specializations) are only marked for now;
                // their specialized symbols are not enumerated.
                result.Add("is_generic", true);
            }

            AddDoc(result, factory.Documentation);
            return result;
        }

        private OrderedDict BuildEnum(IEnumeration enumeration)
        {
            var options = new List<object>();
            foreach (IEnumOption option in enumeration.Options)
            {
                var entry = new OrderedDict { { "name", option.Name } };
                if (option.HasValue)
                {
                    entry.Add("value", option.Value);
                }
                options.Add(entry);
            }

            return new OrderedDict
            {
                { "name", enumeration.Name },
                { "options", options }
            };
        }

        // ------------------------------------------------------------------
        //  Documentation
        // ------------------------------------------------------------------

        /// <summary>Renders the documentation comment as the single string it is in the
        /// header. Every doc element keeps the raw source slice it was parsed from, so
        /// the comment is reassembled from those in order of appearance rather than
        /// re-rendered from the parsed model, which would drop whatever the model has no
        /// place for (@code blocks, formatting, unknown tags).</summary>
        private void AddDoc(OrderedDict target, IDocComment doc)
        {
            if (doc == null)
            {
                return;
            }

            // The brief and the first description paragraph are held apart from the tag
            // list, but every element consumed one slot of the parser's tag counter and
            // the tag list is in counter order, so the source order is recovered by
            // giving the tags the slots the brief and description did not take. The
            // description is not always first: plenty of headers put it after @param.
            var slots = new SortedDictionary<int, KeyValuePair<bool, string>>();
            var taken = new HashSet<int>();
            Action<int, bool, string> put = (index, isDescription, raw) =>
                slots[index] = new KeyValuePair<bool, string>(isDescription, raw);

            if (doc.Brief != null)
            {
                put(doc.Brief.TagIndex, false, doc.Brief.RawText);
                taken.Add(doc.Brief.TagIndex);
            }
            if (doc.Description != null)
            {
                put(doc.Description.TagIndex, true, doc.Description.RawText);
                taken.Add(doc.Description.TagIndex);
            }

            int slot = 0;
            foreach (IDocTag tag in doc.Tags)
            {
                while (taken.Contains(slot))
                {
                    slot++;
                }
                put(slot, tag.TagType == TagType.Description, tag.RawText);
                slot++;
            }

            var text = new StringBuilder();
            foreach (KeyValuePair<bool, string> part in slots.Values)
            {
                string cleaned = CleanRawDoc(part.Value);
                if (cleaned.Length == 0)
                {
                    continue;
                }
                if (text.Length > 0)
                {
                    // A description paragraph after a tag is separated by a blank line in
                    // the source: doxygen needs it to end the tag it would otherwise
                    // continue. Anything else follows on the next line.
                    text.Append(part.Key ? "\n\n" : "\n");
                }
                text.Append(cleaned);
            }

            string rendered = text.ToString();
            if (rendered.Length > 0 && !IsGroupStructureOnly(rendered))
            {
                target.Add("doc", rendered);
            }
        }

        /// <summary>Whether the comment only opens or closes a doxygen group. Those
        /// comments document the grouping of the file, not the entity they happen to
        /// precede - rtgen attaches the nearest leading comment, so an undocumented
        /// interface picks up the group's @{ and the factories after it pick up its @}.
        /// No comment in the headers mixes group markers with real documentation.</summary>
        private static bool IsGroupStructureOnly(string doc)
        {
            return doc.Split('\n')
                .Select(line => line.Trim())
                .Where(line => line.Length > 0)
                .All(line => line == "@{" || line == "@}"
                             || GroupTagRegex.IsMatch(line));
        }

        private static readonly System.Text.RegularExpressions.Regex GroupTagRegex =
            new System.Text.RegularExpressions.Regex(@"^@(ingroup|addtogroup|defgroup|weakgroup|name)\b");

        /// <summary>Strips the comment continuation markers a raw slice spans.</summary>
        private static string CleanRawDoc(string raw)
        {
            if (string.IsNullOrEmpty(raw))
            {
                return string.Empty;
            }

            IEnumerable<string> lines = raw
                .Replace("\r\n", "\n")
                .Split('\n')
                .Select(line => line.Trim().TrimStart('*').Trim());
            return string.Join("\n", lines).Trim();
        }


        private static string GuidOrNull(Guid guid)
        {
            return guid == Guid.Empty ? null : guid.ToString();
        }

        /// <summary>The overwhelming default namespace is the library's own (daq); it is
        /// declared once at file level, so entities only carry deviating namespaces
        /// (e.g. the external tf:: taskflow types).</summary>
        private void AddNamespace(OrderedDict target, string ns)
        {
            ns = NormalizeNamespace(ns);
            if (ns != null && ns != NormalizeNamespace(Options.LibraryInfo?.Namespace?.Raw))
            {
                target.Add("namespace", ns);
            }
        }

        private string QualifiedName(string name, string ns)
        {
            ns = NormalizeNamespace(ns);
            return ns != null && ns != NormalizeNamespace(Options.LibraryInfo?.Namespace?.Raw)
                ? ns + "::" + name
                : name;
        }

        private static string NormalizeNamespace(string ns)
        {
            // The parser reports explicit qualifications with trailing colons (daq::).
            ns = ns?.TrimEnd(':');
            return string.IsNullOrEmpty(ns) ? null : ns;
        }
    }
}
