using LightInject;
using RTGen.Interfaces;
using RTGen.Json;
using RTGen.Json.Generators;

[assembly: CompositionRootType(typeof(JsonCompositionRoot))]

namespace RTGen.Json
{
    public class JsonCompositionRoot : ICompositionRoot
    {
        public void Compose(IServiceRegistry serviceRegistry)
        {
            serviceRegistry.Register<IGenerator, JsonGenerator>("json");
            serviceRegistry.Register<IConfigGenerator, JsonConfigGenerator>("json");
        }
    }
}
