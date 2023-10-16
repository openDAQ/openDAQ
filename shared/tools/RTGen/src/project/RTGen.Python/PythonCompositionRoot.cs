using LightInject;
using RTGen.Interfaces;
using RTGen.Python;
using RTGen.Python.Generators;

[assembly: CompositionRootType(typeof(PythonCompositionRoot))]

namespace RTGen.Python
{
    public class PythonCompositionRoot : ICompositionRoot
    {
        public void Compose(IServiceRegistry serviceRegistry)
        {
            serviceRegistry.Register<IGenerator, PythonGenerator>("python");
            serviceRegistry.Register<IConfigGenerator, PythonConfigGenerator>("python");
        }
    }
}
