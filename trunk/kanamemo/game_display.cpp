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


///////////
// Headers
#include "game_display.h"
#include "kana_table.h"
#include "game_window.h"
#include "game_panel.h"
#include "main.h"


///////////////
// Constructor
GameDisplay::GameDisplay(wxWindow *parent)
: wxWindow (parent,-1,wxDefaultPosition,wxSize(400,320))
{
	// Get status bar
	statusBar = ((wxFrame*)parent)->GetStatusBar();
	wxMenuBar *menuBar = ((wxFrame*)parent)->GetMenuBar();
	menuCheck[0] = menuBar->FindItem(Menu_Options_Hiragana);
	menuCheck[1] = menuBar->FindItem(Menu_Options_Katakana);
	EnableGame(false);

	// Initialize
	Reset();
}


//////////////
// Destructor
GameDisplay::~GameDisplay() {
	Save();
	delete table;
}


/////////
// Reset
void GameDisplay::Reset() {
	autoLevel = true;
	status = 0;
	levelUp = 0;
	level[0] = 1;
	level[1] = 1;
	enabled[0] = true;
	enabled[1] = false;
	menuCheck[0]->Check(enabled[0]);
	menuCheck[1]->Check(enabled[1]);
	table = new KanaTable();
	current = NULL;
	ResetTable(0);
	ResetTable(1);
	GetNextKana();
	UpdateStatusBar();
}


/////////////////
// Get next kana
void GameDisplay::GetNextKana() {
	// Select table
	int tn;
	if (!enabled[0]) tn = 1;
	else if (!enabled[1]) tn = 0;
	else tn = rand() % 2;
	curTable = tn;

	// Maximum number of glyphs
	int n = GetNAtLevel(level[curTable],curTable);

	// Add all valid positions to an array
	// Each one will be added a number of times relative to its weight (1 to 20 times)
	std::vector<int> picks;
	int cur;
	for (int i=0;i<n;i++) {
		cur = scores[tn][i];
		if (cur != -100) {
			int weight = 10 - cur;
			if (weight < 1) weight = 1;
			if (weight > 20) weight = 20;
			weight = weight * weight;
			for (int j=0;j<weight;j++) picks.push_back(i);
		}
	}

	// Pick one
	int old = curn;
	do {
		curn = picks[rand()%picks.size()];
	} while (old == curn);
	current = &table->GetEntry(curn);

	// Refresh
	Refresh(false);
}


////////////////
// Enter romaji
void GameDisplay::EnterRomaji(wxString romaji) {
	// Check if it's correct
	bool correct = (romaji == current->hepburn);
	lastEntry = romaji;
	levelUp = 0;

	// Get next
	if (correct) {
		// Add 1 to score
		int temp = scores[curTable][curn] + 1;
		if (temp > 10) temp = 10;
		scores[curTable][curn] = temp;

		// Check if it's OK to level up
		int n = GetNAtLevel(level[curTable],curTable);
		bool ok = true;
		for (int i=0;i<n;i++) {
			if (scores[curTable][i] <= 2) {
				ok = false;
				break;
			}
		}

		// Level up
		if (ok) {
			int n2 = GetNAtLevel(level[curTable]+1,curTable);
			if (n2 != n) {
				levelUp = curTable+1;
				level[curTable]++;
				for (int i=n;i<n2;i++) scores[curTable][i] = 0;
				UpdateStatusBar();
			}
		}

		// Get next
		lastTable = curTable;
		previous = current;
		status = 3;
		GetNextKana();
		Refresh(false);
	}

	// Wrong
	else {
		// Is asking?
		bool isAsking = romaji == _T("?");

		// Points to subtract
		int toSub = 20;
		if (isAsking) toSub = 2;

		// Subtract points from this glyph
		int temp = scores[curTable][curn] - toSub;
		if (temp < -10) temp = -10;
		scores[curTable][curn] = temp;

		// Set status if it's asking
		if (isAsking) status = 5;

		// Otherwise...
		else {
			// Subtract points from the glyph the user typed, if it even exists
			int n = table->GetNumberEntries();
			for (int i=0;i<n;i++) {
				if (table->GetEntry(i).hepburn == romaji) {
					temp = scores[curTable][i];
					if (temp == -100) break;
					temp -= toSub;
					if (temp < -10) temp = -10;
					scores[curTable][i] = temp;
					break;
				}
			}

			// Set status
			if (status == 2 || status == 4) status = 4;
			else if (status == 1) status = 2;
			else status = 1;
		}

		// Refresh
		Refresh(false);
	}

	// Save
	Save();
}


