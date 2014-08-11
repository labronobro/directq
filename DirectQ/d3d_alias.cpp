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
// gl_mesh.c: triangle model functions

#include "quakedef.h"
#include "d3d_model.h"
#include "d3d_quake.h"
#include "resource.h"

void R_LightPoint (entity_t *e, float *c);

// up to 16 color translated skins
image_t d3d_PlayerSkins[256];

void D3D_RotateForEntity (entity_t *e);
void D3D_RotateForEntity (entity_t *e, D3DMATRIX *m);


/*
=================================================================

ALIAS MODEL DISPLAY LIST GENERATION

=================================================================
*/
DWORD *d3d_RemapTable = NULL;
unsigned short *d3d_CopyIndexes = NULL;

void D3D_FinalizeAliasMeshPart (aliashdr_t *hdr, aliaspart_t *part)
{
	// optimize indexes for vertex cache hits.
	// a full optimization (including D3DXOptimizeVertices) actually comes out as slower
	// with *most* Q1 mdls (all that I've tested anyway - ID1 and Quoth) so we don't bother
	// doing it.  This code would actually work with OpenGL as the D3DX functions used
	// don't require a d3d device; they're purely algorithmic.
	QD3DXOptimizeFaces (part->indexes, part->numindexes / 3, part->nummesh, FALSE, d3d_RemapTable);

	Q_MemCpy (d3d_CopyIndexes, part->indexes, part->numindexes * sizeof (unsigned short));

	// reverse the order because the remap table is reversed
	for (int i = 0, j = (part->numindexes / 3) - 1; i < part->numindexes; i += 3, j--)
	{
		part->indexes[i + 0] = d3d_CopyIndexes[d3d_RemapTable[j] * 3 + 0];
		part->indexes[i + 1] = d3d_CopyIndexes[d3d_RemapTable[j] * 3 + 1];
		part->indexes[i + 2] = d3d_CopyIndexes[d3d_RemapTable[j] * 3 + 2];
	}

	// copy out the verts
	part->meshverts = (aliasmesh_t *) MainCache->Alloc (part->meshverts, part->nummesh * sizeof (aliasmesh_t));
}


/*
================
GL_MakeAliasModelDisplayLists

Generate a list of trifans or strips
for the model, which holds for all frames
================
*/
void GL_MakeAliasModelDisplayLists (aliashdr_t *hdr, stvert_t *stverts, dtriangle_t *triangles)
{
	// nothing to begin with
	hdr->parts = NULL;
	aliaspart_t *part = NULL;

	// also reserve space for the index remap tables
	int max_mesh = (SCRATCHBUF_SIZE / (sizeof (aliasmesh_t) + sizeof (DWORD) + sizeof (unsigned short) * 3));

	// set up holding areas for the remapping
	d3d_RemapTable = (DWORD *) (scratchbuf + sizeof (aliasmesh_t) * max_mesh);
	d3d_CopyIndexes = (unsigned short *) (d3d_RemapTable + max_mesh);

	// create a pool of indexes for use by the model
	unsigned short *indexes = (unsigned short *) MainCache->Alloc (3 * sizeof (unsigned short) * hdr->numtris);

	for (int i = 0, v = 0; i < hdr->numtris; i++)
	{
		if (!hdr->parts || (part && part->nummesh >= max_mesh))
		{
			// copy to the cache
			if (part) D3D_FinalizeAliasMeshPart (hdr, part);

			// go to a new part
			part = (aliaspart_t *) MainCache->Alloc (sizeof (aliaspart_t));

			// link it in
			part->next = hdr->parts;
			hdr->parts = part;

			// take a chunk of indexes for it
			part->indexes = indexes;

			// take a chunk of the scratch buffer for it
			part->meshverts = (aliasmesh_t *) scratchbuf;
			part->nummesh = 0;
			part->numindexes = 0;
		}

		for (int j = 0; j < 3; j++, indexes++, part->numindexes++)
		{
			// this is nothing to do with an index buffer, it's an index into hdr->vertexes
			int vertindex = triangles[i].vertindex[j];

			// basic s/t coords
			int s = stverts[vertindex].s;
			int t = stverts[vertindex].t;

			// check for back side and adjust texcoord s
			if (!triangles[i].facesfront && stverts[vertindex].onseam) s += hdr->skinwidth / 2;

			// see does this vert already exist
			for (v = 0; v < part->nummesh; v++)
			{
				// it could use the same xyz but have different s and t
				if (part->meshverts[v].vertindex == vertindex && (int) part->meshverts[v].s == s && (int) part->meshverts[v].t == t)
				{
					// exists; emit an index for it
					part->indexes[part->numindexes] = v;
					break;
				}
			}

			if (v == part->nummesh)
			{
				// doesn't exist; emit a new vert and index
				part->indexes[part->numindexes] = part->nummesh;

				part->meshverts[part->nummesh].vertindex = vertindex;
				part->meshverts[part->nummesh].s = s;
				part->meshverts[part->nummesh++].t = t;
			}
		}
	}

	// finish the last one
	if (part) D3D_FinalizeAliasMeshPart (hdr, part);

	// dropped alias skin padding because there are too many special cases
	for (part = hdr->parts; part; part = part->next)
	{
		for (int i = 0; i < part->nummesh; i++)
		{
			part->meshverts[i].s = part->meshverts[i].s / (float) hdr->skinwidth;
			part->meshverts[i].t = part->meshverts[i].t / (float) hdr->skinheight;
		}
	}

	// not delerped yet (view models only)
	hdr->mfdelerp = false;
}


/*
=============================================================

  ALIAS MODELS

=============================================================
*/


