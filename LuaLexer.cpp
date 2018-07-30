#include "LuaLexer.h"

#include "LuaToken.h"
#include <iostream>

void LuaLexer::LexAll()
{
	if (_lines.empty())
		return;

	_line = 0;
	_col = 0;
	_lines[0].mTokens.clear();

	char c = GetNext();

	while(c != '\0')
	{
		if (IsWhitespace(c))
		{
			c = GetNext();
			continue;
		}

		switch (c)
		{
		case '+':
			AddToken(LuaToken::TYPE_PLUS);
			ColorCurrent(TextEditor::PaletteIndex::Default);
			break;
		case '-':
			
			if (PeekNext() == '-') // Comment or long comment
			{
				ColorCurrent(TextEditor::PaletteIndex::Comment);
				GetNext();
				ColorCurrent(TextEditor::PaletteIndex::Comment);
				const size_t commentStart = _col - 2;

				auto [longComment, level] = ConsumeBeginLongComment(commentStart);

				if(longComment)
				{
					if(!ConsumeLongBracket(level, TextEditor::PaletteIndex::Comment))
						return;
				}
				else
				{
					// Point at the second '-' or at the point ConsumeBeginLongComment have placed us
					size_t commentEnd = _col - 1;

					// Consume characters (and increment commentEnd) until end of line or end of file
					do
					{
						c = PeekNext();
						if (c == '\n' || c == '\0')
						{
							AddToken({ LuaToken::TYPE_COMMENT, commentStart , commentEnd });
							break;
						}

						++commentEnd;

						GetNext();
						ColorCurrent(TextEditor::PaletteIndex::Comment);
					} while (true);
				}
			}
			else
			{
				ColorCurrent(TextEditor::PaletteIndex::Default);
				AddToken(LuaToken::TYPE_MINUS);
			}
			break;
		case '*':
			ColorCurrent(TextEditor::PaletteIndex::Default);
			AddToken(LuaToken::TYPE_MUL);
			break;
		case '/':
			ColorCurrent(TextEditor::PaletteIndex::Default);
			AddToken(LuaToken::TYPE_DIV);
			break;
		case '%':
			ColorCurrent(TextEditor::PaletteIndex::Default);
			AddToken(LuaToken::TYPE_MOD);
			break;
		case '^':
			ColorCurrent(TextEditor::PaletteIndex::Default);
			AddToken(LuaToken::TYPE_EXP);
			break;
		case '#':
			ColorCurrent(TextEditor::PaletteIndex::Default);
			AddToken(LuaToken::TYPE_HASH);
			break;
		case '=':
			ColorCurrent(TextEditor::PaletteIndex::Default);
			if (PeekNext() == '=')
			{
				GetNext();
				ColorCurrent(TextEditor::PaletteIndex::Default);
				AddToken(LuaToken::TYPE_EQ);
			}
			else
				AddToken(LuaToken::TYPE_ASSIGN);
			break;
		case '~':
			if(PeekNext() == '=')
			{
				ColorCurrent(TextEditor::PaletteIndex::Default);
				GetNext();
				ColorCurrent(TextEditor::PaletteIndex::Default);
				AddToken(LuaToken::TYPE_NEQ);
			}
			else // Adding error, but keep parsing so the rest of the code get syntax highlight
			{
				ColorCurrent(TextEditor::PaletteIndex::ErrorMarker);
				AddToken({ LuaToken::TYPE_ERROR_STRAY_TILDE });
			}
			break;
		case '<':
			ColorCurrent(TextEditor::PaletteIndex::Default);
			if (PeekNext() == '=')
			{
				GetNext();
				ColorCurrent(TextEditor::PaletteIndex::Default);
				AddToken(LuaToken::TYPE_LE);
			}
			else 
				AddToken(LuaToken::TYPE_LT);
			break;
		case '>':
			ColorCurrent(TextEditor::PaletteIndex::Default);
			if (PeekNext() == '=')
			{
				GetNext();
				ColorCurrent(TextEditor::PaletteIndex::Default);
				AddToken(LuaToken::TYPE_GE);
			}
			else
				AddToken(LuaToken::TYPE_GT);
			break;
		case '(':
			ColorCurrent(TextEditor::PaletteIndex::Default);
			AddToken(LuaToken::TYPE_LEFT_B);
			break;
		case ')':
			ColorCurrent(TextEditor::PaletteIndex::Default);
			AddToken(LuaToken::TYPE_RIGHT_B);
			break;
		case '{':
			ColorCurrent(TextEditor::PaletteIndex::Default);
			AddToken(LuaToken::TYPE_LEFT_CB);
			break;
		case '}':
			ColorCurrent(TextEditor::PaletteIndex::Default);
			AddToken(LuaToken::TYPE_RIGHT_CB);
			break;
		case '[':
			{
				const size_t stringStart = _col - 1;

				auto[longString, level] = ConsumeBeginLongString(stringStart);

				if (longString)
				{
					if (!ConsumeLongBracket(level, TextEditor::PaletteIndex::String))
						return;
				}
				else
				{
					// ConsumeBeginLongString might have failed to match. Need move _col back
					_col = stringStart + 1;

					ColorCurrent(TextEditor::PaletteIndex::Default);
					AddToken(LuaToken::TYPE_LEFT_SB);
				}
			}
			break;
		case ']':
			ColorCurrent(TextEditor::PaletteIndex::Default);
			AddToken(LuaToken::TYPE_RIGHT_SB);
			break;
		case ';':
			AddToken(LuaToken::TYPE_SEMI_COLON);
			ColorCurrent(TextEditor::PaletteIndex::Punctuation);
			break;
		case ':':
			AddToken(LuaToken::TYPE_COLON);
			ColorCurrent(TextEditor::PaletteIndex::Punctuation);
			break;
		case ',':
			AddToken(LuaToken::TYPE_COMMA);
			ColorCurrent(TextEditor::PaletteIndex::Punctuation);
			break;
		case '.':
			c = PeekNext();
			if (c == '.')
			{
				ColorCurrent(TextEditor::PaletteIndex::Punctuation);
				GetNext();
				if (PeekNext() == '.')
				{
					ColorCurrent(TextEditor::PaletteIndex::Punctuation);
					GetNext();
					AddToken(LuaToken::TYPE_VARARG);
				}
				else
					AddToken(LuaToken::TYPE_CONCAT);
			}
			else
			{
				if(IsBeginningOfIdentifier(c))
					break;

				if (isdigit(c))
				{
					_identifierStart = _col - 1;
					ConsumeDotNumber(c);
					break;
				}
				
				ColorCurrent(TextEditor::PaletteIndex::Punctuation);
				AddToken(LuaToken::TYPE_DOT);
			}
			break;
		case '\'':
			ColorCurrent(TextEditor::PaletteIndex::String);
			if(!ConsumeString<'\''>(c))
				return;
			break;
		case '"':
			ColorCurrent(TextEditor::PaletteIndex::String);
			if (!ConsumeString<'"'>(c))
				return;
			break;
		default:

			// Start for ConsumeNumber
			_identifierStart = _col - 1;
			
			if (IsBeginningOfIdentifier(c))
			{
				ConsumeIdentifier(c);
				break;
			}

			if(isdigit(c))
			{
				ConsumeNumber(c);
				break;
			}

			ColorCurrent(TextEditor::PaletteIndex::ErrorMarker);
			AddToken({ LuaToken::TYPE_ERROR_BAD_CHARACTER });
			break;
		}

		c = GetNext();
	}
}

