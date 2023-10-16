using System;
using System.CodeDom;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using RTGen.Interfaces;
using RTGen.Interfaces.Doc;

namespace RTGen.Types
{
    /// <summary>Factory function declaration info.</summary>
    [Serializable]
    public class RTFactory : IRTFactory
    {
        private string _tag;

        /// <summary>Initializes a new instance of the <see cref="RTFactory" /> class.</summary>
        public RTFactory(string prettyName, string interfaceName, string createFuncName, IEnumerable<IArgument> args)
        {
            PrettyName = prettyName;
            Name = createFuncName;
            Arguments = args.ToArray();
            InterfaceName = interfaceName;

            IsGeneric = false;
            Options = FactoryOptions.None;
            IsConstructorWithMultipleFactories = false;
        }

        /// <summary>Copies the factory info to a new instance.</summary>
        /// <param name="factory">The factory to copy.</param>
        /// <param name="constructor">Whether to ommit the first parameter.</param>
        public RTFactory(IRTFactory factory, bool constructor = false)
        {
            PrettyName = factory.PrettyName;
            Name = factory.Name;
            InterfaceName = factory.InterfaceName;
            IsConstructorWithMultipleFactories = factory.IsConstructorWithMultipleFactories;

            IsGeneric = factory.IsGeneric;

            Arguments = constructor
                            ? factory.Arguments.Skip(1).ToArray()
                            : factory.Arguments;
        }

        /// <summary>Copies the factory info to a new instance.</summary>
        /// <param name="factory">The factory to copy.</param>
        /// <param name="kind"></param>
        public RTFactory(IRTFactory factory, IArgument kind)
        {
            PrettyName = factory.PrettyName;
            Name = factory.Name;
            InterfaceName = factory.InterfaceName;
            IsConstructorWithMultipleFactories = factory.IsConstructorWithMultipleFactories;

            Arguments = new IArgument[factory.Arguments.Length];
            Arguments[0] = kind;

            for (int i = 1; i < factory.Arguments.Length; i++)
            {
                Arguments[i] = factory.Arguments[i];
            }
        }

        /// <summary>The identifier name of the factory function.</summary>
        public string Name { get; protected set; }

        /// <summary>Factory pretty name.</summary>
        public string PrettyName { get; }

        /// <summary>Factory tag if multiple factories have the same parameters and types.</summary>
        public string Tag
        {
            get
            {
                if (!IsConstructorWithMultipleFactories && _tag != null)
                {
                    return _tag;
                }

                string objName = InterfaceName.Substring(1);
                return PrettyName.EndsWith(objName) || PrettyName.StartsWith(objName)
                           ? PrettyName.Replace(objName, "")
                           : PrettyName;

            }
            set => _tag = value;
        }

        /// <summary>Whether this is a disambiguated constructor using multiple factories.</summary>
        public virtual bool IsConstructorWithMultipleFactories { get; }

        /// <summary>Factory return interface name.</summary>
        public string InterfaceName { get; }

        /// <summary>Factory function argument info.</summary>
        public IArgument[] Arguments { get; }

        /// <summary>Additional generation options</summary>
        public FactoryOptions Options { get; set; }

        /// <summary>If the factory has arguments with generic/template types.</summary>
        public virtual bool IsGeneric { get; }

        /// <summary>Documentation comment info.</summary>
        public IDocComment Documentation { get; set; }

        /// <summary>Converts the factory to a plain method.</summary>
        /// <returns>Factory as a method instance.</returns>
        public virtual IOverload ToOverload(bool constructor = false)
        {
            Method m;
            if (constructor)
            {
                m = new Method(Name, Arguments.ToList());
                for (int i = Arguments.Length - 1; i >= 0; i--)
                {
                    if (m.Arguments[i].IsOutParam)
                    {
                        m.Arguments.RemoveAt(i);
                        break;
                    }
                }

                if (_tag == "_string")
                {
                    m.Overloads[0].Type = OverloadType.Constructor;
                    m.Overloads[0].Tag = "string";
                }
            }
            else
            {
                m = new Method(Name, Arguments);
            }

            return m.Overloads[0];
        }

        /// <summary>Indicates whether the current object is equal to another object of the same type.</summary>
        /// <returns>true if the current object is equal to the <paramref name="other" /> parameter; otherwise, false.</returns>
        /// <param name="other">An object to compare with this object.</param>
        public bool Equals(IRTFactory other)
        {
            if (other == null)
            {
                return false;
            }

            bool eq = Arguments.Length == other.Arguments.Length
                      && InterfaceName == other.InterfaceName
                      && IsGeneric == other.IsGeneric;

            if (!eq)
            {
                return false;
            }

            for (var i = 0; i < Arguments.Length; i++)
            {
                if (!Arguments[i].Equals(other.Arguments[i]))
                {
                    return false;
                }
            }

            return true;
        }

        /// <summary>Creates a new object that is a deep copy of the current instance. </summary>
        /// <returns>A new object that is a deep copy of this instance.</returns>
        public virtual IRTFactory Clone()
        {
            return new RTFactory(this.PrettyName, this.InterfaceName, this.Name, this.Arguments.Select(a => a.Clone()))
            {
                Options = this.Options,
            };
        }

        /// <summary>Returns a string that represents the current object.</summary>
        /// <returns>A string that represents the current object.</returns>
        /// <filterpriority>2</filterpriority>
        public override string ToString()
        {
            return $"ErrCode INTERFACE_FUNC {Name}({GenerateArguments()})";
        }

        private string GenerateArguments()
        {
            if (Arguments == null)
            {
                return "";
            }

            StringBuilder sb = new StringBuilder();

            bool first = true;
            foreach (IArgument argument in Arguments)
            {
                if (!first)
                {
                    sb.Append(", ");
                }
                else
                {
                    first = false;
                }

                sb.Append($"{argument.Type.Name}{argument.Type.Modifiers} {argument.Name}");
            }

            return sb.ToString(); 
        }
    }
}
