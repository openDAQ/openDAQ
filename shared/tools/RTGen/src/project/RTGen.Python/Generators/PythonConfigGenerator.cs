using RTGen.Interfaces;

namespace RTGen.Python.Generators
{
    public class PythonConfigGenerator : IConfigGenerator
    {
        public IGeneratorOptions Options { get; set; }
        public IVersionInfo Version { get; }

        public void GenerateConfigFile(string templatePath)
        {
        }
    }
}
