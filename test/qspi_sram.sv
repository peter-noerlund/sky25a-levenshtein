`default_nettype none

/* verilator lint_off UNUSEDSIGNAL */
module qspi_sram
    #(
        parameter CMD_WRITE_1S_1S_1S=8'h02,
        parameter CMD_READ_1S_1S_1S=8'h03,
        parameter CMD_WRITE_1S_4S_4S=8'h38,
        parameter CMD_READ_1S_4S_4S=8'hEB,
        parameter WAIT_CYCLES=6,
        parameter VERBOSE=1
    )
    (
        input wire sck,
        input wire ss_n,
        input wire [3:0] sio_in,
        output reg [3:0] sio_out
    );

    localparam STATE_COMMAND        = 4'd0;
    localparam STATE_ADDRESS        = 4'd1;
    localparam STATE_ADDRESS_QUAD   = 4'd2;
    localparam STATE_WAIT           = 4'd3;
    localparam STATE_READ           = 4'd4;
    localparam STATE_READ_QUAD      = 4'd5;
    localparam STATE_WRITE          = 4'd6;
    localparam STATE_WRITE_QUAD     = 4'd7;
    localparam STATE_FAIL           = 4'd8;

    wire mosi;

    wire [7:0] next_command;
    wire [23:0] next_address;
    wire [23:0] next_address_quad;
    wire [7:0] next_read_buffer;
    wire [7:0] next_read_buffer_quad;
    wire [7:0] next_write_buffer;
    wire [7:0] next_write_buffer_quad;

    reg [7:0] memory [16777215:0];

    reg [7:0] command;
    reg [23:0] address;
    reg [3:0] state;
    reg [4:0] counter;
    reg [7:0] read_buffer;
    reg [7:0] write_buffer;

    assign mosi = sio_in[0];
    assign next_command = {command[6:0], mosi};
    assign next_address = {address[22:0], mosi};
    assign next_address_quad = {address[19:0], sio_in};
    assign next_read_buffer = {read_buffer[6:0], mosi};
    assign next_read_buffer_quad = {read_buffer[3:0], sio_in};
    assign next_write_buffer = {write_buffer[6:0], mosi};
    assign next_write_buffer_quad = {write_buffer[3:0], sio_in};

    initial begin
        state = STATE_COMMAND;
        counter = 5'd0;
`ifdef VERILATOR
        sio_out = 4'b0000;
`else // VERILATOR
        sio_out = 4'bzzzz;
`endif // VERILATOR
    end

    always @ (posedge sck, negedge sck, posedge ss_n, negedge ss_n) begin
        if (ss_n) begin
            state <= STATE_COMMAND;
            counter <= 5'd0;
`ifdef VERILATOR
            sio_out <= 4'b0000;
`else // VERILATOR
            sio_out <= 4'bzzzz;
`endif // VERILATOR
        end else if (!ss_n) begin
            case (state)
                STATE_COMMAND: begin
                    if (sck) begin
                        command <= next_command;
                        if (counter == 5'd7) begin
                            if (next_command == CMD_READ_1S_1S_1S || next_command == CMD_WRITE_1S_1S_1S) begin
                                state <= STATE_ADDRESS;
                            end else if (next_command == CMD_READ_1S_4S_4S || next_command == CMD_WRITE_1S_4S_4S) begin
                                state <= STATE_ADDRESS_QUAD;
                            end else begin
                                state <= STATE_FAIL;
                            end
                            counter <= 5'd0;
                        end else begin
                            counter <= counter + 5'd1;
                        end
                    end
                end

                STATE_ADDRESS: begin
                    if (sck) begin
                        address <= next_address;
                        if (counter == 5'd23) begin
                            if (command == CMD_READ_1S_1S_1S) begin
                                state <= STATE_READ;
                                read_buffer <= memory[next_address];
                            end else begin
                                state <= STATE_WRITE;
                            end
                            counter <= 5'd0;
                        end else begin
                            counter <= counter + 5'd1;
                        end
                    end
                end

                STATE_ADDRESS_QUAD: begin
                    if (sck) begin
                        address <= next_address_quad;
                        if (counter == 5'd5) begin
                            if (command == CMD_READ_1S_4S_4S) begin
                                state <= STATE_WAIT;
                                read_buffer <= memory[next_address_quad];
                            end else begin
                                state <= STATE_WRITE_QUAD;
                            end
                            counter <= 5'd0;
                        end else begin
                            counter <= counter + 5'd1;
                        end
                    end
                end

                STATE_WAIT: begin
                    if (sck) begin
                        if (counter == 5'(WAIT_CYCLES - 1)) begin
                            state <= STATE_READ_QUAD;
                            counter <= 5'd0;
                        end else begin
                            counter <= counter + 5'd1;
                        end
                    end
                end

                STATE_READ: begin
                    if (!sck) begin
`ifdef VERILATOR
                        sio_out <= {2'b00, read_buffer[7], 1'b0};
`else // VERILATOR
                        sio_out <= {2'bzz, read_buffer[7], 1'bz};
`endif // VERILATOR
                        read_buffer <= next_read_buffer;
                        if (counter == 5'd0) begin
                            address <= address + 24'd1;
                        end
                        if (counter == 5'd7) begin
                            if (VERBOSE) begin
                                $display("R [%06X] => %02X", address - 1, memory[address - 1]);
                            end
                            read_buffer <= memory[address];
                            counter <= 5'd0;
                        end else begin
                            counter <= counter + 5'd1;
                        end
                    end
                end

                STATE_READ_QUAD: begin
                    if (!sck) begin
                        sio_out <= read_buffer[7:4];
                        read_buffer <= next_read_buffer_quad;
                        if (counter == 5'd0) begin
                            counter <= 5'd1;
                        end else begin
                            if (VERBOSE) begin
                                $display("R [%06X] => %02X", address, memory[address]);
                            end
                            read_buffer <= memory[address + 24'd1];
                            address <= address + 24'd1;
                            counter <= 5'd0;
                        end
                    end
                end

                STATE_WRITE: begin
                    if (sck) begin
                        write_buffer <= next_write_buffer;
                        if (counter == 5'd7) begin
                            if (VERBOSE) begin
                                $display("W [%06X] <= %02X", address, next_write_buffer);
                            end
                            memory[address] <= next_write_buffer;
                            address <= address + 24'd1;
                            counter <= 5'd0;
                        end else begin
                            counter <= counter + 5'd1;
                        end
                    end
                end

                STATE_WRITE_QUAD: begin
                    if (sck) begin
                        write_buffer <= next_write_buffer_quad;
                        if (counter == 5'd1) begin
                            if (VERBOSE) begin
                                $display("W [%06X] <= %02X", address, next_write_buffer_quad);
                            end
                            memory[address] <= next_write_buffer_quad;
                            address <= address + 24'd1;
                            counter <= 5'd0;
                        end else begin
                            counter <= 5'd1;
                        end
                    end
                end

                default: begin
                    
                end
            endcase
        end
    end    
endmodule
