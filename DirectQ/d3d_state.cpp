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

#include "quakedef.h"
#include "d3d_model.h"
#include "d3d_quake.h"
#include "winquake.h"

/*
===================================================================================================================

		STATE MANAGEMENT

	Here Direct3D makes things really easy for us by using a separate enum and separate member functions for
	each type of state.  This would be a fucking nightmare under OpenGL...

	Note that the runtime will filter state changes on a non-pure device, but we'll also do it ourselves to
	avoid the overhead of sending them to the runtime in the first place, which may be more optimal

===================================================================================================================
*/


DWORD d3d_RenderStates[256];

void D3D_SetRenderState (D3DRENDERSTATETYPE State, DWORD Value)
{
	if (d3d_RenderStates[(int) State] == Value)
	{
		// filter out rendundant changes before they go to the API
		return;
	}
	else
	{
		d3d_Device->SetRenderState (State, Value);
		d3d_RenderStates[(int) State] = Value;
	}
}


void D3D_SetRenderStatef (D3DRENDERSTATETYPE State, float Value)
{
	// some states require float input
	D3D_SetRenderState (State, ((DWORD *) &Value)[0]);
}


DWORD d3d_SamplerStates[8][16];

void D3D_SetSamplerState (UINT sampler, D3DSAMPLERSTATETYPE type, DWORD state)
{
	if (d3d_SamplerStates[sampler][(int) type] == state)
	{
		// filter out rendundant changes before they go to the API
		return;
	}
	else
	{
		d3d_Device->SetSamplerState (sampler, type, state);
		d3d_SamplerStates[sampler][(int) type] = state;
	}
}


typedef struct d3d_streamsource_s
{
	LPDIRECT3DVERTEXBUFFER9 vb;
	DWORD offset;
	DWORD stride;
	UINT freq;
} d3d_streamsource_t;

d3d_streamsource_t d3d_VertexStreams[16];

void D3D_SetStreamSource (DWORD stream, LPDIRECT3DVERTEXBUFFER9 vb, DWORD offset, DWORD stride, UINT freq)
{
	d3d_streamsource_t *str = &d3d_VertexStreams[stream];

	if (str->offset != offset || str->stride != stride || str->vb != vb)
	{
		d3d_Device->SetStreamSource (stream, vb, offset, stride);
		if (vb) d3d_RenderDef.numsss++;

		str->offset = offset;
		str->stride = stride;
		str->vb = vb;
	}

	if (str->freq != freq && d3d_GlobalCaps.supportInstancing)
	{
		d3d_Device->SetStreamSourceFreq (stream, freq);
		str->freq = freq;
	}
}


LPDIRECT3DINDEXBUFFER9 d3d_IB = NULL;

void D3D_SetIndices (LPDIRECT3DINDEXBUFFER9 ib)
{
	if (d3d_IB == ib)
	{
		// filter out rendundant changes before they go to the API
		return;
	}
	else
	{
		d3d_Device->SetIndices (ib);
		d3d_IB = ib;
	}
}


void D3D_UnbindStreams (void)
{
	for (int i = 0; i < 16; i++)
	{
		if (d3d_VertexStreams[i].vb)
		{
			d3d_VertexStreams[i].vb = NULL;
			d3d_VertexStreams[i].freq = 0;
			d3d_VertexStreams[i].offset = 0;
			d3d_VertexStreams[i].stride = 0;

			d3d_Device->SetStreamSource (i, NULL, 0, 0);
		}
	}

	D3D_SetIndices (NULL);
}


LPDIRECT3DVERTEXDECLARATION9 d3d_VD = NULL;

void D3D_SetVertexDeclaration (LPDIRECT3DVERTEXDECLARATION9 vd)
{
	if (d3d_VD == vd)
	{
		// filter out rendundant changes before they go to the API
		return;
	}
	else
	{
		// the debug runtime reports a NULL vertex decl as invalid so just store it
		if (vd) d3d_Device->SetVertexDeclaration (vd);
		d3d_VD = vd;
	}
}


