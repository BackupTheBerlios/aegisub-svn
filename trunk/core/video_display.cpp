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


////////////
// Includes
#include "video_display.h"
#include "vfr.h"
#include "ass_file.h"
#include "ass_time.h"
#include "ass_dialogue.h"
#include "subs_grid.h"
#include "vfw_wrap.h"
#include "options.h"
#include "subs_edit_box.h"
#include "audio_display.h"
#include "main.h"
#include "video_slider.h"
#include <wx/image.h>
#include <string.h>
#include <wx/clipbrd.h>
#include <wx/filename.h>
#include <wx/config.h>
#include "FexTracker.h"
#include "FexTrackingFeature.h"
#include "FexMovement.h"


///////
// IDs
enum {
	VIDEO_MENU_COPY_TO_CLIPBOARD = 1230,
	VIDEO_MENU_COPY_COORDS,
	VIDEO_MENU_SAVE_SNAPSHOT,
	VIDEO_PLAY_TIMER
};


///////////////
// Event table
BEGIN_EVENT_TABLE(VideoDisplay, wxWindow)
    EVT_MOUSE_EVENTS(VideoDisplay::OnMouseEvent)
	EVT_LEAVE_WINDOW(VideoDisplay::OnMouseLeave)
    EVT_PAINT(VideoDisplay::OnPaint)

	EVT_TIMER(VIDEO_PLAY_TIMER,VideoDisplay::OnPlayTimer)

	EVT_MENU(VIDEO_MENU_COPY_TO_CLIPBOARD,VideoDisplay::OnCopyToClipboard)
	EVT_MENU(VIDEO_MENU_SAVE_SNAPSHOT,VideoDisplay::OnSaveSnapshot)
	EVT_MENU(VIDEO_MENU_COPY_COORDS,VideoDisplay::OnCopyCoords)
END_EVENT_TABLE()


