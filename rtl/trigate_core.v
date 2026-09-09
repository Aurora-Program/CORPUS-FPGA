// SPDX-License-Identifier: Apache-2.0
// Single clock elastic response. Consume and replace are allowed on one edge.
module trigate_core (
    input wire clk, rst,
    input wire request_valid,
    output wire request_ready,
    input wire [9:0] request_values,
    input wire [7:0] request_allowed,
    output reg response_valid,
    input wire response_ready,
    output reg [9:0] response_values,
    output reg [1:0] response_status, response_areas,
    output reg [7:0] response_support,
    output reg response_needs_base_refinement, response_direct
);
    wire [9:0] values;
    wire [1:0] status, areas;
    wire [7:0] support;
    wire needs_base, direct;
    assign request_ready = !rst && (!response_valid || response_ready);
    trigate_step step(request_values, request_allowed, values, status, areas,
                      support, needs_base, direct);
    always @(posedge clk) begin
        if (rst) begin
            response_valid <= 0;
            response_values <= 10'b0101010101;
            response_status <= 0;
            response_areas <= 3;
            response_support <= 0;
            response_needs_base_refinement <= 0;
            response_direct <= 0;
        end else if (request_ready) begin
            response_valid <= request_valid;
            if (request_valid) begin
                response_values <= values;
                response_status <= status;
                response_areas <= areas;
                response_support <= support;
                response_needs_base_refinement <= needs_base;
                response_direct <= direct;
            end
        end
    end
endmodule
