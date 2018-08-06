#pragma once

#include <vector>
#include "LuaBlock.h"
#include "LuaVariable.h"
#include <numeric>
#include <any>

class LuaFunctionState
{
	size_t _lineDefined;
	LuaFunctionState* _prevFunc = nullptr;
	bool _isVararg;

	// All local declarations in the functions scope
	std::vector<LuaLocal> _localDefinitions;
	// All locals in the functions scope
	std::vector<LuaLocal> _locals;
	// All usages of upvalues for this function
	std::vector<LuaUpvalue> _upvalues;
	// Names of all used upvalues
	std::vector<std::string> _upvalueNames;

	// Active blocks
	std::vector<LuaBlock> _blocks;

	// All references to a variable, in the function scope.
	std::vector<LuaVariable> _variables;

public:
	LuaFunctionState(size_t lineDefined, LuaFunctionState* prevFunc = nullptr)
		: _lineDefined(lineDefined), _prevFunc(prevFunc), _isVararg(false)
	{}

	void SetVararg()
	{
		_isVararg = true;
	}

	bool IsVararg() const
	{
		return _isVararg;
	}

	bool IsBreakable() const
	{
		return std::any_of(_blocks.rbegin(), _blocks.rend(), 
			[](const LuaBlock& block){ return block.IsBreakable(); }
		);
	}

	void OpenBlock(bool breakable)
	{
		_blocks.emplace_back(breakable);
	}

	void CloseBlock(size_t lastLineDefined)
	{
		for (const auto& local : (*_blocks.rbegin()).Close(lastLineDefined))
			_variables.emplace_back(local);
		_blocks.pop_back();
	}

	const std::vector<LuaVariable>& Close(size_t endLine)
	{
		// Close scope of all locals and add them to the variables
		for (auto& local : _locals)
		{
			local.SetLastLineDefined(endLine);
			_variables.emplace_back(local);
		}
		_locals.clear();

		// Close scope of all upvalues and add them to the variables
		for (auto& upvalue : _upvalues)
		{
			upvalue.SetLastLineDefined(endLine);
			_variables.emplace_back(upvalue);
		}
		_upvalues.clear();

		return _variables;
	}

	void AddLocal(const std::string& name, const LuaVariableLocation& loc)
	{
		// TODO Verify std function
		auto count = std::count_if(_localDefinitions.begin(), _localDefinitions.end(),
			[name](LuaLocal& local) { return local._name == name; }
		);

		if(_blocks.empty())
		{
			_localDefinitions.emplace_back(name, loc, loc._line, count);
			_locals.emplace_back(name, loc, loc._line, count);
		}
		else
		{
			// TODO Verify these std functions
			count = std::accumulate(_blocks.begin(), _blocks.end(), count,
				[name](size_t acc, const LuaBlock& block) { return acc + block.GetLocalCount(name); }
			);

			(*_blocks.rbegin()).AddLocalDefinition(name, loc, count);
		}
	}

	// Returns the closest value with this name.
	// Turns locals outside the function into upvalues
	LuaVariable Variable(const std::string& name, const LuaVariableLocation& loc, bool currentFunction = true)
	{
		// Check if local in a block
		for(auto iter = _blocks.rbegin(); iter != _blocks.rend(); ++iter)
		{
			auto foundLocal = iter->GetLocal(name);
			if(std::holds_alternative<LuaLocal>(foundLocal))
			{
				auto local = std::get<LuaLocal>(foundLocal);
				if (currentFunction)
				{
					_blocks.rbegin()->AddLocal(LuaLocal(name, loc, local._lineDefined, local._count));
				}

				return local;
			}
		}

		// Check if local in function scope
		const auto foundLocal = std::find_if(_localDefinitions.rbegin(), _localDefinitions.rend(),
			[name](const LuaLocal& local) { return local._name == name; }
		);
		if (foundLocal != _localDefinitions.rend())
		{
			if (currentFunction)
				_locals.emplace_back(LuaLocal(name, loc, foundLocal->_lineDefined, foundLocal->_count));
			return *foundLocal;
		}

		// Check if upvalue
		const auto foundUpvalue = std::find_if(_upvalueNames.rbegin(), _upvalueNames.rend(),
			[name](const std::string& upvalueName) { return upvalueName == name; }
		);
		if (foundUpvalue != _upvalueNames.rend())
		{
			auto upvalue = LuaUpvalue(name, loc, _lineDefined);
			if (currentFunction)
				_variables.emplace_back(upvalue);
			return upvalue;
		}

		// Not a variable in this scope. Look at previous function 
		if(_prevFunc)
		{
			const auto res = _prevFunc->Variable(name, loc, false);

			// Upvalue or local in previous function?
			if(std::holds_alternative<LuaUpvalue>(res) || std::holds_alternative<LuaLocal>(res))
			{
				// Make into upvalue for this function
				_upvalueNames.push_back(name);

				auto upvalue = LuaUpvalue(name, loc, _lineDefined);
				if (currentFunction)
					_upvalues.push_back(upvalue);
				return upvalue;
			} 
		}

		// Was a global
		auto global = LuaGlobal(name, loc);
		if(currentFunction)
			_variables.emplace_back(global);
		return global;
	}
};
