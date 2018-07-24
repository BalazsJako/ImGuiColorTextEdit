#pragma once

#include "TextEditor.h"

class LuaLexer
{
	TextEditor::Lines& _lines;
	size_t _line;
	size_t _col;

	size_t _identifierStart;

public:
	LuaLexer(TextEditor::Lines& lines)
		: _lines(lines), _line(0), _col(0), _identifierStart(0)
	{}

	void LexAll();

private:

	char GetNext();
	char PeekNext() const;

	void ColorCurrent(TextEditor::PaletteIndex color) const;
	void ColorRange(size_t begin, size_t end, TextEditor::PaletteIndex color) const;

	void AddToken(LuaToken&& token) const;

	template<char Delimiter>
	bool ConsumeString(char c)
	{
		const size_t stringStart = _col + 1;
		bool ignoreNext = false;

		bool searchString = true;
		while (searchString)
		{
			c = GetNext();
			ColorCurrent(TextEditor::PaletteIndex::String);

			switch (c)
			{
			case '\n':
			case '\0':
				AddToken(LuaToken::TYPE_ERROR_STRING);
				return false;
			case '\\':
				ignoreNext = !ignoreNext;
				break;
			case Delimiter:
				if (ignoreNext)
				{
					ignoreNext = false;
				}
				else
				{
					searchString = false;
				}
				break;
			default:
				if (ignoreNext)
				{
					ignoreNext = false;
					// TODO Verify escape sequence
				}
				else
				{
					// TODO Something? Probably not
				}
				break;
			}

			if (c == '\n' || c == '\0')
			{
				searchString = false;
			}
		}

		const size_t stringEnd = _col - 1;

		AddToken({ LuaToken::TYPE_STRING, stringStart, stringEnd });

		return true;
	}

	// Returns true and level of bracket if a long bracket is detected, false and whatever otherwise
	std::tuple<bool, LuaToken::Level> ConsumeBeginLongComment(size_t pos);
	
	// Returns true and level of bracket if a long bracket is detected, false and whatever otherwise
	std::tuple<bool, LuaToken::Level> ConsumeBeginLongString(size_t pos);
	
	bool ConsumeLongBracket(LuaToken::Level level, TextEditor::PaletteIndex color);

	void ConsumeIdentifier(char c);
	void ConsumeAnd();
	void ConsumeDo();
	void ConsumeBreak();
	void ConsumeFalseForFunction();
	void ConsumeIfIn();
	void ConsumeLocal();
	void ConsumeEndElseElseif();
	void ConsumeElseElseif();
	void ConsumeNilNot();
	void ConsumeOr();
	void ConsumeRepeatReturn();
	void ConsumeThenTrue();
	void ConsumeUntil();
	void ConsumeWhile();
	void ConsumeName(char c);

	// c is a number from GetNext
	void ConsumeNumber(char c);
	// c is the first character peeked after dot
	void ConsumeDotNumber(char c);
	// c is first character peeked after x
	void ConsumeHexNumber(char c);

	static bool IsBeginningOfIdentifier(char c)
	{
		return c >= 'a' && c <= 'z' ||
			c >= 'A' && c <= 'Z' ||
			c == '_';
	}

	static bool IsPartOfIdentifier(char c)
	{
		return IsBeginningOfIdentifier(c) || c >= '0' && c <= '9';
	}

	static bool IsWhitespace(char c)
	{
		return c == ' ' || c == '\t' || c == '\n';
	}
};
