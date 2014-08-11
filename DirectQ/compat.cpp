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

// contains copies of the missing cvars and commands from WinQuake and GLQuake for purposes
// of soaking up abuse from mods that like to dance with the devil that is stuffcmd.
// any other compatibility soaks and suchlike should also go here...
#include "quakedef.h"

// register compatiblity cvars.  if these already exist as a real cvar they will be just ignored.
// if a real cvar is registered with the same name as a prior compatibility cvar the real cvar will stomp the compatibility cvar
void Cvar_MakeCompatLayer (void)
{
	cvar_t *scr_screensize_compat = new cvar_t ("screensize", "0", CVAR_COMPAT);
	cvar_t *chase_back_compat = new cvar_t ("chase_back", "0", CVAR_COMPAT);
	cvar_t *chase_up_compat = new cvar_t ("chase_up", "0", CVAR_COMPAT);
	cvar_t *chase_right_compat = new cvar_t ("chase_right", "0", CVAR_COMPAT);
	cvar_t *chase_active_compat = new cvar_t ("chase_active", "0", CVAR_COMPAT);
	cvar_t *cl_upspeed_compat = new cvar_t ("cl_upspeed", "0", CVAR_COMPAT);
	cvar_t *cl_forwardspeed_compat = new cvar_t ("cl_forwardspeed", "0", CVAR_COMPAT);
	cvar_t *cl_backspeed_compat = new cvar_t ("cl_backspeed", "0", CVAR_COMPAT);
	cvar_t *cl_sidespeed_compat = new cvar_t ("cl_sidespeed", "0", CVAR_COMPAT);
	cvar_t *cl_movespeedkey_compat = new cvar_t ("cl_movespeedkey", "0", CVAR_COMPAT);
	cvar_t *cl_yawspeed_compat = new cvar_t ("cl_yawspeed", "0", CVAR_COMPAT);
	cvar_t *cl_pitchspeed_compat = new cvar_t ("cl_pitchspeed", "0", CVAR_COMPAT);
	cvar_t *cl_anglespeedkey_compat = new cvar_t ("cl_anglespeedkey", "0", CVAR_COMPAT);
	cvar_t *cl_name_compat = new cvar_t ("_cl_name", "0", CVAR_COMPAT);
	cvar_t *cl_color_compat = new cvar_t ("_cl_color", "0", CVAR_COMPAT);
	cvar_t *cl_shownet_compat = new cvar_t ("cl_shownet", "0", CVAR_COMPAT);
	cvar_t *cl_nolerp_compat = new cvar_t ("cl_nolerp", "0", CVAR_COMPAT);
	cvar_t *lookspring_compat = new cvar_t ("lookspring", "0", CVAR_COMPAT);
	cvar_t *lookstrafe_compat = new cvar_t ("lookstrafe", "0", CVAR_COMPAT);
	cvar_t *sensitivity_compat = new cvar_t ("sensitivity", "0", CVAR_COMPAT);
	cvar_t *m_pitch_compat = new cvar_t ("m_pitch", "0", CVAR_COMPAT);
	cvar_t *m_yaw_compat = new cvar_t ("m_yaw", "0", CVAR_COMPAT);
	cvar_t *m_forward_compat = new cvar_t ("m_forward", "0", CVAR_COMPAT);
	cvar_t *m_side_compat = new cvar_t ("m_side", "0", CVAR_COMPAT);
	cvar_t *registered_compat = new cvar_t ("registered", "0", CVAR_COMPAT);
	cvar_t *cmdline_compat = new cvar_t ("cmdline", "0", CVAR_COMPAT);
	cvar_t *con_notifytime_compat = new cvar_t ("con_notifytime", "0", CVAR_COMPAT);
	cvar_t *d_subdiv16_compat = new cvar_t ("d_subdiv16", "0", CVAR_COMPAT);
	cvar_t *d_mipcap_compat = new cvar_t ("d_mipcap", "0", CVAR_COMPAT);
	cvar_t *d_mipscale_compat = new cvar_t ("d_mipscale", "0", CVAR_COMPAT);
	cvar_t *gl_nobind_compat = new cvar_t ("gl_nobind", "0", CVAR_COMPAT);
	cvar_t *gl_subdivide_size_compat = new cvar_t ("gl_subdivide_size", "0", CVAR_COMPAT);
	cvar_t *r_norefresh_compat = new cvar_t ("r_norefresh", "0", CVAR_COMPAT);
	cvar_t *r_drawentities_compat = new cvar_t ("r_drawentities", "0", CVAR_COMPAT);
	cvar_t *r_drawviewmodel_compat = new cvar_t ("r_drawviewmodel", "0", CVAR_COMPAT);
	cvar_t *r_fullbright_compat = new cvar_t ("r_fullbright", "0", CVAR_COMPAT);
	cvar_t *r_lightmap_compat = new cvar_t ("r_lightmap", "0", CVAR_COMPAT);
	cvar_t *r_shadows_compat = new cvar_t ("r_shadows", "0", CVAR_COMPAT);
	cvar_t *r_mirroralpha_compat = new cvar_t ("r_mirroralpha", "0", CVAR_COMPAT);
	cvar_t *r_wateralpha_compat = new cvar_t ("r_wateralpha", "0", CVAR_COMPAT);
	cvar_t *r_dynamic_compat = new cvar_t ("r_dynamic", "0", CVAR_COMPAT);
	cvar_t *r_novis_compat = new cvar_t ("r_novis", "0", CVAR_COMPAT);
	cvar_t *gl_finish_compat = new cvar_t ("gl_finish", "0", CVAR_COMPAT);
	cvar_t *gl_cull_compat = new cvar_t ("gl_cull", "0", CVAR_COMPAT);
	cvar_t *gl_texsort_compat = new cvar_t ("gl_texsort", "0", CVAR_COMPAT);
	cvar_t *gl_smoothmodels_compat = new cvar_t ("gl_smoothmodels", "0", CVAR_COMPAT);
	cvar_t *gl_affinemodels_compat = new cvar_t ("gl_affinemodels", "0", CVAR_COMPAT);
	cvar_t *gl_polyblend_compat = new cvar_t ("gl_polyblend", "0", CVAR_COMPAT);
	cvar_t *gl_playermip_compat = new cvar_t ("gl_playermip", "0", CVAR_COMPAT);
	cvar_t *gl_nocolors_compat = new cvar_t ("gl_nocolors", "0", CVAR_COMPAT);
	cvar_t *gl_reporttjunctions_compat = new cvar_t ("gl_reporttjunctions", "0", CVAR_COMPAT);
	cvar_t *scr_fov_compat = new cvar_t ("fov", "0", CVAR_COMPAT);
	cvar_t *scr_conspeed_compat = new cvar_t ("scr_conspeed", "0", CVAR_COMPAT);
	cvar_t *scr_centertime_compat = new cvar_t ("scr_centertime", "0", CVAR_COMPAT);
	cvar_t *scr_showram_compat = new cvar_t ("showram", "0", CVAR_COMPAT);
	cvar_t *scr_showturtle_compat = new cvar_t ("showturtle", "0", CVAR_COMPAT);
	cvar_t *scr_showpause_compat = new cvar_t ("showpause", "0", CVAR_COMPAT);
	cvar_t *scr_printspeed_compat = new cvar_t ("scr_printspeed", "0", CVAR_COMPAT);
	cvar_t *gl_ztrick_compat = new cvar_t ("gl_ztrick", "0", CVAR_COMPAT);
	cvar_t *vid_mode_compat = new cvar_t ("vid_mode", "0", CVAR_COMPAT);
	cvar_t *_vid_default_mode_compat = new cvar_t ("_vid_default_mode", "0", CVAR_COMPAT);
	cvar_t *_vid_default_mode_win_compat = new cvar_t ("_vid_default_mode_win", "0", CVAR_COMPAT);
	cvar_t *vid_wait_compat = new cvar_t ("vid_wait", "0", CVAR_COMPAT);
	cvar_t *vid_nopageflip_compat = new cvar_t ("vid_nopageflip", "0", CVAR_COMPAT);
	cvar_t *_vid_wait_override_compat = new cvar_t ("_vid_wait_override", "0", CVAR_COMPAT);
	cvar_t *vid_config_x_compat = new cvar_t ("vid_config_x", "0", CVAR_COMPAT);
	cvar_t *vid_config_y_compat = new cvar_t ("vid_config_y", "0", CVAR_COMPAT);
	cvar_t *_windowed_mouse_compat = new cvar_t ("_windowed_mouse", "0", CVAR_COMPAT);
	cvar_t *host_speeds_compat = new cvar_t ("host_speeds", "0", CVAR_COMPAT);
	cvar_t *sys_ticrate_compat = new cvar_t ("sys_ticrate", "0", CVAR_COMPAT);
	cvar_t *serverprofile_compat = new cvar_t ("serverprofile", "0", CVAR_COMPAT);
	cvar_t *fraglimit_compat = new cvar_t ("fraglimit", "0", CVAR_COMPAT);
	cvar_t *timelimit_compat = new cvar_t ("timelimit", "0", CVAR_COMPAT);
	cvar_t *teamplay_compat = new cvar_t ("teamplay", "0", CVAR_COMPAT);
	cvar_t *samelevel_compat = new cvar_t ("samelevel", "0", CVAR_COMPAT);
	cvar_t *noexit_compat = new cvar_t ("noexit", "0", CVAR_COMPAT);
	cvar_t *developer_compat = new cvar_t ("developer", "0", CVAR_COMPAT);
	cvar_t *skill_compat = new cvar_t ("skill", "0", CVAR_COMPAT);
	cvar_t *deathmatch_compat = new cvar_t ("deathmatch", "0", CVAR_COMPAT);
	cvar_t *coop_compat = new cvar_t ("coop", "0", CVAR_COMPAT);
	cvar_t *pausable_compat = new cvar_t ("pausable", "0", CVAR_COMPAT);
	cvar_t *temp1_compat = new cvar_t ("temp1", "0", CVAR_COMPAT);
	cvar_t *in_joystick_compat = new cvar_t ("joystick", "0", CVAR_COMPAT);
	cvar_t *joy_name_compat = new cvar_t ("joyname", "0", CVAR_COMPAT);
	cvar_t *joy_advanced_compat = new cvar_t ("joyadvanced", "0", CVAR_COMPAT);
	cvar_t *joy_advaxisx_compat = new cvar_t ("joyadvaxisx", "0", CVAR_COMPAT);
	cvar_t *joy_advaxisy_compat = new cvar_t ("joyadvaxisy", "0", CVAR_COMPAT);
	cvar_t *joy_advaxisz_compat = new cvar_t ("joyadvaxisz", "0", CVAR_COMPAT);
	cvar_t *joy_advaxisr_compat = new cvar_t ("joyadvaxisr", "0", CVAR_COMPAT);
	cvar_t *joy_advaxisu_compat = new cvar_t ("joyadvaxisu", "0", CVAR_COMPAT);
	cvar_t *joy_advaxisv_compat = new cvar_t ("joyadvaxisv", "0", CVAR_COMPAT);
	cvar_t *joy_forwardthreshold_compat = new cvar_t ("joyforwardthreshold", "0", CVAR_COMPAT);
	cvar_t *joy_sidethreshold_compat = new cvar_t ("joysidethreshold", "0", CVAR_COMPAT);
	cvar_t *joy_pitchthreshold_compat = new cvar_t ("joypitchthreshold", "0", CVAR_COMPAT);
	cvar_t *joy_yawthreshold_compat = new cvar_t ("joyyawthreshold", "0", CVAR_COMPAT);
	cvar_t *joy_forwardsensitivity_compat = new cvar_t ("joyforwardsensitivity", "0", CVAR_COMPAT);
	cvar_t *joy_sidesensitivity_compat = new cvar_t ("joysidesensitivity", "0", CVAR_COMPAT);
	cvar_t *joy_pitchsensitivity_compat = new cvar_t ("joypitchsensitivity", "0", CVAR_COMPAT);
	cvar_t *joy_yawsensitivity_compat = new cvar_t ("joyyawsensitivity", "0", CVAR_COMPAT);
	cvar_t *joy_wwhack1_compat = new cvar_t ("joywwhack1", "0", CVAR_COMPAT);
	cvar_t *joy_wwhack2_compat = new cvar_t ("joywwhack2", "0", CVAR_COMPAT);
	cvar_t *net_messagetimeout_compat = new cvar_t ("net_messagetimeout", "0", CVAR_COMPAT);
	cvar_t *hostname_compat = new cvar_t ("hostname", "0", CVAR_COMPAT);
	cvar_t *config_com_port_compat = new cvar_t ("_config_com_port", "0", CVAR_COMPAT);
	cvar_t *config_com_irq_compat = new cvar_t ("_config_com_irq", "0", CVAR_COMPAT);
	cvar_t *config_com_baud_compat = new cvar_t ("_config_com_baud", "0", CVAR_COMPAT);
	cvar_t *config_com_modem_compat = new cvar_t ("_config_com_modem", "0", CVAR_COMPAT);
	cvar_t *config_modem_dialtype_compat = new cvar_t ("_config_modem_dialtype", "0", CVAR_COMPAT);
	cvar_t *config_modem_clear_compat = new cvar_t ("_config_modem_clear", "0", CVAR_COMPAT);
	cvar_t *config_modem_init_compat = new cvar_t ("_config_modem_init", "0", CVAR_COMPAT);
	cvar_t *config_modem_hangup_compat = new cvar_t ("_config_modem_hangup", "0", CVAR_COMPAT);
	cvar_t *idgods_compat = new cvar_t ("idgods", "0", CVAR_COMPAT);
	cvar_t *sv_aim_compat = new cvar_t ("sv_aim", "0", CVAR_COMPAT);
	cvar_t *nomonsters_compat = new cvar_t ("nomonsters", "0", CVAR_COMPAT);
	cvar_t *gamecfg_compat = new cvar_t ("gamecfg", "0", CVAR_COMPAT);
	cvar_t *scratch1_compat = new cvar_t ("scratch1", "0", CVAR_COMPAT);
	cvar_t *scratch2_compat = new cvar_t ("scratch2", "0", CVAR_COMPAT);
	cvar_t *scratch3_compat = new cvar_t ("scratch3", "0", CVAR_COMPAT);
	cvar_t *scratch4_compat = new cvar_t ("scratch4", "0", CVAR_COMPAT);
	cvar_t *savedgamecfg_compat = new cvar_t ("savedgamecfg", "0", CVAR_COMPAT);
	cvar_t *saved1_compat = new cvar_t ("saved1", "0", CVAR_COMPAT);
	cvar_t *saved2_compat = new cvar_t ("saved2", "0", CVAR_COMPAT);
	cvar_t *saved3_compat = new cvar_t ("saved3", "0", CVAR_COMPAT);
	cvar_t *saved4_compat = new cvar_t ("saved4", "0", CVAR_COMPAT);
	cvar_t *r_timegraph_compat = new cvar_t ("r_timegraph", "0", CVAR_COMPAT);
	cvar_t *r_graphheight_compat = new cvar_t ("r_graphheight", "0", CVAR_COMPAT);
	cvar_t *r_clearcolor_compat = new cvar_t ("r_clearcolor", "0", CVAR_COMPAT);
	cvar_t *r_aliasstats_compat = new cvar_t ("r_polymodelstats", "0", CVAR_COMPAT);
	cvar_t *r_dspeeds_compat = new cvar_t ("r_dspeeds", "0", CVAR_COMPAT);
	cvar_t *r_drawflat_compat = new cvar_t ("r_drawflat", "0", CVAR_COMPAT);
	cvar_t *r_ambient_compat = new cvar_t ("r_ambient", "0", CVAR_COMPAT);
	cvar_t *r_reportsurfout_compat = new cvar_t ("r_reportsurfout", "0", CVAR_COMPAT);
	cvar_t *r_maxsurfs_compat = new cvar_t ("r_maxsurfs", "0", CVAR_COMPAT);
	cvar_t *r_numsurfs_compat = new cvar_t ("r_numsurfs", "0", CVAR_COMPAT);
	cvar_t *r_reportedgeout_compat = new cvar_t ("r_reportedgeout", "0", CVAR_COMPAT);
	cvar_t *r_maxedges_compat = new cvar_t ("r_maxedges", "0", CVAR_COMPAT);
	cvar_t *r_numedges_compat = new cvar_t ("r_numedges", "0", CVAR_COMPAT);
	cvar_t *r_aliastransbase_compat = new cvar_t ("r_aliastransbase", "0", CVAR_COMPAT);
	cvar_t *r_aliastransadj_compat = new cvar_t ("r_aliastransadj", "0", CVAR_COMPAT);
	cvar_t *bgmvolume_compat = new cvar_t ("bgmvolume", "0", CVAR_COMPAT);
	cvar_t *volume_compat = new cvar_t ("volume", "0", CVAR_COMPAT);
	cvar_t *nosound_compat = new cvar_t ("nosound", "0", CVAR_COMPAT);
	cvar_t *precache_compat = new cvar_t ("precache", "0", CVAR_COMPAT);
	cvar_t *bgmbuffer_compat = new cvar_t ("bgmbuffer", "0", CVAR_COMPAT);
	cvar_t *ambient_level_compat = new cvar_t ("ambient_level", "0", CVAR_COMPAT);
	cvar_t *ambient_fade_compat = new cvar_t ("ambient_fade", "0", CVAR_COMPAT);
	cvar_t *snd_noextraupdate_compat = new cvar_t ("snd_noextraupdate", "0", CVAR_COMPAT);
	cvar_t *snd_show_compat = new cvar_t ("snd_show", "0", CVAR_COMPAT);
	cvar_t *_snd_mixahead_compat = new cvar_t ("_snd_mixahead", "0", CVAR_COMPAT);
	cvar_t *sv_friction_compat = new cvar_t ("sv_friction", "0", CVAR_COMPAT);
	cvar_t *sv_stopspeed_compat = new cvar_t ("sv_stopspeed", "0", CVAR_COMPAT);
	cvar_t *sv_gravity_compat = new cvar_t ("sv_gravity", "0", CVAR_COMPAT);
	cvar_t *sv_maxvelocity_compat = new cvar_t ("sv_maxvelocity", "0", CVAR_COMPAT);
	cvar_t *sv_nostep_compat = new cvar_t ("sv_nostep", "0", CVAR_COMPAT);
	cvar_t *sv_idealpitchscale_compat = new cvar_t ("sv_idealpitchscale", "0", CVAR_COMPAT);
	cvar_t *sv_maxspeed_compat = new cvar_t ("sv_maxspeed", "0", CVAR_COMPAT);
	cvar_t *sv_accelerate_compat = new cvar_t ("sv_accelerate", "0", CVAR_COMPAT);
	cvar_t *vid_fullscreen_mode_compat = new cvar_t ("vid_fullscreen_mode", "0", CVAR_COMPAT);
	cvar_t *vid_windowed_mode_compat = new cvar_t ("vid_windowed_mode", "0", CVAR_COMPAT);
	cvar_t *block_switch_compat = new cvar_t ("block_switch", "0", CVAR_COMPAT);
	cvar_t *vid_window_x_compat = new cvar_t ("vid_window_x", "0", CVAR_COMPAT);
	cvar_t *vid_window_y_compat = new cvar_t ("vid_window_y", "0", CVAR_COMPAT);
	cvar_t *lcd_x_compat = new cvar_t ("lcd_x", "0", CVAR_COMPAT);
	cvar_t *lcd_yaw_compat = new cvar_t ("lcd_yaw", "0", CVAR_COMPAT);
	cvar_t *scr_ofsx_compat = new cvar_t ("scr_ofsx", "0", CVAR_COMPAT);
	cvar_t *scr_ofsy_compat = new cvar_t ("scr_ofsy", "0", CVAR_COMPAT);
	cvar_t *scr_ofsz_compat = new cvar_t ("scr_ofsz", "0", CVAR_COMPAT);
	cvar_t *cl_rollspeed_compat = new cvar_t ("cl_rollspeed", "0", CVAR_COMPAT);
	cvar_t *cl_rollangle_compat = new cvar_t ("cl_rollangle", "0", CVAR_COMPAT);
	cvar_t *cl_bob_compat = new cvar_t ("cl_bob", "0", CVAR_COMPAT);
	cvar_t *cl_bobcycle_compat = new cvar_t ("cl_bobcycle", "0", CVAR_COMPAT);
	cvar_t *cl_bobup_compat = new cvar_t ("cl_bobup", "0", CVAR_COMPAT);
	cvar_t *v_kicktime_compat = new cvar_t ("v_kicktime", "0", CVAR_COMPAT);
	cvar_t *v_kickroll_compat = new cvar_t ("v_kickroll", "0", CVAR_COMPAT);
	cvar_t *v_kickpitch_compat = new cvar_t ("v_kickpitch", "0", CVAR_COMPAT);
	cvar_t *v_iyaw_cycle_compat = new cvar_t ("v_iyaw_cycle", "0", CVAR_COMPAT);
	cvar_t *v_iroll_cycle_compat = new cvar_t ("v_iroll_cycle", "0", CVAR_COMPAT);
	cvar_t *v_ipitch_cycle_compat = new cvar_t ("v_ipitch_cycle", "0", CVAR_COMPAT);
	cvar_t *v_iyaw_level_compat = new cvar_t ("v_iyaw_level", "0", CVAR_COMPAT);
	cvar_t *v_iroll_level_compat = new cvar_t ("v_iroll_level", "0", CVAR_COMPAT);
	cvar_t *v_ipitch_level_compat = new cvar_t ("v_ipitch_level", "0", CVAR_COMPAT);
	cvar_t *v_idlescale_compat = new cvar_t ("v_idlescale", "0", CVAR_COMPAT);
	cvar_t *crosshair_compat = new cvar_t ("crosshair", "0", CVAR_COMPAT);
	cvar_t *cl_crossx_compat = new cvar_t ("cl_crossx", "0", CVAR_COMPAT);
	cvar_t *cl_crossy_compat = new cvar_t ("cl_crossy", "0", CVAR_COMPAT);
	cvar_t *gl_cshiftpercent_compat = new cvar_t ("gl_cshiftpercent", "0", CVAR_COMPAT);
	cvar_t *v_centermove_compat = new cvar_t ("v_centermove", "0", CVAR_COMPAT);
	cvar_t *v_centerspeed_compat = new cvar_t ("v_centerspeed", "0", CVAR_COMPAT);
	cvar_t *v_gamma_compat = new cvar_t ("gamma", "0", CVAR_COMPAT);
	cvar_t *vid_stretch_by_2_compat = new cvar_t ("vid_stretch_by_2", "0", CVAR_COMPAT);
}

