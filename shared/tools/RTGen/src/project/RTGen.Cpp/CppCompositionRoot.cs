using LightInject;
using RTGen.Cpp;
using RTGen.Interfaces;
using RTGen.Cpp.Generators;

// assembly global attribute so the CompositionRoot is easier to find as its not necessary to scan all types in the assembly.
[assembly: CompositionRootType(typeof(CppCompositionRoot))]

namespace RTGen.Cpp
{
    public class CppCompositionRoot : ICompositionRoot
    {
        /// <summary>Registers all the interface implementations to the RTGen.</summary>
        /// <param name="serviceRegistry">The implementations registry.</param>
        public void Compose(IServiceRegistry serviceRegistry)
        {
            // Register parser / generator with the language identifier. 

            // used when program option "lang" = "cpp"
            serviceRegistry.Register<IGenerator, CppGenerator>("cpp");

            // used when program option "inlang" = "cpp"
            serviceRegistry.Register<IParser, CppParser>("cpp");

            //////////////////////////////////

            // used when program option "inlang" = "cpp"
            serviceRegistry.Register<IParser, TemplateTypesParser>("corestructure_templates");

            // used when program option "lang" = "corestructure_templates"
            serviceRegistry.Register<IGenerator, TemplateTypesGenerator>("corestructure_templates");
        }
    }
}
