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
#include "ass_dialogue.h"
#include "ass_style.h"
#include "ass_file.h"
#include <lualib.h>
#include <lauxlib.h>
#include <wx/msgdlg.h>
#include <assert.h>
#include <algorithm>

namespace Automation4 {

	class LuaStackcheck {
	private:
		lua_State *L;
		int startstack;
	public:
		void check(int additional)
		{
			int top = lua_gettop(L);
			if (top - additional != startstack) {
				wxLogDebug(_T("Lua stack size mismatch. Dumping..."));
				for (int i = top; i > 0; i--) {
					wxString type(lua_typename(L, lua_type(L, i)), wxConvUTF8);
					if (lua_isstring(L, i)) {
						wxLogDebug(type + _T(": ") + wxString(lua_tostring(L, i), wxConvUTF8));
					} else {
						wxLogDebug(type);
					}
				}
				assert(false);
			}
		}
		LuaStackcheck(lua_State *_L) : L(_L) { startstack = lua_gettop(L); }
		~LuaStackcheck() { check(0); }
	};

	int lua_callable_showmessage(lua_State *L)
	{
		wxString msg(lua_tostring(L, 1), wxConvUTF8);
		wxMutexGuiLocker gui;
		wxMessageBox(msg);
		lua_pop(L, 1);
		return 0;
	}

	int lua_callable_set_undo_point(lua_State *L)
	{
		wxString description;
		if (lua_isstring(L, 1)) {
			description = wxString(lua_tostring(L, 1), wxConvUTF8);
			lua_pop(L, 1);
		}
		AssFile::FlagAsModified(description);
		return 0;
	}


