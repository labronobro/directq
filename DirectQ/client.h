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

// client.h

// R_RocketTrail types (yayy!  self-documenting code!)
#define RT_ROCKET			0
#define RT_GRENADE			1
#define RT_GIB				2
#define RT_WIZARD			3
#define RT_ZOMGIB			4
#define RT_KNIGHT			5
#define RT_VORE				6


// if this is changed then CL_SendCmd in cl_main.cpp must be updated to reflect it!!!!
typedef struct
{
	// intended velocities
	float	forwardmove;
	float	sidemove;
	float	upmove;
} usercmd_t;

typedef struct
{
	int		length;

	// this was a char but could give negative array indexes if a mod author uses a lightstyle map > 127, so we make it a byte
	// instead and convert it back to the signed char equivalent in our lookup table
	byte	map[MAX_STYLESTRING];
} lightstyle_t;

typedef struct
{
	char	name[MAX_SCOREBOARDNAME];
	float	entertime;
	int		frags;
	int		colors;			// two 4 bit fields
	int		ping;			// JPG - added this
	int		addr;			// JPG - added this
} scoreboard_t;

// JPG - added this for teamscore status bar
typedef struct
{
	int colors;
	int frags;
} teamscore_t;

typedef struct
{
	int		destcolor[3];
	int		percent;		// 0-256
} cshift_t;

// add an extra shift for v_cshift commands so that they don't mess with contents
#define	CSHIFT_CONTENTS	0
#define	CSHIFT_DAMAGE	1
#define	CSHIFT_BONUS	2
#define	CSHIFT_POWERUP	3
#define CSHIFT_VCSHIFT	4

#define	NUM_CSHIFTS		5

#define	NAME_LENGTH	64


// client_state_t should hold all pieces of the client state

#define	SIGNON_CONNECTED		4			// signon messages to receive before connected

#define DLF_NOCORONA	1	// set if the dynamic light is not to have a corona
#define DLF_KILL		2	// kill the light automatically after 1 frame

typedef struct
{
	float	origin[3];
	float	transformed[3];
	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	int		key;
	int		flags;
	int		visframe;

	// dlights only need to be updated when they're dirtied and the time they are dirtied at is recorded
	bool dirty;
	double dirtytime;

	// coloured light
	unsigned short rgb[3];
} dlight_t;


#define	MAX_MAPSTRING	2048
#define	MAX_DEMOS		64
#define	MAX_DEMONAME	64

typedef enum
{
	ca_disconnected, 	// full screen console with no connection
	ca_connected,		// valid netcon, talking to a server
	ca_runningmap		// everything up and running
} cactive_t;


typedef struct
{
	bool			web;
	char			*name;
	int				percent;
	bool			disconnect;			// set when user tries to disconnect, to allow cleaning up webdownload
} download_t;


// the client_static_t structure is persistant through an arbitrary number
// of server connections
typedef struct
{
	cactive_t	state;

	// personalization data sent to server
	char		mapstring[MAX_QPATH];
	char		spawnparms[MAX_MAPSTRING];	// to restart a level

	// true if a map is running
	bool		maprunning;

	// demo loop control
	int			demonum;		// -1 = don't play demos
	char		demos[MAX_DEMOS][MAX_DEMONAME];		// when not playing

	// demo recording info must be here, because record is started before
	// entering a map (and clearing client_state_t)
	bool	demorecording;
	bool	demoplayback;
	bool	timedemo;
	char	demoname[MAX_DEMONAME];	// current demo
	int			forcetrack;			// -1 = use normal cd track
	int			td_currframe;		// to meter out one message a frame
	double		td_starttime;		// realtime at second frame of timedemo

	download_t	download;

	// connection information
	int			signon;			// 0 to SIGNONS
	struct qsocket_s	*netcon;
	sizebuf_t	message;		// writing buffer to send to server
} client_static_t;

extern client_static_t	cls;

