// Fixed Aurora3 window pilot for FPGA.
// Frame: A6 + 4 packed bytes (13 pilot trits) + DS/DE/DO masks.
// The pilot uses shared refs DS=(I0,I1,I2,O3,O4),
// DE=(K0,K1,K2,O3,O4), and DO=(O0,O1,O2,O3,O4).
module corpus_window_top #(parameter integer CLOCK_HZ = 27000000,
                           parameter integer BAUD = 115200) (
    input wire clk,
    input wire uart_rx,
    output wire uart_tx,
    output wire [5:0] leds
);
    localparam integer CLKS_PER_BIT = CLOCK_HZ / BAUD;
    localparam integer RX_BYTES = 7;
    localparam integer TX_BYTES = 8;
    reg [2:0] startup = 0;
    reg [7:0] rx_count = 0;
    reg [7:0] rx_data_buf [0:12];
    reg [7:0] tx_frame [0:7];
    reg [7:0] tx_count = 0;
    reg tx_pending = 0, tx_start = 0;
    reg [7:0] tx_data = 0;
    reg activity = 0;
    reg [1:0] stage = 0;
    reg [1:0] rounds = 0;
    reg busy = 0;
    reg [1:0] ds_status_latched, de_status_latched, do_status_latched;
    reg [1:0] ds_areas_latched, de_areas_latched, do_areas_latched;
    reg ds_needs_latched, de_needs_latched, do_needs_latched;
    reg [1:0] i_cells [0:12];
    reg [7:0] allowed_ds, allowed_de, allowed_do;
    wire rst = startup != 7;
    wire [7:0] rx_byte;
    wire rx_valid, tx_busy;
    wire [9:0] ds_input = {i_cells[0], i_cells[1], i_cells[2], i_cells[12], i_cells[11]};
    wire [9:0] de_input = {i_cells[3], i_cells[4], i_cells[5], i_cells[12], i_cells[11]};
    wire [9:0] do_input = {i_cells[6], i_cells[7], i_cells[8], i_cells[12], i_cells[11]};
    wire [9:0] ds_values, de_values, do_values;
    wire [1:0] ds_status, de_status, do_status;
    wire [1:0] ds_areas, de_areas, do_areas;
    wire [7:0] ds_support, de_support, do_support;
    wire ds_needs, de_needs, do_needs;
    wire ds_direct, de_direct, do_direct;
    uart_rx #(.CLKS_PER_BIT(CLKS_PER_BIT)) rx_unit (
        .clk(clk), .rx(uart_rx), .data(rx_byte), .valid(rx_valid));
    uart_tx #(.CLKS_PER_BIT(CLKS_PER_BIT)) tx_unit (
        .clk(clk), .data(tx_data), .start(tx_start), .tx(uart_tx), .busy(tx_busy));
    trigate_step ds_gate (ds_input, allowed_ds, ds_values, ds_status, ds_areas,
                          ds_support, ds_needs, ds_direct);
    trigate_step de_gate (de_input, allowed_de, de_values, de_status, de_areas,
                          de_support, de_needs, de_direct);
    trigate_step do_gate (do_input, allowed_do, do_values, do_status, do_areas,
                          do_support, do_needs, do_direct);

    function [1:0] unpack_trit;
        input [7:0] byte_value;
        input [1:0] offset;
        begin
            case (offset)
                0: unpack_trit = byte_value[1:0];
                1: unpack_trit = byte_value[3:2];
                2: unpack_trit = byte_value[5:4];
                default: unpack_trit = byte_value[7:6];
            endcase
        end
    endfunction
    function [7:0] pack4;
        input [1:0] a, b, c, d;
        begin pack4 = {d, c, b, a}; end
    endfunction

    always @(posedge clk) begin
        tx_start <= 0;
        if (rst) begin
            startup <= startup + 1'b1;
            rx_count <= 0;
            tx_count <= 0;
            tx_pending <= 0;
            busy <= 0;
            stage <= 0;
            rounds <= 0;
            ds_status_latched <= 0;
            de_status_latched <= 0;
            do_status_latched <= 0;
            ds_areas_latched <= 0;
            de_areas_latched <= 0;
            do_areas_latched <= 0;
            ds_needs_latched <= 0;
            de_needs_latched <= 0;
            do_needs_latched <= 0;
            activity <= 0;
        end else begin
            if (rx_valid && !busy && !tx_pending) begin
                if (rx_count == 0) begin
                    if (rx_byte == 8'hA6) rx_count <= 1;
                end else begin
                    rx_data_buf[rx_count - 1] <= rx_byte;
                    if (rx_count == RX_BYTES) begin
                        i_cells[0] <= unpack_trit(rx_data_buf[0], 0);
                        i_cells[1] <= unpack_trit(rx_data_buf[0], 1);
                        i_cells[2] <= unpack_trit(rx_data_buf[0], 2);
                        i_cells[3] <= unpack_trit(rx_data_buf[0], 3);
                        i_cells[4] <= unpack_trit(rx_data_buf[1], 0);
                        i_cells[5] <= unpack_trit(rx_data_buf[1], 1);
                        i_cells[6] <= unpack_trit(rx_data_buf[1], 2);
                        i_cells[7] <= unpack_trit(rx_data_buf[1], 3);
                        i_cells[8] <= unpack_trit(rx_data_buf[2], 0);
                        i_cells[9] <= unpack_trit(rx_data_buf[2], 1);
                        i_cells[10] <= unpack_trit(rx_data_buf[2], 2);
                        i_cells[11] <= unpack_trit(rx_data_buf[2], 3);
                        i_cells[12] <= unpack_trit(rx_data_buf[3], 0);
                        allowed_ds <= rx_data_buf[4];
                        allowed_de <= rx_data_buf[5];
                        allowed_do <= rx_byte;
                        busy <= 1;
                        stage <= 0;
                        rounds <= 0;
                        rx_count <= 0;
                    end else rx_count <= rx_count + 1'b1;
                end
            end
            if (busy) begin
                if (stage == 0) begin
                    ds_status_latched <= ds_status;
                    ds_areas_latched <= ds_areas;
                    ds_needs_latched <= ds_needs;
                    i_cells[12] <= ds_values[3:2];
                    i_cells[11] <= ds_values[1:0];
                    stage <= 1;
                end else if (stage == 1) begin
                    de_status_latched <= de_status;
                    de_areas_latched <= de_areas;
                    de_needs_latched <= de_needs;
                    i_cells[12] <= de_values[3:2];
                    i_cells[11] <= de_values[1:0];
                    stage <= 2;
                end else begin
                    do_status_latched <= do_status;
                    do_areas_latched <= do_areas;
                    do_needs_latched <= do_needs;
                    i_cells[6] <= do_values[9:8];
                    i_cells[7] <= do_values[7:6];
                    i_cells[8] <= do_values[5:4];
                    i_cells[12] <= do_values[3:2];
                    i_cells[11] <= do_values[1:0];
                    if (rounds == 2) begin
                        busy <= 0;
                        tx_frame[0] <= 8'h5B;
                        tx_frame[1] <= pack4(i_cells[0], i_cells[1], i_cells[2], i_cells[3]);
                        tx_frame[2] <= pack4(i_cells[4], i_cells[5], do_values[9:8], do_values[7:6]);
                        tx_frame[3] <= pack4(do_values[5:4], i_cells[9], i_cells[10], do_values[1:0]);
                        tx_frame[4] <= {6'b0, do_values[3:2]};
                        tx_frame[5] <= {2'b0, do_status, de_status_latched, ds_status_latched};
                        tx_frame[6] <= {2'b0, do_areas, de_areas_latched, ds_areas_latched};
                        tx_frame[7] <= {5'b0, do_needs, de_needs_latched, ds_needs_latched};
                        tx_count <= 0;
                        tx_pending <= 1;
                        activity <= !activity;
                    end else begin
                        rounds <= rounds + 1'b1;
                        stage <= 0;
                    end
                end
            end
            if (!tx_busy && !tx_start && tx_pending) begin
                tx_data <= tx_frame[tx_count];
                tx_start <= 1;
                if (tx_count == TX_BYTES - 1) tx_pending <= 0;
                else tx_count <= tx_count + 1'b1;
            end
        end
    end
    assign leds = ~{activity, busy, rounds, stage};
endmodule
