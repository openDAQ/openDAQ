using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Linq;

using Antlr4.Runtime;
using Antlr4.Runtime.Misc;
using Antlr4.Runtime.Tree;
using RTGen.Exceptions;
using RTGen.Interfaces;
using RTGen.Interfaces.Doc;
using RTGen.Types;
using RTGen.Types.Doc;
using RTGen.Util;

namespace RTGen.Cpp.Parser {

    class TemplateFactoryInfo
    {
        public string Name { get; set; }
        public string InterfaceName { get; set; }
    }

    enum ParseState
    {
        Leading,
        Class,
        Method,
        Trailing
    }

    public class RTGenListener : RTGen3CommonListener
    {
        private const string GUID_TYPE_NAME = "IntfID";
        private const string GUID_MACRO = "DEFINE_INTFID";
        private const string GUID_MACRO_EXTERNAL = "DEFINE_EXTERNAL_INTFID";
        private const string GUID_MACRO_CUSTOM = "DEFINE_CUSTOM_INTFID";
        private const string GUID_MACRO_LEGACY = "DEFINE_LEGACY_INTFID";

        private readonly Dictionary<string, Guid> _guids;

        private ParseState _state;

        public RTGenListener(IParserOptions options)
            : base(options, new RTFile("::", ",", options.LibraryInfo, options.CoreNamespace))
        {
            _guids = new Dictionary<string, Guid>();
            _state = ParseState.Leading;

            File.AttributeInfo.PtrMappings.Add("void", new SmartPtr("void*", false));
        }

        /// <summary>
        /// Enter a parse tree produced by <see cref="RTGen3.classDecl"/>.
        /// </summary>
        /// <param name="context">The parse tree.</param>
        public override void EnterClassDecl(RTGen3.ClassDeclContext context)
        {
            if (context.classImpl() == null)
            {
                return;
            }

            _state = ParseState.Class;

            var classType = context.classType();

            RTInterface intf = new RTInterface
            {
                Type = GetTypeNameFromContext(classType.interfaceType),
            };

            int lastCommentIndex = File.LeadingDocumentation.Count - 1;
            if (lastCommentIndex >= 0)
            {
                intf.Documentation = File.LeadingDocumentation[lastCommentIndex];
                File.LeadingDocumentation.RemoveAt(lastCommentIndex);
            }

            string intfMacro = classType.DeclareRtIntf()?.GetText() ?? string.Empty;
            if (intfMacro.Contains("_TEMPLATED_"))
            {
                if (intfMacro.EndsWith("_T_U"))
                {
                    intf.Type.GenericArguments = new List<ITypeName>
                    {
                        TypeName.GenericArgument(File.AttributeInfo, "T"),
                        TypeName.GenericArgument(File.AttributeInfo, "U")
                    };
                }
                else if (intfMacro.EndsWith("_T"))
                {
                    intf.Type.GenericArguments = new List<ITypeName>
                    {
                        TypeName.GenericArgument(File.AttributeInfo, "T")
                    };
                }
            }
            File.Classes.Add(intf);

            // Interface template arguments must be defined at this point
            intf.BaseType = GetTypeNameFromContext(classType.baseType);

            GetAndSetClassGuid(context.classImpl(), intf);
            ParseNextClassAttributes(intf);
        }

        public override void ExitClassDecl(RTGen3.ClassDeclContext context)
        {
            if (context.classImpl() != null)
            {
                ParseGetSetPairs(File.CurrentClass);
                _state = ParseState.Trailing;
            }
        }

        private void ParseGetSetPairs(IRTInterface currentClass)
        {
            Dictionary<string, IGetSet> properties = new Dictionary<string, IGetSet>();
            foreach (IMethod method in currentClass.Methods)
            {
                var prefixLength = -1;
                if (method.Name.StartsWith("get", StringComparison.OrdinalIgnoreCase))
                {
                    prefixLength = 3;
                }
                else if (method.Name.StartsWith("is", StringComparison.OrdinalIgnoreCase))
                {
                    prefixLength = 2;
                }

                bool isGetterName = prefixLength > 0;

                if (isGetterName
                    && method.Arguments.Count == 1
                    && method.ReturnsByRef())
                {
                    var propName = method.Name.Substring(prefixLength);
                    if (properties.TryGetValue(propName, out IGetSet prop))
                    {
                        prop.Getter = method;
                    }
                    else
                    {
                        prop = GetSet.AsGetter(propName, method);
                        properties.Add(propName, prop);
                    }
                    method.GetSetPair = prop;
                }
                else if (method.Name.StartsWith("set", StringComparison.OrdinalIgnoreCase)
                         && method.Arguments.Count < 2
                         && method.ReturnType.Name.ToLowerInvariant() == @"errcode")
                {
                    var propName = method.Name.Substring(3);
                    if (properties.TryGetValue(propName, out var prop))
                    {
                        prop.Setter = method;
                    }
                    else
                    {
                        prop = GetSet.AsSetter(propName, method);
                        properties.Add(propName, prop);
                    }

                    method.GetSetPair = prop;
                }
            }

            currentClass.GetSet = properties.Values.ToList();
        }

