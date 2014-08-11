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
// r_main.c

#include "quakedef.h"
#include "d3d_model.h"
#include "d3d_quake.h"

// use unit scales that are also used in most map editors
cvar_t r_automapscroll_x ("r_automapscroll_x", -64.0f, CVAR_ARCHIVE);
cvar_t r_automapscroll_y ("r_automapscroll_y", -64.0f, CVAR_ARCHIVE);
cvar_t r_automapscroll_z ("r_automapscroll_z", 32.0f, CVAR_ARCHIVE);

bool r_automap;
extern bool scr_drawmapshot;

// default automap viewpoint
// x and y are updated on entry to the automap
float r_automap_x = 0;
float r_automap_y = 0;
float r_automap_z = 0;
float r_automap_scale = 5;
int automap_key = -1;
int screenshot_key = -1;

void Cmd_ToggleAutomap_f (void)
{
	if (cls.state != ca_connected) return;

	r_automap = !r_automap;

	// toggle a paused state
	if (key_dest == key_automap)
		key_dest = key_game;
	else key_dest = key_automap;

	// find which key is bound to the automap
	automap_key = Key_GetBinding ("toggleautomap");

	// find any other key defs we want to allow
	screenshot_key = Key_GetBinding ("screenshot");

	r_automap_x = r_refdef.vieworg[0];
	r_automap_y = r_refdef.vieworg[1];
	r_automap_z = 0;
}


void Key_Automap (int key)
{
	switch (key)
	{
	case K_PGUP:
		r_automap_z += r_automapscroll_z.value;
		break;

	case K_PGDN:
		r_automap_z -= r_automapscroll_z.value;
		break;

	case K_UPARROW:
		r_automap_y -= r_automapscroll_y.value;
		break;

	case K_DOWNARROW:
		r_automap_y += r_automapscroll_y.value;
		break;

	case K_LEFTARROW:
		r_automap_x += r_automapscroll_x.value;
		break;

	case K_RIGHTARROW:
		r_automap_x -= r_automapscroll_x.value;
		break;

	case K_HOME:
		r_automap_scale -= 0.5f;
		break;

	case K_END:
		r_automap_scale += 0.5f;
		break;

	default:
		if (key == automap_key)
			Cmd_ToggleAutomap_f ();
		else if (key == screenshot_key)
		{
			Cbuf_InsertText ("screenshot\n");
			Cbuf_Execute ();
		}
		break;
	}

	if (r_automap_scale < 0.5) r_automap_scale = 0.5;
	if (r_automap_scale > 20) r_automap_scale = 20;
}


cmd_t Cmd_ToggleAutomap ("toggleautomap", Cmd_ToggleAutomap_f);

int c_automapsurfs = 0;

bool D3D_DrawAutomap (void)
{
	if (!r_automap) return false;
	if (cls.state != ca_connected) return false;
	if (scr_drawmapshot) return false;
	if (cl.intermission == 1 && key_dest == key_game) return false;
	if (cl.intermission == 2 && key_dest == key_game) return false;

	c_automapsurfs = 0;
	return true;
}


void D3D_AutomapReset (void)
{
}


void RotatePointAroundVector (vec3_t dst, const vec3_t dir, const vec3_t point, float degrees);
void R_AnimateLight (void);
void V_CalcBlend (void);

void D3D_BuildWorld (void);
void D3D_AddWorldSurfacesToRender (void);

void R_SetupParticles (void);
void R_UpdateParticles (void);
void D3D_InitializeTurb (void);
void D3D_DrawAlphaWaterSurfaces (void);
void D3D_DrawOpaqueWaterSurfaces (void);

void R_DrawViewModel (void);
void D3D_DrawAliasModel (entity_t *ent);
void D3D_DrawAliasShadow (entity_t *e);

void R_SetupAliasModels (void);

void D3D_PrepareAliasModel (entity_t *e);

void D3D_BeginUnderwaterWarp (void);
void D3D_EndUnderwaterWarp (void);
void D3D_AutomapReset (void);

void D3D_FinalizeSky (void);

DWORD D3D_OVERBRIGHT_MODULATE = D3DTOP_MODULATE2X;
float d3d_OverbrightModulate = 2.0f;

D3DMATRIX d3d_WorldMatrix;
D3DMATRIX d3d_ProjMatrix;

