`timescale 1ns/1ps
module tb_step;
    reg [9:0] values;
    reg [7:0] allowed;
    wire [9:0] result;
    wire [1:0] status, areas;
    wire [7:0] support;
    wire needs_base, direct;
    reg [23:0] expected, actual;
    integer file_handle, count, scan_result;
    string vectors;
    trigate_step dut(values, allowed, result, status, areas, support, needs_base, direct);
    initial begin
        if (!$value$plusargs("vectors=%s", vectors)) $fatal(1, "missing vectors");
        file_handle = $fopen(vectors, "r");
        if (file_handle == 0) $fatal(1, "cannot open vectors");
        count = 0;
        while (!$feof(file_handle)) begin
            scan_result = $fscanf(file_handle, "%h %h %h\n", values, allowed, expected);
            if (scan_result != 3) $fatal(1, "invalid vector line");
            #1;
            actual = {result, status, areas, support, needs_base, direct};
            if (actual !== expected)
                $fatal(1, "case %0d input=%h mask=%h got=%h expected=%h",
                       count, values, allowed, actual, expected);
            count = count + 1;
        end
        $display("PASS relational vectors: %0d", count);
        $fclose(file_handle);
        $finish;
    end
endmodule