        private void ParseNextClassAttributes(RTInterface intf)
        {
            string interfaceName = intf.Type.Name;

            ISmartPtr smartPtr = null;
            if (Attributes.Next.Type.IsTemplated)
            {
                if (Attributes.PtrMappings.TryGet(interfaceName, out ISmartPtr ptr))
                {
                    smartPtr = ptr;
                    ptr.IsTemplated = true;
                }
                else
                {
                    Attributes.PtrMappings.Add(interfaceName, new SmartPtr(null, true));

                    // must re-query the mappings (possible merge)
                    smartPtr = Attributes.PtrMappings[interfaceName];
                }

                if (Attributes.Next.Type.TryGet(AttributeInfo.DEFAULT_TEMPLATE_ALIAS, out bool defaultAlias))
                {
                    smartPtr.HasDefaultAlias = defaultAlias;

                    if (defaultAlias && Attributes.Next.Type.TryGet(AttributeInfo.DEFAULT_TEMPLATE_ALIAS_NAME, out string aliasName))
                    {
                        smartPtr.DefaultAliasName = aliasName;
                    }
                }
                else
                {
                    Attributes.PtrMappings.Get(interfaceName).HasDefaultAlias = true;
                }
                Attributes.Next.Type.IsTemplated = false;
            }

            if (smartPtr == null)
            {
                Attributes.PtrMappings.Add(interfaceName, new SmartPtr(null, false));

                // must re-query the mappings (possible merge)
                smartPtr = Attributes.PtrMappings[interfaceName];
            }

            if (Attributes.Next.Type.TryGet("IsDefaultPtr", out bool isDefault))
            {
                smartPtr.IsDefaultPtr = isDefault;

                if (string.IsNullOrEmpty(smartPtr.Name) && !smartPtr.IsDefaultPtr)
                {
                    smartPtr.Name = intf.Type.Wrapper.Name + "Base";
                }
            }

            if (Attributes.Next.Type.IsUiControl)
            {
                ((TypeFlags) intf.Type.Flags).IsUiControl = true;
                Attributes.Next.Type.IsUiControl = false;
            }

            if (Attributes.Next.Type.PropertyInfo != null)
            {
                if (string.IsNullOrEmpty(Attributes.Next.Type.PropertyInfo.ClassName))
                {
                    Attributes.Next.Type.PropertyInfo.ClassName = intf.Type.NonInterfaceName;
                }

                ((TypeName) intf.Type).PropertyClass = Attributes.Next.Type.PropertyInfo;
                if (string.IsNullOrEmpty(intf.Type.PropertyClass.ParentClassName))
                {
                    intf.Type.PropertyClass.ParentClassName = intf.BaseType.NonInterfaceName;
                }

                Attributes.Next.Type.PropertyInfo = null;
            }

            if (Attributes.Next.Type.IsDecorated)
            {
                ((TypeFlags)intf.Type.Flags).IsDecorated = true;
            }

            if (Attributes.Next.Type.TryGet("IsCoreConfig", out bool isCoreConfig))
            {
                ((TypeFlags)intf.Type.Flags).IsCoreConfig = true;
            }
        }

        /// <summary>Tries to find the specified interface <see cref="Guid"/> from previously defined variables.</summary>
        /// <param name="context">The interface/class parse tree.</param>
        /// <param name="intf">The interface object representation for which to find the <see cref="Guid"/>.</param>
        private void GetAndSetClassGuid(RTGen3.ClassImplContext context, IRTInterface intf)
        {
            RTGen3.ClassMembersContext[] classMembers = context.classMembers();

            string guidVarName = null;
            string namespaceName = null;
            INamespace baseNamespace = null;
            foreach (RTGen3.ClassMembersContext classMember in classMembers)
            {
                RTGen3.MacroContext macro = classMember.macro();

                RTGen3.MacroArgumentsContext arguments = macro?.macroArguments();
                string macroName = macro?.MacroIdentifier().GetText();

                //we look for a macro with arguments
                if ((arguments == null) || string.IsNullOrEmpty(macroName))
                {
                    continue;
                }

                bool custom = false;
                bool legacy = false;
                if (macroName != GUID_MACRO && macroName != GUID_MACRO_EXTERNAL)
                {
                    if (macroName == GUID_MACRO_CUSTOM && arguments.macroArg().Length == 2)
                    {
                        custom = true;
                    }
                    else if (macroName == GUID_MACRO_LEGACY && arguments.macroArg().Length == 1)
                    {
                        legacy = true;
                    }
                    else
                    {
                        //unknown macro or unexpected number of arguments
                        continue;
                    }
                }

                if (custom)
                {
                    guidVarName = arguments.macroArg()[0].GetText();
                    namespaceName = arguments.macroArg()[1].GetText(); //already reversed namespace name
                }
                else if (legacy)
                {
                    guidVarName = arguments.macroArg()[0].GetText();
                    baseNamespace = new Namespace(AttributeInfo.DEFAULT_LEGACY_NAMESPACE);
                }
                else
                {
                    guidVarName = arguments.GetText();
                    baseNamespace = new Namespace(AttributeInfo.DEFAULT_CORE_NAMESPACE);
                }

                //now we have namespaceName and guidVarName so we can end the loop
                break;
            }

            //there was no GUID macro?
            if (string.IsNullOrEmpty(guidVarName))
            {
                guidVarName = intf.Type.Name;
                baseNamespace = new Namespace(AttributeInfo.DEFAULT_CORE_NAMESPACE);

                //use the given namespace for non-default-namespace files
                if (!File.StartNamespaceMacro.Equals(AttributeInfo.DEFAULT_CORE_NAMESPACE_MACRO))
                {
                    baseNamespace = intf.Type.UnmappedNamespace
                                    ?? Options.LibraryInfo.Namespace
                                    ?? baseNamespace; //fallback
                }
            }

            if (baseNamespace != null)
            {
                namespaceName = string.Join(".", baseNamespace.Components.Reverse());
            }

            guidVarName = guidVarName.Trim('"');

            if (_guids.TryGetValue(guidVarName, out Guid guid))
            {
                intf.Type.Guid = guid;
            }
            else
            {
                intf.Type.Guid = Utility.InterfaceUuid(guidVarName + "." + namespaceName);
            }
        }

