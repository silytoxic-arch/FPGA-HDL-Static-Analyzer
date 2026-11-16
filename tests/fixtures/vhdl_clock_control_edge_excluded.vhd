-- R-9-2 negative boundary: an event-plus-level clock edge is not a MUX control.
entity clock_control_edge_excluded is
    port(clk, din : in bit; q : out bit);
end entity;

architecture rtl of clock_control_edge_excluded is
begin
    sequential_logic: process(clk)
    begin
        if clk'event and clk = '1' then
            q <= din;
        end if;
    end process;
end architecture;
