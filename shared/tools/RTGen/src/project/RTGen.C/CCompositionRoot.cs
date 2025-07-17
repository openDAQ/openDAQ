using LightInject;
using RTGen.Interfaces;
using RTGen.C;
using RTGen.C.Generators;

[assembly: CompositionRootType(typeof(CCompositionRoot))]

namespace RTGen.C
{
    public class CCompositionRoot : ICompositionRoot
    {
        public void Compose(IServiceRegistry serviceRegistry)
        {
            serviceRegistry.Register<IGenerator, CGenerator>("c");
        }
    }
}