void D3D_SetAllStates (void)
{
	// force all renderstates to recache
	for (int i = 0; i < 256; i++)
		d3d_RenderStates[i] = 0x35012560;

	for (int i = 0; i < 16; i++)
		for (int s = 0; s < 8; s++)
			d3d_SamplerStates[s][i] = 0x35012560;

	// force vbos, ibos and vertdecls to rebind
	for (int s = 0; s < 16; s++)
	{
		// force a reset with vb NULL
		d3d_VertexStreams[s].offset = ~d3d_VertexStreams[s].offset;
		d3d_VertexStreams[s].stride = ~d3d_VertexStreams[s].stride;
		d3d_VertexStreams[s].vb = NULL;
	}

	d3d_VD = NULL;
	d3d_IB = NULL;

	// force hlsl state to recache
	D3DHLSL_InvalidateState ();
}


/*
========================================================================================================================

	BATCHED UP STATES

========================================================================================================================
*/

void D3D_BackfaceCull (DWORD D3D_CULLTYPE)
{
	// culling passes through here instead of direct so that we can test the gl_cull cvar
	if (!gl_cull.value)
		D3D_SetRenderState (D3DRS_CULLMODE, D3DCULL_NONE);
	else D3D_SetRenderState (D3DRS_CULLMODE, D3D_CULLTYPE);
}


void D3D_SetTextureAddress (DWORD stage, DWORD mode)
{
	D3D_SetSamplerState (stage, D3DSAMP_ADDRESSU, mode);
	D3D_SetSamplerState (stage, D3DSAMP_ADDRESSV, mode);
	D3D_SetSamplerState (stage, D3DSAMP_ADDRESSW, mode);
}


#define USING_ANISOTROPY(cap) (r_anisotropicfilter.integer > 1 && d3d_DeviceCaps.MaxAnisotropy > 1 && (d3d_DeviceCaps.TextureFilterCaps & cap))

void D3D_SetTextureFilter (DWORD stage, D3DSAMPLERSTATETYPE type, D3DTEXTUREFILTERTYPE desired)
{
	D3DTEXTUREFILTERTYPE actual = desired;
	extern cvar_t r_anisotropicfilter;
	int filter = 0;

	// check against caps and gracefully degrade
	switch (type)
	{
	case D3DSAMP_MAGFILTER:
		filter = 0;

		switch (desired)
		{
		case D3DTEXF_LINEAR:
			if (d3d_DeviceCaps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR)
			{
				if (USING_ANISOTROPY (D3DPTFILTERCAPS_MAGFANISOTROPIC))
					actual = D3DTEXF_ANISOTROPIC;
				else actual = D3DTEXF_LINEAR;

				break;
			}

		default:
			actual = D3DTEXF_POINT;
		}

		break;

	case D3DSAMP_MINFILTER:
		filter = 2;

		switch (desired)
		{
		case D3DTEXF_LINEAR:
			if (d3d_DeviceCaps.TextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR)
			{
				if (USING_ANISOTROPY (D3DPTFILTERCAPS_MINFANISOTROPIC))
					actual = D3DTEXF_ANISOTROPIC;
				else actual = D3DTEXF_LINEAR;

				break;
			}

		default:
			actual = D3DTEXF_POINT;
		}

		break;


	case D3DSAMP_MIPFILTER:
		filter = 1;

		switch (desired)
		{
		case D3DTEXF_LINEAR:
			if (d3d_DeviceCaps.TextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR)
			{
				actual = D3DTEXF_LINEAR;
				break;
			}

		case D3DTEXF_NONE:
			// corrected no mip filtering
			break;

		default:
			if (d3d_DeviceCaps.TextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT)
			{
				actual = D3DTEXF_POINT;
				break;
			}

			actual = D3DTEXF_NONE;
		}

		break;

	default:
		return;
	}

	D3D_SetSamplerState (stage, type, actual);
}


