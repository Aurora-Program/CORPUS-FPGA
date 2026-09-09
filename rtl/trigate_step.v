// SPDX-License-Identifier: Apache-2.0
// One local action, no combinational feedback, no dictionary decisions.
module trigate_step (
    input wire [9:0] values_i,
    input wire [7:0] allowed_i,
    output reg [9:0] values_o,
    output reg [1:0] status_o, // WAIT=0 CHANGED=1 CLOSED=2 CONFLICT=3
    output reg [1:0] areas_o,
    output reg [7:0] support_o,
    output reg needs_base_refinement_o,
    output wire direct_o
);
    function [1:0] canonical;
        input [1:0] v;
        begin canonical = (v == 0 || v == 3) ? v : 2'b01; end
    endfunction
    reg [1:0] a, b, m, r, e, d;
    reg base_open, match_candidate, cv, ca, cb, cm;
    reg [2:0] seen_zero, seen_one;
    integer i, opens, areas;
    assign direct_o = (status_o == 2) && (values_o[1:0] == 0);
    always @* begin
        a = canonical(values_i[9:8]);
        b = canonical(values_i[7:6]);
        m = canonical(values_i[5:4]);
        r = canonical(values_i[3:2]);
        e = canonical(values_i[1:0]);
        values_o = {a,b,m,r,e};
        status_o = 0;
        support_o = 0;
        needs_base_refinement_o = 0;
        opens = 0;
        if (a == 1) opens = opens + 1;
        if (b == 1) opens = opens + 1;
        if (m == 1) opens = opens + 1;
        base_open = opens >= 2;
        areas = 0;
        if (base_open) areas = areas + 1;
        if (r == 1) areas = areas + 1;
        if (e == 1) areas = areas + 1;
        areas_o = areas[1:0];
        d = 1;
        if ((a==0 && b==0) || (a==0 && m==0) || (b==0 && m==0)) d = 0;
        if ((a==3 && b==3) || (a==3 && m==3) || (b==3 && m==3)) d = 3;
        seen_zero = 0;
        seen_one = 0;
        ca = 0; cb = 0; cm = 0; cv = 0; match_candidate = 0;
        for (i = 0; i < 8; i = i + 1) begin
            ca = (i & 4) != 0;
            cb = (i & 2) != 0;
            cm = (i & 1) != 0;
            cv = (ca & cb) | (ca & cm) | (cb & cm);
            match_candidate = allowed_i[i] &&
                ((a==1) || ((a==3)==ca)) &&
                ((b==1) || ((b==3)==cb)) &&
                ((m==1) || ((m==3)==cm));
            // Residual E with open R must never become an orientation constraint.
            if (r != 1 && e != 1 && (cv ^ (e==3)) != (r==3))
                match_candidate = 0;
            if (match_candidate) begin
                support_o[i] = 1;
                seen_one = seen_one | {ca,cb,cm};
                seen_zero = seen_zero | ~{ca,cb,cm};
            end
        end
        if (support_o == 0) begin
            status_o = 3;
        end else if (areas == 0) begin
            if (d == 1) needs_base_refinement_o = 1;
            else status_o = 2;
        end else if (areas == 1) begin
            if (base_open) begin
                if (a==1 && (seen_zero[2] != seen_one[2]))
                    values_o[9:8] = seen_one[2] ? 3 : 0;
                if (b==1 && (seen_zero[1] != seen_one[1]))
                    values_o[7:6] = seen_one[1] ? 3 : 0;
                if (m==1 && (seen_zero[0] != seen_one[0]))
                    values_o[5:4] = seen_one[0] ? 3 : 0;
            end else if (r==1 && d!=1) begin
                values_o[3:2] = (d==3) ^ (e==3) ? 3 : 0;
            end else if (e==1 && d!=1) begin
                values_o[1:0] = (d==3) ^ (r==3) ? 3 : 0;
            end
            if (values_o != {a,b,m,r,e}) status_o = 1;
        end
    end
endmodule