// render definition for this frame
d3d_renderdef_t d3d_RenderDef;

mplane_t	frustum[4];

// view origin
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

// screen size info
refdef_t	r_refdef;

texture_t	*r_notexture_mip;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value


void R_MarkLeaves (void);

cvar_t	r_norefresh ("r_norefresh","0");
cvar_t	r_drawentities ("r_drawentities","1");
cvar_t	r_drawviewmodel ("r_drawviewmodel","1");
cvar_t	r_speeds ("r_speeds","0");
cvar_t	r_fullbright ("r_fullbright","0");
cvar_t	r_lightmap ("r_lightmap","0");
cvar_t	r_shadows ("r_shadows", "0", CVAR_ARCHIVE);
cvar_t	r_wateralpha ("r_wateralpha", 1.0f, CVAR_ARCHIVE);
cvar_t	r_dynamic ("r_dynamic","1");
cvar_t	r_novis ("r_novis","0");

cvar_t	gl_clear ("gl_clear","1", CVAR_ARCHIVE);
cvar_t	gl_cull ("gl_cull","1");
cvar_t	gl_smoothmodels ("gl_smoothmodels","1");
cvar_t	gl_affinemodels ("gl_affinemodels","0");
cvar_t	gl_polyblend ("gl_polyblend","1");
cvar_t	gl_nocolors ("gl_nocolors","0");
cvar_t	gl_doubleeyes ("gl_doubleeys", "1");

// fog
cvar_t gl_fogenable ("gl_fogenable", 0.0f, CVAR_ARCHIVE);
cvar_t gl_fogsky ("gl_fogsky", 1.0f, CVAR_ARCHIVE);
cvar_t gl_fogred ("gl_fogred", 0.85f, CVAR_ARCHIVE);
cvar_t gl_foggreen ("gl_foggreen", 0.55f, CVAR_ARCHIVE);
cvar_t gl_fogblue ("gl_fogblue", 0.3f, CVAR_ARCHIVE);
cvar_t gl_fogstart ("gl_fogstart", 10.0f, CVAR_ARCHIVE);
cvar_t gl_fogend ("gl_fogend", 2048.0f, CVAR_ARCHIVE);
cvar_t gl_fogdensity ("gl_fogdensity", 0.001f, CVAR_ARCHIVE);

// hmmm- this does nothing yet...
cvar_t gl_underwaterfog ("gl_underwaterfog", 1, CVAR_ARCHIVE);

cvar_t r_lockfrustum ("r_lockfrustum", 0.0f);
cvar_t r_wireframe ("r_wireframe", 0.0f);

// dynamic far clipping plane depending on map size
float r_clipdist;

extern float r_automap_x;
extern float r_automap_y;
extern float r_automap_z;
extern float r_automap_scale;
int automap_culls = 0;
cvar_t r_automap_nearclip ("r_automap_nearclip", "48", CVAR_ARCHIVE);

bool R_AutomapCull (vec3_t emins, vec3_t emaxs)
{
	// assume it's going to be culled
	automap_culls++;

	// too near
	if (emins[2] > (r_refdef.vieworg[2] + r_automap_nearclip.integer + r_automap_z)) return true;

	// check against screen dimensions which have been scaled and translated to automap space
	if (emaxs[0] < frustum[0].dist) return true;
	if (emins[0] > frustum[1].dist) return true;
	if (emaxs[1] < frustum[2].dist) return true;
	if (emins[1] > frustum[3].dist) return true;

	// not culled
	automap_culls--;
	return false;
}


void D3D_BeginVisedicts (void)
{
	d3d_RenderDef.numvisedicts = 0;
}


void D3D_AddVisEdict (entity_t *ent)
{
	// check for entities with no models
	if (!ent->model) return;

	// only add entities with supported model types
	if (ent->model->type == mod_alias || ent->model->type == mod_brush || ent->model->type == mod_sprite)
	{
		ent->nocullbox = false;

		if (d3d_RenderDef.numvisedicts < MAX_VISEDICTS)
		{
			d3d_RenderDef.visedicts[d3d_RenderDef.numvisedicts] = ent;
			d3d_RenderDef.numvisedicts++;
		}
	}
	else Con_DPrintf ("D3D_AddVisEdict: Unknown entity model type for %s\n", ent->model->name);
}