	void LuaAssFile::AssEntryToLua(AssEntry *e)
	{
		lua_newtable(L);

		wxString section(e->group);
		lua_pushstring(L, section.mb_str(wxConvUTF8));
		lua_setfield(L, -2, "section");

		wxString raw(e->GetEntryData());
		lua_pushstring(L, raw.mb_str(wxConvUTF8));
		lua_setfield(L, -2, "raw");

		if (raw.Trim().IsEmpty()) {
			lua_pushstring(L, "clear");

		} else if (raw[0] == _T(';')) {
			// "text" field, same as "raw" but with semicolon stripped
			wxString text(raw, 1, raw.size()-1);
			lua_pushstring(L, text.mb_str(wxConvUTF8));
			lua_setfield(L, -2, "text");

			lua_pushstring(L, "comment");

		} else if (raw[0] == _T('[')) {
			lua_pushstring(L, "head");

		} else if (section.Lower() == _T("[script info]")) {
			// assumed "info" class

			// first "key"
			wxString key = raw.BeforeFirst(_T(':'));
			lua_pushstring(L, key.mb_str(wxConvUTF8));
			lua_setfield(L, -2, "key");

			// then "value"
			wxString value = raw.AfterFirst(_T(':'));
			lua_pushstring(L, value.mb_str(wxConvUTF8));
			lua_setfield(L, -2, "value");

			lua_pushstring(L, "info");

		} else if (raw.Left(7).Lower() == _T("format:")) {

			// TODO: parse the format line; just use a tokenizer

			lua_pushstring(L, "format");

		} else if (e->GetType() == ENTRY_DIALOGUE) {
			AssDialogue *dia = e->GetAsDialogue(e);

			lua_pushboolean(L, (int)dia->Comment);
			lua_setfield(L, -2, "comment");

			lua_pushnumber(L, dia->Layer);
			lua_setfield(L, -2, "layer");

			lua_pushnumber(L, dia->Start.GetMS());
			lua_setfield(L, -2, "start_time");
			lua_pushnumber(L, dia->End.GetMS());
			lua_setfield(L, -2, "end_time");

			lua_pushstring(L, dia->Style.mb_str(wxConvUTF8));
			lua_setfield(L, -2, "style");
			lua_pushstring(L, dia->Actor.mb_str(wxConvUTF8));
			lua_setfield(L, -2, "actor");

			lua_pushnumber(L, dia->MarginL);
			lua_setfield(L, -2, "margin_l");
			lua_pushnumber(L, dia->MarginR);
			lua_setfield(L, -2, "margin_r");
			lua_pushnumber(L, dia->MarginV); // duplicating MarginV to margin_t and margin_b here
			lua_setfield(L, -2, "margin_t");
			lua_pushnumber(L, dia->MarginV);
			lua_setfield(L, -2, "margin_b");

			lua_pushstring(L, dia->Effect.mb_str(wxConvUTF8));
			lua_setfield(L, -2, "effect");

			lua_pushstring(L, ""); // tentative AS5 field
			lua_setfield(L, -2, "userdata");

			lua_pushstring(L, dia->Text.mb_str(wxConvUTF8));
			lua_setfield(L, -2, "text");

			lua_pushstring(L, "dialogue");

		} else if (e->GetType() == ENTRY_STYLE) {
			AssStyle *sty = e->GetAsStyle(e);

			lua_pushstring(L, sty->name.mb_str(wxConvUTF8));
			lua_setfield(L, -2, "name");

			lua_pushstring(L, sty->font.mb_str(wxConvUTF8));
			lua_setfield(L, -2, "fontname");
			lua_pushnumber(L, sty->fontsize);
			lua_setfield(L, -2, "fontsize");

			lua_pushstring(L, sty->primary.GetASSFormatted(true).mb_str(wxConvUTF8));
			lua_setfield(L, -2, "color1");
			lua_pushstring(L, sty->secondary.GetASSFormatted(true).mb_str(wxConvUTF8));
			lua_setfield(L, -2, "color2");
			lua_pushstring(L, sty->outline.GetASSFormatted(true).mb_str(wxConvUTF8));
			lua_setfield(L, -2, "color3");
			lua_pushstring(L, sty->shadow.GetASSFormatted(true).mb_str(wxConvUTF8));
			lua_setfield(L, -2, "color4");

			lua_pushboolean(L, (int)sty->bold);
			lua_setfield(L, -2, "bold");
			lua_pushboolean(L, (int)sty->italic);
			lua_setfield(L, -2, "italic");
			lua_pushboolean(L, (int)sty->underline);
			lua_setfield(L, -2, "underline");
			lua_pushboolean(L, (int)sty->strikeout);
			lua_setfield(L, -2, "strikeout");

			lua_pushnumber(L, sty->scalex);
			lua_setfield(L, -2, "scale_x");
			lua_pushnumber(L, sty->scaley);
			lua_setfield(L, -2, "scale_y");

			lua_pushnumber(L, sty->spacing);
			lua_setfield(L, -2, "spacing");

			lua_pushnumber(L, sty->angle);
			lua_setfield(L, -2, "angle");

			lua_pushnumber(L, sty->borderstyle);
			lua_setfield(L, -2, "borderstyle");
			lua_pushnumber(L, sty->outline_w);
			lua_setfield(L, -2, "outline");
			lua_pushnumber(L, sty->shadow_w);
			lua_setfield(L, -2, "shadow");

			lua_pushnumber(L, sty->alignment);
			lua_setfield(L, -2, "align");

			lua_pushnumber(L, sty->MarginL);
			lua_setfield(L, -2, "margin_l");
			lua_pushnumber(L, sty->MarginR);
			lua_setfield(L, -2, "margin_r");
			lua_pushnumber(L, sty->MarginV); // duplicating MarginV to margin_t and margin_b here
			lua_setfield(L, -2, "margin_t");
			lua_pushnumber(L, sty->MarginV);
			lua_setfield(L, -2, "margin_b");

			lua_pushnumber(L, sty->encoding);
			lua_setfield(L, -2, "encoding");

			lua_pushnumber(L, 2); // From STS.h: "0: window, 1: video, 2: undefined (~window)"
			lua_setfield(L, -2, "relative_to");

			lua_pushboolean(L, false); // vertical writing, tentative AS5 field
			lua_setfield(L, -2, "vertical");

			lua_pushstring(L, "style");

		} else {
			lua_pushstring(L, "unknown");
		}
		// store class of item; last thing done for each class specific code must be pushing the class name
		lua_setfield(L, -2, "class");
	}

