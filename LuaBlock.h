#pragma once

#include <vector>
#include "LuaVariable.h"
#include "LuaVariableLocation.h"

class LuaBlock
{
	bool _breakable;
	std::vector<LuaLocal> _locals;
	
public:

	LuaBlock(bool breakable)
		: _breakable(breakable)
	{}

	bool IsBreakable() const
	{
		return _breakable;
	}

	void AddLocal(std::string name, const LuaVariableLocation& loc, size_t count)
	{
		_locals.emplace_back(std::move(name), loc, loc._line, count);
	}

	size_t GetLocalCount(const std::string& name) const
	{
		return std::count_if(_locals.begin(), _locals.end(),
			[name](const LuaLocal& local) { return local._name == name; }
		);
	}

	LuaVariable GetLocal(const std::string& name) const
	{
		const auto found = std::find_if(_locals.rbegin(), _locals.rend(),
			[name](const LuaLocal& local) { return local._name == name; }
		);

		if(found != _locals.rend())
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