char LuaLexer::GetNext()
{
	//std::cout << "GetNext: \t" << _line << ", " << _col << " ; ";
	std::vector<TextEditor::Glyph>& glyphs = _lines[_line].mGlyphs;

	// After last character on this line?
	if (_col == glyphs.size())
	{
		// More lines after this one?
		if (_line + 1 < _lines.size())
		{
			_col = 0;
			++_line;
			_lines[_line].mTokens.clear();

			//std::cout << _line << ", " << _col << " : \\n\n";
			return '\n';
		}

		//std::cout << _line << ", " << _col << " : \\0\n";
		return '\0';
	}

	//std::cout << _line << ", " << _col + 1 << " : " << glyphs[_col].mChar << '\n';
	glyphs[_col].mColorIndex = TextEditor::PaletteIndex::Default;
	return glyphs[_col++].mChar;
}

char LuaLexer::PeekNext() const
{
	std::vector<TextEditor::Glyph>& glyphs = _lines[_line].mGlyphs;

	// After last character on this line?
	if (_col == glyphs.size())
	{
		// More lines after this one?
		if (_line + 1 < _lines.size())
		{
			//std::cout << "PeekNext: \t" << _line << ", " << _col << " : \\n\n";
			return '\n';
		}

		//std::cout << "PeekNext: \t" << _line << ", " << _col << " : \\0\n";
		return '\0';
	}

	//std::cout << "PeekNext: \t" << _line << ", " << _col << " : " << glyphs[_col].mChar << '\n';
	return glyphs[_col].mChar;
}


