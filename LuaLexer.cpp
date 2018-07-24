#include "LuaLexer.h"

#include "LuaToken.h"

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
			break;
		case '-':
			if (PeekNext() == '-') // Comment or long comment
			{
				GetNext();
				const size_t commentStart = _col - 2;

				auto [longComment, level] = ConsumeBeginLongComment(commentStart);

				if(longComment)
				{
					if(!ConsumeLongBracket(level))
						return;
				}
				else
				{
					// Point at the second '-'
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
					} while (true);
				}
			}
			else
				AddToken(LuaToken::TYPE_MINUS);
			break;
		case '*':
			AddToken(LuaToken::TYPE_MUL);
			break;
		case '/':
			AddToken(LuaToken::TYPE_DIV);
			break;
		case '%':
			AddToken(LuaToken::TYPE_MOD);
			break;
		case '^':
			AddToken(LuaToken::TYPE_EXP);
			break;
		case '#':
			AddToken(LuaToken::TYPE_HASH);
			break;
		case '=':
			if (PeekNext() == '=')
			{
				GetNext();
				AddToken(LuaToken::TYPE_EQ);
			}
			else
				AddToken(LuaToken::TYPE_ASSIGN);
			break;
		case '~':
			if(PeekNext() == '=')
			{
				GetNext();
				AddToken(LuaToken::TYPE_NEQ);
			}
			else // Adding error, but keep parsing so the rest of the code get syntax highlight
				AddToken({ LuaToken::TYPE_ERROR_STRAY_TILDE });
			break;
		case '<':
			if (PeekNext() == '=')
			{
				GetNext();
				AddToken(LuaToken::TYPE_LE);
			}
			else
				AddToken(LuaToken::TYPE_LT);
			break;
		case '>':
			if (PeekNext() == '=')
			{
				GetNext();
				AddToken(LuaToken::TYPE_GE);
			}
			else
				AddToken(LuaToken::TYPE_GT);
			break;
		case '(':
			AddToken(LuaToken::TYPE_LEFT_B);
			break;
		case ')':
			AddToken(LuaToken::TYPE_RIGHT_B);
			break;
		case '{':
			AddToken(LuaToken::TYPE_LEFT_CB);
			break;
		case '}':
			AddToken(LuaToken::TYPE_RIGHT_CB);
			break;
		case '[':
			{
				const size_t stringStart = _col - 1;

				auto[longString, level] = ConsumeBeginLongString(stringStart);

				if (longString)
				{
					if (!ConsumeLongBracket(level))
						return;
				}
				else
				{
					// ConsumeBeginLongString might have failed to match. Need move _col back
					_col = stringStart + 1;

					AddToken(LuaToken::TYPE_LEFT_SB);
				}
			}
			break;
		case ']':
			AddToken(LuaToken::TYPE_RIGHT_SB);
			break;
		case ';':
			AddToken(LuaToken::TYPE_SEMI_COLON);
			break;
		case ':':
			AddToken(LuaToken::TYPE_COLON);
			break;
		case ',':
			AddToken(LuaToken::TYPE_COMMA);
			break;
		case '.':
			if (PeekNext() == '.')
			{
				GetNext();
				if (PeekNext() == '.')
				{
					GetNext();
					AddToken(LuaToken::TYPE_VARARG);
				}
				else
					AddToken(LuaToken::TYPE_CONCAT);
			}
			else
				AddToken(LuaToken::TYPE_DOT);
			break;
		case '\'':
			if(!ConsumeString<'\''>(c))
				return;
			break;
		case '"':
			if (!ConsumeString<'"'>(c))
				return;
			break;
		default:
			if (IsBeginningOfIdentifier(c))
				ConsumeIdentifier(c);
			else
				AddToken({ LuaToken::TYPE_ERROR_BAD_CHARACTER });
			break;
		}

		// TODO Verify that all operation leave the next character alone
		c = GetNext();
	}
}