        private uint ParseExpression(RTGen3.ExpressionContext expression, Enumeration enumDef)
        {
            if (expression.ChildCount > 1)
            {
                return GetExpressionValue(expression, enumDef);
            }

            string value = expression.GetText();
            if (value.StartsWith("0x", StringComparison.OrdinalIgnoreCase))
            {
                value = value.Substring(2);
            }
            else if (value.StartsWith("0b", StringComparison.OrdinalIgnoreCase))
            {
                try
                {
                    value = Convert.ToInt32(value.Substring(2), 2).ToString(CultureInfo.InvariantCulture);
                }
                catch
                {
                    Log.Warning($"Unable to parse binary enum value \"{value}\".");
                }
            }

            if (!uint.TryParse(value, NumberStyles.HexNumber, CultureInfo.InvariantCulture, out uint parsedValue))
            {
                return uint.Parse(enumDef.Options.Where(opt => opt.Name == expression.GetText())
                                         .Select(opt => opt.Value)
                                         .First());
            }
            return parsedValue;
        }

        private uint GetExpressionValue(RTGen3.ExpressionContext expression, Enumeration enumDef)
        {
            uint lhs = ParseExpression(expression.expression(0), enumDef);
            uint rhs = ParseExpression(expression.expression(1), enumDef);

            switch (expression.binOperator().GetText())
            {
                case "&":
                    return lhs & rhs;
                case "|":
                    return lhs | rhs;
                default:
                    throw new ParserException("Unknown binary operator.");
            }
        }

        private bool TryGetEnumValue(RTGen3.ExpressionContext value, Enumeration enumDef, EnumOption opt, out int counterValue)
        {
            if (value.ChildCount > 1)
            {
                counterValue = (int) GetExpressionValue(value, enumDef);
                return true;
            }

            NumberStyles numberStyle = NumberStyles.Integer;

            opt.Value = value.GetText();

            if (opt.Value.StartsWith("0x", StringComparison.OrdinalIgnoreCase))
            {
                numberStyle = NumberStyles.HexNumber;
                opt.Value = opt.Value.Substring(2);
            }
            else if (opt.Value.StartsWith("0b", StringComparison.OrdinalIgnoreCase))
            {
                try
                {
                    opt.Value = Convert.ToInt32(opt.Value.Substring(2), 2).ToString(CultureInfo.InvariantCulture);
                }
                catch
                {
                    Log.Warning($"Unable to parse binary enum value \"{opt.Value}\".");
                }
            }

            if (!int.TryParse(opt.Value, numberStyle, CultureInfo.InvariantCulture, out int parsedCounter))
            {
                counterValue = 0;
                return false;
            }

            //store the integer value due to language differences for hex (0x1 or $1)
            opt.Value = parsedCounter.ToString(CultureInfo.InvariantCulture);
            counterValue = parsedCounter;

            return true;
        }

        /// <summary>
        /// Enter a parse tree produced by <see cref="RTGen3.enumDecl"/>.
        /// <para>The default implementation does nothing.</para>
        /// </summary>
        /// <param name="context">The parse tree.</param>
        public override void EnterEnumDecl(RTGen3.EnumDeclContext context)
        {
            string enumName = context.Identifier().GetText();

            Enumeration enumDef = new Enumeration(enumName);

            RTGen3.EnumMembersContext options = context.enumImpl()?.enumMembers();
            if (options == null)
            {
                File.Enums.Add(enumDef);
                return;
            }

            long counter = -1;
            foreach (RTGen3.EnumMemberContext option in options.enumMember())
            {
                EnumOption opt = new EnumOption {
                    Name = option.Identifier().GetText()
                };

                RTGen3.EnumValueContext value = option.enumValue();
                if (value != null)
                {
                    if (value.expr() != null)
                    {
                        if (!TryGetEnumValue(value.expr().expression(), enumDef, opt, out int parsedCounter))
                        {
                            counter++;
                            Log.Warning($"Unable to parse enum value \"{opt.Value}\" incrementing the enum value to: {counter}.");
                        }
                        else
                        {
                            counter = parsedCounter;
                        }
                    }
                    else
                    {
                        counter++;
                        opt.Value = counter.ToString(CultureInfo.InvariantCulture);

                        Log.Warning(counter != 0
                                        ? $"Macro \"{value.GetText()}\" used as an enum value! Macro is NOT evaluated but assumed to increment by 1 [from {counter - 1} to {counter}]."
                                        : $"Macro \"{value.GetText()}\" used as an enum value! Macro is NOT evaluated but Enum is assumed to start with {counter}.");
                    }
                }
                else
                {
                    counter++;
                }

                enumDef.Options.Add(opt);
            }

            File.AttributeInfo.ValueTypes.Add(enumName, true);

            File.Enums.Add(enumDef);
        }

