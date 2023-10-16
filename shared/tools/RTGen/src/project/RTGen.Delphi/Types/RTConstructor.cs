using System;
using System.Collections.Generic;
using System.Linq;
using RTGen.Interfaces;
using RTGen.Types;

namespace RTGen.Delphi.Types
{
    [Serializable]
    internal class FactoryWithTag
    {
        public FactoryWithTag(string name, string tag)
        {
            Tag = tag;
            Name = name;
        }

        public FactoryWithTag(IRTFactory factory)
        {
            Name = factory.Name;
            Tag = factory.Tag;
        }

        public string Tag { get; private set; }

        public string Name { get; private set; }
    }

    class RTFactories
    {
        public RTFactories(IList<IRTFactory> factories, int index)
        {
            Factories = factories;
            Index = index;
        }

        public IList<IRTFactory> Factories { get; }
        public int Index { get; }
    }

    [Serializable]
    class RTConstructor : RTFactory
    {
        public RTConstructor(IRTFactory factory, IArgument kind, string enumPrefix = null) : base(factory, kind)
        {
            IsConstructorWithMultipleFactories = true;

            Factories = new List<FactoryWithTag>
            {
                new FactoryWithTag(factory.Name, factory.Tag)
            };

            EnumPrefix = enumPrefix;
        }
        
        public override bool IsConstructorWithMultipleFactories { get; }

        public List<FactoryWithTag> Factories { get; }

        public string EnumPrefix { get; }

        /// <summary>Returns the factory without the disambiguating enum (Kind parameter)</summary>
        /// <returns>Constructor as a method with only actual parameters.</returns>
        public IOverload ToWrappedOverload()
        {
            return new Method(Name, Arguments.Skip(1).ToArray()).Overloads[0];
        }
    }
}
