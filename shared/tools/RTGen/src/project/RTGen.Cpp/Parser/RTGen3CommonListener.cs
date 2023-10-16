using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using RTGen.Interfaces;
using RTGen.Types;

namespace RTGen.Cpp.Parser
{
    public class RTGen3CommonListener : RTGen3BaseListener
    {
        protected readonly IParserOptions Options;

        protected RTGen3CommonListener(IParserOptions options, RTFile file)
        {
            Options = options;
            File = file;
        }

        public RTFile File { get; }

        protected AttributeInfo Attributes => (AttributeInfo)File.AttributeInfo;

        /// <summary>Create TypeName info from the parsed type.</summary>
        /// <param name="context"></param>
        /// <param name="useLibraryDefault"></param>
        /// <returns></returns>
        protected TypeName GetTypeNameFromContext(RTGen3.TypeContext context, bool useLibraryDefault = true)
        {
            if (context == null)
            {
                return null;
            }

            if (context.Identifier() == null)
            {
                return GetTypeNameFromContext(context.type());
            }

            string namespaceName = null;
            string typeIdentifier = context.Identifier().GetText();

            bool templateArg = false;
            if (File.CurrentClass != null)
            {
                templateArg = File.CurrentClass.Type.GenericArguments != null &&
                              File.CurrentClass.Type.GenericArguments.Any(type => type.Namespace.Components.Length == 0 &&
                                                                                  type.Name == typeIdentifier);
            }

            if (useLibraryDefault && context.@namespace() == null
                                  && !char.IsLower(typeIdentifier[0])
                                  && !templateArg
                )
            {
                if (File.StartNamespaceMacro == AttributeInfo.DEFAULT_CORE_NAMESPACE_MACRO
                    && string.IsNullOrEmpty(File.EndNamespaceMacro))
                {
                    // Type is between BEGIN / END namespace macros
                    namespaceName = AttributeInfo.DEFAULT_CORE_NAMESPACE;
                }
                else
                {
                    namespaceName = !File.AttributeInfo.IsCoreType(typeIdentifier)
                        ? Options.LibraryInfo.Namespace?.Raw
                        : File.AttributeInfo.CoreNamespace?.Raw;
                }
            }
            else if (context.@namespace() != null)
            {
                namespaceName = context.@namespace().GetText();
            }

            List<ITypeName> genericArgs = null;
            RTGen3.TemplateArgsContext genericTypes = context.templateArgs();
            if (genericTypes != null)
            {
                genericArgs = new List<ITypeName>();
                foreach (var templateType in genericTypes.templateIdentifier())
                {
                    genericArgs.Add(GetTypeNameFromContext(templateType.type()));
                }
            }

            return new TypeName(
                File.AttributeInfo,
                namespaceName,
                typeIdentifier,
                context.ptrOperators()?.GetText()
            )
            {
                GenericArguments = genericArgs,
                IsGenericArgument = templateArg
            };
        }
    }
}
