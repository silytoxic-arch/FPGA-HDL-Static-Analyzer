entity shared_async_dynamic_action is
    port (clk, rst, data_in : in bit; q : out bit);
end entity;

architecture rtl of shared_async_dynamic_action is
begin
    p : process (rst, clk)
    begin
        if rst = '1' then
            q <= data_in;
        elsif rst = '0' then
            q <= '1';
        elsif rising_edge(clk) then
            q <= data_in;
        end if;
    end process;
end architecture;
