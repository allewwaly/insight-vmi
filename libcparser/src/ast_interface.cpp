/*
 * ast_interface.cpp
 *
 *  Created on: 14.03.2011
 *      Author: chrschn
 */

#include <ast_interface.h>

#include <QList>
#include <QStack>
#include <QHash>
#include <QString>
#include <string.h>
#include <astsymbol.h>
#include <astscopemanager.h>
#include <astbuilder.h>
#include <abstractsyntaxtree.h>
#include <debug.h>
#include <bugreport.h>


pASTNode new_ASTNode(pASTBuilder builder)
{
    return builder->newASTNode();
}


pASTNode new_ASTNode1(pASTBuilder builder, pASTNode parent,
        pANTLR3_COMMON_TOKEN start, struct ASTScope* scope)
{
    return builder->newASTNode(parent, start, scope);
}


pASTNode new_ASTNode2(pASTBuilder builder, pASTNode parent,
        pANTLR3_COMMON_TOKEN start, enum ASTNodeType type, struct ASTScope* scope)
{
    return builder->newASTNode(parent, start, type, scope);
}


pASTNode pushdown_binary_expression(pASTBuilder builder,
        pANTLR3_COMMON_TOKEN op, pASTNode old_root, pASTNode right)
{
    return builder->pushdownBinaryExpression(op, old_root, right);
}


pASTNodeList new_ASTNodeList(pASTBuilder builder, pASTNode item,
        pASTNodeList tail)
{
    return builder->newASTNodeList(item, tail);
}


pASTTokenList new_ASTTokenList(pASTBuilder builder, pANTLR3_COMMON_TOKEN item, pASTTokenList tail)
{
    return builder->newASTTokenList(item, tail);
}


void push_parent_node(pASTBuilder builder, pASTNode node)
{
    builder->pushParentNode(node);
}


void pop_parent_node(pASTBuilder builder)
{
    builder->popParentNode();
}


pASTNode parent_node(pASTBuilder builder)
{
    return builder->parentNode();
}


void scopePush(pASTBuilder builder, pASTNode node)
{
	builder->pushScope(node);
}


void scopePop(pASTBuilder builder)
{
    builder->popScope();
}


void scopeAddSymbol(pASTBuilder builder, pANTLR3_STRING name, enum ASTSymbolType type,
		pASTNode node)
{
	scopeAddSymbol2(builder, name, type, node, node->scope);
}


void scopeAddSymbol2(pASTBuilder builder, pANTLR3_STRING name,
		enum ASTSymbolType type, pASTNode node, pASTScope scope)
{
//	QString s = builder->ast()->antlrStringToStr(name);

//	int line = node ?  node->start->line : 0;
//
//	switch (type) {
//	case stTypedef:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a TYPEDEF");
//		break;
//	case stVariableDecl:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a VARIABLE NAME (declaration)");
//		break;
//	case stVariableDef:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a VARIABLE NAME (definition)");
//		break;
//	case stStructOrUnionDecl:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a STRUCT NAME (declaration)");
//		break;
//	case stStructOrUnionDef:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a STRUCT NAME (definition)");
//		break;
//	case stStructMember:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a STRUCT MEMBER");
//		break;
//	case stFunctionDecl:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a FUNCTION NAME (declaration)");
//		break;
//	case stFunctionDef:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a FUNCTION NAME (definition)");
//		break;
//	case stFunctionParam:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a PARAMETER");
//		break;
//	case stEnumDecl:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a ENUM NAME (declaration)");
//		break;
//	case stEnumDef:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a ENUM NAME (definition)");
//		break;
//	case stEnumerator:
//		debugmsg("Line " << line << ": Symbol \"" << qPrintable(s) << "\" is a ENUMERATOR");
//		break;
//	case stNull:
//		break;
//	}

	builder->addSymbol(name, type, node, scope);
}


pASTScope scopeCurrent(pASTBuilder builder)
{
	return builder->currentScope();
}


ANTLR3_BOOLEAN isTypeName(pASTBuilder builder, pANTLR3_STRING name)
{
	if (builder->isTypeName(name)) {
//		debugmsg("\"" << name->chars << "\" is a defined type");
		return ANTLR3_TRUE;
	}
//	debugmsg("\"" << name->chars << "\" is NOT a type");
	return ANTLR3_FALSE;
}


ANTLR3_BOOLEAN isSymbolName(pASTBuilder builder, pANTLR3_STRING name)
{
	if (builder->isSymbolName(name)) {
//	    debugmsg("\"" << name->chars << "\" is a symbol");
		return ANTLR3_TRUE;
	}
//    debugmsg("\"" << name->chars << "\" is NOT a symbol");
	return ANTLR3_FALSE;
}


ANTLR3_BOOLEAN isInitializer(pASTNode node)
{
    while (node) {
        if (node->type == nt_initializer)
            return ANTLR3_TRUE;
        node = node->parent;
    }
    return ANTLR3_FALSE;
}

