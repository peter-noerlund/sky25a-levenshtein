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

    assign uo_out[6:0] = 7'b0000000;
    assign uio_oe = 8'b11001011;
    assign uio_out[2] = 1'b0;
    assign uio_out[5:4] = 2'b00;

    wire spi_cyc;
    wire spi_stb;
    wire [22:0] spi_adr;
    wire [7:0] spi_dwr;
    wire spi_we;
    wire spi_ack;
    wire spi_err;
    wire spi_rty;
    wire [7:0] spi_drd;

    wire sram_cyc;
    wire sram_stb;
    wire [22:0] sram_adr;
    wire [7:0] sram_dwr;
    wire sram_we;
    wire sram_ack;
    wire sram_err;
    wire sram_rty;
    wire [7:0] sram_drd;

    wire [1:0] sram_config;

    /* verilator lint_off UNUSEDSIGNAL */
    wire sram_sel;
    /* verilator lint_on UNUSEDSIGNAL */

    wire ctrl_master_cyc;
    wire ctrl_master_stb;
    wire [22:0] ctrl_master_adr;
    wire ctrl_master_we;
    wire [7:0] ctrl_master_dwr;
    wire ctrl_master_ack;
    wire ctrl_master_err;
    wire ctrl_master_rty;
    wire [7:0] ctrl_master_drd;
    /* verilator lint_off UNUSEDSIGNAL */
    wire ctrl_master_sel;
    /* verilator lint_on UNUSEDSIGNAL */

    wire ctrl_slave_cyc;
    wire ctrl_slave_stb;
    wire [22:0] ctrl_slave_adr;
    wire ctrl_slave_we;
    wire [7:0] ctrl_slave_dwr;
    wire ctrl_slave_ack;
    wire ctrl_slave_err;
    wire ctrl_slave_rty;
    wire [7:0] ctrl_slave_drd;
    /* verilator lint_off UNUSEDSIGNAL */
    wire ctrl_slave_sel;
    /* verilator lint_on UNUSEDSIGNAL */

    spi_wishbone_bridge spi(
        .clk_i(clk),
        .rst_i(!rst_n),

        .spi_ss_n(ui_in[4]),
        .spi_sck(ui_in[5]),
        .spi_mosi(ui_in[6]),
        .spi_miso(uo_out[7]),

        .cyc_o(spi_cyc),
        .stb_o(spi_stb),
        .adr_o(spi_adr),
        .dat_o(spi_dwr),
        .we_o(spi_we),
        .ack_i(spi_ack),
        .err_i(spi_err),
        .rty_i(spi_rty),
        .dat_i(spi_drd)
    );

    levenshtein_controller #(.MASTER_ADDR_WIDTH(23), .SLAVE_ADDR_WIDTH(23)) levenshtein_ctrl (
        .clk_i(clk),
        .rst_i(!rst_n),

        .wbm_cyc_o(ctrl_master_cyc),
        .wbm_stb_o(ctrl_master_stb),
        .wbm_adr_o(ctrl_master_adr),
        .wbm_we_o(ctrl_master_we),
        .wbm_dat_o(ctrl_master_dwr),
        .wbm_ack_i(ctrl_master_ack),
        .wbm_err_i(ctrl_master_err),
        .wbm_rty_i(ctrl_master_rty),
        .wbm_dat_i(ctrl_master_drd),

        .wbs_cyc_i(ctrl_slave_cyc),
        .wbs_stb_i(ctrl_slave_stb),
        .wbs_adr_i(ctrl_slave_adr),
        .wbs_we_i(ctrl_slave_we),
        .wbs_dat_i(ctrl_slave_dwr),
        .wbs_ack_o(ctrl_slave_ack),
        .wbs_err_o(ctrl_slave_err),
        .wbs_rty_o(ctrl_slave_rty),
        .wbs_dat_o(ctrl_slave_drd),

        .sram_config(sram_config)
    );

    spi_controller spi_ctrl(
        .clk_i(clk),
        .rst_i(!rst_n),

        .cyc_i(sram_cyc),
        .stb_i(sram_stb),
        .adr_i({1'b0, sram_adr}),
        .dat_i(sram_dwr),
        .we_i(sram_we),
        .ack_o(sram_ack),
        .err_o(sram_err),
        .rty_o(sram_rty),
        .dat_o(sram_drd),

        .sck(uio_out[3]),
        .mosi(uio_out[1]),
        .miso(uio_in[2]),
        .cs_n(uio_out[0]),
        .cs2_n(uio_out[6]),
        .cs3_n(uio_out[7]),

        .sram_config(sram_config)
    );


    wb_interconnect #(.ADDR_WIDTH(23)) intercon(
        .clk_i(clk),
        .rst_i(!rst_n),

        .wbm0_cyc_i(spi_cyc),
        .wbm0_stb_i(spi_stb),
        .wbm0_adr_i(spi_adr),
        .wbm0_we_i(spi_we),
        .wbm0_sel_i(1'b0),
        .wbm0_dat_i(spi_dwr),
        .wbm0_ack_o(spi_ack),
        .wbm0_err_o(spi_err),
        .wbm0_rty_o(spi_rty),
        .wbm0_dat_o(spi_drd),

        .wbm1_cyc_i(ctrl_master_cyc),
        .wbm1_stb_i(ctrl_master_stb),
        .wbm1_adr_i(ctrl_master_adr),
        .wbm1_we_i(ctrl_master_we),
        .wbm1_sel_i(1'b0),
        .wbm1_dat_i(ctrl_master_dwr),
        .wbm1_ack_o(ctrl_master_ack),
        .wbm1_err_o(ctrl_master_err),
        .wbm1_rty_o(ctrl_master_rty),
        .wbm1_dat_o(ctrl_master_drd),

        .wbs0_cyc_o(ctrl_slave_cyc),
        .wbs0_stb_o(ctrl_slave_stb),
        .wbs0_adr_o(ctrl_slave_adr),
        .wbs0_we_o(ctrl_slave_we),
        .wbs0_sel_o(ctrl_slave_sel),
        .wbs0_dat_o(ctrl_slave_dwr),
        .wbs0_ack_i(ctrl_slave_ack),
        .wbs0_err_i(ctrl_slave_err),
        .wbs0_rty_i(ctrl_slave_rty),
        .wbs0_dat_i(ctrl_slave_drd),

        .wbs1_cyc_o(sram_cyc),
        .wbs1_stb_o(sram_stb),
        .wbs1_adr_o(sram_adr),
        .wbs1_we_o(sram_we),
        .wbs1_sel_o(sram_sel),
        .wbs1_dat_o(sram_dwr),
        .wbs1_ack_i(sram_ack),
        .wbs1_err_i(sram_err),
        .wbs1_rty_i(sram_rty),
        .wbs1_dat_i(sram_drd)
    );
endmodule