char LuaLexer::GetNext()
{
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

			return '\n';
		}

		return '\0';
	}

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
			return '\n';
		}

		return '\0';
	}

	return glyphs[_col].mChar;
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

		char c{ PeekNext() };
		if (c == '[')
		{
			GetNext();
			AddToken({ LuaToken::TYPE_BEGIN_LONG_COMMENT, {0, pos} });
			return { true, 0 };
		}

		if (c == '=')
		{
			GetNext();
			LuaToken::Level level {1};

			bool search = true;
			while (search)
			{
				c = PeekNext();

				switch (c)
				{
				case '[':
					GetNext();
					AddToken({ LuaToken::TYPE_BEGIN_LONG_COMMENT, {level, pos} });
					return { false, level };
				case '=':
					GetNext();
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
		GetNext();
		AddToken({ LuaToken::TYPE_BEGIN_LONG_STRING, {0, pos} });
		return { true, 0 };
	}

	if (c == '=')
	{
		GetNext();
		LuaToken::Level level {1};

		bool search = true;
		while (search)
		{
			c = PeekNext();

			switch (c)
			{
			case '[':
				GetNext();
				AddToken({ LuaToken::TYPE_BEGIN_LONG_STRING, {level, pos} });
				return { false, level };
			case '=':
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

bool LuaLexer::ConsumeLongBracket(LuaToken::Level level)
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

				c = PeekNext();
				switch (c)
				{
				case ']':
					AddToken({ LuaToken::TYPE_END_LONG_BRACKET, LuaToken::Bracket(level, _col) });
					GetNext();
					return true;
				case '\0':
					search = false;
					break;
				default:
					GetNext();
					c = PeekNext();
					break;
				}
				break;
			case '\0':
				search = false;
				break;
			default:
				GetNext();
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
					c = PeekNext();

					bool searchEquals = true;
					while (searchEquals)
					{
						switch (c)
						{
						case ']':
							if (level != currentLevel)
							{
								AddToken({ LuaToken::TYPE_END_LONG_BRACKET, LuaToken::Bracket(level, _col) });
								GetNext();
								return true;
							}

							GetNext();
							searchEquals = false;
							break;
						case '=':
							GetNext();
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
		GetNext();
		return ConsumeAnd();
	case 'b':
		GetNext();
		return ConsumeBreak();
	case 'd':
		GetNext();
		return ConsumeDo();
	case 'f':
		GetNext();
		return ConsumeFalseForFunction();
	case 'i':
		GetNext();
		return ConsumeIfIn();
	case 'l':
		GetNext();
		return ConsumeLocal();
	case 'n':
		GetNext();
		return ConsumeNilNot();
	case 'o':
		GetNext();
		return ConsumeOr();
	case 'r':
		GetNext();
		return ConsumeRepeatReturn();
	case 't':
		GetNext();
		return ConsumeThenTrue();
	case 'u':
		GetNext();
		return ConsumeUntil();
	case 'w':
		GetNext();
		return ConsumeWhile();
	default:
		return ConsumeName();
	}
}

void LuaLexer::ConsumeAnd()
{
	char c = PeekNext();
	if(c != 'n')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (c != 'd')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if(IsPartOfIdentifier(c))
		return ConsumeName();

	AddToken({LuaToken::TYPE_AND});
}

void LuaLexer::ConsumeDo()
{
	char c = PeekNext();
	if (c != 'o')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (IsPartOfIdentifier(c))
		return ConsumeName();

	AddToken({LuaToken::TYPE_DO});
}

void LuaLexer::ConsumeBreak()
{
	char c = PeekNext();
	if (c != 'r')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (c != 'e')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (c != 'a')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (c != 'k')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (IsPartOfIdentifier(c))
		return ConsumeName();

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
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (c != 's')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (c != 'e')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName();

		AddToken({LuaToken::TYPE_FALSE});
		break;
	case 'o':
		GetNext();
		
		c = PeekNext();
		if (c != 'r')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName();

		AddToken({LuaToken::TYPE_FOR});
		break;
	case 'u':
		GetNext();

		c = PeekNext();
		if (c != 'n')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (c != 'c')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (c != 't')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (c != 'i')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (c != 'o')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (c != 'n')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName();

		AddToken({LuaToken::TYPE_FUNCTION});
		break;
	default:
		return ConsumeName();
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
			return ConsumeName();

		AddToken({LuaToken::TYPE_IF});
		break;
	case 'n':
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName();

		AddToken({LuaToken::TYPE_IN});
		break;
	default:
		return ConsumeName();
	}
}

void LuaLexer::ConsumeLocal()
{
	char c = PeekNext();
	if (c != 'o')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (c != 'c')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (c != 'a')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (c != 'l')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (IsPartOfIdentifier(c))
		return ConsumeName();

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
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName();

		AddToken({LuaToken::TYPE_END});
		break;
	case 'l':
		GetNext();
		return ConsumeElseElseif();
	default:
		return ConsumeName();
	}
}

void LuaLexer::ConsumeElseElseif()
{
	char c = PeekNext();
	if (c != 's')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (c != 'e')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (!IsPartOfIdentifier(c))
	{
		AddToken({LuaToken::TYPE_ELSE});
		return;
	}

	if (c != 'i')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (c != 'f')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (IsPartOfIdentifier(c))
		return ConsumeName();

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
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName();

		AddToken({LuaToken::TYPE_NIL});
		break;
	case 'o':
		GetNext();

		c = PeekNext();
		if (c != 't')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName();

		AddToken({LuaToken::TYPE_NOT});
		break;
	default:
		return ConsumeName();
	}
}

void LuaLexer::ConsumeOr()
{
	char c = PeekNext();
	if (c != 'r')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (IsPartOfIdentifier(c))
		return ConsumeName();

	AddToken({LuaToken::TYPE_OR});
}

void LuaLexer::ConsumeRepeatReturn()
{
	char c = PeekNext();
	if (c != 'e')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	switch (c)
	{
	case 'p':
		GetNext();

		c = PeekNext();
		if (c != 'e')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (c != 'a')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (c != 't')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName();

		AddToken({LuaToken::TYPE_REPEAT});
		break;
	case 't':
		GetNext();

		c = PeekNext();
		if (c != 'u')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (c != 'r')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (c != 'n')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName();

		AddToken({LuaToken::TYPE_RETURN});
		break;
	default:
		return ConsumeName();
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
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (c != 'n')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName();

		AddToken({LuaToken::TYPE_THEN});
		break;
	case 'r':
		GetNext();

		c = PeekNext();
		if (c != 'u')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (c != 'e')
			return ConsumeName();
		GetNext();

		c = PeekNext();
		if (IsPartOfIdentifier(c))
			return ConsumeName();

		AddToken({LuaToken::TYPE_TRUE});
		break;
	default:
		return ConsumeName();
	}
}

void LuaLexer::ConsumeUntil()
{
	char c = PeekNext();
	if (c != 'n')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (c != 't')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (c != 'i')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (c != 'l')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (IsPartOfIdentifier(c))
		return ConsumeName();

	AddToken({LuaToken::TYPE_UNTIL});
}

void LuaLexer::ConsumeWhile()
{
	char c = PeekNext();
	if (c != 'h')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (c != 'i')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (c != 'l')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (c != 'e')
		return ConsumeName();
	GetNext();

	c = PeekNext();
	if (IsPartOfIdentifier(c))
		return ConsumeName();

	AddToken({LuaToken::TYPE_WHILE});
}

void LuaLexer::ConsumeName()
{
	char c = PeekNext();
	while(IsPartOfIdentifier(c))
	{
		GetNext();
		c = PeekNext();
	}

	const size_t identifierEnd = _col - 1;

	AddToken({ LuaToken::TYPE_NAME, { _identifierStart, identifierEnd} });
}

