/*
Copyright (C) 2007 Peter Mackay and Chris Swindle.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <cstddef>
#include <stdio.h>

#include <pspaudiolib.h>

#include <mad.h>

extern "C"
{
#include "../quakedef.h"
}

extern 	cvar_t bgmtype;
extern	cvar_t bgmvolume;


namespace quake
{
	namespace cd
	{
		struct Sample
		{
			short left;
			short right;
		};

		static int				file = -1;
		
		static int		     last_track = 1;
		static mad_stream		stream;
		static mad_frame		frame;
		static mad_synth		synth;
		static unsigned char	fileBuffer[8192];
		static std::size_t		samplesRead;

		static bool	 playing  = false;
		static bool	 paused   = false;
		static bool	 enabled  = false;
		static float cdvolume = 0;
	

		static bool fillFileBuffer()
		{
			// Should be playing a track but aren't?
			//Con_Printf("fillFileBuffer: ");
			if (file >= 0) {
	
				playing = true;

			// Find out how much to keep and how much to fill.
			const std::size_t bytesToKeep = stream.bufend - stream.next_frame;
			std::size_t bytesToFill = sizeof(fileBuffer) - bytesToKeep;

			// Want to keep any bytes?
			if (bytesToKeep)
			{
				// Copy the tail to the head.
				memmove(fileBuffer, fileBuffer + sizeof(fileBuffer) - bytesToKeep, bytesToKeep);
			}

			// Read into the rest of the file buffer.
			unsigned char* bufferPos = fileBuffer + bytesToKeep;
				
			//Con_Printf(" bytes to fill %d \n",bytesToFill);
			while (bytesToFill > 0)
			{
				// Read some.
					const std::size_t bytesRead = Sys_FileRead(file , bufferPos, bytesToFill);

					//Con_Printf("    bytes read: %d \n",bytesRead);
				// EOF?
					if (bytesRead == 0)
				{
						Sys_FileSeek(file, 0);
					continue;
				
				}

				// Adjust where we're writing to.
					bytesToFill -= bytesRead;
					bufferPos += bytesRead;
			}

			} else {
				playing = false;
				return false;			
			}
			return true;
		}

		static void decode()
		{
			// While we need to fill the buffer...
			while (
				(mad_frame_decode(&frame, &stream) == -1) &&
				((stream.error == MAD_ERROR_BUFLEN) || (stream.error == MAD_ERROR_BUFPTR))
				)
			{
				// Fill up the remainder of the file buffer.
				fillFileBuffer();

				// Give new buffer to the stream.
				mad_stream_buffer(&stream, fileBuffer, sizeof(fileBuffer));
			}

			// Synth the frame.
			mad_synth_frame(&synth, &frame);
		}

		static inline short convertSample(mad_fixed_t sample)
		{
			/* round */
			sample += (1L << (MAD_F_FRACBITS - 16));

			/* clip */
			if (sample >= MAD_F_ONE)
				sample = MAD_F_ONE - 1;
			else if (sample < -MAD_F_ONE)
				sample = -MAD_F_ONE;

			/* quantize */
			return sample >> (MAD_F_FRACBITS + 1 - 16);
		}

		static void convertLeftSamples(Sample* first, Sample* last, const mad_fixed_t* src)
		{
			for (Sample* dst = first; dst != last; ++dst)
			{
				dst->left = convertSample(*src++);
			}
		}

		static void convertRightSamples(Sample* first, Sample* last, const mad_fixed_t* src)
		{
			for (Sample* dst = first; dst != last; ++dst)
			{
				dst->right = convertSample(*src++);
			}
		}

		static void fillOutputBuffer(void* buffer, unsigned int samplesToWrite, void* userData)
		{
			//Con_Printf("fillOutputBuffer\n");
			if (playing == false || paused == true)
				return;

			// Where are we writing to?
			Sample* destination = static_cast<Sample*> (buffer);

			// While we've got samples to write...
			while (samplesToWrite > 0)
			{
				// Enough samples available?
				const unsigned int samplesAvailable = synth.pcm.length - samplesRead;
				if (samplesAvailable > samplesToWrite)
				{
					// Write samplesToWrite samples.
					convertLeftSamples(destination, destination + samplesToWrite, &synth.pcm.samples[0][samplesRead]);
					convertRightSamples(destination, destination + samplesToWrite, &synth.pcm.samples[1][samplesRead]);

					// We're still using the same PCM data.
					samplesRead += samplesToWrite;

					// Done.
					samplesToWrite = 0;
				}
				else
				{
					// Write samplesAvailable samples.
					convertLeftSamples(destination, destination + samplesAvailable, &synth.pcm.samples[0][samplesRead]);
					convertRightSamples(destination, destination + samplesAvailable, &synth.pcm.samples[1][samplesRead]);

					// We need more PCM data.
					samplesRead = 0;
					decode();

					// We've still got more to write.
					destination += samplesAvailable;
					samplesToWrite -= samplesAvailable;
				}
			}
		}
	}
}

