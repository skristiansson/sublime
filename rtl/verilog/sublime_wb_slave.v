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

//
// Bus interface - Provides access to the synthesizers registers
// and wavetables.
//

module sublime_wb_slave #(
	parameter NUM_VOICES = 8,
	parameter WAVETABLE_SIZE = 8192,
	parameter WB_AW = 32,
	parameter WB_DW = 32
)(
	input 				    clk,
	input 				    rst,

	output [NUM_VOICES-1:0] 	    nco0_enable,
	output [NUM_VOICES-1:0] 	    nco0_sync,
	output [NUM_VOICES*8-1:0] 	    nco0_offset,
	output [NUM_VOICES*32-1:0] 	    nco0_freq,

	output [NUM_VOICES-1:0] 	    nco1_enable,
	output [NUM_VOICES-1:0] 	    nco1_sync,
	output [NUM_VOICES*8-1:0] 	    nco1_offset,
	output [NUM_VOICES*32-1:0] 	    nco1_freq,

	output [NUM_VOICES*3-1:0] 	    nco_mixmode,

	output 				    wavetable0_we,
	output 				    wavetable1_we,
	output [$clog2(WAVETABLE_SIZE)-1:0] wavetable_write_addr,
	output [31:0] 			    wavetable_write_data,

	output [NUM_VOICES-1:0] 	    envelope_gate,
	output [NUM_VOICES*8-1:0] 	    envelope_attack,
	output [NUM_VOICES*8-1:0] 	    envelope_decay,
	output [NUM_VOICES*8-1:0] 	    envelope_sustain,
	output [NUM_VOICES*8-1:0] 	    envelope_release,

	output [NUM_VOICES*8-1:0] 	    velocity,

	input [31:0] 			    left_sample,
	input [31:0] 			    right_sample,

	// Wishbone slave interface
	input [WB_AW-1:0] 		    wb_adr_i,
	input [WB_DW-1:0] 		    wb_dat_i,
	input [WB_DW/8-1:0] 		    wb_sel_i,
	input 				    wb_we_i,
	input 				    wb_cyc_i,
	input 				    wb_stb_i,
	input [2:0] 			    wb_cti_i,
	input [1:0] 			    wb_bte_i,
	output [WB_DW-1:0] 		    wb_dat_o,
	output reg 			    wb_ack_o,
	output 				    wb_err_o,
	output 				    wb_rty_o
);

//
// Register map
// +--------------+-------------------------+
// | 0x00000000   | voice0 osc0 frequency   |
// +--------------+-------------------------+
// | 0x00000004   | voice0 osc1 frequency   |
// +--------------+-------------------------+
// | 0x00000008   | voice0 control          |
// +--------------+-------------------------+
// | 0x0000000c   | voice0 envelope         |
// +--------------+-------------------------+
// | 0x00000010   | voice1 osc0 frequency   |
// +--------------+-------------------------+
// | 0x00000014   | voice1 osc1 frequency   |
// +--------------+-------------------------+
// | 0x00000018   | voice1 control          |
// +--------------+-------------------------+
// | 0x0000001c   | voice1 envelope         |
// +--------------+-------------------------+
// | ...          | ...                     |
// +--------------+-------------------------+
// | 0x000007f0   | voice127 osc0 frequency |
// +--------------+-------------------------+
// | 0x000007f4   | voice127 osc1 frequency |
// +--------------+-------------------------+
// | 0x000007f8   | voice127 control        |
// +--------------+-------------------------+
// | 0x000007fc   | voice127 envelope       |
// +--------------+-------------------------+
// | 0x00000800   | left audio sample       |
// +--------------+-------------------------+
// | 0x00000804   | right audio sample      |
// +--------------+-------------------------+
// | 0x00000808   | main control            |
// +--------------+-------------------------+
// | 0x0000080c - | reserved                |
// | 0x0000fffc   |                         |
// +--------------+-------------------------+
// | 0x00010000 - | wavetable0              |
// | 0x0001fffc   |                         |
// +--------------+-------------------------+
// | 0x00020000 - | wavetable1              |
// | 0x0002fffc   |                         |
// +--------------+-------------------------+
//
// NOTE 1: Even though register addresses for voices up to 127 are defined,
// only voice registers 0 - (NUM_VOICES-1) will actually be present.
//
// NOTE 2: The largest possible wavetable size is 16K entries, but when
// smaller wavetables implemented in the hardware (i.e. WAVETABLE_SIZE is
// smaller), there will be empty address space in each wavetable.
//
// Register descripions:
//
// voiceX control
// +-------------+-------------+---------------+-----------+-----------+
// |       31:24 |       23:16 |          15:8 |         7 |         6 |
// +-------------+-------------+---------------+-----------+-----------+
// | osc0 offset | osc1 offset |      velocity | osc0 sync | osc1 sync |
// +-------------+-------------+---------------+-----------+-----------+
// +-------------+---------------------------+-------------+-------------+
// |         5:3 |                         2 |           1 |           0 |
// +-------------+---------------------------+-------------+-------------+
// | osc mixmode | 1 = note on, 0 = note off | osc1 enable | osc0 enable |
// +-------------+---------------------------+-------------+-------------+
//
// osc0/osc1 sync - When those signals are asserted, the oscillator will
// restart. The most useful use case for this is to assert them at the
// same time to get them in sync with each other.
//
// voiceX envelope
// +--------+-------+---------+---------+
// |  31:24 | 23:16 |    15:8 |     7:0 |
// +--------+-------+---------+---------+
// | attack | decay | sustain | release |
// +--------+-------+---------+---------+
//
// Main control
// +----------+----------+
// |  31:1    | 0        |
// +----------+----------+
// | reserved | sync all |
// +----------+----------+

