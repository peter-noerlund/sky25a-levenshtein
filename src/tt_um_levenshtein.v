/*
 * Copyright (c) 2024 Your Name
 * SPDX-License-Identifier: Apache-2.0
 */

`default_nettype none

module tt_um_levenshtein
    /* verilator lint_off UNUSEDSIGNAL */
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
    /* verilator lint_on UNUSEDSIGNAL */

    assign uo_out[7:5] = 3'b000;
    assign uo_out[3:1] = 3'b000;
    assign uio_oe = 8'b10111011;
    assign uio_out[2] = 1'b0;
    assign uio_out[6] = 1'b0;

    wire uart1_cyc;
    wire uart1_stb;
    wire [22:0] uart1_adr;
    wire [7:0] uart1_dwr;
    wire uart1_we;
    wire uart1_ack;
    wire uart1_err;
    wire uart1_rty;
    wire [7:0] uart1_drd;

    wire uart2_cyc;
    wire uart2_stb;
    wire [22:0] uart2_adr;
    wire [7:0] uart2_dwr;
    wire uart2_we;
    wire uart2_ack;
    wire uart2_err;
    wire uart2_rty;
    wire [7:0] uart2_drd;

    wire sram1_cyc;
    wire sram1_stb;
    wire [21:0] sram1_adr;
    wire [7:0] sram1_dwr;
    wire sram1_we;
    wire sram1_ack;
    wire sram1_err;
    wire sram1_rty;

    /* verilator lint_off UNUSEDSIGNAL */
    wire sram1_sel;
    wire [7:0] sram1_drd;
    wire [2:0] sram1_cti;
    wire [1:0] sram1_bte;
    /* verilator lint_on UNUSEDSIGNAL */

    wire sram2_cyc;
    wire sram2_stb;
    wire [21:0] sram2_adr;
    wire [7:0] sram2_dwr;
    wire sram2_we;
    wire sram2_ack;
    wire sram2_err;
    wire sram2_rty;

    /* verilator lint_off UNUSEDSIGNAL */
    wire sram2_sel;
    wire [7:0] sram2_drd;
    wire [2:0] sram2_cti;
    wire [1:0] sram2_bte;
    /* verilator lint_on UNUSEDSIGNAL */

    uart2wb uart1(
        .clk_i(clk),
        .rst_i(!rst_n),

        .uart_rxd(ui_in[3]),
        .uart_txd(uo_out[4]),

        .cyc_o(uart1_cyc),
        .stb_o(uart1_stb),
        .adr_o(uart1_adr),
        .dat_o(uart1_dwr),
        .we_o(uart1_we),
        .ack_i(uart1_ack),
        .err_i(uart1_err),
        .rty_i(uart1_rty),
        .dat_i(uart1_drd)
    );

    uart2wb uart2(
        .clk_i(clk),
        .rst_i(!rst_n),

        .uart_rxd(ui_in[7]),
        .uart_txd(uo_out[0]),

        .cyc_o(uart2_cyc),
        .stb_o(uart2_stb),
        .adr_o(uart2_adr),
        .dat_o(uart2_dwr),
        .we_o(uart2_we),
        .ack_i(uart2_ack),
        .err_i(uart2_err),
        .rty_i(uart2_rty),
        .dat_i(uart2_drd)
    );

    spi_controller sram1(
        .clk_i(clk),
        .rst_i(!rst_n),

        .sck(uio_out[3]),
        .mosi(uio_out[1]),
        .miso(uio_in[2]),
        .ss_n(uio_out[0]),
        
        .cyc_i(sram1_cyc),
        .stb_i(sram1_stb),
        .adr_i({2'b00, sram1_adr}),
        .dat_i(sram1_dwr),
        .we_i(sram1_we),
        .ack_o(sram1_ack),
        .err_o(sram1_err),
        .rty_o(sram1_rty),
        .dat_o(sram1_drd)
    );

    spi_controller sram2(
        .clk_i(clk),
        .rst_i(!rst_n),

        .sck(uio_out[7]),
        .mosi(uio_out[5]),
        .miso(uio_in[6]),
        .ss_n(uio_out[4]),
        
        .cyc_i(sram2_cyc),
        .stb_i(sram2_stb),
        .adr_i({2'b00, sram2_adr}),
        .dat_i(sram2_dwr),
        .we_i(sram2_we),
        .ack_o(sram2_ack),
        .err_o(sram2_err),
        .rty_o(sram2_rty),
        .dat_o(sram2_drd)
    );

    wb_interconnect #(.ADDR_WIDTH(23)) intercon(
        .clk_i(clk),
        .rst_i(!rst_n),

        .wbm0_cyc_i(uart1_cyc),
        .wbm0_stb_i(uart1_stb),
        .wbm0_adr_i(uart1_adr),
        .wbm0_we_i(uart1_we),
        .wbm0_sel_i(1'b0),
        .wbm0_dat_i(uart1_dwr),
        .wbm0_cti_i(3'b000),
        .wbm0_bte_i(2'b00),
        .wbm0_ack_o(uart1_ack),
        .wbm0_err_o(uart1_err),
        .wbm0_rty_o(uart1_rty),
        .wbm0_dat_o(uart1_drd),

        .wbm1_cyc_i(uart2_cyc),
        .wbm1_stb_i(uart2_stb),
        .wbm1_adr_i(uart2_adr),
        .wbm1_we_i(uart2_we),
        .wbm1_sel_i(1'b0),
        .wbm1_dat_i(uart2_dwr),
        .wbm1_cti_i(3'b000),
        .wbm1_bte_i(2'b00),
        .wbm1_ack_o(uart2_ack),
        .wbm1_err_o(uart2_err),
        .wbm1_rty_o(uart2_rty),
        .wbm1_dat_o(uart2_drd),

        .wbs0_cyc_o(sram1_cyc),
        .wbs0_stb_o(sram1_stb),
        .wbs0_adr_o(sram1_adr),
        .wbs0_we_o(sram1_we),
        .wbs0_sel_o(sram1_sel),
        .wbs0_dat_o(sram1_dwr),
        .wbs0_cti_o(sram1_cti),
        .wbs0_bte_o(sram1_bte),
        .wbs0_ack_i(sram1_ack),
        .wbs0_err_i(sram1_err),
        .wbs0_rty_i(sram1_rty),
        .wbs0_dat_i(sram1_drd),

        .wbs1_cyc_o(sram2_cyc),
        .wbs1_stb_o(sram2_stb),
        .wbs1_adr_o(sram2_adr),
        .wbs1_we_o(sram2_we),
        .wbs1_sel_o(sram2_sel),
        .wbs1_dat_o(sram2_dwr),
        .wbs1_cti_o(sram2_cti),
        .wbs1_bte_o(sram2_bte),
        .wbs1_ack_i(sram2_ack),
        .wbs1_err_i(sram2_err),
        .wbs1_rty_i(sram2_rty),
        .wbs1_dat_i(sram2_drd)
    );
endmodule
