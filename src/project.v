/*
 * Copyright (c) 2024 Your Name
 * SPDX-License-Identifier: Apache-2.0
 */

`default_nettype none

module tt_um_levenshtein
    (
        input  wire [7:0] ui_in,    // Dedicated inputs
        output wire [7:0] uo_out,   // Dedicated outputs
        input  wire [7:0] uio_in,   // IOs: Input path
        output wire [7:0] uio_out,  // IOs: Output path
        output wire [7:0] uio_oe,   // IOs: Enable path (active high: 0=input, 1=output)
        input  wire       ena,      // always 1 when the design is powered, so you can ignore it
        input  wire       clk,      // clock
        input  wire       rst_n     // reset_n - low to reset
    );

    assign uio_oe = 8'b00000000;
    assign uio_out = 8'b00000000;
    assign uo_out[7:5] = 3'b000;
    assign uo_out[3:0] = 4'b0000;

    wire cyc_o;
    wire stb_o;
    wire [22:0] adr_o;
    wire [7:0] dat_o;
    wire we_o;
    wire ack_i;
    wire err_i;
    wire rty_i;
    wire stall_i;
    wire [7:0] dat_i;

    uart2wb uart(
        .clk_i(clk),
        .rst_i(!rst_n),
        .uart_rxd(ui_in[3]),
        .uart_txd(uo_out[4]),

        .cyc_o(cyc_o),
        .stb_o(stb_o),
        .adr_o(adr_o),
        .dat_o(dat_o),
        .we_o(we_o),
        .ack_i(ack_i),
        .err_i(err_i),
        .rty_i(rty_i),
        .stall_i(stall_i),
        .dat_i(dat_i)
    );

    wb2sram #(.ADDR_WIDTH(2)) sram(
        .clk_i(clk),
        .rst_i(!rst_n),
        
        .cyc_i(cyc_o),
        .stb_i(stb_o),
        .adr_i(adr_o[1:0]),
        .dat_i(dat_o),
        .we_i(we_o),
        .ack_o(ack_i),
        .err_o(err_i),
        .rty_o(rty_i),
        .stall_o(stall_i),
        .dat_o(dat_i)
    );
endmodule
