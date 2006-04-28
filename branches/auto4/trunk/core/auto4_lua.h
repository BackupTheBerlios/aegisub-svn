// Copyright (c) 2006, Niels Martin Hansen
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Aegisub Group nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// -----------------------------------------------------------------------------
//
// AEGISUB
//
// Website: http://aegisub.cellosoft.com
// Contact: mailto:zeratul@cellosoft.com
//

#pragma once

#ifndef AUTO4_LUA_H
#define AUTO4_LUA_H

#include "auto4_base.h"
#include <wx/thread.h>
#include <lua.h>
#include <lauxlib.h>

namespace Automation4 {

	class LuaPackage;

	// A lua_State with a mutex
	struct LuaState {
		lua_State *L;
		wxMutex m;
		LuaState();
		~LuaState();
	};

	// Implement the Lua scripting engine
	class LuaScript : public Script {
		friend class LuaPackage;
	private:
		LuaState *L;
		std::vector<LuaPackage*> packages;

		void Create(); // load script and create internal structures etc.
		void Destroy(); // destroy internal structures, unreg features and delete environment

	public:
		LuaScript(const wxString &filename);
		virtual ~LuaScript();

		virtual void Reload();
	};

	// A single call to a Lua function, run inside a separate thread.
	// This object should be created on the stack in the function that does the call.
	class LuaThreadedCall : public wxThread {
	private:
		LuaState *L;
		int nargs;
		int nresults;
	protected:
		virtual ExitCode Entry();
	public:
		LuaThreadedCall(LuaState *_L, int _nargs, int _nresults);
	};

	// Base class for packages of C functions that can be installed
	class LuaPackage {
	protected:
		// use the standard luaL_Reg arrays for storing library info, but note
		// that luaL_register() isn't used for the registering. (it doesn't support
		// the namespace stuff i want.)
		LuaPackage(LuaScript *_script, const char *package_name, const luaL_Reg *functions);

		LuaState *L;
		static LuaPackage *GetObject(const char *package_name);

	public:

	};

	class LuaPackageProgress : public LuaPackage {
	private:
		static int f_show(lua_State *L);
		static int f_set(lua_State *L);
		static int f_task(lua_State *L);
		static int f_title(lua_State *L);
		static int f_is_cancelled(lua_State *L);

		wxString task;
		wxString title;
		bool is_cancelled;

		static const char *package_name;
		static const luaL_Reg *functions;
	public:
		LuaPackageProgress(LuaState *L);
	};

};

#endif