// precalculated dot products for quantized angles
// instead of doing interpolation at run time we precalc lerped dots
// they're still somewhat quantized but less so (to less than a degree)
#define SHADEDOT_QUANT 16
#define SHADEDOT_QUANT_LERP 512
float **r_avertexnormal_dots_lerp = NULL;


float Mesh_ScaleVert (aliashdr_t *hdr, drawvertx_t *invert, int index)
{
	float outvert = invert->v[index];

	outvert *= hdr->scale[index];
	outvert += hdr->scale_origin[index];

	return outvert;
}


/*
===================
DelerpMuzzleFlashes

Done at runtime (once only per model) because there is no guarantee that a viewmodel
will follow the naming convention used by ID.  As a result, the only way we have to
be certain that a model is a viewmodel is when we come to draw the viewmodel.
===================
*/
void DelerpMuzzleFlashes (aliashdr_t *hdr)
{
	// already delerped
	if (hdr->mfdelerp) return;

	// done now
	hdr->mfdelerp = true;

	// get pointers to the verts
	drawvertx_t *vertsf0 = hdr->vertexes[0];
	drawvertx_t *vertsf1 = hdr->vertexes[1];
	drawvertx_t *vertsfi;

	// now go through them and compare.  we expect that (a) the animation is sensible and there's no major
	// difference between the 2 frames to be expected, and (b) any verts that do exhibit a major difference
	// can be assumed to belong to the muzzleflash
	for (int j = 0; j < hdr->vertsperframe; j++, vertsf0++, vertsf1++)
	{
		// get difference in front to back movement
		float vdiff = Mesh_ScaleVert (hdr, vertsf1, 0) - Mesh_ScaleVert (hdr, vertsf0, 0);

		// if it's above a certain treshold, assume a muzzleflash and mark for no lerp
		// 10 is the approx lowest range of visible front to back in a view model, so that seems reasonable to work with
		if (vdiff > 10)
			vertsf0->lerpvert = false;
		else vertsf0->lerpvert = true;
	}

	// now we mark every other vert in the model as per the instructions in the first frame
	for (int i = 1; i < hdr->numframes; i++)
	{
		// get pointers to the verts
		vertsf0 = hdr->vertexes[0];
		vertsfi = hdr->vertexes[i];

		for (int j = 0; j < hdr->vertsperframe; j++, vertsf0++, vertsfi++)
		{
			// just copy it across
			vertsfi->lerpvert = vertsf0->lerpvert;
		}
	}
}



extern vec3_t lightspot;
extern mplane_t *lightplane;

d3d_shader_t d3d_ShadowShader =
{
	{
		// number of stages and type
		3,
		VBO_SHADER_TYPE_FIXED
	},
	{
		// color
		{D3DTOP_DISABLE},
		{D3DTOP_DISABLE},
		{D3DTOP_DISABLE}
	},
	{
		// alpha
		{D3DTOP_MODULATE, D3DTA_CURRENT, D3DTA_DIFFUSE},
		{D3DTOP_DISABLE},
		{D3DTOP_DISABLE}
	}
};

