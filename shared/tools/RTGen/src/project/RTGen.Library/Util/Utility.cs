using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using RTGen.Types;
using RTGen.Interfaces;
using RTGen.Exceptions;
using System.Reflection;
using System.Globalization;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;

namespace RTGen.Util
{
    /// <summary>Utility methods.</summary>
    public static class Utility
    {
        /// <summary>Join all enum value names into a string with the specified spearator.</summary>
        /// <param name="separator">The separator to use between enum values.</param>
        /// <typeparam name="TEnum">The enum which value names to get.</typeparam>
        /// <returns>Returns all the enum value names separated by the specified separator.</returns>
        /// <exception cref="ArgumentException"></exception>
        public static string EnumNames<TEnum>(string separator) where TEnum : struct
        {
            Type enumType = typeof(TEnum);

            if (!enumType.IsEnum) {
                throw new ArgumentException("The argument is not an enumeration.");
            }

            return string.Join(separator, enumType.GetEnumNames());
        }

        /// <summary>Parses a semantic version encoded in a string.</summary>
        /// <param name="versionString">A major.minor.patch version encoded in a string.</param>
        /// <returns>A VersionInfo instance from the parsed string.</returns>
        public static VersionInfo ParseVersionInfo(string versionString)
        {
            VersionInfo version = new VersionInfo();
            string[] versionSplit = versionString.Split('.');

            for (int i = 0; i < versionSplit.Length; i++)
            {
                switch (i)
                {
                    case 0:
                        version.Major = int.Parse(versionSplit[i]);
                        break;
                    case 1:
                        version.Minor = int.Parse(versionSplit[i]);
                        break;
                    case 2:
                        version.Patch = int.Parse(versionSplit[i]);
                        break;
                }
            }

            return version;
        }

        /// <summary>Try to load template from the current working directory then falling back to the RTGen directory.</summary>
        /// <param name="fileName">The filename of the template without the folder path.</param>
        /// <returns>The full path to the template if found otherwise throws an exception.</returns>
        /// <exception cref="ArgumentException">Throws when the filename without folder path is <c>null</c> or whitespace.</exception>
        /// <exception cref="FileNotFoundException">Throws when locating the template failed.</exception>
        public static string GetTemplate(string fileName)
        {
            fileName = Path.GetFileName(fileName);

            if (string.IsNullOrEmpty(fileName))
            {
                throw new ArgumentException("Filename must not be null.");
            }

            if (File.Exists(fileName))
            {
                return Path.GetFullPath(fileName);
            }

            string rtGenDir = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location) ?? "";


            string rtGenDirFile = Path.Combine(rtGenDir, "Templates", fileName);
            if (File.Exists(rtGenDirFile))
            {
                return rtGenDirFile;
            }

            rtGenDirFile = Path.Combine(rtGenDir, fileName);
            if (File.Exists(rtGenDirFile))
            {
                return rtGenDirFile;
            }

            throw new TemplateNotFoundException(rtGenDirFile);
        }

        /// <summary>Converts the string to start with uppercase letter.</summary>
        /// <param name="str">The string to capitalize.</param>
        /// <returns>Returns a capitalized input text.</returns>
        public static string Capitalize(this string str)
        {
            if (string.IsNullOrEmpty(str))
            {
                return str;
            }

            return Char.ToUpperInvariant(str[0]) + str.Substring(1);
        }

        /// <summary>Converts the string to start with uppercase letter.</summary>
        /// <param name="str">The string to capitalize.</param>
        /// <returns>Returns a capitalized input text.</returns>
        // ReSharper disable once IdentifierTypo
        public static string Uncapitalize(this string str)
        {
            if (string.IsNullOrEmpty(str))
            {
                return str;
            }

            return Char.ToLowerInvariant(str[0]) + str.Substring(1);
        }

        /// <summary>Computes the fully qualified interface name from the specified <paramref name="type"/> info.</summary>
        /// <param name="type">The type info to use for computing the name.</param>
        /// <param name="suffix">The suffix to append to the interface name.</param>
        /// <returns>Returns the fully qualified interface name if it has a namespace define. Otherwise just returns the interface name.</returns>
        // ReSharper disable once MemberCanBePrivate.Global
        public static string GetFullyQualifiedName(ITypeName type, string suffix = "")
        {
            if (type.Namespace.Components.Length == 0)
            {
                return type.Name + suffix;
            }
            return type.Name + suffix + "." + string.Join(".", type.Namespace.Components.Reverse());
        }

        /// <summary>Gets the default value of a type (similar to default(T))</summary>
        /// <param name="type">The type for which to get a default value.</param>
        /// <returns>Returns the default value for the specified type.</returns>
        public static object GetInitialDefaultValue(this Type type)
        {
            return type.IsValueType
                ? Activator.CreateInstance(type)
                : null;
        }

        /// <summary>Removes all the trailing new lines in the string.</summary>
        /// <param name="str">The string to trim.</param>
        public static string TrimTrailingNewLine(this string str)
        {
            if (string.IsNullOrEmpty(str))
            {
                return str;
            }

            string output = str.TrimEnd(' ', '\r', '\n');
            return output;
        }

        /// <summary>Removes the trailing new line in the string buffer.</summary>
        /// <param name="buffer">The string buffer to trim.</param>
        public static void TrimTrailingNewLine(this StringBuilder buffer)
        {
            if (buffer.Length > 2)
            {
                buffer.Replace(
                    Environment.NewLine,
                    string.Empty,
                    buffer.Length - 2,
                    2
                );
            }
        }

