-- R-9-2 multiplicity: one condition map contains one normalized clock name.
entity clock_control_duplicate_name is
    port(clk, din : in bit; q1, q2 : out bit);
end entity;

architecture rtl of clock_control_duplicate_name is
begin
    source: process(clk)
    begin
        if rising_edge(clk) then
            q1 <= din;
        end if;
    end process;

    control: process(clk, din)
    begin
        if clk and CLK then
            q2 <= din;
        end if;
    end process;
end architecture;
