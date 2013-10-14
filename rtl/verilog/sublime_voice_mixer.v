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

module sublime_voice_mixer #(
	parameter NUM_VOICES = 8
)(
	input 			       clk,
	input 			       rst,

	input [$clog2(NUM_VOICES)-1:0] active_voice,
	input 			       active_voice_changed,

	input [7:0] 		       active_voice_velocity,
	input [31:0] 		       active_voice_data,

	output reg [31:0] 	       mixed_data
);

reg		last_voice;
reg [31:0]	mix;

wire [31:0]	mul_op1;
wire [7:0]	mul_op2;
wire [31:0]	mul_op1_unsigned;
wire [7:0]	mul_op2_unsigned;
reg [39:0]	mul_res;
reg		mul_res_neg;
reg		mul_valid;
wire [31:0]	scaled_voice;

always @(posedge clk)
	last_voice <= active_voice_changed && active_voice == 0;

always @(posedge clk)
	mul_valid <= active_voice_changed;

assign scaled_voice = mul_res_neg ? ~mul_res[39:8] + 1 : mul_res[39:8];

always @(posedge clk)
	if (rst) begin
		mix <= 0;
		mixed_data <= 0;
	end else if (last_voice) begin
		mixed_data <= mix;
		mix <= scaled_voice;
	end else if (mul_valid) begin
		mix <= mix + scaled_voice;
	end

assign mul_op1 = active_voice_data;
assign mul_op2 = active_voice_velocity;

assign mul_op1_unsigned = mul_op1[31] ? ~mul_op1 + 1 : mul_op1;
assign mul_op2_unsigned = mul_op2[7] ? ~mul_op2 + 1 : mul_op2;

always @(posedge clk) begin
	mul_res_neg <= mul_op1[31] ^ mul_op2[7];
	mul_res <= mul_op1_unsigned * mul_op2_unsigned;
end

endmodule