localparam OSC0_SYNC	= 7;
localparam OSC1_SYNC	= 6;
localparam NOTE_ON	= 2;
localparam OSC1_EN	= 1;
localparam OSC0_EN	= 0;

genvar i;

wire wb_write_req;

assign wb_write_req = wb_cyc_i & wb_stb_i & wb_we_i & !wb_ack_o;
assign wb_err_o = 0;
assign wb_rty_o = 0;

always @(posedge clk)
	if (rst)
		wb_ack_o <= 0;
	else
		wb_ack_o <= wb_cyc_i & wb_stb_i & !wb_ack_o;

// Voice registers
wire voice_ce = wb_adr_i[WB_AW-1:11] == 0;
wire [$clog2(NUM_VOICES)-1:0] voice_idx = wb_adr_i[10:4];

reg [31:0] voice_osc0_freq[NUM_VOICES-1:0];
reg [31:0] voice_osc1_freq[NUM_VOICES-1:0];
reg [31:0] voice_ctrl[NUM_VOICES-1:0];
reg [31:0] voice_adsr[NUM_VOICES-1:0];

always @(posedge clk) begin
	if (voice_ce & wb_write_req) begin
		case (wb_adr_i[3:2])
		2'h0:
			voice_osc0_freq[voice_idx] <= wb_dat_i;
		2'h1:
			voice_osc1_freq[voice_idx] <= wb_dat_i;
		2'h2:
			voice_ctrl[voice_idx] <= wb_dat_i;
		2'h3:
			voice_adsr[voice_idx] <= wb_dat_i;
		endcase
	end
end

// Wavetable access
wire wavetable0_ce = wb_adr_i[WB_AW-1:16] == 1;
wire wavetable1_ce = wb_adr_i[WB_AW-1:16] == 2;

assign wavetable0_we = wavetable0_ce & wb_write_req;
assign wavetable1_we = wavetable1_ce & wb_write_req;
assign wavetable_write_addr = wb_adr_i[$clog2(WAVETABLE_SIZE)+2-1:2];
assign wavetable_write_data = wb_dat_i;

// Read access to the synth output
wire left_ce = wb_adr_i[WB_AW-1:11] == 1 && wb_adr_i[10:2] == 0;
wire right_ce = wb_adr_i[WB_AW-1:11] == 1 && wb_adr_i[10:2] == 1;

assign wb_dat_o = left_ce ? left_sample :
		  right_ce ? right_sample :
		  0;

// Main control
reg [31:0] main_control;
wire main_control_ce = wb_adr_i[WB_AW-1:11] == 1 && wb_adr_i[10:2] == 2;
wire sync_all;

always @(posedge clk)
	if (rst)
		main_control <= 0;
	else if (main_control_ce & wb_write_req)
		main_control <= wb_dat_i;

assign sync_all = main_control[0];

// Flatten registers and map them to the out ports
// It will be a glorious day when multidimensional arrays in port
// declarations are supported...
generate
for (i = 0; i < NUM_VOICES; i = i + 1) begin : register_flattening
	assign nco0_enable[i] = voice_ctrl[i][OSC0_EN];
	assign nco0_sync[i] = voice_ctrl[i][OSC0_SYNC] | sync_all;
	assign nco0_offset[8*(i+1)-1:8*i] = voice_ctrl[i][31:24];
	assign nco0_freq[32*(i+1)-1:32*i] = voice_osc0_freq[i];

	assign nco1_enable[i] = voice_ctrl[i][OSC1_EN];
	assign nco1_sync[i] = voice_ctrl[i][OSC1_SYNC] | sync_all;
	assign nco1_offset[8*(i+1)-1:8*i] = voice_ctrl[i][23:16];
	assign nco1_freq[32*(i+1)-1:32*i] = voice_osc1_freq[i];

	assign nco_mixmode[3*(i+1)-1:3*i] = voice_ctrl[i][5:3];

	assign envelope_gate[i] = voice_ctrl[i][NOTE_ON];
	assign envelope_attack[8*(i+1)-1:8*i] = voice_adsr[i][31:24];
	assign envelope_decay[8*(i+1)-1:8*i] = voice_adsr[i][23:16];
	assign envelope_sustain[8*(i+1)-1:8*i] = voice_adsr[i][15:8];
	assign envelope_release[8*(i+1)-1:8*i] = voice_adsr[i][7:0];

	assign velocity[8*(i+1)-1:8*i] = voice_ctrl[i][15:8];
end
endgenerate

endmodule