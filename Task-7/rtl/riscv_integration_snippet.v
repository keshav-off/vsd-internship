//=====================================================================
// riscv_integration_snippet.v
// Reference snippet showing exactly how timer_ip is wired into the
// VSDSquadron RISC-V SoC top-level (riscv.v). This is NOT a standalone
// module — copy the relevant blocks into your own riscv.v.
//=====================================================================

// ---------------------------------------------------------------
// 1. Address decode and wire declarations
// ---------------------------------------------------------------
localparam IO_TIMER_bit       = 4;
localparam TIMER_BASE_WADDR   = 30'h00100010;

wire        timer_sel   = isIO && (mem_wordaddr >= TIMER_BASE_WADDR) &&
                                    (mem_wordaddr <= TIMER_BASE_WADDR + 3);
wire        timer_wr_en = mem_wstrb;
wire        timer_rd_en = !mem_wstrb;
wire [1:0]  timer_addr  = mem_wordaddr[1:0];
wire [31:0] timer_rdata;
wire        timer_timeout;

// ---------------------------------------------------------------
// 2. Timer IP instantiation + LED connection
// ---------------------------------------------------------------
timer_ip TIMER (
    .clk       (clk),
    .resetn    (resetn),
    .sel       (timer_sel),
    .wr_en     (timer_wr_en),
    .rd_en     (timer_rd_en),
    .addr      (timer_addr),
    .wdata     (mem_wdata),
    .rdata     (timer_rdata),
    .timeout_o (timer_timeout)
);

always @(posedge clk) begin
    if (timer_timeout)
        LEDS[0] <= 1'b1;
end

// ---------------------------------------------------------------
// 3. IO_rdata mux — add timer_sel as a mux input
// ---------------------------------------------------------------
wire [31:0] IO_rdata =
    mem_wordaddr[IO_UART_CNTL_bit] ? { 22'b0, !uart_ready, 9'b0} :
    mem_wordaddr[IO_GPIO_bit]      ? gpio_rdata :
    timer_sel                     ? timer_rdata : 32'b0;
