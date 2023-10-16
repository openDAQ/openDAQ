namespace RTGen.Exceptions
{
    /// <summary>Represents an exception when the template could not be located.</summary>
    public class TemplateNotFoundException : GeneratorException
    {
        /// <summary>Initializes a new instance of the <see cref="GeneratorException" /> class.</summary>
        /// <param name="filename">The template that could not be found.</param>
        public TemplateNotFoundException(string filename) : base($"Could not locate the specified template: \"{filename}\".")
        {
        }
    }
}
