using System.CodeDom;
using System.Collections.Generic;
using System.Linq;

namespace RTGen.Cpp.JavaDoc
{
    enum ElementType
    {
        Unknown,
        Text,
        Tag
    }

    class DocComment
    {
        public DocComment()
        {
            Tags = new List<DocTag>();
        }

        public DocBrief Brief { get; set; }

        public IList<DocTag> Tags { get; set; }

        public DocDescription Description { get; set; }
    }

    class DocLine
    {
        public DocLine()
        {
            Elements = new List<DocElement>();
        }

        public string FullText => string.Join(" ", Elements.Select(el => el.RawText));

        public IList<DocElement> Elements { get; set; }

        public override string ToString()
        {
            return FullText;
        }
    }

    class DocElement
    {
        public DocElement(ElementType type, string rawText)
        {
            Type = type;
            RawText = rawText;
        }

        public string RawText { get; set; }

        public ElementType Type { get; set; }

        public override string ToString()
        {
            return RawText;
        }
    }

    class DocText : DocElement
    {
        public DocText(string rawText) : base(ElementType.Text, rawText)
        {
        }
    }

    /*
     * SPECIALIZED TAGS
     */

    enum TagType
    {
        Unknown,
        Brief,
        Throws,
        Param,
        ParamRef,
        RetVal,
        Private,
        Description
    }

    class DocTag : DocElement
    {
        public DocTag(string tagName, TagType type, string rawText) : base(ElementType.Tag, rawText)
        {
            Tag = tagName;
            TagType = type;
        }

        public TagType TagType { get; set; }

        public string Tag { get; set; }
    }

    class DocAttribute : DocTag
    {
        public DocAttribute(string tagName, TagType type, string rawText) : base(tagName, type, rawText)
        {
            Lines = new List<DocLine>();
        }

        public IList<DocLine> Lines { get; set; }
    }

    /*
     * Tags
     */

    class DocDescription : DocAttribute
    {
        public DocDescription(string rawText) : base("", TagType.Description, rawText)
        {
        }
    }

    class DocBrief : DocAttribute
    {
        public DocBrief(string rawText) : base("brief", TagType.Brief, rawText)
        {
        }
    }

    class DocThrows : DocAttribute
    {
        public DocThrows(string exceptionName, string rawText) : base("throws", TagType.Throws, rawText)
        {
            ExceptionName = exceptionName;
        }

        public string ExceptionName { get; set; }
    }

    class DocParam : DocAttribute
    {
        public DocParam(string paramName, string rawText) : base("param", TagType.Param, rawText)
        {
            ParamName = paramName;
        }

        public string ParamName { get; set; }
    }

    class DocParamRef : DocTag
    {
        public DocParamRef(string paramName, string rawText) : base("p", TagType.ParamRef, rawText)
        {
            ParamName = paramName;
        }

        public string ParamName { get; set; }
    }

    class DocRetVal : DocAttribute
    {
        public DocRetVal(string returnValue, string rawText) : base("retval", TagType.RetVal, rawText)
        {
            ReturnValue = returnValue;
        }

        public string ReturnValue { get; set; }
    }


    class DocPrivate : DocTag
    {
        public DocPrivate() : base("private", TagType.Private, string.Empty)
        {
        }
    }

    class UnknownTag : DocAttribute
    {
        public UnknownTag(string tagName, string rawText) : base(tagName, TagType.Unknown, rawText)
        {
        }
    }
}