/////////////////
// Reset a table
void GameDisplay::ResetTable(int id) {
	// Get entries for this table
	int n = table->GetNumberEntries();

	// Set size properly
	scores[id].resize(n);

	// Reset with -100
	for (int i=0;i<n;i++) scores[id][i] = -100;

	// Enable enough for the level
	int leveln = GetNAtLevel(level[id],id);
	for (int i=0;i<leveln;i++) scores[id][i] = 0;
}


/////////////////////////////////
// Get number of glyphs at level
int GameDisplay::GetNAtLevel(int level,int tablen) {
	int max = table->GetLevels(tablen);
	if (level > max) level = max;
	return table->GetNumberEntries(level);
}


///////////////
// Event table
BEGIN_EVENT_TABLE(GameDisplay,wxWindow)
	EVT_PAINT(GameDisplay::OnPaint)
	EVT_LEFT_DOWN(GameDisplay::OnClick)
END_EVENT_TABLE()


///////////////
// Draw window
void GameDisplay::OnPaint(wxPaintEvent &event) {
	// Begin drawing
	wxPaintDC dc(this);
	dc.BeginDrawing();
	int w,h,tw,th;
	GetClientSize(&w,&h);

	// Background colours
	//wxColour col1(130,177,236);
	//wxColour col2(181,209,244);
	wxColour col1(220,194,105);
	wxColour col2(245,231,177);

	// Draw background
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(col1);
	dc.DrawRectangle(0,0,w,h);
	dc.SetBrush(col2);
	dc.DrawRectangle(5,5,w-10,h-10);
	dc.SetBrush(col1);
	dc.DrawRectangle(10,0,5,h);
	dc.DrawRectangle(w-15,0,5,h);
	dc.DrawRectangle(0,10,w,5);
	dc.DrawRectangle(0,h-15,w,5);

	// Enabled?
	if (!isOn) {
		dc.EndDrawing();
		return;
	}

	// Set font
	wxFont font(128,wxDEFAULT,wxFONTSTYLE_NORMAL,wxNORMAL,0,_T("MS Mincho"),wxFONTENCODING_DEFAULT );
	dc.SetFont(font);
	if (status == 0) dc.SetTextForeground(wxColour(0,0,0));
	if (status == 1 || status == 2 || status == 4) dc.SetTextForeground(wxColour(220,10,10));

	// Get text metrics
	wxString text;
	if (curTable == 0) text = current->hiragana;
	else text = current->katakana;
	dc.GetTextExtent(text,&tw,&th,NULL,NULL,&font);

	// Draw text
	dc.SetPen(wxColour(0,0,0));
	dc.DrawText(text,(w-tw)/2,(h-th)/2);

	// Prepare status text
	font.SetFaceName(_T("Verdana"));
	font.SetPointSize(16);
	dc.SetFont(font);
	wxString topString;
	wxString bottomString;
	wxString bottomString2;
	wxString levelString;

	// Correct
	if (status == 3) {
		dc.SetTextForeground(wxColour(0,128,0));
		topString = wxString(_T("Correct! \""));
		if (lastTable == 0) topString += previous->hiragana;
		if (lastTable == 1) topString += previous->katakana;
		topString += wxString(_T("\" is \"")) + lastEntry + _T("\"!");
	}

	// Wrong
	if (status == 1 || status == 2 || status == 4) {
		// Top text
		const KanaEntry *real = table->FindByRomaji(lastEntry);
		topString = wxString(_T("Wrong!"));
		if (real) {
			topString += _T(" \"");
			if (curTable == 0) topString += real->hiragana;
			if (curTable == 1) topString += real->katakana;
			topString += wxString(_T("\" is \"")) + lastEntry + _T("\"!");
		}
		else topString += _T(" \"") + lastEntry + _T("\" doesn't exist!");

		// Find hints
		wxString group = current->group;
		wxString vowel = current->hepburn.Right(1);
		if (vowel == _T("n")) {
			group = vowel;
			vowel = _T("");
		}

		// Bottom text (hints)
		bottomString = wxString(_T("Hint: \""));
		if (curTable == 0) bottomString += current->hiragana;
		if (curTable == 1) bottomString += current->katakana;
		if (status == 4) {
			bottomString += _T("\" is \"") + current->hepburn + _T("\".");
		}
		else {
			if (group == _T("a")) {
				if (status == 1) bottomString += _T("\" is a vowel.");
				else bottomString += _T("\" is the vowel \"") + current->hepburn + _T("\".");
			}
			else {
				bottomString += _T("\" is from the group of \"") + group + _T("\"");
				if (status == 1 || vowel == _T("")) bottomString += _T(".");
				else bottomString2 = _T("ending with the vowel \"") + vowel + _T("\".");
			}
		}
	}

	// Asking for help
	if (status == 5) {
		dc.SetTextForeground(wxColour(96,64,0));
		bottomString = wxString(_T("\""));
		if (curTable == 0) bottomString += current->hiragana;
		if (curTable == 1) bottomString += current->katakana;
		bottomString += _T("\" is \"") + current->hepburn + _T("\".");
	}

	// Level up
	if (levelUp) {
		levelString = _T("Level up on ");
		if (levelUp == 1) levelString += _T("hiragana!");
		if (levelUp == 2) levelString += _T("katakana!");
	}

	// Draw top text
	int th2=0,tw2=0;
	dc.GetTextExtent(topString,&tw,&th,NULL,NULL,&font);
	dc.DrawText(topString,(w-tw)/2,20);
	font.SetPointSize(14);
	dc.SetFont(font);
	dc.GetTextExtent(levelString,&tw2,&th2,NULL,NULL,&font);
	dc.DrawText(levelString,(w-tw2)/2,20+th);

	// Draw bottom text
	font.SetPointSize(12);
	dc.SetFont(font);
	dc.GetTextExtent(bottomString,&tw,&th,NULL,NULL,&font);
	if (!bottomString2.IsEmpty()) dc.GetTextExtent(bottomString2,&tw2,&th2,NULL,NULL,&font);
	else {
		tw2 = 0;
		th2 = 0;
	}
	dc.DrawText(bottomString,(w-tw)/2,h-20-th-th2);
	dc.DrawText(bottomString2,(w-tw2)/2,h-20-th2);

	// Done
	dc.EndDrawing();
}


