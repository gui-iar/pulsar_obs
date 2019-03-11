#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: B210 2
# Generated: Mon Mar 11 13:33:37 2019
##################################################


from datetime import datetime
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


class B210_2(gr.top_block):

    def __init__(self, bins=4096, display_integration=2, freq=150e6, gain=20, minutes=10, samp_rate=50e6):
        gr.top_block.__init__(self, "B210 2")

        ##################################################
        # Parameters
        ##################################################
        self.bins = bins
        self.display_integration = display_integration
        self.freq = freq
        self.gain = gain
        self.minutes = minutes
        self.samp_rate = samp_rate

        ##################################################
        # Variables
        ##################################################
        self.timenow = timenow = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.prefix = prefix = "/home/ggancio/obs/gnur/"
        self.raw = raw = prefix + timenow + "_" + str(bins) + "_" + str(samp_rate/1e6)  + "_" + str(freq/1e6) + "_raw.dat"

        ##################################################
        # Blocks
        ##################################################
        self.uhd_usrp_source_0 = uhd.usrp_source(
        	",".join(("", '')),
        	uhd.stream_args(
        		cpu_format="sc16",
        		otw_format='sc8',
        		channels=range(1),
        	),
        )
        self.uhd_usrp_source_0.set_clock_source('external', 0)
        self.uhd_usrp_source_0.set_samp_rate(samp_rate)
        self.uhd_usrp_source_0.set_time_now(uhd.time_spec(time.time()), uhd.ALL_MBOARDS)
        self.uhd_usrp_source_0.set_center_freq(freq, 0)
        self.uhd_usrp_source_0.set_gain(gain, 0)
        self.uhd_usrp_source_0.set_bandwidth(samp_rate, 0)
        self.fft_vxx_0 = fft.fft_vcc(bins, True, (window.blackmanharris(bins)), True, 1)
        self.blocks_stream_to_vector_0 = blocks.stream_to_vector(gr.sizeof_gr_complex*1, bins)
        self.blocks_nlog10_ff_0_0 = blocks.nlog10_ff(10, bins, 0)
        self.blocks_multiply_conjugate_cc_0 = blocks.multiply_conjugate_cc(bins)
        self.blocks_interleaved_short_to_complex_0 = blocks.interleaved_short_to_complex(True, False)
        self.blocks_integrate_xx_0 = blocks.integrate_ff(int(display_integration*samp_rate/bins), bins)
        self.blocks_head_0 = blocks.head(gr.sizeof_gr_complex*1, int(minutes*60*(samp_rate)))
        self.blocks_file_sink_0 = blocks.file_sink(gr.sizeof_float*bins, raw, False)
        self.blocks_file_sink_0.set_unbuffered(True)
        self.blocks_complex_to_mag_squared_0 = blocks.complex_to_mag_squared(bins)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.blocks_complex_to_mag_squared_0, 0), (self.blocks_integrate_xx_0, 0))
        self.connect((self.blocks_head_0, 0), (self.blocks_stream_to_vector_0, 0))
        self.connect((self.blocks_integrate_xx_0, 0), (self.blocks_nlog10_ff_0_0, 0))
        self.connect((self.blocks_interleaved_short_to_complex_0, 0), (self.blocks_head_0, 0))
        self.connect((self.blocks_multiply_conjugate_cc_0, 0), (self.blocks_complex_to_mag_squared_0, 0))
        self.connect((self.blocks_nlog10_ff_0_0, 0), (self.blocks_file_sink_0, 0))
        self.connect((self.blocks_stream_to_vector_0, 0), (self.fft_vxx_0, 0))
        self.connect((self.fft_vxx_0, 0), (self.blocks_multiply_conjugate_cc_0, 0))
        self.connect((self.fft_vxx_0, 0), (self.blocks_multiply_conjugate_cc_0, 1))
        self.connect((self.uhd_usrp_source_0, 0), (self.blocks_interleaved_short_to_complex_0, 0))

    def get_bins(self):
        return self.bins

    def set_bins(self, bins):
        self.bins = bins
        self.set_raw(self.prefix + self.timenow + "_" + str(self.bins) + "_" + str(self.samp_rate/1e6)  + "_" + str(self.freq/1e6) + "_raw.dat")

    def get_display_integration(self):
        return self.display_integration

    def set_display_integration(self, display_integration):
        self.display_integration = display_integration

    def get_freq(self):
        return self.freq

    def set_freq(self, freq):
        self.freq = freq
        self.set_raw(self.prefix + self.timenow + "_" + str(self.bins) + "_" + str(self.samp_rate/1e6)  + "_" + str(self.freq/1e6) + "_raw.dat")
        self.uhd_usrp_source_0.set_center_freq(self.freq, 0)

    def get_gain(self):
        return self.gain

    def set_gain(self, gain):
        self.gain = gain
        self.uhd_usrp_source_0.set_gain(self.gain, 0)


    def get_minutes(self):
        return self.minutes

    def set_minutes(self, minutes):
        self.minutes = minutes
        self.blocks_head_0.set_length(int(self.minutes*60*(self.samp_rate)))

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.set_raw(self.prefix + self.timenow + "_" + str(self.bins) + "_" + str(self.samp_rate/1e6)  + "_" + str(self.freq/1e6) + "_raw.dat")
        self.uhd_usrp_source_0.set_samp_rate(self.samp_rate)
        self.uhd_usrp_source_0.set_bandwidth(self.samp_rate, 0)
        self.blocks_head_0.set_length(int(self.minutes*60*(self.samp_rate)))

    def get_timenow(self):
        return self.timenow

    def set_timenow(self, timenow):
        self.timenow = timenow
        self.set_raw(self.prefix + self.timenow + "_" + str(self.bins) + "_" + str(self.samp_rate/1e6)  + "_" + str(self.freq/1e6) + "_raw.dat")

    def get_prefix(self):
        return self.prefix

    def set_prefix(self, prefix):
        self.prefix = prefix
        self.set_raw(self.prefix + self.timenow + "_" + str(self.bins) + "_" + str(self.samp_rate/1e6)  + "_" + str(self.freq/1e6) + "_raw.dat")

    def get_raw(self):
        return self.raw

    def set_raw(self, raw):
        self.raw = raw
        self.blocks_file_sink_0.open(self.raw)


def argument_parser():
    parser = OptionParser(usage="%prog: [options]", option_class=eng_option)
    parser.add_option(
        "", "--bins", dest="bins", type="intx", default=4096,
        help="Set bins [default=%default]")
    parser.add_option(
        "", "--display-integration", dest="display_integration", type="intx", default=2,
        help="Set int [default=%default]")
    parser.add_option(
        "", "--freq", dest="freq", type="eng_float", default=eng_notation.num_to_str(150e6),
        help="Set freq [default=%default]")
    parser.add_option(
        "", "--gain", dest="gain", type="intx", default=20,
        help="Set gain [default=%default]")
    parser.add_option(
        "", "--minutes", dest="minutes", type="intx", default=10,
        help="Set min [default=%default]")
    parser.add_option(
        "", "--samp-rate", dest="samp_rate", type="eng_float", default=eng_notation.num_to_str(50e6),
        help="Set rate [default=%default]")
    return parser


def main(top_block_cls=B210_2, options=None):
    if options is None:
        options, _ = argument_parser().parse_args()

    tb = top_block_cls(bins=options.bins, display_integration=options.display_integration, freq=options.freq, gain=options.gain, minutes=options.minutes, samp_rate=options.samp_rate)
    tb.start()
    tb.wait()


if __name__ == '__main__':
    main()