        /// <summary>Removes all the trailing new lines in the string buffer.</summary>
        /// <param name="buffer">The string buffer to trim.</param>
        public static void TrimTrailingNewLines(this StringBuilder buffer)
        {
            int cutPosition = buffer.Length;
            int newLineLength = Environment.NewLine.Length;

            for (int i = buffer.Length - newLineLength; i > 0; i -= newLineLength)
            {
                bool isNewLine = true;
                for (int j = 0; j < newLineLength; j++)
                {
                    if (buffer[i + j] != Environment.NewLine[j])
                    {
                        isNewLine = false;
                        break;
                    }
                }

                if (isNewLine)
                {
                    cutPosition = i;
                }
                else
                {
                    break;
                }
            }

            if (cutPosition == buffer.Length)
            {
                return;
            }

            //cutPosition += newLineLength;

            buffer.Remove(cutPosition, buffer.Length - cutPosition);
        }

        /// <summary>Converts parameter <paramref name="typeName"/> from camel case to snake case beginning at <paramref name="start"/> offset.</summary>
        /// <param name="sb">Buffer to write snake-case type name.</param>
        /// <param name="typeName">The camel case string to convert.</param>
        /// <param name="start">The offset at which to start converting.</param>
        /// <returns>Returns the same StringBuilder, but with the converted type name appended.</returns>
        public static StringBuilder ToLowerSnakeCase(this StringBuilder sb, string typeName, int start)
        {
            for (var i = start; i < typeName.Length; i++)
            {
                char letter = typeName[i];
                if (char.IsUpper(letter) && i != 0)
                {
                    sb.Append("_");
                }

                sb.Append(char.ToLower(letter, CultureInfo.InvariantCulture));
            }

            return sb;
        }

        /// <summary>Returns the UUID v5 of the specified interface name.</summary>
        /// <returns>Returns the version 5 GUID of the interface as used in RT Core.</returns>
        public static Guid InterfaceUuid(ITypeName type, string suffix = "")
        {
            return Guid5.Create(Guid5.DnsNamespace, GetFullyQualifiedName(type, suffix));
        }

        /// <summary>Returns the UUID v5 of the specified interface name.</summary>
        /// <param name="fullyQualifiedName">The fully qualified name of the interface. (e.g.: "ISomeInterface.Core.RT.Dewesoft").</param>
        /// <returns>Returns the version 5 GUID of the interface as used in RT Core.</returns>
        public static Guid InterfaceUuid(string fullyQualifiedName)
        {
            return Guid5.Create(Guid5.DnsNamespace, fullyQualifiedName);
        }

        /// <summary>Creates a ITypeName instance from the fully qualified C++ type name.</summary>
        /// <param name="typeText">The fully qualified C++ type name.</param>
        /// <param name="info">The attribute info to pass to the ITypeName.</param>
        /// <returns>Returns the interface type info or <c>null</c> if the text is empty or <c>null</c>.</returns>
        public static ITypeName GetTypeNameFromString(string typeText, IAttributeInfo info)
        {
            if (string.IsNullOrEmpty(typeText))
            {
                return null;
            }

            typeText = typeText.Trim('"');

            const string NAMESPACE_SEPARATOR = "::";
            int last = typeText.LastIndexOf(NAMESPACE_SEPARATOR, StringComparison.InvariantCultureIgnoreCase);
            if (last == -1)
            {
                string argNamespace = !info.IsCoreType(typeText)
                                          ? info.DefaultNamespace?.Raw
                                          : AttributeInfo.DEFAULT_CORE_NAMESPACE;

                return new TypeName(info, argNamespace, typeText, null);
            }

            string nsText = typeText.Substring(0, last);
            typeText = typeText.Substring(last + NAMESPACE_SEPARATOR.Length, typeText.Length - last - NAMESPACE_SEPARATOR.Length);

            return new TypeName(info, nsText, typeText, null);
        }

        /// <summary>Converts parameter <paramref name="typeName"/> from camel case to snake case beginning at <paramref name="start"/> offset.</summary>
        /// <param name="typeName">The camel case string to convert.</param>
        /// <param name="start">The offset at which to start converting.</param>
        /// <returns>Returns the converted type name.</returns>
        public static string ToLowerSnakeCase(this string typeName, int start = 0)
        {
            StringBuilder buffer = new StringBuilder();

            for (var i = start; i < typeName.Length; i++)
            {
                char letter = typeName[i];
                if (char.IsUpper(letter) && i != 0)
                {
                    buffer.Append("_");
                }

                buffer.Append(char.ToLower(letter, CultureInfo.InvariantCulture));
            }

            return buffer.ToString();
        }

        /// <summary>
        /// Perform a deep copy of the object using a BinaryFormatter.
        /// IMPORTANT: the object class must be marked as [Serializable] and have an parameterless constructor.
        /// </summary>
        /// <typeparam name="T">The type of object being deep copied.</typeparam>
        /// <param name="source">The object instance to deep copy.</param>
        /// <returns>The deep copied object.</returns>
        public static T Clone<T>(this T source)
        {
            // Don't serialize a null object, simply return the default for that object
            if (ReferenceEquals(source, null))
            {
                return default(T);
            }

            IFormatter formatter = new BinaryFormatter();
            using (Stream stream = new MemoryStream())
            {
                formatter.Serialize(stream, source);
                stream.Seek(0, SeekOrigin.Begin);
                return (T)formatter.Deserialize(stream);
            }
        }

        /// <summary>Deconstruct key-value pair into a key and a value.</summary>
        /// <param name="tuple">The key-value pair to deconstruct.</param>
        /// <param name="key">The resulting key.</param>
        /// <param name="value">The resulting value.</param>
        /// <typeparam name="T1">The type of the key.</typeparam>
        /// <typeparam name="T2">The type of the value.</typeparam>
        public static void Deconstruct<T1, T2>(this KeyValuePair<T1, T2> tuple, out T1 key, out T2 value)
        {
            key = tuple.Key;
            value = tuple.Value;
        }
    }
}