void D3D_SetTextureMipmap (DWORD stage, D3DTEXTUREFILTERTYPE texfilter, D3DTEXTUREFILTERTYPE mipfilter)
{
	D3D_SetTextureFilter (stage, D3DSAMP_MAGFILTER, texfilter);
	D3D_SetTextureFilter (stage, D3DSAMP_MINFILTER, texfilter);
	D3D_SetTextureFilter (stage, D3DSAMP_MIPFILTER, mipfilter);
}


void D3DState_PicmipChange (cvar_t *var)
{
	float picmip = 0;

	if (var->integer < 0)
		picmip = 0;
	else picmip = var->integer;

	for (int i = 0; i < 8; i++)
	{
		D3D_SetSamplerState (i, D3DSAMP_MAXMIPLEVEL, var->integer > 0 ? var->integer : 0);
	}
}


cvar_t gl_picmip ("gl_picmip", "0", 0, D3DState_PicmipChange);


void D3DState_SetAlphaBlend (DWORD enable, DWORD op, DWORD srcblend, DWORD dstblend)
{
	if (enable)
	{
		D3D_SetRenderState (D3DRS_ALPHABLENDENABLE, TRUE);
		D3D_SetRenderState (D3DRS_BLENDOP, op);
		D3D_SetRenderState (D3DRS_SRCBLEND, srcblend);
		D3D_SetRenderState (D3DRS_DESTBLEND, dstblend);
	}
	else D3D_SetRenderState (D3DRS_ALPHABLENDENABLE, FALSE);
}


void D3DState_SetZBuffer (D3DZBUFFERTYPE enable, DWORD write, D3DCMPFUNC comp)
{
	D3D_SetRenderState (D3DRS_ZENABLE, enable);
	D3D_SetRenderState (D3DRS_ZWRITEENABLE, write);
	D3D_SetRenderState (D3DRS_ZFUNC, comp);
}


void D3DState_SetStencil (BOOL enable)
{
	if (enable)
	{
		D3D_SetRenderState (D3DRS_STENCILENABLE, TRUE);
		D3D_SetRenderState (D3DRS_STENCILFUNC, D3DCMP_EQUAL);
		D3D_SetRenderState (D3DRS_STENCILREF, 0x00000001);
		D3D_SetRenderState (D3DRS_STENCILMASK, 0x00000002);
		D3D_SetRenderState (D3DRS_STENCILWRITEMASK, 0xFFFFFFFF);
		D3D_SetRenderState (D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
		D3D_SetRenderState (D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
		D3D_SetRenderState (D3DRS_STENCILPASS, D3DSTENCILOP_INCRSAT);
	}
	else D3D_SetRenderState (D3DRS_STENCILENABLE, FALSE);
}


void D3DState_SetAlphaTest (BOOL enable, D3DCMPFUNC comp, DWORD ref)
{
	if (enable)
	{
		D3D_SetRenderState (D3DRS_ALPHATESTENABLE, TRUE);
		D3D_SetRenderState (D3DRS_ALPHAFUNC, comp);
		D3D_SetRenderState (D3DRS_ALPHAREF, ref);
	}
	else D3D_SetRenderState (D3DRS_ALPHATESTENABLE, FALSE);
}


void D3DState_EnableShadows (bool enable)
{
	if (enable)
	{
		D3DState_SetAlphaBlend (TRUE);
		D3DState_SetZBuffer (D3DZB_TRUE, FALSE);

		// of course, we all know that Direct3D lacks Stencil Buffer and Polygon Offset support,
		// so what you're looking at here doesn't really exist.  Just pretend you didn't see it,
		// you'll be OK
		if (d3d_GlobalCaps.DepthStencilFormat == D3DFMT_D24S8)
		{
			D3DState_SetStencil (TRUE);
			D3DHLSL_SetAlpha (r_shadows.value);
		}
		else D3DHLSL_SetAlpha (1.0f);
	}
	else
	{
		if (d3d_GlobalCaps.DepthStencilFormat == D3DFMT_D24S8)
			D3DState_SetStencil (FALSE);

		D3DState_SetAlphaBlend (FALSE);
		D3DState_SetZBuffer (D3DZB_TRUE, TRUE);
	}
}

