#include "LuaParser.h"

bool LuaParser::ParseAll()
{
	// TODO Temporary, to not halt the program
	//return true;

	if (_lines.empty())
		return true;

	_line = 0;
	_col = 0;
	_lines[0].mTokens.clear();

	NextToken();
	if(_currentToken->_type == LuaToken::TYPE_EOS)
		return true;

	try
	{
		OpenFunction(1);
		_currentFunctionState->SetVararg(); /* main func. is always vararg */
		Chunk();

		Check(LuaToken::TYPE_EOS);

		CloseFunction(_line);
	}
	catch(LuaSyntaxException& e)
	{
		return false;
	}
	
	return true;
}

std::pair<std::string, size_t> LuaParser::GetError()
{
	return {_errorMsg, _errorLine};
}

const std::vector<LuaVariable>& LuaParser::GetVariables() const
{
	return _variables;
}

void LuaParser::NextToken()
{
	std::vector<LuaToken>& tokens = _lines[_line].mTokens;

	// After last token on this line?
	if (_col == tokens.size())
	{
		// More lines after this one?
		if (_line + 1 < _lines.size())
		{
			_col = 1;
			++_line;

			// Skip lines with no tokens
			while(_lines[_line].mTokens.empty())
			{
				++_line;
			}

			_currentToken = &_lines[_line].mTokens[0];
			return;
		}

		// EOS
		return;
	}

	_currentToken = &tokens[_col++];
}

LuaToken* LuaParser::PeekToken() const
{
	std::vector<LuaToken>& tokens = _lines[_line].mTokens;

	// After last token on this line?
	if (_col == tokens.size())
	{
		// More lines after this one?
		if (_line + 1 < _lines.size())
		{
			size_t line = _line + 1;

			// Skip lines with no tokens
			while (_lines[line].mTokens.empty())
			{
				++line;
			}

			return &_lines[line].mTokens[0];
		}

		// EOS
		return &_lines[_line].mTokens[_col - 1];
	}

	return &tokens[_col];
}

bool LuaParser::BlockFollow() const
{
	switch (_currentToken->_type)
	{
	case LuaToken::TYPE_ELSE: case LuaToken::TYPE_ELSEIF: case LuaToken::TYPE_END:
	case LuaToken::TYPE_UNTIL: case LuaToken::TYPE_EOS:
		return true;
	default:
		return false;
	}
}

bool LuaParser::TestNext(LuaToken::Type type)
{
	if (_currentToken->_type == type)
	{
		NextToken();
		return true;
	}
	return false;
}

void LuaParser::CheckMatch(LuaToken::Type what, LuaToken::Type who, size_t where)
{
	if (!TestNext(what))
	{
		if (where == _line)
			ErrorExpected(what);
		else
		{
			SyntaxError(LuaToken::ToString(what) + " expected (to close " + 
				LuaToken::ToString(who) + " at line " + std::to_string(where) + ")");
		}
	}
}

void LuaParser::SyntaxError(const std::string& msg)
{
	// NOTE the error message i present in the editor is a stripped down
	// version of the original. So i shouldn't try to copy the original syntax error function

	_errorMsg = msg + " near " + LuaToken::ToString(_currentToken->_type);
	// TODO Error should maybe be presented on other line than current?
	_errorLine = _line;
	throw LuaSyntaxException();
}

void LuaParser::ErrorExpected(LuaToken::Type token)
{
	SyntaxError(LuaToken::ToString(token) + " expected");
}

void LuaParser::Check(LuaToken::Type type)
{
	if (_currentToken->_type != type)
	{
		ErrorExpected(type);
	}
}

void LuaParser::CheckNext(LuaToken::Type type)
{
	Check(type);
	NextToken();
}

//
void LuaParser::Chunk()
{
	bool islast = false;
	//enterlevel(ls);
	while (!islast && !BlockFollow())
	{
		islast = Statement();
		TestNext(LuaToken::TYPE_SEMI_COLON);
	}
	//leavelevel(ls);
}

void LuaParser::Block()
{
	/* block -> chunk */
	_currentFunctionState->OpenBlock(false);
	Chunk();
	// TODO Verify that line is ok
	_currentFunctionState->CloseBlock(_line);
}

