`default_nettype none

module top
    (
        input  wire ICE_CLK,
        
        input wire USB_TXD_SCK,
        input wire USB_RXD_MOSI,
        output wire USB_RTS_MISO,
        input wire USB_CTS_SS,

        inout wire [7:0] PMOD,
        output wire LED4
    );

    wire clk;
    wire clk_locked;
    wire rst_n;
    wire [7:0] ui_in;
    wire [7:0] uo_out;
    wire [7:0] uio_in;
    wire [7:0] uio_out;
    wire [7:0] uio_oe;

    assign ui_in[4] = USB_CTS_SS;
    assign ui_in[5] = USB_TXD_SCK;
    assign ui_in[6] = USB_RXD_MOSI;
    assign USB_RTS_MISO = uo_out[7];

    assign rst_n = clk_locked;

    assign LED4 = clk_locked;

    pll pll(
        .clock_in(ICE_CLK),
        .clock_out(clk),
        .locked(clk_locked)
    );

    SB_IO #(
        .PIN_TYPE(6'b1010_01)
    ) uio_pin[7:0] (
        .PACKAGE_PIN(PMOD),
        .OUTPUT_ENABLE(uio_oe),
        .D_OUT_0(uio_out),
        .D_IN_0(uio_in),
    );

    tt_um_levenshtein levenshtein (
        .clk(clk),
        .rst_n(rst_n),
        .ena(1'b1),
        .ui_in(ui_in),
        .uo_out(uo_out),
        .uio_in(uio_in),
        .uio_out(uio_out),
        .uio_oe(uio_oe)
    );
endmodule
