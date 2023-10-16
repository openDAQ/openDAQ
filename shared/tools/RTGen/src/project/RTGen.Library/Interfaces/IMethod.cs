using System;
using System.Collections.Generic;
using RTGen.Interfaces.Doc;

namespace RTGen.Interfaces
{
    /// <summary>Represents a method in an interface.</summary>
    public interface IMethod : ICloneable<IMethod>
    {
        /// <summary>Method modifiers (virtual, static, const etc.)</summary>
        IList<string> Modifiers { get; }

        /// <summary>List of method arguments (type, name, modifiers etc.).</summary>
        IList<IArgument> Arguments { get; }

        /// <summary>The type method returns if its not <c>void</c> (procedure).</summary>
        /// <remarks>Almost always <c>ErrCode</c>.</remarks>
        ITypeName ReturnType { get; set; }

        /// <summary>Method property info.</summary>
        IProperty Property { get; }

        /// <summary>The method name.</summary>
        /// <example>GetText</example>
        string Name { get; set; }

        /// <summary>If method is <c>abstract virtual</c>.</summary>
        bool IsPure { get; set; }

        /// <summary>If the method should not be generated for a certain generator.</summary>
        GeneratorType IsIgnored { get; set; }

        /// <summary>If the method should not be generated for a certain generator.</summary>
        bool ReturnSelf { get; set; }

        /// <summary>The calling convention used to call the method.</summary>
        /// <example>__stdcall, __cdecl, __fastcall</example>
        string CallingConvention { get; }

        /// <summary>Overloads of the same method with different arguments and return types (interface, wrapper, helper).</summary>
        IList<IOverload> Overloads { get; }

        /// <summary>Documentation comment info.</summary>
        IDocComment Documentation { get; }

        /// <summary>The last OUT, BY REFERENCE or return by pointer argument.</summary>
        /// <returns>Returns the last by ref argument otherwise <c>null</c>.</returns>
        IArgument GetLastByRefArgument();

        /// <summary>The calling convention used to call the method. If "stdcall" then return appropriate C++ macro.</summary>
        /// <example>INTERFACE_FUNC, __cdecl, __fastcall</example>
        string GetCallingConventionMacro();

        /// <summary>Checks if the method returns by reference (has an out parameter).</summary>
        /// <returns>Returns <c>true</c> if method returns by reference otherwise <c>false</c>.</returns>
        bool ReturnsByRef();

        /// <summary>Gets the get/set method pair that contains this method. Is <c>nullptr</c> if not a get/set method.</summary>
        IGetSet GetSetPair { get; set; }
    }
}
