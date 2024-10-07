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
    
    assign uio_oe[0] = 1'b1;        // PMOD expanded SPI (CS)
    assign uio_oe[3] = 1'b1;        // PMOD expanded SPI (SCK)
    assign uio_oe[7:6] = 2'b11;     // PMOD expanded SPI (CS2 + CS3)

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
    wire sram_we;
    wire [7:0] sram_dwr;
    wire [2:0] sram_cti;
    wire [1:0] sram_bte;
    wire sram_ack;
    wire sram_err;
    wire sram_rty;
    wire [7:0] sram_drd;

    wire [1:0] sram_config;

    /* verilator lint_off UNUSEDSIGNAL */
    wire sram_sel;
    /* verilator lint_on UNUSEDSIGNAL */

    wire spi_sram_cyc;
    wire spi_sram_stb;
    wire [22:0] spi_sram_adr;
    wire spi_sram_we;
    wire spi_sram_sel;
    wire [7:0] spi_sram_dwr;
    wire [2:0] spi_sram_cti;
    wire [1:0] spi_sram_bte;
    wire spi_sram_ack;
    wire spi_sram_rty;
    wire spi_sram_err;
    wire [7:0] spi_sram_drd;

    wire ctrl_master_cyc;
    wire ctrl_master_stb;
    wire [22:0] ctrl_master_adr;
    wire ctrl_master_we;
    wire [7:0] ctrl_master_dwr;
    wire [2:0] ctrl_master_cti;
    wire [1:0] ctrl_master_bte;
    wire ctrl_master_ack;
    wire ctrl_master_err;
    wire ctrl_master_rty;
    wire [7:0] ctrl_master_drd;
    /* verilator lint_off UNUSEDSIGNAL */
    wire ctrl_master_sel;
    /* verilator lint_on UNUSEDSIGNAL */

    wire ctrl_slave_cyc;
    wire ctrl_slave_stb;
    wire [2:0] ctrl_slave_adr;
    wire ctrl_slave_we;
    wire [7:0] ctrl_slave_dwr;
    wire [2:0] ctrl_slave_cti;
    wire [1:0] ctrl_slave_bte;
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

    levenshtein_controller #(.MASTER_ADDR_WIDTH(23), .SLAVE_ADDR_WIDTH(3), .BITVECTOR_WIDTH(16)) levenshtein_ctrl (
        .clk_i(clk),
        .rst_i(!rst_n),

        .wbm_cyc_o(ctrl_master_cyc),
        .wbm_stb_o(ctrl_master_stb),
        .wbm_adr_o(ctrl_master_adr),
        .wbm_we_o(ctrl_master_we),
        .wbm_dat_o(ctrl_master_dwr),
        .wbm_cti_o(ctrl_master_cti),
        .wbm_bte_o(ctrl_master_bte),
        .wbm_ack_i(ctrl_master_ack),
        .wbm_err_i(ctrl_master_err),
        .wbm_rty_i(ctrl_master_rty),
        .wbm_dat_i(ctrl_master_drd),

        .wbs_cyc_i(ctrl_slave_cyc),
        .wbs_stb_i(ctrl_slave_stb),
        .wbs_adr_i(ctrl_slave_adr),
        .wbs_we_i(ctrl_slave_we),
        .wbs_dat_i(ctrl_slave_dwr),
        .wbs_cti_i(ctrl_slave_cti),
        .wbs_bte_i(ctrl_slave_bte),
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
        .cti_i(sram_cti),
        .bte_i(sram_bte),
        .ack_o(sram_ack),
        .err_o(sram_err),
        .rty_o(sram_rty),
        .dat_o(sram_drd),

        .sck(uio_out[3]),
        .sio_in({uio_in[5:4], uio_in[2:1]}),
        .sio_out({uio_out[5:4], uio_out[2:1]}),
        .sio_oe({uio_oe[5:4], uio_oe[2:1]}),
        .cs_n(uio_out[0]),
        .cs2_n(uio_out[6]),
        .cs3_n(uio_out[7]),

        .sram_config(sram_config)
    );

    wb_interconnect #(.ADDR_WIDTH(23), .SHARED_BUS(0)) intercon(
        .wbs_cyc_i(spi_cyc),
        .wbs_stb_i(spi_stb),
        .wbs_adr_i(spi_adr),
        .wbs_we_i(spi_we),
        .wbs_sel_i(1'b0),
        .wbs_dat_i(spi_dwr),
        .wbs_cti_i(3'b000),
        .wbs_bte_i(2'b00),
        .wbs_ack_o(spi_ack),
        .wbs_rty_o(spi_rty),
        .wbs_err_o(spi_err),
        .wbs_dat_o(spi_drd),

        .wbm0_cyc_o(ctrl_slave_cyc),
        .wbm0_stb_o(ctrl_slave_stb),
        .wbm0_adr_o(ctrl_slave_adr),
        .wbm0_we_o(ctrl_slave_we),
        .wbm0_sel_o(ctrl_slave_sel),
        .wbm0_dat_o(ctrl_slave_dwr),
        .wbm0_cti_o(ctrl_slave_cti),
        .wbm0_bte_o(ctrl_slave_bte),
        .wbm0_ack_i(ctrl_slave_ack),
        .wbm0_rty_i(ctrl_slave_rty),
        .wbm0_err_i(ctrl_slave_err),
        .wbm0_dat_i(ctrl_slave_drd),

        .wbm1_cyc_o(spi_sram_cyc),
        .wbm1_stb_o(spi_sram_stb),
        .wbm1_adr_o(spi_sram_adr),
        .wbm1_we_o(spi_sram_we),
        .wbm1_sel_o(spi_sram_sel),
        .wbm1_dat_o(spi_sram_dwr),
        .wbm1_cti_o(spi_sram_cti),
        .wbm1_bte_o(spi_sram_bte),
        .wbm1_ack_i(spi_sram_ack),
        .wbm1_rty_i(spi_sram_rty),
        .wbm1_err_i(spi_sram_err),
        .wbm1_dat_i(spi_sram_drd)
    );
    
    wb_arbiter #(.ADDR_WIDTH(23)) arbiter(
        .clk_i(clk),
        .rst_i(!rst_n),

        .wbs0_cyc_i(spi_sram_cyc),
        .wbs0_stb_i(spi_sram_stb),
        .wbs0_adr_i(spi_sram_adr),
        .wbs0_we_i(spi_sram_we),
        .wbs0_sel_i(spi_sram_sel),
        .wbs0_dat_i(spi_sram_dwr),
        .wbs0_cti_i(spi_sram_cti),
        .wbs0_bte_i(spi_sram_bte),
        .wbs0_ack_o(spi_sram_ack),
        .wbs0_err_o(spi_sram_err),
        .wbs0_rty_o(spi_sram_rty),
        .wbs0_dat_o(spi_sram_drd),

        .wbs1_cyc_i(ctrl_master_cyc),
        .wbs1_stb_i(ctrl_master_stb),
        .wbs1_adr_i(ctrl_master_adr),
        .wbs1_we_i(ctrl_master_we),
        .wbs1_sel_i(1'b0),
        .wbs1_dat_i(ctrl_master_dwr),
        .wbs1_cti_i(ctrl_master_cti),
        .wbs1_bte_i(ctrl_master_bte),
        .wbs1_ack_o(ctrl_master_ack),
        .wbs1_err_o(ctrl_master_err),
        .wbs1_rty_o(ctrl_master_rty),
        .wbs1_dat_o(ctrl_master_drd),

        .wbm_cyc_o(sram_cyc),
        .wbm_stb_o(sram_stb),
        .wbm_adr_o(sram_adr),
        .wbm_we_o(sram_we),
        .wbm_sel_o(sram_sel),
        .wbm_dat_o(sram_dwr),
        .wbm_cti_o(sram_cti),
        .wbm_bte_o(sram_bte),
        .wbm_ack_i(sram_ack),
        .wbm_rty_i(sram_rty),
        .wbm_err_i(sram_err),
        .wbm_dat_i(sram_drd)
    );
endmodule
