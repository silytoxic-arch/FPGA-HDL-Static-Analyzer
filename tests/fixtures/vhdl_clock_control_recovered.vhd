-- R-9-2 recovered positive forms: NOT, AND/OR and scalar equality.
entity clock_control_recovered is
    port(clk, en, din : in bit; q1, q2, q3 : out bit);
end entity;

architecture rtl of clock_control_recovered is
begin
    clock_source: process(clk)
    begin
        if rising_edge(clk) then
            q1 <= din;
        end if;
    end process;

    not_control: process(clk, din)
    begin
        if not clk then
            q1 <= din;
        end if;
    end process;

    compound_control: process(clk, en, din)
    begin
        if clk and en then
            q2 <= din;
        end if;
    end process;

    equality_control: process(clk, din)
    begin
        if clk = '1' then
            q3 <= din;
        end if;
    end process;
end architecture;
