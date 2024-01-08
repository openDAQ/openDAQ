﻿using System;
using System.Collections.Generic;
using System.Text;
using RTGen.Generation;
using RTGen.Interfaces;
using RTGen.Interfaces.Doc;
using RTGen.Types;
using RTGen.Util;

namespace RTGen.Python.Generators
{
    public class PythonGenerator : TemplateGenerator
    {
        public PythonGenerator()
        {
            GenerateMethods = false;
        }

        public override IVersionInfo Version => new VersionInfo
        {
            Major = 2,
            Minor = 0,
            Patch = 0
        };

        public override IRTFile RtFile
        {
            get => base.RtFile;
            set
            {
                base.RtFile = value;
                RtFile.AttributeInfo.NamespaceSeparator = "::";

                IRTInterface currentClass = value.CurrentClass;
                string typeName = currentClass.Type.NonInterfaceName;

                Options.Filename = "py_" + typeName.ToLowerSnakeCase();
                Options.GeneratedExtension = ".cpp";
            }
        }

        private static string GetBriefDocumentationString(IDocComment documentation)
        {
            if (documentation == null || documentation.Brief == null)
                return String.Empty;

            StringBuilder briefDocumentation = new StringBuilder();

            foreach (IDocLine line in documentation.Brief.Lines)
                briefDocumentation.Append(line).Append(" ");
            briefDocumentation.Length -= 1;

            return briefDocumentation.ToString().Replace("\"", "\\\"");
        }

        protected override void SetCustomVariables()
        {
            Variables.Add("HeaderName", "py_" + RtFile.AttributeInfo.DefaultLibrary.ToLowerSnakeCase());
            Variables.Add("ClassDocumentation", GetBriefDocumentationString(RtFile.CurrentClass.Documentation));
        }

        protected override void OnVariablesReady()
        {
            string factoryConstructorImpl = GenerateFactoryConstructors();
            string enumImpl = GenerateEnumImplementations();
            string methodImpl = GenerateMethodImplementations();

            Variables.Add("FactoryConstructorsImpl", factoryConstructorImpl);
            Variables.Add("EnumImplementations", enumImpl);
            Variables.Add("MethodImplementations", methodImpl);
        }

        protected override string GetFileVariable(IRTInterface rtClass, string variable)
        {
            switch (variable)
            {
                case "InterfaceName":
                    {
                        return rtClass.Type.Name;
                    }
                case "WrapperName":
                    {
                        // the logic below does not seem to work for IEventArgs
                        if (rtClass.Type.Name == "IEventArgs")
                            return "EventArgsPtr<>";

                        if (RtFile.AttributeInfo.PtrMappings.TryGet(rtClass.Type.Name, out ISmartPtr ptr) && ptr.HasDefaultAlias)
                            return ptr.DefaultAliasName;
                        else
                            return rtClass.Type.Wrapper.Name;
                    }
            }

            return base.GetFileVariable(rtClass, variable);
        }

        private string GetPyBind11TypeName(IArgument argument)
        {
            if (!argument.IsOutParam)
            {
                string argumentName = argument.Type.Name;

                if (argumentName == "IString")
                    return "const std::string&";
                else if (argumentName == "SizeT")
                    return "const size_t";
                else if (argumentName == "Bool")
                    return "const bool";
                else if (argumentName == "IBaseObject")
                    return "const py::object&";
            }

            return argument.Type.FullName(false) + argument.Type.Modifiers;
        }

        private string GetPyBind11WrappedName(IArgument argument)
        {
            if (!argument.IsOutParam && (argument.Type.Name == "IBaseObject"))
                return "pyObjectToBaseObject(" + argument.Name + ")";
            else
                return argument.Name;
        }

        private string GenerateEnumImplementations()
        {
            StringBuilder enumsImpl = new StringBuilder();

            // start with an empty line (to get nicer spacing)
            if (RtFile.Enums.Count > 0)
                enumsImpl.AppendLine();

            foreach (IEnumeration enumeration in RtFile.Enums)
            {
                enumsImpl.AppendLine(String.Format("    py::enum_<daq::{0}>(m, \"{0}\")", enumeration.Name));

                string leadingSpaces = "".PadRight(8);
                foreach (IEnumOption enumOption in enumeration.Options)
                    enumsImpl.AppendLine(leadingSpaces + String.Format(".value(\"{1}\", daq::{0}::{1})", enumeration.Name, enumOption.Name));
                enumsImpl.TrimTrailingNewLine();
                enumsImpl.AppendLine(";");
            }

            return enumsImpl.ToString();
        }