void LuaParser::EnterBlock(bool breakable)
{
	_currentFunctionState->OpenBlock(breakable);
}

void LuaParser::LeaveBlock()
{
	_currentFunctionState->CloseBlock(_line);
}

void LuaParser::OpenFunction(size_t lineDefined)
{
	if(_functionStates.empty())
	{
		_functionStates.emplace_back(lineDefined);
		_currentFunctionState = &(*_functionStates.rbegin());
	}
	else
	{
		auto previousFunctionState = &(*_functionStates.rbegin());
		_functionStates.emplace_back(lineDefined, previousFunctionState);
		_currentFunctionState = &(*_functionStates.rbegin());
	}
}

void LuaParser::CloseFunction(size_t lastLineDefined)
{
	if(_currentFunctionState == nullptr)
	{
		// ERROR
	}

	// Add all variables in this function to the text editor
	const auto& variables = _currentFunctionState->Close(lastLineDefined);
	for (const auto& variable : variables)
		_textEditor->AddVariable(variable);

	// Set previous function state to current
	_functionStates.pop_back();
	if(_functionStates.empty())
		_currentFunctionState = nullptr;
	else
		_currentFunctionState = &(*_functionStates.rbegin());
}

bool LuaParser::FuncName()
{
	/* funcname -> NAME {field} [`:' NAME] */

	bool needself = false;
	Check(LuaToken::TYPE_NAME);
	auto& data = std::get<LuaToken::NameData>(_currentToken->_data);
	_currentFunctionState->Variable(data._name, { _line, data._startCol, data._endCol });
	NextToken();
	while (_currentToken->_type == '.')
		Field();

	if (_currentToken->_type == ':')
	{
		needself = true;
		Field();
	}
	return needself;
}

void LuaParser::FuncArgs()
{
	const auto line = _line;
	switch (_currentToken->_type)
	{
	case LuaToken::TYPE_LEFT_B:
		/* funcargs -> `(' [ explist1 ] `)' */
		// TODO Ambiguous syntax. Need to capture the line before this one
		//if (line != ls->lastline)
		//	SyntaxError("ambiguous syntax (function call x new statement)");
		NextToken();
		if (_currentToken->_type != LuaToken::TYPE_RIGHT_B) // Arglist not empty?
		{
			ExpList1();
		}
		CheckMatch(LuaToken::TYPE_RIGHT_B, LuaToken::TYPE_LEFT_B, line);
		break;

	case LuaToken::TYPE_LEFT_CB:
		/* funcargs -> constructor */
		Constructor();
		break;
	case LuaToken::TYPE_STRING:
		/* funcargs -> STRING */
		NextToken();
		break;
	default:
		SyntaxError("function arguments expected");
		break;
	}
}

void LuaParser::ExpList1()
{
	/* explist1 -> expr { `,' expr } */
	Expr();
	while (TestNext(LuaToken::TYPE_COMMA))
	{
		Expr();
	}
}

void LuaParser::Body(bool needSelf, size_t line)
{
	/* body ->  `(' parlist `)' chunk END */

	OpenFunction(line);
	CheckNext(LuaToken::TYPE_LEFT_B);
	if (needSelf)
	{
		// TODO SOMETHING????? Yeah. self is an implicit local variable.
		// TODO Need to add it to function state, without treating it as a variable in the text
	}
	ParList();

	CheckNext(LuaToken::TYPE_RIGHT_B);
	Chunk();

	const auto lastlinedefined = _line;
	CheckMatch(LuaToken::TYPE_END, LuaToken::TYPE_FUNCTION, line);
	CloseFunction(lastlinedefined);
}


