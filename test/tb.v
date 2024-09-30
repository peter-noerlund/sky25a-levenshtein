`default_nettype none
`timescale 1ns / 1ps

/* This testbench just instantiates the module and makes some convenient wires
   that can be driven / tested by the cocotb test.py.
*/
module tb ();
    // Dump the signals to a VCD file. You can view it with gtkwave.
    initial begin
        $dumpfile("tb.vcd");
        $dumpvars(0, tb);
        #1;
    end

    // Wire up the inputs and outputs:
    reg clk;
    reg rst_n;
    reg ena;
    reg [7:0] ui_in;
    reg [7:0] uio_in;
    wire [7:0] uo_out;
    wire [7:0] uio_out;
    wire [7:0] uio_oe;
`ifdef GL_TEST
    wire vpwr;
    wire vgnd;

    assign vpwr = 1'b1;
    assign vgnd = 1'b0;
`endif

    // Replace tt_um_example with your module name:
    tt_um_levenshtein user_project (

        // Include power ports for the Gate Level test:
`ifdef GL_TEST
        .VPWR(vpwr),
        .VGND(vgnd),
`endif

        .ui_in  (ui_in),    // Dedicated inputs
        .uo_out (uo_out),   // Dedicated outputs
        .uio_in (uio_in),   // IOs: Input path
        .uio_out(uio_out),  // IOs: Output path
        .uio_oe (uio_oe),   // IOs: Enable path (active high: 0=input, 1=output)
        .ena    (ena),      // enable - goes high when design is selected
        .clk    (clk),      // clock
        .rst_n  (rst_n)     // not reset
    );

    qspi_sram #(.VERBOSE(0)) pmod_sram(
        .sck(uio_out[3]),
        .ss_n(uio_out[0]),
        .sio_in({uio_out[5], uio_out[4], uio_out[2], uio_out[1]}),
        .sio_out({uio_in[5], uio_in[4], uio_in[2], uio_in[1]})
    );
endmodule
