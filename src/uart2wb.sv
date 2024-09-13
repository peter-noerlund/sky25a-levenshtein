`timescale 1ns/1ps
`default_nettype none

module uart2wb
    (
        input wire clk_i,
        input wire rst_i,

        //! @virtualbus UART @dir in UART
        input wire uart_rxd,
        output reg uart_txd,
        //! @end

        //! @virtualbus M_WB @dir output Wishbone master bus
        output reg cyc_o,
        output reg stb_o,
        output wire [22:0] adr_o,
        output wire [7:0] dat_o,
        output wire we_o,
        input wire ack_i,
        input wire err_i,
        input wire rty_i,
        input wire stall_i, // TODO
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

    reg [4:0] clk_counter;
    reg [31:0] buffer;
    reg [2:0] state;
    reg [1:0] byte_counter;
    reg [2:0] bit_counter;
    reg last_uart_rxd;

    assign we_o = buffer[31];
    assign adr_o = buffer[30:8];
    assign dat_o = buffer[7:0];

    always @ (posedge clk_i) begin
        if (rst_i) begin
            state <= STATE_READ_UART_SYNC;
            byte_counter <= 2'd0;
        end else begin
            case (state)
                STATE_READ_UART_SYNC: begin
                    //assert(clk_counter == 4'd0);
                    //assert(byte_counter < 2'd3);
                    if (last_uart_rxd && !uart_rxd) begin
                        state <= STATE_READ_UART_START_BIT;
                    end
                end

                STATE_READ_UART_START_BIT: begin
                    //assert(byte_counter < 2'd3);
                    if (clk_counter[3] == 1'b1) begin
                        //assert(clk_counter == 5'd8);
                        clk_counter[3] <= 1'b0;
                        state <= STATE_READ_UART_DATA_BITS;
                    end else begin
                        //assert(clk_counter < 5'd8);
                        clk_counter <= clk_counter + 5'd1;
                    end
                end

                STATE_READ_UART_DATA_BITS: begin
                    if (clk_counter[4] == 1'b1) begin
                        //assert(clk_counter == 5'd16);

                        clk_counter[4] <= 1'b0;
                        buffer <= {buffer[30:1], uart_rxd};
                        if (bit_counter == 3'd7) begin
                            state <= STATE_READ_UART_STOP_BIT;
                        end
                        bit_counter <= bit_counter + 3'd1;
                    end else begin
                        //assert(clk_counter < 5'd16);
                        clk_counter <= clk_counter + 5'd1;
                    end
                end

                STATE_READ_UART_STOP_BIT: begin
                    if (clk_counter[4] == 1'b1) begin
                        //assert(clk_counter == 5'd16);

                        if (uart_rxd == 1'b1) begin
                            if (byte_counter == 2'd3) begin
                                state <= STATE_WISHBONE;
                                cyc_o <= 1'b1;
                                stb_o <= 1'b1;
                            end
                            byte_counter <= byte_counter + 2'd1;
                        end else begin
                            state <= STATE_READ_UART_SYNC;
                            byte_counter <= 2'd0;
                        end
                    end else begin
                        //assert(clk_counter < 5'd16);
                        clk_counter <= clk_counter + 5'd1;
                    end
                end

                STATE_WISHBONE: begin
                    if (!stall_i) begin
                        stb_o <= 1'b0;
                    end
                    if (ack_i || err_i || rty_i) begin
                        //assert(uart_txd == 1'b1);
                        //assert(clk_counter == 5'd0);

                        if (we_o) begin
                            buffer[7:0] <= 8'h00;
                        end else begin
                            buffer[7:0] <= dat_i;
                        end
                        cyc_o <= 1'b0;
                        uart_txd <= 1'b0;
                        state <= STATE_WRITE_UART_START_BIT;
                    end
                end

                STATE_WRITE_UART_START_BIT: begin
                    if (clk_counter[4] == 1'b1) begin
                        //assert(clk_counter == 5'd16);
                        //assert(byte_counter == 3'd0);

                        clk_counter[4] <= 1'b0;

                        state <= STATE_WRITE_UART_DATA_BITS;
                    end else begin
                        clk_counter <= clk_counter + 5'd1;
                    end
                end

                STATE_WRITE_UART_DATA_BITS: begin
                    if (clk_counter[4] == 1'b1) begin
                        //assert(clk_counter == 5'd16);
                        
                        uart_txd <= buffer[7];
                        buffer[7:1] <= buffer[6:0];

                        if (bit_counter == 3'd7) begin
                            state <= STATE_WRITE_UART_STOP_BIT;
                            uart_txd <= 1'b1;
                        end
                        bit_counter <= bit_counter + 3'd1;
                    end else begin
                        clk_counter <= clk_counter + 5'd1;
                    end
                end

                STATE_WRITE_UART_STOP_BIT: begin
                    if (clk_counter[4] == 1'b1) begin
                        //assert (clk_counter == 5'd16);

                        clk_counter[4] <= 1'b0;

                        state <= STATE_READ_UART_SYNC;
                    end else begin
                        clk_counter <= clk_counter + 5'd1;
                    end
                end
            endcase
        end

        last_uart_rxd <= uart_rxd;
    end
endmodule