        private string GenerateFactoryConstructors()
        {
            StringBuilder factoriesImpl = new StringBuilder();

            // start with an empty line (to get nicer spacing)
            if (RtFile.Factories.Count > 0)
                factoriesImpl.AppendLine();

            foreach (IRTFactory factory in RtFile.Factories)
            {
                // TODO: temporary workaround to comment out WinDebugLoggerSink from non-Windows platforms
                bool win32Only = (factory.PrettyName == "WinDebugLoggerSink");
                if (win32Only)
                    factoriesImpl.AppendLine("#ifdef _WIN32");
                // TODO: temporary workaround to comment out MiMallocAllocator when no MIMALLOC support is present
                bool mimallocNeeded = (factory.PrettyName == "MiMallocAllocator");
                if (mimallocNeeded)
                    factoriesImpl.AppendLine("#ifdef OPENDAQ_MIMALLOC_SUPPORT");

                factoriesImpl.AppendLine(String.Format("    m.def(\"{0}\", &daq::{0}_Create);", factory.PrettyName));

                if (win32Only || mimallocNeeded)
                    factoriesImpl.AppendLine("#endif");
            }

            return factoriesImpl.ToString();
        }

        /*
         * A method for pybind11 typically looks like this:
         *

            // name of the function
            cls.def("add_device",
                // a lambda function with arguments (the first argument is the class on which the function is defined)
                [](daq::IDevice *object, const std::string& connectionString)
                {
                    const auto objectPtr = daq::DevicePtr::Borrow(object);
                    // the last part depends on what we want to return:
                    // - regular objects are ".detach()"-ed
                    // - strings get converted ".toStdString()"
                    // - simple types such as bool, int, ... get returned as simple types
                    // - or we don't need to return anything at all
                    return objectPtr.addDevice(connectionString).detach();
                },
                // named arguments
                py::arg("connection_string"),
                // documentation
                "Connects to a device at the given connection string and returns it.");

         *
         * When we have a property with both a getter and setter, like get_name() and set_name(std::string name),
         * we can better define a property with two lambda functions:
         *

            cls.def_property("name",
                [](daq::IPropertyInfo *object)
                {
                    const auto objectPtr = daq::PropertyInfoPtrBase::Borrow(object);
                    return objectPtr.getName().toStdString();
                },
                [](daq::IPropertyInfo *object, const std::string& name)
                {
                    const auto objectPtr = daq::PropertyInfoPtrBase::Borrow(object);
                    objectPtr.setName(name);
                },
                "The property's name.");

         *
         * For read-only properties we use .def_property_readonly(...) instead.
         */
        private string GenerateMethodLambda(IRTInterface rtClass, IMethod method)
        {
            StringBuilder impl = new StringBuilder();

            List<string> argumentsListIn = new List<string>();
            List<string> argumentsListPass = new List<string>();
            List<string> argumentAnnotations = new List<string>();
            List<IArgument> argumentsOut = new List<IArgument>();
            string argumentsIn = "";
            string argumentsPass = "";

            // argument annotations are not supported in properties
            bool generateArgumentAnnotations = (method.GetSetPair == null);
            // lists of arguments
            foreach (IArgument argument in method.Overloads[0].Arguments)
            {
                if (argument.IsOutParam)
                {
                    argumentsOut.Add(argument);
                }
                else
                {
                    argumentsListIn.Add(GetPyBind11TypeName(argument) + " " + argument.Name);
                    argumentsListPass.Add(GetPyBind11WrappedName(argument));
                    if (generateArgumentAnnotations)
                    {
                        string defaultArgumentValueAssignment = "";
                        if (argument.DefaultValue != null)
                            defaultArgumentValueAssignment = " = " + argument.DefaultValue;
                        argumentAnnotations.Add("py::arg(\"" + argument.Name.ToLowerSnakeCase() + "\")" + defaultArgumentValueAssignment);
                    }
                }
            }

            if (argumentsListIn.Count > 0)
            {
                argumentsIn = ", " + String.Join(", ", argumentsListIn);
                argumentsPass = String.Join(", ", argumentsListPass);
            }

            string wrapperName = GetFileVariable(rtClass, "WrapperName");
            string interfaceName = GetFileVariable(rtClass, "InterfaceName");

            string leadingSpaces = "".PadRight(8);

            impl.AppendLine(leadingSpaces + String.Format("[](daq::{0} *object{1})", interfaceName, argumentsIn));
            impl.AppendLine(leadingSpaces + "{");
            impl.AppendLine(leadingSpaces + String.Format("    const auto objectPtr = daq::{0}::Borrow(object);", wrapperName));

            string doReturn = method.ReturnsByRef() ? "return " : "";
            string convertToPython = "";
            if (argumentsOut.Count > 0)
            {
                IArgument argumentOut = argumentsOut[0];

                // we need to call .toStdString() to return a native string rather than openDAQ object
                if (argumentOut.Type.Name == "IString")
                    convertToPython = ".toStdString()";
                // objects need to be detached, while integers, booleans, enums etc. shouldn't be
                else if (!argumentOut.Type.Flags.IsValueType && argumentOut.Type.UnmappedName != "void")
                {
                    if (argumentOut.Type.Name != "IBaseObject")
                        convertToPython = ".detach()";
                    else
                    {
                        doReturn = "return baseObjectToPyObject(";
                        convertToPython = ")";
                    }
                }
            }

            impl.AppendLine(leadingSpaces + "    " + String.Format("{0}objectPtr.{1}({2}){3};", doReturn, method.Name, argumentsPass, convertToPython));
            impl.AppendLine(leadingSpaces + "},");

            if (argumentAnnotations.Count > 0)
                impl.AppendLine(leadingSpaces + String.Join(", ", argumentAnnotations) + ",");

            return impl.ToString();
        }

