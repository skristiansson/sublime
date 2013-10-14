module sublime_nco_tb;

wire enable;
wire sync;
wire [31:0] freq;
wire [31:0] offset;
wire [31:0] wave_addr;
reg clk = 1'b1;
reg rst = 1'b1;

always #5 clk <= ~clk;
initial #100 rst =0;

sublime_nco sublime_nco0 (
	.clk		(clk),
	.rst		(rst),
	.enable		(enable),
	.sync		(sync),
	.freq		(freq),
	.offset		(offset),
	.wave_addr	(wave_addr)
);

assign offset = 32'h10000000;
assign freq = 32'h08000000;
assign sync = rst;
assign enable = 1;

initial begin
	if($test$plusargs("vcd")) begin
		$dumpfile("testlog.vcd");
		$dumpvars(0);
	end
end

endmodule