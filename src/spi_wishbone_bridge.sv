`default_nettype none

module spi_wishbone_bridge
    (
        input wire clk_i,
        input wire rst_i,

        input wire spi_sck,
        input wire spi_ss_n,
        input wire spi_mosi,
        output wire spi_miso,

        output wire cyc_o,
        output wire stb_o,
        output wire [22:0] adr_o,
        output wire [7:0] dat_o,
        output wire we_o,
        input wire ack_i,
        input wire err_i,
        input wire rty_i,
        input wire [7:0] dat_i
    );

    localparam STATE_COMMAND = 2'd0;
    localparam STATE_WISHBONE = 2'd1;
    localparam STATE_RESPONSE = 2'd2;

    reg [1:0] sck_sync;
    reg [1:0] mosi_sync;
    reg [1:0] ss_n_sync;
    reg miso;
    reg [31:0] buffer;
    reg cyc;
    reg last_sck;
    reg [1:0] state;
    reg [4:0] counter;
    
    wire sck;
    wire ss_n;
    wire mosi;

    assign sck = sck_sync[1];
    assign mosi = mosi_sync[1];
    assign ss_n = ss_n_sync[1];
    assign spi_miso = miso;
    assign cyc_o = cyc;
    assign stb_o = cyc;
    assign we_o = buffer[31];
    assign adr_o = buffer[30:8];
    assign dat_o = buffer[7:0];

    always @ (posedge clk_i) begin
        if (rst_i || ss_n) begin
            miso <= 1'b0;
            counter <= 5'd0;
            state <= STATE_COMMAND;
            buffer <= 32'h00000000;
            cyc <= 1'b0;
        end else begin
            if (state == STATE_COMMAND) begin
                if (!last_sck && sck) begin
                    buffer <= {buffer[30:0], mosi};
                    if (counter == 5'd31) begin
                        state <= STATE_WISHBONE;
                        cyc <= 1'b1;
                    end
                end
            end else if (state == STATE_WISHBONE) begin
                if (ack_i || err_i || rty_i) begin
                    buffer[8:0] <= {1'b1, dat_i};
                    cyc <= 1'b0;
                    state <= STATE_RESPONSE;
                end
            end else begin
                if (!last_sck && sck) begin
                    miso <= buffer[8];
                    buffer[8:0] <= {buffer[7:0], 1'b0};
                end
            end

            if (!last_sck && sck) begin
                counter <= counter + 5'd1;
            end
        end

        sck_sync <= {sck_sync[0], spi_sck};
        mosi_sync <= {mosi_sync[0], spi_mosi};
        ss_n_sync <= {ss_n_sync[0], spi_ss_n};
        last_sck <= sck;
    end
endmodule
