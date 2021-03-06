/*
 * (c) 2016 Cedric PAILLE
 * Simple Spectrum Analyzer for RTL Dongle
 *
 */

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Box.H>
#include <stdio.h>
#include <math.h>
#include <graph_container.h>
#include <scanner.h>
#include <pthread.h>
#include <unistd.h>
#include <vector>
#include "scanner_thread.h"

class FrequencyCounter : public Fl_Counter
{
public:
	FrequencyCounter(int X, int Y, int W,int H,const char*L=0) : Fl_Counter(X, Y, W, H, L){
		step(10,100);
		m_focus = false;
		m_stepa = m_stepb = m_stepc = 0.;
	}

	void set_step(double a, double b, double c = 0.){
		step(a, b);
		m_stepa = a;
		m_stepb = b;
		m_stepc = c;
	}

protected:
	bool m_focus;
	float m_stepa, m_stepb, m_stepc;
	int handle(int event){
		int hret = Fl_Counter::handle(event);
		float old_value = value();
		float wheel_dy, new_val;

		switch(event){
		case FL_MOUSEWHEEL:
			if (m_focus){
				wheel_dy = Fl::event_dy() * -1. ;

				if ( Fl::event_shift() )
					wheel_dy *= m_stepb;
				else if( Fl::event_ctrl() )
					wheel_dy *= m_stepc;
				else
					wheel_dy *= m_stepa;

				new_val = old_value + wheel_dy;
				if (new_val > minimum() && new_val < maximum())
					value(new_val);
				do_callback();
				return 1;
			}
			break;
		case FL_ENTER:
			m_focus = true;
			hret = 1;
			break;
		case FL_LEAVE:
			m_focus = false;
			hret = 1;
			break;
		default:
			break;
		}

		return hret;
	}
};

class ParametersWidget : public Fl_Double_Window{
public:
	ParametersWidget(int X, int Y, int W,int H,Graph_container* graph, const char*L=0) : Fl_Double_Window(X, Y, W,H,L) , m_settings (get_scanner_settings()) {
		//m_frame = new Fl_Box(0, 0, W, H, "Hey");
		//m_frame->box(FL_EMBOSSED_FRAME);
		m_startfreq = new FrequencyCounter(10, 10, W - 20, 20, "Start Frequency (Mhz)");
		m_stopfreq = new FrequencyCounter(10, 50, W - 20, 20, "Stop Frequency (MHz)");
		m_span = new FrequencyCounter(10, 90, W - 20, 20, "Span (KHz)");
		m_ppm_slider = new Fl_Slider(10, 140, W - 20, 20, "PPM Correction");
		m_crop_slider = new Fl_Slider(10, 190, W - 20, 20, "Crop %");
		m_gain_slider = new Fl_Slider(10, 240, W - 20, 20, "Gain :");
		m_windows_choice = new Fl_Choice(10, 280, W - 20, 20);
		m_power_meter = new Fl_Progress(10, 310, W -20, 20, "Power meter");

		m_power_meter->type(FL_HORIZONTAL);
		m_power_meter->minimum(-80);
		m_power_meter->maximum(0.);

		m_windows_choice->add("RECTANGLE");
		m_windows_choice->add("HAMMING");
		m_windows_choice->add("BLACKMAN");
		m_windows_choice->add("BLACKMAN-HARRIS");
		m_windows_choice->add("HANN-POISSON");
		m_windows_choice->add("YOUSSEF");
		m_windows_choice->add("KAISER");
		m_windows_choice->add("BARTLETT");
		m_windows_choice->value(0);

		m_crop_slider->type(FL_HORIZONTAL);
		m_ppm_slider->type(FL_HORIZONTAL);
		m_crop_slider->range(0, 100);
		m_crop_slider->value(20);

		m_gain_slider->type(FL_HORIZONTAL);
		m_gain_slider->range(-1, 50);
		m_gain_slider->value(-1);

		m_startfreq->minimum(20.);
		m_startfreq->maximum(1800.);
		m_startfreq->value(88.);
		m_startfreq->set_step(1., 10., 100.);

		m_stopfreq->minimum(21.);
		m_stopfreq->maximum(1800.);
		m_stopfreq->set_step(1., 10., 100.);
		m_stopfreq->value(108.);

		m_span->minimum(0.001);
		m_span->maximum(900000.);
		m_span->value(2.5);
		m_span->set_step(0.01, 0.1, 1.);
		m_ppm_slider->range(-100, 100);
		m_ppm_slider->value(0.);

		m_startfreq->callback(value_callback, this);
		m_stopfreq->callback(value_callback, this);
		m_span->callback(value_callback, this);
		m_ppm_slider->callback(value_callback, this);
		m_crop_slider->callback(value_callback, this);
		m_gain_slider->callback(value_callback, this);
		end();
		m_graph = graph;
		refresh_graph_window();
	}

