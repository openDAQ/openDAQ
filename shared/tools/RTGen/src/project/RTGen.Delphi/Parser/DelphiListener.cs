using System;
using System.Collections.Generic;
using System.Linq;
using Antlr4.Runtime.Misc;
using Antlr4.Runtime.Tree;
using RTGen.Interfaces;
using RTGen.Types;
using RTGen.Util;

namespace RTGen.Delphi.Parser
{
    class DelphiListener : DelphiParserBaseListener
    {
        private readonly TypeManager _typeManager;
        private string _unit;

        public DelphiListener()
        {
            File = new RTFile(".", ";", null);
            _typeManager = new TypeManager(File);
        }

        public RTFile File { get; }

        public override void EnterUnit([NotNull] DelphiParser.UnitContext context)
        {
            if (_unit != null)
            {
                throw new Exception("Unit should only be declared once.");
            }

            _unit = context.@namespace().GetText();
        }

        public override void EnterUsings([NotNull] DelphiParser.UsingsContext context)
        {
            DelphiParser.NamespaceContext[] namespaces = context.@namespace();
            foreach (DelphiParser.NamespaceContext namespaceContext in namespaces)
            {
                string @namespace = namespaceContext.GetText();
                File.Includes.Add(@namespace);
            }
        }

        public override void EnterRtFunc([NotNull] DelphiParser.RtFuncContext context)
        {
            string funcName = context.Identifier().GetText();

            RTAttribute attribute = new RTAttribute(funcName);
            DelphiParser.RtArgumentsContext argumentsContext = context.rtArguments();
            if (argumentsContext == null)
            {
                attribute.Arguments = new IRTAttributeArgument[0];
            }
            else
            {
                DelphiParser.RtArgContext[] argumentContexts = argumentsContext.rtArg();
                attribute.Arguments = argumentContexts.Select(ac => new RTAttributeArgument(ac.GetText(), null)).ToArray<IRTAttributeArgument>();
            }

            if (!File.AttributeInfo.HandleRtAttribute(attribute))
            {
                Log.Warning($"Ignoring unknown rtComment function: {funcName}.");
            }
        }

        public override void EnterGlobalFunctionDecl([NotNull] DelphiParser.GlobalFunctionDeclContext context)
        {
            DelphiParser.FunctionDeclContext functionContext = context.functionDecl();
            Method function = CreateFunction(functionContext);
            File.GlobalMethods.Add(function);
        }

        public override void EnterGlobalProcedureDecl([NotNull] DelphiParser.GlobalProcedureDeclContext context)
        {
            DelphiParser.ProcedureDeclContext procedureContext = context.procedureDecl();
            Method procedure = CreateProcedure(procedureContext);
            File.GlobalMethods.Add(procedure);
        }

        public override void EnterDelegateDecl([NotNull] DelphiParser.DelegateDeclContext context)
        {
            // ignored for now
        }

