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

// refresh.h -- public interface to refresh functions

#define	MAXCLIPPLANES	11

#define	TOP_RANGE		16			// soldier uniform colors
#define	BOTTOM_RANGE	96

//=============================================================================

typedef struct aliascache_s
{
	// all the info we need to draw the model is here
	float blend[2];

	struct image_s *teximage;
	struct image_s *lumaimage;
	vec3_t lightspot;
	struct mplane_s *lightplane;
} aliasstate_t;


#define LERP_MOVESTEP	(1 << 0) // this is a MOVETYPE_STEP entity, enable movement lerp
#define LERP_RESETANIM	(1 << 1) // disable anim lerping until next anim frame
#define LERP_RESETANIM2	(1 << 2) // set this and previous flag to disable anim lerping for two anim frames
#define LERP_RESETMOVE	(1 << 3) // disable movement lerping until next origin/angles change
#define LERP_FINISH		(1 << 4) // use lerpfinish time from server update instead of assuming interval of 0.1

#define LERP_CURR	0
#define LERP_LAST	1

typedef struct entity_s
{
	vec3_t				mins, maxs;		// bounding box used to cull the entity
	vec3_t				trueorigin;
	vec3_t				bboxscale;

	bool				forcelink;		// model changed

	int						update_type;

	entity_state_t			baseline;		// to fill in defaults in updates

	double					msgtime;		// so binary comparison with cl.mtime[0] is valid

	vec3_t					oldorg;			// for particle spawning

	vec3_t					msg_origins[2];	// last two updates (0 is newest)
	vec3_t					origin;
	vec3_t					msg_angles[2];	// last two updates (0 is newest)
	vec3_t					angles;
	struct model_s			*model;			// NULL = no model
	struct efrag_s			*efrag;
	int						frame;
	float					syncbase;		// for client-side animations
	float					skinbase;
	float					posebase;
	int						effects;		// light, particals, etc
	int						skinnum;		// for Alias models
	int						visframe;		// last frame this entity was found in an active leaf
	int						relinkframe;	// static entities only; frame when added to the visedicts list
	vec3_t					modelorg;		// relative to r_origin

	// occlusion queries
	bool					Occluded;

	// allocation sequence number in the current map
	int						allocnum;

	// player skins
	int						playerskin;

	// allocated at runtime client and server side
	int						entnum;

	// sort order for MDLs
	union
	{
		unsigned int sortorder;
		unsigned short sortpose[2];
	};

	// interpolation
	float		framestarttime;
	int			lerppose[2];

	float		translatestarttime;
	vec3_t		lerporigin[2];

	float		rotatestarttime;
	vec3_t		lerpangles[2];

	// allows an alpha value to be assigned to any entity
	int			alphaval;
	float		lerpinterval;
	int			lerpflags;

	// light averaging
	float		shadelight[3];

	// false if the entity is to be subjected to bbox culling
	bool		nocullbox;

	// distance from client (for depth sorting)
	float		dist;

	// FIXME: could turn these into a union
	// done (trivial_accept is only on alias models, topnode only on brush models)
	union
	{
		int						trivial_accept;
		struct mnode_s			*topnode;		// for bmodels, first world node
	};
	//  that splits bmodel, or NULL if
	//  not split

	// the matrix used for transforming this entity
	// D3DXMATRIX in *incredibly* unhappy living in an entity_t struct...
	D3DMATRIX		matrix;

	// check transforms for all model types
	bool			rotated;

	// cached info about models
	// these can't be in a union as the entity slot could be reused for a different model type
	aliasstate_t		aliasstate;
	struct mspriteframe_s *currspriteframe;
} entity_t;


// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	// so much of this is just crap that's no longer needed
	/*
	int			vrectright, vrectbottom;	// right & bottom screen coords
	int			aliasvrectright, aliasvrectbottom;	// scaled Alias versions
	float		vrectrightedge;			// rightmost right edge we care about,
	//  for use in edge list
	float		fvrectx, fvrecty;		// for floating-point compares
	float		fvrectx_adj, fvrecty_adj; // left and top edges, for clamping
	int			vrect_x_adj_shift20;	// (vrect.x + 0.5 - epsilon) << 20
	int			vrectright_adj_shift20;	// (vrectright + 0.5 - epsilon) << 20
	float		fvrectright_adj, fvrectbottom_adj;
	// right and bottom edges, for clamping
	float		fvrectright;			// rightmost edge, for Alias clamping
	float		fvrectbottom;			// bottommost edge, for Alias clamping
	float		horizontalFieldOfView;	// at Z = 1.0, this many X is visible
	// 2.0 = 90 degrees
	float		xOrigin;			// should probably allways be 0.5
	float		yOrigin;			// between be around 0.3 to 0.5
	*/

	vec3_t		vieworigin;
	vec3_t		viewangles;

	float		fov_x, fov_y;
} refdef_t;


// refresh
extern	int		reinit_surfcache;


extern	refdef_t	r_refdef;

typedef struct r_viewvecs_s
{
	vec3_t forward;
	vec3_t up;
	vec3_t right;
} r_viewvecs_t;

extern	r_viewvecs_t	r_viewvectors;


extern	struct texture_s	*r_notexture_mip;


void R_Init (void);
void R_InitTextures (void);
void R_NewMap (void);

void R_ParseParticleEffect (void);
void R_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count);
void R_RocketTrail (vec3_t start, vec3_t end, int type);

#ifdef QUAKE2
void R_DarkFieldParticles (entity_t *ent);
#endif
void R_EntityParticles (entity_t *ent);
void R_BlobExplosion (vec3_t org);
void R_ParticleExplosion (vec3_t org);
void R_ParticleExplosion2 (vec3_t org, int colorStart, int colorLength);
void R_LavaSplash (vec3_t org);
void R_TeleportSplash (vec3_t org);


// surface cache related
extern	int		reinit_surfcache;	// if 1, surface cache is currently empty and

int	D_SurfaceCacheForRes (int width, int height);
void D_DeleteSurfaceCache (void);
void D_InitCaches (void *buffer, int size);

