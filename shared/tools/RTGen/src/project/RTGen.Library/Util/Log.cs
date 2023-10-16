using System;
using System.IO;

namespace RTGen.Util
{
    /// <summary>Simple logging utility class.</summary>
    public static class Log
    {
        static Log() {
            WarningColor = ConsoleColor.Yellow;
            ErrorColor = ConsoleColor.Red;
            InfoColor = Console.ForegroundColor;

            LogInfoWriter = Console.Out;
            LogErrorWriter = Console.Error;
        }

        /// <summary>Whether the logger is in <b>verbose</b> mode there are more info messages and they are more detailed.</summary>
        public static bool Verbose { get; set; }

        /// <summary>If the logger has been used at all.</summary>
        public static bool HasLogged { get; private set; }

        /// <summary>If logger outputted any warnings.</summary>
        public static bool HasWarnings { get; private set; }

        /// <summary>If the logger outputted any errors.</summary>
        public static bool HasErrors { get; private set; }

        /// <summary>The console color when printing the warning to the screen.</summary>
        public static ConsoleColor WarningColor { get; set; }

        /// <summary>The console color when printing an error to the screen.</summary>
        public static ConsoleColor ErrorColor { get; set; }

        /// <summary>The console color when printing info to the screen.</summary>
        public static ConsoleColor InfoColor { get; set; }

        /// <summary>Where to output error and warning info. By default it is the console error stream.</summary>
        public static TextWriter LogErrorWriter { get; set; }

        /// <summary>Where to output error and warning info. By default it is the console.</summary>
        public static TextWriter LogInfoWriter { get; set; }

        /// <summary>Log an info message.</summary>
        /// <param name="message">The message.</param>
        public static void Info(string message) {
            LogWithColor(message, LogInfoWriter, InfoColor);
        }

        /// <summary>Log a warning message.</summary>
        /// <param name="message">The message.</param>
        public static void Warning(string message) {
            LogWithColor($"Warning: {message}", LogErrorWriter, WarningColor);
            HasWarnings = true;
        }

        /// <summary>Log an error message.</summary>
        /// <param name="message">The message.</param>
        public static void Error(string message) {
            LogWithColor(message, LogErrorWriter, ErrorColor);
            HasErrors = true;
        }

        private static void LogWithColor(string message, TextWriter writer, ConsoleColor color) {
            ConsoleColor backup = Console.ForegroundColor;
            Console.ForegroundColor = color;

            writer.WriteLine(message);

            Console.ForegroundColor = backup;

            HasLogged = true;
        }
    }
}
