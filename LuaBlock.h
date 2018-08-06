#pragma once

#include <vector>
#include "LuaVariable.h"
#include "LuaVariableLocation.h"

class LuaBlock
{
	bool _breakable;
	// All local declarations in the blocks scope
	std::vector<LuaLocal> _localDefinitions;
	// All locals  in the blocks scope
	std::vector<LuaLocal> _locals;
	
public:

	LuaBlock(bool breakable)
		: _breakable(breakable)
	{}

	bool IsBreakable() const
	{
		return _breakable;
	}

	void AddLocalDefinition(const std::string& name, const LuaVariableLocation& loc, size_t count)
	{
		_localDefinitions.emplace_back(name, loc, loc._line, count);
		_locals.emplace_back(name, loc, loc._line, count);
	}

	void AddLocal(LuaLocal&& local)
	{
		_locals.emplace_back(local);
	}

	size_t GetLocalCount(const std::string& name) const
	{
		return std::count_if(_localDefinitions.begin(), _localDefinitions.end(),
			[name](const LuaLocal& local) { return local._name == name; }
		);
	}

	LuaVariable GetLocal(const std::string& name) const
	{
		const auto found = std::find_if(_localDefinitions.rbegin(), _localDefinitions.rend(),
			[name](const LuaLocal& local) { return local._name == name; }
		);

		if(found != _localDefinitions.rend())
			return *found;

		return LuaVariable();
	}

	const std::vector<LuaLocal>& Close(size_t lastLineDefined)
	{
		for (auto& local : _locals)
			local.SetLastLineDefined(lastLineDefined);
		return _locals;
	}
};