        /// <summary>
        /// Exit a parse tree produced by <see cref="RTGen3.methodDecl"/>.
        /// </summary>
        /// <param name="context">The parse tree.</param>
        public override void ExitMethodDecl(RTGen3.MethodDeclContext context)
        {
            Method method = new Method();

            RTGen3.MethodModifiersContext modifiers = context.methodModifiers();
            if (modifiers != null) {
                method.Modifiers.Add(modifiers.GetText());
            }

            var constContext = context.Const();
            if (constContext != null)
            {
                method.Modifiers.Add(constContext.GetText());
            }

            method.ReturnType = GetTypeNameFromContext(context.type());

            ITerminalNode callingConvention = context.CallingConvention();
            if (callingConvention != null) {
                method.CallingConvention = callingConvention.GetText();
            }

            method.Name = context.Identifier().GetText();
            method.IsPure = context.pureFuncDecl() != null;

            GetMethodArguments(method, context.arguments());

            // Check if its an event subscribe/unsubscribe
            if (Attributes.Next.Method.EventInfo != null)
            {
                AddEventMethod(method, Attributes.Next.Method.EventInfo);
            }

            // Check if its a PropertyObject method
            IRTInterface currentClass = File.CurrentClass;

            //if (info.NextMethodPropertyInfo)
            if (currentClass.Type.PropertyClass != null)
            {
                method.Property = new Property(method, Attributes.Next.Method.PropertyInfo);
            }

            method.IsIgnored = Attributes.Next.Method.IsIgnored;
            method.ReturnSelf = Attributes.Next.Method.ReturnSelf;
            method.Documentation = Attributes.Next.Method.Documentation;
            method.OverloadFor = Attributes.Next.Method.OverloadFor;

            foreach (var eval in Attributes.Next.Method.Polymorphics)
            {
                var argument = method.Arguments.FirstOrDefault(arg => arg.Name == eval.Key && !arg.IsOutParam);
                if (argument == null)
                    continue;
                argument.IsPolymorphic = true;
            }

            currentClass.Methods.Add(method);
            Attributes.Next.Method.Reset();
        }

        private void AddEventMethod(IMethod method, MethodEventInfo info)
        {
            IDictionary<string, IEvent> events = File.CurrentClass.Events;

            if (!events.TryGetValue(info.EventName, out IEvent eventInfo))
            {
                eventInfo = new Event
                {
                    EventArgsType = info.EventArgsType
                };

                events.Add(info.EventName, eventInfo);
            }

            switch (info.MethodType)
            {
                case EventMethodType.Add:
                    eventInfo.AddMethod = method;
                    break;
                case EventMethodType.Remove:
                    eventInfo.RemoveMethod = method;
                    break;
            }

            if (eventInfo.EventArgsType == null)
            {
                eventInfo.EventArgsType = info.EventArgsType;
            }
        }

        /// <summary>
        /// Exit a parse tree produced by <see cref="RTGen3.varDecl"/>.
        /// </summary>
        /// <param name="context">The parse tree.</param>
        public override void ExitVarDecl(RTGen3.VarDeclContext context)
        {
            // Check for possible GUID matches

            TypeName type = GetTypeNameFromContext(context.type());
            if (type.Name != GUID_TYPE_NAME) {
                return;
            }

            // Check that GUID is 'static const'
            RTGen3.VarModifiersContext modifiers = context.varModifiers();
            if (modifiers == null || modifiers.ChildCount != 2) {
                return;
            }

            bool hasStatic = false;
            bool hasConst = false;

            foreach (RTGen3.VarModifierContext modifier in modifiers.varModifier()) {
                if (modifier.Static() != null) {
                    hasStatic = true;
                }
                else if ((modifier.Const() != null) || (modifier.Constexpr() != null)) { //headers contain 'static constexpr' for GUIDs
                    hasConst = true;
                }
            }

            if (!hasConst || !hasStatic) {
                return;
            }

            RTGen3.AssignmentContext assignment = context.assignment();
            if (assignment == null) {
                return;
            }

            try {
                Guid guid = Guid.Parse(assignment.expression().GetText()
                                       .Replace("{{", "{")     //somehow the C++ headers contain one extra pair of {}
                                       .Replace("}}}", "}}")); //which the Guid parser does not understand
                _guids.Add(context.Identifier().GetText(), guid);
            }
            catch (FormatException) {
                // Not a valid GUID;
            }
        }

        /// <summary>Creates object representations of the parsed method's arguments.</summary>
        /// <param name="method">The method to which to add the arguments.</param>
        /// <param name="context">The arguments parse tree.</param>
        private void GetMethodArguments(Method method, RTGen3.ArgumentsContext context)
        {
            if (context == null) {
                return;
            }

            // List<IArray> needParameterIndex = new List<IArray>();
            foreach (RTGen3.ArgContext argument in context.arg())
            {
                bool isConst = false;
                var argValue = argument.argValue();

                if (argValue.GetChild(0) is ITerminalNode firstToken && firstToken.Symbol.Type == RTGen3Lexer.Const)
                {
                    isConst = true;
                }

                string argName = argValue.Identifier().GetText();
                Attributes.Next.Method.Arguments.TryGet(argName, out IArgumentInfo argInfo);

                IArgument arg = new Argument(
                    GetTypeNameFromContext(argValue.type()),
                    argName,
                    isConst,
                    argInfo?.ArrayInfo,
                    false,
                    argInfo?.StealRef ?? false
                );

                if (argInfo?.ElementTypes != null)
                {
                    arg.Type.GenericArguments = argInfo.ElementTypes;
                }

                var defaultValue = argument.defaultArgValue();
                if (defaultValue != null)
                {
                    ((Argument)arg).DefaultValue = defaultValue.literal().GetText();
                }

                method.Arguments.Add(arg);
            }
        }

