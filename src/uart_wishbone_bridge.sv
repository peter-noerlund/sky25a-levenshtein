`default_nettype none

module uart_wishbone_bridge
    (
        input wire clk_i,
        input wire rst_i,

        //! @virtualbus UART @dir in UART
        input wire uart_rxd,
        output reg uart_txd,
        //! @end

        //! @virtualbus M_WB @dir output Wishbone master bus
        output wire cyc_o,
        output wire stb_o,
        output wire [22:0] adr_o,
        output wire [7:0] dat_o,
        output wire we_o,
        input wire ack_i,
        input wire err_i,
        input wire rty_i,
        input wire [7:0] dat_i
        //! @end
    );

    localparam STATE_READ_UART_SYNC = 3'd0;
    localparam STATE_READ_UART_START_BIT = 3'd1;
    localparam STATE_READ_UART_DATA_BITS = 3'd2;
    localparam STATE_READ_UART_STOP_BIT = 3'd3;
    localparam STATE_WISHBONE = 3'd4;
    localparam STATE_WRITE_UART_START_BIT = 3'd5;
    localparam STATE_WRITE_UART_DATA_BITS = 3'd6;
    localparam STATE_WRITE_UART_STOP_BIT = 3'd7;

    wire rxd;
    reg [1:0] uart_rxd_shift;
    reg [3:0] clk_counter;
    reg [31:0] buffer;
    reg [2:0] state;
    reg [1:0] byte_counter;
    reg [2:0] bit_counter;
    reg cyc;
    reg [1:0] rxd_sync;

    // Data is transmitted little endian

    assign we_o = buffer[24];
    assign adr_o = {
        buffer[25], buffer[26], buffer[27], buffer[28], buffer[29], buffer[30], buffer[31],
        buffer[16], buffer[17], buffer[18], buffer[19], buffer[20], buffer[21], buffer[22], buffer[23],
        buffer[8], buffer[9], buffer[10], buffer[11], buffer[12], buffer[13], buffer[14], buffer[15]
    };
    assign dat_o = {buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]};

    assign cyc_o = cyc;
    assign stb_o = cyc;
    assign rxd = uart_rxd_shift[1];

    assign rxd = rxd_sync[1];

    always @ (posedge clk_i) begin
        if (rst_i) begin
            state <= STATE_READ_UART_SYNC;
            bit_counter <= 3'd0;
            byte_counter <= 2'd0;
            clk_counter <= 4'd0;
            cyc <= 1'b0;
            buffer <= 32'h00000000;
            uart_txd <= 1'b1;
            uart_rxd_shift <= 2'b11;
        end else begin
            case (state)
                STATE_READ_UART_SYNC: begin
                    if (!rxd) begin
                        state <= STATE_READ_UART_START_BIT;
                    end
                end

                STATE_READ_UART_START_BIT: begin
                    if (clk_counter[2:0] == 3'd6) begin
                        clk_counter[2:0] <= 3'd0;
                        if (!rxd) begin
                            state <= STATE_READ_UART_DATA_BITS;
                        end else begin
                            state <= STATE_READ_UART_SYNC;
                        end
                    end else begin
                        clk_counter <= clk_counter + 4'd1;
                    end
                end

                STATE_READ_UART_DATA_BITS: begin
                    if (clk_counter == 4'd15) begin
                        buffer <= {buffer[30:0], rxd};
                        if (bit_counter == 3'd7) begin
                            state <= STATE_READ_UART_STOP_BIT;
                        end
                        bit_counter <= bit_counter + 3'd1;
                    end
                    clk_counter <= clk_counter + 4'd1;
                end

                STATE_READ_UART_STOP_BIT: begin
                    if (clk_counter == 4'd15) begin
                        if (!rxd) begin
                            state <= STATE_READ_UART_SYNC;
                        end else begin
                            if (byte_counter == 2'd3) begin
                                state <= STATE_WISHBONE;
                            end else begin
                                state <= STATE_READ_UART_SYNC;
                            end
                            byte_counter <= byte_counter + 2'd1;
                        end
                    end
                    clk_counter <= clk_counter + 4'd1;
                end

                STATE_WISHBONE: begin
                    if (clk_counter == 4'd7) begin
                        if (!cyc) begin
                            cyc <= 1'b1;
                        end else if (ack_i || err_i || rty_i) begin
                            if (we_o) begin
                                buffer[7:0] <= 8'h00;
                            end else begin
                                buffer[7:0] <= dat_i;
                            end
                            cyc <= 1'b0;
                            uart_txd <= 1'b0;
                            state <= STATE_WRITE_UART_START_BIT;
                            clk_counter <= 4'd0;
                        end
                    end else begin
                        clk_counter <= clk_counter + 4'd1;
                    end
                end

                STATE_WRITE_UART_START_BIT: begin
                    if (clk_counter == 4'd15) begin
                        state <= STATE_WRITE_UART_DATA_BITS;
                        uart_txd <= buffer[0];
                        buffer[6:0] <= buffer[7:1];
                    end
                    clk_counter <= clk_counter + 4'd1;
                end

                STATE_WRITE_UART_DATA_BITS: begin
                    if (clk_counter == 4'd15) begin
                        
                        uart_txd <= buffer[0];
                        buffer[6:0] <= buffer[7:1];

                        if (bit_counter == 3'd7) begin
                            state <= STATE_WRITE_UART_STOP_BIT;
                            uart_txd <= 1'b1;
                        end
                        bit_counter <= bit_counter + 3'd1;
                    end
                    clk_counter <= clk_counter + 4'd1;
                end

                STATE_WRITE_UART_STOP_BIT: begin
                    if (clk_counter == 4'd15) begin
                        state <= STATE_READ_UART_SYNC;
                    end
                    clk_counter <= clk_counter + 4'd1;
                end
            endcase

            uart_rxd_shift <= {uart_rxd_shift[0], uart_rxd};
        end

        rxd_sync <= {rxd_sync[0], uart_rxd};
    end
endmodule