void LuaLexer::ColorCurrent(TextEditor::PaletteIndex color) const
{
	// Current is a newline
	if(_col == 0)
		return;

	_lines[_line].mGlyphs[_col - 1].mColorIndex = color;
}

void LuaLexer::ColorRange(size_t begin, size_t end, TextEditor::PaletteIndex color) const
{
	auto& glyphs = _lines[_line].mGlyphs;
	for(size_t i = begin; i <= end; ++i)
	{
		glyphs[i].mColorIndex = color;
	}
}

void LuaLexer::AddToken(LuaToken&& token) const
{
	assert(_line < _lines.size());
	_lines[_line].mTokens.emplace_back(token);
}

std::tuple<bool, LuaToken::Level> LuaLexer::ConsumeBeginLongComment(size_t pos)
{
	if (PeekNext() == '[')
	{
		GetNext();
		ColorCurrent(TextEditor::PaletteIndex::Comment);

		char c{ PeekNext() };
		if (c == '[')
		{
			GetNext();
			ColorCurrent(TextEditor::PaletteIndex::Comment);
			AddToken({ LuaToken::TYPE_BEGIN_LONG_COMMENT, {0, pos} });
			return { true, 0 };
		}

		if (c == '=')
		{
			GetNext();
			ColorCurrent(TextEditor::PaletteIndex::Comment);
			LuaToken::Level level {1};

			bool search = true;
			while (search)
			{
				c = PeekNext();

				switch (c)
				{
				case '[':
					GetNext();
					ColorCurrent(TextEditor::PaletteIndex::Comment);
					AddToken({ LuaToken::TYPE_BEGIN_LONG_COMMENT, {level, pos} });
					return { false, level };
				case '=':
					GetNext();
					ColorCurrent(TextEditor::PaletteIndex::Comment);
					++level;
					break;
				default:
					search = false;
					break;
				}
			}
		}
	}

	return { false, 0 };
}

// Returns true and level of bracket if a long bracket is detected, false and whatever otherwise
std::tuple<bool, LuaToken::Level> LuaLexer::ConsumeBeginLongString(size_t pos)
{
	char c{ PeekNext() };
	if (c == '[')
	{
		ColorCurrent(TextEditor::PaletteIndex::String);
		GetNext();
		ColorCurrent(TextEditor::PaletteIndex::String);
		AddToken({ LuaToken::TYPE_BEGIN_LONG_STRING, {0, pos} });
		return { true, 0 };
	}

	if (c == '=')
	{
		ColorCurrent(TextEditor::PaletteIndex::String);
		GetNext();
		LuaToken::Level level {1};

		bool search = true;
		while (search)
		{
			c = PeekNext();

			switch (c)
			{
			case '[':
				ColorCurrent(TextEditor::PaletteIndex::String);
				GetNext();
				ColorCurrent(TextEditor::PaletteIndex::String);
				AddToken({ LuaToken::TYPE_BEGIN_LONG_STRING, {level, pos} });
				return { true, level };
			case '=':
				ColorCurrent(TextEditor::PaletteIndex::String);
				GetNext();
				++level;
				break;
			default:
				// We have consumed '=' characters, but this wans't a long string. Caller need to move _col back
				search = false;
				break;
			}
		}
	}

	return { false, 0 };
}

