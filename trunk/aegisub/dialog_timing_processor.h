// Copyright (c) 2005, Rodrigo Braz Monteiro
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

#ifndef DIALOG_TIMING_PROCESSOR
#define DIALOG_TIMING_PROCESSOR


///////////
// Headers
#include <wx/wxprec.h>
#include <vector>


//////////////
// Prototypes
class SubtitlesGrid;
class AssDialogue;


////////////////
// Dialog Class
class DialogTimingProcessor : public wxDialog {
private:
	SubtitlesGrid *grid;
	wxStaticBoxSizer *KeyframesSizer;

	wxCheckBox *onlySelection;

	wxTextCtrl *leadIn;
	wxTextCtrl *leadOut;
	wxCheckBox *hasLeadIn;
	wxCheckBox *hasLeadOut;

	wxCheckBox *keysEnable;
	wxTextCtrl *keysStartBefore;
	wxTextCtrl *keysStartAfter;
	wxTextCtrl *keysEndBefore;
	wxTextCtrl *keysEndAfter;

	wxCheckBox *adjsEnable;
	wxTextCtrl *adjascentThres;
	wxSlider *adjascentBias;

	wxCheckListBox *StyleList;
	wxButton *ApplyButton;
	wxString leadInTime,leadOutTime,thresStartBefore,thresStartAfter,thresEndBefore,thresEndAfter,adjsThresTime;

	wxArrayInt KeyFrames;

	void OnCheckBox(wxCommandEvent &event);
	void OnSelectAll(wxCommandEvent &event);
	void OnSelectNone(wxCommandEvent &event);
	void OnApply(wxCommandEvent &event);

	void UpdateControls();
	void Process();
	int GetClosestKeyFrame(int frame);
	bool StyleOK(wxString styleName);

	std::vector<AssDialogue*> Sorted;
	AssDialogue *GetSortedDialogue(int n);
	void SortDialogues();

public:
	DialogTimingProcessor(wxWindow *parent,SubtitlesGrid *grid);

	DECLARE_EVENT_TABLE()
};


///////
// IDs
enum {
	CHECK_ENABLE_LEADIN = 1850,
	CHECK_ENABLE_LEADOUT,
	CHECK_ENABLE_KEYFRAME,
	CHECK_ENABLE_ADJASCENT,
	BUTTON_SELECT_ALL,
	BUTTON_SELECT_NONE,
	TIMING_STYLE_LIST
};


#endif
