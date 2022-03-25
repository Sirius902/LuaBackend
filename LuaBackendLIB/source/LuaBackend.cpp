#include "LuaBackend.h"
#include "CRC.h"

string _pathStr = "";

LuaBackend::LuaBackend(vector<string> ScriptPaths, uint64_t BaseInput)
{
	frameLimit = 16;
	loadedScripts = vector<LuaScript*>();
	LoadScripts(ScriptPaths, BaseInput);
}

void LuaBackend::LoadScripts(vector<string> ScriptPaths, uint64_t BaseInput)
{
	loadedScripts.clear();

	for (auto scriptsDir : ScriptPaths) {
		if (!filesystem::is_directory(scriptsDir)) {
			continue;
		}

		for (auto _path : filesystem::directory_iterator(scriptsDir))
		{
			LuaScript* _script = new LuaScript();

			_script->luaState.open_libraries
			(
				lib::base,
				lib::package,
				lib::coroutine,
				lib::string,
				lib::os,
				lib::math,
				lib::table,
				lib::io,
				lib::bit32,
				lib::utf8
			);

			_script->luaState.set_exception_handler(&ExceptionHandle);

			SetFunctions(&_script->luaState);

			string _luaPath = scriptsDir;
			_luaPath.append("\\io_packages\\?.lua");

			string _dllPath = scriptsDir;
			_dllPath.append("\\io_packages\\?.dll");

			_script->luaState["package"]["path"] = _luaPath;
			_script->luaState["package"]["cpath"] = _dllPath;

			string _loadPath = scriptsDir;
			_loadPath.append("\\io_load");

			_script->luaState["LOAD_PATH"] = _loadPath;
			_script->luaState["SCRIPT_PATH"] = scriptsDir;
			_script->luaState["CHEATS_PATH"] = "NOT_AVAILABLE";

			string _pathFull = MemoryLib::PName;
			auto _pathExe = _pathFull.substr(_pathFull.find_last_of("\\") + 1);

			_script->luaState["ENGINE_VERSION"] = 5;
			_script->luaState["ENGINE_TYPE"] = "BACKEND";
			_script->luaState["GAME_ID"] = CRC::Calculate(_pathExe.c_str(), _pathExe.length(), CRC::CRC_32());
			_script->luaState["BASE_ADDR"] = BaseInput;

			string _filePath(_path.path().u8string());

			if (_path.path().extension() == ".lua")
			{
				string _luaName = _filePath.substr(_filePath.find_last_of("\\") + 1);
				_script->luaState["LUA_NAME"] = _luaName.substr(0, _luaName.size() - 4);

				ConsoleLib::MessageOutput("Found script: \"" + _filePath + "\" Initializing...\n", 0);

				auto _result = _script->luaState.script_file(_filePath, &sol::script_pass_on_error);

				_script->initFunction = _script->luaState["_OnInit"];
				_script->frameFunction = _script->luaState["_OnFrame"];

				if (_result.valid())
				{
					if (!_script->initFunction && !_script->frameFunction)
					{
						ConsoleLib::MessageOutput("No event handlers exist or all of them have errors.\n", 3);
						ConsoleLib::MessageOutput("Initialization of this script cannot continue...\n", 3);
						return;
					}

					if (!_script->initFunction)
						ConsoleLib::MessageOutput("The event handler for initialization either has errors or does not exist.\n", 2);

					if (!_script->frameFunction)
						ConsoleLib::MessageOutput("The event handler for framedraw either has errors or does not exist.\n", 2);

					ConsoleLib::MessageOutput("Initialization of this script was successful!\n\n", 1);

					loadedScripts.push_back(_script);
				}

				else
				{
					sol::error err = _result;
					ConsoleLib::MessageOutput(err.what() + '\n', 3);
					ConsoleLib::MessageOutput("Initialization of this script was aborted.\n", 3);
				}
			}
		}
	}
}