        /// <summary>
        /// Exit a parse tree produced by <see cref="RTGen3.typedefDecl"/>.
        /// <para>The default implementation does nothing.</para>
        /// </summary>
        /// <param name="context">The parse tree.</param>
        public override void ExitTypedefDecl(RTGen3.TypedefDeclContext context)
        {
            // if the actual type is a value-type, treat the aliased type as value-type too

            string actualType = context.type(0).GetText();
            string aliasType = context.type(1).GetText();

            if (File.AttributeInfo.ValueTypes.TryGet(actualType, out bool isValueType) && isValueType)
            {
                File.AttributeInfo.ValueTypes.Add(aliasType, true);
            }
        }

        public override void ExitUsingDecl(RTGen3.UsingDeclContext context)
        {
            File.TypeAliases.Add(context.Identifier().GetText(), GetTypeNameFromContext(context.type()));
        }

        /// <summary>
        /// Enter a parse tree produced by <see cref="RTGen3.macro"/>.
        /// </summary>
        /// <param name="context">The parse tree.</param>
        public override void EnterMacro(RTGen3.MacroContext context)
        {
            string macroName = context.MacroIdentifier().GetText();

            if (context.macroArguments() == null)
            {
                string macroUpper = macroName.ToUpperInvariant();

                if (macroUpper.StartsWith("BEGIN_"))
                {
                    File.StartNamespaceMacro = macroName;
                }

                if (macroUpper.StartsWith("END_"))
                {
                    File.EndNamespaceMacro = macroName;
                }
            }
            else if (macroName.StartsWith("OPENDAQ_DECLARE_CLASS_FACTORY"))
            {
                ParseFactory(macroName, context.macroArguments());
            }
        }

        private string GetCreateFuncFromObjName(string objName)
        {
            return $"create{objName}";
        }

        private string GetInterfaceNameFromFactory(string intfName)
        {
            return intfName.Split(new[] { "::" }, StringSplitOptions.RemoveEmptyEntries).Last();
        }

        private TypeName GetTypeNameFromFactoryContext(RTGen3.TypeContext context)
        {
            TypeName typeName = GetTypeNameFromContext(context);
            if (typeName.Namespace.Components.Length == 0)
            {
                typeName.Namespace.Raw = File.CurrentClass.Type.Namespace.Raw;
            }

            typeName.CalculateGuid();

            return typeName;
        }

        private Argument GetArgumentFromFactoryInterfaceName(string interfaceName)
        {
            TypeName intfTypeName = new TypeName(File.AttributeInfo, File.CurrentClass.Type.Namespace.Raw, interfaceName, "**");
            Argument intfArgName = new Argument(intfTypeName, "obj");

            return intfArgName;
        }

        private List<IArgument> ParseFactoryArguments(RTGen3.MacroArgContext[] args, int start, string interfaceName)
        {
            List<IArgument> factoryArgs = new List<IArgument> { GetArgumentFromFactoryInterfaceName(interfaceName) };

            for (int i = start; i < args.Length; i += 2)
            {
                TypeName argType = GetTypeNameFromFactoryContext(args[i].macroArgValue().type());
                string argName = args[i + 1].GetText();

                Attributes.Next.Method.Arguments.TryGet(argName, out IArgumentInfo argInfo);

                IArgument arg = new Argument(
                    argType,
                    argName
                );

                if (argInfo?.ElementTypes != null)
                {
                    arg.Type.GenericArguments = argInfo.ElementTypes;
                }

                factoryArgs.Add(arg);
            }

            return factoryArgs;
        }

        private RTFactory GetFactory(RTGen3.MacroArgContext[] args)
        {
            if (args.Length < 2)
            {
                throw new ParserSemanticException(args[0].Start.Line, args[0].Start.Column, "Factory function missing required parameters.");
            }

            string factoryName = args[1].GetText();
            string interfaceName = "I" + GetInterfaceNameFromFactory(args[1].GetText());
            string createFuncName = GetCreateFuncFromObjName(args[1].GetText());

            int argCount = args.Length - 2;
            if (argCount % 2 != 0 && argCount > 2)
            {
                throw new ParserSemanticException(args[2].Start.Line, args[2].Start.Column, "Factory parameter name and type do not match");
            }

            RTFactory factory = new RTFactory(factoryName,
                                              interfaceName,
                                              createFuncName,
                                              ParseFactoryArguments(args, 2, interfaceName));
            return factory;
        }

        private List<IArgument> ParseTemplateFactoryParams(RTGen3.MacroArgValueContext args)
        {
            var macro = args.macro();
            if (macro == null)
            {
                throw new ParserSemanticException(args.Start.Line, args.Start.Column, "Missing template params");
            }

            string paramsMacro = macro.MacroIdentifier().GetText();
            if (paramsMacro != "OPENDAQ_FACTORY_PARAMS")
            {
                throw new ParserSemanticException(args.Start.Line, args.Start.Column, $"Unknown or missing template params: \"{paramsMacro}\"");
            }

            return macro.macroArguments() != null
                       ? ParseTemplateFactoryArguments(macro.macroArguments())
                       : new List<IArgument>();
        }

