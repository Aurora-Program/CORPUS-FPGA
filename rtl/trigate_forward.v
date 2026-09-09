// SPDX-License-Identifier: Apache-2.0
// Semantic encoding: 0=00, 1=11, open=01 or 10; outputs canonicalize open=01.
module trigate_forward (
    input wire [1:0] a, b, m, orientation,
    output reg [1:0] r, e
);
    wire z = ((a == 0) && (b == 0)) || ((a == 0) && (m == 0)) ||
             ((b == 0) && (m == 0));
    wire o = ((a == 3) && (b == 3)) || ((a == 3) && (m == 3)) ||
             ((b == 3) && (m == 3));
    wire has_zero = (a == 0) || (b == 0) || (m == 0);
    wire has_one = (a == 3) || (b == 3) || (m == 3);
    always @* begin
        r = 2'b01;
        e = 2'b01;
        if (z || o) begin
            if (orientation == 0 || orientation == 3) begin
                r = (o ^ (orientation == 3)) ? 2'b11 : 2'b00;
                e = orientation;
            end
        end else if (has_zero != has_one) begin
            e = has_one ? 2'b11 : 2'b00;
        end
    end
endmodule
