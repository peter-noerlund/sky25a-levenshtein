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
        input wire [3:0] sio_in,    //! Input pins (sio_in[1] is MISO)
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

    logic [7:0] read_command;
    logic [7:0] write_command;
    logic ss_n;
    logic [4:0] bit_counter;
    wire is_burst;

    assign err_o = 1'b0;
    assign rty_o = 1'b0;

    assign cs_n = sram_config == CONFIG_CS ? ss_n : 1'b1;
    assign cs2_n = sram_config == CONFIG_CS2 ? ss_n : 1'b1;
    assign cs3_n = sram_config == CONFIG_CS3 ? ss_n : 1'b1;
    assign is_burst = !we_i && cti_i == CTI_INCREMENTING_BURST && bte_i == BTE_LINEAR;
    assign read_command = 8'hEB;
    assign write_command = 8'h38;

    /*
        Signals

        bit_counter |
        0 (ss_n)    | Send command bit 7
        0           | Send command bit 6
        1           | Send command bit 5
        2           | Send command bit 4
        3           | Send command bit 3
        4           | Send command bit 2
        5           | Send command bit 1
        6           | Send command bit 0
        7           | Send address bit 23-20
        8           | Send address bit 19-16
        9           | Send address bit 15-12
        10          | Send address bit 11-8
        11          | Send address bit 7-4
        12          | Send address bit 3-0
        13          | Send data bit 7-4         | Toggle input pins
        14          | Send data bit 3-0         |
        15          |                           |
        16          |                           |
        17          |                           |
        18          |                           |
        19          |                           | Receive data bit 7-4
        20          |                           | Receive data bit 3-0


    */

    always @ (posedge clk_i) begin
        if (rst_i || !cyc_i || !stb_i) begin
            ack_o <= 1'b0;
            ss_n <= 1'b1;
            sck <= 1'b0;
            bit_counter <= 5'd0;
            sio_oe <= 4'b0000;
            sio_out <= 4'b0000;
        end else begin
            if (bit_counter == 5'd0) begin
                ss_n <= 1'b0;
                sio_oe <= 4'b0001;
                sio_out[0] <= we_i ? write_command[7] : read_command[7];
            end
            if (!ss_n) begin
                sck <= ~sck;
            end
            if (sck) begin
                if (bit_counter <= 5'd6) begin
                    sio_out[0] <= we_i ? write_command[6 - bit_counter] : read_command[6 - bit_counter];
                end
                if (bit_counter >= 5'd7 && bit_counter <= 5'd12) begin
                    sio_oe <= 4'b1111;
                    sio_out <= adr_i[5'd23 - (bit_counter - 5'd7) * 5'd4 -: 4];
                end

                if (we_i) begin
                    if (bit_counter == 5'd13) begin
                        sio_out <= dat_i[7:4];
                    end
                    if (bit_counter == 5'd14) begin
                        sio_out <= dat_i[3:0];
                    end
                    if (bit_counter == 5'd15) begin
                        sio_oe <= 4'b0000;
                        ss_n <= 1'b1;
                        ack_o <= 1'b1;
                    end
                end else begin
                    if (bit_counter == 5'd13) begin
                        sio_oe <= 4'b0000;
                    end
                    if (bit_counter == 5'd21) begin
                        ack_o <= 1'b1;
                        if (!is_burst) begin
                            ss_n <= 1'b1;
                        end
                    end
                end
                dat_o <= {dat_o[3:0], sio_in};
            end

            if (we_i && bit_counter == 5'd16) begin
                bit_counter <= 5'd0;
            end else if (!we_i && bit_counter == 5'd23) begin
                bit_counter <= 5'd0;
            end else if (sck && !we_i && bit_counter == 5'd21 && is_burst) begin
                bit_counter <= 5'd20;
            end else if (sck) begin
                bit_counter <= bit_counter + 5'd1;
            end

            if (ack_o) begin
                ack_o <= 1'b0;
            end
        end
    end
endmodule