        private List<IArgument> ParseTemplateFactoryArguments(RTGen3.MacroArgumentsContext macroArguments)
        {
            List<IArgument> arguments = new List<IArgument>();

            TypeName typeName = null;
            foreach (var macroArg in macroArguments.macroArg())
            {
                var argValue = macroArg.macroArgValue();

                var childContext = argValue.GetRuleContext<ParserRuleContext>(0);
                switch (childContext.RuleIndex)
                {
                    case RTGen3.RULE_macro:
                    {
                        RTGen3.MacroContext macroContext = ((RTGen3.MacroContext) childContext);
                        if (macroContext.macroArguments() != null)
                        {

                            var typeContext = macroContext.macroArguments()
                                                          .macroArg(0)
                                                          .macroArgValue()
                                                          .type();

                            typeName = GetTypeNameFromFactoryContext(typeContext);
                            AddGenericArguments(macroContext, typeName);
                        }
                        else
                        {
                            switch (macroContext.GetText())
                            {
                                case "OPENDAQ_TEMPLATE_T":
                                    typeName = TypeName.GenericArgument(Attributes, "T");
                                    break;
                                case "OPENDAQ_TEMPLATE_U":
                                    typeName = TypeName.GenericArgument(Attributes, "U");
                                    break;
                                default:
                                    throw new ParserSemanticException(macroContext.Start.Line, macroContext.Start.Column, $"Unknown Factory parameter macro: {macroContext.GetText()}.");
                            }
                        }
                        break;
                    }
                    case RTGen3.RULE_type:
                    case RTGen3.RULE_literal:
                    {
                        var paramName = childContext.GetText();
                        if (typeName == null)
                        {
                            throw new ParserSemanticException(childContext.Start.Line,
                                                              childContext.Start.Column,
                                                              $"Factory argument without type-definition: \"{paramName}\"");
                        }

                        arguments.Add(new Argument(typeName, paramName));
                        typeName = null;
                        break;
                    }
                    default:
                        throw new ParserSemanticException(argValue.Start.Line,
                                                          argValue.Start.Column,
                                                          $"Unknown macro argument type: \"{argValue.GetText()}\"");
                }
            }

            return arguments;
        }

        private void AddGenericArguments(RTGen3.MacroContext macroContext, TypeName typeName)
        {
            switch (macroContext.MacroIdentifier().GetText())
            {
                case "OPENDAQ_TEMPLATED_PTR_T":
                {
                    typeName.Modifiers = "*";
                    typeName.GenericArguments = new List<ITypeName>
                    {
                        TypeName.GenericArgument(File.AttributeInfo, "T")
                    };
                    break;
                }
                case "OPENDAQ_TEMPLATED_PTR_T_U":
                {
                    typeName.Modifiers = "*";
                    typeName.GenericArguments = new List<ITypeName>
                    {
                        TypeName.GenericArgument(File.AttributeInfo, "T"),
                        TypeName.GenericArgument(File.AttributeInfo, "U"),
                    };
                    break;
                }
                case "OPENDAQ_PTR":
                    typeName.Modifiers = "*";
                    break;
            }
        }

        private bool ParseTemplateFactoryInfo(RTGen3.MacroArgContext macroArgContext, out TemplateFactoryInfo factory)
        {
            factory = null;

            RTGen3.MacroContext factoryInfo = macroArgContext.macroArgValue().macro();
            if (factoryInfo == null || factoryInfo.macroArguments()?.macroArg(0).GetText() != "LIBRARY_FACTORY")
            {
                return false;
            }

            RTGen3.MacroArgContext[] factoryInfoArgs = factoryInfo.macroArguments().macroArg();

            factory = new TemplateFactoryInfo
            {
                InterfaceName = factoryInfoArgs[2].macroArgValue()
                                                  .macro()?.macroArguments()
                                                  .GetText(),
                Name = factoryInfoArgs[1].GetText()
            };

            return true;
        }


        private List<IRTFactory> GetFactoryTemplate1(RTGen3.MacroArgContext[] args)
        {
            if (args.Length != 3)
            {
                throw new ParserSemanticException(args[0].Start.Line, args[0].Start.Column, "Factory function missing required parameters.");
            }

            string sampleTypesMacro = args[1].GetText();

            if (!ParseTemplateFactoryInfo(args[0], out TemplateFactoryInfo factory) ||
                !Options.PredefinedTypeArguments.TryGetValue(sampleTypesMacro, out ISampleTypes sampleTypes))
            {
                return null;
            }

            if (sampleTypes.Rank != 1)
            {
                throw new ParserException($"Sample type rank of \"{sampleTypesMacro}\" is {sampleTypes.Rank} but factory needs 1. Please fix the source.");
            }

            List<IArgument> factoryArgs = ParseTemplateFactoryParams(args[2].macroArgValue());
            factoryArgs.Insert(0, new Argument(new TypeName(File.AttributeInfo,
                                                      Options.LibraryInfo.Namespace?.Raw,
                                                      factory.InterfaceName,
                                                      "**"
                                                   )
                                               {
                                                   GenericArguments = new List<ITypeName>
                                                   {
                                                       TypeName.GenericArgument(File.AttributeInfo, "T"),
                                                   }
                                               }
                                               , factory.Name));

            List<IRTFactory> genericFactories = new List<IRTFactory>
            {
                new RTGenericFactory(factory.Name,
                                     factory.InterfaceName,
                                     factoryArgs,
                                     sampleTypes)
            };

            return genericFactories;
        }