	void refresh_graph_window(){
		int percent = m_crop_slider->value();
		snprintf(m_crop_buff, 32, "Crop : %i\%%", (int)percent);

		float start_freq = m_startfreq->value();
		float stop_freq = m_stopfreq->value();
		float span = m_span->value();

		if (stop_freq <= start_freq){
			stop_freq = start_freq + 2;
			m_stopfreq->value(stop_freq);
		}

		if (span < 0.01){
			span = 0.01;
			m_span->value(span);
		}

		int gain = m_gain_slider->value();
		if (gain == -1){
			m_gain_slider->label("Automatic Gain Control");
			m_settings.gain = RTL_GAIN_AUTO;
		} else {
			snprintf(m_gain_buff, 32, "Gain : %i dB", gain);
			m_settings.gain = gain * 10;
			m_gain_slider->label(m_gain_buff);
		}

		m_crop_slider->label(m_crop_buff);
		m_settings.lower_freq = 1000000. * start_freq;
		m_settings.upper_freq = 1000000. * stop_freq;
		m_settings.step_freq  = 1000. * span;
		m_settings.ppm_correction = (int)m_ppm_slider->value();
		m_settings.crop		  = (float)percent / 100.;
		m_settings.window_type = (window_types)m_windows_choice->value();
		m_settings.params_changed = true;

		snprintf(m_ppm_buff, 32, "PPM Correction: %i", m_settings.ppm_correction);
		m_ppm_slider->label(m_ppm_buff);

		m_graph->set_window(m_settings.lower_freq, m_settings.upper_freq, -80, 10);
		// Ask FLTK to redraw panel
		damage(FL_DAMAGE_ALL);
	}
	void set_power(float x){
		m_power_meter->value(x);
		if (x > -20.)
			m_power_meter->selection_color(FL_GREEN);
		else if (x > -40)
			m_power_meter->selection_color(FL_YELLOW);
		else
			m_power_meter->selection_color(FL_RED);
	}
protected:

private:
	Graph_container  *m_graph;
	FrequencyCounter *m_stopfreq, *m_startfreq, *m_span;
	Fl_Slider 		 *m_crop_slider, *m_ppm_slider, *m_gain_slider;
	Fl_Choice	     *m_windows_choice;
	Fl_Progress		 *m_power_meter;
	Fl_Box			 *m_frame;
	Scanner_settings& m_settings;
	char 		      m_crop_buff[32];
	char 		      m_ppm_buff[32],m_gain_buff[32];

	static void value_callback(Fl_Widget* w, void *data){
		ParametersWidget* pw = (ParametersWidget*)data;
		FrequencyCounter* f = (FrequencyCounter*)w;
		pw->refresh_graph_window();
	}
};

class AppWindow : public Fl_Double_Window {
	Graph_container *m_graph_container;
	ParametersWidget *m_parameter_widgt;
public:
    AppWindow(int W,int H,const char*L=0) : Fl_Double_Window(W,H,L) {
    	m_graph_container = new Graph_container(5, 5, w()-200, h()-5);
        m_parameter_widgt = new ParametersWidget(w() - 210, 5, 200, h() - 5, m_graph_container);
        size_range(500,400);
        end();

        m_parameter_widgt->refresh_graph_window();
    }

    void reset_view(){
    	m_graph_container->reset();
    }

    void set_buffer(std::vector<float>* buff){
    	m_graph_container->set_buffer(buff);
    	m_parameter_widgt->set_power(m_graph_container->get_power_at_cursor());
    }

protected:
    void resize(int X,int Y,int W,int H){
    	m_graph_container->position(5, 5);
    	m_graph_container->size(W - 200,H - 5);
    	m_parameter_widgt->position(W - 200, 5);
    	m_parameter_widgt->size(W - m_graph_container->w(), H - 5);
    	redraw();
    }
};

int main() {
	Fl::lock();

	start_scanner_thread();

	AppWindow win(500, 300, "RTL Spectrum Analyzer");
    win.resizable(win);
    win.show();

    set_ui_ready();

	/* run thread */
	while (Fl::wait() > 0) {
		std::vector<float>* buffer = (std::vector<float>*)Fl::thread_message();
		if (buffer) {
			win.set_buffer(buffer);
		}
	}

	printf("Asking scanner thread to terminate...\n");
	terminate_scanner_thread();
	join_scanner_thread();
	printf("Scanner thread finished.\n");
}
    