        // TODO: a temporary set of functions that are not properly supported yet
        private bool isMethodTemporarilyDisabled(IMethod method)
        {
            // These two methods return raw pointers,
            // it's not exactly clear what the bindings are supposed to do here.
            if (method.Name == "getData" || method.Name == "getRawData")
                return true;

            foreach (IArgument argument in method.Overloads[0].Arguments)
            {
                // IEvent is not properly supported yet,
                // any method involving events doesn't compile yet.
                if (argument.Type.Name == "IEvent")
                    return true;
            }

            return false;
        }

        private int GetGetterPrefixLength(String name)
        {
            int prefixLength = -1;
            if (name != null)
            {
                if (name.StartsWith("get", StringComparison.OrdinalIgnoreCase))
                {
                    prefixLength = 3;
                }
                else if (name.StartsWith("is", StringComparison.OrdinalIgnoreCase))
                {
                    prefixLength = 2;
                }
            }
            return prefixLength;
        }

        /*
         * A method to generate readonly properties from getters with optional params
         * 
         * When we have a method satisfying the following conditions:
         * 
         * 1. It is a getter
         * 2. It has the first and only one output parameter
         * 3. Rest of the parameters are optional (have default value)
         * 
         * We can clone it, clean all of the optionals and make it read_only_property.
         * 
         */
        private void AddExtraProperties()
        {
            foreach (IRTInterface rtClass in RtFile.Classes)
            {
                IDictionary<int, IMethod> extraProperties = new SortedDictionary<int, IMethod>();
                //index is needed for correct insertion
                for (int i = 0; i < rtClass.Methods.Count; i++)
                {
                    IMethod method = rtClass.Methods[i];

                    //skip non-getters
                    int getterPrefix = GetGetterPrefixLength(method.Name);
                    if (getterPrefix < 0) continue;

                    bool isMethodHasTheFirstAndOnlyOneOutputArgument = false;
                    //if less than 2 args then no optional exists
                    bool isMethodHasAllSecondaryArgumentsOptional = method.Arguments.Count > 1;
                    for (int k = 0; k < method.Arguments.Count; k++)
                    {
                        IArgument argument = method.Arguments[k];
                        if (k == 0)
                        {
                            //check the first Argument is the only one out param
                            isMethodHasTheFirstAndOnlyOneOutputArgument = argument.IsOutParam;
                        }
                        else
                        {
                            isMethodHasTheFirstAndOnlyOneOutputArgument &= !argument.IsOutParam;
                            isMethodHasAllSecondaryArgumentsOptional &= (argument.DefaultValue != null);
                        }
                    }
                    if (isMethodHasTheFirstAndOnlyOneOutputArgument && isMethodHasAllSecondaryArgumentsOptional)
                    {
                        //clone method with both conditions satisfied
                        extraProperties.Add(i, method.Clone());
                    }
                }
                //remove optionals in the method to make it readonly property and
                //insert the property into the human readable place in the class
                int propsInserted = 0;
                foreach (KeyValuePair<int, IMethod> indexedMethod in extraProperties)
                {
                    int methodIndex = indexedMethod.Key;
                    IMethod method = indexedMethod.Value;
                    int getterPrefixLen = GetGetterPrefixLength(method.Name);
                    String propertyName = method.Name.Substring(getterPrefixLen);

                    method.GetSetPair = new GetSet(propertyName);
                    method.GetSetPair.Getter = method;

                    for (int i = method.Arguments.Count - 1; i >= 0; i--)
                    {
                        if (!method.Arguments[i].IsOutParam || method.Arguments[i].DefaultValue != null) method.Arguments.RemoveAt(i);
                    }

                    //insert the propery into the place right before the getter
                    rtClass.Methods.Insert(propsInserted + methodIndex, method);
                    propsInserted++;
                }

            }

        }