/*
=================
R_CullBox

Returns true if the box is completely outside the frustum
Only checks the sides; meaning that there is optimization scope (check behind)
=================
*/
bool R_CullBox (vec3_t emins, vec3_t emaxs)
{
	int i;
	mplane_t *p;

	// different culling for the automap
	if (d3d_RenderDef.automap) return R_AutomapCull (emins, emaxs);

	// test order is left/right/bottom/top
	for (i = 0; i < 4; i++)
	{
		p = frustum + i;

		switch (p->signbits)
		{
			// signbits signify which corner of the bbox is nearer the plane
			// note - as only one point/dist test is done per bbox, there is NO advantage to using a bounding sphere
		default:
		case 0:
			if (p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2] < p->dist)
				return true;
			break;

		case 1:
			if (p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2] < p->dist)
				return true;
			break;

		case 2:
			if (p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2] < p->dist)
				return true;
			break;

		case 3:
			if (p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2] < p->dist)
				return true;
			break;

		case 4:
			if (p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2] < p->dist)
				return true;
			break;

		case 5:
			if (p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2] < p->dist)
				return true;
			break;

		case 6:
			if (p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2] < p->dist)
				return true;
			break;

		case 7:
			if (p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2] < p->dist)
				return true;
			break;
		}
	}

	return false;
}


void D3D_RotateForEntity (entity_t *e, D3DMATRIX *m)
{
	D3D_TranslateMatrix (m, e->origin[0], e->origin[1], e->origin[2]);
	D3D_RotateMatrix (m, 0, 0, 1, e->angles[1]);
	D3D_RotateMatrix (m, 0, 1, 0, -e->angles[0]);
	D3D_RotateMatrix (m, 1, 0, 0, e->angles[2]);

	if (e->model->type == mod_alias)
	{
		aliashdr_t *hdr = e->model->aliashdr;

		D3D_TranslateMatrix (m, hdr->scale_origin[0], hdr->scale_origin[1], hdr->scale_origin[2]);
		D3D_ScaleMatrix (m, hdr->scale[0], hdr->scale[1], hdr->scale[2]);
	}
}


void D3D_RotateForEntity (entity_t *e)
{
	D3D_RotateForEntity (e, &e->matrix);
}


//==================================================================================

int SignbitsForPlane (mplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test
	bits = 0;

	for (j = 0; j < 3; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1 << j;
	}

	return bits;
}


/*
===============
TurnVector -- johnfitz

turn forward towards side on the plane defined by forward and side
if angle = 90, the result will be equal to side
assumes side and forward are perpendicular, and normalized
to turn away from side, use a negative angle
===============
*/
#define DEG2RAD(a) ((a) * (M_PI / 180.0f))

void TurnVector (vec3_t out, const vec3_t forward, const vec3_t side, float angle)
{
	float scale_forward, scale_side;

	scale_forward = cos (DEG2RAD (angle));
	scale_side = sin (DEG2RAD (angle));

	out[0] = scale_forward * forward[0] + scale_side * side[0];
	out[1] = scale_forward * forward[1] + scale_side * side[1];
	out[2] = scale_forward * forward[2] + scale_side * side[2];
}


