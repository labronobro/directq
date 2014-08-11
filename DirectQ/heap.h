/*
Copyright (C) 1996-1997 Id Software, Inc.

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


extern byte *scratchbuf;
#define SCRATCHBUF_SIZE 262144

// fast memcpy and memset replacements
// memcpy and memset in MS CRT are just straight char-by-char operations!!!
void *Q_MemCpy (void *dst, void *src, int size);
void *Q_MemSet (void *dst, int val, int size);

// interface
void Pool_Init (void);

void *Zone_Alloc (int size);
void Zone_FreeMemory (void *ptr);
void Zone_Compact (void);

// wrapper to ensure that the pointer is NULL after a free op
#define Zone_Free(ptr) {Zone_FreeMemory (ptr); ptr = NULL;}


class CQuakeHunk
{
public:
	CQuakeHunk (int maxsizemb);
	~CQuakeHunk (void);
	void *Alloc (int size);
	void Free (void);

	int GetLowMark (void);
	void FreeToLowMark (int mark);

private:
	void Initialize (void);
	int MaxSize;	// maximum memory reserved by this buffer (converted to bytes in constructor)
	int LowMark;	// current memory pointer position
	int HighMark;	// size of all committed memory so far

	char Name[64];

	byte *BasePtr;
};


class CQuakeZone
{
public:
	CQuakeZone (void);
	~CQuakeZone (void);
	void *Alloc (int size);
	void Free (void *data);
	void Compact (void);
	void Discard (void);

private:
	void EnsureHeap (void);
	HANDLE hHeap;
	int Size;
	int Peak;
};


class CQuakeCache
{
public:
	CQuakeCache (void);
	~CQuakeCache (void);
	void *Alloc (int size);
	void *Alloc (void *data, int size);
	void *Alloc (char *name, void *data, int size);
	void *Check (char *name);
	void Flush (void);

private:
	void Init (void);
	CQuakeZone *Heap;
	struct cacheobject_s *Head;
};


// space buffers
extern CQuakeHunk *MainHunk;
extern CQuakeZone *GameZone;
extern CQuakeZone *MapZone;
extern CQuakeCache *MainCache;
extern CQuakeZone *MainZone;
