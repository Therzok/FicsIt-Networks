#include "LuaFileSystemAPI.h"

#include "LuaInstance.h"

#include "FicsItKernel/FicsItFS/FileSystem.h"
#include "FicsItKernel/FicsItFS/FINFileSystemState.h"

#define LuaFunc(funcName) \
int funcName(lua_State* L) { \
	KernelSystem* kernel = *(KernelSystem**)lua_touserdata(L, lua_upvalueindex(1)); \
	FicsItFS::Root* self = kernel->getFileSystem(); \
	if (!self) return luaL_error(L, "component is invalid");

#define LuaFileFuncName(funcName) luaFile ## funcName
#define LuaFileFunc(funcName) \
int LuaFileFuncName(funcName) (lua_State* L) { \
	auto self = (LuaFile*)luaL_checkudata(L, 1, "File"); \
	if (!self) return luaL_error(L, "file is invalid"); \
	auto file = self->file; \
	if (!file) return luaL_error(L, "file is invalid");

#define CatchExceptionLua \
	catch (const std::exception& ex) { \
		return luaL_error(L, ex.what()); \
	}

namespace FicsItKernel {
	namespace Lua {
		LuaFunc(makeFileSystem)
			std::string type = luaL_checkstring(L, 1);
			std::string name = luaL_checkstring(L, 2);
			FileSystem::SRef<FileSystem::Device> device;
			if (type == "tmpfs") {
				device = new FileSystem::MemDevice();
			} else return luaL_argerror(L, 1, "No valid FileSystem Type");
			if (!device.isValid()) luaL_argerror(L, 1, "Invalid Device");
			FileSystem::SRef<FicsItFS::DevDevice> dev = self->getDevDevice();
			lua_pushboolean(L, dev.isValid() ? dev->addDevice(device, name) : false);
			return 1;
		}

		LuaFunc(initFileSystem)
			std::string path = luaL_checkstring(L, 1);
			lua_pushboolean(L, kernel->initFileSystem(path));
			return 1;
		}

		LuaFunc(open)
			std::string mode = "r";
			if (lua_isstring(L, 2)) mode = lua_tostring(L, 2);
			FileSystem::FileMode m;
			if (mode == "r") m = FileSystem::READ;
			else if (mode == "w") m = FileSystem::WRITE;
			else if (mode == "a") m = FileSystem::APPEND;
			else if (mode == "+r") m = FileSystem::UPDATE_READ;
			else if (mode == "+w") m = FileSystem::UPDATE_WRITE;
			else if (mode == "+a") m = FileSystem::UPDATE_APPEND;
			else luaL_argerror(L, 2, "is not valid file mode");
			try {
				luaFile(L, self->open(luaL_checkstring(L, 1), m));
			} CatchExceptionLua
			return 1;
		}

		LuaFunc(createDir)
			auto path = luaL_checkstring(L, 1);
			auto all = lua_toboolean(L, 2);
			try {
				self->createDir(path, all);
			} CatchExceptionLua
			return 0;
		}

		LuaFunc(remove)
			auto path = luaL_checkstring(L, 1);
			bool all = lua_toboolean(L, 2);
			try {
				self->remove(path, all);
			} CatchExceptionLua
			return 0;
		}

		LuaFunc(move)
			auto from = luaL_checkstring(L, 1);
			auto to = luaL_checkstring(L, 2);
			try {
				lua_pushboolean(L, (bool)self->move(from, to));
			} CatchExceptionLua
			return 1;
		}

		LuaFunc(exists)
			auto path = luaL_checkstring(L, 1);
			try {
				lua_pushboolean(L, self->get(path).isValid());
			} CatchExceptionLua
			return 1;
		}

		LuaFunc(childs)
			auto path = luaL_checkstring(L, 1);
			std::unordered_set<std::string> childs;
			try {
				childs = self->childs(path);
			} CatchExceptionLua
			lua_newtable(L);
			int i = 1;
			for (auto& child : childs) {
				lua_pushstring(L, child.c_str());
				lua_seti(L, -2, i++);
			}
			return 1;
		}

		LuaFunc(isFile)
			auto path = luaL_checkstring(L, 1);
			try {
				lua_pushboolean(L, !!dynamic_cast<FileSystem::File*>(self->get(path).get()));
			} CatchExceptionLua
			return 1;
		}

		LuaFunc(isDir)
			auto path = luaL_checkstring(L, 1);
			try {
				lua_pushboolean(L, !!dynamic_cast<FileSystem::Directory*>(self->get(path).get()));
			} CatchExceptionLua
			return 1;
		}

		LuaFunc(mount)
			auto devPath = luaL_checkstring(L, 1);
			auto mountPath = luaL_checkstring(L, 2);
			try {
				lua_pushboolean(L, FileSystem::DeviceNode::mount(*self, devPath, mountPath));
			} CatchExceptionLua
			return 1;
		}

		LuaFunc(unmount)
			auto mountPath = luaL_checkstring(L, 1);
			try {
				lua_pushboolean(L, self->unmount(mountPath));
			} CatchExceptionLua
			return 1;
		}

		LuaFunc(doFile)
			FileSystem::SRef<FileSystem::File> p;
			try {
				p = self->get(luaL_checkstring(L, 1));
			} CatchExceptionLua
			if (!p.isValid()) return luaL_argerror(L, 1, "path is no valid file");
			FileSystem::SRef<FileSystem::FileStream> s;
			try {
				s = p->open(FileSystem::READ);
			} CatchExceptionLua
			if (!s.isValid()) return luaL_error(L, "not able to create filestream");
			int n = lua_gettop(L);
			std::string code;
			try {
				code = s->readAll();
			} CatchExceptionLua
			luaL_dofile(L, code.c_str());
			try {
				s->close();
			} CatchExceptionLua
			return lua_gettop(L) - n;
		}

		LuaFunc(loadFile)
			FileSystem::SRef<FileSystem::File> p;
			try {
				p = self->get(luaL_checkstring(L, 1));
			} CatchExceptionLua
			if (!p.isValid()) return luaL_argerror(L, 1, "path is no valid file");
			FileSystem::SRef<FileSystem::FileStream> s;
			try {
				s = p->open(FileSystem::READ);
			} CatchExceptionLua
			if (!s.isValid()) return luaL_error(L, "not able to create filestream");
			std::string code;
			try {
				code = s->readAll();
			} CatchExceptionLua
			luaL_loadfile(L, code.c_str());
			try {
				s->close();
			} CatchExceptionLua
			return 1;
		}

		static const luaL_Reg luaFileSystemLib[] = {
			{"makeFileSystem", makeFileSystem},
			{"initFileSystem", initFileSystem},
			{"open", open},
			{"createDir", createDir},
			{"remove", remove},
			{"move", move},
			{"exists", exists},
			{"childs", childs},
			{"isFile", isFile},
			{"isDir", isDir},
			{"mount", mount},
			{"unmount", unmount},
			{"doFile", doFile},
			{"loadFile", loadFile},
			{NULL,NULL}
		};

		LuaFileFunc(Close)
			try {
				file->close();
			} CatchExceptionLua
			return 0;
		}

		LuaFileFunc(Write)
			auto s = lua_gettop(L);
			for (int i = 2; i <= s; ++i) {
				std::string str = luaL_checkstring(L, i);
				try {
					file->write(str);
				} CatchExceptionLua
			}
			return 0;
		}

		LuaFileFunc(Flush)
			try {
				file->flush();
			} CatchExceptionLua
			return 0;
		}

		LuaFileFunc(Read)
			auto args = lua_gettop(L);
			for (int i = 2; i <= args; ++i) {
				bool invalidFormat = false;
				try {
					if (lua_isnumber(L, i)) {
						if (file->isEOF()) lua_pushnil(L);
						auto n = lua_tointeger(L, i);
						lua_pushstring(L, file->readChars(n).c_str());
					} else {
						char fo = 'l';
						if (lua_isstring(L, i)) {
							std::string s = lua_tostring(L, i);
							if (s.size() != 2 || s[0] != '*') fo = '\0';
							fo = s[1];
						}
						switch (fo) {
						case 'n':
						{
							lua_pushnumber(L, file->readNumber());
							break;
						} case 'a':
						{
							lua_pushstring(L, file->readAll().c_str());
							break;
						} case 'l':
						{
							if (!file->isEOF()) lua_pushstring(L, file->readLine().c_str());
							else lua_pushnil(L);
							break;
						}
						default:
							invalidFormat = true;
						}
					}
				} catch (const std::exception& ex) {
					luaL_error(L, ex.what());
				}
				if (invalidFormat) return luaL_argerror(L, i, "no valid format");
			}
			return args - 1;
		}

		LuaFileFunc(ReadLine)
			try {
				if (file->isEOF()) lua_pushnil(L);
				else lua_pushstring(L, file->readLine().c_str());
			} CatchExceptionLua
			return 1;
		}

		int luaFileLines(lua_State * L) {
			auto f = (LuaFile*)luaL_checkudata(L, 1, "File");
			lua_pushcclosure(L, luaFileReadLine, 1);
			return 1;
		}

		int luaFileSeek(lua_State * L) {
			auto f = (LuaFile*)luaL_checkudata(L, 1, "File");
			std::string w = "cur";
			std::int64_t off = 0;
			if (lua_isstring(L, 2)) w = lua_tostring(L, 2);
			if (lua_isinteger(L, 3)) off = lua_tointeger(L, 3);
			int64_t seek;
			try {
				seek = f->file->seek(w, off);
			} CatchExceptionLua
			lua_pushinteger(L, seek);
			return 1;
		}

		int luaFileString(lua_State * L) {
			auto f = (LuaFile*)luaL_checkudata(L, 1, "File");
			std::string text;
			try {
				text = f->file->readAll();
			} CatchExceptionLua
			lua_pushstring(L, text.c_str());
			return 1;
		}

		int luaFileGC(lua_State * L) {
			auto f = (LuaFile*)luaL_checkudata(L, 1, "File");
			try {
				f->file->close();
			} CatchExceptionLua
			f->~LuaFile();
			return 0;
		}

		static const luaL_Reg luaFileLib[] = {
			{"close", luaFileClose},
			{"write", luaFileWrite},
			{"flush", luaFileFlush},
			{"read", luaFileRead},
			{"lines", luaFileLines},
			{"seek", luaFileSeek},
			{"__tostring", luaFileString},
			{"__gc", luaFileGC},
			{NULL, NULL}
		};

		void luaFile(lua_State* L, FileSystem::SRef<FileSystem::FileStream> file) {
			if (!file.isValid()) {
				lua_pushnil(L);
				return;
			}
			auto f = (LuaFile*)lua_newuserdata(L, sizeof(LuaFile));
			luaL_setmetatable(L, "File");
			new (f) LuaFile{std::move(file)};
		}

		void setupFileSystemAPI(KernelSystem* kernel, lua_State * L) {
			luaL_newlibtable(L, luaFileSystemLib);
			auto& fs_ud = *(KernelSystem**)lua_newuserdata(L, sizeof(void*));
			fs_ud = kernel;
			luaL_setfuncs(L, luaFileSystemLib, 1);
			lua_setglobal(L, "filesystem");

			luaL_newmetatable(L, "File");
			lua_pushvalue(L, -1);
			lua_setfield(L, -2, "__index");
			luaL_setfuncs(L, luaFileLib, 0);
			lua_pop(L, 1);
		}
	}
}