	AssEntry *LuaAssFile::LuaToAssEntry()
	{
		// assume an assentry table is on the top of the stack
		// convert it to a real AssEntry object, and pop the table from the stack

		if (!lua_istable(L, -1)) {
			lua_pushstring(L, "Can't convert a non-table value to AssEntry");
			lua_error(L);
			return 0;
		}

		lua_getfield(L, -1, "class");
		if (!lua_isstring(L, -1)) {
			lua_pushstring(L, "Table lacks 'class' field, can't convert to AssEntry");
			lua_error(L);
			return 0;
		}
		wxString lclass(lua_tostring(L, -1), wxConvUTF8);
		lclass.MakeLower();
		lua_pop(L, 1);

		AssEntry *result;

#define GETSTRING(varname, fieldname, lineclass)		\
	lua_getfield(L, -1, fieldname);						\
	if (!lua_isstring(L, -1)) {							\
		lua_pushstring(L, "Invalid string '" fieldname "' field in '" lineclass "' class subtitle line"); \
		lua_error(L);									\
		return 0;										\
	}													\
	wxString varname (lua_tostring(L, -1), wxConvUTF8);	\
	lua_pop(L, 1);
#define GETFLOAT(varname, fieldname, lineclass)			\
	lua_getfield(L, -1, fieldname);						\
	if (!lua_isnumber(L, -1)) {							\
		lua_pushstring(L, "Invalid number '" fieldname "' field in '" lineclass "' class subtitle line"); \
		lua_error(L);									\
		return 0;										\
	}													\
	float varname = lua_tonumber(L, -1);				\
	lua_pop(L, 1);
#define GETINT(varname, fieldname, lineclass)			\
	lua_getfield(L, -1, fieldname);						\
	if (!lua_isnumber(L, -1)) {							\
		lua_pushstring(L, "Invalid number '" fieldname "' field in '" lineclass "' class subtitle line"); \
		lua_error(L);									\
		return 0;										\
	}													\
	int varname = lua_tointeger(L, -1);					\
	lua_pop(L, 1);
#define GETBOOL(varname, fieldname, lineclass)			\
	lua_getfield(L, -1, fieldname);						\
	if (!lua_isboolean(L, -1)) {						\
		lua_pushstring(L, "Invalid boolean '" fieldname "' field in '" lineclass "' class subtitle line"); \
		lua_error(L);									\
		return 0;										\
	}													\
	bool varname = !!lua_toboolean(L, -1);				\
	lua_pop(L, 1);

		GETSTRING(section, "section", "common")

		if (lclass == _T("clear")) {
			result = new AssEntry(_T(""));
			result->group = section;

		} else if (lclass == _T("comment")) {
			GETSTRING(raw, "text", "comment")
			raw.Prepend(_T(";"));
			result = new AssEntry(raw);
			result->group = section;

		} else if (lclass == _T("head")) {
			result = new AssEntry(section);
			result->group = section;

		} else if (lclass == _T("info")) {
			GETSTRING(key, "key", "info")
			GETSTRING(value, "value", "info")
			result = new AssEntry(wxString::Format(_T("%s: %s"), key.c_str(), value.c_str()));
			result->group = _T("[Script Info]"); // just so it can be read correctly back

		} else if (lclass == _T("format")) {
			// ohshi- ...
			// *FIXME* maybe ignore the actual data and just put some default stuff based on section?
			result = new AssEntry(_T("Format: Auto4,Is,Broken"));
			result->group = section;

		} else if (lclass == _T("style")) {
			GETSTRING(name, "name", "style")
			GETSTRING(fontname, "fontname", "style")
			GETFLOAT(fontsize, "fontsize", "style")
			GETSTRING(color1, "color1", "style")
			GETSTRING(color2, "color2", "style")
			GETSTRING(color3, "color3", "style")
			GETSTRING(color4, "color4", "style")
			GETBOOL(bold, "bold", "style")
			GETBOOL(italic, "italic", "style")
			GETBOOL(underline, "underline", "style")
			GETBOOL(strikeout, "strikeout", "style")
			GETFLOAT(scale_x, "scale_x", "style")
			GETFLOAT(scale_y, "scale_y", "style")
			GETINT(spacing, "spacing", "style")
			GETFLOAT(angle, "angle", "style")
			GETINT(borderstyle, "borderstyle", "style")
			GETFLOAT(outline, "outline", "style")
			GETFLOAT(shadow, "shadow", "style")
			GETINT(align, "align", "style")
			GETINT(margin_l, "margin_l", "style")
			GETINT(margin_r, "margin_r", "style")
			GETINT(margin_t, "margin_t", "style")
			//GETINT(margin_b, "margin_b", "style") // skipping for now, since it's not used anyway
			GETINT(encoding, "encoding", "style")
			// leaving out relative_to and vertical

			AssStyle *sty = new AssStyle();
			sty->name = name;
			sty->font = fontname;
			sty->fontsize = fontsize;
			sty->primary.ParseASS(color1);
			sty->secondary.ParseASS(color2);
			sty->outline.ParseASS(color3);
			sty->shadow.ParseASS(color4);
			sty->bold = bold;
			sty->italic = italic;
			sty->underline = underline;
			sty->strikeout = strikeout;
			sty->scalex = scale_x;
			sty->scaley = scale_y;
			sty->spacing = spacing;
			sty->angle = angle;
			sty->borderstyle = borderstyle;
			sty->outline_w = outline;
			sty->shadow_w = shadow;
			sty->alignment = align;
			sty->MarginL = margin_l;
			sty->MarginR = margin_r;
			sty->MarginV = margin_t;
			sty->encoding = encoding;
			sty->UpdateData();

			result = sty;

		} else if (lclass == _T("stylex")) {
			lua_pushstring(L, "Found line with class 'stylex' which is not supported. Wait until AS5 is a reality.");
			lua_error(L);
			return 0;

		} else if (lclass == _T("dialogue")) {
			GETBOOL(comment, "comment", "dialogue")
			GETINT(layer, "layer", "dialogue")
			GETINT(start_time, "start_time", "dialogue")
			GETINT(end_time, "end_time", "dialogue")
			GETSTRING(style, "style", "dialogue")
			GETSTRING(actor, "actor", "dialogue")
			GETINT(margin_l, "margin_l", "dialogue")
			GETINT(margin_r, "margin_r", "dialogue")
			GETINT(margin_t, "margin_t", "dialogue")
			//GETINT(margin_b, "margin_b", "dialogue") // skipping for now, since it's not used anyway
			GETSTRING(effect, "effect", "dialogue")
			//GETSTRING(userdata, "userdata", "dialogue")
			GETSTRING(text, "text", "dialogue")

			AssDialogue *dia = new AssDialogue();
			dia->Comment = comment;
			dia->Layer = layer;
			dia->Start.SetMS(start_time);
			dia->End.SetMS(end_time);
			dia->Style = style;
			dia->Actor = actor;
			dia->MarginL = margin_l;
			dia->MarginR = margin_r;
			dia->MarginV = margin_t;
			dia->Effect = effect;
			dia->Text = text;
			dia->UpdateData();

			result = dia;

		} else {
			lua_pushfstring(L, "Found line with unknown class: %s", lclass.mb_str(wxConvUTF8));
			lua_error(L);
			return 0;
		}

#undef GETSTRING
#undef GETFLOAT
#undef GETINT
#undef GETBOOL

		lua_pop(L, 1);
		return result;
	}

