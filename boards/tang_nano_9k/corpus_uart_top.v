// PC-facing CORPUS interface. Request frame: A5, values low, values high,
// allowed mask. Response frame: 5A followed by the 24-bit core response.
module corpus_uart_top #(parameter integer CLOCK_HZ = 27000000,
                         parameter integer BAUD = 115200) (
    input wire clk,
    input wire uart_rx,
    output wire uart_tx,
    output wire [5:0] leds
);
    localparam integer CLKS_PER_BIT = CLOCK_HZ / BAUD;
    reg [2:0] startup = 0;
    reg [1:0] rx_count = 0;
    reg [9:0] request_values = 0;
    reg [7:0] request_allowed = 0;
    reg request_valid = 0;
    reg [31:0] tx_frame = 0;
    reg [2:0] tx_count = 0;
    reg tx_pending = 0;
    reg tx_start = 0;
    reg [7:0] tx_data = 0;
    reg activity = 0;
    wire rst = startup != 7;
    wire [7:0] rx_data;
    wire rx_valid;
    wire tx_busy;
    wire request_ready;
    wire response_valid;
    wire [9:0] response_values;
    wire [1:0] response_status, response_areas;
    wire [7:0] response_support;
    wire response_needs_base_refinement, response_direct;
    wire [23:0] response = {response_values, response_status, response_areas,
        response_support, response_needs_base_refinement, response_direct};

    uart_rx #(.CLKS_PER_BIT(CLKS_PER_BIT)) rx_unit (
        .clk(clk), .rx(uart_rx), .data(rx_data), .valid(rx_valid));
    uart_tx #(.CLKS_PER_BIT(CLKS_PER_BIT)) tx_unit (
        .clk(clk), .data(tx_data), .start(tx_start), .tx(uart_tx), .busy(tx_busy));
    trigate_core core (
        .clk(clk), .rst(rst), .request_valid(request_valid),
        .request_ready(request_ready), .request_values(request_values),
        .request_allowed(request_allowed), .response_valid(response_valid),
        .response_ready(1'b1), .response_values(response_values),
        .response_status(response_status), .response_areas(response_areas),
        .response_support(response_support),
        .response_needs_base_refinement(response_needs_base_refinement),
        .response_direct(response_direct));

    always @(posedge clk) begin
        tx_start <= 0;
        request_valid <= 0;
        if (rst) begin
            startup <= startup + 1'b1;
            rx_count <= 0;
            tx_count <= 0;
            tx_pending <= 0;
            tx_frame <= 0;
            activity <= 0;
        end else begin
            if (rx_valid) begin
                if (rx_count == 0) begin
                    if (rx_data == 8'hA5) rx_count <= 1;
                end else if (rx_count == 1) begin
                    request_values[7:0] <= rx_data;
                    rx_count <= 2;
                end else if (rx_count == 2) begin
                    request_values[9:8] <= rx_data[1:0];
                    rx_count <= 3;
                end else begin
                    request_allowed <= rx_data;
                    request_valid <= 1;
                    rx_count <= 0;
                end
            end
            if (response_valid) begin
                tx_frame <= {8'h5A, response};
                tx_count <= 0;
                tx_pending <= 1;
                activity <= !activity;
            end
            if (!tx_busy && !tx_start && tx_pending) begin
                tx_data <= tx_frame[31:24];
                tx_frame <= {tx_frame[23:0], 8'h00};
                tx_count <= tx_count + 1'b1;
                if (tx_count == 3) tx_pending <= 0;
                tx_start <= 1;
            end
        end
    end

    assign leds = ~{activity, 1'b1, 4'b0000};
endmodule