bool LuaLexer::ConsumeLongBracket(LuaToken::Level level, TextEditor::PaletteIndex color)
{
	if(level == 0)
	{
		char c{ PeekNext() };

		bool search = true;
		while (search)
		{
			switch (c)
			{
			case ']':
				GetNext();
				ColorCurrent(color);

				c = PeekNext();
				switch (c)
				{
				case ']':
					AddToken({ LuaToken::TYPE_END_LONG_BRACKET, LuaToken::Bracket(level, _col) });
					GetNext();
					ColorCurrent(color);
					return true;
				case '\0':
					search = false;
					break;
				default:
					GetNext();
					ColorCurrent(color);
					c = PeekNext();
					break;
				}
				break;
			case '\0':
				search = false;
				break;
			default:
				GetNext();
				ColorCurrent(color);
				c = PeekNext();
				break;
			}
		}
	}
	else
	{
		char c{ PeekNext() };

		bool searchBracket = true;
		while (searchBracket)
		{
			LuaToken::Level currentLevel = 0;

			switch (c)
			{
			case ']':
				{
					GetNext();
					ColorCurrent(color);
					c = PeekNext();

					bool searchEquals = true;
					while (searchEquals)
					{
						switch (c)
						{
						case ']':
							if (level == currentLevel)
							{
								AddToken({ LuaToken::TYPE_END_LONG_BRACKET, LuaToken::Bracket(level, _col) });
								GetNext();
								ColorCurrent(color);
								return true;
							}

							GetNext();
							ColorCurrent(color);
							searchEquals = false;
							break;
						case '=':
							GetNext();
							ColorCurrent(color);
							c = PeekNext();
							++currentLevel;
							if (currentLevel > level)
							{
								searchEquals = false;
							}
							break;
						case '\0':
							searchEquals = searchBracket = false;
							break;
						default:
							GetNext();
							ColorCurrent(color);
							c = PeekNext();
							searchEquals = false;
							break;
						}
					}
				}
				break;
			case '\0':
				searchBracket = false;
				break;
			default:
				GetNext();
				ColorCurrent(color);
				c = PeekNext();
				break;
			}
		}
	}

	AddToken({LuaToken::TYPE_ERROR_BRACKET});
	return false;
}

void LuaLexer::ConsumeIdentifier(char c)
{
	_identifierStart = _col - 1;

	switch (c)
	{
	case 'a':
		return ConsumeAnd();
	case 'b':
		return ConsumeBreak();
	case 'd':
		return ConsumeDo();
	case 'e':
		return ConsumeEndElseElseif();
	case 'f':
		return ConsumeFalseForFunction();
	case 'i':
		return ConsumeIfIn();
	case 'l':
		return ConsumeLocal();
	case 'n':
		return ConsumeNilNot();
	case 'o':
		return ConsumeOr();
	case 'r':
		return ConsumeRepeatReturn();
	case 't':
		return ConsumeThenTrue();
	case 'u':
		return ConsumeUntil();
	case 'w':
		return ConsumeWhile();
	default:
		return ConsumeName(PeekNext());
	}
}

void LuaLexer::ConsumeAnd()
{
	char c = PeekNext();
	if(c != 'n')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (c != 'd')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if(IsPartOfIdentifier(c))
		return ConsumeName(c);

	ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
	AddToken({LuaToken::TYPE_AND});
}

void LuaLexer::ConsumeDo()
{
	char c = PeekNext();
	if (c != 'o')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (IsPartOfIdentifier(c))
		return ConsumeName(c);

	ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
	AddToken({LuaToken::TYPE_DO});
}

void LuaLexer::ConsumeBreak()
{
	char c = PeekNext();
	if (c != 'r')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (c != 'e')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (c != 'a')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (c != 'k')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (IsPartOfIdentifier(c))
		return ConsumeName(c);

	ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
	AddToken({LuaToken::TYPE_BREAK});
}