void R_SetFrustum (float fovx, float fovy)
{
	int		i;

	// retain the old frustum unless we're in the first few frames in which case we want one to be done
	// as a baseline
	if (r_lockfrustum.integer && d3d_RenderDef.framecount > 5) return;

	if (d3d_RenderDef.automap)
	{
		// scale and translate to automap space for R_AutomapCull
		frustum[0].dist = r_automap_x - ((vid.width / 2) * r_automap_scale);
		frustum[1].dist = r_automap_x + ((vid.width / 2) * r_automap_scale);
		frustum[2].dist = r_automap_y - ((vid.height / 2) * r_automap_scale);
		frustum[3].dist = r_automap_y + ((vid.height / 2) * r_automap_scale);
		return;
	}

	// frustum cull check order is left/right/bottom/top
	TurnVector (frustum[0].normal, vpn, vright, fovx / 2 - 90);  // left plane
	TurnVector (frustum[1].normal, vpn, vright, 90 - fovx / 2);  // right plane
	TurnVector (frustum[2].normal, vpn, vup, 90 - fovy / 2);  // bottom plane
	TurnVector (frustum[3].normal, vpn, vup, fovy / 2 - 90);  // top plane

	for (i = 0; i < 4; i++)
	{
		// is this necessary???
		// VectorNormalize (frustum[i].normal);

		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}


/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
	// don't allow cheats in multiplayer
	if (cl.maxclients > 1) Cvar_Set ("r_fullbright", "0");

	R_AnimateLight ();

	d3d_RenderDef.framecount++;

	// current viewleaf
	d3d_RenderDef.oldviewleaf = d3d_RenderDef.viewleaf;
	d3d_RenderDef.viewleaf = Mod_PointInLeaf (r_refdef.vieworg, cl.worldmodel);

	d3d_RenderDef.fov_x = r_refdef.fov_x;
	d3d_RenderDef.fov_y = r_refdef.fov_y;

	// this should be moved to d3d_warp
	if (d3d_RenderDef.viewleaf->contents != CONTENTS_EMPTY && d3d_RenderDef.viewleaf->contents != CONTENTS_SOLID)
	{
		extern cvar_t r_waterwarp;
		extern bool UnderwaterValid;
		extern cvar_t r_waterwarptime;

		if (r_waterwarp.integer > 1 || (!UnderwaterValid && r_waterwarp.integer))
		{
			// derive an approx speed
			float warpspeed = cl.time * (r_waterwarptime.value / 2.0f);

			// warp the fov
			d3d_RenderDef.fov_x = atan (tan (DEG2RAD (r_refdef.fov_x) / 2) * (0.97 + sin (warpspeed) * 0.03)) * 2 / (M_PI / 180.0);
			d3d_RenderDef.fov_y = atan (tan (DEG2RAD (r_refdef.fov_y) / 2) * (1.03 - sin (warpspeed) * 0.03)) * 2 / (M_PI / 180.0);
		}
	}

	VectorCopy (r_refdef.vieworg, r_origin);
	AngleVectors (r_refdef.viewangles, vpn, vright, vup);
}


void D3D_RescaleLumas (void);

void D3D_PrepareOverbright (void)
{
	extern cvar_t r_overbright;

	// bound 0 to 2 - (float) 0 is required for overload
	if (r_overbright.integer < 0) Cvar_Set (&r_overbright, (float) 0);
	if (r_overbright.integer > 2) Cvar_Set (&r_overbright, 2);

	if (r_overbright.integer < 1)
	{
		D3D_OVERBRIGHT_MODULATE = D3DTOP_MODULATE;
		d3d_OverbrightModulate = 1.0f;
	}
	else if (r_overbright.integer > 1)
	{
		D3D_OVERBRIGHT_MODULATE = D3DTOP_MODULATE4X;
		d3d_OverbrightModulate = 4.0f;
	}
	else
	{
		D3D_OVERBRIGHT_MODULATE = D3DTOP_MODULATE2X;
		d3d_OverbrightModulate = 2.0f;
	}

	D3D_RescaleLumas ();
}