void D3D_DrawAliasShadows (entity_t **ents, int numents)
{
	// this is a hack to get around a non-zero r_shadows being triggered by a combination of r_shadows 0 and
	// low precision FP rounding errors, thereby causing unnecesary slowdowns.
	byte shadealpha = BYTE_CLAMP (r_shadows.value * 255.0f);

	if (shadealpha < 1) return;

	bool stateset = false;
	DWORD shadecolor = D3DCOLOR_ARGB (shadealpha, 0, 0, 0);
	D3D_VBOBegin (D3DPT_TRIANGLELIST, sizeof (aliaspolyvert_t));

	for (int i = 0; i < numents; i++)
	{
		entity_t *ent = ents[i];

		if (ent->alphaval < 255) continue;
		if (ent->visframe != d3d_RenderDef.framecount) continue;

		// easier access
		aliasstate_t *aliasstate = &ent->aliasstate;
		aliashdr_t *hdr = ent->model->aliashdr;

		// don't crash
		if (!aliasstate->lightplane) continue;

		if (!stateset)
		{
			// state for shadows
			D3D_SetRenderState (D3DRS_ZWRITEENABLE, FALSE);
			D3D_SetRenderState (D3DRS_ALPHABLENDENABLE, TRUE);
			D3D_SetRenderState (D3DRS_BLENDOP, D3DBLENDOP_ADD);
			D3D_SetRenderState (D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			D3D_SetRenderState (D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

			D3D_SetVertexDeclaration (d3d_VDXyzDiffuse);

			if (d3d_GlobalCaps.DepthStencilFormat == D3DFMT_D24S8)
			{
				// of course, we all know that Direct3D lacks Stencil Buffer and Polygon Offset support,
				// so what you're looking at here doesn't really exist.  Those of you who froth at the mouth
				// and like to think it's still the 1990s had probably better look away now.
				D3D_SetRenderState (D3DRS_STENCILENABLE, TRUE);
				D3D_SetRenderState (D3DRS_STENCILFUNC, D3DCMP_EQUAL);
				D3D_SetRenderState (D3DRS_STENCILREF, 0x00000001);
				D3D_SetRenderState (D3DRS_STENCILMASK, 0x00000002);
				D3D_SetRenderState (D3DRS_STENCILWRITEMASK, 0xFFFFFFFF);
				D3D_SetRenderState (D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
				D3D_SetRenderState (D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
				D3D_SetRenderState (D3DRS_STENCILPASS, D3DSTENCILOP_INCRSAT);
			}

			stateset = true;
		}

		// build the transformation for this entity.
		// we need to run this in software as we are now potentially submitting multiple alias models in a single batch
		D3D_LoadIdentity (&ent->matrix);
		D3D_TranslateMatrix (&ent->matrix, ent->origin[0], ent->origin[1], ent->origin[2]);
		D3D_RotateMatrix (&ent->matrix, 0, 0, 1, ent->angles[1]);

		// the scaling needs to be included at this time
		D3D_TranslateMatrix (&ent->matrix, hdr->scale_origin[0], hdr->scale_origin[1], hdr->scale_origin[2]);
		D3D_ScaleMatrix (&ent->matrix, hdr->scale[0], hdr->scale[1], hdr->scale[2]);

		for (aliaspart_t *part = hdr->parts; part; part = part->next)
		{
			// check for overflow
			D3D_VBOCheckOverflow (part->nummesh, part->numindexes);

			// note the hack here - just sending NULL into the shader won't add it!!! (really should fix this)
			D3D_VBOAddShader (&d3d_ShadowShader, NULL, d3d_PlayerSkins[0].d3d_Texture);

			// accumulate vert counts
			D3D_VBOAddShadowVerts (ent, hdr, part, aliasstate, shadecolor);
			d3d_RenderDef.alias_polys += hdr->numtris;
		}
	}

	// required even if there is nothing so that the buffers will unlock
	D3D_VBORender ();

	if (stateset)
	{
		// back to normal
		if (d3d_GlobalCaps.DepthStencilFormat == D3DFMT_D24S8)
			D3D_SetRenderState (D3DRS_STENCILENABLE, FALSE);

		D3D_SetRenderState (D3DRS_ALPHABLENDENABLE, FALSE);
		D3D_SetRenderState (D3DRS_ZWRITEENABLE, TRUE);
		D3D_SetTextureState (0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		D3D_SetTextureState (0, D3DTSS_COLOROP, D3DTOP_DISABLE);
	}
}


/*
=================
D3D_SetupAliasFrame

=================
*/
extern cvar_t r_lerpframe;

float D3D_SetupAliasFrame (entity_t *ent, aliashdr_t *hdr)
{
	int pose, numposes;
	float interval;
	bool lerpmdl = true;
	float blend;
	float frame_interval;

	// silently revert to frame 0
	if ((ent->frame >= hdr->numframes) || (ent->frame < 0)) ent->frame = 0;

	pose = hdr->frames[ent->frame].firstpose;
	numposes = hdr->frames[ent->frame].numposes;

	if (numposes > 1)
	{
		// client-side animations
		interval = hdr->frames[ent->frame].interval;

		// software quake adds syncbase here so that animations using the same model aren't in lockstep
		// trouble is that syncbase is always 0 for them so we provide new fields for it instead...
		pose += (int) ((cl.time + ent->posebase) / interval) % numposes;

		// not really needed for non-interpolated mdls, but does no harm
		frame_interval = interval;
	}
	else frame_interval = 0.1;

	// conditions for turning lerping off (the SNG bug is no longer an issue)
	if (hdr->nummeshframes == 1) lerpmdl = false;			// only one pose
	if (ent->lastpose == ent->currpose) lerpmdl = false;		// both frames are identical
	if (!r_lerpframe.value) lerpmdl = false;

	// interpolation
	if (ent->currpose == -1 || ent->lastpose == -1)
	{
		// new entity so reset to no interpolation initially
		ent->frame_start_time = cl.time;
		ent->lastpose = ent->currpose = pose;
		blend = 0;
	}
	else if (ent->lastpose == ent->currpose && ent->currpose == 0 && ent != &cl.viewent)
	{
		// "dying throes" interpolation bug - begin a new sequence with both poses the same
		// this happens when an entity is spawned client-side
		ent->frame_start_time = cl.time;
		ent->lastpose = ent->currpose = pose;
		blend = 0;
	}
	else if (pose == 0 && ent == &cl.viewent)
	{
		// don't interpolate from previous pose on frame 0 of the viewent
		ent->frame_start_time = cl.time;
		ent->lastpose = ent->currpose = pose;
		blend = 0;
	}
	else if (ent->currpose != pose || !lerpmdl)
	{
		// begin a new interpolation sequence
		ent->frame_start_time = cl.time;
		ent->lastpose = ent->currpose;
		ent->currpose = pose;
		blend = 0;
	}
	else blend = (cl.time - ent->frame_start_time) / frame_interval;

	// if a viewmodel is switched and the previous had a current frame number higher than the number of frames
	// in the new one, DirectQ will crash so we need to fix that.  this is also a general case sanity check.
	if (ent->lastpose >= hdr->nummeshframes) ent->lastpose = ent->currpose = 0; else if (ent->lastpose < 0) ent->lastpose = ent->currpose = hdr->nummeshframes - 1;
	if (ent->currpose >= hdr->nummeshframes) ent->lastpose = ent->currpose = 0; else if (ent->currpose < 0) ent->lastpose = ent->currpose = hdr->nummeshframes - 1;

	// don't let blend pass 1
	if (cl.paused || blend > 1.0) blend = 1.0;

	return blend;
}


void D3D_DrawAliasModel (entity_t *ent);

// to do -
// a more robust general method is needed.  use the middle line of the colormap, always store out texels
// detect if the colormap changes and work on the actual proper texture instead of a copy in the playerskins array
// no, because we're not creating a colormap for the entity any more.  in fact storing the colormap in the ent
// and the translation in cl.scores is now largely redundant...
// also no because working on the skin directly will break with instanced models
bool D3D_TranslateAliasSkin (entity_t *ent)
{
	// no translation
	if (ent->playerskin < 0) return false;
	if (!ent->model) return false;
	if (ent->model->type != mod_alias) return false;
	if (!(ent->model->flags & EF_PLAYER)) return false;

	// sanity
	ent->playerskin &= 255;

	// already built a skin for this colour
	if (d3d_PlayerSkins[ent->playerskin].d3d_Texture) return true;

	byte	translate[256];
	byte	*original;
	static int skinsize = -1;
	static byte *translated = NULL;

	aliashdr_t *paliashdr = ent->model->aliashdr;
	int s = paliashdr->skinwidth * paliashdr->skinheight;

	if (ent->skinnum < 0 || ent->skinnum >= paliashdr->numskins)
	{
		Con_Printf ("(%d): Invalid player skin #%d\n", ent->playerskin, ent->skinnum);
		original = paliashdr->skins[0].texels;
	}
	else original = paliashdr->skins[ent->skinnum].texels;

	// no texels were saved
	if (!original) return false;

	if (s & 3)
	{
		Con_Printf ("D3D_TranslateAliasSkin: s & 3\n");
		return false;
	}

	int top = ent->playerskin & 0xf0;
	int bottom = (ent->playerskin & 15) << 4;

	// baseline has no palette translation
	for (int i = 0; i < 256; i++) translate[i] = i;

	for (int i = 0; i < 16; i++)
	{
		if (top < 128)	// the artists made some backwards ranges.  sigh.
			translate[TOP_RANGE + i] = top + i;
		else
			translate[TOP_RANGE + i] = top + 15 - i;
				
		if (bottom < 128)
			translate[BOTTOM_RANGE + i] = bottom + i;
		else translate[BOTTOM_RANGE + i] = bottom + 15 - i;
	}

	// recreate the texture
	SAFE_RELEASE (d3d_PlayerSkins[ent->playerskin].d3d_Texture);

	// check for size change
	if (skinsize != s)
	{
		// cache the size
		skinsize = s;

		// free the current buffer
		if (translated) Zone_Free (translated);
		translated = NULL;
	}

	// create a new buffer only if required (more optimization)
	if (!translated) translated = (byte *) Zone_Alloc (s);

	for (int i = 0; i < s; i += 4)
	{
		translated[i] = translate[original[i]];
		translated[i + 1] = translate[original[i + 1]];
		translated[i + 2] = translate[original[i + 2]];
		translated[i + 3] = translate[original[i + 3]];
	}

	// don't compress these because it takes too long
	D3D_UploadTexture
	(
		&d3d_PlayerSkins[ent->playerskin].d3d_Texture,
		translated,
		paliashdr->skinwidth,
		paliashdr->skinheight,
		IMAGE_MIPMAP | IMAGE_NOCOMPRESS | IMAGE_NOEXTERN | IMAGE_PADDABLE
	);

	// Con_Printf ("Translated skin to %i\n", ent->playerskin);
	// success
	return true;
}


void D3D_SetupAliasModel (entity_t *ent)
{
	vec3_t mins, maxs;

	// take pointers for easier access
	aliashdr_t *hdr = ent->model->aliashdr;
	aliasstate_t *aliasstate = &ent->aliasstate;

	// assume that the model has been culled
	ent->visframe = -1;

	// revert to full model culling here
	VectorAdd (ent->origin, ent->model->mins, mins);
	VectorAdd (ent->origin, ent->model->maxs, maxs);

	// the gun or the chase model are never culled away
	if (ent == cl_entities[cl.viewentity] && chase_active.value)
		;	// no bbox culling on certain entities
	else if (ent->nocullbox)
		;	// no bbox culling on certain entities
	else if (R_CullBox (mins, maxs))
	{
		// no further setup needed
		return;
	}

	// the model has not been culled now
	ent->visframe = d3d_RenderDef.framecount;
	aliasstate->lightplane = NULL;

	if (ent == cl_entities[cl.viewentity] && chase_active.value)
	{
		// adjust angles (done here instead of chase_update as we only want the angle for rendering to be adjusted)
		ent->angles[0] *= 0.3;
	}

	// setup the frame for drawing and store the interpolation blend
	// this can't be moved back to the client because static entities and the viewmodel need to go through it
	float blend = D3D_SetupAliasFrame (ent, hdr);

	// use cubic interpolation
	aliasstate->lastlerp = 2 * (blend * blend * blend) - 3 * (blend * blend) + 1;
	aliasstate->currlerp = 3 * (blend * blend) - 2 * (blend * blend * blend);

	// get lighting information
	vec3_t shadelight;
	R_LightPoint (ent, shadelight);

	// average light between frames
	for (int i = 0; i < 3; i++)
	{
		shadelight[i] = (shadelight[i] + ent->shadelight[i]) / 2;
		ent->shadelight[i] = shadelight[i];
	}

	// precomputed and pre-interpolated shading dot products
	aliasstate->shadedots = r_avertexnormal_dots_lerp[((int) (ent->angles[1] * (SHADEDOT_QUANT_LERP / 360.0))) & (SHADEDOT_QUANT_LERP - 1)];

	// store out for shadows
	VectorCopy (lightspot, aliasstate->lightspot);
	aliasstate->lightplane = lightplane;

	// get texturing info
	// software quake randomises the base animation and so should we
	int anim = (int) ((cl.time + ent->skinbase) * 10) & 3;

	// switch the entity to a skin texture at &d3d_PlayerSkins[ent->playerskin]
	// move all skin translation to here (only if translation succeeds)
	if (D3D_TranslateAliasSkin (ent))
	{
		aliasstate->teximage = &d3d_PlayerSkins[ent->playerskin];
		d3d_PlayerSkins[ent->playerskin].LastUsage = 0;
	}
	else aliasstate->teximage = hdr->skins[ent->skinnum].texture[anim];

	// fullbright texture (can be NULL)
	aliasstate->lumaimage = hdr->skins[ent->skinnum].fullbright[anim];
}


bool R_ViewInsideBBox (entity_t *ent)
{
	float bbmins[3] =
	{
		ent->origin[0] + ent->model->mins[0],
		ent->origin[1] + ent->model->mins[1],
		ent->origin[2] + ent->model->mins[2]
	};

	float bbmaxs[3] =
	{
		ent->origin[0] + ent->model->maxs[0],
		ent->origin[1] + ent->model->maxs[1],
		ent->origin[2] + ent->model->maxs[2]
	};

	if (r_origin[0] < bbmins[0]) return false;
	if (r_origin[1] < bbmins[1]) return false;
	if (r_origin[2] < bbmins[2]) return false;

	if (r_origin[0] > bbmaxs[0]) return false;
	if (r_origin[1] > bbmaxs[1]) return false;
	if (r_origin[2] > bbmaxs[2]) return false;

	// inside
	return true;
}


void R_DrawBBox (entity_t *ent)
{
	vec3_t bboxverts[8];

	unsigned short bboxindexes[36] =
	{
		0, 2, 6, 0, 6, 4,
		1, 3, 7, 1, 7, 5,
		0, 1, 3, 0, 3, 2,
		4, 5, 7, 4, 7, 6,
		0, 1, 5, 0, 5, 4,
		2, 3, 7, 2, 7, 6
	};

	bboxverts[0][0] = ent->origin[0] + ent->model->mins[0];
	bboxverts[0][1] = ent->origin[1] + ent->model->mins[1];
	bboxverts[0][2] = ent->origin[2] + ent->model->mins[2];

	bboxverts[1][0] = ent->origin[0] + ent->model->mins[0];
	bboxverts[1][1] = ent->origin[1] + ent->model->mins[1];
	bboxverts[1][2] = ent->origin[2] + ent->model->maxs[2];

	bboxverts[2][0] = ent->origin[0] + ent->model->mins[0];
	bboxverts[2][1] = ent->origin[1] + ent->model->maxs[1];
	bboxverts[2][2] = ent->origin[2] + ent->model->mins[2];

	bboxverts[3][0] = ent->origin[0] + ent->model->mins[0];
	bboxverts[3][1] = ent->origin[1] + ent->model->maxs[1];
	bboxverts[3][2] = ent->origin[2] + ent->model->maxs[2];

	bboxverts[4][0] = ent->origin[0] + ent->model->maxs[0];
	bboxverts[4][1] = ent->origin[1] + ent->model->mins[1];
	bboxverts[4][2] = ent->origin[2] + ent->model->mins[2];

	bboxverts[5][0] = ent->origin[0] + ent->model->maxs[0];
	bboxverts[5][1] = ent->origin[1] + ent->model->mins[1];
	bboxverts[5][2] = ent->origin[2] + ent->model->maxs[2];

	bboxverts[6][0] = ent->origin[0] + ent->model->maxs[0];
	bboxverts[6][1] = ent->origin[1] + ent->model->maxs[1];
	bboxverts[6][2] = ent->origin[2] + ent->model->mins[2];

	bboxverts[7][0] = ent->origin[0] + ent->model->maxs[0];
	bboxverts[7][1] = ent->origin[1] + ent->model->maxs[1];
	bboxverts[7][2] = ent->origin[2] + ent->model->maxs[2];

	D3D_DrawUserPrimitive (D3DPT_TRIANGLELIST, 8, 12, bboxindexes, bboxverts, sizeof (vec3_t));
	d3d_RenderDef.alias_polys++;
}


d3d_shader_t d3d_AliasShaderLuma =
{
	{
		// number of stages and type
		3,
		VBO_SHADER_TYPE_FIXED
	},
	{
		// color
		{D3DTOP_ADD, D3DTA_TEXTURE, D3DTA_DIFFUSE},
		{D3D_OVERBRIGHT_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT},
		{D3DTOP_DISABLE}
	},
	{
		// alpha
		{D3DTOP_DISABLE},
		{D3DTOP_DISABLE},
		{D3DTOP_DISABLE}
	}
};

d3d_shader_t d3d_AliasShaderNoLuma =
{
	{
		// number of stages and type
		3,
		VBO_SHADER_TYPE_FIXED
	},
	{
		// color
		{D3D_OVERBRIGHT_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT},
		{D3DTOP_DISABLE},
		{D3DTOP_DISABLE}
	},
	{
		// alpha
		{D3DTOP_DISABLE},
		{D3DTOP_DISABLE},
		{D3DTOP_DISABLE}
	}
};


void D3D_DrawAliasBatch (entity_t **ents, int numents)
{
	D3D_SetTextureAddressMode (D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP);
	D3D_SetTexCoordIndexes (0, 0);
	D3D_SetTextureMipmap (0, d3d_3DFilterMag, d3d_3DFilterMin, d3d_3DFilterMip);
	D3D_SetTextureMipmap (1, d3d_3DFilterMag, d3d_3DFilterMin, d3d_3DFilterMip);

	// set correct overbright mode
	d3d_AliasShaderLuma.ColorDef[1].Op = D3D_OVERBRIGHT_MODULATE;
	d3d_AliasShaderNoLuma.ColorDef[0].Op = D3D_OVERBRIGHT_MODULATE;

	D3D_SetRenderState (D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	D3D_SetVertexDeclaration (d3d_VDXyzDiffuseTex1);

	// begin a new render batch
	D3D_VBOBegin (D3DPT_TRIANGLELIST, sizeof (aliaspolyvert_t));

	for (int i = 0; i < numents; i++)
	{
		entity_t *ent = ents[i];
		aliashdr_t *hdr = ent->model->aliashdr;

		// skip conditions
		if (ent->visframe != d3d_RenderDef.framecount) continue;
		if (ent->occluded) continue;

		// take pointers for easier access
		aliasstate_t *aliasstate = &ent->aliasstate;

		// prydon gets this
		if (!aliasstate->teximage) continue;
		if (!aliasstate->teximage->d3d_Texture) continue;

		// build the transformation for this entity.
		// we need to run this in software as we are now potentially submitting multiple alias models in a single batch
		// breaking out to hardware t&l for batches that contain a single ent is actually slower in all cases...
		D3D_LoadIdentity (&ent->matrix);
		D3D_RotateForEntity (ent, &ent->matrix);

		// interpolation clearing gets this
		if (ent->currpose < 0) ent->currpose = 0;
		if (ent->lastpose < 0) ent->lastpose = 0;

		for (aliaspart_t *part = hdr->parts; part; part = part->next)
		{
			// check for overflow
			D3D_VBOCheckOverflow (part->nummesh, part->numindexes);

			// add a new shader (note: these self-filter so no worry about doing it each time)
			if (aliasstate->lumaimage && d3d_GlobalCaps.NumTMUs > 1)
				D3D_VBOAddShader (&d3d_AliasShaderLuma, aliasstate->lumaimage->d3d_Texture, aliasstate->teximage->d3d_Texture);
			else D3D_VBOAddShader (&d3d_AliasShaderNoLuma, aliasstate->teximage->d3d_Texture);

			// submit it
			D3D_VBOAddAliasVerts (ent, hdr, part, aliasstate);
			d3d_RenderDef.alias_polys += hdr->numtris;
		}
	}

	// render the alias verts
	D3D_VBORender ();

	// done
	D3D_SetRenderState (D3DRS_SHADEMODE, D3DSHADE_FLAT);
}


entity_t **d3d_AliasEdicts = NULL;
int d3d_NumAliasEdicts = 0;


int D3D_AliasModelSortFunc (entity_t **e1, entity_t **e2)
{
	return (int) (e1[0]->model - e2[0]->model);
}


#define QUERY_IDLE		0
#define QUERY_WAITING	1

// a query is either idle or waiting; makes more sense than d3d's "signalled"/etc nonsense.
// this struct contains the IDirect3DQuery9 object as well as a flag indicating it's state.  
typedef struct d3d_occlusionquery_s
{
	LPDIRECT3DQUERY9 Query;
	int State;
	entity_t *Entity;
} d3d_occlusionquery_t;


CQuakeZone *OcclusionsHeap = NULL;
d3d_occlusionquery_t **d3d_OcclusionQueries = NULL;
int d3d_NumOcclusionQueries = 0;
int numoccluded = 0;

void D3D_RegisterOcclusionQuery (entity_t *ent)
{
	// occlusions not supported
	if (!d3d_GlobalCaps.supportOcclusion) return;

	if (ent->effects & EF_NEVEROCCLUDE) return;

	// already has one
	if (ent->occlusion)
	{
		if (ent->occlusion->Entity != ent)
		{
			// if this changes we need to remove the occlusion from the entity
			ent->occlusion = NULL;
			ent->occluded = false;
		}

		// still OK
		if (ent->occlusion) return;
	}

	if (!OcclusionsHeap)
	{
		OcclusionsHeap = new CQuakeZone ();
		d3d_OcclusionQueries = (d3d_occlusionquery_t **) OcclusionsHeap->Alloc (MAX_VISEDICTS * sizeof (d3d_occlusionquery_t *));
		d3d_NumOcclusionQueries = 0;
	}

	// create a new query
	d3d_occlusionquery_t *q = (d3d_occlusionquery_t *) OcclusionsHeap->Alloc (sizeof (d3d_occlusionquery_t));
	ent->occlusion = q;
	d3d_OcclusionQueries[d3d_NumOcclusionQueries] = ent->occlusion;
	d3d_NumOcclusionQueries++;

	ent->occlusion->Query = NULL;
	ent->occlusion->State = QUERY_IDLE;
	ent->occlusion->Entity = ent;

	// entity is not occluded be default
	ent->occluded = false;
}


void D3D_ClearOcclusionQueries (void)
{
	if (!OcclusionsHeap) return;
	if (!d3d_GlobalCaps.supportOcclusion) return;

	for (int i = 0; i < d3d_NumOcclusionQueries; i++)
	{
		d3d_OcclusionQueries[i]->Entity->occlusion = NULL;
		d3d_OcclusionQueries[i]->Entity->occluded = false;
		SAFE_RELEASE (d3d_OcclusionQueries[i]->Query);
	}

	d3d_OcclusionQueries = NULL;
	d3d_NumOcclusionQueries = 0;
	SAFE_DELETE (OcclusionsHeap);
}


void D3D_IssueQuery (entity_t *ent, d3d_occlusionquery_t *q)
{
	// create on demand
	if (!q->Query)
	{
		// all queries are created idle (if only the rest of life was like that!)
		d3d_Device->CreateQuery (D3DQUERYTYPE_OCCLUSION, &q->Query);
		q->State = QUERY_IDLE;
	}

	// if this is true then the query is currently busy doing something and cannot be issued
	if (q->State != QUERY_IDLE) return;

	// issue the query
	q->Query->Issue (D3DISSUE_BEGIN);
	R_DrawBBox (ent);
	q->Query->Issue (D3DISSUE_END);

	// the query is now waiting
	q->State = QUERY_WAITING;
}


// we need to return -1 because 0 is a valid result and therefore we need
// another means of signalling that the query is not ready yet
int D3D_GetQueryResults (d3d_occlusionquery_t *q)
{
	// create on demand
	if (!q->Query)
	{
		// all queries are created idle (if only the rest of life was like that!)
		d3d_Device->CreateQuery (D3DQUERYTYPE_OCCLUSION, &q->Query);
		q->State = QUERY_IDLE;

		// no valid result to return
		return -1;
	}

	// query was not issued
	if (q->State != QUERY_WAITING) return -1;

	DWORD Visible = 0;

	// peek at the query to get it's result
	hr = q->Query->GetData ((void *) &Visible, sizeof (DWORD), D3DGETDATA_FLUSH);

	if (hr == S_OK)
	{
		// query was completed; go back to idle and return the result
		q->State = QUERY_IDLE;
		return (Visible > 1 ? 1 : 0);
	}
	else if (hr == S_FALSE)
	{
		// still waiting
		return -1;
	}
	else
	{
		// something bad happened; destroy the query
		SAFE_RELEASE (q->Query);
		return -1;
	}
}


void D3D_RunOcclusionQueries (entity_t **ents, int numents)
{
	if (cls.timedemo) return;
	if (!d3d_GlobalCaps.supportOcclusion) return;

	bool stateset = false;

	//if (numoccluded) Con_Printf ("occluded %i entities\n", numoccluded);

	numoccluded = 0;

	// if (d3d_NumOcclusionQueries) Con_Printf ("%i occlusion queries\n", d3d_NumOcclusionQueries);

	for (int i = 0; i < numents; i++)
	{
		// some ents never get occlusion queries on them
		if (ents[i]->effects & EF_NEVEROCCLUDE) continue;

		// don't bother if the model is too simple
		if (ents[i]->model->aliashdr->numtris < 24) continue;

		// if there are a small enough number of entities on screen we don't bother either
		if (d3d_NumAliasEdicts < 5)
		{
			ents[i]->occluded = false;
			continue;
		}

		// if the view is inside the bbox don't check for occlusion on the entity
		if (R_ViewInsideBBox (ents[i]))
		{
			// Con_Printf ("view inside bbox for %s\n", ents[i]->model->name);
			ents[i]->occluded = false;
			continue;
		}

		// register a query for the entity
		D3D_RegisterOcclusionQuery (ents[i]);

		// no occlusion object (should never happen...)
		if (!ents[i]->occlusion)
		{
			// not occluded
			ents[i]->occluded = false;
			continue;
		}

		if (!stateset)
		{
			// only change state if we need to
			D3D_SetVertexDeclaration (d3d_VDXyz);
			D3D_SetTextureColorMode (0, D3DTOP_DISABLE);
			D3D_SetRenderState (D3DRS_COLORWRITEENABLE, 0);
			D3D_SetRenderState (D3DRS_ZWRITEENABLE, FALSE);
			D3D_SetRenderState (D3DRS_CULLMODE, D3DCULL_NONE);

			stateset = true;
		}

		// get the last set of results
		int blah = D3D_GetQueryResults (ents[i]->occlusion);

		switch (blah)
		{
		case 0:
			// ent is occluded
			ents[i]->occluded = true;
			numoccluded++;
			break;

		case -1:
			// no results in yet
			break;

		default:
			// entity is not occluded
			ents[i]->occluded = false;
		}

		// issue this set
		D3D_IssueQuery (ents[i], ents[i]->occlusion);
	}

	if (stateset)
	{
		D3D_SetRenderState (D3DRS_COLORWRITEENABLE, 0x0000000F);
		D3D_BackfaceCull (D3DCULL_CCW);
		D3D_SetRenderState (D3DRS_ZWRITEENABLE, TRUE);
	}
}


void D3D_UpdateOcclusionQueries (void)
{
	if (cls.timedemo) return;
	if (!d3d_GlobalCaps.supportOcclusion) return;

	numoccluded = 0;

	// this is run every pass through the main loop even if other updates are skipped (by Host_FilterTime).
	// it needs to be run for entities that are also not in the PVS/frustum so that queries which have expired
	// have the best chance of being available for testing again the next time the entity is visible.
	for (int i = 0; i < d3d_NumOcclusionQueries; i++)
	{
		entity_t *ent = d3d_OcclusionQueries[i]->Entity;

		if (!ent->occlusion) continue;
		if (ent->effects & EF_NEVEROCCLUDE) continue;

		d3d_occlusionquery_t *q = ent->occlusion;

		// query has not yet been issued
		if (!q->Query) continue;
		if (q->State != QUERY_WAITING) continue;

		DWORD Visible;

		// query status but don't flush
		hr = q->Query->GetData ((void *) &Visible, sizeof (DWORD), 0);

		if (hr == S_OK)
		{
			if (Visible)
				ent->occluded = false;
			else
			{
				ent->occluded = true;
				numoccluded++;
			}

			q->State = QUERY_IDLE;
		}
		else if (hr == S_FALSE)
		{
			// no change
		}
		else
		{
			// auto-destruct
			SAFE_RELEASE (q->Query);
		}
	}
}


void D3D_RenderAliasModels (void)
{
	if (!r_drawentities.integer) return;

	if (!d3d_AliasEdicts) d3d_AliasEdicts = (entity_t **) Zone_Alloc (sizeof (entity_t *) * MAX_VISEDICTS);
	d3d_NumAliasEdicts = 0;

	for (int i = 0; i < d3d_RenderDef.numvisedicts; i++)
	{
		entity_t *ent = d3d_RenderDef.visedicts[i];

		if (!ent->model) continue;
		if (ent->model->type != mod_alias) continue;

		// initial setup
		D3D_SetupAliasModel (ent);

		if (ent->visframe != d3d_RenderDef.framecount) continue;

		if (ent->alphaval < 255)
			D3D_AddToAlphaList (ent);
		else d3d_AliasEdicts[d3d_NumAliasEdicts++] = ent;
	}

	if (!d3d_NumAliasEdicts) return;

	// sort the alias edicts by model
	// (to do - chain these in a list instead to save memory, remove limits and run faster...)
	qsort
	(
		d3d_AliasEdicts,
		d3d_NumAliasEdicts,
		sizeof (entity_t *),
		(int (*) (const void *, const void *)) D3D_AliasModelSortFunc
	);

	// everything that's needed for occlusion queries
	D3D_RunOcclusionQueries (d3d_AliasEdicts, d3d_NumAliasEdicts);
	D3D_DrawAliasBatch (d3d_AliasEdicts, d3d_NumAliasEdicts);
	D3D_DrawAliasShadows (d3d_AliasEdicts, d3d_NumAliasEdicts);
}


void R_DrawViewModel (void)
{
	// conditions for switching off view model
	if (r_drawviewmodel.value < 0.01f) return;
	if (chase_active.value) return;
	if (cl.stats[STAT_HEALTH] <= 0) return;
	if (!cl.viewent.model) return;
	if (!r_drawentities.value) return;
	if (d3d_RenderDef.automap) return;

	// select view ent
	entity_t *ent = &cl.viewent;
	ent->occluded = false;

	// the viewmodel should always be an alias model
	if (ent->model->type != mod_alias) return;

	// never check for bbox culling on the viewmodel
	ent->nocullbox = true;

	// hack the depth range to prevent view model from poking into walls
	D3D_SetRenderStatef (D3DRS_DEPTHBIAS, -0.3f);

	// delerp muzzleflashes here
	DelerpMuzzleFlashes (ent->model->aliashdr);

	if ((cl.items & IT_INVISIBILITY) || r_drawviewmodel.value < 0.99f)
	{
		// enable blending
		D3D_SetRenderState (D3DRS_ALPHABLENDENABLE, TRUE);
		D3D_SetRenderState (D3DRS_BLENDOP, D3DBLENDOP_ADD);
		D3D_SetRenderState (D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		D3D_SetRenderState (D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		D3D_SetRenderState (D3DRS_ZWRITEENABLE, FALSE);

		// initial alpha
		ent->alphaval = (int) (r_drawviewmodel.value * 255.0f);

		// adjust for invisibility
		if (cl.items & IT_INVISIBILITY) ent->alphaval >>= 1;

		// final range
		ent->alphaval = BYTE_CLAMP (ent->alphaval);
	}

	// adjust projection to a constant y fov for wide-angle views
	if (d3d_RenderDef.fov_x > 88.741)
	{
		D3DXMATRIX fovmatrix;
		QD3DXMatrixPerspectiveFovRH (&fovmatrix, D3DXToRadian (68.038704f), ((float) d3d_CurrentMode.Width / (float) d3d_CurrentMode.Height), 4, 4096);
		d3d_Device->SetTransform (D3DTS_PROJECTION, &fovmatrix);
	}

	// add it to the list
	D3D_SetupAliasModel (ent);
	D3D_DrawAliasBatch (&ent, 1);

	// restore original projection
	if (d3d_RenderDef.fov_x >= 90)
		d3d_Device->SetTransform (D3DTS_PROJECTION, &d3d_ProjMatrix);

	if (cl.items & IT_INVISIBILITY)
	{
		// disable blending (done)
		ent->alphaval = 255;
		D3D_SetRenderState (D3DRS_ZWRITEENABLE, TRUE);
		D3D_SetRenderState (D3DRS_ALPHABLENDENABLE, FALSE);
	}

	// restore the hacked depth range
	D3D_SetRenderStatef (D3DRS_DEPTHBIAS, 0.0f);
}



void D3D_CreateShadeDots (void)
{
	// already done
	if (r_avertexnormal_dots_lerp) return;

	float *r_avertexnormal_dots;
	int dotslen = Sys_LoadResourceData (IDR_ANORMDOTS, (void **) &r_avertexnormal_dots);

	if (dotslen != 16384) Sys_Error ("Corrupted dots lump!");

	// create the buffer to hold it
	r_avertexnormal_dots_lerp = (float **) Zone_Alloc (sizeof (float *) * SHADEDOT_QUANT_LERP);

	for (int i = 0; i < SHADEDOT_QUANT_LERP; i++)
		r_avertexnormal_dots_lerp[i] = (float *) Zone_Alloc (sizeof (float) * 256);

	// now interpolate between them
	for (int i = 0, j = 1; i < SHADEDOT_QUANT; i++, j++)
	{
		int diff = SHADEDOT_QUANT_LERP / SHADEDOT_QUANT;

		for (int dot = 0; dot < 256; dot++)
		{
			// these are the 2 points to interpolate between
			float l1 = r_avertexnormal_dots[(i & (SHADEDOT_QUANT - 1)) * 256 + dot];
			float l2 = r_avertexnormal_dots[(j & (SHADEDOT_QUANT - 1)) * 256 + dot];

			for (int d = 0; d < diff; d++)
			{
				// this is the point we're going to write into
				int p = ((i * diff) + d) & (SHADEDOT_QUANT_LERP - 1);

				// these are the two results of the linear interpolation
				float p1 = (((float) diff - d) / (float) diff) * l1;
				float p2 = ((float) d / (float) diff) * l2;

				// and we write it in
				r_avertexnormal_dots_lerp[p][dot] = p1 + p2;
			}
		}
	}
}


