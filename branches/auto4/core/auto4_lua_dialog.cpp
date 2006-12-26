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
// Contact: mailto:jiifurusu@gmail.com
//

#include "auto4_lua.h"
#include <lualib.h>
#include <lauxlib.h>
#include <wx/window.h>
#include <wx/spinctrl.h>
#include <wx/gbsizer.h>
#include <wx/button.h>
#include <wx/validate.h>
#include <assert.h>

namespace Automation4 {


	// LuaConfigDialogControl

	LuaConfigDialogControl::LuaConfigDialogControl(lua_State *L)
	{
		// Assume top of stack is a control table (don't do checking)

		lua_getfield(L, -1, "name");
		name = wxString(lua_tostring(L, -1), wxConvUTF8);
		lua_pop(L, 1);

		lua_getfield(L, -1, "x");
		x = lua_tointeger(L, -1);
		if (x < 1) x = 1;
		lua_pop(L, 1);

		lua_getfield(L, -1, "y");
		y = lua_tointeger(L, -1);
		if (y < 1) y = 1;
		lua_pop(L, 1);

		lua_getfield(L, -1, "width");
		width = lua_tointeger(L, -1);
		if (width < 1) width = 1;
		lua_pop(L, 1);

		lua_getfield(L, -1, "height");
		height = lua_tointeger(L, -1);
		if (height < 1) height = 1;
		lua_pop(L, 1);

		lua_getfield(L, -1, "hint");
		hint = wxString(lua_tostring(L, -1), wxConvUTF8);
		lua_pop(L, 1);
	}

	namespace LuaControl {

		// Label

		class Label : public LuaConfigDialogControl {
		public:
			wxString label;

			Label(lua_State *L)
				: LuaConfigDialogControl(L)
			{
				lua_getfield(L, -1, "label");
				label = wxString(lua_tostring(L, -1), wxConvUTF8);
				lua_pop(L, 1);
			}

			wxControl *Create(wxWindow *parent)
			{
				return cw = new wxStaticText(parent, -1, label);
			}

			void ControlReadBack()
			{
				// Nothing here
			}

			void LuaReadBack(lua_State *L)
			{
				// Label doesn't produce output, so let it be nil
				lua_pushnil(L);
			}
		};


		// Basic edit

		class Edit : public LuaConfigDialogControl {
		public:
			wxString text;

			Edit(lua_State *L)
				: LuaConfigDialogControl(L)
			{
				lua_getfield(L, -1, "text");
				text = wxString(lua_tostring(L, -1), wxConvUTF8);
				lua_pop(L, 1);
			}

			wxControl *Create(wxWindow *parent)
			{
				return cw = new wxTextCtrl(parent, -1, text, wxDefaultPosition, wxDefaultSize, 0);
			}

			void ControlReadBack()
			{
				text = ((wxTextCtrl*)cw)->GetValue();
			}

			void LuaReadBack(lua_State *L)
			{
				lua_pushstring(L, text.mb_str(wxConvUTF8));
			}

		};

		
		// Multiline edit

		class Textbox : public Edit {
		public:

			Textbox(lua_State *L)
				: Edit(L)
			{
				// Nothing more
			}

			wxControl *Create(wxWindow *parent)
			{
				cw = new wxTextCtrl(parent, -1, text, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
				cw->SetMinSize(wxSize(0, 30));
				return cw;
			}

		};


		// Integer only edit

		class IntEdit : public Edit {
		public:
			int value;
			bool hasspin;
			int min, max;

			IntEdit(lua_State *L)
				: Edit(L)
			{
				lua_getfield(L, -1, "value");
				value = lua_tointeger(L, -1);
				lua_pop(L, 1);

				hasspin = false;

				lua_getfield(L, -1, "min");
				if (!lua_isnumber(L, -1))
					goto nospin;
				min = lua_tointeger(L, -1);
				lua_pop(L, 1);

				lua_getfield(L, -1, "max");
				if (!lua_isnumber(L, -1))
					goto nospin;
				max = lua_tointeger(L, -1);
				lua_pop(L, 1);

				hasspin = true;
nospin:
				if (!hasspin) {
					lua_pop(L, 1);
				}
			}

			typedef wxValidator IntTextValidator; // TODO
			wxControl *Create(wxWindow *parent)
			{
				if (hasspin) {
					return cw = new wxSpinCtrl(parent, -1, wxString::Format(_T("%d"), value), wxDefaultPosition, wxDefaultSize, min, max, value);
				} else {
					return cw = new wxTextCtrl(parent, -1, text, wxDefaultPosition, wxDefaultSize, 0, IntTextValidator());
				}
			}

