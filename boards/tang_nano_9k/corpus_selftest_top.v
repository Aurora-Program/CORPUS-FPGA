// SPDX-License-Identifier: Apache-2.0
// 27 MHz onboard oscillator and six active-low LEDs; no external wiring.
module corpus_selftest_top #(parameter integer CLOCK_HZ = 27000000) (
    input wire clk,
    output wire [5:0] leds
);
    reg [2:0] startup = 0;
    always @(posedge clk) if (startup != 7) startup <= startup + 1'b1;
    wire rst = startup != 7;
    reg [31:0] ticks = 0;
    reg [3:0] next_case = 0, shown_case = 0;
    reg [3:0] active_case = 0;
    reg [23:0] expected = 0;
    reg failed = 0, last_pass = 0, heartbeat = 0;
    reg [4:0] completed = 0;
// BEGIN GENERATED CASES
function [9:0] test_input;
    input [3:0] index;
    begin
        case (index)
            4'd0: test_input = 10'h34;
            4'd1: test_input = 10'h3c1;
            4'd2: test_input = 10'h5c;
            4'd3: test_input = 10'h50;
            4'd4: test_input = 10'h357;
            4'd5: test_input = 10'h3c0;
            4'd6: test_input = 10'h3c3;
            4'd7: test_input = 10'h10;
            4'd8: test_input = 10'hd0;
            4'd9: test_input = 10'h155;
            4'd10: test_input = 10'h3d7;
            4'd11: test_input = 10'h17;
            4'd12: test_input = 10'h350;
            4'd13: test_input = 10'h11c;
            4'd14: test_input = 10'h14c;
            4'd15: test_input = 10'h15c;
        endcase
    end
endfunction
function [23:0] test_expected;
    input [3:0] index;
    begin
        case (index)
            4'd0: test_expected = 24'hc1408;
            4'd1: test_expected = 24'hf0d500;
            4'd2: test_expected = 24'h3f1420;
            4'd3: test_expected = 24'h14041c;
            4'd4: test_expected = 24'hd5cbc0;
            4'd5: test_expected = 24'hf03000;
            4'd6: test_expected = 24'hf0e100;
            4'd7: test_expected = 24'h4200d;
            4'd8: test_expected = 24'h340012;
            4'd9: test_expected = 24'h554ffc;
            4'd10: test_expected = 24'hf4d700;
            4'd11: test_expected = 24'h7d40c;
            4'd12: test_expected = 24'hc01440;
            4'd13: test_expected = 24'hcf1480;
            4'd14: test_expected = 24'hf31500;
            4'd15: test_expected = 24'h5707a0;
        endcase
    end
endfunction
// END GENERATED CASES
    wire request_valid = !rst && ticks == 0;
    wire request_ready, response_valid;
    wire [9:0] response_values;
    wire [1:0] response_status, response_areas;
    wire [7:0] response_support;
    wire response_needs_base_refinement, response_direct;
    wire [23:0] response = {response_values, response_status, response_areas,
        response_support, response_needs_base_refinement, response_direct};
    trigate_core core (
        .clk(clk), .rst(rst), .request_valid(request_valid),
        .request_ready(request_ready), .request_values(test_input(next_case)),
        .request_allowed(8'hff), .response_valid(response_valid),
        .response_ready(1'b1), .response_values(response_values),
        .response_status(response_status), .response_areas(response_areas),
        .response_support(response_support),
        .response_needs_base_refinement(response_needs_base_refinement),
        .response_direct(response_direct)
    );
    always @(posedge clk) begin
        if (rst) begin
            ticks <= 0; next_case <= 0; shown_case <= 0; active_case <= 0;
            expected <= 0; failed <= 0; last_pass <= 0; heartbeat <= 0;
            completed <= 0;
        end else begin
            if (ticks == CLOCK_HZ - 1) ticks <= 0;
            else ticks <= ticks + 1'b1;
            if (request_valid && request_ready) begin
                expected <= test_expected(next_case);
                active_case <= next_case;
                next_case <= next_case + 1'b1;
            end
            if (response_valid) begin
                if (response != expected) failed <= 1;
                last_pass <= response == expected;
                shown_case <= active_case;
                heartbeat <= !heartbeat;
                if (completed < 16) completed <= completed + 1'b1;
            end
        end
    end
    // LEDs 0..3 show case id; LED4 on = passed so far; LED5 alternates per case.
    assign leds = ~{heartbeat, (last_pass && !failed), shown_case};
endmodule