void LuaLexer::ConsumeFalseForFunction()
{
	char c = PeekNext();
	switch(c)
	{
	case 'a':
		GetNext();

		c = PeekNext();
		if (c != 'l')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (c != 's')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (c != 'e')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName(c);

		ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
		AddToken({LuaToken::TYPE_FALSE});
		break;
	case 'o':
		GetNext();
		
		c = PeekNext();
		if (c != 'r')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName(c);

		ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
		AddToken({LuaToken::TYPE_FOR});
		break;
	case 'u':
		GetNext();

		c = PeekNext();
		if (c != 'n')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (c != 'c')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (c != 't')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (c != 'i')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (c != 'o')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (c != 'n')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName(c);

		ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
		AddToken({LuaToken::TYPE_FUNCTION});
		break;
	default:
		return ConsumeName(c);
	}
}

void LuaLexer::ConsumeIfIn()
{
	char c = PeekNext();
	switch (c)
	{
	case 'f':
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName(c);

		ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
		AddToken({LuaToken::TYPE_IF});
		break;
	case 'n':
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName(c);

		ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
		AddToken({LuaToken::TYPE_IN});
		break;
	default:
		return ConsumeName(c);
	}
}

void LuaLexer::ConsumeLocal()
{
	char c = PeekNext();
	if (c != 'o')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (c != 'c')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (c != 'a')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (c != 'l')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (IsPartOfIdentifier(c))
		return ConsumeName(c);

	ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
	AddToken({LuaToken::TYPE_LOCAL});
}

void LuaLexer::ConsumeEndElseElseif()
{
	char c = PeekNext();
	switch (c)
	{
	case 'n':
		GetNext();

		c = PeekNext();
		if (c != 'd')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName(c);

		ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
		AddToken({LuaToken::TYPE_END});
		break;
	case 'l':
		GetNext();
		return ConsumeElseElseif();
	default:
		return ConsumeName(c);
	}
}

void LuaLexer::ConsumeElseElseif()
{
	char c = PeekNext();
	if (c != 's')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (c != 'e')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (!IsPartOfIdentifier(c))
	{
		ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
		AddToken({LuaToken::TYPE_ELSE});
		return;
	}

	if (c != 'i')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (c != 'f')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (IsPartOfIdentifier(c))
		return ConsumeName(c);

	ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
	AddToken({LuaToken::TYPE_ELSEIF});
}

void LuaLexer::ConsumeNilNot()
{
	char c = PeekNext();
	switch (c)
	{
	case 'i':
		GetNext();

		c = PeekNext();
		if (c != 'l')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName(c);

		ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
		AddToken({LuaToken::TYPE_NIL});
		break;
	case 'o':
		GetNext();

		c = PeekNext();
		if (c != 't')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName(c);

		ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
		AddToken({LuaToken::TYPE_NOT});
		break;
	default:
		return ConsumeName(c);
	}
}

void LuaLexer::ConsumeOr()
{
	char c = PeekNext();
	if (c != 'r')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (IsPartOfIdentifier(c))
		return ConsumeName(c);

	ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
	AddToken({LuaToken::TYPE_OR});
}

void LuaLexer::ConsumeRepeatReturn()
{
	char c = PeekNext();
	if (c != 'e')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	switch (c)
	{
	case 'p':
		GetNext();

		c = PeekNext();
		if (c != 'e')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (c != 'a')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (c != 't')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName(c);

		ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
		AddToken({LuaToken::TYPE_REPEAT});
		break;
	case 't':
		GetNext();

		c = PeekNext();
		if (c != 'u')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (c != 'r')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (c != 'n')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName(c);

		ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
		AddToken({LuaToken::TYPE_RETURN});
		break;
	default:
		return ConsumeName(c);
	}
}

void LuaLexer::ConsumeThenTrue()
{
	char c = PeekNext();

	switch (c)
	{
	case 'h':
		GetNext();

		c = PeekNext();
		if (c != 'e')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (c != 'n')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName(c);

		ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
		AddToken({LuaToken::TYPE_THEN});
		break;
	case 'r':
		GetNext();

		c = PeekNext();
		if (c != 'u')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (c != 'e')
			return ConsumeName(c);
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName(c);

		ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
		AddToken({LuaToken::TYPE_TRUE});
		break;
	default:
		return ConsumeName(c);
	}
}

void LuaLexer::ConsumeUntil()
{
	char c = PeekNext();
	if (c != 'n')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (c != 't')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (c != 'i')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (c != 'l')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (IsPartOfIdentifier(c))
		return ConsumeName(c);

	ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
	AddToken({LuaToken::TYPE_UNTIL});
}