void LuaParser::ParList()
{
	/* parlist -> [ param { `,' param } ] */
	if (_currentToken->_type != LuaToken::TYPE_RIGHT_B)
	{  /* is `parlist' not empty? */
		do
		{
			switch (_currentToken->_type)
			{
			case LuaToken::TYPE_NAME:
				{  /* param -> NAME */
					auto& data = std::get<LuaToken::NameData>(_currentToken->_data);
					_currentFunctionState->AddLocal(data._name, { _line, data._startCol, data._endCol });
					NextToken();
					break;
				}
			case LuaToken::TYPE_VARARG:
				/* param -> `...' */
				NextToken();
				_currentFunctionState->SetVararg();
				break;
			default:
				SyntaxError("<name> or '...' expected");
			}
		} while (!_currentFunctionState->IsVararg() && TestNext(LuaToken::TYPE_COMMA));
	}
}

bool LuaParser::Statement()
{
	size_t line = _line;  /* may be needed for error messages */
	switch (_currentToken->_type)
	{
	case LuaToken::TYPE_IF:
		{   /* stat -> ifstat */
			IfStat(line);
			return false;
		}
	case LuaToken::TYPE_WHILE:
		{   /* stat -> whilestat */
			WhileStat(line);
			return false;
		}
	case LuaToken::TYPE_DO:
		{   /* stat -> DO block END */
			NextToken();  /* skip DO */
			Block();
			CheckMatch(LuaToken::TYPE_END, LuaToken::TYPE_DO, line);
			return false;
		}
	case LuaToken::TYPE_FOR:
		{   /* stat -> forstat */
			ForStat(line);
			return false;
		}
	case LuaToken::TYPE_REPEAT:
		{   /* stat -> repeatstat */
			RepeatStat(line);
			return false;
		}
	case LuaToken::TYPE_FUNCTION:
		{
			FuncStat(line);  /* stat -> funcstat */
			return false;
		}
	case LuaToken::TYPE_LOCAL:
		{   /* stat -> localstat */
			NextToken();  /* skip LOCAL */
			if (TestNext(LuaToken::TYPE_FUNCTION))  /* local function? */
				LocalFunction();
			else
				LocalStat();
			return false;
		}
	case LuaToken::TYPE_RETURN:
		{   /* stat -> retstat */
			RetStat();
			return true;  /* must be last statement */
		}
	case LuaToken::TYPE_BREAK:
		{   /* stat -> breakstat */
			NextToken();  /* skip BREAK */
			BreakStat();
			return true;  /* must be last statement */
		}
	default:
		{
			ExprStat();
			return false;  /* to avoid warnings */
		}
	}
}

void LuaParser::IfStat(size_t line)
{
	/* ifstat -> IF cond THEN block {ELSEIF cond THEN block} [ELSE block] END */

	TestThenBlock();  /* IF cond THEN block */
	while (_currentToken->_type == LuaToken::TYPE_ELSEIF)
	{
		TestThenBlock();  /* IF cond THEN block */
	}
	if (_currentToken->_type == LuaToken::TYPE_ELSE)
	{
		NextToken(); // skip ELSE
		Block();  /* `else' part */
	}

	CheckMatch(LuaToken::TYPE_END, LuaToken::TYPE_IF, line);
}

void LuaParser::WhileStat(size_t line)
{
	/* whilestat -> WHILE cond DO block END */
	NextToken();  /* skip WHILE */
	Cond();
	EnterBlock(true);
	CheckNext(LuaToken::TYPE_DO);
	Block();
	CheckMatch(LuaToken::TYPE_END, LuaToken::TYPE_WHILE, line);
	LeaveBlock();
}

void LuaParser::ForStat(size_t line)
{
	/* forstat -> FOR (fornum | forlist) END */

	EnterBlock(true);  /* scope for loop and control variables */
	NextToken();  /* skip `for' */

	/* first variable name */
	Check(LuaToken::TYPE_NAME);
	auto& data = std::get<LuaToken::NameData>(_currentToken->_data);
	_currentFunctionState->AddLocal(data._name, { _line, data._startCol, data._endCol });
	NextToken();

	switch (_currentToken->_type)
	{
	case LuaToken::TYPE_ASSIGN:
		ForNum(line);
		break;
	case LuaToken::TYPE_COMMA:
	case LuaToken::TYPE_IN:
		ForList();
		break;
	default:
		SyntaxError("'=' or 'in' expected");
	}
	CheckMatch(LuaToken::TYPE_END, LuaToken::TYPE_FOR, line);
	LeaveBlock();  /* loop scope (`break' jumps to this point) */
}

