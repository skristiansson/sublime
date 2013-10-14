`timescale 1ns/1ns
module sublime_tb;

parameter NUM_VOICES = 8;
parameter WAVETABLE_SIZE = 8192;	// Should be a power of 2
parameter WB_AW = 32;
parameter WB_DW = 32;
parameter MAX_BURST_LEN = 16;

reg 			clk = 1'b1;
reg 			rst = 1'b1;
reg 			err;
wire [31:0]		left_sample;
wire [31:0]		right_sample;

wire [WB_AW-1:0]	wb_m2s_adr;
wire [WB_DW-1:0]	wb_m2s_dat;
wire [WB_DW/8-1:0]	wb_m2s_sel;
wire		 	wb_m2s_we;
wire			wb_m2s_cyc;
wire			wb_m2s_stb;
wire [2:0] 		wb_m2s_cti;
wire [1:0]		wb_m2s_bte;
wire [WB_DW-1:0]	wb_s2m_dat;
wire			wb_s2m_ack;
wire			wb_s2m_err;
wire			wb_s2m_rty;

vlog_tb_utils vlog_tb_utils0();

always #10 clk <= ~clk; // 50 MHz
initial #100 rst = 0;

wb_bfm_master #(
	.MAX_BURST_LENGTH (MAX_BURST_LEN)
) wb_bfm_master (
	.wb_clk_i (clk),
	.wb_rst_i (rst),
	.wb_adr_o (wb_m2s_adr),
	.wb_dat_o (wb_m2s_dat),
	.wb_sel_o (wb_m2s_sel),
	.wb_we_o  (wb_m2s_we),
	.wb_cyc_o (wb_m2s_cyc),
	.wb_stb_o (wb_m2s_stb),
	.wb_cti_o (wb_m2s_cti),
	.wb_bte_o (wb_m2s_bte),
	.wb_dat_i (wb_s2m_dat),
	.wb_ack_i (wb_s2m_ack),
	.wb_err_i (wb_s2m_err),
	.wb_rty_i (wb_s2m_rty)
);

sublime #(
	.NUM_VOICES(NUM_VOICES),
	.WAVETABLE_SIZE(WAVETABLE_SIZE),	// Should be a power of 2
	.WB_AW(WB_AW),
	.WB_DW(WB_DW)
) sublime0 (
	.clk(clk),
	.rst(rst),

	// Stereo output streams
	.left_sample(left_sample),
	.right_sample(right_sample),

	// Wishbone slave interface
	.wb_adr_i(wb_m2s_adr),
	.wb_dat_i(wb_m2s_dat),
	.wb_sel_i(wb_m2s_sel),
	.wb_we_i(wb_m2s_we),
	.wb_cyc_i(wb_m2s_cyc),
	.wb_stb_i(wb_m2s_stb),
	.wb_cti_i(wb_m2s_cti),
	.wb_bte_i(wb_m2s_bte),
	.wb_dat_o(wb_s2m_dat),
	.wb_ack_o(wb_s2m_ack),
	.wb_err_o(wb_s2m_err),
	.wb_rty_o(wb_s2m_rty)
);
localparam VOICE0_BASE		= 32'h00000000;
localparam VOICE1_BASE		= 32'h00000010;
localparam VOICE2_BASE		= 32'h00000020;
localparam VOICE3_BASE		= 32'h00000030;

localparam VOICE0_OSC0_FREQ	= VOICE0_BASE + 32'h0;
localparam VOICE0_OSC1_FREQ	= VOICE0_BASE + 32'h4;
localparam VOICE0_CTRL		= VOICE0_BASE + 32'h8;
localparam VOICE0_ENVELOPE	= VOICE0_BASE + 32'hc;

localparam VOICE1_OSC0_FREQ	= VOICE1_BASE + 32'h0;
localparam VOICE1_OSC1_FREQ	= VOICE1_BASE + 32'h4;
localparam VOICE1_CTRL		= VOICE1_BASE + 32'h8;
localparam VOICE1_ENVELOPE	= VOICE1_BASE + 32'hc;

localparam VOICE2_OSC0_FREQ	= VOICE2_BASE + 32'h0;
localparam VOICE2_OSC1_FREQ	= VOICE2_BASE + 32'h4;
localparam VOICE2_CTRL		= VOICE2_BASE + 32'h8;
localparam VOICE2_ENVELOPE	= VOICE2_BASE + 32'hc;

localparam MAIN_CONTROL		= 32'h00000808;

localparam WAVETABLE0_BASE	= 32'h00010000;
localparam WAVETABLE1_BASE	= 32'h00020000;

localparam TRIANGLE_MULTIPLIER	= (1<<31)/(WAVETABLE_SIZE/4);
integer i;
initial begin
	wb_bfm_master.reset();

	// Reset all voice registers
	for (i = VOICE0_BASE; i < (VOICE0_BASE + 4*NUM_VOICES)*4; i=i+4) begin
		wb_bfm_master.write(i, 32'd0, 4'hf, err);
	end

	// Write a triangle waveform to wavetable0
	for (i = 0; i < WAVETABLE_SIZE/4; i = i+1) begin
		wb_bfm_master.write(WAVETABLE0_BASE+i*4,
				    i*TRIANGLE_MULTIPLIER,
				    4'hf, err);
		wb_bfm_master.write(WAVETABLE1_BASE+i*4,
				    i*TRIANGLE_MULTIPLIER,
				    4'hf, err);
	end
	for (i = 0; i < WAVETABLE_SIZE/4; i = i+1) begin
		wb_bfm_master.write(WAVETABLE0_BASE+(i+WAVETABLE_SIZE/4)*4,
				    ((1<<31)-1)-i*TRIANGLE_MULTIPLIER,
				    4'hf, err);
		wb_bfm_master.write(WAVETABLE1_BASE+(i+WAVETABLE_SIZE/4)*4,
				    ((1<<31)-1)-i*TRIANGLE_MULTIPLIER,
				    4'hf, err);
	end
	for (i = 0; i < WAVETABLE_SIZE/4; i = i+1) begin
		wb_bfm_master.write(WAVETABLE0_BASE+(i+WAVETABLE_SIZE/2)*4,
				    -i*TRIANGLE_MULTIPLIER,
				    4'hf, err);
		wb_bfm_master.write(WAVETABLE1_BASE+(i+WAVETABLE_SIZE/2)*4,
				    -i*TRIANGLE_MULTIPLIER,
				    4'hf, err);
	end
	for (i = 0; i < WAVETABLE_SIZE/4; i = i+1) begin
		wb_bfm_master.write(WAVETABLE0_BASE+(i+3*WAVETABLE_SIZE/4)*4,
				    i*TRIANGLE_MULTIPLIER - ((1<<31)-1),
				    4'hf, err);
		wb_bfm_master.write(WAVETABLE1_BASE+(i+3*WAVETABLE_SIZE/4)*4,
				    i*TRIANGLE_MULTIPLIER - ((1<<31)-1),
				    4'hf, err);
	end

	// Enable oscs and set velocity
	wb_bfm_master.write(VOICE0_CTRL, 32'h3f03, 4'hf, err);
	wb_bfm_master.write(VOICE1_CTRL, 32'h3f03, 4'hf, err);
	wb_bfm_master.write(VOICE2_CTRL, 32'h3f03, 4'hf, err);
	// Set osc frequency to 440Hz (#A4)
	// 2^32/(50e6/440) = 37796
	wb_bfm_master.write(VOICE0_OSC0_FREQ, 32'd37796, 4'hf, err);
	wb_bfm_master.write(VOICE0_OSC1_FREQ, 32'd37796, 4'hf, err);
	wb_bfm_master.write(VOICE1_OSC0_FREQ, 32'd37796, 4'hf, err);
	wb_bfm_master.write(VOICE1_OSC1_FREQ, 32'd37796, 4'hf, err);
	wb_bfm_master.write(VOICE2_OSC0_FREQ, 32'd37796, 4'hf, err);
	wb_bfm_master.write(VOICE2_OSC1_FREQ, 32'd37796, 4'hf, err);

	// Assert sync to all voices
	wb_bfm_master.write(MAIN_CONTROL, 32'h1, 4'hf, err);

	// Deassert sync to all voices
	wb_bfm_master.write(MAIN_CONTROL, 32'h0, 4'hf, err);

	#5000000 $finish();
end

endmodule