        private List<IRTFactory> GetFactoryTemplate2(RTGen3.MacroArgContext[] args)
        {
            if (args.Length != 3)
            {
                throw new ParserSemanticException(args[0].Start.Line, args[0].Start.Column, "Factory function missing required parameters.");
            }

            string sampleTypesMacro = args[1].GetText();

            if (!ParseTemplateFactoryInfo(args[0], out TemplateFactoryInfo factory) ||
                !Options.PredefinedTypeArguments.TryGetValue(sampleTypesMacro, out ISampleTypes sampleTypes))
            {
                return null;
            }

            if (sampleTypes.Rank != 2)
            {
                throw new ParserException($"Sample type rank of \"{sampleTypesMacro}\" is {sampleTypes.Rank} but factory needs 2. Please fix the source.");
            }

            List<IArgument> factoryArgs = ParseTemplateFactoryParams(args[2].macroArgValue());
            factoryArgs.Insert(0, new Argument(new TypeName(File.AttributeInfo,
                                                      Options.LibraryInfo.Namespace?.Raw,
                                                      factory.InterfaceName,
                                                      "**"
                                                   )
                                               {
                                                   GenericArguments = new List<ITypeName>
                                                   {
                                                       TypeName.GenericArgument(File.AttributeInfo, "T"),
                                                       TypeName.GenericArgument(File.AttributeInfo, "U"),
                                                   }
                                               }
                                               , factory.Name));

            List<IRTFactory> genericFactories = new List<IRTFactory>
            {
                new RTGenericFactory(factory.Name,
                                     factory.InterfaceName,
                                     factoryArgs,
                                     sampleTypes) { Options = FactoryOptions.NoDeclaration }
            };

            return genericFactories;
        }

        private RTFactory GetFactoryWithInterface(RTGen3.MacroArgContext[] args)
        {
            if (args.Length < 3)
            {
                throw new ParserException($"Line {args[0].Start.Line},{args[0].Start.Column}: Factory function missing required parameters.");
            }

            int argStart = 3;
            string factoryName = args[1].GetText();
            string interfaceName = GetInterfaceNameFromFactory(args[2].GetText());
            string createFuncName = GetCreateFuncFromObjName(args[1].GetText());

            // Actual factory argument count
            int argCount = args.Length - 3;
            if (argCount % 2 != 0)
            {
                var firstFactoryParam = args[3];
                if (argCount > 0 && firstFactoryParam.GetText() != "OPENDAQ_FACTORY_PARAMS()")
                {
                    throw new ParserSemanticException(
                        firstFactoryParam.Start.Line,
                        firstFactoryParam.Start.Column,
                        "Factory parameter name and type do not match");
                }
                else
                {
                    ++argStart;
                }
            }

            RTFactory factory = new RTFactory(factoryName, interfaceName, createFuncName, ParseFactoryArguments(args, argStart, interfaceName));
            return factory;
        }

        private RTFactory GetFactoryWithInterfaceAndCreateFunc(RTGen3.MacroArgContext[] args, bool funcOnly = false)
        {
            if (args.Length < 4)
            {
                throw new ParserSemanticException(args[0].Start.Line, args[0].Start.Column, "Factory function missing required parameters.");
            }


            string factoryName = args[1].GetText();
            string interfaceName = GetInterfaceNameFromFactory(args[2].GetText());
            string createFuncName = funcOnly ? $"create{factoryName}" : args[3].GetText();

            int argumentsStart = funcOnly ? 3 : 4;

            int argCount = args.Length - argumentsStart;
            if (argCount % 2 != 0 && argCount > 4)
            {
                throw new ParserSemanticException(args[4].Start.Line, args[4].Start.Column, "Factory parameter name and type do not match");
            }

            RTFactory factory = new RTFactory(factoryName,
                                              interfaceName,
                                              createFuncName,
                                              ParseFactoryArguments(args, argumentsStart, interfaceName));
            return factory;
        }

        private void ParseFactory(string macroName, RTGen3.MacroArgumentsContext arguments)
        {
            if (Attributes.Next.Factory.Options.HasFlag(FactoryOptions.Hide))
            {
                Attributes.Next.Factory.Clear();
                return;
            }

            RTGen3.MacroArgContext[] args = arguments.macroArg();

            if (args.Length > 0 && macroName.Contains("_TEMPLATE_"))
            {
                List<IRTFactory> factories = ParseGenericFactory(macroName, args);
                Attributes.Next.Factory.Clear();

                if (factories == null)
                {
                    return;
                }

                foreach (IRTFactory rtFactory in factories)
                {
                    File.Factories.Add(rtFactory);
                }
            }
            else
            {
                if (args[0].GetText() != "LIBRARY_FACTORY")
                {
                    return;
                }

                RTFactory factory;
                switch (macroName)
                {
                    case "OPENDAQ_DECLARE_CLASS_FACTORY":
                        factory = GetFactory(args);
                        break;
                    case "OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE":
                        factory = GetFactoryWithInterface(args);
                        break;
                    case "OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC":
                        factory = GetFactoryWithInterfaceAndCreateFunc(args);
                        break;
                    case "OPENDAQ_DECLARE_CLASS_FACTORY_FUNC_WITH_INTERFACE":
                        factory = GetFactoryWithInterfaceAndCreateFunc(args, true);
                        break;
                    default:
                        throw new ParserException(
                            $"Line {arguments.Start.Line},{arguments.Start.Column}: Unknown factory function \"{macroName}\".");
                }

                factory.Options = Attributes.Next.Factory.Options;

                IDocComment doc = Attributes.Next.Factory.Documentation;
                if (doc != null)
                {
                    factory.Documentation = doc;
                    File.TrailingDocumentation.Remove(doc);
                }

                Attributes.Next.Factory.Clear();

                File.Factories.Add(factory);
            }
        }

