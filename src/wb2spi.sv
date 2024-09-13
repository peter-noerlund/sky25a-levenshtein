`default_nettype none

module wb2spi
    (
        input wire clk_i,
        input wire rst_i,

        input wire cyc_i,
        input wire stb_i,
        input wire [22:0] adr_i,
        input wire we_i,
        input wire [7:0] dat_i,
        output reg ack_o,
        output wire err_o,
        output wire rty_o,
        output reg [7:0] dat_o,

        output reg sck,
        output reg mosi,
        input wire miso,
        output reg ss_n
    );

    // READ  = 03 AA AA AA DD
    // WRITE = 02 AA AA AA DD

    assign err_o = 1'b0;
    assign rty_o = 1'b0;

    reg [6:0] bit_counter;

    always @ (posedge clk_i) begin
        if (rst_i) begin
            ack_o <= 1'b0;
            ss_n <= 1'b1;
            sck <= 1'b0;
            bit_counter <= 6'd0;
        end else begin
            if (cyc_i && stb_i && !ack_o) begin
                ss_n <= 1'b0;
                sck <= ~sck;
                if (!sck) begin
                    if (bit_counter >= 6'd0 && bit_counter <= 6'd5) begin
                        mosi <= 1'b0;
                    end else if (bit_counter == 6'd6) begin
                        mosi <= 1'b1;
                    end else if (bit_counter == 6'd7) begin
                        mosi <= !we_i;                        
                    end else if (bit_counter == 6'd8) begin
                        mosi <= 1'b0;
                    end else if (bit_counter >= 6'd9 && bit_counter <= 6'd31) begin
                        mosi <= adr_i[bit_counter - 5'd9];
                    end else if (bit_counter >= 6'd32 && bit_counter <= 6'd39) begin
                        mosi <= dat_i[bit_counter - 5'd32];
                        dat_o <= {dat_o[6:0], miso};
                    end else if (bit_counter == 6'd40) begin
                        ss_n <= 1'b0;
                    end
                    bit_counter <= bit_counter + 6'd1;
                end
            end else begin
                ack_o <= 1'b0;
                ss_n <= 1'b1;
                sck <= 1'b0;
                bit_counter <= 6'd0;
            end
        end
    end
endmodule