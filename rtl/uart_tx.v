// 8N1 UART transmitter.
module uart_tx #(parameter integer CLKS_PER_BIT = 234) (
    input wire clk,
    input wire [7:0] data,
    input wire start,
    output reg tx = 1,
    output reg busy = 0
);
    reg [15:0] ticks = 0;
    reg [3:0] bit_index = 0;
    reg [9:0] frame = 10'b1111111111;

    always @(posedge clk) begin
        if (!busy) begin
            tx <= 1;
            ticks <= 0;
            bit_index <= 0;
            if (start) begin
                frame <= {1'b1, data, 1'b0};
                tx <= 0;
                busy <= 1;
            end
        end else if (ticks == CLKS_PER_BIT - 1) begin
            ticks <= 0;
            if (bit_index == 9) begin
                busy <= 0;
                tx <= 1;
            end else begin
                bit_index <= bit_index + 1'b1;
                tx <= frame[bit_index + 1'b1];
            end
        end else ticks <= ticks + 1'b1;
    end
endmodule