        private List<IRTFactory> ParseGenericFactory(string macroName, RTGen3.MacroArgContext[] args)
        {
            if (macroName.StartsWith("OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T_U"))
            {
                return GetFactoryTemplate2(args);
            }
            else if (macroName.StartsWith("OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE_TEMPLATE_T"))
            {
                return GetFactoryTemplate1(args);
            }

            throw new ParserException($"Line {args[0].Start.Line},{args[0].Start.Column}: Unknown factory function \"{macroName}\".");
        }

        /// <summary>Create the object representation of the parsed RT Attribute.</summary>
        /// <param name="context">The RT Attribute parse tree.</param>
        /// <returns>Returns RT Attribute object representation if successful.</returns>
        private RTAttribute GetAttributeFromContext(RTGen3.RtAttributeContext context)
        {
            RTAttribute attribute = new RTAttribute(context.rtIdentifier().GetText());

            RTGen3.ArgumentsContext argumentsList = context.arguments();
            if (argumentsList == null)
            {
                // ReSharper disable once UseArrayEmptyMethod
                attribute.Arguments = new IRTAttributeArgument[0];
                return attribute;
            }

            RTGen3.ArgContext[] arguments = argumentsList.arg();
            attribute.Arguments = new IRTAttributeArgument[arguments.Length];

            for (int argc = 0; argc < arguments.Length; argc++)
            {
                RTGen3.NamedParameterContext namedParameter = arguments[argc].namedParameter();
                RTGen3.ArgValueContext argValue = arguments[argc].argValue();

                attribute.Arguments[argc] = new RTAttributeArgument(
                    // Get unparsed text
                    arguments[argc].Start.InputStream.GetText(Interval.Of(argValue.Start.StartIndex, argValue.Stop.StopIndex)),
                    namedParameter?.Identifier().GetText()
                );

                RTGen3.TypeContext type = argValue.type();
                ITerminalNode identifier = argValue.Identifier();

                if (type != null && identifier != null)
                {
                    ((RTAttributeArgument) attribute.Arguments[argc]).Type = new Argument(
                        GetTypeNameFromContext(type),
                        identifier.GetText(),
                        argValue.Const().Length != 0
                    );
                }
                else if (attribute.Name == "elementType" && argc > 0)
                {
                    ((RTAttributeArgument) attribute.Arguments[argc]).TypeInfo = Utility.GetTypeNameFromString(
                        attribute.Arguments[argc].Value,
                        File.AttributeInfo
                    );
                }
            }

            return attribute;
        }

        /// <summary>Exit a parse tree produced by <see cref="RTGen3.rtAttribute"/>.</summary>
        /// <param name="context">The parse tree.</param>
        public override void ExitRtAttribute(RTGen3.RtAttributeContext context)
        {
            if (!File.AttributeInfo.HandleRtAttribute(GetAttributeFromContext(context)))
            {
                string attributeName = context.rtIdentifier().GetText();
                Log.Warning($"Ignoring unknown rtComment function: {attributeName}.");
            }
        }

        /// <summary>Exit a parse tree produced by <see cref="RTGen3.includeName"/>.</summary>
        /// <param name="context">The parse tree.</param>
        public override void ExitIncludeName(RTGen3.IncludeNameContext context)
        {
            if (Attributes.Next.AddHeader)
            {
                File.AttributeInfo.AdditionalHeaders.Add(context.GetText());
                Attributes.Next.AddHeader = false;
            }
        }

        #region Comments parsing

        public override void ExitDocComment(RTGen3.DocCommentContext context)
        {
            JavaDocLexer lexer = new JavaDocLexer(CharStreams.fromString(context.GetText()));

            CommonTokenStream tokens = new CommonTokenStream(lexer);
            JavaDoc parser = new JavaDoc(tokens) {
                BuildParseTree = true,
                ErrorHandler = new JavaDocErrorStrategy(context.Start.Line)
            };

            JavaDoc.StartContext tree = parser.start();
            if (parser.NumberOfSyntaxErrors > 0)
            {
                throw new ParserException("Syntax errors occurred. Exiting.");
            }

            JavaDocParser listener = new JavaDocParser();
            ParseTreeWalker.Default.Walk(listener, tree);

            IDocComment docComment = listener.Documentation;
            switch (_state)
            {
                case ParseState.Leading:
                    File.LeadingDocumentation.Add(docComment);
                    break;
                case ParseState.Class:
                case ParseState.Method:
                    Attributes.Next.Method.Documentation = docComment;
                    break;
                case ParseState.Trailing:
                    File.TrailingDocumentation.Add(docComment);
                    Attributes.Next.Factory.Documentation = docComment;
                    break;
                default:
                    throw new GeneratorException($"Invalid generator state: {_state}");
            }
        }

        #endregion
    }
}
