`default_nettype none

module levenshtein_controller
    #(
        parameter MASTER_ADDR_WIDTH=24,
        parameter SLAVE_ADDR_WIDTH=24
    )
    (
        input wire clk_i,
        input wire rst_i,

        //! @virtualbus WBM @dir out Wishbone master
        output wire wbm_cyc_o,
        output wire wbm_stb_o,
        output wire [MASTER_ADDR_WIDTH - 1 : 0] wbm_adr_o,
        output wire wbm_we_o,
        output wire [7:0] wbm_dat_o,
        input wire wbm_ack_i,
        input wire wbm_err_i,
        input wire wbm_rty_i,
        input wire [7:0] wbm_dat_i,
        //! @end

        //! @virtualbus WBS @dir in Wishbone slave
        input wire wbs_cyc_i,
        input wire wbs_stb_i,
        /* verilator lint_off UNUSEDSIGNAL */
        input wire [SLAVE_ADDR_WIDTH - 1 : 0] wbs_adr_i,
        /* verilator lint_on UNUSEDSIGNAL */
        input wire wbs_we_i,
        /* verilator lint_off UNUSEDSIGNAL */
        input wire [7:0] wbs_dat_i,
        /* verilator lint_on UNUSEDSIGNAL */
        output reg wbs_ack_o,
        output wire wbs_err_o,
        output wire wbs_rty_o,
        output wire [7:0] wbs_dat_o
        //! @end
    );

    reg enabled;
    reg error;
    reg [2:0] word_length;

    localparam STATE_READ_DICT = 2'd0;
    localparam STATE_READ_VECTOR = 2'd1;
    localparam STATE_LEVENSHTEIN = 2'd2;
    localparam STATE_WRITE_RESULT = 2'd3;

    localparam DICT_ADDR_WIDTH = MASTER_ADDR_WIDTH - 1;
    localparam RESULT_ADDR_WIDTH = MASTER_ADDR_WIDTH - 2;

    reg [1:0] state;
    reg [DICT_ADDR_WIDTH - 1 : 0] dict_address;
    reg [RESULT_ADDR_WIDTH - 1 : 0] result_address;
    reg [7:0] pm;
    reg cyc;
    reg [3:0] d;

    assign wbs_err_o = 1'b0;
    assign wbs_rty_o = 1'b0;
    assign wbs_dat_o = {enabled, error, 3'b000, word_length};

    assign wbm_cyc_o = cyc;
    assign wbm_stb_o = cyc;
    assign wbm_adr_o = (state == STATE_READ_DICT ? {1'b1, dict_address} : (state == STATE_READ_VECTOR ? MASTER_ADDR_WIDTH'(pm) : {2'b01, result_address}));
    assign wbm_we_o = (state == STATE_WRITE_RESULT);
    assign wbm_dat_o = {4'h0, d};

    always @ (posedge clk_i) begin
        if (rst_i) begin
            enabled <= 1'b0;
            wbs_ack_o <= 1'b0;

            state <= STATE_READ_DICT;
            cyc <= 1'b0;
            dict_address <= DICT_ADDR_WIDTH'(0);
            result_address <= RESULT_ADDR_WIDTH'(0);
            d <= 4'd0;
        end else begin
            if (wbs_cyc_i && wbs_stb_i && !wbs_ack_o) begin
                if (wbs_we_i) begin
                    enabled <= wbs_dat_i[7];
                    error <= 1'b0;
                    word_length <= wbs_dat_i[2:0];
                end
                wbs_ack_o <= 1'b1;
            end else begin
                wbs_ack_o <= 1'b0;
            end
        
            if (enabled) begin
                case (state)
                    STATE_READ_DICT: begin
                        if (!cyc) begin
                            cyc <= 1'b1;
                        end else if (wbm_ack_i) begin
                            pm <= wbm_dat_i;
                            if (wbm_dat_i[7] == 1'b1) begin
                                state <= STATE_WRITE_RESULT;
                            end else begin
                                state <= STATE_READ_VECTOR;
                            end
                            cyc <= 1'b0;
                            dict_address <= dict_address + DICT_ADDR_WIDTH'(1);
                        end else if (wbm_err_i || wbm_rty_i) begin
                            enabled <= 1'b0;
                            error <= 1'b1;
                        end
                    end

                    STATE_READ_VECTOR: begin
                        if (!cyc) begin
                            cyc <= 1'b1;
                        end else if (wbm_ack_i) begin
                            pm <= wbm_dat_i;
                            cyc <= 1'b0;
                            state <= STATE_LEVENSHTEIN;
                        end else if (wbm_err_i || wbm_rty_i) begin
                            enabled <= 1'b0;
                            error <= 1'b1;
                        end
                    end

                    STATE_LEVENSHTEIN: begin
                        state <= STATE_READ_DICT;
                    end

                    STATE_WRITE_RESULT: begin
                        if (!cyc) begin
                            cyc <= 1'b1;
                        end else if (wbm_ack_i) begin
                            result_address <= result_address + RESULT_ADDR_WIDTH'(1);
                            cyc <= 1'b0;
                            d <= pm[3:0];
                            if (pm == 8'hFF) begin
                                enabled <= 1'b0;
                            end
                            state <= STATE_READ_DICT;
                        end else if (wbm_err_i || wbm_rty_i) begin
                            enabled <= 1'b0;
                            error <= 1'b1;
                        end
                    end
                endcase
            end
        end
    end
endmodule
