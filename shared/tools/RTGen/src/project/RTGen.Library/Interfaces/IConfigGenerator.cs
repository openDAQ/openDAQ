using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RTGen.Interfaces
{
    /// <summary>Config / DLL load file generator.</summary>
    public interface IConfigGenerator : IGeneratorBase
    {
        /// <summary>Generate the config to a file.</summary>
        /// <param name="templatePath">The template to generate from. Pass <c>null</c> if not generating from a template.</param>
        void GenerateConfigFile(string templatePath);
    }
}
