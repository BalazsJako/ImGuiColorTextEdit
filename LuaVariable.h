#pragma once

#include <string>
#include <variant>
#include "LuaVariableLocation.h"

struct LuaGlobal
{
	LuaGlobal(std::string name, const LuaVariableLocation& loc)
		: _name(std::move(name)), _location(loc)
	{}

	std::string _name;
	LuaVariableLocation _location;
};

struct LuaLocal
{
	LuaLocal(std::string name, const LuaVariableLocation& loc, size_t lineDefined, size_t count)
		: _name(std::move(name)), _location(loc), _count(count), _lineDefined(lineDefined), _lastLineDefined(0)
	{}

	void SetLastLineDefined(size_t lastLineDefined)
	{
		_lastLineDefined = lastLineDefined;
	}

	std::string _name;
	LuaVariableLocation _location;
	size_t _count;
	size_t _lineDefined;
	size_t _lastLineDefined;
};

struct LuaUpvalue
{
	LuaUpvalue(std::string name, const LuaVariableLocation& loc, size_t lineDefined)
		: _name(std::move(name)), _location(loc), _lineDefined(lineDefined), _lastLineDefined(0)
	{}

	void SetLastLineDefined(size_t lastLineDefined)
	{
		_lastLineDefined = lastLineDefined;
	}

	std::string _name;
	LuaVariableLocation _location;
	size_t _lineDefined;
	size_t _lastLineDefined;
};

typedef std::variant<std::monostate, LuaGlobal, LuaLocal, LuaUpvalue> LuaVariable;
