`default_nettype none

//! Interconnect
//!
//! This interconnect splits the bus into a small register space with reduced
//! address space, and a larger space with the complete address range.
module wb_interconnect
    #(
        parameter ADDR_WIDTH=24,                    //! Address width
        parameter DATA_WIDTH=8,                     //! Data width
        parameter SHARED_BUS=1,                     // 
        parameter SLAVE0_ADDR_WIDTH=3,              //! Number of address bits for slave 0
        parameter SEL_WIDTH=(DATA_WIDTH + 7) / 8    //! Number of sel bits (Calculated automatically)
    )
    (
        //! @virtualbus WBM @dir in Wishbone Slave Port
        input wire wbs_cyc_i,                          //! Cycle
        input wire wbs_stb_i,                          //! Strobe
        input wire [ADDR_WIDTH - 1 : 0] wbs_adr_i,     //! Address
        input wire wbs_we_i,                           //! Write Enable
        input wire [SEL_WIDTH - 1 : 0] wbs_sel_i,      //! Write Select
        input wire [DATA_WIDTH - 1 : 0] wbs_dat_i,     //! Data In
        input wire [2:0] wbs_cti_i,                    //! Cycle Type Indicator
        input wire [1:0] wbs_bte_i,                    //! Burst Type Extension
        output logic wbs_ack_o,                         //! Acknowledge
        output logic wbs_err_o,                         //! Error
        output logic wbs_rty_o,                         //! Retry
        output wire [DATA_WIDTH - 1 : 0] wbs_dat_o,    //! Data Out
        //! @end

        //! @virtualbus WBS0 @dir out Wishbone Master Port 0
        output logic wbm0_cyc_o,                                 //! Cycle
        output logic wbm0_stb_o,                                 //! Strobe
        output wire [SLAVE0_ADDR_WIDTH - 1 : 0] wbm0_adr_o,     //! Address
        output wire wbm0_we_o,                                  //! Write Enable
        output wire [SEL_WIDTH - 1 : 0] wbm0_sel_o,             //! Write Select
        output wire [DATA_WIDTH - 1 : 0] wbm0_dat_o,            //! Data Out
        output wire [2:0] wbm0_cti_o,                           //! Cycle Type Indicator
        output wire [1:0] wbm0_bte_o,                           //! Burst Type Extension
        input wire wbm0_ack_i,                                  //! Acknowledge
        input wire wbm0_err_i,                                  //! Error
        input wire wbm0_rty_i,                                  //! Retry
        input wire [DATA_WIDTH - 1 : 0] wbm0_dat_i,             //! Data In
        //! @end

        //! @virtualbus WBS1 @dir out Wishbone Master Port 1
        output logic wbm1_cyc_o,                         //! Cycle
        output logic wbm1_stb_o,                         //! Strobe
        output wire [ADDR_WIDTH - 1 : 0] wbm1_adr_o,    //! Address
        output wire wbm1_we_o,                          //! Write Enable
        output wire [SEL_WIDTH - 1 : 0] wbm1_sel_o,     //! Write Select
        output wire [DATA_WIDTH - 1 : 0] wbm1_dat_o,    //! Data Out
        output wire [2:0] wbm1_cti_o,                   //! Cycle Type Indicator
        output wire [1:0] wbm1_bte_o,                   //! Burst Type Extension
        input wire wbm1_ack_i,                          //! Acknowledge
        input wire wbm1_err_i,                          //! Error
        input wire wbm1_rty_i,                          //! Retry
        input wire [DATA_WIDTH - 1 : 0] wbm1_dat_i      //! Data In
        //! @end        
    );

    localparam PREFIX_WIDTH = ADDR_WIDTH - SLAVE0_ADDR_WIDTH;

    wire acmp0 = wbs_adr_i[ADDR_WIDTH - 1 : SLAVE0_ADDR_WIDTH] == PREFIX_WIDTH'(0);
    wire acmp1 = !acmp0;

    assign wbm0_stb_o = wbs_stb_i & acmp0;
    assign wbm0_adr_o = wbs_adr_i[SLAVE0_ADDR_WIDTH - 1 : 0];
    assign wbm0_we_o = wbs_we_i;
    assign wbm0_sel_o = wbs_sel_i;
    assign wbm0_dat_o = wbs_dat_i;
    assign wbm0_cti_o = wbs_cti_i;
    assign wbm0_bte_o = wbs_bte_i;
    
    assign wbm1_stb_o = wbs_stb_i & acmp1;
    assign wbm1_adr_o = wbs_adr_i;
    assign wbm1_we_o = wbs_we_i;
    assign wbm1_sel_o = wbs_sel_i;
    assign wbm1_dat_o = wbs_dat_i;
    assign wbm1_cti_o = wbs_cti_i;
    assign wbm1_bte_o = wbs_bte_i;
    assign wbs_dat_o = acmp0 ? wbm0_dat_i : wbm1_dat_i;

    always_comb begin
        if (SHARED_BUS) begin
            wbm0_cyc_o = wbs_cyc_i;
            wbm1_cyc_o = wbs_cyc_i;
            wbs_ack_o = wbm0_ack_i | wbm1_ack_i;
            wbs_rty_o = wbm0_rty_i | wbm1_rty_i;
            wbs_err_o = wbm0_err_i | wbm1_err_i;
        end else begin
            wbm0_cyc_o = wbs_cyc_i & acmp0;
            wbm1_cyc_o = wbs_cyc_i & acmp1;
            wbs_ack_o = acmp0 ? wbm0_ack_i : wbm1_ack_i;
            wbs_rty_o = acmp0 ? wbm0_rty_i : wbm1_rty_i;
            wbs_err_o = acmp0 ? wbm0_err_i : wbm1_err_i;
        end
    end
endmodule