        public override void EnterTypeDecl([NotNull] DelphiParser.TypeDeclContext context)
        {
            if (_unit == null)
            {
                throw new Exception("Unit was not declared.");
            }
            if (!(context.children[0] is TerminalNodeImpl))
            {
                throw new Exception("First child of TypeDecl should be a terminal (the name).");
            }

            ITerminalNode[] identifiers = context.Identifier();

            // name
            string name = identifiers[0].GetText();

            // definition

            // type alias?
            if (identifiers.Length > 1)
            {
                string aliasTypeName = identifiers[1].GetText();
                TypeName aliasType = _typeManager.GetOrCreate(aliasTypeName);
                File.TypeAliases.Add(name, aliasType);

                // if the actual type is a value-type, treat the aliased type as value-type too
                if (File.AttributeInfo.ValueTypes.TryGet(name, out bool isValueType) && isValueType)
                {
                    File.AttributeInfo.ValueTypes.Add(aliasTypeName, true);
                }
                return;
            }

            // pointer type alias?
            {
                DelphiParser.PointerTypeContext pointerTypeContext = context.pointerType();
                if (pointerTypeContext != null)
                {
                    string namespaceName = "";
                    string pointerAliasTypeName = pointerTypeContext.Identifier().GetText();
                    string modifiers = pointerTypeContext.Caret().GetText();

                    TypeName pointerAliasType = _typeManager.GetAndSet(namespaceName, pointerAliasTypeName, modifiers);
                    File.TypeAliases.Add(name, pointerAliasType);

                    // if the actual type is a value-type, treat the aliased type as value-type too
                    if (File.AttributeInfo.ValueTypes.TryGet(name, out bool isValueType) && isValueType)
                    {
                        File.AttributeInfo.ValueTypes.Add(pointerAliasTypeName, true);
                    }
                    return;
                }
            }

            // enum?
            {
                DelphiParser.EnumTypeDeclContext enumTypeContext = context.enumTypeDecl();
                if (enumTypeContext != null)
                {
                    Enumeration newEnum = CreateEnumeration(name, enumTypeContext);
                    File.Enums.Add(newEnum);
                    return;
                }
            }

            // interface?
            {
                DelphiParser.InterfaceDeclContext interfaceContext = context.interfaceDecl();
                if (interfaceContext != null)
                {
                    var newInterface = new RTInterface();
                    {
                        newInterface.Type = _typeManager.GetAndSet(_unit, name, modifiers: null);
                    }
                    SetInterface(newInterface, interfaceContext);
                    File.Classes.Add(newInterface);
                    return;
                }
            }

            throw new Exception("Unsupported type definition.");
        }

        private void SetInterface(RTInterface interf, DelphiParser.InterfaceDeclContext context)
        {
            if (interf.Type == null)
            {
                throw new Exception("The Type of the interface should already be set.");
            }
            if (interf.BaseType != null)
            {
                throw new Exception("The BaseType of the interface should not already be set.");
            }

            if (context.ChildCount == 1)
            {
                // empty interface
                return;
            }

            // base type
            {
                string baseTypeName = context.Identifier().GetText();
                interf.BaseType = _typeManager.GetOrCreate(baseTypeName);
            }

            // guid?
            {
                DelphiParser.GuidDeclContext guidContext = context.guidDecl();
                if (guidContext != null)
                {
                    string guid = guidContext.String().GetText();
                    guid = guid.Replace("{", "").Replace("}", "").Replace("'", "");
                    interf.Type.Guid = Guid.Parse(guid);
                }
            }

            // functions
            {
                DelphiParser.FunctionDeclContext[] functionContexts = context.functionDecl();
                foreach (DelphiParser.FunctionDeclContext functionContext in functionContexts)
                {
                    IMethod newFunction = CreateFunction(functionContext);
                    interf.Methods.Add(newFunction);
                }
            }

            // procedures
            {
                DelphiParser.ProcedureDeclContext[] procedureContexts = context.procedureDecl();
                foreach (DelphiParser.ProcedureDeclContext procedureContext in procedureContexts)
                {
                    IMethod newProcedure = CreateProcedure(procedureContext);
                    interf.Methods.Add(newProcedure);
                }
            }
        }

        private Method CreateProcedure(DelphiParser.ProcedureDeclContext context)
        {
            var procedure = new Method
            {
                Name = context.Identifier().GetText(),
                CallingConvention = context.CallingConvention().GetText()
            };

            // arguments?
            {
                DelphiParser.FunctionParamsContext argumentsContext = context.functionParams();
                if (argumentsContext != null)
                {
                    IEnumerable<Types.Argument> arguments = CreateArguments(argumentsContext);
                    ((List<IArgument>)procedure.Arguments).AddRange(arguments);
                }
            }

            return procedure;
        }

