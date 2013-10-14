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
// Voice control.
// Instantiates the NCOs for each voice and handle the wavetable sharing
// between voices.
// Outputs the current wavetable data and the voice it is associated with
// for both NCOs.
// Write ports into the wavetables are exposed to higher level for wave
// initialization.
//

module sublime_voice_ctrl #(
	parameter NUM_VOICES = 8,
	parameter WAVETABLE_SIZE = 8192
)(
	input 				    clk,
	input 				    rst,

	input [NUM_VOICES-1:0] 		    nco0_enable,
	input [NUM_VOICES-1:0] 		    nco0_sync,
	input [NUM_VOICES*32-1:0] 	    nco0_freq,
	input [NUM_VOICES*8-1:0] 	    nco0_offset,

	input [NUM_VOICES-1:0] 		    nco1_enable,
	input [NUM_VOICES-1:0] 		    nco1_sync,
	input [NUM_VOICES*32-1:0] 	    nco1_freq,
	input [NUM_VOICES*8-1:0] 	    nco1_offset,

	input [NUM_VOICES*3-1:0] 	    nco_mixmode,

	output reg [$clog2(NUM_VOICES)-1:0] active_voice,
	output reg 			    active_voice_changed,
	input 				    active_voice_done,

	input 				    wavetable0_we,
	input [$clog2(WAVETABLE_SIZE)-1:0]  wavetable0_write_addr,
	input [31:0] 			    wavetable0_write_data,

	input 				    wavetable1_we,
	input [$clog2(WAVETABLE_SIZE)-1:0]  wavetable1_write_addr,
	input [31:0] 			    wavetable1_write_data,

	output reg [31:0] 		    active_voice_data
);

genvar i;

reg [$clog2(NUM_VOICES)-1:0]		next_voice;

wire [31:0]				nco0_wave_addr[NUM_VOICES-1:0];
wire [31:0]				nco1_wave_addr[NUM_VOICES-1:0];

wire [$clog2(WAVETABLE_SIZE)-1:0] 	wavetable0_read_addr;
wire [$clog2(WAVETABLE_SIZE)-1:0] 	wavetable1_read_addr;

wire [31:0]				wavetable0_read_data;
wire [31:0]				wavetable1_read_data;

always @(*) begin
	next_voice = active_voice;
	if (active_voice_done) begin
		if (active_voice == 0)
			next_voice = NUM_VOICES-1;
		else
			next_voice = active_voice - 1;
	end
end

// Indicator of which voice have valid output, delayed one clock cycle to be in
// sync with the output from the wavetables.
always @(posedge clk)
	if (rst)
		active_voice <= 0;
	else
		active_voice <= next_voice;

always @(posedge clk)
	if (rst)
		active_voice_changed <= 0;
	else
		active_voice_changed <= active_voice_done;


// Mix output from the two wavetables according to the mixmode
always @(*) begin
	case(nco_mixmode[active_voice])
	3'h0:
		active_voice_data = (wavetable0_read_data +
				     wavetable1_read_data)/2;
	3'h1:
		active_voice_data = wavetable0_read_data - wavetable1_read_data;
	3'h2:
		active_voice_data = wavetable0_read_data | wavetable1_read_data;
	3'h3:
		active_voice_data = wavetable0_read_data ^ wavetable1_read_data;
	3'h4:
		active_voice_data = wavetable0_read_data & wavetable1_read_data;
	endcase
end

assign wavetable0_read_addr =
	nco0_wave_addr[next_voice][31:32-$clog2(WAVETABLE_SIZE)];

assign wavetable1_read_addr =
	nco1_wave_addr[next_voice][31:32-$clog2(WAVETABLE_SIZE)];

// I'm certain you should be able to do get a bit vector by
// doing something like this: ($clog2(WAVETABLE_SIZE)-8)'h0
// But I can't seem to get that to work...
wire [32-$clog2(WAVETABLE_SIZE):1] OFFSET_HI_PAD = 0;
wire [$clog2(WAVETABLE_SIZE)-8:1] OFFSET_LO_PAD = 0;

generate
for (i = 0; i < NUM_VOICES; i=i+1) begin : nco_gen
	sublime_nco nco0 (
		.clk		(clk),
		.rst		(rst),
		.enable		(nco0_enable[i]),
		.sync		(nco0_sync[i]),
		.freq		(nco0_freq[32*(i+1)-1:32*i]),
		.offset		({
				  OFFSET_HI_PAD,
				  nco0_offset[8*(i+1)-1:8*i],
				  OFFSET_LO_PAD
				  }),
		.wave_addr	(nco0_wave_addr[i])
	);

	sublime_nco nco1 (
		.clk		(clk),
		.rst		(rst),
		.enable		(nco1_enable[i]),
		.sync		(nco1_sync[i]),
		.freq		(nco1_freq[32*(i+1)-1:32*i]),
		.offset		({
				  OFFSET_HI_PAD,
				  nco1_offset[8*(i+1)-1:8*i],
				  OFFSET_LO_PAD
				  }),
		.wave_addr	(nco1_wave_addr[i])
	);
end
endgenerate

sublime_simple_dpram_sclk
      #(
	.ADDR_WIDTH($clog2(WAVETABLE_SIZE)),
	.DATA_WIDTH(32)
	)
wavetable0
       (
	.clk			(clk),
	.raddr			(wavetable0_read_addr),
	.waddr			(wavetable0_write_addr),
	.we			(wavetable0_we),
	.din			(wavetable0_write_data),
	.dout			(wavetable0_read_data)
);

sublime_simple_dpram_sclk
      #(
	.ADDR_WIDTH($clog2(WAVETABLE_SIZE)),
	.DATA_WIDTH(32)
	)
wavetable1
       (
	.clk			(clk),
	.raddr			(wavetable1_read_addr),
	.waddr			(wavetable1_write_addr),
	.we			(wavetable1_we),
	.din			(wavetable1_write_data),
	.dout			(wavetable1_read_data)
);

endmodule