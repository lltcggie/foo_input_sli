#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include <stdio.h>
#include <io.h>

#include "foo_prefs.h"
extern "C" {
#include "../src/vgmstream.h"
#include "../src/util.h"
}
#include "foo_vgmstream.h"


static cfg_bool     cfg_LoopForever     ({0xa19e36eb,0x72a0,0x4077,{0x91,0x43,0x38,0xb4,0x05,0xfc,0x91,0xc5}}, DEFAULT_LOOP_FOREVER);
static cfg_bool     cfg_IgnoreLoop      ({0xddda7ab6,0x7bb6,0x4abe,{0xb9,0x66,0x2d,0xb7,0x8f,0xe4,0xcc,0xab}}, DEFAULT_IGNORE_LOOP);
static cfg_string   cfg_LoopCount       ({0xfc8dfd72,0xfae8,0x44cc,{0xbe,0x99,0x1c,0x7b,0x27,0x7a,0xb6,0xb9}}, DEFAULT_LOOP_COUNT);
static cfg_string   cfg_FadeLength      ({0x61da7ef1,0x56a5,0x4368,{0xae,0x06,0xec,0x6f,0xd7,0xe6,0x15,0x5d}}, DEFAULT_FADE_SECONDS);
static cfg_string   cfg_FadeDelay       ({0x73907787,0xaf49,0x4659,{0x96,0x8e,0x9f,0x70,0xa1,0x62,0x49,0xc4}}, DEFAULT_FADE_DELAY_SECONDS);
static cfg_string   cfg_DownmixChannels ({0x5a0e65dd,0xeb37,0x4c67,{0x9a,0xb1,0x3f,0xb0,0xc9,0x7e,0xb0,0xe0}}, DEFAULT_DOWNMIX_CHANNELS);

// Needs to be here in rder to access the static config
void input_vgmstream::load_settings() {
	// no verification needed here, as it is done below
	sscanf(cfg_FadeLength.get_ptr(),"%lf",&fade_seconds);
	sscanf(cfg_LoopCount.get_ptr(),"%lf",&loop_count);
	sscanf(cfg_FadeDelay.get_ptr(),"%lf",&fade_delay_seconds);
	loop_forever = cfg_LoopForever;
	ignore_loop = cfg_IgnoreLoop;
    sscanf(cfg_DownmixChannels.get_ptr(),"%d",&downmix_channels);

    /* exact 0 was allowed before (AKA "intro only") but confuses people and may result in unplayable files */
    if (loop_count <= 0)
        loop_count = 1;
}

const char * vgmstream_prefs::get_name() {
	return input_vgmstream::g_get_name();
}

GUID vgmstream_prefs::get_guid() {
	return input_vgmstream::g_get_preferences_guid();
}

GUID vgmstream_prefs::get_parent_guid() {
	return guid_input;
}

