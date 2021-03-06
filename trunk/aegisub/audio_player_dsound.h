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


///////////
// Headers
#include "setup.h"
#if USE_DIRECTSOUND == 1
#include "audio_player.h"
#include <dsound.h>


//////////////
// Prototypes
class DirectSoundPlayer;


//////////
// Thread
class DirectSoundPlayerThread : public wxThread {
private:
	DirectSoundPlayer *parent;

public:
	bool alive;
	DirectSoundPlayerThread(DirectSoundPlayer *parent);
	~DirectSoundPlayerThread();

	wxThread::ExitCode Entry();
};


////////////////////
// Portaudio player
class DirectSoundPlayer : public AudioPlayer {
	friend class DirectSoundPlayerThread;

private:
	wxMutex DSMutex;

	bool playing;
	float volume;
	int offset;
	int bufSize;

	volatile __int64 playPos;
	volatile __int64 startPos;
	volatile __int64 endPos;

	IDirectSound8 *directSound;
	IDirectSoundBuffer8 *buffer;
	HANDLE notificationEvent;

	void FillBuffer(bool fill);

	DirectSoundPlayerThread *thread;
	bool threadRunning;

public:
	DirectSoundPlayer();
	~DirectSoundPlayer();

	void OpenStream();
	void CloseStream();

	void Play(__int64 start,__int64 count);
	void Stop(bool timerToo=true);
	bool IsPlaying() { return playing; }

	__int64 GetStartPosition() { return startPos; }
	__int64 GetEndPosition() { return endPos; }
	__int64 GetCurrentPosition();
	void SetEndPosition(__int64 pos);
	void SetCurrentPosition(__int64 pos);

	void SetVolume(double vol) { volume = vol; }
	double GetVolume() { return volume; }

	wxMutex *GetMutex() { return &DSMutex; }
};

#endif
