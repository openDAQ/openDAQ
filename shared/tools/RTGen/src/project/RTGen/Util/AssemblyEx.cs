using System;
using System.IO;
using System.Reflection;
using LightInject;

namespace RTGen.Util {

    /// <summary>Utility methods for assembly loading.</summary>
    public static class AssemblyEx
    {

        private static readonly Type CompositionRootType = typeof(CompositionRootTypeAttribute);

        /// <summary>Load assembly from filepath.</summary>
        /// <param name="domain">The domain under which to load the assembly.</param>
        /// <param name="path">The filepath to the assembly.</param>
        /// <returns>The loaded <see cref="Assembly"/> otherwise throws an exception.</returns>
        /// <exception cref="T:System.ArgumentNullException"><paramref name="domain" /> or <paramref name="path"/> is <see langword="null" />. </exception>
        /// <exception cref="T:System.IO.FileNotFoundException"><paramref name="path" /> is not found. </exception>
        /// <exception cref="T:System.BadImageFormatException"><paramref name="path" /> is not a valid assembly. -or-Version 2.0 or later of the common language runtime is currently loaded and <paramref name="path" /> was compiled with a later version.</exception>
        /// <exception cref="T:System.AppDomainUnloadedException">The operation is attempted on an unloaded application domain. </exception>
        /// <exception cref="T:System.IO.FileLoadException">An assembly or module was loaded twice with two different evidences. </exception>
        public static Assembly LoadFromFile(this AppDomain domain, string path)
        {
            AssemblyName name = new AssemblyName { CodeBase = path };
            return domain.Load(name);
        }

        /// <summary>Checks if the specified file is a valid RTGen plugin.</summary>
        /// <param name="domain">The <see cref="AppDomain"/> under which to try to load the assemby.</param>
        /// <param name="pluginFullPath">The full path to the plugin candidate file.</param>
        /// <returns></returns>
        public static bool CheckIsPlugin(AppDomain domain, string pluginFullPath) {

            if (!CheckIsAssembly(pluginFullPath)) {
                return false;
            }

            try {
                Assembly asm = domain.LoadFromFile(pluginFullPath);
                CompositionRootTypeAttribute isPlugin = asm.GetCustomAttribute<CompositionRootTypeAttribute>();

                if (isPlugin != null)
                {
                    return true;
                }

                return false;
            }
            catch (Exception) {
                Log.Warning($"Could not determine if {Path.GetFileName(pluginFullPath)} is a valid plugin.");
                return false;
            }
        }

        /// <summary>Checks if the file is a valid CLR assembly.</summary>
        /// <param name="fileName">Name of the file.</param>
        /// <returns>Returns <c>true</c> if the file is a valid CLR assembly otherwise <c>false</c>.</returns>
        /// <remarks>Taken from http://geekswithblogs.net/rupreet/archive/2005/11/02/58873.aspx </remarks>
        private static bool CheckIsAssembly(string fileName) {
            uint[] dataDictionaryRva = new uint[16];
            uint[] dataDictionarySize = new uint[16];

            using (Stream fs = new FileStream(fileName, FileMode.Open, FileAccess.Read)) {
                using (BinaryReader reader = new BinaryReader(fs)) {

                    //PE Header starts @ 0x3C (60). Its a 4 byte header.
                    fs.Position = 0x3C;

                    uint peHeader = reader.ReadUInt32();

                    //Moving to PE Header start location...
                    fs.Position = peHeader;
                    reader.ReadUInt32();

                    //We can also show all these value, but we will be limiting to the CLI header test.
                    reader.ReadUInt16();
                    reader.ReadUInt16();
                    reader.ReadUInt32();
                    reader.ReadUInt32();
                    reader.ReadUInt32();
                    reader.ReadUInt16();
                    reader.ReadUInt16();

                    /*
                     *    Now we are at the end of the PE Header and from here, the PE Optional Headers starts...
                     *    To go directly to the datadictionary, we'll increase the stream’s current position to with 96 (0x60). 96 because,
                     *    28 for Standard fields, 68 for NT-specific fields
                     *    From here DataDictionary starts...and its of total 128 bytes.
                     *    DataDictionay has 16 directories in total, doing simple maths 128/16 = 8.
                     * 
                     *    So each directory is of 8 bytes.
                     *    In this 8 bytes, 4 bytes is of RVA and 4 bytes of Size.
                     *
                     *    btw, the 15th directory consist of CLR header! if its 0, its not a CLR file :)
                     */
                    ushort dataDictionaryStart = Convert.ToUInt16(Convert.ToUInt16(fs.Position) + 0x60);
                    fs.Position = dataDictionaryStart;
                    for (int i = 0; i < 15; i++) {
                        dataDictionaryRva[i] = reader.ReadUInt32();
                        dataDictionarySize[i] = reader.ReadUInt32();
                    }

                    return dataDictionaryRva[14] != 0;
                }
            }
        }
    }
}
