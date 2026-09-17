module tb;
    reg [9:0] ds_in, de_in, do_in;
    reg [7:0] ds_allowed, de_allowed, do_allowed;
    wire [9:0] ds_out, de_out, do_out;
    wire [1:0] ds_status, de_status, do_status;
    wire [1:0] ds_areas, de_areas, do_areas;
    wire [7:0] ds_support, de_support, do_support;
    wire ds_needs, de_needs, do_needs;
    wire ds_direct, de_direct, do_direct;
    trigate_step ds(ds_in, ds_allowed, ds_out, ds_status, ds_areas, ds_support, ds_needs, ds_direct);
    trigate_step de(de_in, de_allowed, de_out, de_status, de_areas, de_support, de_needs, de_direct);
    trigate_step do_gate(do_in, do_allowed, do_out, do_status, do_areas, do_support, do_needs, do_direct);
    initial begin
        ds_allowed = 8'hff; de_allowed = 8'h00; do_allowed = 8'h55;
        ds_in = {2'b01, 2'b01, 2'b01, 2'b00, 2'b01};
        #1;
        $display("ds status=%0d areas=%0d out=%h", ds_status, ds_areas, ds_out);
        de_in = {2'b00, 2'b00, 2'b00, ds_out[3:2], ds_out[1:0]};
        #1;
        $display("de status=%0d areas=%0d out=%h", de_status, de_areas, de_out);
        do_in = {2'b01, 2'b01, 2'b01, de_out[3:2], de_out[1:0]};
        #1;
        $display("do status=%0d areas=%0d out=%h", do_status, do_areas, do_out);
        $finish;
    end
endmodule