// compatiblity commands
// if an incoming registered command has the same name we replace the function
// if one of these has the same name as a previous command we ignore it.
void Cmd_MakeCompatLayer (void)
{
	cmd_t *compatcmd_1 = new cmd_t ("cd", NULL);
	cmd_t *compatcmd_2 = new cmd_t ("+moveup", NULL);
	cmd_t *compatcmd_3 = new cmd_t ("-moveup", NULL);
	cmd_t *compatcmd_4 = new cmd_t ("+movedown", NULL);
	cmd_t *compatcmd_5 = new cmd_t ("-movedown", NULL);
	cmd_t *compatcmd_6 = new cmd_t ("+left", NULL);
	cmd_t *compatcmd_7 = new cmd_t ("-left", NULL);
	cmd_t *compatcmd_8 = new cmd_t ("+right", NULL);
	cmd_t *compatcmd_9 = new cmd_t ("-right", NULL);
	cmd_t *compatcmd_10 = new cmd_t ("+forward", NULL);
	cmd_t *compatcmd_11 = new cmd_t ("-forward", NULL);
	cmd_t *compatcmd_12 = new cmd_t ("+back", NULL);
	cmd_t *compatcmd_13 = new cmd_t ("-back", NULL);
	cmd_t *compatcmd_14 = new cmd_t ("+lookup", NULL);
	cmd_t *compatcmd_15 = new cmd_t ("-lookup", NULL);
	cmd_t *compatcmd_16 = new cmd_t ("+lookdown", NULL);
	cmd_t *compatcmd_17 = new cmd_t ("-lookdown", NULL);
	cmd_t *compatcmd_18 = new cmd_t ("+strafe", NULL);
	cmd_t *compatcmd_19 = new cmd_t ("-strafe", NULL);
	cmd_t *compatcmd_20 = new cmd_t ("+moveleft", NULL);
	cmd_t *compatcmd_21 = new cmd_t ("-moveleft", NULL);
	cmd_t *compatcmd_22 = new cmd_t ("+moveright", NULL);
	cmd_t *compatcmd_23 = new cmd_t ("-moveright", NULL);
	cmd_t *compatcmd_24 = new cmd_t ("+speed", NULL);
	cmd_t *compatcmd_25 = new cmd_t ("-speed", NULL);
	cmd_t *compatcmd_26 = new cmd_t ("+attack", NULL);
	cmd_t *compatcmd_27 = new cmd_t ("-attack", NULL);
	cmd_t *compatcmd_28 = new cmd_t ("+use", NULL);
	cmd_t *compatcmd_29 = new cmd_t ("-use", NULL);
	cmd_t *compatcmd_30 = new cmd_t ("+jump", NULL);
	cmd_t *compatcmd_31 = new cmd_t ("-jump", NULL);
	cmd_t *compatcmd_32 = new cmd_t ("impulse", NULL);
	cmd_t *compatcmd_33 = new cmd_t ("+klook", NULL);
	cmd_t *compatcmd_34 = new cmd_t ("-klook", NULL);
	cmd_t *compatcmd_35 = new cmd_t ("+mlook", NULL);
	cmd_t *compatcmd_36 = new cmd_t ("-mlook", NULL);
	cmd_t *compatcmd_37 = new cmd_t ("entities", NULL);
	cmd_t *compatcmd_38 = new cmd_t ("disconnect", NULL);
	cmd_t *compatcmd_39 = new cmd_t ("record", NULL);
	cmd_t *compatcmd_40 = new cmd_t ("stop", NULL);
	cmd_t *compatcmd_41 = new cmd_t ("playdemo", NULL);
	cmd_t *compatcmd_42 = new cmd_t ("timedemo", NULL);
	cmd_t *compatcmd_43 = new cmd_t ("stuffcmds", NULL);
	cmd_t *compatcmd_44 = new cmd_t ("exec", NULL);
	cmd_t *compatcmd_45 = new cmd_t ("echo", NULL);
	cmd_t *compatcmd_46 = new cmd_t ("alias", NULL);
	cmd_t *compatcmd_47 = new cmd_t ("cmd", NULL);
	cmd_t *compatcmd_48 = new cmd_t ("wait", NULL);
	cmd_t *compatcmd_49 = new cmd_t ("path", NULL);
	cmd_t *compatcmd_50 = new cmd_t ("toggleconsole", NULL);
	cmd_t *compatcmd_51 = new cmd_t ("messagemode", NULL);
	cmd_t *compatcmd_52 = new cmd_t ("messagemode2", NULL);
	cmd_t *compatcmd_53 = new cmd_t ("clear", NULL);
	cmd_t *compatcmd_55 = new cmd_t ("timerefresh", NULL);
	cmd_t *compatcmd_56 = new cmd_t ("envmap", NULL);
	cmd_t *compatcmd_57 = new cmd_t ("pointfile", NULL);
	cmd_t *compatcmd_58 = new cmd_t ("screenshot", NULL);
	cmd_t *compatcmd_59 = new cmd_t ("sizeup", NULL);
	cmd_t *compatcmd_60 = new cmd_t ("sizedown", NULL);
	cmd_t *compatcmd_61 = new cmd_t ("vid_nummodes", NULL);
	cmd_t *compatcmd_62 = new cmd_t ("vid_describecurrentmode", NULL);
	cmd_t *compatcmd_63 = new cmd_t ("vid_describemode", NULL);
	cmd_t *compatcmd_64 = new cmd_t ("vid_describemodes", NULL);
	cmd_t *compatcmd_65 = new cmd_t ("status", NULL);
	cmd_t *compatcmd_66 = new cmd_t ("quit", NULL);
	cmd_t *compatcmd_67 = new cmd_t ("god", NULL);
	cmd_t *compatcmd_68 = new cmd_t ("notarget", NULL);
	cmd_t *compatcmd_69 = new cmd_t ("fly", NULL);
	cmd_t *compatcmd_70 = new cmd_t ("map", NULL);
	// cmd_t *compatcmd_73 = new cmd_t ("changelevel2", NULL);
	cmd_t *compatcmd_76 = new cmd_t ("name", NULL);
	cmd_t *compatcmd_77 = new cmd_t ("noclip", NULL);
	cmd_t *compatcmd_78 = new cmd_t ("version", NULL);
	cmd_t *compatcmd_79 = new cmd_t ("please", NULL);
	cmd_t *compatcmd_80 = new cmd_t ("say", NULL);
	cmd_t *compatcmd_81 = new cmd_t ("say_team", NULL);
	cmd_t *compatcmd_82 = new cmd_t ("tell", NULL);
	cmd_t *compatcmd_83 = new cmd_t ("color", NULL);
	cmd_t *compatcmd_84 = new cmd_t ("kill", NULL);
	cmd_t *compatcmd_85 = new cmd_t ("pause", NULL);
	cmd_t *compatcmd_86 = new cmd_t ("spawn", NULL);
	cmd_t *compatcmd_87 = new cmd_t ("begin", NULL);
	cmd_t *compatcmd_88 = new cmd_t ("prespawn", NULL);
	cmd_t *compatcmd_89 = new cmd_t ("kick", NULL);
	cmd_t *compatcmd_90 = new cmd_t ("ping", NULL);
	cmd_t *compatcmd_91 = new cmd_t ("load", NULL);
	cmd_t *compatcmd_92 = new cmd_t ("save", NULL);
	cmd_t *compatcmd_93 = new cmd_t ("give", NULL);
	cmd_t *compatcmd_94 = new cmd_t ("startdemos", NULL);
	cmd_t *compatcmd_95 = new cmd_t ("demos", NULL);
	cmd_t *compatcmd_96 = new cmd_t ("stopdemo", NULL);
	cmd_t *compatcmd_97 = new cmd_t ("viewmodel", NULL);
	cmd_t *compatcmd_98 = new cmd_t ("viewframe", NULL);
	cmd_t *compatcmd_99 = new cmd_t ("viewnext", NULL);
	cmd_t *compatcmd_100 = new cmd_t ("viewprev", NULL);
	cmd_t *compatcmd_101 = new cmd_t ("mcache", NULL);
	cmd_t *compatcmd_102 = new cmd_t ("force_centerview", NULL);
	cmd_t *compatcmd_103 = new cmd_t ("joyadvancedupdate", NULL);
	cmd_t *compatcmd_104 = new cmd_t ("bind", NULL);
	cmd_t *compatcmd_105 = new cmd_t ("unbind", NULL);
	cmd_t *compatcmd_106 = new cmd_t ("unbindall", NULL);
	cmd_t *compatcmd_107 = new cmd_t ("togglemenu", NULL);
	cmd_t *compatcmd_108 = new cmd_t ("menu_main", NULL);
	cmd_t *compatcmd_109 = new cmd_t ("menu_singleplayer", NULL);
	cmd_t *compatcmd_110 = new cmd_t ("menu_load", NULL);
	cmd_t *compatcmd_111 = new cmd_t ("menu_save", NULL);
	cmd_t *compatcmd_112 = new cmd_t ("menu_multiplayer", NULL);
	cmd_t *compatcmd_113 = new cmd_t ("menu_setup", NULL);
	cmd_t *compatcmd_114 = new cmd_t ("menu_options", NULL);
	cmd_t *compatcmd_115 = new cmd_t ("menu_keys", NULL);
	cmd_t *compatcmd_116 = new cmd_t ("menu_video", NULL);
	cmd_t *compatcmd_117 = new cmd_t ("help", NULL);
	cmd_t *compatcmd_118 = new cmd_t ("menu_quit", NULL);
	cmd_t *compatcmd_119 = new cmd_t ("net_stats", NULL);
	cmd_t *compatcmd_120 = new cmd_t ("ban", NULL);
	cmd_t *compatcmd_121 = new cmd_t ("test", NULL);
	cmd_t *compatcmd_122 = new cmd_t ("test2", NULL);
	cmd_t *compatcmd_123 = new cmd_t ("slist", NULL);
	cmd_t *compatcmd_124 = new cmd_t ("listen", NULL);
	cmd_t *compatcmd_125 = new cmd_t ("maxplayers", NULL);
	cmd_t *compatcmd_126 = new cmd_t ("port", NULL);
	cmd_t *compatcmd_127 = new cmd_t ("edict", NULL);
	cmd_t *compatcmd_128 = new cmd_t ("edicts", NULL);
	cmd_t *compatcmd_129 = new cmd_t ("edictcount", NULL);
	cmd_t *compatcmd_130 = new cmd_t ("profile", NULL);
	cmd_t *compatcmd_131 = new cmd_t ("timerefresh", NULL);
	cmd_t *compatcmd_132 = new cmd_t ("pointfile", NULL);
	cmd_t *compatcmd_133 = new cmd_t ("+showscores", NULL);
	cmd_t *compatcmd_134 = new cmd_t ("-showscores", NULL);
	cmd_t *compatcmd_135 = new cmd_t ("screenshot", NULL);
	cmd_t *compatcmd_136 = new cmd_t ("sizeup", NULL);
	cmd_t *compatcmd_137 = new cmd_t ("sizedown", NULL);
	cmd_t *compatcmd_138 = new cmd_t ("play", NULL);
	cmd_t *compatcmd_139 = new cmd_t ("playvol", NULL);
	cmd_t *compatcmd_140 = new cmd_t ("stopsound", NULL);
	cmd_t *compatcmd_141 = new cmd_t ("soundlist", NULL);
	cmd_t *compatcmd_142 = new cmd_t ("soundinfo", NULL);
	cmd_t *compatcmd_143 = new cmd_t ("vid_testmode", NULL);
	cmd_t *compatcmd_144 = new cmd_t ("vid_nummodes", NULL);
	cmd_t *compatcmd_145 = new cmd_t ("vid_describecurrentmode", NULL);
	cmd_t *compatcmd_146 = new cmd_t ("vid_describemode", NULL);
	cmd_t *compatcmd_147 = new cmd_t ("vid_describemodes", NULL);
	cmd_t *compatcmd_148 = new cmd_t ("vid_forcemode", NULL);
	cmd_t *compatcmd_149 = new cmd_t ("vid_windowed", NULL);
	cmd_t *compatcmd_150 = new cmd_t ("vid_fullscreen", NULL);
	cmd_t *compatcmd_151 = new cmd_t ("vid_minimize", NULL);
	cmd_t *compatcmd_152 = new cmd_t ("v_cshift", NULL);
	cmd_t *compatcmd_153 = new cmd_t ("bf", NULL);
	cmd_t *compatcmd_154 = new cmd_t ("centerview", NULL);
	cmd_t *compatcmd_155 = new cmd_t ("hunk_report", NULL);
}

