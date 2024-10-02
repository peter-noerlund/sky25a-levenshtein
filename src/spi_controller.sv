`default_nettype none

module spi_controller
    (
        input wire clk_i,
        input wire rst_i,

        //! @virtualbus WB @dir input Wishbone
        input wire cyc_i,
        input wire stb_i,
        input wire [23:0] adr_i,
        input wire we_i,
        input wire [7:0] dat_i,
        input wire [2:0] cti_i,
        input wire [1:0] bte_i,
        output logic ack_o,
        output wire err_o,
        output wire rty_o,
        output logic [7:0] dat_o,
        //! @end

        //! @virtualbus QSPI @dir output QSPI
        output logic sck,           //! Serial clock
        output logic [3:0] sio_out, //! Output pins (sio_out[0] is MOSI)
        /* verilator lint_off UNUSEDSIGNAL */
        input wire [3:0] sio_in,    //! Input pins (sio_in[1] is MISO)
        /* verilator lint_on UNUSEDSIGNAL */
        output logic [3:0] sio_oe,  //! Output enable
        
        output wire cs_n,           //! Primary CS pin
        output wire cs2_n,          //! Secondary CS pin
        output wire cs3_n,          //! Tertiary CS pin
        //! @end

        input wire [1:0] sram_config
    );

    localparam CTI_INCREMENTING_BURST = 3'b010;
    localparam BTE_LINEAR = 2'b00;

    localparam CONFIG_CS = 2'd1;
    localparam CONFIG_CS2 = 2'd2;
    localparam CONFIG_CS3 = 2'd3;

    logic ss_n;
    logic [5:0] bit_counter;
    wire is_burst;

    assign err_o = 1'b0;
    assign rty_o = 1'b0;

    assign cs_n = sram_config == CONFIG_CS ? ss_n : 1'b1;
    assign cs2_n = sram_config == CONFIG_CS2 ? ss_n : 1'b1;
    assign cs3_n = sram_config == CONFIG_CS3 ? ss_n : 1'b1;
    assign is_burst = !we_i && cti_i == CTI_INCREMENTING_BURST && bte_i == BTE_LINEAR;

    always @ (posedge clk_i) begin
        if (rst_i || !cyc_i || !stb_i) begin
            ack_o <= 1'b0;
            ss_n <= 1'b1;
            sck <= 1'b0;
            bit_counter <= 6'd0;
            sio_oe <= 4'b0001;
            sio_out <= 4'b0000;
        end else begin
            if (bit_counter == 6'd0) begin
                ss_n <= 1'b0;
            end
            if (!ss_n) begin
                sck <= ~sck;
            end
            if (sck) begin
                if (bit_counter <= 6'd4) begin
                    sio_out[0] <= 1'b0;
                end
                if (bit_counter == 6'd5) begin
                    sio_out[0] <= 1'b1;
                end
                if (bit_counter == 6'd6) begin
                    sio_out[0] <= !we_i;                        
                end
                if (bit_counter >= 6'd7 && bit_counter <= 6'd30) begin
                    sio_out[0] <= adr_i[5'd23 - 5'(bit_counter - 6'd7)];
                end
                if (bit_counter >= 6'd31 && bit_counter <= 6'd38) begin
                    sio_out[0] <= dat_i[3'd7 - 3'(bit_counter - 6'd31)];
                end
                if (bit_counter == 6'd39) begin
                    ack_o <= 1'b1;
                    sio_out[0] <= 1'b0;
                    if (!is_burst) begin
                        ss_n <= 1'b1;
                    end
                end
                dat_o <= {dat_o[6:0], sio_in[1]};
            end

            if (bit_counter == 6'd40) begin
                bit_counter <= 6'd0;
            end else if (sck && bit_counter == 6'd39 && is_burst) begin
                bit_counter <= 6'd32;
            end else if (sck) begin
                bit_counter <= bit_counter + 6'd1;
            end

            if (ack_o) begin
                ack_o <= 1'b0;
            end
        end
    end
endmodule
