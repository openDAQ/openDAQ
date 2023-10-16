using System;
using System.Collections.Generic;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>A basic implementation of the overload to a method.</summary>
    [Serializable]
    public class Overload : IOverload
    {
        /// <summary>Creates an overload for the provided method.</summary>
        /// <param name="method">The method to overload.</param>
        public Overload(IMethod method) : this(method, OverloadType.Unknown, new List<IArgument>())
        {
            Method = method;
        }

        /// <summary>Creates a specific overload type for the provided method.</summary>
        /// <param name="method">The method to overload.</param>
        /// <param name="type">The type of the overload.</param>
        public Overload(IMethod method, OverloadType type) : this(method, type, new List<IArgument>())
        {
        }

        /// <summary>Creates an overload for the provided method with custom parameters.</summary>
        /// <param name="method">The method to overload.</param>
        /// <param name="type">The type of the overload.</param>
        /// <param name="args">Custom parameters.</param>
        /// <param name="tag">Attach custom data object.</param>
        public Overload(IMethod method, OverloadType type, IList<IArgument> args, object tag = null)
        {
            Arguments = args;
            Type = type;
            Method = method;
            Tag = tag;
        }

        /// <summary>List of method arguments (type, name, modifiers etc.).</summary>
        public IList<IArgument> Arguments { get; set; }

        /// <summary>Type the method returns if its not void (procedure).</summary>
        public ITypeName ReturnType { get; set; }

        /// <summary>The type of the overload.</summary>
        public OverloadType Type { get; set; }

        /// <summary>The method to which this overload belongs to.</summary>
        public IMethod Method { get; set; }

        /// <summary>Holds generator custom info (implementation defined).</summary>
        public object Tag { get; set; }

        /// <summary>The last OUT, BY REFERENCE or return by pointer argument.</summary>
        /// <returns>Returns the last by ref argument otherwise <c>null</c>.</returns>
        public IArgument GetLastByRefArgument()
        {
            for (int i = Arguments.Count - 1; i >= 0; i--)
            {
                IArgument argument = Arguments[i];
                if (argument.IsOutParam)
                {
                    return argument;
                }
            }

            return null;
        }

        /// <summary>Checks if the method returns by reference (has an out parameter).</summary>
        /// <returns>Returns <c>true</c> if method returns by reference otherwise <c>false</c>.</returns>
        public bool ReturnsByRef()
        {
            if (ReturnType == null)
            {
                return false;
            }

            if (ReturnType.Name.ToLowerInvariant() != "errcode")
            {
                return false;
            }

            for (int i = Arguments.Count - 1; i >= 0; i--)
            {
                if (Arguments[i].IsOutParam)
                {
                    return true;
                }
            }

            return false;
        }

        /// <summary>Human representation.</summary>
        public override string ToString()
        {
            string returnType = ReturnType != null ? ReturnType.Name + " " : "";
            string arguments = string.Join(", ", Arguments);
            string callingConvention = Method.CallingConvention != null ? Method.CallingConvention + ";" : "";

            return $"{returnType}{Method.Name}({arguments}); {callingConvention}";
        }
    }
}
