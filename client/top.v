`default_nettype none

module top
    (
        input wire clk,
        input wire rst_n,
        input wire ena,

        input wire uart_rxd,
        output wire uart_txd,

        input wire spi_ss_n,
        input wire spi_sck,
        input wire spi_mosi,
        output wire spi_miso
    );

    /* verilator lint_off UNUSEDSIGNAL */
    wire [7:0] ui_in;
    wire [7:0] uo_out;
    wire [7:0] uio_in;
    wire [7:0] uio_out;
    wire [7:0] uio_oe;
    /* verilator lint_on UNUSEDSIGNAL */

    assign ui_in[2:0] = 3'b000;
    assign ui_in[7] = 1'b1;
    assign uio_in[0] = 1'b0;
    assign uio_in[3] = 1'b0;
    assign uio_in[7:6] = 2'b00;

    assign ui_in[3] = uart_rxd;
    assign uart_txd = uo_out[4];

    assign ui_in[4] = spi_ss_n;
    assign ui_in[5] = spi_sck;
    assign ui_in[6] = spi_mosi;
    assign spi_miso = uo_out[7];

    tt_um_levenshtein levenshtein(
        .clk(clk),
        .rst_n(rst_n),
        .ena(ena),
        .ui_in(ui_in),
        .uo_out(uo_out),
        .uio_in(uio_in),
        .uio_out(uio_out),
        .uio_oe(uio_oe)
    );

    qspi_sram #(.VERBOSE(0)) pmod_sram(
        .sck(uio_out[3]),
        .ss_n(uio_out[0]),
        .sio_in({uio_out[5], uio_out[4], uio_out[2], uio_out[1]}),
        .sio_out({uio_in[5], uio_in[4], uio_in[2], uio_in[1]})
    );
endmodule
