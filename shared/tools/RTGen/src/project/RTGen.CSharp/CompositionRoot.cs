using LightInject;
using RTGen.CSharp;
using RTGen.CSharp.Generators;
using RTGen.Interfaces;

[assembly: CompositionRootType(typeof(CSharpCompositionRoot))]

namespace RTGen.CSharp
{
    public class CSharpCompositionRoot : ICompositionRoot
    {
        public void Compose(IServiceRegistry serviceRegistry)
        {
            serviceRegistry.Register<IGenerator, CSharpGenerator>("csharp");
            serviceRegistry.Register<IConfigGenerator, CSharpConfigGenerator>("csharp");
        }
    }
}
