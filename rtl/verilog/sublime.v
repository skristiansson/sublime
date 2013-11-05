/*
 * Sublime - Subtractive synthesizer
 *
 * Copyright (c) 2013, Stefan Kristiansson <stefan.kristiansson@saunalahti.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and non-source forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in non-source form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS WORK IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * WORK, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

module sublime #(
	parameter NUM_VOICES = 8,
	parameter WAVETABLE_SIZE = 8192,	// Should be a power of 2
	parameter WB_AW = 32,
	parameter WB_DW = 32
)(
	input 		    clk,
	input 		    rst,

	// Stereo output streams
	output [31:0] 	    left_sample,
	output [31:0] 	    right_sample,

	// Wishbone slave interface
	input [WB_AW-1:0]   wb_adr_i,
	input [WB_DW-1:0]   wb_dat_i,
	input [WB_DW/8-1:0] wb_sel_i,
	input 		    wb_we_i,
	input 		    wb_cyc_i,
	input 		    wb_stb_i,
	input [2:0] 	    wb_cti_i,
	input [1:0] 	    wb_bte_i,
	output [WB_DW-1:0]  wb_dat_o,
	output 		    wb_ack_o,
	output 		    wb_err_o,
	output 		    wb_rty_o
);
wire [$clog2(NUM_VOICES)-1:0]		active_voice;
wire					active_voice_changed;

wire [NUM_VOICES-1:0]			nco0_enable;
wire [NUM_VOICES-1:0]			nco0_sync;
wire [NUM_VOICES*8-1:0]			nco0_offset;
wire [NUM_VOICES*32-1:0]		nco0_freq;

wire [NUM_VOICES-1:0]			nco1_enable;
wire [NUM_VOICES-1:0]			nco1_sync;
wire [NUM_VOICES*8-1:0]			nco1_offset;
wire [NUM_VOICES*32-1:0]		nco1_freq;

wire [NUM_VOICES*3-1:0]			nco_mixmode;

wire 					wavetable0_we;
wire 				 	wavetable1_we;
wire [$clog2(WAVETABLE_SIZE)-1:0]	wavetable_write_addr;
wire [31:0] 				wavetable_write_data;

wire [NUM_VOICES*8-1:0]			velocity;
wire [7:0] 				voice_velocity[NUM_VOICES-1:0];
wire [31:0] 				active_voice_data;
wire [31:0] 				mixed_data;

genvar i;

assign left_sample = mixed_data;
assign right_sample = mixed_data;

generate
for (i = 0; i < NUM_VOICES; i = i+1) begin : velocity_gen
	assign voice_velocity[i] = velocity[8*(i+1)-1:8*i];
end
endgenerate

// Signal that indicates that all modules are done processing the
// the current active voice.
wire active_voice_done = 1'b1;

sublime_voice_ctrl #(
	.NUM_VOICES			(NUM_VOICES),
	.WAVETABLE_SIZE			(WAVETABLE_SIZE)
) voice_ctrl0 (
	.clk				(clk),
	.rst				(rst),

	// Outputs
	.active_voice			(active_voice),
	.active_voice_changed		(active_voice_changed),
	.active_voice_data		(active_voice_data),
	// Inputs
	.active_voice_done		(active_voice_done),
	.nco0_enable			(nco0_enable),
	.nco0_sync			(nco0_sync),
	.nco0_freq			(nco0_freq),
	.nco0_offset			(nco0_offset),
	.nco1_enable			(nco1_enable),
	.nco1_sync			(nco1_sync),
	.nco1_freq			(nco1_freq),
	.nco1_offset			(nco1_offset),
	.nco_mixmode			(nco_mixmode),
	.wavetable0_we			(wavetable0_we),
	.wavetable0_write_addr		(wavetable_write_addr),
	.wavetable0_write_data		(wavetable_write_data),
	.wavetable1_we			(wavetable1_we),
	.wavetable1_write_addr		(wavetable_write_addr),
	.wavetable1_write_data		(wavetable_write_data)
);

sublime_voice_mixer #(
	.NUM_VOICES			(NUM_VOICES)
) voice_mixer0 (
	// Outputs
	.mixed_data			(mixed_data),
	// Inputs
	.clk				(clk),
	.rst				(rst),
	.active_voice			(active_voice),
	.active_voice_changed		(active_voice_changed),
	.active_voice_velocity		(voice_velocity[active_voice]),
	.active_voice_data		(active_voice_data)
);

sublime_wb_slave #(
	.NUM_VOICES			(NUM_VOICES),
	.WAVETABLE_SIZE			(WAVETABLE_SIZE)
) wb_slave0 (
	.clk				(clk),
	.rst				(rst),

	.nco0_enable			(nco0_enable),
	.nco0_sync			(nco0_sync),
	.nco0_offset			(nco0_offset),
	.nco0_freq			(nco0_freq),
	.nco1_enable			(nco1_enable),
	.nco1_sync			(nco1_sync),
	.nco1_offset			(nco1_offset),
	.nco1_freq			(nco1_freq),
	.wavetable0_we			(wavetable0_we),
	.wavetable1_we			(wavetable1_we),
	.wavetable_write_addr		(wavetable_write_addr),
	.wavetable_write_data		(wavetable_write_data),
	.velocity			(velocity),
	.nco_mixmode			(nco_mixmode),

	.left_sample			(left_sample),
	.right_sample			(right_sample),

	.wb_adr_i			(wb_adr_i),
	.wb_dat_i			(wb_dat_i),
	.wb_sel_i			(wb_sel_i),
	.wb_we_i			(wb_we_i),
	.wb_cyc_i			(wb_cyc_i),
	.wb_stb_i			(wb_stb_i),
	.wb_cti_i			(wb_cti_i),
	.wb_bte_i			(wb_bte_i),
	.wb_dat_o			(wb_dat_o),
	.wb_ack_o			(wb_ack_o),
	.wb_err_o			(wb_err_o),
	.wb_rty_o			(wb_rty_o)
);

endmodule