parser grammar JavaDoc;

/*
If <sharp> braces are used the argument is a single word.
If (round) braces are used the argument extends until the end of the line on which the command was found.
If {curly} braces are used the argument extends until the next paragraph. Paragraphs are delimited by a blank line or by a section indicator. Note that {curly} braces are also used for command options, here the braces are mandatory and just 'normal' characters. The starting curly brace has to directly follow the command, so without whitespace.
*/

options {
    tokenVocab = JavaDocLexer;
}

start: DocStart NL?
       descrptionWihoutTag? attributes
       (Star NL)* DocEnd
       EOF
     ;

descrptionWihoutTag: Star? docParagraph NL?
                   ;

docBlock:
        ( DocStartSection
        | DocEndSection
        )
        ;

attributes: attribute*
          ;

attribute: Star NL Star docParagraph NL       #DocDescription
         | Star (NL Star)? attributeInline NL #DocAttribute
         | Star? docBlock NL?                 #DocBlockRaw
         ;

attributeInline: DocBrief docParagraph                        # DocBrief
               | DocThrows exceptionName=DocWord docParagraph # DocThrows
               | DocParam paramName=DocWord docParagraph      # DocParam
               | DocParamRef paramRef=DocWord                 # DocParamRef
               | DocRetVal errCode=DocWord docParagraph       # DocRetVal
               | DocPrivate                                   # DocPrivate
               | DocCodeRef codeWord=DocWord                  # DocCodeRef
               | DocAttribute docParagraph                    # DocGeneric
               | DocStartCode                                 # DocCode
               ;

docLine: DocWord (DocWord | Star | attributeInline)*
       ;

docParagraph: docLine (NL Star docParagraph)?
            ;