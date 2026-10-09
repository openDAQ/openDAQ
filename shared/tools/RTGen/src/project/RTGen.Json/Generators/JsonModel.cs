using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;

namespace RTGen.Json.Generators
{
    /// <summary>Insertion-ordered string-keyed map used as the JSON object model.</summary>
    class OrderedDict : IEnumerable<KeyValuePair<string, object>>
    {
        private readonly List<KeyValuePair<string, object>> _items = new List<KeyValuePair<string, object>>();

        public int Count => _items.Count;

        public void Add(string key, object value)
        {
            _items.Add(new KeyValuePair<string, object>(key, value));
        }

        public bool ContainsKey(string key)
        {
            return _items.Any(item => item.Key == key);
        }

        public object Get(string key)
        {
            foreach (KeyValuePair<string, object> item in _items)
            {
                if (item.Key == key)
                {
                    return item.Value;
                }
            }
            return null;
        }

        public void Set(string key, object value)
        {
            for (int i = 0; i < _items.Count; i++)
            {
                if (_items[i].Key == key)
                {
                    _items[i] = new KeyValuePair<string, object>(key, value);
                    return;
                }
            }
            Add(key, value);
        }

        public void Remove(string key)
        {
            _items.RemoveAll(item => item.Key == key);
        }

        public string GetString(string key) => Get(key) as string;

        public List<object> GetList(string key) => Get(key) as List<object>;

        public OrderedDict GetDict(string key) => Get(key) as OrderedDict;

        public IEnumerator<KeyValuePair<string, object>> GetEnumerator() => _items.GetEnumerator();

        IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();
    }

    /// <summary>Minimal JSON serializer/parser for the OrderedDict/List/primitive object
    /// model (avoids a runtime dependency on any JSON library).</summary>
    static class JsonValue
    {
        public static string Serialize(object value, int indent)
        {
            var sb = new StringBuilder();
            Write(sb, value, indent);
            return sb.ToString();
        }

        private static void Write(StringBuilder sb, object value, int indent)
        {
            if (value == null)
            {
                sb.Append("null");
            }
            else if (value is string)
            {
                WriteString(sb, (string)value);
            }
            else if (value is bool)
            {
                sb.Append((bool)value ? "true" : "false");
            }
            else if (value is int)
            {
                sb.Append(((int)value).ToString(CultureInfo.InvariantCulture));
            }
            else if (value is long)
            {
                sb.Append(((long)value).ToString(CultureInfo.InvariantCulture));
            }
            else if (value is double)
            {
                sb.Append(((double)value).ToString("R", CultureInfo.InvariantCulture));
            }
            else if (value is OrderedDict)
            {
                WriteObject(sb, (OrderedDict)value, indent);
            }
            else if (value is IEnumerable<object>)
            {
                WriteArray(sb, (IEnumerable<object>)value, indent);
            }
            else
            {
                WriteString(sb, value.ToString());
            }
        }

        private static void WriteObject(StringBuilder sb, OrderedDict dict, int indent)
        {
            if (dict.Count == 0)
            {
                sb.Append("{}");
                return;
            }

            sb.Append('{');
            bool first = true;
            foreach (KeyValuePair<string, object> item in dict)
            {
                if (!first)
                {
                    sb.Append(',');
                }
                first = false;
                NewLine(sb, indent + 1);
                WriteString(sb, item.Key);
                sb.Append(": ");
                Write(sb, item.Value, indent + 1);
            }
            NewLine(sb, indent);
            sb.Append('}');
        }

        private static void WriteArray(StringBuilder sb, IEnumerable<object> list, int indent)
        {
            var items = list.ToList();
            if (items.Count == 0)
            {
                sb.Append("[]");
                return;
            }

            sb.Append('[');
            bool first = true;
            foreach (object item in items)
            {
                if (!first)
                {
                    sb.Append(',');
                }
                first = false;
                NewLine(sb, indent + 1);
                Write(sb, item, indent + 1);
            }
            NewLine(sb, indent);
            sb.Append(']');
        }

        private static void NewLine(StringBuilder sb, int indent)
        {
            sb.Append('\n');
            sb.Append(' ', indent * 2);
        }

        private static void WriteString(StringBuilder sb, string text)
        {
            sb.Append('"');
            foreach (char c in text)
            {
                switch (c)
                {
                    case '"': sb.Append("\\\""); break;
                    case '\\': sb.Append("\\\\"); break;
                    case '\n': sb.Append("\\n"); break;
                    case '\r': sb.Append("\\r"); break;
                    case '\t': sb.Append("\\t"); break;
                    default:
                        if (c < 0x20)
                        {
                            sb.Append("\\u").Append(((int)c).ToString("x4", CultureInfo.InvariantCulture));
                        }
                        else
                        {
                            sb.Append(c);
                        }
                        break;
                }
            }
            sb.Append('"');
        }

    }
}
