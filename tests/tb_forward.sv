`timescale 1ns/1ps
module tb_forward;
    reg [1:0] a, b, m, orientation;
    wire [1:0] r, e;
    reg [7:0] packed_values;
    reg [3:0] expected;
    integer file_handle, count, scan_result;
    string vectors;
    trigate_forward dut(a,b,m,orientation,r,e);
    initial begin
        if (!$value$plusargs("vectors=%s", vectors)) $fatal(1, "missing vectors");
        file_handle = $fopen(vectors, "r");
        if (file_handle == 0) $fatal(1, "cannot open vectors");
        count = 0;
        while (!$feof(file_handle)) begin
            scan_result = $fscanf(file_handle, "%h %h\n", packed_values, expected);
            if (scan_result != 2) $fatal(1, "invalid vector line");
            {a,b,m,orientation} = packed_values;
            #1;
            if ({r,e} !== expected) $fatal(1, "forward input=%h got=%h expected=%h",
                                         packed_values, {r,e}, expected);
            count = count + 1;
        end
        $display("PASS forward vectors: %0d", count);
        $fclose(file_handle);
        $finish;
    end
endmodule
