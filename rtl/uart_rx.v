// 8N1 UART receiver, sampled at the middle of each bit.
module uart_rx #(parameter integer CLKS_PER_BIT = 234) (
    input wire clk,
    input wire rx,
    output reg [7:0] data,
    output reg valid
);
    reg [1:0] state = 0;
    reg [15:0] ticks = 0;
    reg [2:0] bit_index = 0;
    reg [7:0] shift = 0;

    always @(posedge clk) begin
        valid <= 0;
        case (state)
            0: begin
                ticks <= 0;
                bit_index <= 0;
                if (!rx) state <= 1;
            end
            1: begin
                if (ticks == (CLKS_PER_BIT / 2)) begin
                    ticks <= 0;
                    if (!rx) state <= 2;
                    else state <= 0;
                end else ticks <= ticks + 1'b1;
            end
            2: begin
                if (ticks == CLKS_PER_BIT - 1) begin
                    ticks <= 0;
                    shift[bit_index] <= rx;
                    if (bit_index == 7) state <= 3;
                    else bit_index <= bit_index + 1'b1;
                end else ticks <= ticks + 1'b1;
            end
            default: begin
                if (ticks == CLKS_PER_BIT - 1) begin
                    ticks <= 0;
                    state <= 0;
                    data <= shift;
                    valid <= 1;
                end else ticks <= ticks + 1'b1;
            end
        endcase
    end
endmodule