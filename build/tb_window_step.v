module tb;
  reg [9:0] values;
  reg [7:0] allowed;
  wire [9:0] out_values;
  wire [1:0] status, areas;
  wire [7:0] support;
  wire needs, direct;
  trigate_step dut(values, allowed, out_values, status, areas, support, needs, direct);
  initial begin
    values = {2'b11, 2'b11, 2'b11, 2'b00, 2'b01};
    allowed = 8'hff;
    #1;
    $display("status=%0d areas=%0d values=%h needs=%0d", status, areas, out_values, needs);
    $finish;
  end
endmodule
