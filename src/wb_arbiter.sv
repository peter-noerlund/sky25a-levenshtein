`default_nettype none

//! Wishbone 2:1 Arbiter
//!
//! Connects two masters to a wishbone bus.
//!
//! The master connected to WBS0 will always take priority over WBS1
module wb_arbiter
    #(
        parameter ADDR_WIDTH=24,
        parameter DATA_WIDTH=8,
        parameter SEL_WIDTH=DATA_WIDTH / 8
    )
    (
        input wire clk_i,   //! Clock
        input wire rst_i,   //! Reset

        //! @virtualbus WBS0 @dir in Wishbone Slave Port 0
        input wire wbs0_cyc_i,                          //! Cycle
        input wire wbs0_stb_i,                          //! Strobe
        input wire [ADDR_WIDTH - 1 : 0] wbs0_adr_i,     //! Address
        input wire wbs0_we_i,                           //! Write Enable
        input wire [SEL_WIDTH - 1 : 0] wbs0_sel_i,      //! Write Select
        input wire [DATA_WIDTH - 1 : 0] wbs0_dat_i,     //! Data In
        input wire [2:0] wbs0_cti_i,                    //! Cycle Type Indicator
        input wire [1:0] wbs0_bte_i,                    //! Burst Type Extension
        output wire wbs0_ack_o,                         //! Acknowledge
        output wire wbs0_rty_o,                         //! Retry
        output wire wbs0_err_o,                         //! Error
        output wire [DATA_WIDTH - 1 : 0] wbs0_dat_o,    //! Data Out
        //! @end

        //! @virtualbus WBS1 @dir in Wishbone Slave Port 1
        input wire wbs1_cyc_i,                          //! Cycle
        input wire wbs1_stb_i,                          //! Strobe
        input wire [ADDR_WIDTH - 1 : 0] wbs1_adr_i,     //! Address
        input wire wbs1_we_i,                           //! Write Enable
        input wire [SEL_WIDTH - 1 : 0] wbs1_sel_i,      //! Write Select
        input wire [DATA_WIDTH - 1 : 0] wbs1_dat_i,     //! Data In
        input wire [2:0] wbs1_cti_i,                    //! Cycle Type Indicator
        input wire [1:0] wbs1_bte_i,                    //! Burst Type Extension
        output wire wbs1_ack_o,                         //! Acknowledge
        output wire wbs1_rty_o,                         //! Retry
        output wire wbs1_err_o,                         //! Error
        output wire [DATA_WIDTH - 1 : 0] wbs1_dat_o,    //! Data Out
        //! @end
        
        //! @virtualbus WBM @dir out Wishbone Master Port
        output wire wbm_cyc_o,                          //! Cycle
        output wire wbm_stb_o,                          //! Strobe
        output wire [ADDR_WIDTH - 1 : 0] wbm_adr_o,     //! Address
        output wire wbm_we_o,                           //! Write Enable
        output wire [SEL_WIDTH - 1 : 0] wbm_sel_o,      //! Write Select
        output wire [DATA_WIDTH - 1: 0] wbm_dat_o,      //! Data Out
        output wire [2:0] wbm_cti_o,                    //! Cycle Type Indicator
        output wire [1:0] wbm_bte_o,                    //! Burst Type Indicator
        input wire wbm_ack_i,                           //! Acknowledge
        input wire wbm_rty_i,                           //! Retry
        input wire wbm_err_i,                           //! Error
        input wire [DATA_WIDTH - 1 : 0] wbm_dat_i       //! Data In
        //! @end
    );

    logic cyc;
    logic gnt;
    wire gnt0;
    wire gnt1;

    assign gnt0 = gnt;
    assign gnt1 = ~gnt;

    assign wbs0_ack_o = wbm_ack_i & gnt0;
    assign wbs0_err_o = wbm_err_i & gnt0;
    assign wbs0_rty_o = wbm_rty_i & gnt0;
    assign wbs0_dat_o = wbm_dat_i;

    assign wbs1_ack_o = wbm_ack_i & gnt1;
    assign wbs1_err_o = wbm_err_i & gnt1;
    assign wbs1_rty_o = wbm_rty_i & gnt1;
    assign wbs1_dat_o = wbm_dat_i;

    assign wbm_cyc_o = cyc;
    assign wbm_stb_o = gnt0 ? wbs0_stb_i : wbs1_stb_i;
    assign wbm_adr_o = gnt0 ? wbs0_adr_i : wbs1_adr_i;
    assign wbm_we_o = gnt0 ? wbs0_we_i : wbs1_we_i;
    assign wbm_dat_o = gnt0 ? wbs0_dat_i : wbs1_dat_i;
    assign wbm_sel_o = gnt0 ? wbs0_sel_i : wbs1_sel_i;
    assign wbm_cti_o = gnt0 ? wbs0_cti_i : wbs1_cti_i;
    assign wbm_bte_o = gnt0 ? wbs0_bte_i : wbs1_bte_i;

    always_ff @ (posedge clk_i) begin
        if (rst_i) begin
            cyc <= 1'b0;
            gnt <= 1'b0;
        end else begin
            if (cyc) begin
                cyc <= gnt0 & wbs0_cyc_i | gnt1 & wbs1_cyc_i;
            end else begin
                gnt <= wbs0_cyc_i;
                cyc <= wbs0_cyc_i | wbs1_cyc_i;
            end
        end
    end
endmodule
