#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: A1 Add Pol
# Generated: Tue Feb 12 13:52:19 2019
##################################################


from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import fft
from gnuradio import gr
from gnuradio import uhd
from gnuradio.eng_option import eng_option
from gnuradio.fft import window
from gnuradio.filter import firdes
from optparse import OptionParser
import numpy as np
import time


class A1_add_pol(gr.top_block):

    def __init__(self):
        gr.top_block.__init__(self, "A1 Add Pol")

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 50e6
        self.display_integration = display_integration = 1
        self.bins = bins = 4096

        ##################################################
        # Blocks
        ##################################################
        self.uhd_usrp_source_0_0 = uhd.usrp_source(
        	",".join(("", "serial=314F813")),
        	uhd.stream_args(
        		cpu_format="fc32",
        		channels=range(1),
        	),
        )
        self.uhd_usrp_source_0_0.set_clock_source('external', 0)
        self.uhd_usrp_source_0_0.set_samp_rate(samp_rate)
        self.uhd_usrp_source_0_0.set_time_now(uhd.time_spec(time.time()), uhd.ALL_MBOARDS)
        self.uhd_usrp_source_0_0.set_center_freq(155e6, 0)
        self.uhd_usrp_source_0_0.set_gain(20, 0)
        self.uhd_usrp_source_0_0.set_bandwidth(samp_rate, 0)
        self.uhd_usrp_source_0 = uhd.usrp_source(
        	",".join(("", '')),
        	uhd.stream_args(
        		cpu_format="fc32",
        		channels=range(1),
        	),
        )
        self.uhd_usrp_source_0.set_clock_source('external', 0)
        self.uhd_usrp_source_0.set_samp_rate(samp_rate)
        self.uhd_usrp_source_0.set_time_now(uhd.time_spec(time.time()), uhd.ALL_MBOARDS)
        self.uhd_usrp_source_0.set_center_freq(155e6, 0)
        self.uhd_usrp_source_0.set_gain(20, 0)
        self.uhd_usrp_source_0.set_bandwidth(samp_rate, 0)
        self.fft_vxx_0_1 = fft.fft_vcc(bins, True, (window.blackmanharris(bins)), True, 1)
        self.fft_vxx_0 = fft.fft_vcc(bins, True, (window.blackmanharris(bins)), True, 1)
        self.blocks_vector_to_stream_0 = blocks.vector_to_stream(gr.sizeof_float*1, bins)
        self.blocks_udp_sink_0 = blocks.udp_sink(gr.sizeof_float*1, '163.10.43.84', 7124, 1024, True)
        self.blocks_stream_to_vector_0_1 = blocks.stream_to_vector(gr.sizeof_gr_complex*1, bins)
        self.blocks_stream_to_vector_0 = blocks.stream_to_vector(gr.sizeof_gr_complex*1, bins)
        self.blocks_repeat_0_0_0_0 = blocks.repeat(gr.sizeof_float*bins, 10)
        self.blocks_repeat_0_0_0 = blocks.repeat(gr.sizeof_float*bins, 10)
        self.blocks_integrate_xx_0_1 = blocks.integrate_ff(int(display_integration*samp_rate/bins), bins)
        self.blocks_integrate_xx_0 = blocks.integrate_ff(int(display_integration*samp_rate/bins), bins)
        self.blocks_complex_to_mag_squared_0_1 = blocks.complex_to_mag_squared(bins)
        self.blocks_complex_to_mag_squared_0 = blocks.complex_to_mag_squared(bins)
        self.blocks_add_xx_0 = blocks.add_vff(bins)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.blocks_add_xx_0, 0), (self.blocks_vector_to_stream_0, 0))
        self.connect((self.blocks_complex_to_mag_squared_0, 0), (self.blocks_integrate_xx_0, 0))
        self.connect((self.blocks_complex_to_mag_squared_0_1, 0), (self.blocks_integrate_xx_0_1, 0))
        self.connect((self.blocks_integrate_xx_0, 0), (self.blocks_repeat_0_0_0, 0))
        self.connect((self.blocks_integrate_xx_0_1, 0), (self.blocks_repeat_0_0_0_0, 0))
        self.connect((self.blocks_repeat_0_0_0, 0), (self.blocks_add_xx_0, 0))
        self.connect((self.blocks_repeat_0_0_0_0, 0), (self.blocks_add_xx_0, 1))
        self.connect((self.blocks_stream_to_vector_0, 0), (self.fft_vxx_0, 0))
        self.connect((self.blocks_stream_to_vector_0_1, 0), (self.fft_vxx_0_1, 0))
        self.connect((self.blocks_vector_to_stream_0, 0), (self.blocks_udp_sink_0, 0))
        self.connect((self.fft_vxx_0, 0), (self.blocks_complex_to_mag_squared_0, 0))
        self.connect((self.fft_vxx_0_1, 0), (self.blocks_complex_to_mag_squared_0_1, 0))
        self.connect((self.uhd_usrp_source_0, 0), (self.blocks_stream_to_vector_0, 0))
        self.connect((self.uhd_usrp_source_0_0, 0), (self.blocks_stream_to_vector_0_1, 0))

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.uhd_usrp_source_0_0.set_samp_rate(self.samp_rate)
        self.uhd_usrp_source_0_0.set_bandwidth(self.samp_rate, 0)
        self.uhd_usrp_source_0.set_samp_rate(self.samp_rate)
        self.uhd_usrp_source_0.set_bandwidth(self.samp_rate, 0)

    def get_display_integration(self):
        return self.display_integration

    def set_display_integration(self, display_integration):
        self.display_integration = display_integration

    def get_bins(self):
        return self.bins

    def set_bins(self, bins):
        self.bins = bins


def main(top_block_cls=A1_add_pol, options=None):

    tb = top_block_cls()
    tb.start()
    tb.wait()


if __name__ == '__main__':
    main()
