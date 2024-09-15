`default_nettype none

module wb_arbiter
    #(
        parameter MASTER_COUNT=2,
        parameter GNT_WIDTH=$clog2(MASTER_COUNT)
    )
    (
        input wire clk_i,
        input wire rst_i,

        input wire [MASTER_COUNT - 1 : 0] cyc_i,
        output reg [GNT_WIDTH - 1 : 0] gnt_o,
        output reg cyc_o
    );

    integer i;

    always @ (posedge clk_i) begin
        if (rst_i) begin
            cyc_o <= 1'b0;
            gnt_o <= GNT_WIDTH'(0);
        end else begin
            if (cyc_o) begin
                for (i = 0; i != MASTER_COUNT; i = i + 1) begin
                    if (gnt_o == GNT_WIDTH'(i) && !cyc_i[i]) begin
                        cyc_o <= 1'b0;
                    end
                end
            end else begin
                for (i = 0; i != MASTER_COUNT; i = i + 1) begin
                    if (cyc_i[i]) begin
                        gnt_o <= GNT_WIDTH'(i);
                        cyc_o <= 1'b1;
                    end
                end
            end
        end
    end
endmodule
