#pragma once

#include <variant>

struct LuaToken
{
	enum Type
	{
		TYPE_NAME,

		TYPE_AND,
		TYPE_BREAK,
		TYPE_DO,
		TYPE_ELSE,
		TYPE_ELSEIF,
		TYPE_END,
		TYPE_FALSE,
		TYPE_FOR,
		TYPE_FUNCTION,
		TYPE_IF,
		TYPE_IN,
		TYPE_LOCAL,
		TYPE_NIL,
		TYPE_NOT,
		TYPE_OR,
		TYPE_REPEAT,
		TYPE_RETURN,
		TYPE_THEN,
		TYPE_TRUE,
		TYPE_UNTIL,
		TYPE_WHILE,

		TYPE_PLUS,
		TYPE_MINUS,
		TYPE_MUL,
		TYPE_DIV,
		TYPE_MOD,
		TYPE_EXP,
		TYPE_HASH,
		TYPE_EQ,
		TYPE_NEQ,
		TYPE_LE,
		TYPE_GE,
		TYPE_LT,
		TYPE_GT,
		TYPE_ASSIGN,
		TYPE_LEFT_B,
		TYPE_RIGHT_B,
		TYPE_LEFT_CB,
		TYPE_RIGHT_CB,
		TYPE_LEFT_SB,
		TYPE_RIGHT_SB,
		TYPE_SEMI_COLON,
		TYPE_COLON,
		TYPE_COMMA,
		TYPE_DOT,
		TYPE_CONCAT,
		TYPE_VARARG,

		TYPE_STRING,
		TYPE_NUMBER,

		TYPE_EOS,

		TYPE_ERROR_STRAY_TILDE, // TODO Should probably handle this in another way. Original lexer doesn't do this
		TYPE_ERROR_UNFINISHED_LONG_COMMENT,
		TYPE_ERROR_UNFINISHED_LONG_STRING,
		TYPE_ERROR_INVALID_LONG_STRING_DELIMITER,
		TYPE_ERROR_STRING,
		TYPE_ERROR_BAD_CHARACTER,
		TYPE_ERROR_MALFORMED_NUMBER,
	};

	LuaToken(Type type)
		: _type(type)
	{}

	LuaToken(Type type, std::string str)
		: _type(type), _data(std::move(str))
	{}

	Type _type;

	std::variant<std::monostate, std::string> _data;

	std::string ToString() const
	{
		switch (_type)
		{
		case TYPE_NAME:
			return "";
		case TYPE_AND:
			return "'and'";
		case TYPE_BREAK:
			return "'break'";
		case TYPE_DO:
			return "'do'";
		case TYPE_ELSE:
			return "'else'";
		case TYPE_ELSEIF:
			return "'elseif'";
		case TYPE_END:
			return "'end'";
		case TYPE_FALSE:
			return "'false'";
		case TYPE_FOR:
			return "'for'";
		case TYPE_FUNCTION:
			return "'function'";
		case TYPE_IF:
			return "'if'";
		case TYPE_IN:
			return "'in'";
		case TYPE_LOCAL:
			return "'local'";
		case TYPE_NIL:
			return "'nil'";
		case TYPE_NOT:
			return "'not'";
		case TYPE_OR:
			return "'or'";
		case TYPE_REPEAT:
			return "'repeat'";
		case TYPE_RETURN:
			return "'return'";
		case TYPE_THEN:
			return "'then'";
		case TYPE_TRUE:
			return "'true'";
		case TYPE_UNTIL:
			return "'until'";
		case TYPE_WHILE:
			return "'while'";
		case TYPE_PLUS:
			return "'+'";
		case TYPE_MINUS:
			return "'-'";
		case TYPE_MUL:
			return "'*'";
		case TYPE_DIV:
			return "'/'";
		case TYPE_MOD:
			return "'%'";
		case TYPE_EXP:
			return "'^'";
		case TYPE_HASH:
			return "'#'";
		case TYPE_EQ:
			return "'=='";
		case TYPE_NEQ:
			return "'~='";
		case TYPE_LE:
			return "'<='";
		case TYPE_GE:
			return "'>='";
		case TYPE_LT:
			return "'<'";
		case TYPE_GT:
			return "'>'";
		case TYPE_ASSIGN:
			return "'='";
		case TYPE_LEFT_B:
			return "'('";
		case TYPE_RIGHT_B:
			return "')'";
		case TYPE_LEFT_CB:
			return "'{'";
		case TYPE_RIGHT_CB:
			return "'}'";
		case TYPE_LEFT_SB:
			return "'['";
		case TYPE_RIGHT_SB:
			return "']'";
		case TYPE_SEMI_COLON:
			return "';'";
		case TYPE_COLON:
			return "':'";
		case TYPE_COMMA:
			return "','";
		case TYPE_DOT:
			return "'.'";
		case TYPE_CONCAT:
			return "'..'";
		case TYPE_VARARG:
			return "'...'";
		case TYPE_EOS:
			return "'eos'";
			// TODO What to return on these?
		case TYPE_STRING:
		case TYPE_NUMBER:
		case TYPE_ERROR_STRAY_TILDE:
		case TYPE_ERROR_UNFINISHED_LONG_COMMENT:
		case TYPE_ERROR_UNFINISHED_LONG_STRING:
		case TYPE_ERROR_INVALID_LONG_STRING_DELIMITER:
		case TYPE_ERROR_STRING:
		case TYPE_ERROR_BAD_CHARACTER:
		case TYPE_ERROR_MALFORMED_NUMBER:
		default:
			return "";
		}
	}
};