        private Method CreateFunction(DelphiParser.FunctionDeclContext context)
        {
            var func = new Method();

            // name
            ITerminalNode[] identifiers = context.Identifier();
            {
                // name is optional
                if (identifiers.Length > 1)
                {
                    func.Name = identifiers[0].GetText();
                }
            }

            // arguments
            {
                DelphiParser.FunctionParamsContext paramsContext = context.functionParams();
                if (paramsContext != null)
                {
                    IEnumerable<Types.Argument> arguments = CreateArguments(paramsContext);
                    ((List<IArgument>)func.Arguments).AddRange(arguments);
                }
            }

            // return type
            {
                string returnTypeName = identifiers.Last().GetText();
                func.ReturnType = _typeManager.GetOrCreate(returnTypeName);
            }

            // calling convention
            {
                ITerminalNode callingConvention = context.CallingConvention();
                if (callingConvention != null)
                {
                    func.CallingConvention = callingConvention.GetText();

                    if (func.CallingConvention == "stdcall")
                    {
                        func.CallingConvention = "__stdcall";
                    }
                }
            }

            return func;
        }

        private IEnumerable<Types.Argument> CreateArguments(DelphiParser.FunctionParamsContext context)
        {
            DelphiParser.FunctionParamContext[] argumentContexts = context.functionParam();
            foreach (DelphiParser.FunctionParamContext argumentContext in argumentContexts)
            {
                yield return CreateArgument(argumentContext);
            }
        }

        private Types.Argument CreateArgument(DelphiParser.FunctionParamContext context)
        {
            var argument = new Types.Argument();

            // name
            ITerminalNode[] identifiers = context.Identifier();
            argument.Name = identifiers[0].GetText();

            // type?
            if (identifiers.Length > 1)
            {
                string typeName = identifiers[1].GetText();
                argument.Type = _typeManager.GetOrCreate(typeName);
            }

            // out?
            {
                var outParam = context.Out();
                if (outParam != null)
                {
                    argument.IsOutParam = true;
                }
            }

            return argument;
        }

        private Enumeration CreateEnumeration(string name, DelphiParser.EnumTypeDeclContext context)
        {
            var newEnum = new Enumeration(name);

            DelphiParser.EnumTypePartContext[] optionContexts = context.enumTypePart();
            foreach (DelphiParser.EnumTypePartContext optionContext in optionContexts)
            {
                var option = new EnumOption
                {
                    Name = optionContext.Identifier().GetText()
                };

                ITerminalNode value = optionContext.PrimitiveValue();
                if (value != null)
                {
                    option.Value = value.GetText();
                }
                newEnum.Options.Add(option);
            }

            File.AttributeInfo.ValueTypes.Add(name, true);

            return newEnum;
        }

        private class TypeManager
        {
            private readonly Dictionary<string, string> _nameMapping = new Dictionary<string, string>
            {
                { "Boolean", "daq::Bool" },
                { "RtInt", "daq::Int" },
                { "Cardinal", "uint32_t" },
                { "SizeT", "daq::SizeT" },
                { "ErrCode", "daq::ErrCode"}
            };

            private readonly Dictionary<string, TypeName> _types;
            private readonly RTFile _file;

            public TypeManager(RTFile file)
            {
                _file = file;
                _types = new Dictionary<string, TypeName>();
            }

            public TypeName GetOrCreate(string name)
            {
                name = GetMappedName(name);

                if (_types.TryGetValue(name, out TypeName type))
                {
                    return type;
                }

                return CreateNew(_file.AttributeInfo, "", name, modifiers: null);
            }

            public TypeName GetAndSet(string namespaceName, string name, string modifiers)
            {
                name = GetMappedName(name);

                if (_types.TryGetValue(name, out TypeName type))
                {
                    type.Namespace.Raw = namespaceName;
                    type.Modifiers = modifiers;
                    return type;
                }

                return CreateNew(_file.AttributeInfo, namespaceName, name, modifiers);
            }

            private TypeName CreateNew(IAttributeInfo info, string namespaceName, string name, string modifiers)
            {
                // if it is an interface, it should be marked with a pointer modifier
                if (name.StartsWith("I"))
                {
                    if (modifiers == null)
                    {
                        modifiers = "";
                    }
                    modifiers += "*";
                }

                var newType = new TypeName(info, namespaceName, name, modifiers);
                _types.Add(name, newType);
                return newType;
            }

            private string GetMappedName(string name)
            {
                if (_nameMapping.TryGetValue(name, out string mappedName))
                {
                    return mappedName;
                }
                return name;
            }
        }
    }
}
