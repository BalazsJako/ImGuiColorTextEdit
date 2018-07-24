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

		TYPE_COMMENT,
		TYPE_BEGIN_LONG_COMMENT,
		TYPE_BEGIN_LONG_STRING,
		TYPE_END_LONG_BRACKET,
		TYPE_STRING,

		TYPE_ERROR_STRAY_TILDE,
		TYPE_ERROR_STRING,
		TYPE_ERROR_BRACKET,
		TYPE_ERROR_BAD_CHARACTER,
	};

	struct StartEnd
	{
		StartEnd(size_t start, size_t end)
			: _start(start), _end(end)
		{}

		size_t _start;
		size_t _end;
	};

	struct Level
	{
		Level(size_t level)
			: _level(level)
		{}

		Level& operator++()
		{
			++_level;
			return *this;
		}

		bool operator==(const Level& other) const
		{
			return _level == other._level;
		}

		bool operator!=(const Level& other) const
		{
			return _level != other._level;
		}

		bool operator>(const Level& other) const
		{
			return _level > other._level;
		}

		size_t _level;
	};

	struct Bracket
	{
		Bracket(Level level, size_t pos)
			: _level(level), _pos(pos)
		{}

		Level _level;
		size_t _pos;
	};

	LuaToken(Type type)
		: _type(type)
	{}

	LuaToken(Type type, size_t start, size_t end)
		: _type(type), _data(StartEnd(start, end))
	{}

	LuaToken(Type type, Bracket bracket)
		: _type(type), _data(bracket)
	{}

	Type _type;

	std::variant<std::monostate, StartEnd, Bracket> _data;
};