			void ControlReadBack()
			{
				if (hasspin) {
					value = ((wxSpinCtrl*)cw)->GetValue();
				} else {
					long newval;
					text = ((wxTextCtrl*)cw)->GetValue();
					if (text.ToLong(&newval)) {
						value = newval;
					}
				}
			}

			void LuaReadBack(lua_State *L)
			{
				lua_pushinteger(L, value);
			}

		};


		// Float only edit

		class FloatEdit : public Edit {
		public:
			float value;
			// FIXME: Can't support spin button atm

			FloatEdit(lua_State *L)
				: Edit(L)
			{
				lua_getfield(L, -1, "value");
				value = lua_tointeger(L, -1);
				lua_pop(L, 1);

				// TODO: spin button support
			}

			typedef wxValidator FloatTextValidator;
			wxControl *Create(wxWindow *parent)
			{
				return cw = new wxTextCtrl(parent, -1, text, wxDefaultPosition, wxDefaultSize, 0, FloatTextValidator());
			}

			void ControlReadBack()
			{
				double newval;
				text = ((wxTextCtrl*)cw)->GetValue();
				if (text.ToDouble(&newval)) {
					value = newval;
				}
			}

			void LuaReadBack(lua_State *L)
			{
				lua_pushnumber(L, value);
			}

		};


		// Dropdown

		class Dropdown : public LuaConfigDialogControl {
		public:
			wxArrayString items;
			wxString value;

			Dropdown(lua_State *L)
				: LuaConfigDialogControl(L)
			{
				lua_getfield(L, -1, "value");
				value = wxString(lua_tostring(L, -1), wxConvUTF8);
				lua_pop(L, 1);

				lua_getfield(L, -1, "items");
				lua_pushnil(L);
				while (lua_next(L, -2)) {
					if (lua_isstring(L, -1)) {
						items.Add(wxString(lua_tostring(L, -1), wxConvUTF8));
					}
					lua_pop(L, 1);
				}
				lua_pop(L, 1);
			}

			wxControl *Create(wxWindow *parent)
			{
				return cw = new wxComboBox(parent, -1, value, wxDefaultPosition, wxDefaultSize, items, wxCB_READONLY);
			}

			void ControlReadBack()
			{
				value = ((wxComboBox*)cw)->GetValue();
			}

			void LuaReadBack(lua_State *L)
			{
				lua_pushstring(L, value.mb_str(wxConvUTF8));
			}
			
		};


		// Checkbox

		class Checkbox : public LuaConfigDialogControl {
		public:
			wxString label;
			bool value;

			Checkbox(lua_State *L)
				: LuaConfigDialogControl(L)
			{
				lua_getfield(L, -1, "label");
				label = wxString(lua_tostring(L, -1), wxConvUTF8);
				lua_pop(L, 1);

				lua_getfield(L, -1, "value");
				value = lua_toboolean(L, -1) != 0;
				lua_pop(L, 1);
			}

			wxControl *Create(wxWindow *parent)
			{
				return cw = new wxCheckBox(parent, -1, label);
			}

			void ControlReadBack()
			{
				value = ((wxCheckBox*)cw)->GetValue();
			}

			void LuaReadBack(lua_State *L)
			{
				lua_pushboolean(L, value);
			}

		};

	};


	// LuaConfigDialog

	LuaConfigDialog::LuaConfigDialog(lua_State *L, bool include_buttons)
	{
		if (include_buttons) {
			button_pushed = 0;

			if (!lua_istable(L, -1))
				// Just to avoid deeper indentation...
				goto skipbuttons;
			// Iterate over items in table
			lua_pushnil(L); // initial key
			while (lua_next(L, -2)) {
				// Simply skip invalid items... FIXME, warn here?
				if (lua_isstring(L, -1)) {
					wxString s(lua_tostring(L, -1), wxConvUTF8);
					buttons.push_back(s);
				}
				lua_pop(L, 1);
			}
skipbuttons:
			lua_pop(L, 1);
		}

		// assume top of stack now contains a dialog table
		if (!lua_istable(L, -1)) {
			lua_pushstring(L, "Cannot create config dialog from something non-table");
			lua_error(L);
			assert(false);
		}

		// Ok, so there is a table with controls
		lua_pushnil(L); // initial key
		while (lua_next(L, -2)) {
			if (lua_istable(L, -1)) {
				// Get control class
				lua_getfield(L, -1, "class");
				if (!lua_isstring(L, -1))
					goto badcontrol;
				wxString controlclass(lua_tostring(L, -1), wxConvUTF8);
				controlclass.LowerCase();
				lua_pop(L, 1);

				LuaConfigDialogControl *ctl;

				// Check control class and create relevant control
				if (controlclass == _T("label")) {
					ctl = new LuaControl::Label(L);
				} else if (controlclass == _T("edit")) {
					ctl = new LuaControl::Edit(L);
				} else if (controlclass == _T("intedit")) {
					ctl = new LuaControl::IntEdit(L);
				} else if (controlclass == _T("floatedit")) {
					ctl = new LuaControl::FloatEdit(L);
				} else if (controlclass == _T("textbox")) {
					ctl = new LuaControl::Textbox(L);
				} else if (controlclass == _T("dropdown")) {
					ctl = new LuaControl::Dropdown(L);
				} else if (controlclass == _T("checkbox")) {
					ctl = new LuaControl::Checkbox(L);
				} else if (controlclass == _T("color")) {
					// FIXME
					ctl = new LuaControl::Edit(L);
				} else if (controlclass == _T("coloralpha")) {
					// FIXME
					ctl = new LuaControl::Edit(L);
				} else if (controlclass == _T("alpha")) {
					// FIXME
					ctl = new LuaControl::Edit(L);
				} else {
					goto badcontrol;
				}

				controls.push_back(ctl);

			} else {
badcontrol:
				// not a control...
				// FIXME, better error reporting?
				lua_pushstring(L, "bad control table entry");
				lua_error(L);
			}
			lua_pop(L, 1);
		}
	}

