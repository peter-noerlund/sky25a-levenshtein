`default_nettype none

module spi_controller
    (
        input wire clk_i,
        input wire rst_i,

        input wire cyc_i,
        input wire stb_i,
        input wire [23:0] adr_i,
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

    assign err_o = 1'b0;
    assign rty_o = 1'b0;

    reg [5:0] bit_counter;

    always @ (posedge clk_i) begin
        if (rst_i) begin
            ack_o <= 1'b0;
            ss_n <= 1'b1;
            sck <= 1'b0;
            bit_counter <= 6'd0;
            mosi <= 1'b0;
        end else begin
            if (cyc_i && stb_i && !ack_o) begin
                if (ss_n) begin
                    ss_n <= 1'b0;
                end
                if (!ss_n) begin
                    sck <= ~sck;
                end
                if (sck) begin
                    if (bit_counter <= 6'd4) begin
                        mosi <= 1'b0;
                    end else if (bit_counter == 6'd5) begin
                        mosi <= 1'b1;
                    end else if (bit_counter == 6'd6) begin
                        mosi <= !we_i;                        
                    end else if (bit_counter >= 6'd7 && bit_counter <= 6'd30) begin
                        mosi <= adr_i[5'd23 - 5'(bit_counter - 6'd7)];
                    end else if (bit_counter >= 6'd31 && bit_counter <= 6'd38) begin
                        mosi <= dat_i[3'd7 - 3'(bit_counter - 6'd31)];
                        dat_o <= {dat_o[6:0], miso};
                    end else if (bit_counter == 6'd39) begin
                        ack_o <= 1'b1;
                        mosi <= 1'b0;
                        ss_n <= 1'b1;
                        dat_o <= {dat_o[6:0], miso};
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