/*
=============
D3D_PrepareRender
=============
*/
void R_SetupD3D (void)
{
	// r_wireframe 1 is cheating in multiplayer
	if (r_wireframe.integer)
	{
		if (sv.active)
		{
			if (svs.maxclients > 1)
			{
				// force off for the owner of a listen server
				Cvar_Set (&r_wireframe, 0.0f);
			}
		}
		else if (cls.demoplayback)
		{
			// do nothing
		}
		else if (cls.state == ca_connected)
		{
			// force r_wireframe off if not running a local server
			Cvar_Set (&r_wireframe, 0.0f);
		}
	}

	// always clear the zbuffer
	DWORD d3d_ClearFlags = D3DCLEAR_ZBUFFER;
	D3DVIEWPORT9 d3d_3DViewport;

	// if we have a stencil buffer make sure we clear that too otherwise perf will SUFFER
	if (d3d_GlobalCaps.DepthStencilFormat == D3DFMT_D24S8) d3d_ClearFlags |= D3DCLEAR_STENCIL;

	// get dimensions of viewport
	// we do this here so that we can set up the clear rectangle properly
	d3d_3DViewport.X = r_refdef.vrect.x * d3d_CurrentMode.Width / vid.width;
	d3d_3DViewport.Width = r_refdef.vrect.width * d3d_CurrentMode.Width / vid.width;
	d3d_3DViewport.Y = r_refdef.vrect.y * d3d_CurrentMode.Height / vid.height;
	d3d_3DViewport.Height = r_refdef.vrect.height * d3d_CurrentMode.Height / vid.height;

	// adjust for rounding errors by expanding the viewport by 1 in each direction
	// if it's smaller than the screen size
	if (d3d_3DViewport.X > 0) d3d_3DViewport.X--;
	if (d3d_3DViewport.Y > 0) d3d_3DViewport.Y--;
	if (d3d_3DViewport.Width < d3d_CurrentMode.Width) d3d_3DViewport.Width++;
	if (d3d_3DViewport.Height < d3d_CurrentMode.Height) d3d_3DViewport.Height++;

	// set z range
	d3d_3DViewport.MinZ = 0.0f;
	d3d_3DViewport.MaxZ = 1.0f;

	// set the viewport
	d3d_Device->SetViewport (&d3d_3DViewport);

	// keep an identity view matrix at all times; this simplifies the setup and also lets us
	// skip a matrix multiplication per vertex in our vertex shaders. ;)
	D3DXMATRIX vm;
	D3DXMatrixIdentity (&vm);
	d3d_Device->SetTransform (D3DTS_VIEW, &vm);

	D3D_LoadIdentity (&d3d_WorldMatrix);
	D3D_LoadIdentity (&d3d_ProjMatrix);

	DWORD clearcolor = 0xff000000;

	// select correct color clearing mode
	if (d3d_RenderDef.viewleaf->contents == CONTENTS_SOLID)
	{
		// match winquake's grey if we're noclipping
		clearcolor = 0xff1f1f1f;
		d3d_ClearFlags |= D3DCLEAR_TARGET;
	}
	else if (r_wireframe.integer)
	{
		d3d_ClearFlags |= D3DCLEAR_TARGET;
		clearcolor = 0xff1f1f1f;
	}
	else if (d3d_RenderDef.oldviewleaf)
	{
		// explicitly don't clear the color buffer if we have a contents change
		if (d3d_RenderDef.viewleaf->contents == d3d_RenderDef.oldviewleaf->contents && gl_clear.value)
			d3d_ClearFlags |= D3DCLEAR_TARGET;
	}
	else if (gl_clear.value)
		d3d_ClearFlags |= D3DCLEAR_TARGET;

	// projection matrix depends on drawmode
	if (d3d_RenderDef.automap)
	{
		// change clear color to black and force a clear
		clearcolor = 0xff000000;
		d3d_ClearFlags |= D3DCLEAR_TARGET;

		// nothing has been culled yet
		automap_culls = 0;

		float maxdepth = cl.worldmodel->maxs[2];
		if (fabs (cl.worldmodel->mins[2]) > maxdepth) maxdepth = fabs (cl.worldmodel->mins[2]);
		maxdepth += 100;

		// set a near clipping plane based on the refdef vieworg
		// here we retain the same space as the world coords
		D3DXMatrixOrthoOffCenterRH
		(
			D3D_MakeD3DXMatrix (&d3d_ProjMatrix),
			0,
			vid.width * r_automap_scale,
			0,
			vid.height * r_automap_scale,
			-maxdepth,
			maxdepth
		);

		// translate camera by origin
		D3D_TranslateMatrix
		(
			&d3d_WorldMatrix,
			-(r_automap_x - (vid.width / 2) * r_automap_scale),
			-(r_automap_y - (vid.height / 2) * r_automap_scale),
			-r_refdef.vieworg[2]
		);
	}
	else
	{
		// put z going up
		D3D_RotateMatrix (&d3d_WorldMatrix, 1, 0, 0, -90);
		D3D_RotateMatrix (&d3d_WorldMatrix, 0, 0, 1, 90);

		// rotate camera by angles
		D3D_RotateMatrix (&d3d_WorldMatrix, 1, 0, 0, -r_refdef.viewangles[2]);
		D3D_RotateMatrix (&d3d_WorldMatrix, 0, 1, 0, -r_refdef.viewangles[0]);
		D3D_RotateMatrix (&d3d_WorldMatrix, 0, 0, 1, -r_refdef.viewangles[1]);

		// translate camera by origin
		D3D_TranslateMatrix (&d3d_WorldMatrix, -r_refdef.vieworg[0], -r_refdef.vieworg[1], -r_refdef.vieworg[2]);

		// set projection frustum (infinite projection; the value of r_clipdist is now only used for positioning the sky)
		float xmax = 4.0f * tan ((d3d_RenderDef.fov_x * M_PI) / 360.0);
		float ymax = 4.0f * tan ((d3d_RenderDef.fov_y * M_PI) / 360.0);

		D3DXMatrixPerspectiveOffCenterRH
		(
			D3D_MakeD3DXMatrix (&d3d_ProjMatrix),
			-xmax, xmax, -ymax, ymax, 4.0f, 4096.0f
		);

		// hacky infinite projection
		d3d_ProjMatrix._33 = -1;
		d3d_ProjMatrix._43 = -1;
	}

	// set basic transforms (these must always be run)
	d3d_Device->SetTransform (D3DTS_WORLD, &d3d_WorldMatrix);
	d3d_Device->SetTransform (D3DTS_PROJECTION, &d3d_ProjMatrix);

	// we only need to clear if we're rendering 3D
	d3d_Device->Clear (0, NULL, d3d_ClearFlags, clearcolor, 1.0f, 1);

	// depth testing and writing
	D3D_SetRenderState (D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	D3D_SetRenderState (D3DRS_ZENABLE, D3DZB_TRUE);

	// turn off smooth shading
	D3D_SetRenderState (D3DRS_SHADEMODE, D3DSHADE_FLAT);

	// backface culling
	D3D_BackfaceCull (D3DCULL_CCW);

	// disable all alpha ops
	D3D_SetRenderState (D3DRS_ALPHATESTENABLE, FALSE);
	D3D_DisableAlphaBlend ();
	D3D_SetRenderState (D3DRS_ZWRITEENABLE, TRUE);

	// optionally enable wireframe mode
	if (r_wireframe.integer) D3D_SetRenderState (D3DRS_FILLMODE, D3DFILL_WIREFRAME);
}


void D3D_PrepareFog (void)
{
	// no fog
	if (gl_fogenable.integer <= 0) return;
	if (d3d_RenderDef.automap) return;

	// sanity check cvars
	if (gl_fogstart.value < 10) Cvar_Set (&gl_fogstart, 10.0f);
	if (gl_fogend.value < 10) Cvar_Set (&gl_fogend, 10.0f);

	if (gl_fogdensity.value < 0) Cvar_Set (&gl_fogdensity, 0.0f);
	if (gl_fogdensity.value > 0.01f) Cvar_Set (&gl_fogdensity, 0.01f);

	if (gl_fogend.value < gl_fogstart.value)
	{
		Cvar_Set (&gl_fogstart, 10.0f);
		Cvar_Set (&gl_fogend, 2048.0f);
	}

	D3D_SetRenderState (D3DRS_FOGENABLE, TRUE);

	DWORD fogcolor = D3DCOLOR_XRGB
	(
		BYTE_CLAMP (gl_fogred.value * 255),
		BYTE_CLAMP (gl_foggreen.value * 255),
		BYTE_CLAMP (gl_fogblue.value * 255)
	);

	D3D_SetRenderState (D3DRS_FOGCOLOR, fogcolor);

	// default fog is linear per-vertex
	DWORD fvm = D3DFOG_LINEAR;
	DWORD fpm = D3DFOG_NONE;

	// if we're using pixel shaders we need to switch to per-pixel fog as we don't provide fog
	// as an input to the vertex shader (for simplicity)
	if (d3d_GlobalCaps.usingPixelShaders)
	{
		fpm = D3DFOG_LINEAR;
		fvm = D3DFOG_NONE;
	}

	// now get the real mode - once again, if using pixel shaders we switch all per-vertex modes to per-pixel
	switch (gl_fogenable.integer)
	{
	case 2:
		if (d3d_GlobalCaps.usingPixelShaders)
		{
			fpm = D3DFOG_EXP;
			fvm = D3DFOG_NONE;
		}
		else
		{
			fvm = D3DFOG_EXP;
			fpm = D3DFOG_NONE;
		}
		break;

	case 3:
		if (d3d_GlobalCaps.usingPixelShaders)
		{
			fpm = D3DFOG_EXP2;
			fvm = D3DFOG_NONE;
		}
		else
		{
			fvm = D3DFOG_EXP2;
			fpm = D3DFOG_NONE;
		}
		break;

	case 4:
		fvm = D3DFOG_NONE;
		fpm = D3DFOG_LINEAR;
		break;

	case 5:
		fvm = D3DFOG_NONE;
		fpm = D3DFOG_EXP;
		break;

	case 6:
		fvm = D3DFOG_NONE;
		fpm = D3DFOG_EXP2;
		break;

	default:
		break;
	}

	D3D_SetRenderState (D3DRS_FOGVERTEXMODE, fvm);
	D3D_SetRenderState (D3DRS_FOGTABLEMODE, fpm);
	D3D_SetRenderStatef (D3DRS_FOGDENSITY, gl_fogdensity.value);
	D3D_SetRenderStatef (D3DRS_FOGSTART, gl_fogstart.value);
	D3D_SetRenderStatef (D3DRS_FOGEND, gl_fogend.value);
}


void D3D_UpdateContentsColor (void)
{
	if (d3d_RenderDef.viewleaf->contents == CONTENTS_EMPTY)
	{
		// brought to you today by "disgusting hacks 'r' us"...
		// stuffcmd must be the most EVIL thing ever invented, and as soon as QC 1337 h4x0rz start
		// doing things RIGHT a lot of good will come into the world.
		extern cshift_t cshift_empty;

		cl.cshifts[CSHIFT_CONTENTS].destcolor[0] = cshift_empty.destcolor[0];
		cl.cshifts[CSHIFT_CONTENTS].destcolor[1] = cshift_empty.destcolor[1];
		cl.cshifts[CSHIFT_CONTENTS].destcolor[2] = cshift_empty.destcolor[2];
		cl.cshifts[CSHIFT_CONTENTS].percent = cshift_empty.percent;
	}
	else if (d3d_RenderDef.viewleaf->contentscolor)
	{
		// if the viewleaf has a contents colour set we just override with it
		cl.cshifts[CSHIFT_CONTENTS].destcolor[0] = d3d_RenderDef.viewleaf->contentscolor[0];
		cl.cshifts[CSHIFT_CONTENTS].destcolor[1] = d3d_RenderDef.viewleaf->contentscolor[1];
		cl.cshifts[CSHIFT_CONTENTS].destcolor[2] = d3d_RenderDef.viewleaf->contentscolor[2];

		// these now have more colour so reduce the percent to compensate
		if (d3d_RenderDef.viewleaf->contents == CONTENTS_WATER)
			cl.cshifts[CSHIFT_CONTENTS].percent = 48;
		else if (d3d_RenderDef.viewleaf->contents == CONTENTS_LAVA)
			cl.cshifts[CSHIFT_CONTENTS].percent = 112;
		else if (d3d_RenderDef.viewleaf->contents == CONTENTS_SLIME)
			cl.cshifts[CSHIFT_CONTENTS].percent = 80;
		else cl.cshifts[CSHIFT_CONTENTS].percent = 0;
	}
	else
	{
		// empty or undefined
		V_SetContentsColor (d3d_RenderDef.viewleaf->contents);
	}

	// now calc the blend
	V_CalcBlend ();
}


/*
================
R_SetupBrushModels

all brushmodels (including the world) need an initial setup pass to determine
visibility, update lighting, build texture chains, and so on
================
*/
void R_SetupBrushModels (void)
{
	// mark visible leafs
	R_MarkLeaves ();

	// add fog to the render
	D3D_PrepareFog ();

	// add visible surfaces to the render
	D3D_BuildWorld ();

	// upload any lightmaps that were modified
	d3d_Lightmaps->UploadModified ();

	// some sky modes require extra stuff to be added after the surfs so handle those now
	D3D_FinalizeSky ();

	// finish solid surfaces by adding any such to the solid buffer
	D3D_AddWorldSurfacesToRender ();
}


void R_SetupSpriteModels (void)
{
	if (!r_drawentities.integer) return;

	for (int i = 0; i < d3d_RenderDef.numvisedicts; i++)
	{
		entity_t *ent = d3d_RenderDef.visedicts[i];

		if (!ent->model) continue;
		if (ent->model->type != mod_sprite) continue;

		// sprites always have alpha
		D3D_AddToAlphaList (ent);
	}
}


/*
================
R_ModParanoia

mods sometimes send skin, frame and other numbers that are not valid for the model
so here we need to fix them up...
================
*/
void R_ModParanoia (entity_t *ent)
{
	model_t *mod = ent->model;

	switch (mod->type)
	{
	case mod_alias:
		// fix up skins
		if (ent->skinnum >= mod->aliashdr->numskins) ent->skinnum = 0;
		if (ent->skinnum < 0) ent->skinnum = 0;

		// fix up frame
		if (ent->frame >= mod->aliashdr->numframes) ent->frame = 0;
		if (ent->frame < 0) ent->frame = 0;
		break;

	case mod_brush:
		// only 2 frames in brushmodels for regular and alternate anims
		if (ent->frame) ent->frame = 1;
		break;

	case mod_sprite:
		// fix up frame
		if (ent->frame >= mod->spritehdr->numframes) ent->frame = 0;
		if (ent->frame < 0) ent->frame = 0;
		break;

	default:
		// do nothing
		break;
	}
}


void R_SetupEntitiesOnList (void)
{
	// brush models include the world
	R_SetupBrushModels ();
	R_SetupSpriteModels ();

	// deferred until after the world so that we get statics on the list too
	for (int i = 0; i < d3d_RenderDef.numvisedicts; i++)
	{
		entity_t *ent = d3d_RenderDef.visedicts[i];

		if (!ent->model) continue;

		R_ModParanoia (ent);
	}
}


int r_speedstime = -1;


void D3D_AddExtraInlineModelsToListsForAutomap (void)
{
	if (!d3d_RenderDef.automap) return;

	for (int i = 1; i < MAX_MODELS; i++)
	{
		model_t *m = cl.model_precache[i];

		// end of the list
		if (!m) break;

		// no entity cached
		if (!m->cacheent) continue;

		// model was never seen by the player
		if (!m->wasseen) continue;

		// model was already added to the list
		if (m->cacheent->visframe == d3d_RenderDef.framecount) continue;

		// add it to the list and mark as visible
		m->cacheent->visframe = d3d_RenderDef.framecount;
		D3D_AddVisEdict (m->cacheent);
	}
}


/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView (void)
{
	int time1, time2;

	if (r_norefresh.value) return;
	if (!d3d_RenderDef.worldentity.model || !cl.worldmodel || !cl.worldmodel->brushhdr) return;

	static float old_frametime = 0;

	// get frametime - this was cl.oldtime, creating a framerate dependency
	d3d_RenderDef.frametime = cl.time - old_frametime;
	old_frametime = cl.time;

	if (r_speeds.value) time1 = Sys_DWORDTime ();

	// initialize r_speeds and flags
	d3d_RenderDef.brush_polys = 0;
	d3d_RenderDef.last_alias_polys = d3d_RenderDef.alias_polys;
	d3d_RenderDef.alias_polys = 0;
	d3d_RenderDef.numsss = 0;

	// begin the underwater warp
	D3D_BeginUnderwaterWarp ();

	// setup to render
	R_SetupFrame ();
	R_SetFrustum (d3d_RenderDef.fov_x, d3d_RenderDef.fov_y);
	R_SetupD3D ();
	D3D_UpdateContentsColor ();
	D3D_PrepareOverbright ();
	D3D_AddExtraInlineModelsToListsForAutomap ();

	// setup everything we're going to draw
	R_SetupEntitiesOnList ();
	R_SetupParticles ();

	// upload all lightmaps that were modified (the alpha list might include modified lightmaps too)
	//d3d_Lightmaps->UploadModified ();

	R_SetupAliasModels ();

	// draw all items on the alpha list
	D3D_SortAlphaList ();

	// the viewmodel comes last
	R_DrawViewModel ();

	// ensure
	D3D_SetRenderState (D3DRS_FILLMODE, D3DFILL_SOLID);

	// finish up the underwater warp
	D3D_EndUnderwaterWarp ();

	// update particles
	R_UpdateParticles ();

	// disable fog (always)
	D3D_SetRenderState (D3DRS_FOGENABLE, FALSE);

	if (r_speeds.value)
	{
		time2 = Sys_DWORDTime ();
		r_speedstime = time2 - time1;
	}
	else r_speedstime = -1;
}