        private string GenerateMethodImplementations()
        {
            AddExtraProperties();
            StringBuilder methodImpl = new StringBuilder();

            // start with an empty line (to get nicer spacing)
            if ((RtFile.Classes.Count > 0) && (RtFile.Classes[0].Methods.Count > 0))
                methodImpl.AppendLine();

            foreach (IRTInterface rtClass in RtFile.Classes)
            {
                foreach (IMethod method in rtClass.Methods)
                {
                    bool isTemporaryDisabled = isMethodTemporarilyDisabled(method);
                    bool isThisAGetterFromAGetSetPair = (method.GetSetPair != null) &&
                                                        (method.GetSetPair.Getter == method) &&
                                                        (method.GetSetPair.Setter != null);
                    bool isThisASetterFromAGetSetPair = (method.GetSetPair != null) &&
                                                        (method.GetSetPair.Getter != null) &&
                                                        (method.GetSetPair.Setter == method);
                    bool isThisASetterWithoutAGetter = (method.GetSetPair != null) && (method.GetSetPair.Getter == null);

                    // In getters pybind11 automatically applies return_value_policy::reference_internal,
                    // In that case the returned objects don't get deleted,
                    // so we need to explicitly set return_value_policy::take_ownership instead.
                    //
                    // https://pybind11.readthedocs.io/en/stable/advanced/functions.html#return-value-policies
                    //
                    // (The policy doesn't affect the setter, so it doesn't matter if we apply it to to both
                    // the setter and the getter at once: it will only be used for the getter anyway.)
                    bool needsReturnValuePolicy = (method.GetSetPair != null) &&
                                                  (method.GetSetPair.Getter != null) &&
                                                  // The setter should in principle always contain exactly one argument,
                                                  // but property_info.h contains one weird setter with no arguments.
                                                  (method.Arguments.Count > 0) &&
                                                  // This hasn't been tested, but let's assume that strings and numbers
                                                  // don't need an explicit destructor call.
                                                  (!(method.Arguments[0].Type.Flags.IsValueType || method.Arguments[0].Type.Name == "IString"));

                    // when we end up at the setter and getter is not null,
                    // we must have already finished adding this method next to the getter,
                    // so skip this step ...
                    if (isThisASetterFromAGetSetPair)
                        continue;

                    string defineType = "";
                    if (method.GetSetPair == null)
                        defineType = "def";
                    else if (method.GetSetPair.Setter == null)
                        defineType = "def_property_readonly";
                    else
                        defineType = "def_property";

                    string methodName = (method.GetSetPair == null)
                        ? method.Name.ToLowerSnakeCase()
                        : method.GetSetPair.Name.ToLowerSnakeCase();

                    string leadingSpaces = "".PadRight(8);

                    if (isTemporaryDisabled)
                        methodImpl.AppendLine("    /*");

                    methodImpl.AppendLine(String.Format("    cls.{0}(\"{1}\",", defineType, methodName));

                    string returnValuePolicy = needsReturnValuePolicy
                        ? (leadingSpaces + "py::return_value_policy::take_ownership," + Environment.NewLine)
                        : "";

                    // when both getter and setter are present, create a property, implementing both getter and setter together
                    if (isThisAGetterFromAGetSetPair)
                    {
                        methodImpl.Append(GenerateMethodLambda(rtClass, method.GetSetPair.Getter));
                        methodImpl.Append(GenerateMethodLambda(rtClass, method.GetSetPair.Setter));
                        methodImpl.Append(returnValuePolicy);

                        // TODO: this should ideally be called as
                        //       GetBriefDocumentationString(method.GetSetPair.Documentation)
                        string getterDoc = GetBriefDocumentationString(method.GetSetPair.Getter.Documentation);
                        string setterDoc = GetBriefDocumentationString(method.GetSetPair.Setter.Documentation);
                        string doc = "";
                        if (getterDoc.Length > 0 && setterDoc.Length > 0)
                            doc = getterDoc + " / " + setterDoc;
                        else
                            doc = getterDoc + setterDoc;
                        methodImpl.AppendLine(leadingSpaces + "\"" + doc + "\");");
                    }
                    else
                    {
                        // when we don't have a getter, we pass nullptr to the getter to make a write-only property
                        if (isThisASetterWithoutAGetter)
                            methodImpl.AppendLine(leadingSpaces + "nullptr,");

                        methodImpl.Append(GenerateMethodLambda(rtClass, method));
                        methodImpl.Append(returnValuePolicy);
                        methodImpl.AppendLine(leadingSpaces + "\"" + GetBriefDocumentationString(method.Documentation) + "\");");
                    }

                    if (isTemporaryDisabled)
                        methodImpl.AppendLine("    */");
                }
            }

            return methodImpl.ToString();
        }
    }
}