void LuaBackend::SetFunctions(LuaState* _state)
{
	_state->set_function("ReadByte", MemoryLib::ReadByte);
	_state->set_function("ReadShort", MemoryLib::ReadShort);
	_state->set_function("ReadInt", MemoryLib::ReadInt);
	_state->set_function("ReadLong", MemoryLib::ReadLong);
	_state->set_function("ReadFloat", MemoryLib::ReadFloat);
	_state->set_function("ReadBoolean", MemoryLib::ReadBool);
	_state->set_function("ReadArray", MemoryLib::ReadBytes);
	_state->set_function("ReadString", MemoryLib::ReadString);

	_state->set_function("WriteByte", MemoryLib::WriteByte);
	_state->set_function("WriteShort", MemoryLib::WriteShort);
	_state->set_function("WriteInt", MemoryLib::WriteInt);
	_state->set_function("WriteLong", MemoryLib::WriteLong);
	_state->set_function("WriteFloat", MemoryLib::WriteFloat);
	_state->set_function("WriteBoolean", MemoryLib::WriteBool);
	_state->set_function("WriteArray", MemoryLib::WriteBytes);
	_state->set_function("WriteString", MemoryLib::WriteString);
	_state->set_function("WriteExec", MemoryLib::WriteExec);
	_state->set_function("GetPointer", MemoryLib::GetPointer);

	_state->set_function("ReadByteA", MemoryLib::ReadByteAbsolute);
	_state->set_function("ReadShortA", MemoryLib::ReadShortAbsolute);
	_state->set_function("ReadIntA", MemoryLib::ReadIntAbsolute);
	_state->set_function("ReadLongA", MemoryLib::ReadLongAbsolute);
	_state->set_function("ReadFloatA", MemoryLib::ReadFloatAbsolute);
	_state->set_function("ReadBooleanA", MemoryLib::ReadBoolAbsolute);
	_state->set_function("ReadArrayA", MemoryLib::ReadBytesAbsolute);
	_state->set_function("ReadStringA", MemoryLib::ReadStringAbsolute);

	_state->set_function("WriteByteA", MemoryLib::WriteByteAbsolute);
	_state->set_function("WriteShortA", MemoryLib::WriteShortAbsolute);
	_state->set_function("WriteIntA", MemoryLib::WriteIntAbsolute);
	_state->set_function("WriteLongA", MemoryLib::WriteLongAbsolute);
	_state->set_function("WriteFloatA", MemoryLib::WriteFloatAbsolute);
	_state->set_function("WriteBooleanA", MemoryLib::WriteBoolAbsolute);
	_state->set_function("WriteArrayA", MemoryLib::WriteBytesAbsolute);
	_state->set_function("WriteStringA", MemoryLib::WriteStringAbsolute);
	_state->set_function("GetPointerA", MemoryLib::GetPointerAbsolute);

	_state->set_function("InitializeRPC", DCInstance::InitializeRPC);
	_state->set_function("UpdateDetails", DCInstance::UpdateDetails);
	_state->set_function("UpdateState", DCInstance::UpdateState);
	_state->set_function("UpdateLImage", DCInstance::UpdateLImage);
	_state->set_function("UpdateSImage", DCInstance::UpdateSImage);

	_state->set_function("ULShift32", Operator32Lib::UnsignedShift32);

	_state->set_function("ConsolePrint", 
		sol::overload(
			[_state](sol::object Text) 
			{
				HANDLE _hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

				SetConsoleTextAttribute(_hConsole, 14);
				cout << "[" + _state->globals()["LUA_NAME"].get<string>() + "] ";

				SetConsoleTextAttribute(_hConsole, 7);
				cout << Text.as<string>() << '\n';
			}, 

			[_state](sol::object Text, int MessageType) 
			{
				HANDLE _hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

				SetConsoleTextAttribute(_hConsole, 14);
				cout << "[" + _state->globals()["LUA_NAME"].get<string>() + "] ";

				switch (MessageType)
				{
				case 0:
					SetConsoleTextAttribute(_hConsole, 11);
					cout << "MESSAGE: ";
					break;
				case 1:
					SetConsoleTextAttribute(_hConsole, 10);
					cout << "SUCCESS: ";
					break;
				case 2:
					SetConsoleTextAttribute(_hConsole, 14);
					cout << "WARNING: ";
					break;
				case 3:
					SetConsoleTextAttribute(_hConsole, 12);
					cout << "ERROR: ";
					break;
				}

				SetConsoleTextAttribute(_hConsole, 7);
				cout << Text.as<string>() << '\n';
			}
		)
	);

	_state->set_function("GetHertz",
		[this]()
		{
			switch (frameLimit)
			{
			default:
				return 60;
			case 8:
				return 120;
			case 4:
				return 240;
			}
		});

	_state->set_function("SetHertz",
		[this](int Input)
		{
			switch (Input)
			{
			default:
				frameLimit = 16;
				break;
			case 120:
				frameLimit = 8;
				break;
			case 240:
				frameLimit = 4;
				break;
			}
		});

}