	void LuaAssFile::GetAssEntry(int n)
	{
		entryIter e;
		if (n < last_entry_id/2) {
			// fastest to search from start
			e = ass->Line.begin();
			last_entry_id = n;
			while (n-- > 0) e++;
			last_entry_ptr = e;

		} else if (last_entry_id + n > last_entry_id + (ass->Line.size() - last_entry_id)/2) {
			// fastest to search from end
			int i = ass->Line.size();
			e = ass->Line.end();
			last_entry_id = n;
			while (i-- > n) e--;
			last_entry_ptr = e;

		} else if (last_entry_id > n) {
			// search backwards from last_entry_id
			e = last_entry_ptr;
			while (n < last_entry_id) e--, last_entry_id--;
			last_entry_ptr = e;
			
		} else {
			// search forwards from last_entry_id
			e = last_entry_ptr;
			// reqid and last_entry_id might be equal here, make sure the loop will still work
			while (n > last_entry_id) e++, last_entry_id++;
			last_entry_ptr = e;
		}
	}

	int LuaAssFile::ObjectIndexRead(lua_State *L)
	{
		LuaAssFile *laf = GetObjPointer(L, 1);

		switch (lua_type(L, 2)) {

			case LUA_TNUMBER:
				{
					// read an indexed AssEntry

					// get requested index
					int reqid = lua_tointeger(L, 2);
					if (reqid <= 0 || reqid > laf->ass->Line.size()) {
						lua_pushfstring(L, "Requested out-of-range line from subtitle file: %d", reqid);
						lua_error(L);
						return 0;
					}

					laf->GetAssEntry(reqid-1);
					laf->AssEntryToLua(*laf->last_entry_ptr);
					return 1;
				}

			case LUA_TSTRING:
				{
					// either return n or a function doing further stuff
					const char *idx = lua_tostring(L, 2);

					if (strcmp(idx, "n") == 0) {
						// get number of items
						lua_pushnumber(L, laf->ass->Line.size());
						return 1;

					} else if (strcmp(idx, "delete") == 0) {
						// make a "delete" function
						lua_pushvalue(L, 1);
						lua_pushcclosure(L, ObjectDelete, 1);
						return 1;

					} else if (strcmp(idx, "deleterange") == 0) {
						// make a "deleterange" function
						lua_pushvalue(L, 1);
						lua_pushcclosure(L, ObjectDeleteRange, 1);
						return 1;

					} else if (strcmp(idx, "insert") == 0) {
						// make an "insert" function
						lua_pushvalue(L, 1);
						lua_pushcclosure(L, ObjectInsert, 1);
						return 1;

					} else if (strcmp(idx, "append") == 0) {
						// make an "append" function
						lua_pushvalue(L, 1);
						lua_pushcclosure(L, ObjectAppend, 1);
						return 1;

					} else {
						// idiot
						lua_pushfstring(L, "Invalid indexing in Subtitle File object: '%s'", idx);
						lua_error(L);
						// should never return
					}
					assert(false);
				}

			default:
				{
					// crap, user is stupid!
					lua_pushfstring(L, "Attempt to index a Subtitle File object with value of type '%s'.", lua_typename(L, lua_type(L, 2)));
					lua_error(L);
				}
		}

		assert(false);
	}

