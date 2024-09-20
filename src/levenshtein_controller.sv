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
        input wire [7:0] wbs_dat_i,
        output reg wbs_ack_o,
        output wire wbs_err_o,
        output wire wbs_rty_o,
        output wire [7:0] wbs_dat_o
        //! @end
    );

    localparam BITVECTOR_WIDTH = 16;
    localparam DISTANCE_WIDTH = 8;
    localparam ID_WIDTH = 16;

    localparam ADDR_CTRL = 0;
    localparam ADDR_LENGTH = 1;
    localparam ADDR_MASK_HI = 2;
    localparam ADDR_MASK_LO = 3;
    localparam ADDR_INITIAL_VP_HI = 4;
    localparam ADDR_INITIAL_VP_LO = 5;

    localparam ADDR_STATE = 2'd0;
    localparam ADDR_DISTANCE = 2'd1;
    localparam ADDR_IDX_HI = 2'd2;
    /* verilator lint_off UNUSEDPARAM */
    localparam ADDR_IDX_LO = 2'd3;
    /* verilator lint_on UNUSEDPARAM */

    reg enabled;
    reg [4:0] word_length;
    reg [BITVECTOR_WIDTH - 1 : 0] mask;
    reg [BITVECTOR_WIDTH - 1 : 0] initial_vp;

    localparam STATE_READ_DICT = 2'd0;
    localparam STATE_READ_VECTOR_HI = 2'd1;
    localparam STATE_READ_VECTOR_LO = 2'd2;
    localparam STATE_LEVENSHTEIN = 2'd3;

    localparam DICT_ADDR_WIDTH = MASTER_ADDR_WIDTH - 1;

    reg [1:0] state;
    reg [DICT_ADDR_WIDTH - 1 : 0] dict_address;
    reg cyc;
    reg [BITVECTOR_WIDTH - 1 : 0] pm;
    wire [BITVECTOR_WIDTH - 1 : 0] d0;
    wire [BITVECTOR_WIDTH - 1 : 0] hp;
    wire [BITVECTOR_WIDTH - 1 : 0] hn;
    reg [BITVECTOR_WIDTH - 1 : 0] vp;
    reg [BITVECTOR_WIDTH - 1 : 0] vn;
    reg [DISTANCE_WIDTH - 1 : 0] d;

    reg [ID_WIDTH - 1 : 0] idx;
    reg [ID_WIDTH - 1 : 0] best_idx;
    reg [DISTANCE_WIDTH - 1 : 0] best_distance;

    assign wbs_err_o = 1'b0;
    assign wbs_rty_o = 1'b0;
    assign wbs_dat_o = 
        (wbs_adr_i[1:0] == ADDR_STATE ? {7'b0000000, enabled} :
        (wbs_adr_i[1:0] == ADDR_DISTANCE ? best_distance :
        (wbs_adr_i[1:0] == ADDR_IDX_HI ? best_idx[15:8] : best_idx[7:0])));

    assign wbm_cyc_o = cyc;
    assign wbm_stb_o = cyc;
    assign wbm_adr_o =
        (state == STATE_READ_DICT ? {1'b1, dict_address} :
        (state == STATE_READ_VECTOR_HI ? MASTER_ADDR_WIDTH'({pm[7:0], 1'b0}) :  MASTER_ADDR_WIDTH'({pm[7:0], 1'b1})));
    assign wbm_we_o = 1'b0;
    assign wbm_dat_o = 8'h00;

    assign d0 = (((pm & vp) + vp) ^ vp) | pm | vn;
    assign hp = vn | ~(d0 | vp);
    assign hn = d0 & vp;

    always @ (posedge clk_i) begin
        if (rst_i) begin
            enabled <= 1'b0;
            wbs_ack_o <= 1'b0;

            cyc <= 1'b0;

            dict_address <= DICT_ADDR_WIDTH'(0);
            d <= DISTANCE_WIDTH'(0);
            vp <= BITVECTOR_WIDTH'(0);
            vn <= BITVECTOR_WIDTH'(0);
            state <= STATE_READ_DICT;

            idx <= ID_WIDTH'(0);
            best_idx <= ID_WIDTH'(0);
            best_distance <= DISTANCE_WIDTH'(-1);

            word_length <= 5'd0;
            mask <= 16'h0000;
            initial_vp <= 16'h0000;
        end else begin
            if (wbs_cyc_i && wbs_stb_i && !wbs_ack_o) begin
                if (wbs_we_i) begin
                    if (wbs_adr_i[2:0] == 3'(ADDR_CTRL)) begin
                        enabled <= wbs_dat_i[0];

                        dict_address <= DICT_ADDR_WIDTH'(0);
                        d <= DISTANCE_WIDTH'(word_length);
                        vn <= BITVECTOR_WIDTH'(0);
                        vp <= initial_vp;
                        state <= STATE_READ_DICT;

                        idx <= ID_WIDTH'(0);
                        best_idx <= ID_WIDTH'(0);
                        best_distance <= DISTANCE_WIDTH'(-1);
                    end else if (wbs_adr_i[2:0] == 3'(ADDR_LENGTH)) begin
                        word_length <= wbs_dat_i[4:0];
                    end else if (wbs_adr_i[2:0] == 3'(ADDR_MASK_HI)) begin
                        mask[15:8] <= wbs_dat_i;
                    end else if (wbs_adr_i[2:0] == 3'(ADDR_MASK_LO)) begin
                        mask[7:0] <= wbs_dat_i;
                    end else if (wbs_adr_i[2:0] == 3'(ADDR_INITIAL_VP_HI)) begin
                        initial_vp[15:8] <= wbs_dat_i;
                    end else if (wbs_adr_i[2:0] == 3'(ADDR_INITIAL_VP_LO)) begin
                        initial_vp[7:0] <= wbs_dat_i;
                    end
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
                            pm[7:0] <= wbm_dat_i;
                            if (wbm_dat_i == 8'hFE) begin
                                if (d < best_distance) begin
                                    best_idx <= idx;
                                    best_distance <= d;
                                end
                                idx <= idx + ID_WIDTH'(1);
                                d <= DISTANCE_WIDTH'(word_length);
                                vn <= BITVECTOR_WIDTH'(0);
                                vp <= initial_vp;
                                state <= STATE_READ_DICT;
                            end else if (wbm_dat_i == 8'hFF) begin
                                enabled <= 1'b0;
                            end else begin
                                state <= STATE_READ_VECTOR_HI;
                            end
                            cyc <= 1'b0;
                            dict_address <= dict_address + DICT_ADDR_WIDTH'(1);
                        end else if (wbm_err_i || wbm_rty_i) begin
                            cyc <= 1'b0;
                            enabled <= 1'b0;
                        end
                    end

                    STATE_READ_VECTOR_HI: begin
                        if (!cyc) begin
                            cyc <= 1'b1;
                        end else if (wbm_ack_i) begin
                            pm[15:8] <= wbm_dat_i;
                            state <= STATE_READ_VECTOR_LO;
                        end else if (wbm_err_i || wbm_rty_i) begin
                            cyc <= 1'b0;
                            enabled <= 1'b0;
                        end
                    end

                    STATE_READ_VECTOR_LO: begin
                        if (!cyc) begin
                            cyc <= 1'b1;
                        end else if (wbm_ack_i) begin
                            pm[7:0] <= wbm_dat_i;
                            cyc <= 1'b0;
                            state <= STATE_LEVENSHTEIN;
                        end else if (wbm_err_i || wbm_rty_i) begin
                            cyc <= 1'b0;
                            enabled <= 1'b0;
                        end
                    end

                    STATE_LEVENSHTEIN: begin
                        if ((hp & mask) != BITVECTOR_WIDTH'(0)) begin
                            d <= d + DISTANCE_WIDTH'(1);
                        end else if ((hn & mask) != BITVECTOR_WIDTH'(0)) begin
                            d <= d - DISTANCE_WIDTH'(1);
                        end
                        vp <= (hn << 1) | ~(d0 | ((hp << 1) | BITVECTOR_WIDTH'(1)));
                        vn <= d0 & ((hp << 1) | BITVECTOR_WIDTH'(1));
                        state <= STATE_READ_DICT;
                    end
                endcase
            end
        end
    end
endmodule
