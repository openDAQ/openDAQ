using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using RTGen.Exceptions;
using RTGen.Interfaces;
using RTGen.Types;

namespace RTGen.Util
{
    class AbstractConverter<TReal, TAbstract> : JsonConverter where TReal : TAbstract
    {
        public override Boolean CanConvert(Type objectType) => objectType == typeof(TAbstract);
        public override Object ReadJson(JsonReader reader, Type type, Object value, JsonSerializer jser) => jser.Deserialize<TReal>(reader);
        public override void WriteJson(JsonWriter writer, Object value, JsonSerializer jser) => jser.Serialize(writer, value);
    }

    class SampleTypesConverter : JsonConverter
    {
        public override Boolean CanConvert(Type objectType) => objectType == typeof(ISampleTypes);

        public override Object ReadJson(JsonReader reader, Type type, Object value, JsonSerializer serializer)
        {
            var jObject = JObject.Load(reader);

            SampleTypes st = new SampleTypes("!%$Unknown$%!", jObject["Rank"].Value<int>());
            serializer.Populate(jObject.CreateReader(), st);
            return st;
        }

        public override void WriteJson(JsonWriter writer, Object value, JsonSerializer jser) => jser.Serialize(writer, value);
    }

    class TypeNameConverter : JsonConverter
    {
        private static readonly AttributeInfo AttributeInfo = new AttributeInfo(".", ",", new LibraryInfo());

        public override Boolean CanConvert(Type objectType) => objectType == typeof(ITypeName);

        public override Object ReadJson(JsonReader reader, Type type, Object value, JsonSerializer serializer)
        {
            var jObject = JObject.Load(reader);
            string name = jObject["UnmappedName"].Value<string>();

            ITypeName typeName = new PredefinedTypeName(AttributeInfo, null, name, "");
            serializer.Populate(jObject.CreateReader(), typeName);
            return typeName;
        }

        public override void WriteJson(JsonWriter writer, Object value, JsonSerializer jser)
        {
            jser.Serialize(writer, value);
        } 
    }

    /// <summary>Handles JSON serialization and deserialization.</summary>
    public static class Json
    {
        private static readonly JsonSerializerSettings Settings = new JsonSerializerSettings
        {
            Converters =
            {
                new TypeNameConverter(),
                new SampleTypesConverter(),
                new AbstractConverter<TypeName, ITypeName>(),
                new AbstractConverter<Namespace, INamespace>(),
                new AbstractConverter<WrapperType, IWrapperType>(),
            }
        };

        /// <summary>Serialize the argument to JSON.</summary>
        /// <param name="obj">The object to serialize.</param>
        /// <typeparam name="T">The type of the object.</typeparam>
        /// <returns>The object serialized to a JSON string.</returns>
        public static string Serialize<T>(T obj)
        {
            return JsonConvert.SerializeObject(obj, Formatting.Indented);
        }

        /// <summary>Deserialize the JSON to object.</summary>
        /// <param name="json">The string to deserialize.</param>
        /// <returns>The object deserialized from a JSON string.</returns>
        public static object DeserializeObject(string json)
        {
            return JsonConvert.DeserializeObject(json);
        }

        /// <summary>Deserialize the JSON to object.</summary>
        /// <param name="json">The string to deserialize.</param>
        /// <returns>The object deserialized from a JSON string.</returns>
        public static T Deserialize<T>(string json)
        {
            return JsonConvert.DeserializeObject<T>(json, Settings);
        }
    }
}
