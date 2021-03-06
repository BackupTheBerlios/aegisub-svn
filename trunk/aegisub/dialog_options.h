// Copyright (c) 2006, Rodrigo Braz Monteiro
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


////////////
// Includes
#include <wx/wxprec.h>
#include <vector>
#include "options.h"


//////////////
// Prototypes
#ifdef wxUSE_TREEBOOK
class wxTreebook;
#else
#include <wx/choicebk.h>
typedef wxChoicebook wxTreebook;
#endif


/////////////
// Bind pair
class OptionsBind {
public:
	wxControl *ctrl;
	wxString option;
	int param;
};


////////////////////////
// Options screen class
class DialogOptions: public wxDialog {
private:
	bool needsRestart;

	wxTreebook *book;
	std::vector<OptionsBind> binds;

	void Bind(wxControl *ctrl,wxString option,int param=0);
	void WriteToOptions(bool justApply=false);
	void ReadFromOptions();

	void OnOK(wxCommandEvent &event);
	void OnCancel(wxCommandEvent &event);
	void OnApply(wxCommandEvent &event);

public:
	DialogOptions(wxWindow *parent);
	~DialogOptions();

	DECLARE_EVENT_TABLE()
};