// the client_state_t structure is wiped completely at every
// server signon
typedef struct client_state_s
{
	int			movemessages;	// since connecting to this server
	// throw out the first couple, so the player
	// doesn't accidentally do something the
	// first frame
	usercmd_t	cmd;			// last command sent to the server

	// information for local display
	int			stats[MAX_CL_STATS];	// health, etc
	int			items;			// inventory bit flags
	float		itemgettime[32];	// cl.time of aquiring item, for blinking
	float		faceanimtime;	// use anim frame if cl.time < this

	// don't need previous with hardware because it's not a perf saver
	cshift_t	cshifts[NUM_CSHIFTS];	// color shifts for damage, powerups and content types

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  The server sets punchangle when
	// the view is temporarliy offset, and an angle reset commands at the start
	// of each level and after teleporting.
	vec3_t		mviewangles[2];	// during demo playback viewangles is lerped
								// between these
	vec3_t		viewangles;

	vec3_t		mvelocity[2];	// update by server, used for lean+bob
								// (0 is newest)
	vec3_t		velocity;		// lerped between mvelocity[0] and [1]

	vec3_t		punchangle;		// temporary offset

	float		viewheight;
	float		crouch;			// local amount for smoothing stepups

	bool	paused;			// send over by server
	bool	onground;
	bool	inwater;

	int			intermission;	// don't change view angle, full screen, etc
	int			completed_time;	// latched at intermission start

	double		mtime[2];		// keep message times steady

	double		oldtime;
	double		time;			// clients view of time, should be between
								// servertime and oldservertime to generate
								// a lerp point for other data

	float		idealpitch;
	float		pitchvel;
	bool		nodrift;
	float		driftmove;
	double		laststop;

	double		nexteffecttime;	// the next cl.time at which effects will be spawned
	double		frametime;		// cl.time - cl.oldtime

	float		lastrecievedmessage;	// (realtime) for net trouble icon

	// information that is static for the entire time connected to a server
	struct model_s		**model_precache;
	struct sfx_s		**sound_precache;

	char		*levelname;	// for display on solo scoreboard
	int			viewentity;		// cl_entitites[cl.viewentity] = player
	int			maxclients;
	int			gametype;

	int			Protocol;
	unsigned	PrototcolFlags;

	// refresh related state
	struct model_s	*worldmodel;	// cl_entitites[0].model

	int			num_entities;	// held in cl_entities array

	entity_t	viewent;			// the gun model

	int			cdtrack, looptrack;	// cd audio

	// frag scoreboard
	scoreboard_t	*scores;		// [cl.maxclients]

	teamscore_t		*teamscores;		// [13] - JPG for teamscores in status bar
	bool			teamgame;			// JPG = true for match, false for individual
	int				minutes;			// JPG - for match time in status bar
	int				seconds;			// JPG - for match time in status bar
	float			last_match_time;	// JPG - last time match time was obtained
	float			lastpingtime;		// JPG - last time pings were obtained
	bool			console_ping;		// JPG 1.05 - true if the ping came from the console
	float			laststatustime;		// JPG 1.05 - last time status was obtained
	bool			console_status;		// JPG 1.05 - true if the status came from the console
	float			match_pause_time;	// JPG - time that match was paused (or 0)
	vec3_t			lerpangles;			// JPG - angles now used by view.c so that smooth chasecam doesn't fuck up demos
	vec3_t			death_location;		// JPG 3.20 - used for %d formatting
} client_state_t;


// cvars
extern	cvar_t	cl_name;
extern	cvar_t	cl_color;

extern	cvar_t	cl_upspeed;
extern	cvar_t	cl_forwardspeed;
extern	cvar_t	cl_backspeed;
extern	cvar_t	cl_sidespeed;

extern	cvar_t	cl_movespeedkey;

extern	cvar_t	cl_yawspeed;
extern	cvar_t	cl_pitchspeed;

extern	cvar_t	cl_anglespeedkey;

extern	cvar_t	cl_autofire;

extern	cvar_t	cl_shownet;
extern	cvar_t	cl_nolerp;

extern  cvar_t  cl_autoaim;

extern	cvar_t	cl_pitchdriftspeed;
extern	cvar_t	lookspring;
extern	cvar_t	lookstrafe;
extern	cvar_t	sensitivity;

extern	cvar_t	m_pitch;
extern	cvar_t	m_yaw;
extern	cvar_t	m_forward;
extern	cvar_t	m_side;


extern	client_state_t	cl;

// FIXME, allocate dynamically
extern	entity_t		**cl_entities;
extern	lightstyle_t	*cl_lightstyle;
extern	dlight_t		*cl_dlights;


//=============================================================================

// cl_main
void CL_DecayLights (void);
void CL_PrepEntitiesForRendering (void);

void CL_Init (void);

void CL_EstablishConnection (char *host);
void CL_Signon1 (void);
void CL_Signon2 (void);
void CL_Signon3 (void);
void CL_Signon4 (void);

void CL_Disconnect (void);
void CL_Disconnect_f (void);
void CL_NextDemo (void);

// bumped for new value of MAX_EDICTS
#define			MAX_VISEDICTS	65536

// cl_input
typedef struct
{
	int		down[2];		// key nums holding it down
	int		state;			// low bit is down state
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

void CL_InitInput (void);
void CL_SendCmd (double frametime);
void CL_SendMove (usercmd_t *cmd);

void CL_ParseTEnt (void);
void CL_UpdateTEnts (void);

void CL_ClearState (void);


void CL_UpdateClient (double frametime, bool readfromserver);
void CL_WriteToServer (usercmd_t *cmd);


float CL_KeyState (kbutton_t *key);
char *Key_KeynumToString (int keynum);

// cl_demo.c
void CL_StopPlayback (void);
int CL_GetMessage (void);

void CL_Stop_f (void);
void CL_Record_f (void);
void CL_PlayDemo_f (void);
void CL_TimeDemo_f (void);

// cl_parse.c
void CL_ParseServerMessage (void);

void V_RenderView (void);
void V_UpdateCShifts (void);
void V_Register (void);
void V_ParseDamage (void);
void V_SetContentsColor (int contents);


// cl_tent
void CL_InitTEnts (void);
void CL_SignonReply (void);


#define CLEAR_POSES		1
#define CLEAR_ORIGIN	2
#define CLEAR_ANGLES	4
#define CLEAR_ORIENT	(CLEAR_ORIGIN | CLEAR_ANGLES)
#define CLEAR_ALLLERP	(CLEAR_POSES | CLEAR_ORIGIN | CLEAR_ANGLES)

void CL_ClearInterpolation (entity_t *ent, int clearflags);