void LuaParser::RepeatStat(size_t line)
{
	/* repeatstat -> REPEAT block UNTIL cond */

	EnterBlock(true);  /* loop block */
	EnterBlock(false);  /* scope block */
	NextToken();  /* skip REPEAT */
	Chunk();
	CheckMatch(LuaToken::TYPE_UNTIL, LuaToken::TYPE_REPEAT, line);

	Cond();  /* read condition (inside scope block) */

	LeaveBlock();  /* finish scope */
	LeaveBlock();  /* finish loop */
}

void LuaParser::FuncStat(size_t line)
{
	/* funcstat -> FUNCTION funcname body */

	// TODO Associate doc comment here
	NextToken();  /* skip FUNCTION */
	const bool needself = FuncName();
	
	Body(needself, line);
}

void LuaParser::LocalFunction()
{
	// TODO Associating doc comment could be tricky here
	// TODO The 'local' keyword could come in the way of the comment
	Check(LuaToken::TYPE_NAME);
	auto& data = std::get<LuaToken::NameData>(_currentToken->_data);
	_currentFunctionState->AddLocal(data._name, { _line, data._startCol, data._endCol });
	NextToken();
	
	Body(false, _line);
}

void LuaParser::LocalStat()
{
	std::vector<std::pair<LuaToken::NameData*, size_t>> locals;
	/* stat -> LOCAL NAME {`,' NAME} [`=' explist1] */
	do
	{
		Check(LuaToken::TYPE_NAME);
		locals.emplace_back(&std::get<LuaToken::NameData>(_currentToken->_data), _line);
		NextToken();
	} while (TestNext(LuaToken::TYPE_COMMA));
	if (TestNext(LuaToken::TYPE_ASSIGN))
		ExpList1();

	// Add locals after assignmnts are done
	for (auto& local : locals)
	{
		_currentFunctionState->AddLocal(local.first->_name,
			{ local.second, local.first->_startCol, local.first->_endCol});
	}
}

void LuaParser::RetStat()
{
	/* stat -> RETURN explist */

	NextToken();  /* skip RETURN */
	if(!BlockFollow() && _currentToken->_type != LuaToken::TYPE_SEMI_COLON)
	{
		ExpList1(); /* optional return values */
	}
}

void LuaParser::BreakStat()
{
	if(!_currentFunctionState->IsBreakable())
		SyntaxError("no loop to break");
}

void LuaParser::ExprStat()
{
	const bool functionCall = PrimaryExp();
	if (!functionCall)
	{
		Assignment(1);
	}
}

void LuaParser::TestThenBlock()
{
	/* test_then_block -> [IF | ELSEIF] cond THEN block */
	NextToken();  /* skip IF or ELSEIF */
	Cond();
	CheckNext(LuaToken::TYPE_THEN);
	Block();  /* `then' part */
}

void LuaParser::Cond()
{
	Expr();
}

void LuaParser::ForBody()
{
	/* forbody -> DO block */
	CheckNext(LuaToken::TYPE_DO);
	EnterBlock(false);  /* scope for declared variables */
	Block();
	LeaveBlock();  /* end of scope for declared variables */
}

void LuaParser::ForNum(size_t line)
{
	/* fornum -> NAME = exp1,exp1[,exp1] forbody */

	// TODO NOTE Already made local
	//new_localvar(ls, varname, 3);
	CheckNext(LuaToken::TYPE_ASSIGN);
	Expr();  /* initial value */
	CheckNext(LuaToken::TYPE_COMMA);
	Expr();  /* limit */
	if (TestNext(LuaToken::TYPE_COMMA))
		Expr();  /* optional step */

	ForBody();
}

void LuaParser::ForList()
{
	/* forlist -> NAME {,NAME} IN explist1 forbody */

	while (TestNext(LuaToken::TYPE_COMMA))
	{
		Check(LuaToken::TYPE_NAME);
		auto& data = std::get<LuaToken::NameData>(_currentToken->_data);
		_currentFunctionState->AddLocal(data._name, { _line, data._startCol, data._endCol });
		NextToken();
	}

	CheckNext(LuaToken::TYPE_IN);
	ExpList1();
	ForBody();
}