	LuaConfigDialog::~LuaConfigDialog()
	{
		for (size_t i = 0; i < controls.size(); ++i)
			delete controls[i];
	}

	wxWindow* LuaConfigDialog::CreateWindow(wxWindow *parent)
	{
		wxWindow *w = new wxPanel(parent);
		wxGridBagSizer *s = new wxGridBagSizer(2, 2);

		for (size_t i = 0; i < controls.size(); ++i) {
			LuaConfigDialogControl *c = controls[i];
			c->Create(w);
			s->Add(c->cw, wxGBPosition(c->x, c->y), wxGBSpan(c->width, c->height));
		}

		if (use_buttons) {
			wxStdDialogButtonSizer *bs = new wxStdDialogButtonSizer();
			if (buttons.size() > 0) {
				for (size_t i = 0; i < buttons.size(); ++i) {
					bs->Add(new wxButton(w, 1001+(wxWindowID)i, buttons[i]));
				}
			} else {
				bs->Add(new wxButton(w, wxID_OK));
				bs->Add(new wxButton(w, wxID_CANCEL));
			}
			bs->Realize();
			w->Connect(wxID_ANY, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(LuaConfigDialog::OnButtonPush), 0, this);

			wxBoxSizer *ms = new wxBoxSizer(wxVERTICAL);
			ms->Add(s, 0, wxBOTTOM, 5);
			ms->Add(bs);
			w->SetSizerAndFit(ms);
		} else {
			w->SetSizerAndFit(s);
		}

		return w;
	}

	int LuaConfigDialog::LuaReadBack(lua_State *L)
	{
		// First read back which button was pressed, if any
		if (use_buttons) {
			if (button_pushed == 0) {
				// Always cancel/closed
				lua_pushboolean(L, 0);
			} else {
				if (buttons.size() > 0) {
					// button_pushed is index+1 to reserve 0 for Cancel
					lua_pushstring(L, buttons[button_pushed-1].mb_str(wxConvUTF8));
				} else {
					// Cancel case already covered, must be Ok then
					lua_pushboolean(L, 1);
				}
			}
		}

		// Then read controls back
		lua_newtable(L);
		for (size_t i = 0; i < controls.size(); ++i) {
			controls[i]->LuaReadBack(L);
			lua_setfield(L, -2, controls[i]->name.mb_str(wxConvUTF8));
		}

		if (use_buttons) {
			return 2;
		} else {
			return 1;
		}
	}

	void LuaConfigDialog::ReadBack()
	{
		for (size_t i = 0; i < controls.size(); ++i) {
			controls[i]->ControlReadBack();
		}
	}

	void LuaConfigDialog::OnButtonPush(wxCommandEvent &evt)
	{
		// Let button_pushed == 0 mean "cancelled", such that pushing Cancel or closing the dialog
		// will both result in button_pushed == 0
		if (evt.GetId() == wxID_OK) {
			button_pushed = 1;
		} else if (evt.GetId() == wxID_CANCEL) {
			button_pushed = 0;
		} else {
			// Therefore, when buttons are numbered from 1001 to 1000+n, make sure to set it to i+1
			button_pushed = evt.GetId() - 1000;
			evt.SetId(wxID_OK); // hack to make sure the dialog will be closed
		}
		evt.Skip();
	}

};