BOOL vgmstreamPreferences::OnInitDialog(CWindow, LPARAM) {
	CheckDlgButton(IDC_LOOP_FOREVER, cfg_LoopForever?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(IDC_IGNORE_LOOP, cfg_IgnoreLoop?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(IDC_LOOP_NORMALLY, (!cfg_IgnoreLoop && !cfg_LoopForever)?BST_CHECKED:BST_UNCHECKED);

	uSetDlgItemText(m_hWnd, IDC_LOOP_COUNT, cfg_LoopCount);
	uSetDlgItemText(m_hWnd, IDC_FADE_SECONDS, cfg_FadeLength);
	uSetDlgItemText(m_hWnd, IDC_FADE_DELAY_SECONDS, cfg_FadeDelay);

    uSetDlgItemText(m_hWnd, IDC_DOWNMIX_CHANNELS, cfg_DownmixChannels);


	return TRUE;
}

t_uint32 vgmstreamPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void vgmstreamPreferences::reset() {
	CheckDlgButton(IDC_LOOP_FOREVER, DEFAULT_LOOP_FOREVER?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(IDC_IGNORE_LOOP, DEFAULT_IGNORE_LOOP?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(IDC_LOOP_NORMALLY, (!DEFAULT_IGNORE_LOOP && !DEFAULT_LOOP_FOREVER)?BST_CHECKED:BST_UNCHECKED);

	uSetDlgItemText(m_hWnd, IDC_LOOP_COUNT, DEFAULT_LOOP_COUNT);
	uSetDlgItemText(m_hWnd, IDC_FADE_SECONDS, DEFAULT_FADE_SECONDS);
	uSetDlgItemText(m_hWnd, IDC_FADE_DELAY_SECONDS, DEFAULT_FADE_DELAY_SECONDS);

    uSetDlgItemText(m_hWnd, IDC_DOWNMIX_CHANNELS, DEFAULT_DOWNMIX_CHANNELS);
}


void vgmstreamPreferences::apply() {
	cfg_LoopForever = IsDlgButtonChecked(IDC_LOOP_FOREVER)?true:false;
	cfg_IgnoreLoop = IsDlgButtonChecked(IDC_IGNORE_LOOP)?true:false;

	double temp_fade_seconds;
	double temp_fade_delay_seconds;
	double temp_loop_count;
	int consumed;
    int temp_downmix_channels;

	pfc::string buf;
	buf = uGetDlgItemText(m_hWnd, IDC_FADE_SECONDS);
	if (sscanf(buf.get_ptr(),"%lf%n",&temp_fade_seconds,&consumed)<1
		|| consumed!=strlen(buf.get_ptr()) ||
		temp_fade_seconds<0) {
		uMessageBox(m_hWnd,
				"Invalid value for Fade Length\n"
				"Must be a number greater than or equal to zero",
				"Error",MB_OK|MB_ICONERROR);
		return;
	} else cfg_FadeLength = buf.get_ptr();

	buf = uGetDlgItemText(m_hWnd, IDC_LOOP_COUNT);
	if (sscanf(buf.get_ptr(),"%lf%n",&temp_loop_count,&consumed)<1
		|| consumed!=strlen(buf.get_ptr()) ||
		temp_loop_count<0) {
		uMessageBox(m_hWnd,
				"Invalid value for Loop Count\n"
				"Must be a number greater than or equal to zero",
				"Error",MB_OK|MB_ICONERROR);
		return;
	} else cfg_LoopCount = buf.get_ptr();

	buf = uGetDlgItemText(m_hWnd, IDC_FADE_DELAY_SECONDS);
	if (sscanf(buf.get_ptr(),"%lf%n",&temp_fade_delay_seconds,&consumed)<1
		|| consumed!=strlen(buf.get_ptr()) ||
		temp_fade_delay_seconds<0) {
		uMessageBox(m_hWnd,
				"Invalid value for Fade Delay\n"
				"Must be a number",
				"Error",MB_OK|MB_ICONERROR);
		return;
	} else cfg_FadeDelay = buf.get_ptr();

    buf = uGetDlgItemText(m_hWnd, IDC_DOWNMIX_CHANNELS);
    if (sscanf(buf.get_ptr(),"%d%n",&temp_downmix_channels,&consumed)<1
        || consumed!=strlen(buf.get_ptr()) ||
        temp_downmix_channels<0) {
        uMessageBox(m_hWnd,
                "Invalid value for Downmix Channels\n"
                "Must be a number greater than or equal to zero",
                "Error",MB_OK|MB_ICONERROR);
        return;
    } else cfg_DownmixChannels = buf.get_ptr();

}


bool vgmstreamPreferences::HasChanged() {

	if(IsDlgButtonChecked(IDC_LOOP_FOREVER))
		if(cfg_LoopForever != true) return true;

	if(IsDlgButtonChecked(IDC_IGNORE_LOOP))
		if(cfg_IgnoreLoop != true) return true;

	if(IsDlgButtonChecked(IDC_LOOP_NORMALLY))
		if(cfg_IgnoreLoop != false || cfg_LoopForever != false) return true;

	pfc::string FadeLength(cfg_FadeLength);
	pfc::string FadeDelay(cfg_FadeDelay);
	pfc::string LoopCount(cfg_LoopCount);

	if(FadeLength != uGetDlgItemText(m_hWnd, IDC_FADE_SECONDS)) return true;
	if(FadeDelay != uGetDlgItemText(m_hWnd, IDC_FADE_DELAY_SECONDS)) return true;
	if(LoopCount != uGetDlgItemText(m_hWnd, IDC_LOOP_COUNT)) return true;

    pfc::string DownmixChannels(cfg_DownmixChannels);
    if(DownmixChannels != uGetDlgItemText(m_hWnd, IDC_DOWNMIX_CHANNELS)) return true;

	return FALSE;
}


void vgmstreamPreferences::OnEditChange(UINT, int, CWindow) {
	m_callback->on_state_changed();
}


static preferences_page_factory_t<vgmstream_prefs> g_vgmstream_preferences_page_factory;
