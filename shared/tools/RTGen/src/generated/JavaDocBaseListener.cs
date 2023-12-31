//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     ANTLR Version: 4.11.1
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

// Generated from F:/Dewesoft/C++/git/Blueberry3/shared/tools/RTGen/src\JavaDoc.g4 by ANTLR 4.11.1

// Unreachable code detected
#pragma warning disable 0162
// The variable '...' is assigned but its value is never used
#pragma warning disable 0219
// Missing XML comment for publicly visible type or member '...'
#pragma warning disable 1591
// Ambiguous reference in cref attribute
#pragma warning disable 419


using Antlr4.Runtime.Misc;
using IErrorNode = Antlr4.Runtime.Tree.IErrorNode;
using ITerminalNode = Antlr4.Runtime.Tree.ITerminalNode;
using IToken = Antlr4.Runtime.IToken;
using ParserRuleContext = Antlr4.Runtime.ParserRuleContext;

/// <summary>
/// This class provides an empty implementation of <see cref="IJavaDocListener"/>,
/// which can be extended to create a listener which only needs to handle a subset
/// of the available methods.
/// </summary>
[System.CodeDom.Compiler.GeneratedCode("ANTLR", "4.11.1")]
[System.Diagnostics.DebuggerNonUserCode]
[System.CLSCompliant(false)]
public partial class JavaDocBaseListener : IJavaDocListener {
	/// <summary>
	/// Enter a parse tree produced by <see cref="JavaDoc.start"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void EnterStart([NotNull] JavaDoc.StartContext context) { }
	/// <summary>
	/// Exit a parse tree produced by <see cref="JavaDoc.start"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void ExitStart([NotNull] JavaDoc.StartContext context) { }
	/// <summary>
	/// Enter a parse tree produced by <see cref="JavaDoc.descrptionWihoutTag"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void EnterDescrptionWihoutTag([NotNull] JavaDoc.DescrptionWihoutTagContext context) { }
	/// <summary>
	/// Exit a parse tree produced by <see cref="JavaDoc.descrptionWihoutTag"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void ExitDescrptionWihoutTag([NotNull] JavaDoc.DescrptionWihoutTagContext context) { }
	/// <summary>
	/// Enter a parse tree produced by <see cref="JavaDoc.docBlock"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void EnterDocBlock([NotNull] JavaDoc.DocBlockContext context) { }
	/// <summary>
	/// Exit a parse tree produced by <see cref="JavaDoc.docBlock"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void ExitDocBlock([NotNull] JavaDoc.DocBlockContext context) { }
	/// <summary>
	/// Enter a parse tree produced by <see cref="JavaDoc.attributes"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void EnterAttributes([NotNull] JavaDoc.AttributesContext context) { }
	/// <summary>
	/// Exit a parse tree produced by <see cref="JavaDoc.attributes"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void ExitAttributes([NotNull] JavaDoc.AttributesContext context) { }
	/// <summary>
	/// Enter a parse tree produced by the <c>DocDescription</c>
	/// labeled alternative in <see cref="JavaDoc.attribute"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void EnterDocDescription([NotNull] JavaDoc.DocDescriptionContext context) { }
	/// <summary>
	/// Exit a parse tree produced by the <c>DocDescription</c>
	/// labeled alternative in <see cref="JavaDoc.attribute"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void ExitDocDescription([NotNull] JavaDoc.DocDescriptionContext context) { }
	/// <summary>
	/// Enter a parse tree produced by the <c>DocAttribute</c>
	/// labeled alternative in <see cref="JavaDoc.attribute"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void EnterDocAttribute([NotNull] JavaDoc.DocAttributeContext context) { }
	/// <summary>
	/// Exit a parse tree produced by the <c>DocAttribute</c>
	/// labeled alternative in <see cref="JavaDoc.attribute"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void ExitDocAttribute([NotNull] JavaDoc.DocAttributeContext context) { }
	/// <summary>
	/// Enter a parse tree produced by the <c>DocBlockRaw</c>
	/// labeled alternative in <see cref="JavaDoc.attribute"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void EnterDocBlockRaw([NotNull] JavaDoc.DocBlockRawContext context) { }
	/// <summary>
	/// Exit a parse tree produced by the <c>DocBlockRaw</c>
	/// labeled alternative in <see cref="JavaDoc.attribute"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void ExitDocBlockRaw([NotNull] JavaDoc.DocBlockRawContext context) { }
	/// <summary>
	/// Enter a parse tree produced by the <c>DocBrief</c>
	/// labeled alternative in <see cref="JavaDoc.attributeInline"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void EnterDocBrief([NotNull] JavaDoc.DocBriefContext context) { }
	/// <summary>
	/// Exit a parse tree produced by the <c>DocBrief</c>
	/// labeled alternative in <see cref="JavaDoc.attributeInline"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void ExitDocBrief([NotNull] JavaDoc.DocBriefContext context) { }
	/// <summary>
	/// Enter a parse tree produced by the <c>DocThrows</c>
	/// labeled alternative in <see cref="JavaDoc.attributeInline"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void EnterDocThrows([NotNull] JavaDoc.DocThrowsContext context) { }
	/// <summary>
	/// Exit a parse tree produced by the <c>DocThrows</c>
	/// labeled alternative in <see cref="JavaDoc.attributeInline"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void ExitDocThrows([NotNull] JavaDoc.DocThrowsContext context) { }
	/// <summary>
	/// Enter a parse tree produced by the <c>DocParam</c>
	/// labeled alternative in <see cref="JavaDoc.attributeInline"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void EnterDocParam([NotNull] JavaDoc.DocParamContext context) { }
	/// <summary>
	/// Exit a parse tree produced by the <c>DocParam</c>
	/// labeled alternative in <see cref="JavaDoc.attributeInline"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void ExitDocParam([NotNull] JavaDoc.DocParamContext context) { }
	/// <summary>
	/// Enter a parse tree produced by the <c>DocParamRef</c>
	/// labeled alternative in <see cref="JavaDoc.attributeInline"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void EnterDocParamRef([NotNull] JavaDoc.DocParamRefContext context) { }
	/// <summary>
	/// Exit a parse tree produced by the <c>DocParamRef</c>
	/// labeled alternative in <see cref="JavaDoc.attributeInline"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void ExitDocParamRef([NotNull] JavaDoc.DocParamRefContext context) { }
	/// <summary>
	/// Enter a parse tree produced by the <c>DocRetVal</c>
	/// labeled alternative in <see cref="JavaDoc.attributeInline"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void EnterDocRetVal([NotNull] JavaDoc.DocRetValContext context) { }
	/// <summary>
	/// Exit a parse tree produced by the <c>DocRetVal</c>
	/// labeled alternative in <see cref="JavaDoc.attributeInline"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void ExitDocRetVal([NotNull] JavaDoc.DocRetValContext context) { }
	/// <summary>
	/// Enter a parse tree produced by the <c>DocPrivate</c>
	/// labeled alternative in <see cref="JavaDoc.attributeInline"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void EnterDocPrivate([NotNull] JavaDoc.DocPrivateContext context) { }
	/// <summary>
	/// Exit a parse tree produced by the <c>DocPrivate</c>
	/// labeled alternative in <see cref="JavaDoc.attributeInline"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void ExitDocPrivate([NotNull] JavaDoc.DocPrivateContext context) { }
	/// <summary>
	/// Enter a parse tree produced by the <c>DocCodeRef</c>
	/// labeled alternative in <see cref="JavaDoc.attributeInline"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void EnterDocCodeRef([NotNull] JavaDoc.DocCodeRefContext context) { }
	/// <summary>
	/// Exit a parse tree produced by the <c>DocCodeRef</c>
	/// labeled alternative in <see cref="JavaDoc.attributeInline"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void ExitDocCodeRef([NotNull] JavaDoc.DocCodeRefContext context) { }
	/// <summary>
	/// Enter a parse tree produced by the <c>DocGeneric</c>
	/// labeled alternative in <see cref="JavaDoc.attributeInline"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void EnterDocGeneric([NotNull] JavaDoc.DocGenericContext context) { }
	/// <summary>
	/// Exit a parse tree produced by the <c>DocGeneric</c>
	/// labeled alternative in <see cref="JavaDoc.attributeInline"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void ExitDocGeneric([NotNull] JavaDoc.DocGenericContext context) { }
	/// <summary>
	/// Enter a parse tree produced by the <c>DocCode</c>
	/// labeled alternative in <see cref="JavaDoc.attributeInline"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void EnterDocCode([NotNull] JavaDoc.DocCodeContext context) { }
	/// <summary>
	/// Exit a parse tree produced by the <c>DocCode</c>
	/// labeled alternative in <see cref="JavaDoc.attributeInline"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void ExitDocCode([NotNull] JavaDoc.DocCodeContext context) { }
	/// <summary>
	/// Enter a parse tree produced by <see cref="JavaDoc.docLine"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void EnterDocLine([NotNull] JavaDoc.DocLineContext context) { }
	/// <summary>
	/// Exit a parse tree produced by <see cref="JavaDoc.docLine"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void ExitDocLine([NotNull] JavaDoc.DocLineContext context) { }
	/// <summary>
	/// Enter a parse tree produced by <see cref="JavaDoc.docParagraph"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void EnterDocParagraph([NotNull] JavaDoc.DocParagraphContext context) { }
	/// <summary>
	/// Exit a parse tree produced by <see cref="JavaDoc.docParagraph"/>.
	/// <para>The default implementation does nothing.</para>
	/// </summary>
	/// <param name="context">The parse tree.</param>
	public virtual void ExitDocParagraph([NotNull] JavaDoc.DocParagraphContext context) { }

	/// <inheritdoc/>
	/// <remarks>The default implementation does nothing.</remarks>
	public virtual void EnterEveryRule([NotNull] ParserRuleContext context) { }
	/// <inheritdoc/>
	/// <remarks>The default implementation does nothing.</remarks>
	public virtual void ExitEveryRule([NotNull] ParserRuleContext context) { }
	/// <inheritdoc/>
	/// <remarks>The default implementation does nothing.</remarks>
	public virtual void VisitTerminal([NotNull] ITerminalNode node) { }
	/// <inheritdoc/>
	/// <remarks>The default implementation does nothing.</remarks>
	public virtual void VisitErrorNode([NotNull] IErrorNode node) { }
}