	int LuaAssFile::ObjectIndexWrite(lua_State *L)
	{
		// instead of implementing everything twice, just call the other modification-functions from here
		// after modifying the stack to match their expectations

		if (!lua_isnumber(L, 2)) {
			lua_pushstring(L, "Attempt to write to non-numeric index in subtitle index");
			lua_error(L);
			return 0;
		}
		
		int n = lua_tointeger(L, 2);

		if (n < 0) {
			// insert line so new index is n
			lua_pushinteger(L, -n);
			lua_replace(L, 2);
			return ObjectInsert(L);

		} else if (n == 0) {
			// append line to list
			lua_remove(L, 2);
			return ObjectAppend(L);

		} else {
			// replace line at index n or delete
			if (!lua_isnil(L, 3)) {
				// insert
				LuaAssFile *laf = GetObjPointer(L, 1);
				AssEntry *e = laf->LuaToAssEntry();
				laf->GetAssEntry(n-1);
				delete *laf->last_entry_ptr;
				*laf->last_entry_ptr = e;
				return 0;

			} else {
				// delete
				lua_settop(L, 2); // make sure there's only two items on stack, laf and id
				return ObjectDelete(L);

			}
		}
	}

	int LuaAssFile::ObjectGetLen(lua_State *L)
	{
		LuaAssFile *laf = GetObjPointer(L, 1);
		lua_pushnumber(L, laf->ass->Line.size());
		return 1;
	}

	int LuaAssFile::ObjectDelete(lua_State *L)
	{
		LuaAssFile *laf = GetObjPointer(L, 1);

		// get number of items to delete
		int itemcount = lua_gettop(L);
		std::vector<int> ids;
		ids.reserve(itemcount-1);

		// the the item id's and sort them, so we can delete from last to first,
		// to preserve original numbering
		while (itemcount > 1) {
			if (!lua_isnumber(L, itemcount)) {
				lua_pushstring(L, "Attempt to delete non-numeric line id from Subtitle Object");
				lua_error(L);
				return 0;
			}
			int n = lua_tointeger(L, itemcount);
			if (n > laf->ass->Line.size() || n < 1) {
				lua_pushstring(L, "Attempt to delete out of range line id from Subtitle Object");
				lua_error(L);
				return 0;
			}
			ids.push_back(n-1); // make C-style line ids
		}
		std::sort(ids.begin(), ids.end());

		// now delete the id's backwards
		// start with the last one, to initialise things
		laf->GetAssEntry(ids.back());
		// get an iterator to it, and increase last_entry_ptr so it'll still be valid after deletion, and point to the right index
		entryIter e = laf->last_entry_ptr++;
		laf->ass->Line.erase(e);
		int n = laf->last_entry_id;
		for (int i = ids.size()-2; i >= 0; --i) {
			int id = ids[i];
			while (id > n--) laf->last_entry_ptr--;
			e = laf->last_entry_ptr++;
			delete *e;
			laf->ass->Line.erase(e);
		}
		laf->last_entry_id = n;

		return 0;
	}

	int LuaAssFile::ObjectDeleteRange(lua_State *L)
	{
		LuaAssFile *laf = GetObjPointer(L, 1);

		if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
			lua_pushstring(L, "Non-numeric argument given to deleterange");
			lua_error(L);
			return 0;
		}

		int a = lua_tointeger(L, 2), b = lua_tointeger(L, 3);

