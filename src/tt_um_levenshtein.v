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
    assign uio_oe = 8'b00001011;
    assign uio_out[2] = 1'b0;
    assign uio_out[7:4] = 4'b0000;

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

    wire sram_cyc;
    wire sram_stb;
    wire [21:0] sram_adr;
    wire [7:0] sram_dwr;
    wire sram_we;
    wire sram_ack;
    wire sram_err;
    wire sram_rty;
    wire [7:0] sram_drd;

    /* verilator lint_off UNUSEDSIGNAL */
    wire sram_sel;
    wire [2:0] sram_cti;
    wire [1:0] sram_bte;
    /* verilator lint_on UNUSEDSIGNAL */

    wire ctrl_cyc;
    wire ctrl_stb;
    wire [7:0] ctrl_dwr;
    wire ctrl_we;
    wire ctrl_ack;
    wire ctrl_err;
    wire ctrl_rty;
    wire [7:0] ctrl_drd;

    /* verilator lint_off UNUSEDSIGNAL */
    wire [21:0] ctrl_adr;
    wire ctrl_sel;
    wire [2:0] ctrl_cti;
    wire [1:0] ctrl_bte;
    /* verilator lint_on UNUSEDSIGNAL */

    /* verilator lint_off UNUSEDSIGNAL */
    wire engine_enabled;
    wire [4:0] word_length;
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

    engine_controller ctrl(
        .clk_i(clk),
        .rst_i(!rst_n),

        .cyc_i(ctrl_cyc),
        .stb_i(ctrl_stb),
        .dat_i(ctrl_dwr),
        .we_i(ctrl_we),
        .ack_o(ctrl_ack),
        .err_o(ctrl_err),
        .rty_o(ctrl_rty),
        .dat_o(ctrl_drd),

        .enabled(engine_enabled),
        .word_length(word_length)
    );

    spi_controller sram(
        .clk_i(clk),
        .rst_i(!rst_n),

        .sck(uio_out[3]),
        .mosi(uio_out[1]),
        .miso(uio_in[2]),
        .ss_n(uio_out[0]),
        
        .cyc_i(sram_cyc),
        .stb_i(sram_stb),
        .adr_i({2'b00, sram_adr}),
        .dat_i(sram_dwr),
        .we_i(sram_we),
        .ack_o(sram_ack),
        .err_o(sram_err),
        .rty_o(sram_rty),
        .dat_o(sram_drd)
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

        .wbs0_cyc_o(ctrl_cyc),
        .wbs0_stb_o(ctrl_stb),
        .wbs0_adr_o(ctrl_adr),
        .wbs0_we_o(ctrl_we),
        .wbs0_sel_o(ctrl_sel),
        .wbs0_dat_o(ctrl_dwr),
        .wbs0_cti_o(ctrl_cti),
        .wbs0_bte_o(ctrl_bte),
        .wbs0_ack_i(ctrl_ack),
        .wbs0_err_i(ctrl_err),
        .wbs0_rty_i(ctrl_rty),
        .wbs0_dat_i(ctrl_drd),

        .wbs1_cyc_o(sram_cyc),
        .wbs1_stb_o(sram_stb),
        .wbs1_adr_o(sram_adr),
        .wbs1_we_o(sram_we),
        .wbs1_sel_o(sram_sel),
        .wbs1_dat_o(sram_dwr),
        .wbs1_cti_o(sram_cti),
        .wbs1_bte_o(sram_bte),
        .wbs1_ack_i(sram_ack),
        .wbs1_err_i(sram_err),
        .wbs1_rty_i(sram_rty),
        .wbs1_dat_i(sram_drd)
    );
endmodule