using namespace quake;
using namespace quake::cd;

static void CD_f (void)
{
	char	*command;

	if (Cmd_Argc() < 2)
	{
		Con_Printf("commands:");
		Con_Printf("on, off, reset, remap, \n");
		Con_Printf("play, stop, loop, pause, resume\n");
		Con_Printf("eject, close, info\n");
		return;
	}

	command = Cmd_Argv (1);

	if (Q_strcasecmp(command, "on") == 0)
	{
		enabled = true;
		return;
	}

	if (Q_strcasecmp(command, "off") == 0)
	{
		if (playing)
			CDAudio_Stop();
		enabled = false;
		return;
	}

	if (Q_strcasecmp(command, "reset") == 0)
	{
		enabled = true;
		if (playing)
			CDAudio_Stop();
		return;
	}

	if (Q_strcasecmp(command, "remap") == 0)
	{
		return;
	}

	if (Q_strcasecmp(command, "close") == 0)
	{
		return;
	}

	if (Q_strcasecmp(command, "play") == 0)
	{
		CDAudio_Play((byte)atoi(Cmd_Argv (2)), (qboolean) false);
		return;
	}

	if (Q_strcasecmp(command, "loop") == 0)
	{
		CDAudio_Play((byte)atoi(Cmd_Argv (2)), (qboolean) true);
		return;
	}

	if (Q_strcasecmp(command, "stop") == 0)
	{
		CDAudio_Stop();
		return;
	}

	if (Q_strcasecmp(command, "pause") == 0)
	{
		CDAudio_Pause();
		return;
	}

	if (Q_strcasecmp(command, "resume") == 0)
	{
		CDAudio_Resume();
		return;
	}

	if (Q_strcasecmp(command, "eject") == 0)
	{
		if (playing)
			CDAudio_Stop();
		return;
	}

	if (Q_strcasecmp(command, "info") == 0)
	{
		return;
	}
}

void CDAudio_VolumeChange(float bgmvolume)
{
	int volume = (int) (bgmvolume * (float) PSP_VOLUME_MAX);
	pspAudioSetVolume(1, volume, volume);
	cdvolume = bgmvolume;
}

void CDAudio_Play(byte track, qboolean looping)
{
	last_track = track;
	CDAudio_Stop();

	char path[256];
	sprintf(path, "%s\\MP3\\%02u.mp3", host_parms.basedir, track);
	
	Sys_FileOpenRead(path, &file);
	if (file >= 0)
	{
		Con_Printf("Playing %s\n", path);
		playing = true;
	}
	else
	{
		Con_Printf("Couldn't find %s\n", path);
		playing = false;
		CDAudio_VolumeChange(0);
	}
	

	CDAudio_VolumeChange(bgmvolume.value);
}

void CDAudio_Stop(void)
{
	if (file >= 0)
	{
		Sys_FileClose(file);
		file = -1;
		playing = false;
		CDAudio_VolumeChange(0);
	}
}

void CDAudio_Pause(void)
{
	CDAudio_VolumeChange(0);
	paused = true;
}

void CDAudio_Resume(void)
{
	CDAudio_VolumeChange(bgmvolume.value);
	paused = false;
}

void CDAudio_Update(void)
{
	if (bgmvolume.value != cdvolume)
		CDAudio_VolumeChange(bgmvolume.value);

	
	// Fill the input buffer.
	if (strcmpi(bgmtype.string,"cd") == 0) {
		if (playing == false) {
			CDAudio_Play(last_track, (qboolean) false);
		}
		if (paused == true) {
			CDAudio_Resume();
		}
	
	} else {
		if (paused == false) {
			CDAudio_Pause();
		}
		if (playing == true) {
			CDAudio_Stop();
		}
	}
}

int CDAudio_Init(void)
{
	// Initialise MAD.
	mad_stream_init(&stream);
	mad_frame_init(&frame);
	mad_synth_init(&synth);

	// Set the channel callback.
	// Sound effects use channel 0, CD audio uses channel 1.
	pspAudioSetChannelCallback(1, fillOutputBuffer, 0);
	enabled = true;
	
	Cmd_AddCommand ("cd", CD_f);
	
	Con_Printf("CD Audio Initialized\n");

	return 0;
}

void CDAudio_Shutdown(void)
{
	CDAudio_Stop();

	// Clear the channel callback.
	pspAudioSetChannelCallback(1, 0, 0);

	// Shut down MAD.
	mad_synth_finish(&synth);
	mad_frame_finish(&frame);
	mad_stream_finish(&stream);
}

