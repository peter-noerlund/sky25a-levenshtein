`default_nettype none

/*

Timings from APS6404L-3SQR datasheet (PSRAM chip on QQSPI PSRAM Pmod)

Data is read on the rising edge of CLK.

Clock period is min 30.3ns for READ (tCLK)
Clock high and low width must be between 0.45 and 0.55 of tCLK (tCH and tCL)
Clock transition time is max 1.5ns (tKHKL)

CE# must be held low min 2.5ns before first CLK rise (tCSP)
CE# must be held high min 3.0ns after last CLK rise (tCHD)
CE# must not be held low for more than 8µs (tCEM)
CE# must be held high for min. 18ns between bursts (tCPH)
MISO transitions to high-z max 5.5ns after CE# goes high (tHZ)

Data is latched in rising CLK edge
Setup time for data in is 2 ns before CLK rise (tSP)
Hold time for data in is 2 ns after CLK rise (tHD)

Data is latched out on falling CLK edge
Output delay is 2 - 5.5ns after CLK fall (tACLK)
Data is held after CLK fall for at min. 1.5ns (tKOH)

Write is terminated by pulling CE# high min 3.0ns after last rising CLK (tCHD)
while keeping data valid for 2ns (tHD)
Read is terminated by pulling CE# high min 3.0ns after last rising CLK (tCHD),
but data will not be valid until 2-5.5ns after the last fallign CLK (tACLK) and
data will be invalidated within at most 5.5ns after CE# goes high.

Design is running at 48MHz and SPI clock will be run at 24MHz.

That makes tCLK 41.67ns, tCH 20.8ns and tCL 20.8ns.
Maximum slew for sky130A is 1.5ns, tKHKL is within bounds.


Initiation

            CE#     CLK     MOSI    MISO    |   Action
Clock 0:    1       0       z       z       |   Set CE# low and MOSI to command bit 7
Clock 1:    0       0       C7      z       |   Set CLK high
Clock 2:    0       1       C7      z       |   Device reads bit. Set CLK low and MOSI to command bit 6
Clock 3:    0       0       C6      z       |   Set CLK high
Clock 4:    0       1       C6      z       |   Device reads bit. Set CLK low and MOSI to command bit 5
Clock 5:    0       0       C5      z       |   Set CLK high
Clock 6:    0       1       C5      z       |   Device reads bit. Set CLK low and MOSI to command bit 4
...
Clock 62:   0       1       A1      z       |   Device reads bit. Set CLK low and MOSI to address bit 0
Clock 63:   0       0       A0      z       |   Set CLK high
Clock 64:   0       1       A0      z       |   Device reads bit. Set CLK low

CE# is held low 20.8ns before CLK goes high, which is within bounds of 2.5ns (tCSP)
Data is latched out 20.8ns before CLK goes high, which is within bounds of 2ns (tSP)

Write termination:

            CE#     CLK     MOSI    MISO    |   Action
Clock 78:   0       1       D1      z       |   Device reads bit. Set CLK low and MOSI to data bit 0
Clock 79:   0       0       D0      z       |   Set CLK high
Clock 80:   0       1       D0      z       |   Device reads bit. Set CLK low. Set CE# high. Set MOSI high z
Clock 81:   1       0       z       z       |

CE# is kept low 20.8ns after last CLK rise keeping us within required 3.0ns (tCHD)
Data is kept valid 20.8ns after last CLK rise keeping us within required 2.0n (tHD)

Read:

            CE#     CLK     MOSI    MISO    |   Action
Clock 62:   0       1       A1      z       |   Device reads bit. Set CLK low and MOSI to address bit 0
Clock 63:   0       0       A0      z       |   Set CLK high
Clock 64:   0       1       A0      z       |   Device reads bit. Set CLK low and MOSI to high z
Clock 65:   0       0       z       z       |   Device writes bit within 2-5.5ns. Set CLK high
Clock 66:   0       1       z       D7      |   Read bit. Set CLK low
Clock 67:   0       0       z       x       |   Device writes bit within 2-5.5ns. Set CLK high
Clock 68:   0       1       z       D6      |   Read bit. Set CLK low
Clock 69:   0       0       z       x       |   Device writes bit within 2-5.5ns. Set CLK high
Clock 70:   0       1       z       D5      |   Read bit. Set CLK low
Clock 71:   0       0       z       x       |   Device writes bit within 2-5.5ns. Set CLK high
Clock 72:   0       1       z       D4      |   Read bit. Set CLK low
Clock 73:   0       0       z       x       |   Device writes bit within 2-5.5ns. Set CLK high
Clock 74:   0       1       z       D3      |   Read bit. Set CLK low
Clock 75:   0       0       z       x       |   Device writes bit within 2-5.5ns. Set CLK high
Clock 76:   0       1       z       D2      |   Read bit. Set CLK low
Clock 77:   0       0       z       x       |   Device writes bit within 2-5.5ns. Set CLK high
Clock 78:   0       1       z       D1      |   Read bit. Set CLK low
Clock 79:   0       0       z       x       |   Device writes bit within 2-5.5ns. Set CLK high
Clock 80:   0       1       z       D0      |   Read bit. Set CLK low. Set CE# high
Clock 81:   1       0       z       z       |   

Device writes bit 2-5.5ns (tACLK) after CLK falling edge, which is 15.3-18.8ns before CLK rises

*/
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
                    /*
                    bit_counter=0:  send command bit 6 (1'b0)
                    bit_counter=1:  send command bit 5 (1'b0)
                    bit_counter=2:  send command bit 4 (1'b0)
                    bit_counter=3:  send command bit 3 (1'b0)
                    bit_counter=4:  send_command bit 2 (1'b0)
                    bit_counter=5:  send_command bit 1 (1'b1)
                    bit_counter=6:  send_command bit 0 (!we_i)
                    bit_counter=7:  send address bit 23
                    bit_counter=8:  send address bit 22
                    ...
                    bit_counter=30: send address bit 0
                    bit_counter=31: exchange data bit 7
                    bit_counter=38: exchange data bit 0, set CE# high
                    */
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
