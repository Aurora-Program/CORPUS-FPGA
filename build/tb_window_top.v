module tb;
    reg clk = 0;
    reg uart_rx = 1;
    wire uart_tx;
    wire [5:0] leds;
    corpus_window_top dut(.clk(clk), .uart_rx(uart_rx), .uart_tx(uart_tx), .leds(leds));
    always #1 clk = ~clk;
    task send_byte;
        input [7:0] value;
        integer bit;
        begin
            uart_rx = 0;
            repeat (234) @(posedge clk);
            for (bit = 0; bit < 8; bit = bit + 1) begin
                uart_rx = value[bit];
                repeat (234) @(posedge clk);
            end
            uart_rx = 1;
            repeat (234) @(posedge clk);
        end
    endtask
    initial begin
        send_byte(8'hA6);
        send_byte(8'h3F);
        send_byte(8'h50);
        send_byte(8'h55);
        send_byte(8'h00);
        send_byte(8'hFF);
        send_byte(8'h00);
        send_byte(8'h55);
        repeat (10000) @(posedge clk);
        $display("busy=%0d stage=%0d rounds=%0d ds=%0d/%0d de=%0d/%0d do=%0d/%0d",
                 dut.busy, dut.stage, dut.rounds,
                 dut.ds_status_latched, dut.ds_areas_latched,
                 dut.de_status_latched, dut.de_areas_latched,
                 dut.do_status, dut.do_areas);
        $finish;
    end
endmodule
