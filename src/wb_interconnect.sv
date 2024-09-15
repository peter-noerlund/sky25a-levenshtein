`default_nettype none

module wb_interconnect
    #(
        parameter ADDR_WIDTH=24,
        parameter DATA_WIDTH=8,
        parameter SEL_WIDTH=DATA_WIDTH / 8
    )
    (
        input wire clk_i,

        //! @virtualbus WBM @dir in Wishbone Master
        input wire wbm_cyc_i,                           //! Cycle
        input wire wbm_stb_i,                           //! Strobe
        input wire [ADDR_WIDTH - 1 : 0] wbm_adr_i,      //! Address
        input wire wbm_we_i,                            //! Write Enable
        input wire [DATA_WIDTH - 1 : 0] wbm_dat_i,      //! Data In
        input wire [SEL_WIDTH - 1 : 0] wbm_sel_i,       //! Write Select
        input wire [2:0] wbm_cti_i,                     //! Cycle Type Indicator
        input wire [1:0] wbm_bte_i,                     //! Burst Type Extension
        output reg wbm_ack_o,                          //! Acknowledge
        output reg wbm_err_o,                          //! Error
        output reg wbm_rty_o,                          //! Retry
        output wire wbm_stall_o,                        //! Stall
        output reg [DATA_WIDTH - 1 : 0] wbm_dat_o,     //! Data Out
        //! @end

        //! @virtualbus WBS0 @dir out Wishbone Slave 0
        output wire wbs0_cyc_o,                         //! Cycle
        output wire wbs0_stb_o,                         //! Strobe
        output wire [ADDR_WIDTH - 2 : 0] wbs0_adr_o,    //! Address
        output wire wbs0_we_o,                          //! Write Enable
        output wire [DATA_WIDTH - 1 : 0] wbs0_dat_o,    //! Data Out
        output wire [SEL_WIDTH - 1 : 0] wbs0_sel_o,     //! Write Select
        output wire [2:0] wbs0_cti_o,                   //! Cycle Type Indicator
        output wire [1:0] wbs0_bte_o,                   //! Burst Type Extension
        input wire wbs0_ack_i,                          //! Acknowledge
        input wire wbs0_err_i,                          //! Error
        input wire wbs0_rty_i,                          //! Retry
        input wire wbs0_stall_i,                        //! Stall
        input wire [DATA_WIDTH - 1 : 0] wbs0_dat_i,     //! Data In
        //! @end

        //! @virtualbus WBS1 @dir out Wishbone Slave 1
        output wire wbs1_cyc_o,                         //! Cycle
        output wire wbs1_stb_o,                         //! Strobe
        output wire [ADDR_WIDTH - 2 : 0] wbs1_adr_o,    //! Address
        output wire wbs1_we_o,                          //! Write Enable
        output wire [DATA_WIDTH - 1 : 0] wbs1_dat_o,    //! Data Out
        output wire [SEL_WIDTH - 1 : 0] wbs1_sel_o,     //! Write Select
        output wire [2:0] wbs1_cti_o,                   //! Cycle Type Indicator
        output wire [1:0] wbs1_bte_o,                   //! Burst Type Extension
        input wire wbs1_ack_i,                          //! Acknowledge
        input wire wbs1_err_i,                          //! Error
        input wire wbs1_rty_i,                          //! Retry
        input wire wbs1_stall_i,                        //! Stall
        input wire [DATA_WIDTH - 1 : 0] wbs1_dat_i     //! Data In
        //! @end        
    );

    wire wbs1_sel;

    assign wbs1_sel = wbm_adr_i[ADDR_WIDTH - 1];

    assign wbs0_cyc_o = !wbs1_sel && wbm_cyc_i;
    assign wbs0_stb_o = !wbs1_sel && wbm_stb_i;
    assign wbs0_adr_o = wbm_adr_i[ADDR_WIDTH - 2 : 0];
    assign wbs0_we_o = wbm_we_i;
    assign wbs0_dat_o = wbm_dat_i;
    assign wbs0_sel_o = wbm_sel_i;
    assign wbs0_cti_o = wbm_cti_i;
    assign wbs0_bte_o = wbm_bte_i;

    assign wbs1_cyc_o = wbs1_sel && wbm_cyc_i;
    assign wbs1_stb_o = wbs1_sel && wbm_stb_i;
    assign wbs1_adr_o = wbm_adr_i[ADDR_WIDTH - 2 : 0];
    assign wbs1_we_o = wbm_we_i;
    assign wbs1_dat_o = wbm_dat_i;
    assign wbs1_sel_o = wbm_sel_i;
    assign wbs1_cti_o = wbm_cti_i;
    assign wbs1_bte_o = wbm_bte_i;

    assign wbm_stall_o = wbs1_sel ? wbs1_stall_i : wbs0_stall_i;

    always @ (posedge clk_i) begin
        wbm_ack_o <= wbs0_ack_i || wbs1_ack_i;
        wbm_err_o <= wbs0_err_i || wbs1_err_i;
        wbm_rty_o <= wbs0_rty_i || wbs1_rty_i;
    end

    always @ (posedge clk_i) begin
        wbm_dat_o <= wbs1_sel ? wbs1_dat_i : wbs0_dat_i;
    end
endmodule