// Adapted from displayRecognitionError() in antlr3baserecognizer.c
void displayParserRecognitionError(
        pANTLR3_BASE_RECOGNIZER recognizer, pANTLR3_UINT8 * tokenNames)
{
    // Make sure we deal with the right type of recognizer
    assert(recognizer != 0 && recognizer->type == ANTLR3_TYPE_PARSER);

    // Retrieve some info for easy reading.
    pANTLR3_EXCEPTION ex = recognizer->exception;

    // Indicate this recognizer had an error while processing.
    recognizer->errorCount++;

    QString msg("Parser error in file %1, line %2, offset %3, %4:\n"
                "    %5\n"
                "    => %6");

    // See if there is a 'filename' we can use
    if	(ex->streamName == NULL) {
        msg = msg.arg("(unknown source)");
    }
    else {
        pANTLR3_STRING ftext = ex->streamName->to8(ex->streamName);
        msg = msg.arg((char*)ftext->chars);
    }

    // Next comes the line number and offset
    msg = msg.arg(ex->line).arg(ex->charPositionInLine + 1);

    pANTLR3_COMMON_TOKEN errToken = (pANTLR3_COMMON_TOKEN)(ex->token);

	if  (errToken) {
		if (errToken->type == ANTLR3_TOKEN_EOF)
			msg = msg.arg("at <EOF>");
		else {
			pANTLR3_STRING errTokText = errToken->getText(errToken);
			msg = msg.arg(QString("near '%1'").arg((char*)errTokText->chars));
		}
	}
	else
		msg = msg.arg("at unknown token");

    // Error message
    msg = msg.arg((char*)ex->message);

	switch  (ex->type)
	{
	case ANTLR3_RECOGNITION_EXCEPTION:

		// Indicates that the recognizer received a token
		// in the input that was not predicted. This is the basic exception type
		// from which all others are derived. So we assume it was a syntax error.
		// You may get this if there are not more tokens and more are needed
		// to complete a parse for instance.
		//
		msg = msg.arg("Syntax error.");
		break;

	case    ANTLR3_MISMATCHED_TOKEN_EXCEPTION:

		// We were expecting to see one thing and got another. This is
		// most common error, and here you can spend your efforts to
		// derive more useful error messages based on the expected
		// token set and the last token and so on. The error following
		// bitmaps do a good job of reducing the set that we were looking
		// for down to something small. Knowing what you are parsing may be
		// able to allow you to be even more specific about an error.
		//
		if	(tokenNames == NULL)
			msg = msg.arg("Syntax error.");
		else
			msg = msg.arg(QString("Expected %1.").arg((char*)tokenNames[ex->expecting]));
		break;

	case	ANTLR3_NO_VIABLE_ALT_EXCEPTION:

		// We could not pick any alt decision from the input given
		// so god knows what happened - however when you examine your grammar,
		// you should. It means that at the point where the current token occurred
		// that the DFA indicates nowhere to go from here.
		//
		msg = msg.arg("Cannot match to any predicted input.");

		break;

	case	ANTLR3_MISMATCHED_SET_EXCEPTION:
	{
		// This means we were able to deal with one of a set of
		// possible tokens at this point, but we did not see any
		// member of that set.

		// What tokens could we have accepted at this point in the
		// parse?
		ANTLR3_UINT32 count   = 0;
		ANTLR3_UINT32 numbits = ex->expectingSet->numBits(ex->expectingSet);
		ANTLR3_UINT32 size    = ex->expectingSet->size(ex->expectingSet);

		if (size > 0) {
			QString s("Unexpected input, expected one of: ");

			// However many tokens we could have dealt with here, it is usually
			// not useful to print ALL of the set here. I arbitrarily chose 8
			// here, but you should do whatever makes sense for you of course.
			// No token number 0, so look for bit 1 and on.
			for	(ANTLR3_UINT32 bit = 1; bit < numbits && count < 8 && count < size; bit++)
			{
				if  (tokenNames[bit]) {
					if (count > 0)
						s += ", ";
					s += ((char*)tokenNames[bit]);
					count++;
				}
			}
			msg = msg.arg(s);
		}
		else {
			msg = msg.arg("Actually dude, we didn't seem to be expecting anything "
					"here, or at least I could not work out what I was "
					"expecting, like so many of us these days!");
		}
	}
		break;

	case	ANTLR3_EARLY_EXIT_EXCEPTION:

		// We entered a loop requiring a number of token sequences
		// but found a token that ended that sequence earlier than
		// we should have done.
		//
		msg = msg.arg("Missing elements.");
		break;

	default:

		// We don't handle any other exceptions here, but you can
		// if you wish. If we get an exception that hits this point
		// then we are just going to report what we know about the
		// token.
		//
		msg = msg.arg("Syntax not recognized.");
		break;
	}

	BugReport::reportErr(msg);
}