///////////////
// Constructor
VideoDisplay::VideoDisplay(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
             : wxWindow (parent, id, pos, size, style, name)
{
	provider = NULL;
	curLine = NULL;
	backbuffer = NULL;
	ControlSlider = NULL;
	PositionDisplay = NULL;
	loaded = false;
	frame_n = 0;
	origSize = size;
	arType = 0;
	IsPlaying = false;
	threaded = Options.AsBool(_T("Threaded Video"));
	nextFrame = -1;

	// Create PNG handler
	wxPNGHandler *png = new wxPNGHandler;
	wxImage::AddHandler(png);

	// Set cursor
	wxCursor cursor(wxCURSOR_BLANK);
	SetCursor(cursor);
}


//////////////
// Destructor
VideoDisplay::~VideoDisplay () {
	SetVideo(_T(""));
	delete backbuffer;
}

void  VideoDisplay::UpdateSize() {
	if (provider) {
		w = provider->GetWidth();
		h = provider->GetHeight();

		// Set the size for this control
		SetClientSize(w,h);
		int _w,_h;
		GetSize(&_w,&_h);
		SetSizeHints(_w,_h,_w,_h);
	}
}

///////////////////////
// Sets video filename
void VideoDisplay::SetVideo(const wxString &filename) {
	if (filename == _T("")) {
		delete provider;
		provider = NULL;
		if (VFR_Output.vfr == NULL) VFR_Output.Unload();
		VFR_Input.Unload();

		videoName = _T("");

		frame_n = 0;

		Reset();
	} else {
		SetVideo(_T(""));

		try {
			grid->CommitChanges(true);

			bool usedDirectshow;

			provider = new VideoProvider(filename,grid->GetTempWorkFile(),0.75,usedDirectshow,true);

			// Set keyframes
			if (filename.Right(4).Lower() == _T(".avi"))
				KeyFrames = VFWWrapper::GetKeyFrames(filename);
			else
				KeyFrames.Clear();

			UpdateSize();

			//Gather video parameters
			length = provider->GetFrameCount();
			fps = provider->GetFPS();
			VFR_Input.SetCFR(fps);
			if (!VFR_Output.loaded) VFR_Output.SetCFR(fps,true);

			// Set range of slider
			ControlSlider->SetRange(0,length-1);
			ControlSlider->SetValue(0);

			videoName = filename;

			// Add to recent
			Options.AddToRecentList(filename,_T("Recent vid"));

			RefreshVideo();
		} catch (wxString &e) {
			wxMessageBox(e,_T("Error setting video"),wxICON_ERROR | wxOK);
		}
	}

	loaded = provider != NULL;
	
}

//////////
// Resets
void VideoDisplay::Reset() {
	w = origSize.GetX();
	h = origSize.GetY();
	SetClientSize(w,h);
	int _w,_h;
	GetSize(&_w,&_h);
	SetSizeHints(_w,_h,_w,_h);
}

void VideoDisplay::RefreshSubtitles() {
	provider->RefreshSubtitles();
	RefreshVideo();
}


/////////////////
// OnPaint event
void VideoDisplay::OnPaint(wxPaintEvent& event) {
	wxPaintDC dc(this);

	// Draw frame
	dc.BeginDrawing();
	dc.DrawBitmap(GetFrame(frame_n),0,0);
	dc.EndDrawing();
}


///////////////
// Mouse stuff
void VideoDisplay::OnMouseEvent(wxMouseEvent& event) {
	// Disable when playing
	if (IsPlaying) return;

	// Right click
	if (event.ButtonUp(wxMOUSE_BTN_RIGHT)) {
		wxMenu menu;
		menu.Append(VIDEO_MENU_SAVE_SNAPSHOT,_("Save PNG snapshot"));
		menu.Append(VIDEO_MENU_COPY_TO_CLIPBOARD,_("Copy image to Clipboard"));
		menu.Append(VIDEO_MENU_COPY_COORDS,_("Copy coordinates to Clipboard"));
		PopupMenu(&menu);
		return;
	}

	if( event.ButtonDown(wxMOUSE_BTN_LEFT) )
		bTrackerEditing = 1;
	if( event.ButtonUp(wxMOUSE_BTN_LEFT) )
		bTrackerEditing = 0;

	// Coords
	int x = event.GetX();
	int y = event.GetY();

	// Do tracker influence if needed
	if( bTrackerEditing )
	{
		AssDialogue *curline = grid->GetDialogue(grid->editBox->linen);
		int StartFrame, EndFrame, localframe;
		if( curline 
			&& curline->Tracker 
			&& (StartFrame = VFR_Output.CorrectFrameAtTime(curline->Start.GetMS(),true)) <= frame_n
			&& (EndFrame = VFR_Output.CorrectFrameAtTime(curline->End.GetMS(),false)) >= frame_n 
			&& (localframe = frame_n - StartFrame) < curline->Tracker->GetFrame() 
		) 
		curline->Tracker->InfluenceFeatures( localframe, float(x)/provider->GetZoom(), float(y)/provider->GetZoom(), TrackerEdit );
	}

	// Text of current coords
	int sw,sh;
	GetScriptSize(sw,sh);
	int vx = (sw * x + w/2) / w;
	int vy = (sh * y + h/2) / h;
	wxString text;
	if (!event.ShiftDown()) text = wxString::Format(_T("%i,%i"),vx,vy);
	else text = wxString::Format(_T("%i,%i"),vx - sw,vy - sh);

	// Double click
	if (event.LeftDClick()) {
		grid->editBox->SetOverride(_T("\\pos"),wxString::Format(_T("(%i,%i)"),vx,vy),0);
		grid->editBox->CommitText();
		grid->ass->FlagAsModified();
		grid->CommitChanges();
	}

	// Hover
	if (x != mouse_x || y != mouse_y) {
		// Set coords
		mouse_x = x;
		mouse_y = y;

		// Create backbuffer
		bool needCreate = false;
		if (!backbuffer) needCreate = true;
		else if (backbuffer->GetWidth() != w || backbuffer->GetHeight() != h) {
			needCreate = true;
			delete backbuffer;
		}
		if (needCreate) backbuffer = new wxBitmap(w,h);

		// Prepare drawing
		wxMemoryDC dc;
		dc.SelectObject(*backbuffer);
		dc.BeginDrawing();

		// Draw frame
		dc.DrawBitmap(GetFrame(frame_n),0,0);
		// Draw the control points for FexTracker
		DrawTrackingOverlay( dc );

		dc.SetPen(wxPen(wxColour(255,255,255),1));
		dc.SetLogicalFunction(wxINVERT);

		// Current position info
		if (x >= 0 && x < w && y >= 0 && y < h) {
			// Draw cross
			dc.DrawLine(0,y,w-1,y);
			dc.DrawLine(x,0,x,h-1);

			// Setup text
			wxFont font(10,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_BOLD,false,_T("Verdana"));
			dc.SetFont(font);
			int tw,th;
			GetTextExtent(text,&tw,&th,NULL,NULL,&font);

			// Inversion
			bool left = false;
			bool bottom = false;
			if (x > w/2) left = true;
			if (y < h/2) bottom = true;

			// Text draw coords
			int dx = x,dy = y;
			if (left) dx -= tw + 4;
			else dx += 4;
			if (bottom) dy += 3;
			else dy -= th + 3;

			// Draw text
			dc.SetTextForeground(wxColour(64,64,64));
			dc.DrawText(text,dx+1,dy-1);
			dc.DrawText(text,dx+1,dy+1);
			dc.DrawText(text,dx-1,dy-1);
			dc.DrawText(text,dx-1,dy+1);
			dc.SetTextForeground(wxColour(255,255,255));
			dc.DrawText(text,dx,dy);
		}

		// End
		dc.EndDrawing();

		// Blit to screen
		wxClientDC dcScreen(this);
		dcScreen.BeginDrawing();
		//dcScreen.DrawBitmap(backbuffer,0,0);
		dcScreen.Blit(0,0,w,h,&dc,0,0);
		dcScreen.EndDrawing();
	}
}


//////////////////////
// Mouse left display
void VideoDisplay::OnMouseLeave(wxMouseEvent& event) {
	if (IsPlaying) return;

	bTrackerEditing = 0;

	RefreshVideo();
}


///////////////////////////////////////
// Jumps to a frame and update display
void VideoDisplay::JumpToFrame(int n) {
	// Loaded?
	if (!loaded) return;

	// Prevent intervention during playback
	if (IsPlaying && n != PlayNextFrame) return;

	// Set frame
	GetFrame(n);

	// Display
	RefreshVideo();
	UpdatePositionDisplay();

	// Update slider
	ControlSlider->SetValue(n);

	// Update grid
	if (!IsPlaying && Options.AsBool(_T("Highlight subs in frame"))) grid->UpdateRowColours();
}


////////////////////////////
// Jumps to a specific time
void VideoDisplay::JumpToTime(int ms) {
	JumpToFrame(VFR_Output.CorrectFrameAtTime(ms,true));
}


///////////////////
// Sets zoom level
void VideoDisplay::SetZoom(double value) {
	if (provider) {
		provider->SetZoom(value);
		UpdateSize();
		RefreshVideo();
		GetParent()->Layout();
	}
}


//////////////////////
// Sets zoom position
void VideoDisplay::SetZoomPos(int value) {
	if (value < 0) value = 0;
	if (value > 15) value = 15;
	SetZoom(double(value+1)/8.0);
	if (zoomBox->GetSelection() != value) zoomBox->SetSelection(value);
}


///////////////////
// Sets zoom level
void VideoDisplay::SetAspectRatio(int value) {
	if (provider) {
		if (value == 0)
			provider->SetDAR((float)provider->GetSourceWidth()/(float)provider->GetSourceHeight());
		else if (value == 1)
			provider->SetDAR(4.0/3.0);
		else if (value == 2)
			provider->SetDAR(16.0/9.0);

		UpdateSize();
		RefreshVideo();
		GetParent()->Layout();
	}
}


////////////////////////////
// Updates position display
void VideoDisplay::UpdatePositionDisplay() {
	// Update position display control
	if (!PositionDisplay) {
		throw _T("Position Display not set!");
	}

	// Get time
	int time = VFR_Output.GetTimeAtFrame(frame_n);
	//int time = VFR_Output.CorrectTimeAtFrame(frame_n,false);
	int temp = time;
	int h=0, m=0, s=0, ms=0;
	while (temp >= 3600000) {
		temp -= 3600000;
		h++;
	}
	while (temp >= 60000) {
		temp -= 60000;
		m++;
	}
	while (temp >= 1000) {
		temp -= 1000;
		s++;
	}
	ms = temp;

	// Position display update
	PositionDisplay->SetValue(wxString::Format(_T("%01i:%02i:%02i.%03i - %i"),h,m,s,ms,frame_n));
	if (KeyFrames.Index(frame_n) != wxNOT_FOUND) {
		PositionDisplay->SetBackgroundColour(Options.AsColour(_T("Grid selection background")));
		PositionDisplay->SetForegroundColour(Options.AsColour(_T("Grid selection foreground")));
	}
	else {
		PositionDisplay->SetBackgroundColour(wxNullColour);
		PositionDisplay->SetForegroundColour(wxNullColour);
	}

	// Subs position display update
	UpdateSubsRelativeTime();
}


////////////////////////////////////////////////////
// Updates box with subs position relative to frame
void VideoDisplay::UpdateSubsRelativeTime() {
	// Set variables
	wxString startSign;
	wxString endSign;
	int startOff,endOff;

	// Set start/end
	if (curLine) {
		int time = VFR_Output.GetTimeAtFrame(frame_n);
		startOff = time - curLine->Start.GetMS();
		endOff = time - curLine->End.GetMS();
	}

	// Fallback to zero
	else {
		startOff = 0;
		endOff = 0;
	}

	// Positive signs
	if (startOff > 0) startSign = _T("+");
	if (endOff > 0) endSign = _T("+");

	// Update line
	SubsPosition->SetValue(wxString::Format(_T("%s%ims; %s%ims"),startSign.c_str(),startOff,endSign.c_str(),endOff));
}


/////////////////////
// Copy to clipboard
void VideoDisplay::OnCopyToClipboard(wxCommandEvent &event) {
	if (wxTheClipboard->Open()) {
		wxTheClipboard->SetData(new wxBitmapDataObject(GetFrame(frame_n)));
		wxTheClipboard->Close();
	}
}


/////////////////
// Save snapshot
void VideoDisplay::OnSaveSnapshot(wxCommandEvent &event) {
	SaveSnapshot();
}

void VideoDisplay::SaveSnapshot() {
	// Get path
	wxFileName file = videoName;
	wxString basepath = file.GetPath() + _T("/") + file.GetName();
	wxString path;
	for (int i=0;;i++) {
		path = basepath + wxString::Format(_T("_%03i.png"),i);
		wxFileName tryPath(path);
		if (!tryPath.FileExists()) break;
	}

	// Save
	GetFrame(frame_n).ConvertToImage().SaveFile(path,wxBITMAP_TYPE_PNG);
}


/////////////////////
// Copy coordinates
void VideoDisplay::OnCopyCoords(wxCommandEvent &event) {
	if (wxTheClipboard->Open()) {
		int sw,sh;
		GetScriptSize(sw,sh);
		int vx = (sw * mouse_x + w/2) / w;
		int vy = (sh * mouse_y + h/2) / h;
		wxTheClipboard->SetData(new wxTextDataObject(wxString::Format(_T("%i,%i"),vx,vy)));
		wxTheClipboard->Close();
	}
}


//////////////////
// Draw Tracking Overlay
void VideoDisplay::DrawTrackingOverlay( wxDC &dc )
{
	if( IsPlaying ) return;

	// Get line
	AssDialogue *curline = grid->GetDialogue(grid->editBox->linen);
	if( !curline ) return;

	int StartFrame = VFR_Output.CorrectFrameAtTime(curline->Start.GetMS(),true);
	int EndFrame = VFR_Output.CorrectFrameAtTime(curline->End.GetMS(),false);
	
	if( frame_n<StartFrame || frame_n>EndFrame ) return;

	int localframe = frame_n - StartFrame;

	if( curline->Tracker )
	{
		if( curline->Tracker->GetFrame() <= localframe ) return;

		dc.SetLogicalFunction(wxCOPY);

		for( int i=0;i<curline->Tracker->GetCount();i++ )
		{
			FexTrackingFeature* f = (*curline->Tracker)[i];
			if( f->StartTime > localframe ) continue;
			int llf = localframe - f->StartTime;
			if( f->Pos.size() <= llf ) continue;
			vec2 pt = f->Pos[llf];
			pt.x *= provider->GetZoom();
			pt.y *= provider->GetZoom();
			pt.x = int(pt.x);
			pt.y = int(pt.y);

			dc.SetPen(wxPen(wxColour(255*(1-f->Influence),255*f->Influence,0),1));

			dc.DrawLine( pt.x-2, pt.y, pt.x, pt.y );
			dc.DrawLine( pt.x, pt.y-2, pt.x, pt.y );
			dc.DrawLine( pt.x+1, pt.y, pt.x+3, pt.y );
			dc.DrawLine( pt.x, pt.y+1, pt.x, pt.y+3 );
		}
	}
	else if( curline->Movement )
	{
		if( curline->Movement->Frames.size() <= localframe ) return;

		dc.SetPen(wxPen(wxColour(255,0,0),2));
		FexMovementFrame f = curline->Movement->Frames.lVal[localframe];
		f.Pos.x *= provider->GetZoom();
		f.Pos.y *= provider->GetZoom();
		f.Scale.x *= 30* provider->GetZoom();
		f.Scale.y *= 30* provider->GetZoom();
		dc.DrawLine( f.Pos.x-f.Scale.x, f.Pos.y, f.Pos.x+f.Scale.x+1, f.Pos.y );
		dc.DrawLine( f.Pos.x, f.Pos.y-f.Scale.y, f.Pos.x, f.Pos.y+f.Scale.y+1 );
	}
}


//////////////////
// Refresh screen
void VideoDisplay::RefreshVideo() {
	// Draw frame
	wxClientDC dc(this);
	dc.BeginDrawing();
	dc.DrawBitmap(GetFrame(),0,0);

	// Draw the control points for FexTracker
	DrawTrackingOverlay( dc );

	dc.EndDrawing();
}


//////////////////
// DrawVideoWithOverlay
void VideoDisplay::DrawText( wxPoint Pos, wxString text ) {
	// Draw frame
	wxClientDC dc(this);
	dc.BeginDrawing();
	dc.SetBrush(wxBrush(wxColour(128,128,128),wxSOLID));
	dc.DrawRectangle( 0,0, provider->GetWidth(), provider->GetHeight() );
	dc.SetTextForeground(wxColour(64,64,64));
	dc.DrawText(text,Pos.x+1,Pos.y-1);
	dc.DrawText(text,Pos.x+1,Pos.y+1);
	dc.DrawText(text,Pos.x-1,Pos.y-1);
	dc.DrawText(text,Pos.x-1,Pos.y+1);
	dc.SetTextForeground(wxColour(255,255,255));
	dc.DrawText(text,Pos.x,Pos.y);
	dc.EndDrawing();
}


////////////////////////
// Requests a new frame
wxBitmap VideoDisplay::GetFrame(int n) {
	frame_n = n;
	
	return provider->GetFrame(n);
	RefreshVideo();
}


////////////////////////////
// Get dimensions of script
void VideoDisplay::GetScriptSize(int &sw,int &sh) {
	// Height
	wxString temp = grid->ass->GetScriptInfo(_T("PlayResY"));
	if (temp == _T("") || !temp.IsNumber()) {
		//sh = orig_h;
		sh = 384;
	}
	else {
		long templ;
		temp.ToLong(&templ);
		sh = templ;
	}

	// Width
	temp = grid->ass->GetScriptInfo(_T("PlayResX"));
	if (temp == _T("") || !temp.IsNumber()) {
		sw = 288;
	}
	else {
		long templ;
		temp.ToLong(&templ);
		sw = templ;
	}
}


////////
// Play
void VideoDisplay::Play() {
	// Stop if already playing
	if (IsPlaying) {
		Stop();
		return;
	}

	// Set variables
	IsPlaying = true;
	StartTime = clock();
	PlayTime = StartTime;
	StartFrame = frame_n;
	EndFrame = -1;

	// Start playing audio
	audio->Play(VFR_Output.GetTimeAtFrame(StartFrame),-1);

	// Start timer
	Playback.SetOwner(this,VIDEO_PLAY_TIMER);
	Playback.Start(1);
}


/////////////
// Play line
void VideoDisplay::PlayLine() {
	// Get line
	AssDialogue *curline = grid->GetDialogue(grid->editBox->linen);
	if (!curline) return;

	// Set variables
	IsPlaying = true;
	StartFrame = VFR_Output.CorrectFrameAtTime(curline->Start.GetMS(),true);
	EndFrame = VFR_Output.CorrectFrameAtTime(curline->End.GetMS(),false);

	// Jump to start
	PlayNextFrame = StartFrame;
	JumpToFrame(StartFrame);

	// Set other variables
	StartTime = clock();
	PlayTime = StartTime;

	// Start playing audio
	audio->Play(curline->Start.GetMS(),curline->End.GetMS());

	// Start timer
	Playback.SetOwner(this,VIDEO_PLAY_TIMER);
	Playback.Start(1);
}


////////
// Stop
void VideoDisplay::Stop() {
	Playback.Stop();
	audio->Stop();
	IsPlaying = false;
}


//////////////
// Play timer
void VideoDisplay::OnPlayTimer(wxTimerEvent &event) {
	// Get time difference
	clock_t cur = clock();
	int dif = (clock() - StartTime)*1000/CLOCKS_PER_SEC;
	if (!dif) return;
	PlayTime = cur;

	// Find next frame
	int startMs = VFR_Output.GetTimeAtFrame(StartFrame);
	int nextFrame = frame_n;
	for (int i=0;i<10;i++) {
		if (nextFrame >= length) break;
		if (dif < VFR_Output.GetTimeAtFrame(nextFrame) - startMs) {
			break;
		}
		nextFrame++;
	}

	// Same frame
	if (nextFrame == frame_n) return;

	// End
	if (nextFrame >= length || (EndFrame != -1 && nextFrame > EndFrame)) {
		Stop();
		return;
	}

	// Jump to next frame
	PlayNextFrame = nextFrame;
	JumpToFrame(nextFrame);
}
