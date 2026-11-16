-- R-9-2 positive: class 1 through ELSE-linked source IF with a constant action.
entity clock_control_class1_else is
    port(clk, rst, din : in bit; q1, q2 : out bit);
end entity;

architecture rtl of clock_control_class1_else is
begin
    source: process(clk, rst)
    begin
        if rst = '0' then
            q1 <= '0';
        else
            if rising_edge(clk) then
                q1 <= din;
            end if;
        end if;
    end process;

    control: process(clk, din)
    begin
        if clk then
            q2 <= din;
        end if;
    end process;
end architecture;