/////////////////////
// Update status bar
void GameDisplay::UpdateStatusBar() {
	statusBar->SetStatusText(playerName,0);
	if (enabled[0]) statusBar->SetStatusText(wxString::Format(_T("Hiragana lvl %i"),level[0]),1);
	else statusBar->SetStatusText(_T("Hiragana disabled"),1);
	if (enabled[1]) statusBar->SetStatusText(wxString::Format(_T("Katakana lvl %i"),level[1]),2);
	else statusBar->SetStatusText(_T("Katakana disabled"),2);
}


//////////////////////////
// Enable/disable a table
void GameDisplay::EnableTable(int tn,bool status) {
	if (enabled[tn] != status) {
		enabled[tn] = status;
		if (status == false && curTable == tn) {
			if (enabled[1-curTable] == false) {
				enabled[1-curTable] = true;
				menuCheck[1-curTable]->Check(true);
			}
			status = 0;
			GetNextKana();
		}
	}
	UpdateStatusBar();
}


/////////////
// Set level
void GameDisplay::SetLevel(int tablen,int lvl) {
	// Set values
	int max = table->GetNumberEntries();
	int n = table->GetNumberEntries(lvl);
	for (int i=0;i<n;i++) if (scores[tablen][i] == -100) scores[tablen][i] = 0;
	for (int i=n;i<max;i++) scores[tablen][i] = -100;
	level[tablen] = lvl;

	// Check if currently shown image needs to be switched
	if (tablen == curTable && current->level > lvl) {
		status = 0;
		GetNextKana();
	}

	// Update status
	UpdateStatusBar();
}


