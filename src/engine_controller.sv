`default_nettype none

/* verilator lint_off UNUSEDSIGNAL */
module engine_controller
    (
        input wire clk_i,
        input wire rst_i,

        //! @virtualbus WB @dir in
        input wire cyc_i,
        input wire stb_i,
        input wire we_i,
        input wire [7:0] dat_i,
        output reg ack_o,
        output wire err_o,
        output wire rty_o,
        output wire [7:0] dat_o,
        //! @end

        output reg enabled,
        output reg [4:0] word_length
    );

    assign err_o = 1'b0;
    assign rty_o = 1'b0;

    assign dat_o = {enabled, 2'b00, word_length};

    always @ (posedge clk_i) begin
        if (rst_i) begin
            enabled <= 1'b0;
            ack_o <= 1'b0;
        end else begin
            if (cyc_i && stb_i && !ack_o) begin
                if (we_i) begin
                    enabled <= dat_i[7];
                    word_length <= dat_i[4:0];
                end
                ack_o <= 1'b1;
            end else begin
                ack_o <= 1'b0;
            end
        end
    end
endmodule
