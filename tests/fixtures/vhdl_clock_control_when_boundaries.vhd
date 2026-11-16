-- R-9-2 boundaries: WHEN does not recurse NOT or select the EQ right operand.
entity clock_control_when_boundaries is
    port(clk, en, din : in bit; q0, q1, q2 : out bit);
end entity;

architecture rtl of clock_control_when_boundaries is
begin
    source: process(clk)
    begin
        if rising_edge(clk) then
            q0 <= din;
        end if;
    end process;

    q1 <= din when not clk else '0';
    q2 <= din when en = clk else '0';
end architecture;