///////////////
// Enable game
void GameDisplay::EnableGame(bool enable) {
	UpdateStatusBar();
	if (isOn == enable) return;
	isOn = enable;
	Refresh(false);
}


/////////////
// Save game
void GameDisplay::Save() {
	// Player name set?
	if (playerName.IsEmpty()) return;

	// Open file
	wxString path = KanaMemo::folderName + _T("/") + playerName + _T(".usr");
	FILE *fp = fopen(path.mb_str(wxConvLocal),"wb");
	if (!fp) return;

	// Write version
	char buffer[128];
	strcpy(buffer,"kanamemo V2");
	fwrite(buffer,1,12,fp);

	// Write enabled status
	char temp;
	temp = enabled[0] ? 1 : 0;
	fwrite(&temp,1,1,fp);
	temp = enabled[1] ? 1 : 0;
	fwrite(&temp,1,1,fp);

	// Write levels
	temp = level[0];
	fwrite(&temp,1,1,fp);
	temp = level[1];
	fwrite(&temp,1,1,fp);

	// Write autolevel flag
	temp = autoLevel;
	fwrite(&temp,1,1,fp);

	// Write scores
	int temp2;
	temp2 = scores[0].size();
	fwrite(&temp2,1,4,fp);
	for (int i=0;i<temp2;i++) fwrite(&scores[0][i],1,4,fp);
	temp2 = scores[1].size();
	fwrite(&temp2,1,4,fp);
	for (int i=0;i<temp2;i++) fwrite(&scores[1][i],1,4,fp);

	// Close file
	fclose(fp);
}


/////////////
// Load game
void GameDisplay::Load() {
	// Player name set?
	if (playerName.IsEmpty()) return;

	// Open file
	wxString path = KanaMemo::folderName + _T("/") + playerName + _T(".usr");
	FILE *fp = fopen(path.mb_str(wxConvLocal),"rb");
	if (!fp) return;

	// Reset
	Reset();

	// Read version
	int version;
	char buffer[128];
	fread(&buffer,1,12,fp);
	if (strcmp("kanamemo V2",buffer) == 0) version = 2;
	else if (strcmp("kanamemo V1",buffer) == 0) version = 1;
	else {
		fclose(fp);
		return;
	}

	// Read enabled status
	char temp;
	fread(&temp,1,1,fp);
	enabled[0] = temp == 1;
	fread(&temp,1,1,fp);
	enabled[1] = temp == 1;

	// Read levels
	fread(&temp,1,1,fp);
	level[0] = temp;
	fread(&temp,1,1,fp);
	level[1] = temp;

	// Read autolevel flag
	if (version >= 2) {
		fread(&temp,1,1,fp);
		autoLevel = temp == 1;
	}

	// Read scores
	int temp2;
	fread(&temp2,1,4,fp);
	scores[0].resize(temp2);
	for (int i=0;i<temp2;i++) fread(&scores[0][i],1,4,fp);
	fread(&temp2,1,4,fp);
	scores[1].resize(temp2);
	for (int i=0;i<temp2;i++) fread(&scores[1][i],1,4,fp);

	// Close file
	fclose(fp);

	// Update
	menuCheck[0]->Check(enabled[0]);
	menuCheck[1]->Check(enabled[1]);
	UpdateStatusBar();
	GetNextKana();
	Refresh(false);
}


/////////////////////////
// Click on display area
void GameDisplay::OnClick(wxMouseEvent &event) {
	panel->enterField->SetFocus();
}