void LuaLexer::ConsumeWhile()
{
	char c = PeekNext();
	if (c != 'h')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (c != 'i')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (c != 'l')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (c != 'e')
		return ConsumeName(c);
	GetNext();

	c = PeekNext();
	if (IsPartOfIdentifier(c))
		return ConsumeName(c);

	ColorRange(_identifierStart, _col - 1, TextEditor::PaletteIndex::Keyword);
	AddToken({LuaToken::TYPE_WHILE});
}

void LuaLexer::ConsumeName(char c)
{
	while(IsPartOfIdentifier(c))
	{
		GetNext();
		c = PeekNext();
	}

	const size_t identifierEnd = _col - 1;

	ColorRange(_identifierStart, identifierEnd, TextEditor::PaletteIndex::Identifier);

	const size_t strSize = identifierEnd - _identifierStart + 1;

	std::string name;
	name.reserve(strSize);
	auto& glyphs = _lines[_line].mGlyphs;
	for(size_t i = _identifierStart; i <= identifierEnd; ++i)
		name.append(1, glyphs[i].mChar);

	AddToken({ LuaToken::TYPE_NAME, name });
}

void LuaLexer::ConsumeNumber(char c)
{
	const char first = c;
	c = PeekNext();

	// Is hex number
	if(first == '0' && (c == 'x' || c == 'X'))
	{
		GetNext();
		return ConsumeHexNumber(PeekNext());
	}

	// Consume optional digits
	while (isdigit(c))
	{
		GetNext();
		c = PeekNext();
	}

	// Optional dot
	if(c == '.')
	{
		return ConsumeDotNumber(c);
	}

	if (IsBeginningOfIdentifier(c))
	{
		// TODO all characters after the digits, until the next whitespace, should be in the message
		MalformedNumber(_identifierStart, _col);
		return;
	}

	const size_t end = _col - 1;
	ColorRange(_identifierStart, end, TextEditor::PaletteIndex::Number);
	AddToken({ LuaToken::TYPE_NUMBER });
}

void LuaLexer::ConsumeDotNumber(char c)
{
	GetNext();

	// Consume optional digits
	c = PeekNext();
	while(isdigit(c))
	{
		GetNext();
		c = PeekNext();
	}

	if(c == 'e' || c == 'E')
	{
		GetNext();
		c = PeekNext();

		// Optional -
		if (c == '-')
		{
			GetNext();
			c = PeekNext();
		}

		// Atleast one digit
		if (!isdigit(c))
		{
			// TODO all characters after the 'e', until the next whitespace, should be in the message
			MalformedNumber(_identifierStart, _col - 1);
			return;
		}

		GetNext();

		// Consume all following digits
		c = PeekNext();
		while (isdigit(c))
		{
			GetNext();
			c = PeekNext();
		}
	}

	if(IsBeginningOfIdentifier(c))
	{
		MalformedNumber(_identifierStart, _col);
		return;
	}

	const size_t end = _col - 1;
	ColorRange(_identifierStart, end, TextEditor::PaletteIndex::Number);
	AddToken({ LuaToken::TYPE_NUMBER });
}

void LuaLexer::ConsumeHexNumber(char c)
{
	while(isxdigit(c))
	{
		GetNext();
		c = PeekNext();
	}

	if (IsBeginningOfIdentifier(c))
	{
		MalformedNumber(_identifierStart, _col);
		return;
	}

	const size_t end = _col - 1;
	ColorRange(_identifierStart, end, TextEditor::PaletteIndex::Number);
	AddToken({ LuaToken::TYPE_NUMBER });
}

void LuaLexer::MalformedNumber(size_t start, size_t end) const
{
	auto& glyphs = _lines[_line].mGlyphs;

	std::string msg("malformed number near '");
	const size_t numSize = end - start + 2 + msg.size();
	msg.reserve(numSize + 1);
	
	for(size_t i = start; i <= end; ++i)
		msg.append(1, glyphs[i].mChar);
	msg.append(1, '\'');

	AddToken({ LuaToken::TYPE_ERROR_MALFORMED_NUMBER, msg });
}
