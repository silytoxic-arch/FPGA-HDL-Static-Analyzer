-- R-9-2 boundary: ELSE-linked edge with a dynamic first action remains class 4.
entity clock_control_class4_else is
    port(clk, rst, din : in bit; q1, q2 : out bit);
end entity;

architecture rtl of clock_control_class4_else is
begin
    source: process(clk, rst, din)
    begin
        if rst = '0' then
            q1 <= din;
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
