#ifndef LUABACKEND
#define LUABACKEND

#include <iostream>
#include <filesystem>
#include <sol/sol.hpp>

#include <MemoryLib.h>
#include <ConsoleLib.h>
#include <Operator32Lib.h>
#include <DCInstance.h>

using namespace sol;
using namespace std;

using LuaState = sol::state;
using LuaFunction = sol::safe_function;

class LuaBackend
{
	public:
		struct LuaScript
		{
			LuaState luaState;
			LuaFunction initFunction;
			LuaFunction frameFunction;
		};

		std::vector<LuaScript*> loadedScripts;
		int frameLimit;

		static int ExceptionHandle(lua_State* luaState, sol::optional<const std::exception&> thrownException, sol::string_view description)
		{
			const std::exception _ex = *thrownException;
			ConsoleLib::MessageOutput(_ex.what() + '\n', 3);

			return sol::stack::push(luaState, _ex.what());
		}

		void SetFunctions(LuaState*);
		void LoadScripts(vector<string>, uint64_t);
		LuaBackend(vector<string>, uint64_t);
};

#endif