void LuaParser::Assignment(int nVars)
{
	// TODO After the whole assingment the potential lh locals need to be added

	// TODO What does this condition check???? If it was a call?
	//check_condition(ls, VLOCAL <= lh->v.k && lh->v.k <= VINDEXED,
	//	"syntax error");
	if (TestNext(LuaToken::TYPE_COMMA))
	{  /* assignment -> `,' primaryexp assignment */

		bool functionCall = PrimaryExp();
		// TODO Could check for illegal function call here

		//if (nv.v.k == VLOCAL)
		//	check_conflict(ls, lh, &nv.v);
		//luaY_checklimit(ls->fs, nvars, LUAI_MAXCCALLS - ls->L->nCcalls,
		//	"variables in assignment");
		Assignment(nVars + 1);
	}
	else
	{  /* assignment -> `=' explist1 */
		CheckNext(LuaToken::TYPE_ASSIGN);
		ExpList1();
	}
}

bool LuaParser::IsUnopr() const
{
	switch (_currentToken->_type)
	{
	case LuaToken::TYPE_NOT:
	case LuaToken::TYPE_MINUS:
	case LuaToken::TYPE_HASH:
		return true;
	default:
		return false;
	}
}

bool LuaParser::IsBinopr() const
{
	switch (_currentToken->_type)
	{
	case LuaToken::TYPE_PLUS:
	case LuaToken::TYPE_MINUS:
	case LuaToken::TYPE_MUL:
	case LuaToken::TYPE_DIV:
	case LuaToken::TYPE_MOD:
	case LuaToken::TYPE_EXP:
	case LuaToken::TYPE_CONCAT:
	case LuaToken::TYPE_NEQ:
	case LuaToken::TYPE_EQ:
	case LuaToken::TYPE_LT:
	case LuaToken::TYPE_LE:
	case LuaToken::TYPE_GT:
	case LuaToken::TYPE_GE:
	case LuaToken::TYPE_AND:
	case LuaToken::TYPE_OR:
		return true;
	default:
		return false;
	}
}

void LuaParser::PrefixExp()
{
	/* prefixexp -> NAME | '(' expr ')' */
	switch (_currentToken->_type)
	{
	case LuaToken::TYPE_LEFT_B:
		{
			const auto line = _line;
			NextToken();
			Expr();
			CheckMatch(LuaToken::TYPE_RIGHT_B, LuaToken::TYPE_LEFT_B, line);
		}
		break;
	case LuaToken::TYPE_NAME:
		{
			auto& data = std::get<LuaToken::NameData>(_currentToken->_data);
			_currentFunctionState->Variable(data._name, {_line, data ._startCol, data._endCol});
			NextToken();
		}
		break;
	default:
		SyntaxError("unexpected symbol");
	}
}

bool LuaParser::PrimaryExp()
{
	/* primaryexp ->
		prefixexp { `.' NAME | `[' exp `]' | `:' NAME funcargs | funcargs } */

	bool functionCall = false;

	//FuncState *fs = ls->fs;
	PrefixExp();
	for (;;)
	{
		switch (_currentToken->_type)
		{
		case LuaToken::TYPE_DOT:
		{  /* field */
			Field();
			functionCall = false;
			break;
		}
		case LuaToken::TYPE_LEFT_SB:
		{  /* `[' exp1 `]' */
			YIndex();
			functionCall = false;
			break;
		}
		case LuaToken::TYPE_COLON:
		{  /* `:' NAME funcargs */
			NextToken();
			Check(LuaToken::TYPE_NAME);
			// TODO Add name to function state somehow? Would need relation to previous variables
			NextToken();
			// TODO SELF??
			//luaK_self(fs, v, &key);
			FuncArgs();
			functionCall = true;
			break;
		}
		case LuaToken::TYPE_LEFT_B:
		case LuaToken::TYPE_STRING:
		case LuaToken::TYPE_LEFT_CB:
		{  /* funcargs */
			FuncArgs();
			functionCall = true;
			break;
		}
		default:
			return functionCall;
		}
	}
}

