`timescale 1ns/1ps
module tb_core;
    reg clk = 0;
    always #5 clk = !clk;
    reg rst = 1, request_valid = 0, response_ready = 0;
    reg [9:0] request_values = 0;
    reg [7:0] request_allowed = 255;
    wire request_ready, response_valid;
    wire [9:0] response_values;
    wire [1:0] response_status, response_areas;
    wire [7:0] response_support;
    wire response_needs_base_refinement, response_direct;
    reg [23:0] expected [0:1023];
    reg [9:0] inputs [0:1023];
    reg [7:0] masks [0:1023];
    integer file_handle, n, rc, sent, received, cycle;
    reg accepted, consumed;
    reg [23:0] held;
    reg was_stalled;
    string vectors;
    wire [23:0] response = {response_values, response_status, response_areas,
        response_support, response_needs_base_refinement, response_direct};
    trigate_core dut(.*);
    initial begin
        if (!$value$plusargs("vectors=%s", vectors)) $fatal(1, "missing vectors");
        file_handle = $fopen(vectors, "r");
        if (file_handle == 0) $fatal(1, "cannot open vectors");
        n = 0;
        while (!$feof(file_handle) && n < 1024) begin
            rc = $fscanf(file_handle, "%h %h %h\n", inputs[n], masks[n], expected[n]);
            if (rc != 3) $fatal(1, "invalid vector line");
            n = n + 1;
        end
        $fclose(file_handle);
        @(posedge clk); #1;
        if (response_valid || request_ready) $fatal(1, "reset handshake failed");
        @(negedge clk); rst = 0;
        sent = 0; received = 0; cycle = 0; was_stalled = 0;
        while (received < n && cycle < 10000) begin
            // Deterministic gaps and backpressure exercise consume-and-replace.
            request_valid = sent < n && cycle % 5 != 0;
            response_ready = cycle % 7 >= 3;
            if (sent < n) begin
                request_values = inputs[sent];
                request_allowed = masks[sent];
            end
            #1;
            if (was_stalled && (!response_valid || response !== held))
                $fatal(1, "response changed under backpressure");
            accepted = request_valid && request_ready;
            consumed = response_valid && response_ready;
            if (consumed) begin
                if (received >= sent || response !== expected[received])
                    $fatal(1, "response loss, duplication or reorder at %0d", received);
                received = received + 1;
            end
            was_stalled = response_valid && !response_ready;
            held = response;
            @(posedge clk); #1;
            if (accepted) sent = sent + 1;
            cycle = cycle + 1;
            @(negedge clk);
        end
        if (sent != n || received != n) $fatal(1, "stream timeout");
        // Reset discards a pending response and permits a clean restart.
        request_valid = 1; response_ready = 0; request_values = inputs[0];
        request_allowed = masks[0];
        @(posedge clk); #1;
        if (!response_valid) $fatal(1, "pending response missing");
        @(negedge clk); rst = 1;
        @(posedge clk); #1;
        if (response_valid || request_ready) $fatal(1, "reset did not flush response");
        @(negedge clk); rst = 0;
        @(posedge clk); #1;
        if (!response_valid || response !== expected[0]) $fatal(1, "restart failed");
        $display("PASS clocked stream: %0d requests, backpressure and reset", n);
        $finish;
    end
endmodule
