/*
 * Copyright 2012 Marshmallow Engine. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this list of
 *      conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice, this list
 *      of conditions and the following disclaimer in the documentation and/or other materials
 *      provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY MARSHMALLOW ENGINE ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MARSHMALLOW ENGINE OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Marshmallow Engine.
 */

#include "../backend_p.h"

/*!
 * @file
 *
 * @author Guillermo A. Amaral B. (gamaral) <g@maral.me>
 */

#include <cassert>

#ifdef __APPLE__
#  include <OpenAL/al.h>
#  include <OpenAL/alc.h>
#else
#  include <AL/al.h>
#  include <AL/alc.h>
#endif

#include "core/global.h"
#include "core/identifier.h"
#include "core/logger.h"

#include "game/config.h"

#define OPENAL_BUFFERS_MAX 3

MARSHMALLOW_NAMESPACE_BEGIN
namespace Audio { /****************************************** Audio Namespace */
namespace { /************************** Audio::Backend::<anonymous> Namespace */

	ALCdevice  *s_device;
	ALCcontext *s_context;

} /************************************ Audio::Backend::<anonymous> Namespace */

bool
Backend::Initialize(void)
{
	s_device = alcOpenDevice(NULL);
	s_context = alcCreateContext(s_device, NULL);
	alcMakeContextCurrent(s_context);
	alcProcessContext(s_context);

	alListener3f(AL_POSITION, 0, 0, 0);
	alListener3f(AL_VELOCITY, 0, 0, 0);
	alListener3f(AL_ORIENTATION, 0, 0, -1);

	return(true);
}

void
Backend::Finalize(void)
{
	alcDestroyContext(s_context), s_context = 0;
	alcCloseDevice(s_device), s_device = 0;
}

void
Backend::Tick(float delta)
{
	MMUNUSED(delta);
}

const Core::Identifier &
Backend::Implementation(void)
{
	static const Core::Identifier s_implementation("OpenAL");
	return(s_implementation);
}

namespace Backend { /******************************* Audio::Backend Namespace */

struct PCM::Handle
{
	ALuint source;

	ALenum  format;
	ALsizei rate;
	ALuint  buffers[OPENAL_BUFFERS_MAX];
	uint8_t available;

	char  *buffer;
	size_t buffer_size;
};

PCM::Handle *
PCM::Open(uint32_t sample_rate, uint8_t bit_depth, uint8_t channels)
{
	PCM::Handle *l_handle(0);
	ALuint l_source(0);
	ALenum l_format;

	/*
	 * Translate format
	 */
	if (16 == bit_depth && 2 == channels)
		l_format = AL_FORMAT_STEREO16;
	else if (16 == bit_depth && 1 == channels)
		l_format = AL_FORMAT_MONO16;
	else if (8 == bit_depth && 2 == channels)
		l_format = AL_FORMAT_STEREO8;
	else if (8 == bit_depth && 1 == channels)
		l_format = AL_FORMAT_MONO8;
	else {
		MMERROR("Unsupported bit depth and channel combination.");
		return(0);
	}

	alGenSources(1, &l_source);

	alSourcef(l_source,  AL_PITCH, 1.f);
	alSourcef(l_source,  AL_GAIN,  1.f);
	alSource3f(l_source, AL_POSITION, .0f, .0f, .0f);
	alSource3f(l_source, AL_VELOCITY, .0f, .0f, .0f);
	alSourcei(l_source,  AL_LOOPING, AL_FALSE);

	l_handle = new PCM::Handle;

	l_handle->source = l_source;
	l_handle->format = l_format;
	l_handle->rate = sample_rate;
	l_handle->buffer_size =
	    ((sample_rate * OPENAL_BUFFERS_MAX)/MARSHMALLOW_ENGINE_FRAMERATE) * (bit_depth/8) * channels;
	l_handle->buffer = new char[l_handle->buffer_size];
	l_handle->available = ~0;

	alGenBuffers(OPENAL_BUFFERS_MAX, l_handle->buffers);

	alGetError();

	MMDEBUG("OpenAL PCM device opened. " << bool(l_handle));

	return(l_handle);
}

void
PCM::Close(Handle *pcm_handle)
{
	assert(pcm_handle && "Tried to use invalid PCM device!");

	alSourceStop(pcm_handle->source);
	alDeleteBuffers(OPENAL_BUFFERS_MAX, pcm_handle->buffers);
	alDeleteSources(1, &pcm_handle->source);

	delete[] pcm_handle->buffer, pcm_handle->buffer = 0;
	delete pcm_handle;

	MMDEBUG("OpenAL PCM device closed.");
}

bool
PCM::Write(Handle *pcm_handle, size_t bsize)
{
	assert(pcm_handle && "Tried to use invalid PCM device!");

	int l_buffer(-1);
	ALint l_state(0);

	alGetError();

	alGetSourcei(pcm_handle->source, AL_SOURCE_STATE, &l_state);
	if (alGetError() != AL_NO_ERROR) {
		MMERROR("alGetSourcei(AL_SOURCE_STATE) failed!");
		return(false);
	}

	while (l_state != AL_INITIAL) {
		ALuint l_processed(0);
		alSourceUnqueueBuffers(pcm_handle->source, 1, &l_processed);
		if (alGetError() == AL_NO_ERROR) {
			for (int i = 0; i < OPENAL_BUFFERS_MAX; ++i)
				if (pcm_handle->buffers[i] == l_processed) {
					pcm_handle->available |= static_cast<uint8_t>(1 << i);
					l_buffer = i;
					break;
				}
		}
		else break;
	}

	if (-1 == l_buffer) {
		for (int i = 0; i < OPENAL_BUFFERS_MAX; ++i) {
			if (pcm_handle->available & (1 << i)) {
				l_buffer = i;
				break;
			}
		}

		if (-1 == l_buffer) {
			if (l_state == AL_INITIAL)
				alSourcePlay(pcm_handle->source);
			return(false);
		}
	}

	alBufferData(pcm_handle->buffers[l_buffer],
	             pcm_handle->format,
	             pcm_handle->buffer,
	             static_cast<ALsizei>(bsize),
	             pcm_handle->rate);
	if (alGetError() != AL_NO_ERROR) {
		MMERROR("alBufferData failed! " << l_buffer);
		return(false);
	}

	alSourceQueueBuffers(pcm_handle->source, 1, &pcm_handle->buffers[l_buffer]);
	if (alGetError() != AL_NO_ERROR) {
		MMERROR("alSourceQueueBuffers failed!");
		return(false);
	}

	pcm_handle->available &= static_cast<uint8_t>(~(1 << l_buffer));

	if (l_state == AL_STOPPED ||
	   (l_state == AL_INITIAL && bsize < pcm_handle->buffer_size))
		alSourcePlay(pcm_handle->source);

	return(true);
}

void
PCM::Buffer(Handle *pcm_handle, char *&buffer, size_t &bsize)
{
	assert(pcm_handle && "Tried to use invalid PCM device!");

	buffer = pcm_handle->buffer;
	bsize  = pcm_handle->buffer_size;
}

} /************************************************* Audio::Backend Namespace */
} /********************************************************** Audio Namespace */
MARSHMALLOW_NAMESPACE_END

