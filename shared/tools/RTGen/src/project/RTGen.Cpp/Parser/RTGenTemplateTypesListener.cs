using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Antlr4.Runtime;
using Antlr4.Runtime.Misc;
using RTGen.Exceptions;
using RTGen.Interfaces;
using RTGen.Types;

namespace RTGen.Cpp.Parser
{
    class RTGenTemplateTypesListener : RTGen3CommonListener
    {
        private readonly Dictionary<string, ISampleTypes> _sampleTypes;

        private SampleTypes _types;
        private string _prevMacro;
        private int _typeLevel;

        public RTGenTemplateTypesListener(IParserOptions options)
            : base(options, new RTFile("::", ",", options.LibraryInfo))
        {
            _sampleTypes = new Dictionary<string, ISampleTypes>();
            File.Tag = _sampleTypes;

            _typeLevel = 0;
            _prevMacro = null;
        }

        public override void EnterMacro(RTGen3.MacroContext context)
        {
            string macro = context.MacroIdentifier().GetText();
            if (macro != "OPENDAQ_SAMPLE_TYPES")
            {
                _prevMacro = macro;
                return;
            }
            else if (_prevMacro != null && macro == "OPENDAQ_SAMPLE_TYPES")
            {
                var macroArguments = context.macroArguments();

                int backSlashes = macroArguments.BackSlash().Length + 1;
                if (backSlashes == 1)
                {
                    _types = new SampleTypes(_prevMacro, 1);
                }
                else
                {
                    int argCount = macroArguments.macroArg().Length;
                    if (argCount % backSlashes != 0)
                    {
                        IToken start = macroArguments.Start;
                        throw new ParserSemanticException(start.Line, start.Column, "Arguments per line do not match");
                    }

                    _types = new SampleTypes(_prevMacro, argCount / backSlashes);
                }

                _sampleTypes.Add(_prevMacro, _types);
            }
        }

        public override void EnterType(RTGen3.TypeContext context)
        {
            // Ignore template arguments (only whole types)
            if (++_typeLevel == 1)
            {
                TypeName typeName = GetTypeNameFromContext(context);
                AdjustValueTypes(typeName);

                _types?.AddType(typeName);
            }
        }

        private static void AdjustValueTypes(TypeName typeName)
        {
            if (!typeName.Flags.IsValueType)
            {
                return;
            }

            string unmappedName = typeName.UnmappedName;

            if (unmappedName == "uint8_t")
            {
                typeName.Name = "byte";
            }
            else if (unmappedName.EndsWith("_t", StringComparison.Ordinal))
            {
                typeName.Name = unmappedName.Substring(0, unmappedName.Length - 2);
            }
        }

        public override void ExitType(RTGen3.TypeContext context)
        {
            _typeLevel--;
        }

        public override void ExitMacro(RTGen3.MacroContext context)
        {
            _types = null;
        }
    }
}
