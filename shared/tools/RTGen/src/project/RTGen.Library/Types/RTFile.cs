using System.Collections.Generic;
using RTGen.Interfaces;
using RTGen.Interfaces.Doc;

namespace RTGen.Types
{
    /// <summary>Object representation of the source file.</summary>
    public class RTFile : IRTFile
    {
        /// <summary>Initializes a new instance of the <see cref="RTFile" /> class.</summary>
        public RTFile(string namespaceSeparator,
                      string parameterSeparator,
                      ILibraryInfo libraryInfo,
                      INamespace coreNamespace = null)
        {
            Enums = new List<IEnumeration>();
            Variables = new List<ITypeName>();
            Classes = new List<IRTInterface>();
            Includes = new List<string>();
            AttributeInfo = new AttributeInfo(namespaceSeparator, parameterSeparator, libraryInfo, coreNamespace);
            GlobalMethods = new List<IMethod>();
            TypeAliases = new Dictionary<string, ITypeName>();
            Factories = new List<IRTFactory>();
            LeadingDocumentation = new List<IDocComment>();
            TrailingDocumentation = new List<IDocComment>();
        }

        /// <summary>Global variables.</summary>
        public IList<ITypeName> Variables { get; set; }

        /// <summary>File enumerations.</summary>
        public IList<IEnumeration> Enums { get; set; }

        /// <summary>File interfaces/classes.</summary>
        public IList<IRTInterface> Classes { get; set; }

        /// <summary>Global methods.</summary>
        public IList<IMethod> GlobalMethods { get; set; }

        /// <summary>Type aliases. (typedef / using) </summary>
        public IDictionary<string, ITypeName> TypeAliases { get; set; }

        /// <summary>File headers/includes/usings.</summary>
        public IList<string> Includes { get; set; }

        /// <summary>Factory methods declared in the file.</summary>
        public IList<IRTFactory> Factories { get; set; }

        /// <summary>Start namespace macro.</summary>
        /// <example>BEGIN_MUI_NAMESPACE, BEGIN_NAMESPACE_DEWESOFT_RT_CORE.</example>
        public string StartNamespaceMacro { get; set; }

        /// <summary>End namespace macro.</summary>
        /// <example>END_MUI_NAMESPACE, END_NAMESPACE_DEWESOFT_RT_CORE.</example>
        public string EndNamespaceMacro { get; set; }

        /// <summary>The filename of the source file.</summary>
        public string SourceFileName { get; set; }

        /// <summary>Additional info using RT Attributes in RT Comments.</summary>
        public IAttributeInfo AttributeInfo { get; set; }

        /// <summary>Special feature flags.</summary>
        public FileFeatures AdditionalFeatures { get; set; }

        /// <summary>Currently parsed class.</summary>
        public IRTInterface CurrentClass
        {
            get
            {
                if (Classes.Count == 0)
                {
                    return null;
                }

                return Classes[Classes.Count - 1];
            }
        }

        /// <summary>Private data.</summary>
        public object Tag { get; set; }

        /// <summary>Documentation comments before the interface.</summary>
        public IList<IDocComment> LeadingDocumentation { get; set; }

        /// <summary>Documentation comments after the interface.</summary>
        public IList<IDocComment> TrailingDocumentation { get; set; }
    }
}
