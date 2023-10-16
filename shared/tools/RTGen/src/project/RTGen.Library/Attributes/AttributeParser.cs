using RTGen.Exceptions;
using RTGen.Interfaces;

namespace RTGen.Attributes
{
    /// <summary>Represents RT Attribute parser with model.</summary>
    /// <typeparam name="TModel">The type of the mode.</typeparam>
    public abstract class AttributeParser<TModel>
    {
        /// <summary>The model to fill.</summary>
        protected readonly TModel Model;

        private readonly string[] _argumentNames;

        /// <summary>Initializes the Attribute parser.</summary>
        /// <param name="model">The model class to fill with parsed values.</param>
        public AttributeParser(TModel model)
        {
            Model = model;
            _argumentNames = new string[0];
        }

        /// <summary>Initializes the Attribute parser.</summary>
        /// <param name="model">The name of the parameter.</param>
        /// <param name="argumentNames">An array of parameter names in the order they should be specified it they are not named.</param>
        public AttributeParser(TModel model, string[] argumentNames)
        {
            Model = model;
            _argumentNames = argumentNames;
        }

        /// <summary>Parses the attribute parameters.</summary>
        /// <param name="arguments">The attribute parameters.</param>
        /// <exception cref="RTAttributeException">Throws on unknown parameters or invalid values.</exception>
        public virtual void Parse(IRTAttributeArgument[] arguments)
        {
            bool namedParam = false;

            for (int i = 0; i < arguments.Length; i++)
            {
                if (namedParam && !arguments[i].IsNamedParameter)
                {
                    throw new RTAttributeException("Only named parameters can follow after a named parameter.");
                }

                if (arguments[i].IsNamedParameter)
                {
                    namedParam = true;
                }

                ParseParameter(namedParam ? arguments[i].Name : _argumentNames[i], arguments[i].Value);
            }
        }

        /// <summary>Parses the parameter value and sets the model accordingly.</summary>
        /// <param name="paramName">The name of the parameter.</param>
        /// <param name="value">The parameter value.</param>
        protected abstract void ParseParameter(string paramName, string value);

        /// <summary>Parses the string as boolean otherwise throws an exception.</summary>
        /// <param name="boolString">The string to parse.</param>
        /// <param name="parameterName">The name of the parameter.</param>
        /// <returns>Returns the parsed boolean value otherwise throws an exception.</returns>
        /// <exception cref="RTAttributeException">Throws when the string could not be parsed to boolean.</exception>
        protected static bool ParseBoolOrThrow(string boolString, string parameterName)
        {
            if (bool.TryParse(boolString, out bool value))
            {
                return value;
            }

            throw new RTAttributeException($"Failed to covert \"{parameterName}\" parameter to bool: (\"{boolString}\")");
        }
    }
}
