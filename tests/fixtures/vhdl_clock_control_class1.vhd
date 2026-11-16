-- R-9-2 positive: relation class 1 has a constant asynchronous action.
entity clock_control_class1 is
    port(clk, rst, din : in bit; q1, q2 : out bit);
end entity;

architecture rtl of clock_control_class1 is
begin
    source: process(clk, rst)
    begin
        if rst = '0' then
            q1 <= '0';
        elsif rising_edge(clk) then
            q1 <= din;
        end if;
    end process;

    control: process(clk, din)
    begin
        if clk then
            q2 <= din;
        end if;
    end process;
end architecture;
