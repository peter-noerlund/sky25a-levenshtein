`default_nettype none

module wb2sram
    #(
        parameter ADDR_WIDTH=2
    )
    (
        input wire clk_i,
        input wire rst_i,

        input wire cyc_i,
        input wire stb_i,
        input wire [ADDR_WIDTH - 1 : 0] adr_i,
        input wire [7:0] dat_i,
        input wire we_i,
        output reg ack_o,
        output wire err_o,
        output wire rty_o,
        output wire stall_o,
        output reg [7:0] dat_o
    );

    localparam BYTE_COUNT = 2**ADDR_WIDTH;

    reg [7:0] memory [BYTE_COUNT - 1 : 0];

    assign err_o = 1'b0;
    assign rty_o = 1'b0;
    assign stall_o = 1'b0;

    always @ (posedge clk_i) begin
        if (rst_i) begin
            ack_o <= 1'b0;
        end else begin
            if (cyc_i && stb_i) begin
                if (we_i) begin
                    memory[adr_i] <= dat_i;
                end else begin
                    dat_o <= memory[adr_i];
                end
                ack_o <= 1'b1;
            end else begin
                ack_o <= 1'b0;
            end
        end
    end
endmodule