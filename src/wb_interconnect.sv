`default_nettype none

module wb_interconnect
    #(
        parameter ADDR_WIDTH=24,
        parameter DATA_WIDTH=8,
        parameter SEL_WIDTH=DATA_WIDTH / 8
    )
    (
        input wire clk_i,
        input wire rst_i,

        //! @virtualbus WBM0 @dir in Wishbone Master 0
        input wire wbm0_cyc_i,                          //! Cycle
        input wire wbm0_stb_i,                          //! Strobe
        input wire [ADDR_WIDTH - 1 : 0] wbm0_adr_i,     //! Address
        input wire wbm0_we_i,                           //! Write Enable
        input wire [SEL_WIDTH - 1 : 0] wbm0_sel_i,      //! Write Select
        input wire [DATA_WIDTH - 1 : 0] wbm0_dat_i,     //! Data In
        output wire wbm0_ack_o,                         //! Acknowledge
        output wire wbm0_err_o,                         //! Error
        output wire wbm0_rty_o,                         //! Retry
        output wire [DATA_WIDTH - 1 : 0] wbm0_dat_o,    //! Data Out
        //! @end

        //! @virtualbus WBM1 @dir in Wishbone Master 1
        input wire wbm1_cyc_i,                          //! Cycle
        input wire wbm1_stb_i,                          //! Strobe
        input wire [ADDR_WIDTH - 1 : 0] wbm1_adr_i,     //! Address
        input wire wbm1_we_i,                           //! Write Enable
        input wire [SEL_WIDTH - 1 : 0] wbm1_sel_i,      //! Write Select
        input wire [DATA_WIDTH - 1 : 0] wbm1_dat_i,     //! Data In
        output wire wbm1_ack_o,                         //! Acknowledge
        output wire wbm1_err_o,                         //! Error
        output wire wbm1_rty_o,                         //! Retry
        output wire [DATA_WIDTH - 1 : 0] wbm1_dat_o,    //! Data Out
        //! @end

        //! @virtualbus WBS0 @dir out Wishbone Slave 0
        output wire wbs0_cyc_o,                         //! Cycle
        output wire wbs0_stb_o,                         //! Strobe
        output wire [ADDR_WIDTH - 2 : 0] wbs0_adr_o,    //! Address
        output wire wbs0_we_o,                          //! Write Enable
        output wire [SEL_WIDTH - 1 : 0] wbs0_sel_o,     //! Write Select
        output wire [DATA_WIDTH - 1 : 0] wbs0_dat_o,    //! Data Out
        input wire wbs0_ack_i,                          //! Acknowledge
        input wire wbs0_err_i,                          //! Error
        input wire wbs0_rty_i,                          //! Retry
        input wire [DATA_WIDTH - 1 : 0] wbs0_dat_i,     //! Data In
        //! @end

        //! @virtualbus WBS1 @dir out Wishbone Slave 1
        output wire wbs1_cyc_o,                         //! Cycle
        output wire wbs1_stb_o,                         //! Strobe
        output wire [ADDR_WIDTH - 2 : 0] wbs1_adr_o,    //! Address
        output wire wbs1_we_o,                          //! Write Enable
        output wire [SEL_WIDTH - 1 : 0] wbs1_sel_o,     //! Write Select
        output wire [DATA_WIDTH - 1 : 0] wbs1_dat_o,    //! Data Out
        input wire wbs1_ack_i,                          //! Acknowledge
        input wire wbs1_err_i,                          //! Error
        input wire wbs1_rty_i,                          //! Retry
        input wire [DATA_WIDTH - 1 : 0] wbs1_dat_i      //! Data In
        //! @end        
    );

    wire gnt;
    wire gnt0;
    wire gnt1;

    wire ack;
    wire err;
    wire rty;
    wire [DATA_WIDTH - 1 : 0] drd;

    wire cyc;
    wire stb;
    wire we;
    wire [SEL_WIDTH - 1 : 0] sel;
    wire [DATA_WIDTH - 1 : 0] dwr;
    wire [ADDR_WIDTH - 1: 0] adr;

    wire acmp0 = adr[ADDR_WIDTH - 1] == 1'b0;
    wire acmp1 = adr[ADDR_WIDTH - 1] == 1'b1;

    assign gnt0 = gnt == 0;
    assign gnt1 = gnt == 1;

    assign wbm0_ack_o = ack & gnt0;
    assign wbm0_err_o = err & gnt0;
    assign wbm0_rty_o = rty & gnt0;
    assign wbm0_dat_o = drd;

    assign wbm1_ack_o = ack & gnt1;
    assign wbm1_err_o = err & gnt1;
    assign wbm1_rty_o = rty & gnt1;
    assign wbm1_dat_o = drd;

    assign wbs0_cyc_o = cyc & acmp0;
    assign wbs0_stb_o = stb & acmp0;
    assign wbs0_adr_o = adr[ADDR_WIDTH - 2 : 0];
    assign wbs0_we_o = we;
    assign wbs0_sel_o = sel;
    assign wbs0_dat_o = dwr;

    assign wbs1_cyc_o = cyc & acmp1;
    assign wbs1_stb_o = stb & acmp1;
    assign wbs1_adr_o = adr[ADDR_WIDTH - 2 : 0];
    assign wbs1_we_o = we;
    assign wbs1_sel_o = sel;
    assign wbs1_dat_o = dwr;

    assign ack = wbs0_ack_i || wbs1_ack_i;
    assign err = wbs0_err_i || wbs1_err_i;
    assign rty = wbs0_rty_i || wbs1_rty_i;
    assign drd = acmp0 ? wbs0_dat_i : wbs1_dat_i;

    assign stb = gnt0 ? wbm0_stb_i : wbm1_stb_i;
    assign adr = gnt0 ? wbm0_adr_i : wbm1_adr_i;
    assign we = gnt0 ? wbm0_we_i : wbm1_we_i;
    assign sel = gnt0 ? wbm0_sel_i : wbm1_sel_i;
    assign dwr = gnt0 ? wbm0_dat_i : wbm1_dat_i;

    wb_arbiter #(.MASTER_COUNT(2)) arbiter(
        .clk_i(clk_i),
        .rst_i(rst_i),

        .cyc_i({wbm1_cyc_i, wbm0_cyc_i}),
        .gnt_o(gnt),
        .cyc_o(cyc)
    );
endmodule
