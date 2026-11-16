-- R-9-2 positive: direct and EQ-left clock controls in conditional assignments.
entity clock_control_when is
    port(clk, din : in bit; q0, q1, q2 : out bit);
end entity;

architecture rtl of clock_control_when is
begin
    source: process(clk)
    begin
        if rising_edge(clk) then
            q0 <= din;
        end if;
    end process;

    q1 <= din when clk else '0';
    q2 <= din when clk = '1' else '0';
end architecture;
