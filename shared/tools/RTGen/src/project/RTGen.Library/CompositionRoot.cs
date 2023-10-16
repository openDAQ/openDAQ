using System;
using LightInject;
using RTGen;
using RTGen.Generation;
using RTGen.Interfaces;
using RTGen.Util;

[assembly: CompositionRootType(typeof(CompositionRoot))]

namespace RTGen
{
    /// <summary>Registers fallback generator.</summary>
    public class CompositionRoot : ICompositionRoot
    {
        private static readonly Type GeneratorType = typeof(IGenerator);

        /// <summary>Registers a fallback template generator when the desired output generator is not found.</summary>
        /// <param name="serviceRegistry">The implementations registry.</param>
        public void Compose(IServiceRegistry serviceRegistry)
        {
            serviceRegistry.RegisterFallback(
                (type, serviceName) => type == GeneratorType,
                request =>
                {
                    Log.Warning($"No plugin found for {request.ServiceType.Name} with name \"{request.ServiceName}\". Falling back to base template generator.");
                    return new FallbackGenerator();
                });
        }
    }
}
