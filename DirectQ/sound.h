/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

// sound.h -- client sound i/o functions

#ifndef __SOUND__
#define __SOUND__

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

typedef enum {SIS_SUCCESS, SIS_FAILURE, SIS_NOTAVAIL} sndinitstat;


// !!! if this is changed, it much be changed in asm_i386.h too !!!
typedef struct portable_samplepair_s
{
	int left;
	int right;
} portable_samplepair_t;

// !!! if this is changed, it much be changed in asm_i386.h too !!!
typedef struct sfxcache_s
{
	int 	length;
	int 	loopstart;
	int 	speed;
	int 	stereo;
	int		memsize;
	short	data[1];		// variable sized
} sfxcache_t;


typedef struct sfx_s
{
	char 	name[MAX_QPATH];
	sfxcache_t *sndcache;
} sfx_t;

extern CQuakeCache *SoundCache;
extern CQuakeZone *SoundHeap;

typedef struct dms_s
{
	bool		gamealive;
	bool		soundalive;
	bool		splitbuffer;
	int				samples;				// mono samples in buffer
	int				submission_chunk;		// don't mix less than this #
	int				samplepos;				// in mono samples
	int				speed;
	unsigned char	*buffer;
} dma_t;

// !!! if this is changed, it much be changed in asm_i386.h too !!!
typedef struct channel_s
{
	sfx_t	*sfx;			// sfx number
	int		leftvol;		// 0-255 volume
	int		rightvol;		// 0-255 volume
	int		end;			// end time in global paintsamples
	int 	pos;			// sample position in sfx
	int		looping;		// where to loop, -1 = no looping
	int		entnum;			// to allow overriding a specific sound
	int		entchannel;
	vec3_t	origin;			// origin of sound effect
	vec_t	dist_mult;		// distance multiplier (attenuation/clipK)
	int		master_vol;		// 0-255 master volume
} channel_t;

typedef struct wavinfo_s
{
	int		rate;
	int		width;
	int		channels;
	int		byterate;
	int		blockalign;
	int		loopstart;
	int		samples;
	int		dataofs;		// chunk starts this many bytes from file start
} wavinfo_t;

void S_Init (void);
void S_Startup (void);
void S_Shutdown (void);
void S_StartSound (int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol,  float attenuation);
void S_StaticSound (sfx_t *sfx, vec3_t origin, float vol, float attenuation);
void S_StopSound (int entnum, int entchannel);
void S_StopAllSounds (bool clear);
void S_ClearBuffer (void);
void S_Update (double frametime, vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);

sfx_t *S_PrecacheSound (char *sample);
void S_TouchSound (char *sample);
void S_ClearPrecache (void);
void S_BeginPrecaching (void);
void S_EndPrecaching (void);
void S_PaintChannels (int endtime);
void S_InitPaintChannels (void);

// picks a channel based on priorities, empty slots, number of channels
channel_t *SND_PickChannel (int entnum, int entchannel);

// spatializes a channel
void SND_Spatialize (channel_t *ch);

// initializes cycling through a DMA buffer and returns information on it
bool SNDDMA_Init (void);

// gets the current DMA position
int SNDDMA_GetDMAPos (void);

// shutdown the DMA xfer.
void SNDDMA_Shutdown (void);

// ====================================================================
// User-setable variables
// ====================================================================

#define	MAX_CHANNELS			1028
#define	MAX_DYNAMIC_CHANNELS	512


// 0 to NUM_AMBIENTS -1 = water, etc
// NUM_AMBIENTS to MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS - 1	= normal entity sounds
// MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS to total_channels = static sounds

extern	channel_t   channels[MAX_CHANNELS];

extern	int			total_channels;

extern int		paintedtime;
extern vec3_t listener_origin;
extern vec3_t listener_forward;
extern vec3_t listener_right;
extern vec3_t listener_up;
extern volatile dma_t *shm;
extern volatile dma_t sn;

extern	cvar_t bgmvolume;
extern	cvar_t volume;

extern bool	snd_initialized;

extern int		snd_blocked;

void S_LocalSound (char *s);
sfxcache_t *S_LoadSound (sfx_t *s);
void S_BlockSound (bool block);

wavinfo_t GetWavinfo (char *name, byte *wav, int wavlength);

void S_AmbientOff (void);
void S_AmbientOn (void);

bool S_GetBufferLock (DWORD dwOffset, DWORD dwBytes, void **pbuf, DWORD *dwSize, void **pbuf2, DWORD *dwSize2, DWORD dwFlags);

#endif