void LuaParser::SimpleExp()
{
	/* simpleexp -> NUMBER | STRING | NIL | true | false | ... |
	constructor | FUNCTION body | primaryexp */

	switch (_currentToken->_type)
	{
	case LuaToken::TYPE_NUMBER:
		//init_exp(v, VKNUM, 0);
		//v->u.nval = ls->t.seminfo.r;
		break;
	case LuaToken::TYPE_STRING:
		//codestring(ls, v, ls->t.seminfo.ts);
		break;
	case LuaToken::TYPE_NIL:
		//init_exp(v, VNIL, 0);
		break;
	case LuaToken::TYPE_TRUE:
		//init_exp(v, VTRUE, 0);
		break;
	case LuaToken::TYPE_FALSE:
		//init_exp(v, VFALSE, 0);
		break;
	case LuaToken::TYPE_VARARG:
		/* vararg */
		//FuncState *fs = ls->fs;

		// TODO Add error check
		//check_condition(ls, fs->f->is_vararg,
		//	"cannot use " LUA_QL("...") " outside a vararg function");
		// TODO What does this line achieve?
		//fs->f->is_vararg &= ~VARARG_NEEDSARG;  /* don't need 'arg' */
		//init_exp(v, VVARARG, luaK_codeABC(fs, OP_VARARG, 0, 1, 0));
		break;
	case LuaToken::TYPE_LEFT_CB:
		/* constructor */
		Constructor();
		return;
	case LuaToken::TYPE_FUNCTION:
		NextToken();
		Body(false, _line);
		return;
	default:
		PrimaryExp();
		return;
	}
	NextToken();
}

void LuaParser::Expr()
{
	SubExpr();
}

void LuaParser::SubExpr()
{
	// subexpr -> (simpleexp | unop subexpr) { binop subexpr }
	//enterlevel(ls);
	if (IsUnopr())
	{
		NextToken();
		SubExpr();
	}
	else
		SimpleExp();

	if (IsBinopr())
	{
		NextToken();
		SubExpr();
	}
	//leavelevel(ls);
}

void LuaParser::YIndex()
{
	/* index -> '[' expr ']' */
	NextToken(); /* skip the '[' */
	Expr();
	CheckNext(LuaToken::TYPE_RIGHT_SB);
}

void LuaParser::Field()
{
	/* field -> ['.' | ':'] NAME */
	NextToken();
	Check(LuaToken::TYPE_NAME);
	// TODO add variable somehow
	NextToken();
}

void LuaParser::Constructor()
{
	/* constructor -> ?? */
	const size_t line = _line;

	CheckNext(LuaToken::TYPE_LEFT_CB);
	do
	{
		if (_currentToken->_type == LuaToken::TYPE_RIGHT_CB)
			break;
		
		switch (_currentToken->_type)
		{
		case LuaToken::TYPE_NAME:
			/* may be listfields or recfields */
			if (PeekToken()->_type != LuaToken::TYPE_ASSIGN)  /* expression? */
				ListField();
			else
				RecField();
			break;
		case LuaToken::TYPE_LEFT_SB:
			/* constructor_item -> recfield */
			RecField();
			break;
		default:
			/* constructor_part -> listfield */
			ListField();
			break;
		}
	} while (TestNext(LuaToken::TYPE_COMMA) || TestNext(LuaToken::TYPE_SEMI_COLON));
	CheckMatch(LuaToken::TYPE_RIGHT_CB, LuaToken::TYPE_LEFT_CB, line);
}

void LuaParser::RecField()
{
	/* recfield -> (NAME | `['exp1`]') = exp1 */

	if (_currentToken->_type == LuaToken::TYPE_NAME)
	{
		//luaY_checklimit(fs, cc->nh, MAX_INT, "items in a constructor");
		//checkname(ls, &key);
		NextToken();
	}
	else  /* ls->t.token == '[' */
		YIndex();
	CheckNext(LuaToken::TYPE_ASSIGN);
	Expr();
}

void LuaParser::ListField()
{
	Expr();
	//luaY_checklimit(ls->fs, cc->na, MAX_INT, "items in a constructor");
}