		if (a < 1) a = 1;
		if (b > laf->ass->Line.size()) b = laf->ass->Line.size();

		if (b < a) return 0;

		if (a == b) {
			laf->GetAssEntry(a-1);
			entryIter e = laf->last_entry_ptr++;
			delete *e;
			laf->ass->Line.erase(e);
			return 0;
		}

		entryIter ai, bi;
		laf->GetAssEntry(a-1);
		ai = laf->last_entry_ptr;
		laf->GetAssEntry(b-1);
		bi = laf->last_entry_ptr;
		laf->last_entry_ptr++;

		laf->ass->Line.erase(ai, bi);

		return 0;
	}

	int LuaAssFile::ObjectAppend(lua_State *L)
	{
		LuaAssFile *laf = GetObjPointer(L, 1);

		int n = lua_gettop(L);

		if (laf->last_entry_ptr != laf->ass->Line.begin()) {
			laf->last_entry_ptr--;
			laf->last_entry_id--;
		}

		for (int i = 2; i <= n; i++) {
			lua_pushvalue(L, i);
			AssEntry *e = laf->LuaToAssEntry();
			laf->ass->Line.push_back(e);
		}

		return 0;
	}

	int LuaAssFile::ObjectInsert(lua_State *L)
	{
		LuaAssFile *laf = GetObjPointer(L, 1);

		if (!lua_isnumber(L, 2)) {
			lua_pushstring(L, "Can't insert at non-numeric index");
			lua_error(L);
			return 0;
		}

		int n = lua_gettop(L);

		laf->GetAssEntry(lua_tonumber(L, 2)-1);

		for (int i = 3; i <= n; i++) {
			lua_pushvalue(L, i);
			AssEntry *e = laf->LuaToAssEntry();
			laf->ass->Line.insert(laf->last_entry_ptr, e);
			laf->last_entry_id++;
		}

		return 0;
	}

	int LuaAssFile::ObjectGarbageCollect(lua_State *L)
	{
		LuaAssFile *laf = GetObjPointer(L, 1);
		delete laf;
		wxLogDebug(_T(">>gc<< Garbage collected LuaAssFile"));
		return 0;
	}

	LuaAssFile *LuaAssFile::GetObjPointer(lua_State *L, int idx)
	{
		void *ud = lua_touserdata(L, idx);
		return *((LuaAssFile**)ud);
	}

	LuaAssFile::~LuaAssFile()
	{
	}

	LuaAssFile::LuaAssFile(lua_State *_L, AssFile *_ass)
		: ass(_ass)
		, L(_L)
	{
		// prepare cursor
		last_entry_ptr = ass->Line.begin();
		last_entry_id = 0;

		// prepare userdata object
		void *ud = lua_newuserdata(L, sizeof(LuaAssFile*));
		*((LuaAssFile**)ud) = this;

		// make the metatable
		lua_newtable(L);
		lua_pushcfunction(L, ObjectIndexRead);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, ObjectIndexWrite);
		lua_setfield(L, -2, "__newindex");
		lua_pushcfunction(L, ObjectGetLen);
		lua_setfield(L, -2, "__len");
		lua_pushcfunction(L, ObjectGarbageCollect);
		lua_setfield(L, -2, "__gc");
		lua_setmetatable(L, -2);
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
			L = lua_open();
			LuaStackcheck _stackcheck(L);

			// register standard libs
			lua_pushcfunction(L, luaopen_base); lua_call(L, 0, 0);
			lua_pushcfunction(L, luaopen_base); lua_call(L, 0, 0);
			lua_pushcfunction(L, luaopen_package); lua_call(L, 0, 0);
			lua_pushcfunction(L, luaopen_string); lua_call(L, 0, 0);
			lua_pushcfunction(L, luaopen_table); lua_call(L, 0, 0);
			lua_pushcfunction(L, luaopen_math); lua_call(L, 0, 0);
			_stackcheck.check(0);
			// FIXME!
			// unregister dofile() and loadfile() and provide alternative,
			// or hack those two functions instead?

			// TODO: register automation libs

			// prepare stuff in the registry
			// reference to the script object
			lua_pushlightuserdata(L, this);
			lua_setfield(L, LUA_REGISTRYINDEX, "aegisub");
			// the "feature" table
			// integer indexed, using same indexes as "features" vector in the base Script class
			lua_newtable(L);
			lua_setfield(L, LUA_REGISTRYINDEX, "features");
			_stackcheck.check(0);

			// for now: a stupid test function
			lua_pushstring(L, "showmessage");
			lua_pushcfunction(L, lua_callable_showmessage);
			lua_settable(L, LUA_GLOBALSINDEX);
			_stackcheck.check(0);

			// make "aegisub" table
			lua_pushstring(L, "aegisub");
			lua_newtable(L);
			lua_pushcfunction(L, LuaFeatureMacro::LuaRegister);
			lua_setfield(L, -2, "register_macro");
			lua_pushcfunction(L, lua_callable_set_undo_point);
			lua_setfield(L, -2, "set_undo_point");
			lua_settable(L, LUA_GLOBALSINDEX);
			_stackcheck.check(0);

			// load user script
			if (luaL_loadfile(L, GetFilename().mb_str(wxConvUTF8))) {
				wxString *err = new wxString(lua_tostring(L, -1), wxConvUTF8);
				err->Prepend(_T("An error occurred loading the Lua script file \"") + GetFilename() + _T("\":\n\n"));
				throw err->c_str();
			}
			_stackcheck.check(1);
			// and execute it
			// this is where features are registered
			// TODO: make sure a progress window is ready here!
			{
				LuaThreadedCall call(L, 0, 0);
				// FIXME: should display modal progress window here!?
				if (call.Wait()) {
					// error occurred, assumed to be on top of Lua stack
					wxString err(lua_tostring(L, -1), wxConvUTF8);
					err.Prepend(_T("An error occurred initialising the Lua script file \"") + GetFilename() + _T("\":\n\n"));
					throw err.c_str();
				}
			}
			_stackcheck.check(0);
			lua_getglobal(L, "script_name");
			if (lua_isstring(L, -1)) {
				name = wxString(lua_tostring(L, -1), wxConvUTF8);
				lua_pop(L, 1);
			} else {
				name = GetFilename();
			}
			// if we got this far, the script should be ready
			_stackcheck.check(0);

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
		lua_close(L);
		L = 0;

		loaded = false;
	}

	void LuaScript::Reload()
	{
		Destroy();
		Create();
	}

	LuaThreadedCall::LuaThreadedCall(lua_State *_L, int _nargs, int _nresults)
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
		int result = lua_pcall(L, nargs, nresults, 0);
		lua_gc(L, LUA_GCCOLLECT, 0);
		return (wxThread::ExitCode)result;
	}


	LuaFeature::LuaFeature(lua_State *_L, ScriptFeatureClass _featureclass, wxString &_name)
		: Feature(_featureclass, _name)
		, L(_L)
	{
	}

	void LuaFeature::RegisterFeature()
	{
		// get the LuaScript objects
		lua_getfield(L, LUA_REGISTRYINDEX, "aegisub");
		LuaScript *s = (LuaScript*)lua_touserdata(L, -1);
		lua_pop(L, 1);

		// add the Feature object
		s->features.push_back(this);

		// get the index+1 it was pushed into
		myid = s->features.size()-1;

		// create table with the functions
		// get features table
		lua_getfield(L, LUA_REGISTRYINDEX, "features");
		lua_pushvalue(L, -2);
		lua_rawseti(L, -2, myid);
		lua_pop(L, 1);
	}

	void LuaFeature::GetFeatureFunction(int functionid)
	{
		// get feature table
		lua_getfield(L, LUA_REGISTRYINDEX, "features");
		// get this feature's function pointers
		lua_rawgeti(L, -1, myid);
		// get pointer for validation function
		lua_rawgeti(L, -1, functionid);
		lua_remove(L, -2);
		lua_remove(L, -2);
	}

	void LuaFeature::CreateIntegerArray(std::vector<int> &ints)
	{
		// create an array-style table with an integer vector in it
		// leave the new table on top of the stack
		lua_newtable(L);
		for (int i = 0; i != ints.size(); ++i) {
			lua_pushinteger(L, ints[i]+1);
			lua_rawseti(L, -2, i+1);
		}
	}

	void LuaFeature::ThrowError()
	{
		wxString err(lua_tostring(L, -1), wxConvUTF8);
		lua_pop(L, 1);
		wxLogError(err);
	}


	int LuaFeatureMacro::LuaRegister(lua_State *L)
	{
		wxString _name(lua_tostring(L, 1), wxConvUTF8);
		wxString _description(lua_tostring(L, 2), wxConvUTF8);
		const char *_menustring = lua_tostring(L, 3);
		MacroMenu _menu = MACROMENU_NONE;

		if (strcmp(_menustring, "edit") == 0) _menu = MACROMENU_EDIT;
		else if (strcmp(_menustring, "video") == 0) _menu = MACROMENU_VIDEO;
		else if (strcmp(_menustring, "audio") == 0) _menu = MACROMENU_AUDIO;
		else if (strcmp(_menustring, "tools") == 0) _menu = MACROMENU_TOOLS;
		else if (strcmp(_menustring, "right") == 0) _menu = MACROMENU_RIGHT;

		if (_menu == MACROMENU_NONE) {
			lua_pushstring(L, "Error registering macro: Invalid menu name.");
			lua_error(L);
		}

		LuaFeatureMacro *macro = new LuaFeatureMacro(_name, _description, _menu, L);

		return 0;
	}

	LuaFeatureMacro::LuaFeatureMacro(wxString &_name, wxString &_description, MacroMenu _menu, lua_State *_L)
		: LuaFeature(_L, SCRIPTFEATURE_MACRO, _name)
		, FeatureMacro(_name, _description, _menu)
		, Feature(SCRIPTFEATURE_MACRO, _name)
	{
		// new table for containing the functions for this feature
		lua_newtable(L);
		// store processing function
		// TODO: check for being a function
		lua_pushvalue(L, 4);
		lua_rawseti(L, -2, 1);
		// and validation function
		// TODO: check for existing and being a function
		lua_pushvalue(L, 5);
		no_validate = !lua_isfunction(L, -1);
		lua_rawseti(L, -2, 2);
		// make the feature known
		RegisterFeature();
		// and remove the feature function table again
		lua_pop(L, 1);
	}

	bool LuaFeatureMacro::Validate(AssFile *subs, std::vector<int> &selected, int active)
	{
		if (no_validate)
			return true;

		wxLogDebug(_T("Validate start"));

		int startstack = lua_gettop(L);

		GetFeatureFunction(2);
		wxLogDebug(_T("Got function"));

		// prepare function call
		LuaAssFile *subsobj = new LuaAssFile(L, subs); // subs object
		CreateIntegerArray(selected); // selected items
		lua_pushinteger(L, -1); // active line
		wxLogDebug(_T("Prepared parameters"));
		// do call
		LuaThreadedCall call(L, 3, 1);
		wxLogDebug(_T("Waiting for call to finish"));
		wxThread::ExitCode code = call.Wait();
		wxLogDebug(_T("Exit code was %d"), (int)code);
		wxLogDebug(_T("Getting result"));
		// get result
		bool result = lua_toboolean(L, -1);

		// clean up stack
		lua_pop(L, 1);
		// stack should be empty now
		assert(lua_gettop(L) == startstack);
		wxLogDebug(_T("Finished validation"));

		return result;
	}

	void LuaFeatureMacro::Process(AssFile *subs, std::vector<int> &selected, int active)
	{
		LuaStackcheck _stackcheck(L);

		wxLogDebug(_T("Process start"));

		GetFeatureFunction(1);
		wxLogDebug(_T("Got function"));
		_stackcheck.check(1);

		// prepare function call
		LuaAssFile *subsobj = new LuaAssFile(L, subs); // subs object
		CreateIntegerArray(selected); // selected items
		lua_pushinteger(L, -1); // active line
		wxLogDebug(_T("Prepared parameters"));
		// do call
		LuaThreadedCall call(L, 3, 0);
		wxLogDebug(_T("Waiting for call to finish"));
		wxThread::ExitCode code = call.Wait();
		wxLogDebug(_T("Exit code was %d"), (int)code);
		if (code) ThrowError();

		// stack should be empty now
		_stackcheck.check(0);
		wxLogDebug(_T("Finished processing"));
	}


	class LuaScriptFactory : public ScriptFactory {
	public:
		LuaScriptFactory()
		{
			engine_name = _T("Lua");
			Register(this);
		}

		virtual Script* Produce(const wxString &filename) const
		{
			// *FIXME* temporary measure
			return new LuaScript(filename);
			// Check if the file can be parsed as a Lua script;
			// create a temporary interpreter for that and attempt to load
			// but NOT execute the script
			lua_State *L = lua_open();
			if (luaL_loadfile(L, filename.mb_str(wxConvUTF8)) == 0) {
				// success! parsed it without errors
				lua_close(L);
				return new LuaScript(filename);
			} else {
				lua_close(L);
				return 0;
			}
		}
	};

	static LuaScriptFactory _lua_factory;

};
