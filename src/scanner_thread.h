/*
 * (c) 2016 Cedric PAILLE
 * Simple Spectrum Analyzer for RTL Dongle
 *
 */

#ifndef SCANNER_THREAD_H
#define SCANNER_THREAD_H

#include "scanner.h"

struct Scanner_settings{
	Scanner_settings();
	int lower_freq, upper_freq, step_freq;
	double crop;
	int rtl_dev_index;
	bool direct_sampling;
	bool offset_tuning;
	int gain;
	int ppm_correction;
	window_types window_type;
	bool params_changed;
};

Scanner_settings& get_scanner_settings();
void 	set_ui_ready();
void* 	scanner_thread(void* user_data);
void 	start_scanner_thread();
void 	join_scanner_thread();
void 	terminate_scanner_thread();

void	ui_draw_complete(bool complete);

#endif
