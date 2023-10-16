using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using Antlr4.Runtime;
using Antlr4.Runtime.Misc;
using Antlr4.Runtime.Tree;
using RTGen.Interfaces.Doc;
using RTGen.Types.Doc;

namespace RTGen.Cpp.Parser
{
    class JavaDocParser : JavaDocBaseListener
    {
        private int _tagCounter;

        public JavaDocParser()
        {
            _tagCounter = 0;
            Documentation = new DocComment();
        }

        public IDocComment Documentation { get; }

        private DocTag ParseInlineAttribute(JavaDoc.AttributeInlineContext context)
        {
            string rawText = GetRawText(context);
            switch (context.Start.Type)
            {
                case JavaDoc.DocBrief:
                    return ParseDocBrief((JavaDoc.DocBriefContext) context, rawText);
                case JavaDoc.DocThrows:
                    return ParseDocThrows((JavaDoc.DocThrowsContext) context, rawText);
                case JavaDoc.DocParam:
                    return ParseDocParam((JavaDoc.DocParamContext) context, rawText);
                case JavaDoc.DocParamRef:
                    return ParseDocParamRef((JavaDoc.DocParamRefContext) context, rawText);
                case JavaDoc.DocPrivate:
                    return new DocPrivate();
                case JavaDoc.DocRetVal:
                    return ParseRetVal((JavaDoc.DocRetValContext)context, rawText);
                case JavaDoc.DocStartCode:
                    return new UnknownTag("code", rawText);
                case JavaDoc.DocCodeRef:
                    return new UnknownTag("c", rawText);
                case JavaDoc.DocAttribute:
                    return ParseUnknownAttribute((JavaDoc.DocGenericContext) context, rawText);
            }

            return null;
        }

        private DocRetVal ParseRetVal(JavaDoc.DocRetValContext context, string rawText)
        {
            var tag = new DocRetVal(context.errCode.Text, rawText);
            ParseParagraph(tag, context.docParagraph());

            return tag;
        }

        private DocBrief ParseDocBrief(JavaDoc.DocBriefContext context, string rawText)
        {
            var tag = new DocBrief(rawText, _tagCounter);
            ParseParagraph(tag, context.docParagraph());

            return tag;
        }

        private DocTag ParseUnknownAttribute(JavaDoc.DocGenericContext context, string rawText)
        {
            var tag = new UnknownTag(context.DocAttribute()
                                            .GetText()
                                            .TrimStart('@'),
                                     rawText);
            ParseParagraph(tag, context.docParagraph());

            return tag;
        }

        private DocTag ParseDocParamRef(JavaDoc.DocParamRefContext context, string rawText)
        {
            return new DocParamRef(context.paramRef.Text, rawText);
        }

        private DocTag ParseDocParam(JavaDoc.DocParamContext context, string rawText)
        {
            var tag = new DocParam(context.paramName.Text,
                                   context.GetChild(0).GetText() == "@param[out]",
                                   rawText);
            ParseParagraph(tag, context.docParagraph());

            return tag;
        }

        private DocTag ParseDocThrows(JavaDoc.DocThrowsContext context, string rawText)
        {
            var tag = new DocThrows(context.exceptionName.Text, rawText);
            ParseParagraph(tag, context.docParagraph());

            return tag;
        }

        public override void ExitDescrptionWihoutTag(JavaDoc.DescrptionWihoutTagContext context)
        {
            ParseParagraph(ParseDescription(context), context.docParagraph());
        }

        public override void ExitDocDescription(JavaDoc.DocDescriptionContext context)
        {
            ParseParagraph(ParseDescription(context), context.docParagraph());
        }

        private DocDescription ParseDescription(ParserRuleContext context)
        {
            DocDescription description;
            if (Documentation.Description == null ||
                Documentation.Description != null && Documentation.Description.TagIndex != _tagCounter)
            {
                description = new DocDescription(GetRawText(context), _tagCounter++);
                if (description.RawText
                               .TrimStart('*')
                               .StartsWith(Environment.NewLine, StringComparison.Ordinal))
                {
                    description.Lines.Add(new DocLine
                    {
                        Elements =
                        {
                            new DocText("")
                        }
                    });
                }

                if (Documentation.Description == null)
                {
                    Documentation.Description = description;
                }
                else
                {
                    Documentation.Tags.Add(description);
                }
            }
            else
            {
                description = (DocDescription)Documentation.Description;
                description.RawText += " " + GetRawText(context);
                description.Lines.Add(new DocLine
                {
                    Elements =
                    {
                        new DocText("")
                    }
                });
            }

            return description;
        }

        private string GetRawText(ParserRuleContext context)
        {
            return context.Start
                          .InputStream
                          .GetText(new Interval(context.Start.StartIndex, context.Stop.StopIndex))
                          ;
        }

        public override void ExitDocBrief(JavaDoc.DocBriefContext context)
        {
            var fullContext = (context.Parent is JavaDoc.DocAttributeContext)
                                  ? context.Parent
                                  : context;

            DocBrief brief = new DocBrief(GetRawText((ParserRuleContext)fullContext), _tagCounter++);
            ParseParagraph(brief, context.docParagraph());

            Documentation.Brief = brief;
        }

        public override void ExitDocBlockRaw(JavaDoc.DocBlockRawContext context)
        {
            string rawText = GetRawText(context);
            Documentation.Tags.Add(new DocTag(context.docBlock().GetText(), TagType.Block, rawText));
            _tagCounter++;
        }

        public override void ExitDocAttribute(JavaDoc.DocAttributeContext context)
        {
            JavaDoc.AttributeInlineContext inline = context.attributeInline();
            if (inline.Start.Type == JavaDoc.DocBrief)
            {
                return;
            }

            DocTag attribute = ParseInlineAttribute(inline);
            if (attribute != null)
            {
                Documentation.Tags.Add(attribute);
                _tagCounter++;
            }
        }

        private DocLine ParseLine(JavaDoc.DocLineContext line)
        {
            var docLine = new DocLine();
            if (line.attributeInline().Length == 0)
            {
                string rawText = GetRawText(line);
                docLine.Elements.Add(new DocText(rawText));
            }
            else
            {
                bool setStart = false;
                int start = line.Start.StartIndex;
                int end = line.Start.StartIndex;

                foreach (IParseTree tree in line.children)
                {
                    switch (tree)
                    {
                        case JavaDoc.AttributeInlineContext inlineContext:
                        {
                            if (start != end)
                            {
                                string text = line.Start.InputStream.GetText(new Interval(start, end));
                                docLine.Elements.Add(new DocText(text));
                            }

                            docLine.Elements.Add(ParseInlineAttribute(inlineContext));

                            start = inlineContext.Stop.StopIndex + 1;
                            setStart = true;
                            break;
                        }
                        case ITerminalNode node:
                        {
                            if (setStart)
                            {
                                start = node.Symbol.StartIndex;
                                setStart = false;
                            }

                            end = node.Symbol.StopIndex;
                            break;
                        }
                    }
                }

                if (end == line.Stop.StopIndex && start < end)
                {
                    string text = line.Start.InputStream.GetText(new Interval(start, end));
                    docLine.Elements.Add(new DocText(text));
                }
            }

            return docLine;
        }

        private void ParseParagraph(DocAttribute model, JavaDoc.DocParagraphContext paragraph)
        {
            while (paragraph != null)
            {
                model.Lines.Add(ParseLine(paragraph.docLine()));
                paragraph = paragraph.docParagraph();
            }
        }
    }
}
