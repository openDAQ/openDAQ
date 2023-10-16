using System;
using System.Collections.Generic;
using RTGen.Interfaces;

namespace RTGen.Types
{
    public static class TypeInfo {
        public static readonly string DefaultBasePtrNamespace = "RT::Core::";
        public static readonly string DefaultBasePtr = "ObjectPtr";

        private static readonly Dictionary<string, string> IntfToSmartPtr;
        private static readonly Dictionary<string, string> CustomPtrHeaders;
        private static readonly Dictionary<string, Action<object>> RTFunctionHandlers;
        private static readonly Dictionary<string, INamespace> NamespaceOverrides;

        static TypeInfo() {
            NamespaceOverrides = new Dictionary<string, INamespace>();
            AdditionalHeaders = new List<string>();

            RTFunctionHandlers = new Dictionary<string, Action<object>> {
                //{ "interfaceSmartPtr", InterfaceToSmartPtr},
                //{ "includeHeader" , IncludeHeader },
                //{ "includeHeaders" , IncludeHeader },
                //{ "interfaceNamespace", InterfaceNamespace }
            };

            IntfToSmartPtr = new Dictionary<string, string>();

            CustomPtrHeaders = new Dictionary<string, string> {
                { "IBaseObject", "<coretypes/objectptr.h>"}
            };
        }

        public static List<string> AdditionalHeaders { get; }

        public static bool AddNextHeader { get; set; }

        #region CustomHeaders

        public static bool HasCustomHeader(string typeName) {
            return CustomPtrHeaders.ContainsKey(typeName);
        }

        public static void AddCustomTypeHeader(string typeName, string header) {
            CustomPtrHeaders.Add(typeName, header);
        }

        public static string GetCustomHeader(string typeName) {
            return CustomPtrHeaders[typeName];
        }

        #endregion

        public static string GetDefaultBasePtrFull()
        {
            return DefaultBasePtrNamespace + DefaultBasePtr;
        }

        #region InterfaceMapping

        public static void AddMapping(string interfaceName, string ptrName) {
            IntfToSmartPtr.Add(interfaceName, ptrName);
        }

        public static bool HasCustomMapping(string interfaceName) {
            return IntfToSmartPtr.ContainsKey(interfaceName);
        }

        public static string GetPtrMapping(string interfaceName) {
            return IntfToSmartPtr[interfaceName];
        }

        #endregion

        #region NamespaceOverride

        public static bool HasNamespaceOverride(string typeName)
        {
            return NamespaceOverrides.ContainsKey(typeName);
        }

        public static void AddNamespaceOverride(string typeName, string namespaceName)
        {
            //TODO: Fix!
            //NamespaceOverrides.Add(typeName, new Namespace(namespaceName));
        }

        public static INamespace GetNamespaceOverride(string typeName)
        {
            return NamespaceOverrides[typeName];
        }

        #endregion

        #region RtFuncs

        public static bool RtFuncExist(string funcName) {
            return RTFunctionHandlers.ContainsKey(funcName);
        }

        //public static void RunRtFunc(string name, RTGenParser.RtFuncContext context) {
        //    if (!RTFunctionHandlers.ContainsKey(name)) {
        //        return;
        //    }

        //    RTFunctionHandlers[name](context);
        //}

        //public static void AddRtFunc(string name, Action<RTGenParser.RtFuncContext> handler) {
        //    RTFunctionHandlers[name] = handler;
        //}

        //private static void IncludeHeader(RTGenParser.RtFuncContext context) {
        //    RTGenParser.ArgumentsContext arguments = context.arguments();

        //    if (arguments == null) {
        //        AddNextHeader = true;
        //        return;
        //    }

        //    AddNextHeader = false;

        //    foreach (RTGenParser.ArgContext argument in arguments.arg()) {
        //        AdditionalHeaders.Add(argument.GetText());
        //    }
        //}

        //private static void InterfaceToSmartPtr(RTGenParser.RtFuncContext context) {
        //    RTGenParser.ArgumentsContext argumentsList = context.arguments();

        //    if (argumentsList == null) {
        //        throw new RTFuncException("RtFunc \"interfaceSmartPtr\" must have arguments.");
        //    }

        //    RTGenParser.ArgContext[] arguments = argumentsList.arg();
        //    if (arguments.Length >= 2 && arguments.Length <= 3) {
        //        string interfaceArg = arguments[0].GetText();
        //        string ptrArg = arguments[1].GetText();

        //        IntfToSmartPtr.Add(interfaceArg, ptrArg);

        //        if (arguments.Length == 3) {
        //            AddCustomTypeHeader(ptrArg, arguments[2].GetText().Trim('"', '<', '>'));
        //        }
        //    }
        //    else {
        //        throw new RTFuncException("RtFunc \"interfaceSmartPtr\" must have at least 2 arguments.");
        //    }
        //}

        //private static void InterfaceNamespace(RTGenParser.RtFuncContext context)
        //{
        //    RTGenParser.ArgumentsContext argumentsList = context.arguments();

        //    if (argumentsList == null)
        //    {
        //        throw new RTFuncException("RtFunc \"interfaceNamespace\" must have arguments.");
        //    }

        //    RTGenParser.ArgContext[] arguments = argumentsList.arg();
        //    if (arguments.Length != 2)
        //    {
        //        throw new RTFuncException("RtFunc \"interfaceNamespace\" must have at least 2 arguments.");
        //    }

        //    AddNamespaceOverride(arguments[0].GetText(), arguments[1].GetText().Trim('"'));
        //}

        #endregion
    }
}
