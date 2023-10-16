using LightInject;
using RTGen.Delphi;
using RTGen.Delphi.Generators;
using RTGen.Interfaces;

[assembly: CompositionRootType(typeof(DelphiCompositionRoot))]

namespace RTGen.Delphi
{
    public class DelphiCompositionRoot : ICompositionRoot
    {
        public void Compose(IServiceRegistry serviceRegistry)
        {
            serviceRegistry.Register<IGenerator, DelphiGenerator>("delphi");
            serviceRegistry.Register<IConfigGenerator, DelphiConfigGenerator>("delphi");
            serviceRegistry.Register<IParser, DelphiSourceParser>("delphi");
        }
    }
}
