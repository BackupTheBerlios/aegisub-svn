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

#include "auto4_lua.h"
#include <lualib.h>
#include <lauxlib.h>

namespace Automation4 {

	LuaState::LuaState()
	{
		L = lua_open();
		if (!L)
			throw _T("Failed to create Lua environment.");
	}

	LuaState::~LuaState()
	{
		// hmm... make sure the interpreter is actually unused before closing?
		// not sure if this is good...
		m.Lock();
		lua_close(L);
	}

	LuaScript::LuaScript(const wxString &filename)
		: Script(filename)
		, L(0)
	{
		Create();
	}

	LuaScript::~LuaScript()
	{
		if (L) Destroy();
	}

	void LuaScript::Create()
	{
		Destroy();

		loaded = true;

		try {
			// create lua environment
			L = new LuaState();

			// register standard libs
			luaopen_base(L->L);
			luaopen_package(L->L);
			luaopen_string(L->L);
			luaopen_table(L->L);
			luaopen_math(L->L);
			// FIXME!
			// unregister dofile() and loadfile() and provide alternative,
			// or hack those two functions instead?

			// TODO: register automation libs
			// TODO: load user script

		}
		catch (const wchar_t *e) {
			Destroy();
			loaded = false;
			throw e;
		}
	}

	void LuaScript::Destroy()
	{
		// Assume the script object is clean if there's no Lua state
		if (!L) return;

		// remove features
		for (std::vector<Feature*>::iterator i = features.begin(); i != features.end(); i++) {
			delete *i;
		}
		features.clear();

		// delete environment
		lua_close(L->L);
		L = 0;

		loaded = false;
	}

	void LuaScript::Reload()
	{
		Destroy();
		Create();
	}

	LuaThreadedCall::LuaThreadedCall(LuaState *_L, int _nargs, int _nresults)
		: wxThread(wxTHREAD_JOINABLE)
		, L(_L)
		, nargs(_nargs)
		, nresults(_nresults)
	{
		Create();
		Run();
	}

	wxThread::ExitCode LuaThreadedCall::Entry()
	{
		// make sure we have the mutex while lua is running
		wxMutexLocker lock(L->m);
		lua_call(L->L, nargs, nresults);
		return 0;
	}

};
