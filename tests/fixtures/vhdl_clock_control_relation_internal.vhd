-- R-9-2 boundary: class-1 and class-2 relation conditions exclude themselves.
entity clock_control_relation_internal is
    port(clk, din : in bit; q1, q2 : out bit);
end entity;

architecture rtl of clock_control_relation_internal is
begin
    class1_source: process(clk)
    begin
        if clk = '0' then
            q1 <= '0';
        elsif rising_edge(clk) then
            q1 <= din;
        end if;
    end process;

    class2_source: process(clk)
    begin
        if rising_edge(clk) then
            if clk = '0' then
                q2 <= din;
            end if;
        end if;
    end process;
end architecture;
