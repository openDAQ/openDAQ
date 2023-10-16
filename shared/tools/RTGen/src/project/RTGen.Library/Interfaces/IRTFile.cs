using System.Collections.Generic;
using System.Runtime.Serialization;
using RTGen.Interfaces.Doc;

namespace RTGen.Interfaces
{
    /// <summary>Object representation of the source file.</summary>
    public interface IRTFile
    {
        /// <summary>Global variables.</summary>
        IList<ITypeName> Variables { get; set; }

        /// <summary>File enumerations.</summary>
        IList<IEnumeration> Enums { get; set; }

        /// <summary>File interfaces/classes.</summary>
        IList<IRTInterface> Classes { get; set; }

        /// <summary>File headers/includes/usings.</summary>
        IList<string> Includes { get; set; }

        /// <summary>Global methods.</summary>
        IList<IMethod> GlobalMethods { get; set; }

        /// <summary>Factory methods declared in the file.</summary>
        IList<IRTFactory> Factories { get; set; }

        /// <summary>Type aliases.</summary>
        IDictionary<string, ITypeName> TypeAliases { get; set; }

        /// <summary>Start namespace macro.</summary>
        /// <example>BEGIN_MUI_NAMESPACE, BEGIN_NAMESPACE_DEWESOFT_RT_CORE.</example>
        string StartNamespaceMacro { get; set; }

        /// <summary>End namespace macro.</summary>
        /// <example>END_MUI_NAMESPACE, END_NAMESPACE_DEWESOFT_RT_CORE.</example>
        string EndNamespaceMacro { get; set; }

        /// <summary>The filename of the parsed file.</summary>
        string SourceFileName { get; set; }

        /// <summary>Additional info using RT Attributes in RT Comments.</summary>
        IAttributeInfo AttributeInfo { get; set; }

        // TODO: Remove when support for generating multiple interfaces in one file is added.
        /// <summary>Currently parsed class.</summary>
        [IgnoreDataMember]
        IRTInterface CurrentClass { get; }

        /// <summary>Special feature flags.</summary>
        FileFeatures AdditionalFeatures { get; set; }

        /// <summary>Private data.</summary>
        object Tag { get; set; }

        /// <summary>Documentation comments before the interface.</summary>
        IList<IDocComment> LeadingDocumentation { get; set; }

        /// <summary>Documentation comments after the interface.</summary>
        IList<IDocComment> TrailingDocumentation { get; set; }
    }
}
