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
#include <wx/event.h>
#include <lua.h>
#include <lauxlib.h>

namespace Automation4 {

	class LuaAssFile {
	private:
		AssFile *ass;
		lua_State *L;

		// keep a cursor of last accessed item to avoid walking over the entire file on every access
		std::list<AssEntry*>::iterator last_entry_ptr;
		int last_entry_id;
		void GetAssEntry(int n); // set last_entry_ptr to point to item n

		void AssEntryToLua(AssEntry *e); // makes a Lua representation of AssEntry and places on the top of the stack
		AssEntry *LuaToAssEntry(); // assumes a Lua representation of AssEntry on the top of the stack, and creates an AssEntry object of it

		static int ObjectIndexRead(lua_State *L);
		static int ObjectIndexWrite(lua_State *L);
		static int ObjectGetLen(lua_State *L);
		static int ObjectDelete(lua_State *L);
		static int ObjectDeleteRange(lua_State *L);
		static int ObjectAppend(lua_State *L);
		static int ObjectInsert(lua_State *L);
		static int ObjectGarbageCollect(lua_State *L);

		static LuaAssFile *GetObjPointer(lua_State *L, int idx);

		~LuaAssFile();
	public:
		LuaAssFile(lua_State *_L, AssFile *_ass);
	};

	class LuaProgressSink : public ProgressSink {
	private:
		lua_State *L;

		static int LuaSetProgress(lua_State *L);
		static int LuaSetTask(lua_State *L);
		static int LuaSetTitle(lua_State *L);
		static int LuaGetCancelled(lua_State *L);
		static int LuaDebugOut(lua_State *L);

	public:
		LuaProgressSink(lua_State *L);
	};

	class LuaFeature : public virtual Feature {
	protected:
		lua_State *L;
		int myid;

		LuaFeature(lua_State *_L, ScriptFeatureClass _featureclass, wxString &_name);

		void RegisterFeature();

		void GetFeatureFunction(int functionid);
		void CreateIntegerArray(std::vector<int> &ints);
		void ThrowError();
	};

	class LuaScript : public Script {
		friend class LuaFeature;

	private:
		lua_State *L;

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
		lua_State *L;
		wxEvtHandler *evthandler;
		int nargs;
		int nresults;
	public:
		LuaThreadedCall(lua_State *_L, int _nargs, int _nresults);
		virtual ExitCode Entry();
	};

	class LuaFeatureMacro : public FeatureMacro, LuaFeature {
	private:
		bool no_validate;
	protected:
		LuaFeatureMacro(wxString &_name, wxString &_description, MacroMenu _menu, lua_State *_L);
	public:
		static int LuaRegister(lua_State *L);
		virtual ~LuaFeatureMacro() { }

		virtual bool Validate(AssFile *subs, std::vector<int> &selected, int active);
		virtual void Process(AssFile *subs, std::vector<int> &selected, int active);
	};

};

#endif
