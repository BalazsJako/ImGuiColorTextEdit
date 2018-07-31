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
	void ConsumeString(char c)
	{
		const size_t stringStart = _col - 1;
		bool ignoreNext = false;

		bool searchString = true;
		while (searchString)
		{
			c = PeekNext();
			ColorCurrent(TextEditor::PaletteIndex::String);

			switch (c)
			{
			case '\n':
			case '\0':
				UnfinishedString(stringStart);
				return;
			case '\\':
				GetNext();
				ignoreNext = !ignoreNext;
				break;
			case Delimiter:
				GetNext();
				ColorCurrent(TextEditor::PaletteIndex::String);
				if (ignoreNext)
					ignoreNext = false;
				else
					searchString = false;
				break;
			default:
				GetNext();
				if (ignoreNext)
				{
					ignoreNext = false;
					// TODO Verify escape sequence
				}
				break;
			}
		}

		AddToken({ LuaToken::TYPE_STRING });
	}

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

	void MalformedNumber(size_t start, size_t end) const;

	bool ConsumeLongComment();
	bool ConsumeLongString();

	void UnfinishedString(size_t start) const;

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
