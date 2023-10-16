using System;
using System.Collections.Generic;
using System.Linq;
using RTGen.Interfaces;
using RTGen.Types;
using RTGen.Util;

namespace RTGen.Delphi.Types
{
    class InterfaceSpecialization : Specialization
    {
        public InterfaceSpecialization(IRTInterface intf, IList<ITypeName> types)
            : base(Specialize(intf, types), types)
        {
        }

        private static IRTInterface Specialize(IRTInterface intf, IList<ITypeName> types)
        {
            if (!intf.Type.HasGenericArguments)
            {
                return intf;
            }

            int templateArgIndex = 0;
            IRTInterface specializedInteface = intf.Clone();
            ITypeName interfaceType = specializedInteface.Type;
            for (int i = 0; i < interfaceType.GenericArguments.Count; i++)
            {
                if (!interfaceType.GenericArguments[i].IsGenericArgument)
                {
                    continue;
                }

                interfaceType.GenericArguments[i] = types[templateArgIndex++];
            }

            interfaceType.Guid = InterfaceUuidGeneric(interfaceType);

            return specializedInteface;
        }


        /// <summary>Returns the UUID v5 of the specified generic interface name.</summary>
        /// <returns>Returns the version 5 GUID of the interface as used in RT Core.</returns>
        private static Guid InterfaceUuidGeneric(ITypeName type, string suffix = "")
        {
            string nameGeneric = GetFullyQualifiedNameGeneric(type, suffix);
            return Guid5.Create(Guid5.DnsNamespace, nameGeneric);
        }

        /// <summary>Computes the fully qualified interface name with generic type parameter from the specified <paramref name="type"/> info.</summary>
        /// <param name="type">The type info to use for computing the name.</param>
        /// <param name="suffix">The suffix to append to the interface name.</param>
        /// <returns>Returns the fully qualified interface name if it has a namespace define. Otherwise just returns the interface name.</returns>
        // ReSharper disable once MemberCanBePrivate.Global
        public static string GetFullyQualifiedNameGeneric(ITypeName type, string suffix = "")
        {
            if (type.Namespace.Components.Length == 0)
            {
                return GenericName(type) + suffix;
            }
            return GenericName(type) + suffix + "." + string.Join(".", type.Namespace.Components.Reverse());
        }

        private static string GenericName(ITypeName typeName)
        {
            if (typeName.GenericArguments == null)
            {
                return typeName.UnmappedName;
            }

            string genericArgs = string.Join(",", typeName.GenericArguments.Select(arg => arg?.UnmappedName));
            return $"{typeName.Name}<{genericArgs}>";
        }
    }
}
