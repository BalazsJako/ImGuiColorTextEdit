#pragma once
// This parser is influenced by the structure of lua5.1.5 lparser.c

#include "TextEditor.h"
#include "LuaFunctionState.h"

class LuaSymbolTable;

class LuaParser
{
	class LuaSyntaxException : public std::exception
	{};

	// Used to add variable data to the text
	TextEditor* _textEditor;

	TextEditor::Lines& _lines;
	size_t _line;
	size_t _col;

	LuaToken* _currentToken;

	std::list<LuaFunctionState> _functionStates;
	LuaFunctionState* _currentFunctionState;

	std::vector<LuaVariable> _variables;
	// TODO main chunk is always vararg

	std::string _errorMsg;
	size_t _errorLine;

public:
	LuaParser(TextEditor::Lines& lines, TextEditor* textEditor)
		: _textEditor(textEditor), _lines(lines), _line(0), _col(0), _currentToken(nullptr),
		  _currentFunctionState(nullptr), _errorLine(0)
	{}

	bool ParseAll();

	std::pair<std::string, size_t> GetError();

private:

	const std::vector<LuaVariable>& GetVariables() const;

	void NextToken();
	LuaToken* PeekToken() const;

	bool BlockFollow() const;

	bool TestNext(LuaToken::Type type); // DONE
	void CheckMatch(LuaToken::Type what, LuaToken::Type who, size_t where);
	void SyntaxError(const std::string& msg);
	void ErrorExpected(LuaToken::Type token);

	void Check(LuaToken::Type type);
	void CheckNext(LuaToken::Type type);

	void Chunk(); // DONE
	void Block();

	void EnterBlock(bool breakable = false);
	void LeaveBlock(size_t lastLineDefined);

	void OpenFunction(size_t lineDefined);
	void CloseFunction(size_t lastLineDefined);

	// Return true if needs self
	bool FuncName();
	void FuncArgs();

	void ExpList1();
	void Body(bool needSelf, size_t line);
	void ParList();

	// Returns true if this is a last statement
	bool Statement(); // DONE
	void IfStat(size_t line); // DONE
	void WhileStat(size_t line);
	void ForStat(size_t line);
	void RepeatStat(size_t line);
	void FuncStat(size_t line);
	void LocalFunction();
	void LocalStat();
	void RetStat();
	void BreakStat();
	void ExprStat();

	void TestThenBlock(); // DONE

	void Cond();

	void ForBody();
	void ForNum(size_t line);
	void ForList();

	void Assignment(int nVars);

	bool IsUnopr() const; // DONE
	bool IsBinopr() const; // DONE
	void PrefixExp(); // DONE
	// Return true if it's a function call
	bool PrimaryExp(); // DONE ISH
	void SimpleExp(); // DONE ISH
	void Expr(); //DONE
	void SubExpr(); // DONE

	void YIndex(); // DONE
	
	void Field(); // DONE

	void Constructor();
	void RecField();
	void ListField();
};
