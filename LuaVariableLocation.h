#pragma once

struct LuaVariableLocation
{
	LuaVariableLocation(size_t line, size_t startCol, size_t endCol)
		: _line(line), _startCol(startCol), _endCol(endCol)
	{}

	size_t _line;
	size_t _startCol;
	size_t _endCol;
};
