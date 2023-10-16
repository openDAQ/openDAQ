using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using RTGen.Delphi.Generators;
using RTGen.Delphi.Types;
using RTGen.Interfaces;
using RTGen.Types;
using Argument = RTGen.Types.Argument;

namespace RTGen.Delphi.Util
{
    internal class FactoryConstructor
    {
        private readonly IAttributeInfo _info;
        private readonly string _tagName;

        public FactoryConstructor(IRTFactory factory, IAttributeInfo info)
        {
            Factories = new List<IRTFactory> {
                factory
            };

            _info = info;
            _tagName = factory.InterfaceName.Substring(1) + "Type";
        }

        public List<IRTFactory> Factories { get; }

        public IRTFactory ToFactory(IRTFile file)
        {
            if (Factories.Count == 1)
            {
                return !Factories[0].IsGeneric
                           ? new RTFactory(Factories[0], true)
                           : new RTGenericFactory((IRTGenericFactory)Factories[0], true);
            }

            file.Enums.Add(CreateEnumFromTags());

            IArgument enumKind = new Argument(new TypeName(_info, (INamespace) null, $"T{_tagName}", ""), "kind");
            RTConstructor constructor = new RTConstructor(Factories[0], enumKind, DelphiGenerator.GetEnumPrefix(_tagName));

            for (int i = 1; i < Factories.Count; i++)
            {
                constructor.Factories.Add(new FactoryWithTag(Factories[i]));
            }

            return constructor;
        }

        private IEnumeration CreateEnumFromTags()
        {
            List<IEnumOption> options = new List<IEnumOption>();

            foreach (IRTFactory factory in Factories)
            {
                options.Add(new EnumOption(factory.Tag));
            }

            return new Enumeration(_tagName, options);
        }
    }
}
