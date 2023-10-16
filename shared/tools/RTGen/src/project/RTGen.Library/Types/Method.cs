using System;
using System.Collections.Generic;
using System.Runtime.Serialization;
using RTGen.Interfaces;
using RTGen.Interfaces.Doc;
using RTGen.Util;

namespace RTGen.Types
{
    /// <summary>Represents a method in an interface.</summary>
    [Serializable]
    public class Method : IMethod
    {
        private const string CALLING_CONVENTION_MACRO = "INTERFACE_FUNC";
        private const string INTERFACE_CALLING_CONVENTION = "stdcall";
        private string _callingConvention;

        /// <summary>Initializes a new instance of the <see cref="Method" /> class.</summary>
        public Method()
        {
            Modifiers = new List<string>();
            Overloads = new List<IOverload>
            {
                new Overload(this, OverloadType.Interface)
            };
        }

        /// <summary>Initializes a new instance of the <see cref="Method" /> class.</summary>
        public Method(string name, IList<IArgument> arguments) : this(name, arguments, new List<string>())
        {
        }

        private Method(string name, IList<IArgument> arguments, IList<string> modifiers)
        {
            Name = name;
            Modifiers = modifiers;
            Overloads = new List<IOverload>
            {
                new Overload(this, OverloadType.Interface, arguments)
            };

            CallingConvention = @"cdecl";
        }

        /// <summary>Method modifiers (virtual, static, const etc.)</summary>
        public IList<string> Modifiers { get; }

        /// <summary>List of method arguments (type, name, modifiers etc.).</summary>
        public IList<IArgument> Arguments => Overloads[0].Arguments;

        /// <summary>Type the method returns if its not void (procedure).</summary>
        /// <remarks>Almost always <c>ErrCode</c>.</remarks>
        public ITypeName ReturnType
        {
            get => Overloads[0].ReturnType;
            set => Overloads[0].ReturnType = value;
        }

        /// <summary>The property associated with the method otherwise <c>null</c>.</summary>
        public IProperty Property { get; set; }

        /// <summary>The method name.</summary>
        /// <example>GetText</example>
        public string Name { get; set; }

        /// <summary>If method is <c>abstract virtual</c>.</summary>
        public bool IsPure { get; set; }

        /// <summary>If the method should not be generated for a certain generator.</summary>
        public GeneratorType IsIgnored { get; set; }

        /// <summary>If the method should not be generated for a certain generator.</summary>
        public bool ReturnSelf { get; set; }

        /// <summary>The calling convention used to call the method.</summary>
        /// <example>__stdcall, __cdecl, __fastcall</example>
        public string CallingConvention
        {
            get => _callingConvention;
            set
            {
                if (value == CALLING_CONVENTION_MACRO)
                {
                    _callingConvention = INTERFACE_CALLING_CONVENTION;
                    return;
                }

                _callingConvention = value;
            }
        }

        /// <summary>Helper and overloads of the same method but with SmartPtrs or language-native types.</summary>
        public IList<IOverload> Overloads { get; }

        /// <summary>Documentation comment info.</summary>
        public IDocComment Documentation { get; set; }

        /// <summary>Gets the get/set method pair that contains this method. Is <c>nullptr</c> if not a get/set method.</summary>
        [IgnoreDataMember]
        public IGetSet GetSetPair { get; set; }

        /// <summary>The calling convention used to call the method. If "stdcall" then return appropriate C++ macro.</summary>
        /// <example>INTERFACE_FUNC, __cdecl, __fastcall</example>
        public string GetCallingConventionMacro()
        {
            string callingConvention = _callingConvention;
            if (callingConvention == CALLING_CONVENTION_MACRO)
            {
                return CALLING_CONVENTION_MACRO;
            }

            return callingConvention;
        }

        /// <summary>The last OUT, BY REFERENCE or return by pointer argument.</summary>
        /// <returns>Returns the last by ref argument otherwise <c>null</c>.</returns>
        public IArgument GetLastByRefArgument()
        {
            return Overloads[0].GetLastByRefArgument();
        }

        /// <summary>Checks if the method returns by reference (has an out parameter).</summary>
        /// <returns>Returns <c>true</c> if method returns by reference otherwise <c>false</c>.</returns>
        public bool ReturnsByRef()
        {
            return Overloads[0].ReturnsByRef();
        }

        /// <summary>Deep clone the Method.</summary>
        /// <returns>A deep clone of the method.</returns>
        public IMethod Clone()
        {
            return Utility.Clone(this);
        }

        /// <summary>Human representation.</summary>
        public override string ToString()
        {
            return
                $"{(ReturnType != null ? ReturnType.Name + " " : "")}{Name}({string.Join(", ", Arguments)}); {(CallingConvention != null ? CallingConvention + ";" : "")}";
        }
    }
}
