`timescale 1ns/1ps
module tb_board;
    reg clk = 0;
    always #5 clk = !clk;
    wire [5:0] leds;
    corpus_selftest_top #(.CLOCK_HZ(16)) dut(clk, leds);
    initial begin
        repeat (300) @(posedge clk);
        #1;
        if (dut.completed != 16 || dut.failed || !dut.last_pass)
            $fatal(1, "board selftest failed: completed=%0d failed=%b",
                   dut.completed, dut.failed);
        if (leds[4] !== 0) $fatal(1, "pass LED polarity wrong");
        $display("PASS Tang Nano 9K selftest simulation: all 16 cases");
        $finish;
    end
endmodule
